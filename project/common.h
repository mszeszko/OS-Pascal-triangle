/*
			Maciej Szeszko,
			id: ms335814,
			University of Warsaw
*/

#ifndef __COMMON_H_
#define __COMMON_H_

#include "constants.h"

extern void prepareParentPipes(int* readPipe, int* writePipe);

extern void initParentPipes(int* readPipe, int* writePipe);

extern void closeParentDescriptors(int readDsc, int writeDsc);

extern void closeOverridenStandardDescriptors();

/* Pipe naming is fit to PARENT process pipes,
	 so that 'readPipe' the one that is used by PARENT to read messages from. */
extern void closeUnusedChildDescriptors(int* readPipe, int* writePipe);

extern void reassignChildDescriptor(int descriptor, int* pipe);

extern void readConfirmationSymbol(const int readDsc,
	struct ConfirmationMsg* confirmationMsg, int confirmationMsgSize);

#endif
