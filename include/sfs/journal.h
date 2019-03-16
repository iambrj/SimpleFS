// journal.h: Journal for the File System

#include "sfs/disk.h"
#include "sfs/fs.h"

class Journal {
	public:
		// Operation codes
		const static uint32_t OP_WRITE = 0;
		const static uint32_t OP_READ = 0;
		const static uint32_t OP_BEGUN = 2;
		const static uint32_t OP_ENDED = 3;

			// Error codes

	private:

			struct WriteInfo {
				const uint32_t opCode = OP_WRITE;
				size_t inumber;
				char *data;
				size_t length;
				size_t offset;
			};

			struct ReadInfo {
				const uint32_t opCode = OP_READ;
				size_t inumber;
				char *data;
				size_t length;
				size_t offset;
			};

			union OpInfo {
				WriteInfo writeInfo;
				ReadInfo readInfo;
			};

			// Internal helper functions

			bool checkOperation(OpInfo opInfo);
	public:
			// Begin an operation
			// @param

			bool startOperation(ssize_t opCode, OpInfo opInfo);
			bool endOperation();
			ssize_t checkJournal();
			bool recoverJournal();
};
