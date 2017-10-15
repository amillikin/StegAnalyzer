/********************************************************************************************
*										StegAnalyzer.cpp 									*
*																							*
*	DESCRIPTION: Takes a wave file and attempts to detect steganographic properties			*
*				 Input Parameters: StegAnalyzer <infile> <outfile>							*
*																							*
*																							*
*	AUTHOR: Aaron Millikin											START DATE: 9/21/2017	*
*********************************************************************************************/
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

vector<unsigned char> secretMsg;

// "RIFF" chunk descriptor
struct mstrChnk {
	char chunkID[4];              //BE: "RIFF" in ASCII (0x52494646)
	unsigned long chunkSize;      //LE: 36 + subChunk2Size -> (4 + (8 + subChunk1Size) + (8 + subChunk2Size)
	char format[4];               //BE: "WAVE" in ASCII (0x57415645)
};

// "fmt" sub-chunk: Describes the sound data's format
struct fmtChnk {
	char subChunk1ID[4];          //BE: "fmt" in ASCII (0x666d7420)
	unsigned long subChunk1Size;  //LE: Size of rest of subchunk following this value. (16 if PCM)
	unsigned short audioFormat;   //LE: Indicates forms of compression (1:PCM,6:mulaw,7:alaw,257:IBM Mu-Law,258:IBM A-Law,259:ADPCM)
	unsigned short numChannels;   //LE: Number of Channels: Mono=1, Stereo=2, etc.
	unsigned long sampleRate;     //LE: Sample Rate: CD commonly = 44100
	unsigned long byteRate;       //LE: sampleRate*numChannels*bitsPerSample/8
	unsigned short blockAlign;    //LE: numChannels*bitsPerSample/8
	unsigned short bitsPerSample; //LE: 8=8bit, 16=16bit, etc.
};

// "data" sub-chunk: Contains size of data and actual sound data
struct dataChnk {
	char subChunk2ID[4];          //BE: "data" (0x64617461)
	unsigned long subChunk2Size;  //LE: numSamples*numChannels*bitsPerSample/8 (size of actual sound data following this value)
	                              // LE: Remaining data in file is raw sound data of size specified by subChunk2Size
};

// Assuming 2 Channels, 2 Byte samples
struct sample {
	signed short left;
	signed short right;
};

//Handles parts necessary to create a full byte from only sample LSBs
struct boStruct {
	sample s;
	int bitCnt;
	unsigned char byteOut;
};

//LSB in 1st ch parameter is shifted in first, followed by the second
unsigned char createByte2CH(int shift, short ch1in, short ch2in, unsigned char byteOut) {
	ch1in = (ch1in & 0x01) << (0 + 2 * shift);
	ch2in = (ch2in & 0x01) << (1 + 2 * shift);
	byteOut |= ch1in | ch2in;
	return byteOut;
}

//LSB in channel shifted into correct position
unsigned char createByte1CH(int shift, short chIn, unsigned char byteOut) {
	byteOut |= ((chIn & 0x01) << (2 * shift));
	return byteOut;
}

boStruct doLR(boStruct bo) {
	//Shift bits into byte to be output
	bo.byteOut = createByte2CH(bo.bitCnt, bo.s.left, bo.s.right, bo.byteOut);
	 
	//If we reach the final bit of a byte, add byte to vector, 
	//clear byteOut, and reset bit counter, otherwise, incremenet bit counter
	if (bo.bitCnt == 3) {
		secretMsg.push_back(bo.byteOut);
		bo.byteOut = NULL;
		bo.bitCnt = 0;
	}
	else {
		bo.bitCnt++;
	}
	return bo;
}

boStruct doRL(boStruct bo) {
	//Shift bits into byte to be output
	bo.byteOut = createByte2CH(bo.bitCnt, bo.s.right, bo.s.left, bo.byteOut);

	//If we reach the final bit of a byte, add byte to vector, 
	//clear byteOut, and reset bit counter, otherwise, incremenet bit counter
	if (bo.bitCnt == 3) {
		secretMsg.push_back(bo.byteOut);
		bo.byteOut = NULL;
		bo.bitCnt = 0;
	}
	else {
		bo.bitCnt++;
	}
	return bo;
}

