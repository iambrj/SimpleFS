// journal.cpp: Journal for the File System

#include "sfs/journal.h"
#include <vector.h>

bool Journal::startOperation(ssize_t opCode, OpInfo opInfo)
{
	OpInfo* op = new OpInfo;
	op = opInfo;
}
bool Journal::endOperation(ssize_t opCode, OpInfo opInfo)
{
}
ssize_t Journal::checkJournal()
{
}
bool Journal::recoverJournal()
{
}
