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
	disk->read(0, block.Data);

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

	for(uint32_t i = 1; i <= FileSystem::INODES_PER_BLOCK; i++)
	{
		if(block.Inodes[i].Valid)
		{
			std::cout << "Inode " << i + 1 << ":" << std::endl;
			std::cout << "\t" << "size: " << block.Inodes[i].Size << std::endl;
			std::cout << "\t" << "direct blocks: " << ceil( 1.0 * block.Inodes[i].Size / Disk::BLOCK_SIZE ) << std::endl;
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
		for(uint32_t j = 0; j < FileSystem::INODES_PER_BLOCK; j++)
		{
			iblock.Inodes[j].Valid = 0;
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

	// Load Inode blocks into main memory

	memInodes = new Block [memSuperBlock -> Super.InodeBlocks];
	for(uint32_t i = 0; i < (memSuperBlock -> Super.InodeBlocks); i++)
	{
		disk -> read(i + 1, memInodes[i].Data);
	}

	// Load bitmap into main memory by going through all the inodes

	memBmap = new bool [memSuperBlock -> Super.Blocks - memSuperBlock -> Super.InodeBlocks - 1];
	memBmap = 0;

	for(uint32_t i = 1; i <= memSuperBlock -> Super.InodeBlocks; i++)
	{
		Block block;
		disk -> read(i, &block.Data);

		for(uint32_t j = 0; j < INODES_PER_BLOCK; j++)
		{
			if(block.Inodes[j].Valid)
			{
				for(uint32_t k = 0; k < POINTERS_PER_INODE; k++)
				{
					if(block.Inodes[j].Direct[k] != BLOCK_UNSET)
					{
						memBmap[block.Inodes[j].Direct[k]] = true;
					}
				}
			}
		}
	}
	
	return true;
}

// Create inode ----------------------------------------------------------------

ssize_t FileSystem::create() {
	// Locate free inode in inode table

	for(uint32_t i = 0, j; i < memSuperBlock -> Super.InodeBlocks; i++)
	{
		for(j = 0; j < INODES_PER_BLOCK; j++)

		if(!memInodes[i].Inodes[j].Valid)
		{
			memInodes[i].Inodes[j].Valid = 1;
			memInodes[i].Inodes[j].Size = 0;
			for(uint32_t k = 0; k < POINTERS_PER_INODE; k++)
			{
				memInodes[i].Inodes[j].Direct[k] = 0;
			}
			memInodes[i].Inodes[j].Indirect = 0;
		}

		return i * INODES_PER_BLOCK + j;
	}

	return -1;
}

// Remove inode ----------------------------------------------------------------

bool FileSystem::remove(size_t inumber) {
	
	// Out-of-bounds checks

	if(inumber < 0 || inumber >= memSuperBlock -> Super.Inodes)
	{
		std::cout << "Error: inumber = "<< inumber << " out of bounds" << std::endl;
		return false;
	}

	// TODO: abstract these into a private function

	uint32_t blockNumber = inumber / INODES_PER_BLOCK;
	uint32_t inodeNumber = inumber % INODES_PER_BLOCK;

	// Double free check

	if(!memInodes[blockNumber].Inodes[inodeNumber].Valid)
	{
		std::cout << "Error: trying to deleting free inode" << std::endl;
		return false;
	}

	// Clear inode in inode table

	memInodes[blockNumber].Inodes[inodeNumber].Size = 0;
	memInodes[blockNumber].Inodes[inodeNumber].Valid = 0;

	// Free direct blocks

	for(uint32_t i = 0; i < POINTERS_PER_INODE; i++)
	{
		memInodes[blockNumber].Inodes[inodeNumber].Direct[i] = 0;
	}

	// Free indirect blocks

	memInodes[blockNumber].Inodes[inodeNumber].Indirect = 0;

	// Update bitmap
	// TODO:	improve efficiency by directly setting only freed blocks to false
	// 			instead of rechecking every data block

	memBmap = 0;
	for(uint32_t i = 1; i <= memSuperBlock -> Super.InodeBlocks; i++)
	{

		for(uint32_t j = 0; j < INODES_PER_BLOCK; j++)
		{
			if(memInodes[i].Inodes[j].Valid)
			{
				for(uint32_t k = 0; k < POINTERS_PER_INODE; k++)
				{
					if(memInodes[i].Inodes[j].Direct[k] != BLOCK_UNSET)
					{
						memBmap[memInodes[i].Inodes[j].Direct[k]] = true;
					}
				}
			}
		}
	}
	
	return true;
}

// Inode stat ------------------------------------------------------------------

ssize_t FileSystem::stat(size_t inumber) {

	// TODO: abstract these invalid checks into a private function
	
	// Out-of-bounds checks

	if(inumber < 0 || inumber >= memSuperBlock -> Super.Inodes)
	{
		std::cout << "Error: inumber = "<< inumber << " out of bounds" << std::endl;
		return -1;
	}

	// TODO: abstract these into a private function

	uint32_t blockNumber = inumber / INODES_PER_BLOCK;
	uint32_t inodeNumber = inumber % INODES_PER_BLOCK;

	// Check for invalid inode

	if(!memInodes[blockNumber].Inodes[inodeNumber].Valid)
	{
		std::cout << "Error: inumber " << inumber << " invalid" << std::endl;
		return -1;
	}

	return memInodes[blockNumber].Inodes[inodeNumber].Size;
}

// Read from inode -------------------------------------------------------------

ssize_t FileSystem::read(size_t inumber, void *data, size_t length, size_t offset) {
	// Load inode information

	// Adjust length

	// Read block and copy to data
	return 0;
}

// Write to inode --------------------------------------------------------------

ssize_t FileSystem::write(size_t inumber, void *data, size_t length, size_t offset) {
	// Load inode

	// Write block and copy to data
	return 0;
}
