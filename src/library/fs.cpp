// fs.cpp: File System

#include "sfs/fs.h"

#include <algorithm>

#include <cassert>
#include <iostream>
#include <cstring>
#include <cmath>

// Debug file system -----------------------------------------------------------

void FileSystem::debug(Disk *disk) {
	Block block; 
	// Read Superblock
	disk -> read(0, block.Data);

	std::cout << "SuperBlock:" << std::endl;
	if(block.Super.MagicNumber == 0xf0f03410) 
	{
		std::cout << "magic number is valid" << std::endl;
		std::cout << "\t" << block.Super.Blocks << " blocks" << std::endl;
		std::cout << "\t" << block.Super.InodeBlocks << " inode blocks" << std::endl;
		std::cout << "\t" << block.Super.Inodes << " inodes" << std::endl;
	}
	else 
	{
		std::cout << "magic number is invalid" << std::endl;
	}

	// Read Inode blocks

	for(uint32_t i = 1; i <= block.Super.InodeBlocks; i++)
	{

		Block iblock;
		disk -> read(i, iblock.Data);

		for(uint32_t j = 0; j < FileSystem::INODES_PER_BLOCK; j++)
		if(block.Inodes[i].Valid)
		{
			std::cout << "Inode " << i * INODES_PER_BLOCK + j + 1 << ":" << std::endl;
			std::cout << "\t" << "size: " << block.Inodes[j].Size << std::endl;
			std::cout << "\t" << "direct blocks: " << ceil( 1.0 * block.Inodes[j].Size / Disk::BLOCK_SIZE ) << std::endl;
		}
	}
}

// Format file system ----------------------------------------------------------

bool FileSystem::format(Disk *disk) {

	std::cout << "Beginning format..." << std::endl;

	// Write superblock

	Block block;
	std::cout << "Creating SuperBlock..." << std::endl;

	std::cout << "Setting MagicNumber to 0xf0f03410" << std::endl; 
	block.Super.MagicNumber = 0xf0f03410;
	std::cout << "MagicNumber set" << std::endl;

	std::cout << "Setting Blocks to " << disk -> size() << std::endl;
	block.Super.Blocks = disk -> size();
	std::cout << "Blocks set to " << disk -> size() << std::endl;


	std::cout << "Setting InodeBlocks (" << INODES_PERCENT << "%) to " << disk -> size() / FileSystem::INODES_PERCENT << std::endl;
	block.Super.InodeBlocks = disk -> size() / FileSystem::INODES_PERCENT;
	std::cout << "InodeBlocks set to " << block.Super.InodeBlocks << std::endl;


	std::cout << "Setting Inodes to " << block.Super.InodeBlocks * FileSystem::INODES_PER_BLOCK << std::endl;
	block.Super.Inodes = block.Super.InodeBlocks * FileSystem::INODES_PER_BLOCK;
	std::cout << "Set Inodes to " << block.Super.InodeBlocks * FileSystem::INODES_PER_BLOCK << std::endl;

	std::cout << "Writing superblock to disk..." << std::endl;
	disk -> Disk::write(0, &block.Data);
	std::cout << "SuperBlock written..." << std::endl;

	std::cout << "Writing inode table..." << std::endl;
	for(uint32_t i = 1; i <= block.Super.InodeBlocks; i++)
	{
		Block iblock;

		// Make all blocks invalid and set all direct pointers to zero
		for(uint32_t j = 0; j < FileSystem::INODES_PER_BLOCK; j++)
		{
			iblock.Inodes[j].Valid = 0;

			for(uint32_t k = 0; k < FileSystem::POINTERS_PER_INODE; k++)
			{
				iblock.Inodes[j].Direct[k] = 0;
			}
		}

		disk -> Disk::write(i, &iblock);
	}

	std::cout << "Inode table written" << std::endl;

	// Clear all other blocks

	std::cout << "Clearing remaining blocks..." << std::endl;
	for(size_t i = 1 + block.Super.InodeBlocks; i < disk -> size(); i++)
	{
		char temp = 0;
		disk -> Disk::write(i, &temp);
	}
	std::cout << "Remaining blocks cleared" << std::endl;
	return true;
}

// Mount file system -----------------------------------------------------------

bool FileSystem::mount(Disk *disk) {

	if(disk -> mounted())
	{
		std::cout << "Already mounted!" << std::endl;
		return false;
	}

	// Set device and mount
	
	disk -> mount();


	// Load SuperBlock into main memory

	memSuperBlock = new Block;
	disk -> read(0, memSuperBlock);
	if(memSuperBlock -> Super.MagicNumber != 0xf0f03410)
	{
		std::cout << "MagicNumber missing!" << std::endl;
		return false;
	}

	// Load Inode array into main memory

	memInodes = new Inode [memSuperBlock -> Super.Inodes];

	for(uint32_t i = 0, j; i < memSuperBlock -> Super.InodeBlocks; i++)
	{

		Block block;
		disk -> read(i + 1, block.Data);

		for(j = 0; j < INODES_PER_BLOCK; j++)
		{
			memInodes[i * INODES_PER_BLOCK + j] = block.Inodes[j];
		}

	}

	// Load blockmap into main memory by going through all the inodes

	memBmap = new Block [memSuperBlock -> Super.Blocks - memSuperBlock -> Super.InodeBlocks - 1];

	loadMemBmap(disk);

	return true;
}

