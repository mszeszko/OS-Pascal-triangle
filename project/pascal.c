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

// Pipe naming is fit to original Pascal process pipes,
// so that 'readPipe' the one that is used by Pascal to read messages from.
void closeWorkerDescriptors(int* readPipe, int* writePipe) {
	if (close(readPipe[0]) == -1)
		syserr("Initial worker, readPipe[0].\n");
	if (close(readPipe[1]) == -1)
		syserr("Initial worker, readPipe[0].\n");
	if (close(writePipe[0]) == -1)
		syserr("Initial worker, readPipe[0].\n");
	if (close(writePipe[1]) == -1)
		syserr("Initial worker, readPipe[0].\n");
}

void reassignWorkerDescriptor(int descriptor, int* pipe) {
	if (close(descriptor) == -1)
		syserr("Error while closing descriptor: %d, FIRST worker.\n", descriptor);
	if (dup(pipe[descriptor]) != descriptor)
		syserr("Descriptor: %d not overriden, FIRST worker.\n", descriptor);
}

void readNPrintData(const int readDsc) {
	const int structSize = sizeof(struct TriangleCeofficient);
	struct TriangleCeofficient dataMsg;
	const int coeffSize = sizeof(dataMsg.ceofficient);

	int readDataMsg;
	int writeDataMsg;
	// Read data portions till EOF will be detected.
	while (readDataMsg = read(readDsc, &dataMsg, structSize)) {
		// Error occured.
		if (readDataMsg == -1)
			syserr("Error while reading triangle results, Pascal\n");
		
		// By default `structSize` bytes should be read.
		if (readDataMsg != structSize)
			syserr("Illegal struct size while reading triangle results in Pascal\n");
		
		// Print ceofficient to STDOUT.
		writeDataMsg = write(STDOUT_FILENO, &(dataMsg.ceofficient), ceoffSize);
		
		// Handle write message..
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
}

void readConfirmationSymbol(const int readDsc) {
	struct ConfirmationMsg confirmationMsg;
	if (read(readDsc, &confirmationMsg, sizeof(struct ConfirmationMsg)) == -1)
		syserr("Error while reading Worker response.\n");
	if (confirmationMsg.result = success)
		syserr("Unknown job confirmation response.\n");
}

void prepareNextRequestMsg(enum Token token, const int workersLeft,
	struct RequestMsg* requestMsg) {
	requestMsg->token = token;
	requestMsg->workersLeft = workersLeft;
	requestMsg->previousCeofficient = 1;
}

void readWorkerResponse(const int readDsc, enum PascalMode* mode,
	struct RequestMsg* requestMsg, int* workers, const int maxWorkers) {

	switch (*mode) {
		case init:
			readConfirmationSymbol(readDsc);
			*mode = compute;
			prepareNextRequestMsg(compute,
														1,
														requestMsg);
			break;
		case compute:
			readConfirmationSymbol(readDsc);
			if (*workers == maxWorkers) {
				*mode = gatherResults;
				prepareNextRequestMsg(gatherResults,
															*workers,
															requestMsg);
			} else {	
				prepareNextRequestMsg(compute,
															requestMsg->workersLeft + 1,
															requestMsg);
			}
			break;
		case gatherResults:
			readNPrintData(readDsc);
			*mode = waitAndClose;
			prepareNextRequestMsg(waitAndClose,
														*workers,
														requestMsg);
			break;
		case waitAndClose:
			readConfirmationSymbol();
			break;
	}  // switch(mode)
}

int main(int argc, char* argv[]) {
	// Usage info in case of invalid parameters number.
	if (argc != 2)
		fatal("Usage: %s <number of row in Pascals' Triangle>\n", argv[0]);
	
	// Eliminate requests with illegal row number.
	const int targetRow = atoi(argv[1]);
	if (targetRow <= 0)
		fatal("Row number should be expressed with positive value!\n");

	const int maxWorkers = targetRow;
	int workers = maxWorkers;

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
	prepareNextRequestMsg(init, workers, &requestMsg);

	switch (fork()) {
		case -1:
			syserr("Error in fork, Pascal\n");
		case 0:
			// Prepare first worker process.
			reassignWorkerDescriptor(STDOUT_FILENO, &readPipe);
			reassignWorkerDescriptor(STDIN_FILENO, &writePipe);
			
			// Close pipes in worker process - free resources.
			closeWorkerDescriptors(&readPipe, &writePipe);
			
			execl("./worker", "worker", argv[1]);
			syserr("Error in execl\n");
		default:
			preparePascalPipes(&readPipe, &writePipe);
			while (true) {
				// Write directly to the FIRST Worker process.
				if (write(writeDsc, &requestMsg, sizeof(struct RequestMsg)) == -1)
					syserr("Error while sending request message to initial Worker.\n");

				// Read worker response.
				readWorkerResponse(readDsc, &mode, &requestMsg, &workers, targetRow);

				// Wait for Worker process.
				if (mode == waitAndClose) {
					if (wait(NULL) == -1)
						syserr("Error in child process, PASCAL.\n");
					break;
				}
			}
			closePascalDescriptors(&readPipe, &writePipe);
	}  // switch(fork())
	
	exit(EXIT_SUCCESS);
}
