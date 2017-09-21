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
using namespace std;

struct mstrChnk {
	// "RIFF" chunk descriptor
	char chunkID[4];                 //BE: "RIFF" in ASCII (0x52494646)
	unsigned long chunkSize;         //LE: 36 + subChunk2Size -> (4 + (8 + subChunk1Size) + (8 + subChunk2Size)
	char format[4];                  //BE: "WAVE" in ASCII (0x57415645)
};
struct fmtChnk {
	// "fmt" sub-chunk: Describes the sound data's format
	char subChunk1ID[4];             //BE: "fmt" in ASCII (0x666d7420)
	unsigned long subChunk1Size;     //LE: Size of rest of subchunk following this value. (16 if PCM)
	unsigned short audioFormat;   //LE: Indicates forms of compression (1:PCM,6:mulaw,7:alaw,257:IBM Mu-Law,258:IBM A-Law,259:ADPCM)
	unsigned short numChannels;   //LE: Number of Channels: Mono=1, Stereo=2, etc.
	unsigned long sampleRate;     //LE: Sample Rate: CD commonly = 44100
	unsigned long byteRate;       //LE: sampleRate*numChannels*bitsPerSample/8
	unsigned short blockAlign;    //LE: numChannels*bitsPerSample/8
	unsigned short bitsPerSample; //LE: 8=8bit, 16=16bit, etc.
};
struct dataChnk {
	// "data" sub-chunk: Contains size of data and actual sound data
	char subChunk2ID[4];             //BE: "data" (0x64617461)
	unsigned long subChunk2Size;  //LE: numSamples*numChannels*bitsPerSample/8 (size of actual sound data following this value)
	                                 // LE: Remaining data in file is raw sound data of size specified by subChunk2Size
};

void prompt()
{
	cout << "Welcome to Aaron's Steganography Analyzer." << endl;
	cout << "Accepted input: StegAnalyzer <infile> <outfile>" << endl;
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
	int fileSize;
	int format, numCh, sampRate, byteRate, blockAlign, bps, soundSize;

	if (argc != 3) {
		cout << "Incorrect number of arguments supplied." << endl;
		prompt();
		return 1;
	}

	inFile.open(argv[1], ios::in | ios::binary);
	if (!inFile) {
		cout << "Can't open input file " << argv[4] << endl;
		prompt();
		return 1;
	}

	outFile.open(argv[2], ios::out);
	if (!outFile) {
		cout << "Can't open output file " << argv[5] << endl;
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
	
	inFile.close();
	outFile.close();

	secondsElapsed = (clock() - startTime) / CLOCKS_PER_SEC;
	cout << secondsElapsed << endl;

	return 0;
}