// Create inode ----------------------------------------------------------------
ssize_t FileSystem::create() {

	// Locate free inode in inode array

	for(uint32_t i = 0; i < memSuperBlock -> Super.Inodes; i++)
	{
		if(!memInodes[i].Valid)
		{
			memInodes[i].Valid = 1;
			memInodes[i].Size = 0;
			for(uint32_t j = 0; j < POINTERS_PER_INODE; j++)
			{
				memInodes[i].Direct[j] = 0;
			}
			memInodes[i].Indirect = 0;
			return i;
		}
	}

	return -1;
}

// Remove inode ----------------------------------------------------------------

bool FileSystem::remove(size_t inumber) {

	if(!isInumberValid(inumber))
	{
		return false;
	}

	// Clear inode in inode table

	memInodes[inumber].Size = 0;
	memInodes[inumber].Valid = 0;

	// Free direct blocks

	for(uint32_t i = 0; i < POINTERS_PER_INODE; i++)
	{
		memInodes[inumber].Direct[i] = 0;
	}

	// Free indirect blocks

	memInodes[inumber].Indirect = 0;

	// Update blockmap

	for(uint32_t i = 0; i < POINTERS_PER_INODE; i++)
	{
		strcpy((memBmap[memInodes[inumber].Direct[i]]).Data, "\0");
	}

	return true;
}

// Inode stat ------------------------------------------------------------------

ssize_t FileSystem::stat(size_t inumber) {

	if(!isInumberValid(inumber))
	{
		return -1;
	}

	return memInodes[inumber].Size;
}

// Read from inode -------------------------------------------------------------

ssize_t FileSystem::read(size_t inumber, char *data, size_t length, size_t offset) {

	// Out-of-bounds checks

	if(inumber < 0 || inumber >= memSuperBlock -> Super.Inodes)
	{
		std::cout << "Error: inumber = "<< inumber << " out of bounds" << std::endl;
		return -1;
	}

	// Check for invalid inode

	if(!memInodes[inumber].Valid)
	{
		std::cout << "Error: inumber " << inumber << " invalid" << std::endl;
		return -1;
	}

	// Load inode and initialize data

	uint32_t startPointer = offset / Disk::BLOCK_SIZE;
	uint32_t endPointer = (offset + length) / Disk::BLOCK_SIZE;
	uint32_t blocksRead = 0;

	data = 0;

	// Check for overflow

	if(endPointer >= FileSystem::POINTERS_PER_INODE)
	{
		endPointer = POINTERS_PER_INODE;
	}

	// Use temporary buffer to read blocks into

	char* buffer = new char [(endPointer - startPointer) * Disk::BLOCK_SIZE];
	buffer = 0;

	for(uint32_t i = startPointer; i < endPointer; i++)
	{
		strcat(buffer, (memBmap[memInodes[inumber].Direct[i]]).Data);
		blocksRead++;
	}

	// Copy to required data and return blocksRead

	data = new char [length + 1];
	strncpy(data, buffer + (offset % Disk::BLOCK_SIZE), length);

	return blocksRead;
}

// Write to inode --------------------------------------------------------------

ssize_t FileSystem::write(size_t inumber, char *data, size_t length, size_t offset) {

	// Out-of-bounds checks

	if(inumber < 0 || inumber >= memSuperBlock -> Super.Inodes)
	{
		std::cout << "Error: inumber = "<< inumber << " out of bounds" << std::endl;
		return -1;
	}

	// Check for invalid inode

	if(!memInodes[inumber].Valid)
	{
		std::cout << "Error: inumber " << inumber << " invalid" << std::endl;
		return -1;
	}

	// Load inode and initialize data

	uint32_t startPointer = offset / Disk::BLOCK_SIZE;
	uint32_t endPointer = (offset + length) / Disk::BLOCK_SIZE;
	uint32_t blocksWritten = Disk::BLOCK_SIZE * (endPointer - startPointer) + (Disk::BLOCK_SIZE - offset);

	data = 0;

	// Check for overflow

	if(endPointer >= FileSystem::POINTERS_PER_INODE)
	{
		endPointer = POINTERS_PER_INODE;
	}

	// Write blocks to memBmap

	for(uint32_t i = startPointer; i < endPointer; i++)
	{
		strcpy((memBmap[memInodes[inumber].Direct[i]]).Data, data + Disk::BLOCK_SIZE * (i - startPointer) - offset);
	}

	return blocksWritten;
}

// Internal helper functions --------------------------------------------------

bool FileSystem::isInumberValid(size_t inumber)
{
	// Out-of-bounds checks

	if(inumber < 0 || inumber >= memSuperBlock -> Super.Inodes)
	{
		std::cout << "Error: inumber = "<< inumber << " out of bounds" << std::endl;
		return false;
	}

	// Check for invalid inode

	if(!memInodes[inumber].Valid)
	{
		std::cout << "Error: inumber " << inumber << " invalid" << std::endl;
		return false;
	}

	return true;
}

uint32_t FileSystem::getBlockNumber(size_t inumber)
{
	return inumber / FileSystem::INODES_PER_BLOCK + 1;
}

void FileSystem::loadMemBmap(Disk *disk)
{
	for(uint32_t i = 0; i < memSuperBlock -> Super.Inodes; i++)
	{
		for(uint32_t j = 0; j < FileSystem::POINTERS_PER_INODE; j++)
		{

			if(memInodes[i].Direct[j])
			{
				disk -> read(memInodes[i].Direct[j], (memBmap[memInodes[i].Direct[j]]).Data);
			}

		}
	}
}