boStruct doL(boStruct bo) {
	//Shift bits into byte to be output
	bo.byteOut = createByte1CH(bo.bitCnt, bo.s.left, bo.byteOut);

	//If we reach the final bit of a byte, add byte to vector, 
	//clear byteOut, and reset bit counter, otherwise, incremenet bit counter
	if (bo.bitCnt == 7) {
		secretMsg.push_back(bo.byteOut);
		bo.byteOut = NULL;
		bo.bitCnt = 0;
	}
	else {
		bo.bitCnt++;
	}
	return bo;
}

boStruct doR(boStruct bo) {
	//Shift bits into byte to be output
	bo.byteOut = createByte1CH(bo.bitCnt, bo.s.right, bo.byteOut);

	//If we reach the final bit of a byte, add byte to vector, 
	//clear byteOut, and reset bit counter, otherwise, incremenet bit counter
	if (bo.bitCnt == 7) {
		secretMsg.push_back(bo.byteOut);
		bo.byteOut = NULL;
		bo.bitCnt = 0;
	}
	else {
		bo.bitCnt++;
	}
	return bo;
}

boStruct doAlt(boStruct bo, bool leftCh) {
	//Shifts bits from current channel into byte to be output
	if (leftCh) {
		bo.byteOut = createByte1CH(bo.bitCnt, bo.s.left, bo.byteOut);
	}
	else {
		bo.byteOut = createByte1CH(bo.bitCnt, bo.s.right, bo.byteOut);
	}

	//If we reach the final bit of a byte, add byte to vector, 
	//clear byteOut, and reset bit counter, otherwise, incremenet bit counter
	if (bo.bitCnt == 7) {
		secretMsg.push_back(bo.byteOut);
		bo.byteOut = NULL;
		bo.bitCnt = 0;
	}
	else {
		bo.bitCnt++;
	}
	return bo;
}

//Converts a string to all uppercase characters
string upCase(string str) {
	transform(str.begin(), str.end(), str.begin(), toupper);
	return str;
}

//Checks valid mode
bool validMode(string mode) {
	if (mode == "L" || mode == "R" || mode == "LR" || mode == "RL") {
		return true;
	}
	else {
		cout << "Not a valid mode." << endl;
		return false;
	}
}

