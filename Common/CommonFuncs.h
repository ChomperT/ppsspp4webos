// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#pragma once

#ifdef _WIN32
#define SLEEP(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP(x) usleep(x*1000)
#endif

#if defined(IOS) || defined(MIPS)
#include <signal.h>
#endif

template <bool> struct CompileTimeAssert;
template<> struct CompileTimeAssert<true> {};

#define b2(x)   (   (x) | (   (x) >> 1) )
#define b4(x)   ( b2(x) | ( b2(x) >> 2) )
#define b8(x)   ( b4(x) | ( b4(x) >> 4) )
#define b16(x)  ( b8(x) | ( b8(x) >> 8) )  
#define b32(x)  (b16(x) | (b16(x) >>16) )
#define ROUND_UP_POW2(x)	(b32(x - 1) + 1)

#ifndef _WIN32

#include <errno.h>
#ifdef __linux__
#include <byteswap.h>
#elif defined __FreeBSD__
#include <sys/endian.h>
#endif

// go to debugger mode
#ifdef GEKKO
	#define Crash()
#else
// Assume !ARM && !MIPS = x86
#if !defined(ARM) && !defined(MIPS)
	#define Crash() {asm ("int $3");}
#else
  #define Crash() {kill( getpid(), SIGINT ) ; }
#endif
#endif
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))

inline u32 __rotl(u32 x, int shift) {
    shift &= 31;
    if (!shift) return x;
    return (x << shift) | (x >> (32 - shift));
}

inline u64 __rotl64(u64 x, unsigned int shift){
	unsigned int n = shift % 64;
	return (x << n) | (x >> (64 - n));
}

inline u32 __rotr(u32 x, int shift) {
    shift &= 31;
    if (!shift) return x;
    return (x >> shift) | (x << (32 - shift));
}

inline u64 __rotr64(u64 x, unsigned int shift){
	unsigned int n = shift % 64;
	return (x >> n) | (x << (64 - n));
}

#else // WIN32
// Function Cross-Compatibility
	#define strcasecmp _stricmp
	#define strncasecmp _strnicmp
	#define unlink _unlink
	#define snprintf _snprintf
	#define vscprintf _vscprintf
	#define __rotl _rotl
	#define __rotl64 _rotl64
	#define __rotr _rotr
	#define __rotr64 _rotr64

// 64 bit offsets for windows
	#define fseeko _fseeki64
	#define ftello _ftelli64
	#define atoll _atoi64
	#define stat64 _stat64
	#define fstat64 _fstat64
	#define fileno _fileno

	#if _M_IX86
		#define Crash() {__asm int 3}
	#else
extern "C" {
	__declspec(dllimport) void __stdcall DebugBreak(void);
}
		#define Crash() {DebugBreak();}
	#endif // M_IX86
#endif // WIN32 ndef

// Generic function to get last error message.
// Call directly after the command or use the error num.
// This function might change the error code.
// Defined in Misc.cpp.
const char* GetLastErrorMsg();

namespace Common
{
inline u8 swap8(u8 _data) {return _data;}

#ifdef _WIN32
#undef swap32
inline u16 swap16(u16 _data) {return _byteswap_ushort(_data);}
inline u32 swap32(u32 _data) {return _byteswap_ulong (_data);}
inline u64 swap64(u64 _data) {return _byteswap_uint64(_data);}
/*
#elif __linux__
inline u16 swap16(u16 _data) {return bswap_16(_data);}
inline u32 swap32(u32 _data) {return bswap_32(_data);}
inline u64 swap64(u64 _data) {return bswap_64(_data);}
#elif __APPLE__
inline __attribute__((always_inline)) u16 swap16(u16 _data)
	{return (_data >> 8) | (_data << 8);}
inline __attribute__((always_inline)) u32 swap32(u32 _data)
	{return __builtin_bswap32(_data);}
inline __attribute__((always_inline)) u64 swap64(u64 _data)
	{return __builtin_bswap64(_data);}
#elif __FreeBSD__
inline u16 swap16(u16 _data) {return bswap16(_data);}
inline u32 swap32(u32 _data) {return bswap32(_data);}
inline u64 swap64(u64 _data) {return bswap64(_data);}
*/
#else
// Slow generic implementation.
//inline u16 swap16(u16 data) {return (data >> 8) | (data << 8);}
//inline u32 swap32(u32 data) {return (swap16(data) << 16) | swap16(data >> 16);}
//inline u64 swap64(u64 data) {return ((u64)swap32(data) << 32) | swap32(data >> 32);}
#endif

//inline u16 swap16(const u8* _pData) {return swap16(*(const u16*)_pData);}
//inline u32 swap32(const u8* _pData) {return swap32(*(const u32*)_pData);}
//inline u64 swap64(const u8* _pData) {return swap64(*(const u64*)_pData);}

}  // Namespace Common
