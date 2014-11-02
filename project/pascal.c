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


void preparePascalPipes(int* readPipe, int* writePipe) {
	if (close(readPipe[1]) == -1)
		syserr("Error in preparing read pipe, Pascal\n");
	if (close(writePipe[0]) == -1) {
		syserr("Error in preparing write pipe, Pascal\n");
	}
}

void initPascalPipes(int* readPipe, int* writePipe) {
	if (pipe(readPipe) == -1)
		syserr("Error in reading pipe, Pascal\n");
	if (pipe(writePipe) == -1)
		syserr("Error in writing pipe, Pascal\n");
}

void closePascalDescriptors(int* readPipe, int* writePipe) {
	if (close(readPipe[0]) == -1)
		syserr("Error in closing read pipe, Pascal\n");
	if (close(writePipe[1]) == -1) {
		syserr("Error in closing write pipe, Pascal\n");
	}
}

void readNPrintData(const int readDsc) {
	const int structSize = sizeof(struct TriangleCeofficient);
	struct TriangleCeofficient dataMsg;
	const int coeffSize = sizeof(dataMsg.ceofficient);

	int readDataMsg;
	int writeDataMsg;
	while (readDataMsg = read(readDsc, &dataMsg, structSize)) {
		// EOF, data has been properly printed.
		if (readDataMsg == 0)
			break;
		// Error occured.
		if (readDataMsg == -1)
			syserr("Error while reading triangle results, Pascal\n");
		
		// By default `sizeOfStructure` bytes should be read.
		if (readDataMsg != structSize)
			syserr("Illegal structure size while reading triangle results, Pascal\n");
		
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

void reassignWorkerDescriptor(int descriptor, int* pipe) {
	if (close(descriptor) == -1)
		syserr("Error while closing descriptor: %d, FIRST worker.\n", descriptor);
	if (dup(pipe[descriptor]) != descriptor)
		syserr("Descriptor: %d not overriden, FIRST worker.\n", descriptor);
}

void setNextRequestMsg(enum Token token, const int workersLeft,
	struct RequestMsg* requestMsg) {
	requestMsg->token = token;
	requestMsg->workersLeft = workersLeft;
}

void readConfirmationSymbol(const int readDsc) {
	char result;
	if (read(readDsc, &result, sizeof(char)) == -1)
		syserr("Error while reading Worker response.\n");
	if (result != success)
		syserr("Unknown confirmation symbol.\n");
}

void readWorkerResponse(const int readDsc, enum PascalMode* mode,
	struct RequestMsg* requestMsg, int* workers, const int maxWorkers) {

	switch (*mode) {
		case init:
			readConfirmationSymbol(readDsc);
			*mode = commitComputation;
			setNextRequestMsg(compute, 1);
			break;
		case compute:
			readConfirmationSymbol(readDsc);
			if (*workers == maxWorkers)
				*mode = gatherResults;
			else	
				setNextRequestMsg(compute, requestMsg->workersLeft + 1);
			break;
		case gatherResults:
			readNPrintData(readDsc);
			*mode = waitAndClose;
			break;
		/*case waitAndClose:
			readConfirmationSymbol();*/

	}  // switch(mode)
int main(int argc, char* argv[]) {
	// Usage info in case of invalid parameters number.
	if (argc != 2)
		fatal("Usage: %s <number of row in Pascals' Triangle>\n", argv[0]);
	
	// Eliminate requests with illegal row number.
	const int targetRow = atoi(argv[1]);
	if (targetRow <= 0)
		fatal("Row number should be expressed with positive value!\n");

	int workers = targetRow;

	// Create and initialize pipes.
	int readPipe[2];
	int writePipe[2];
	
	initPascalPipes(&readPipe, &writePipe);

	// Set flags and constants.
	enum PascalMode mode = initChain;
	enum Token token = init;
	const int writeDsc = writePipe[1];
	const int readDsc = readPipe[0];

	// Initialize first Pascals' request.
	struct RequestMsg requestMsg;
	setNextRequestMsg(init, workers);

	switch (fork()) {
		case -1:
			syserr("Error in fork, Pascal\n");
		case 0:
			// Prepare first worker process.
			reassignWorkerDescriptor(STDOUT_FILENO, &readPipe);
			reassignWorkerDescriptor(STDIN_FILENO, &writePipe);
			// Close pipes in worker process - free resources.
			if (close(readPipe[0]) == -1)
				syserr("Initial worker, readPipe[0].\n");
			if (close(readPipe[1]) == -1)
				syserr("Initial worker, readPipe[0].\n");
			if (close(writePipe[0]) == -1)
				syserr("Initial worker, readPipe[0].\n");
			if (close(writePipe[1]) == -1)
				syserr("Initial worker, readPipe[0].\n");

			execl("./worker", "worker", argv[1]);
			syserr("Error in execl\n");
		default:
			preparePascalPipes(&readPipe, &writePipe);
			while (true) {
				// Write directly to the FIRST Worker process.
				if (write(writeDsc, &requestMsg, sizeof(struct RequestMsg)) == -1)
					syserr("Error while sending request message to initial Worker.\n");
				// Read worker response.
				readWorkerResponse(....);

				if (mode == każ im się ubić) {
				}
			}
			
			printData(readDsc);
			closePascalDescriptors(&readPipe, &writePipe);
	}
	
	exit(EXIT_SUCCESS);
}