void prompt()
{
	cout << "Welcome to Aaron's Steganography Analyzer." << endl;
	cout << "Accepted input: StegAnalyzer <mode> <infile> <outfile>" << endl;
	cout << "Accepted modes: L R LR RL (CH LSB is pulled from)" << endl;
}
int main(int argc, char* argv[]) {
	clock_t startTime = clock();
	double secondsElapsed;
	errno_t err;
	ifstream inFile;
	ofstream outFile;
	struct mstrChnk masterChunk;
	struct fmtChnk fmtChunk;
	struct dataChnk dataChunk;
	struct sample sample;
	struct boStruct bo;
	string mode;
	int fileSize;
	int secretSize;
	//int format, numCh, sampRate, byteRate, blockAlign, bps, soundSize;

	if (argc != 4) {
		cout << "Incorrect number of arguments supplied." << endl;
		prompt();
		return 1;
	}


	inFile.open(argv[2], ios::in | ios::binary);
	if (!inFile) {
		cout << "Can't open input file " << argv[1] << endl;
		prompt();
		return 1;
	}

	outFile.open(argv[3], ios::out | ios::binary);
	if (!outFile) {
		cout << "Can't open output file " << argv[2] << endl;
		prompt();
		return 1;
	}

	//Get Filesize
	fileSize = 0;
	inFile.seekg(0, ios::end);
	fileSize = inFile.tellg();
	inFile.seekg(0, ios::beg);

	//Read file 
	inFile.read((char *)&masterChunk, sizeof(masterChunk));
	inFile.read((char *)&fmtChunk, sizeof(fmtChunk));
	for (int i = 0; i < (fmtChunk.subChunk1Size - 16); i++) {
		inFile.get(); //Extra stuff in format header.. I'll determine if I want to save this later
	}
	inFile.read((char *)&dataChunk, sizeof(dataChunk));
	
	/*
	//Output data to file
	outFile << "File: " << argv[1] << endl;
	outFile << "Filesize: " << fileSize << endl;
	outFile << "Format: " << fmtChunk.audioFormat << endl;
	outFile << "Number of Channels: " << fmtChunk.numChannels << endl;
	outFile << "Sample Rate: " << fmtChunk.sampleRate << endl;
	outFile << "Byte Rate: " << fmtChunk.byteRate << endl;
	outFile << "Block Align: " << fmtChunk.blockAlign << endl;
	outFile << "Bits Per Sample: " << fmtChunk.bitsPerSample << endl;
	outFile << "Sound Data Size: " << dataChunk.subChunk2Size << endl;
	outFile << "Song Duration: " << dataChunk.subChunk2Size / fmtChunk.blockAlign / fmtChunk.sampleRate << endl;
	*/

	secretSize = dataChunk.subChunk2Size / fmtChunk.numChannels;
	
	bo.bitCnt = 0;
	bo.byteOut = NULL;

	//Stepping through sound data one sample at a time
	//The following shifts LSB of each sample into 
	//position of the byte being constructed 

	if (mode == "LR") {
		//byteOut = {S0L,S0R,S1L,S1R,S2L,S2R,S3L,S3R}... 
		for (int i = 0; i < dataChunk.subChunk2Size / fmtChunk.blockAlign; i++) {
			inFile.read((char *)&bo.s, sizeof(bo.s));
			bo = doLR(bo);
		}
	}

	else if (mode == "RL") { 
		//byteOut = { S0R,S0L,S1R,S1L,S2R,S2L,S3R,S3L }...
		for (int i = 0; i < dataChunk.subChunk2Size / fmtChunk.blockAlign; i++) {
			inFile.read((char *)&bo.s, sizeof(bo.s));
			bo = doRL(bo);
		}
	}
	//byteOut = { S0L,S1L,S2L,S3L,S4L,S5L,S6L,S7L }...
	else if (mode == "L") {
		
		for (int i = 0; i < dataChunk.subChunk2Size / fmtChunk.blockAlign; i++) {
			inFile.read((char *)&bo.s, sizeof(bo.s));
			bo = doL(bo);
		}
	}
	//byteOut = { S0R,S1R,S2R,S3R,S4R,S5R,S6R,S7R }...
	else if (mode == "R") {
		//Stepping through sound data one sample at a time and formatting for left and right channels
		for (int i = 0; i < dataChunk.subChunk2Size / fmtChunk.blockAlign; i++) {
			inFile.read((char *)&bo.s, sizeof(bo.s));
			bo = doR(bo);
		}
	}
	//Alternates until a full byte is completed L then R
	else if (mode == "LRA"){
		bool isL = true;
		//Stepping through sound data one sample at a time and formatting for left and right channels
		for (int i = 0; i < dataChunk.subChunk2Size / fmtChunk.blockAlign; i++) {
			inFile.read((char *)&bo.s, sizeof(bo.s));
			if (bo.bitCnt == 7) {
				bo = doAlt(bo, isL);
				isL = !isL;
			}
			else {
				bo = doAlt(bo, isL);
			}
		}
	}
	//Alternates until a full byte is completed R then L
	else if (mode == "RLA") {
		bool isL = false;
		//Stepping through sound data one sample at a time and formatting for left and right channels
		for (int i = 0; i < dataChunk.subChunk2Size / fmtChunk.blockAlign; i++) {
			inFile.read((char *)&bo.s, sizeof(bo.s));
			if (bo.bitCnt == 7) {
				bo = doAlt(bo, isL);
				isL = !isL;
			}
			else {
				bo = doAlt(bo, isL);
			}
		}
	}

	//Outputs a file of all characters generated, can create a skip counter if I need to pop without outputting.
	while (secretMsg.begin() != secretMsg.end()) {
		if (isalnum(secretMsg.back())) {
			outFile << secretMsg.back();
		}
		secretMsg.pop_back();
	}

	inFile.close();
	outFile.close();

	secondsElapsed = (clock() - startTime) / CLOCKS_PER_SEC;
	cout << secondsElapsed << endl;

	return 0;
}
