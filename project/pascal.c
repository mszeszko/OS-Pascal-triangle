/*
			Maciej Szeszko,
			id: ms335814,
			University of Warsaw
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "constants.h"
#include "err.h"


void preparePipes(int* readPipe, int* writePipe) {
	if (close(readPipe[1]) == -1)
		syserr("Error in preparing read pipe, Pascal\n");
	if (close(writePipe[0]) == -1) {
		syserr("Error in preparing write pipe, Pascal\n");
	}
}

void initPipes(int* readPipe, int* writePipe) {
	if (pipe(readPipe) == -1)
		syserr("Error in reading pipe, Pascal\n");
	if (pipe(writePipe) == -1)
		syserr("Error in writing pipe, Pascal\n");
}

void closeDescriptors(int* readPipe, int* writePipe) {
	if (close(readPipe[0]) == -1)
		syserr("Error in closing read pipe, Pascal\n");
	if (close(writePipe[1]) == -1) {
		syserr("Error in closing write pipe, Pascal\n");
	}
}

void printData(const int readDsc) {
	const int structSize = sizeof(struct TriangleCeofficient);
	struct TriangleCeofficient dataMsg;
	const int coeffSize = sizeof(dataMsg.ceofficient);

	int readDataMsg;
	int writeDataMsg;
	while (readDataMsg = read(readDsc, &dataMsg, structSize)) {
		// Check read msg.
		if (readDataMsg == 0)
			break;
		if (readDataMsg == -1)
			syserr("Error while reading triangle results, Pascal\n");
		
		// By default `sizeOfStructure` bytes should be read.
		if (readDataMsg != structSize)
			syserr("Illegal structure size while readin triangle results, Pascal\n");
		
		writeDataMsg = write(STDOUT_FILENO, &(dataMsg.ceofficient), ceoffSize);
		
		switch (writeDataMsg) {
			case -1:
				syserr("Error while writing triangle results to STDOUT\n");
			case 0:
				syserr("Written 0 bytes to STODUT, Pascal\n");
			case ceoffSize:
				// OK
				break;
			default:
				// Exactly `coeffSize` bytes should be written to STDOUT.
				syserr("Written unexpected number of bytes to STDOUT, Pascal\n");
		}
}

int main(int argc, char* argv[]) {
	// TODO(mszeszko): Usage info in case of invalid parameters.

	// Create and initialize pipes.
	int readPipe[2];
	int writePipe[2];
	
	initPipes(&readPipe, &writePipe);

	// Set flags and constants.
	enum PascalMode mode = initChain;
	enum Token token = init;
	const int writeDsc = writePipe[1];
	const int readDsc = readPipe[0];

	switch (fork()) {
		case -1:
			syserr("Error in fork, Pascal\n");
		case 0:
			break;
		default:
			preparePipes(&readPipe, &writePipe);

			while (true) {
				// TODO(mszeszko): Communication with first process - Worker1.
			}
			
			printData(readDsc);
			closePipes(&readPipe, &writePipe);
	}
	
	exit(EXIT_SUCCESS);
}
