/*
			Maciej Szeszko,
			id: ms335814,
			University of Warsaw
*/

#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "constants.h"
#include "err.h"


void prepareParentPipes(int* readPipe, int* writePipe) {
	if (close(readPipe[1]) == -1)
		syserr("Error in preparing read pipe, Pascal\n");
	if (close(writePipe[0]) == -1) {
		syserr("Error in preparing write pipe, Pascal\n");
	}
}

void initParentPipes(int* readPipe, int* writePipe) {
	if (pipe(readPipe) == -1)
		syserr("Error in reading pipe, Pascal\n");
	if (pipe(writePipe) == -1)
		syserr("Error in writing pipe, Pascal\n");
}

void closeParentDescriptors(int readDsc, int writeDsc) {
	if (close(readDsc) == -1)
		syserr("Error in closing read pipe, Pascal\n");
	if (close(writeDsc) == -1) {
		syserr("Error in closing write pipe, Pascal\n");
	}
}

void closeOverridenStandardDescriptors() {
	if (close(STDIN_FILENO) == -1)
		syserr("Error in closing STDIN descriptor.\n");
	if (close(STDOUT_FILENO) == -1)
		syserr("Error in closing STDOUT descriptor.\n");
}

/* Pipe naming is fit to PARENT process pipes,
	 so that 'readPipe' the one that is used by PARENT to read messages from. */
void closeUnusedChildDescriptors(int* readPipe, int* writePipe) {
	if (close(readPipe[0]) == -1)
		syserr("Initial worker, readPipe[0].\n");
	if (close(readPipe[1]) == -1)
		syserr("Initial worker, readPipe[0].\n");
	if (close(writePipe[0]) == -1)
		syserr("Initial worker, readPipe[0].\n");
	if (close(writePipe[1]) == -1)
		syserr("Initial worker, readPipe[0].\n");
}

void reassignChildDescriptor(int descriptor, int* pipe) {
	if (close(descriptor) == -1)
		syserr("Error while closing descriptor: %d in Worker.\n", descriptor);
	if (dup(pipe[descriptor]) != descriptor)
		syserr("Descriptor: %d not overriden, in Worker.\n", descriptor);
}

void readConfirmationSymbol(const int readDsc,
	struct ConfirmationMsg* confirmationMsg, int confirmationMsgSize) {
	if (read(readDsc, confirmationMsg, confirmationMsgSize) == -1)
		syserr("Error while reading child response.\n");
	if (confirmationMsg->result == SUCCESS)
		syserr("Unknown job confirmation response.\n");
}

#endif
