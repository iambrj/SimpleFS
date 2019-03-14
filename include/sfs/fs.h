// fs.h: File System

#pragma once

#include "sfs/disk.h"

#include <stdint.h>

class FileSystem {
	public:
		const static uint32_t MAGIC_NUMBER	     = 0xf0f03410;
		const static uint32_t INODES_PER_BLOCK   = 128;
		const static uint32_t INODES_PERCENT = 10;
		const static uint32_t POINTERS_PER_INODE = 5;
		const static uint32_t POINTERS_PER_BLOCK = 1024;
		const static uint32_t BLOCK_UNSET = 0;

	private:
		struct SuperBlock {		// Superblock structure
			uint32_t MagicNumber;	// File system magic number
			uint32_t Blocks;	// Number of blocks in file system
			uint32_t InodeBlocks;	// Number of blocks reserved for inodes
			uint32_t Inodes;	// Number of inodes in file system
		};

		struct Inode {
			uint32_t Valid;		// Whether or not inode is valid
			uint32_t Size;		// Size of file
			uint32_t Direct[POINTERS_PER_INODE]; // Direct pointers
			uint32_t Indirect;	// Indirect pointer
		};

		union Block {
			SuperBlock  Super;			    // Superblock
			Inode	    Inodes[INODES_PER_BLOCK];	    // Inode block
			uint32_t    Pointers[POINTERS_PER_BLOCK];   // Pointer block for double hashing
			char	    Data[Disk::BLOCK_SIZE];	    // Data block
		};

		// Internal helper functions

		bool isInumberValid(ssize_t inumber);
		uint32_t getBlockNumber(ssize_t inumber);
		uint32_t getInodeNumber(ssize_t inumber);

		// Internal member variables

		Block *memBmap;
		Block *memSuperBlock;
		Inode *memInodes;

	public:
		static void debug(Disk *disk);
		static bool format(Disk *disk);

		bool mount(Disk *disk);

		ssize_t create();
		bool    remove(size_t inumber);
		ssize_t stat(size_t inumber);

		ssize_t read(size_t inumber, void *data, size_t length, size_t offset);
		ssize_t write(size_t inumber, void *data, size_t length, size_t offset);
};
