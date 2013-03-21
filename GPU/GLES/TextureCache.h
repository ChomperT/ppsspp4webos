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

#include "../Globals.h"
#include "gfx_es2/fbo.h"
#include "GPU/GPUState.h"

struct VirtualFramebuffer;

class TextureCache 
{
public:
	TextureCache();
	~TextureCache();

	void SetTexture();

	void Clear(bool delete_them);
	void StartFrame();
	void Invalidate(u32 addr, int size, bool force);
	void InvalidateAll(bool force);

	// FramebufferManager keeps TextureCache updated about what regions of memory
	// are being rendered to. This is barebones so far.
	void NotifyFramebuffer(u32 address, VirtualFramebuffer *framebuffer);
	void NotifyFramebufferDestroyed(u32 address, VirtualFramebuffer *framebuffer);

	size_t NumLoadedTextures() const {
		return cache.size();
	}

	bool DecodeTexture(u8 *output, GPUgstate state);

private:
	struct TexCacheEntry {
		// After marking STATUS_UNRELIABLE, if it stays the same this many frames we'll trust it again.
		const static int FRAMES_REGAIN_TRUST = 1000;

		enum Status {
			STATUS_HASHING,
			STATUS_RELIABLE,  // cache, don't hash
			STATUS_UNRELIABLE,  // never cache
		};

		// Status, but int so we can zero initialize.
		int status;
		u32 addr;
		u32 hash;
		VirtualFramebuffer *framebuffer;  // if null, not sourced from an FBO.
		u32 sizeInRAM;
		int lastFrame;
		int numFrames;
		u32 framesUntilNextFullHash;
		u8 format;
		u16 dim;
		u8 clutformat;
		u32 clutaddr;
		u32 cluthash;
		u32 texture;  //GLuint
		int invalidHint;
		u32 fullhash;
		int maxLevel;
		float lodBias;

		// Cache the current filter settings so we can avoid setting it again.
		u8 magFilt;
		u8 minFilt;
		bool sClamp;
		bool tClamp;
	};

	void Decimate();  // Run this once per frame to get rid of old textures.
	void *UnswizzleFromMem(u32 texaddr, u32 bytesPerPixel, u32 level);
	void *readIndexedTex(int level, u32 texaddr, int bytesPerIndex);
	void UpdateSamplingParams(TexCacheEntry &entry, bool force);
	void LoadTextureLevel(TexCacheEntry &entry, int level);

	TexCacheEntry *GetEntryAt(u32 texaddr);

	typedef std::map<u64, TexCacheEntry> TexCache;
	TexCache cache;

	u32 *tmpTexBuf32;
	u16 *tmpTexBuf16;

	u32 *tmpTexBufRearrange;

	u32 *clutBuf32;
	u16 *clutBuf16;

	u32 lastBoundTexture;
	float maxAnisotropyLevel;
};

