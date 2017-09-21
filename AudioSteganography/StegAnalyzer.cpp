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
ifstream inFile;
ofstream outFile;

struct waveFile {
	// "RIFF" chunk descriptor
	char chunkID[4];
	char chunkSize[4];
	char format[4];
	// "fmt" sub-chunk
	char subChunk1ID[4];
	char subChunk1Size[4];
	char audioFormat[2];
	char numChannels[2];
	char sampleRate[4];
	char byteRate[4];
	char blockAlign[2];
	char bitsPerSample[2];
	// "data" sub-chunk
	char subChunk2ID[4];
	char subChunk2Size[4];
	// Remaining data in file is raw sound data of size specified by subChunk2Size
};

void prompt()
{
	cout << "Welcome to Aaron's Steganography Analyzer." << endl;
	cout << "Accepted input: StegAnalyzer <infile> <outfile>" << endl;
}
int main(int argc, char* argv[]) {
	clock_t startTime = clock();
	double secondsElapsed;
	char freqMode;
	errno_t err;

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

	freqMode = argv[1][1];
	switch (freqMode) {
	case 's': cout << "Analyzing bits..." << endl;
		analyzeSingleBits();
		break;
	case 'b': cout << "Analyzing bytes..." << endl;
		analyzeSingleBytes();
		break;
	case 'd': cout << "Analyzing digrams..." << endl;
		analyzeDigrams();
		break;
	case 't': cout << "Analyzing trigrams..." << endl;
		analyzeTrigrams();
		break;
	case 'o': cout << "Analyzing octets..." << endl;
		analyzeOctets();
		break;
	default: cout << "Modes: single-bit (0), single-Byte (1), digram (2), trigram (3), octet (8)" << endl;
		return 1;
	}

	inFile.close();
	outFile.close();

	secondsElapsed = (clock() - startTime) / CLOCKS_PER_SEC;
	cout << secondsElapsed << endl;

	return 0;
}
