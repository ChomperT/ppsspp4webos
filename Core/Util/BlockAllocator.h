// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#pragma once

#include "../../Globals.h"

#include <vector>
#include <list>
#include <cstring>

class PointerWrap;

// Generic allocator thingy. Allocates blocks from a range.

class BlockAllocator
{
public:
	BlockAllocator(int grain = 16);  // 16 byte granularity by default.
	~BlockAllocator();

	void Init(u32 _rangeStart, u32 _rangeSize);
	void Shutdown();

	void ListBlocks();

	// WARNING: size can be modified upwards!
	u32 Alloc(u32 &size, bool fromTop = false, const char *tag = 0);
	u32 AllocAligned(u32 &size, u32 sizeGrain, u32 grain, bool fromTop = false, const char *tag = 0);
	u32 AllocAt(u32 position, u32 size, const char *tag = 0);

	bool Free(u32 position);
	bool FreeExact(u32 position);
	bool IsBlockFree(u32 position) {
		Block *b = GetBlockFromAddress(position);
		if (b)
			return !b->taken;
		else
			return false;
	}

	void MergeFreeBlocks();

	u32 GetBlockStartFromAddress(u32 addr);
	u32 GetBlockSizeFromAddress(u32 addr);
	u32 GetLargestFreeBlockSize();
	u32 GetTotalFreeBytes();

	void DoState(PointerWrap &p);

private:
	void CheckBlocks();

	struct Block
	{
		Block(u32 _start, u32 _size, bool _taken) : start(_start), size(_size), taken(_taken)
		{
			strcpy(tag, "(untitled)");
		}
		void SetTag(const char *_tag) {
			if (_tag)
				strncpy(tag, _tag, 32);
			else
				strncpy(tag, "---", 32);
			tag[31] = 0;
		}
		void DoState(PointerWrap &p);
		u32 start;
		u32 size;
		bool taken;
		char tag[32];
	};

	std::list<Block> blocks;
	u32 rangeStart_;
	u32 rangeSize_;

	u32 grain_;

	Block *GetBlockFromAddress(u32 addr);
	std::list<Block>::iterator GetBlockIterFromAddress(u32 addr);
};
