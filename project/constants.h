/*
			Maciej Szeszko,
			id: ms335814,
			University of Warsaw
*/

enum WorkerMode {
	ready = 0 // Created and initialized, but not active yet.
	compute = 1, // 
	send = 2, // Send processed data to the predecessor.
	closed = 3 // Wait for child process, free allocated resources and exit.
};

enum PascalMode {
	initChain = 0, // Chain of worker processes still does not exist.
	commitComputation = 1, //
	gatherResults = 2, //
	waitAndClose = 3 // --------||--------
};

enum Token {
	init = 0, //
	compute = 1, // 
	gatherResults = 2, //
	waitAndClose = 3
};


// Success message:
const char success = '#';

struct ProcessConfirmMsg {
	// After accomplishing request for speficic token,
	// process is obliged to return confirmation message to the predecessor.
	char msg;
};

// 'Y' stands for "Yes, I'm the last one.".
struct TriangleCeofficient {
	int ceofficient;
	char isLast;
};

struct RequestMsg {
	enum Token token; // One of tokens from Token enum.
	int workersLeft; // Number of workers required for complete current row.
};
