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
	}
	else 
	{
		std::cout << "magic number is invalid" << std::endl;
	}
	std::cout << "\t" << block.Super.Blocks << " blocks" << std::endl;
	std::cout << "\t" << block.Super.InodeBlocks << " inode blocks" << std::endl;
	std::cout << "\t" << block.Super.Inodes << " inodes" << std::endl;

	// Read Inode blocks

	for(uint32_t i = 0; i < INODES_PER_BLOCK; i++)
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
	// Write superblock

	// Clear all other blocks
	return true;
}

// Mount file system -----------------------------------------------------------

bool FileSystem::mount(Disk *disk) {
	// Read superblock

	// Set device and mount

	// Copy metadata

	// Allocate free block bitmap

	return true;
}

// Create inode ----------------------------------------------------------------

ssize_t FileSystem::create() {
	// Locate free inode in inode table

	// Record inode if found
	return 0;
}

// Remove inode ----------------------------------------------------------------

bool FileSystem::remove(size_t inumber) {
	// Load inode information

	// Free direct blocks

	// Free indirect blocks

	// Clear inode in inode table
	return true;
}

// Inode stat ------------------------------------------------------------------

ssize_t FileSystem::stat(size_t inumber) {
	// Load inode information
	return 0;
}

// Read from inode -------------------------------------------------------------

ssize_t FileSystem::read(size_t inumber, char *data, size_t length, size_t offset) {
	// Load inode information

	// Adjust length

	// Read block and copy to data
	return 0;
}

// Write to inode --------------------------------------------------------------

ssize_t FileSystem::write(size_t inumber, char *data, size_t length, size_t offset) {
	// Load inode

	// Write block and copy to data
	return 0;
}
