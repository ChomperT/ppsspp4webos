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

#ifdef _WIN32
#include <windows.h>
#endif

#include "../Config.h"
#include "../Host.h"
#include "../SaveState.h"
#include "HLE.h"
#include "../MIPS/MIPS.h"
#include "../HW/MemoryStick.h"
#include "Core/CoreTiming.h"
#include "Core/Reporting.h"

#include "../FileSystems/FileSystem.h"
#include "../FileSystems/MetaFileSystem.h"
#include "../FileSystems/ISOFileSystem.h"
#include "../FileSystems/DirectoryFileSystem.h"

extern "C" {
#include "ext/libkirk/amctrl.h"
};

#include "sceIo.h"
#include "sceRtc.h"
#include "sceKernel.h"
#include "sceKernelMemory.h"
#include "sceKernelThread.h"

// For headless screenshots.
#include "sceDisplay.h"

const int ERROR_ERRNO_FILE_NOT_FOUND               = 0x80010002;
const int ERROR_ERRNO_FILE_ALREADY_EXISTS          = 0x80010011;
const int ERROR_MEMSTICK_DEVCTL_BAD_PARAMS         = 0x80220081;
const int ERROR_MEMSTICK_DEVCTL_TOO_MANY_CALLBACKS = 0x80220082;
const int ERROR_KERNEL_BAD_FILE_DESCRIPTOR		   = 0x80020323;

#define PSP_DEV_TYPE_ALIAS 0x20

/*

TODO: async io is missing features!

flash0: - fat access - system file volume
flash1: - fat access - configuration file volume
flashfat#: this too

lflash: - block access - entire flash
fatms: memstick
isofs: fat access - umd
disc0: fat access - umd
ms0: - fat access - memcard
umd: - block access - umd
irda?: - (?=0..9) block access - infra-red port (doesnt support seeking, maybe send/recieve data from port tho)
mscm0: - block access - memstick cm??
umd00: block access - umd
umd01: block access - umd
*/

#define O_RDONLY		0x0001
#define O_WRONLY		0x0002
#define O_RDWR			0x0003
#define O_NBLOCK		0x0010
#define O_APPEND		0x0100
#define O_CREAT			0x0200
#define O_TRUNC			0x0400
#define O_NOWAIT		0x8000
#define O_NPDRM         0x40000000

// chstat
#define SCE_CST_MODE    0x0001
#define SCE_CST_ATTR    0x0002
#define SCE_CST_SIZE    0x0004
#define SCE_CST_CT      0x0008
#define SCE_CST_AT      0x0010
#define SCE_CST_MT      0x0020
#define SCE_CST_PRVT    0x0040

typedef s32 SceMode;
typedef s64 SceOff;
typedef u64 SceIores;

int asyncNotifyEvent = -1;

#define SCE_STM_FDIR 0x1000
#define SCE_STM_FREG 0x2000
#define SCE_STM_FLNK 0x4000

enum {
	TYPE_DIR=0x10,
	TYPE_FILE=0x20
};

#ifdef __SYMBIAN32__
#undef st_ctime
#undef st_atime
#undef st_mtime
#endif

struct SceIoStat {
	SceMode st_mode;
	unsigned int st_attr;
	SceOff st_size;
	ScePspDateTime st_ctime;
	ScePspDateTime st_atime;
	ScePspDateTime st_mtime;
	unsigned int st_private[6];
};

struct SceIoDirEnt {
	SceIoStat d_stat;
	char d_name[256];
	u32 d_private;
};
#ifndef __SYMBIAN32__
struct dirent {
	u32 unk0;
	u32 type;
	u32 size;
	u32 unk[19];
	char name[0x108];
};
#endif

class FileNode : public KernelObject {
public:
	FileNode() : callbackID(0), callbackArg(0), asyncResult(0), pendingAsyncResult(false), sectorBlockMode(false), closePending(false), npdrm(0), pgdInfo(NULL) {}
	~FileNode() {
		pspFileSystem.CloseFile(handle);
	}
	const char *GetName() {return fullpath.c_str();}
	const char *GetTypeName() {return "OpenFile";}
	void GetQuickInfo(char *ptr, int size) {
		sprintf(ptr, "Seekpos: %08x", (u32)pspFileSystem.GetSeekPos(handle));
	}
	static u32 GetMissingErrorCode() { return SCE_KERNEL_ERROR_BADF; }
	int GetIDType() const { return PPSSPP_KERNEL_TMID_File; }

	virtual void DoState(PointerWrap &p) {
		p.Do(fullpath);
		p.Do(handle);
		p.Do(callbackID);
		p.Do(callbackArg);
		p.Do(asyncResult);
		p.Do(pendingAsyncResult);
		p.Do(sectorBlockMode);
		p.Do(closePending);
		p.Do(info);
		p.Do(openMode);

		// TODO: Savestate PGD files?

		p.DoMarker("File");
	}

	std::string fullpath;
	u32 handle;

	u32 callbackID;
	u32 callbackArg;

	s64 asyncResult;
	bool pendingAsyncResult;

	bool sectorBlockMode;
	// TODO: Use an enum instead?
	bool closePending;

	PSPFileInfo info;
	u32 openMode;

	u32 npdrm;
	PGD_DESC *pgdInfo;
};

/******************************************************************************/



/******************************************************************************/

void __IoCompleteAsyncIO(SceUID id);

static void TellFsThreadEnded (SceUID threadID) {
	pspFileSystem.ThreadEnded(threadID);
}

// Async IO seems to work roughly like this:
// 1. Game calls SceIo*Async() to start the process.
// 2. This runs a thread with a customizable priority.
// 3. The operation runs, which takes an inconsistent amount of time from UMD.
// 4. Once done (regardless of other syscalls), the fd-registered callback is notified.
// 5. The game can find out via *CB() or sceKernelCheckCallback().
// 6. At this point, the fd is STILL not usable.
// 7. One must call sceIoWaitAsync / sceIoWaitAsyncCB / sceIoPollAsync / possibly sceIoGetAsyncStat.
// 8. Finally, the fd is usable (or closed via sceIoCloseAsync.)  Presumably the io thread has joined now.

// TODO: Closed files are a bit special: until the fd is reused (?), the async result is still available.
// Clearly a buffer is used, it doesn't seem like they are actually kernel objects.

// TODO: We don't do any of that yet.
// For now, let's at least delay the callback mnotification.
void __IoAsyncNotify(u64 userdata, int cyclesLate) {
	SceUID threadID = userdata >> 32;
	SceUID fd = (SceUID) (userdata & 0xFFFFFFFF);
	__IoCompleteAsyncIO(fd);

	u32 error;
	SceUID waitID = __KernelGetWaitID(threadID, WAITTYPE_IO, error);
	u32 address = __KernelGetWaitValue(threadID, error);
	if (waitID == fd && error == 0) {
		__KernelResumeThreadFromWait(threadID, 0);

		FileNode *f = kernelObjects.Get<FileNode>(fd, error);
		if (Memory::IsValidAddress(address) && f) {
			Memory::Write_U64((u64) f->asyncResult, address);
		}
	}
}

void __IoInit() {
	INFO_LOG(HLE, "Starting up I/O...");

	MemoryStick_SetFatState(PSP_FAT_MEMORYSTICK_STATE_ASSIGNED);

	asyncNotifyEvent = CoreTiming::RegisterEvent("IoAsyncNotify", __IoAsyncNotify);

#ifdef _WIN32

	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR], file[_MAX_FNAME], ext[_MAX_EXT];
	char memstickpath[_MAX_PATH];
	char flash0path[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));

	char *winpos = strstr(path_buffer, "Windows");
	if (winpos)
	*winpos = 0;
	strcat(path_buffer, "dummy.txt");

	_splitpath_s(path_buffer, drive, dir, file, ext );

	// Mount a couple of filesystems
	sprintf(memstickpath, "%s%smemstick\\", drive, dir);
	sprintf(flash0path, "%s%sflash0\\", drive, dir);

#else
	// TODO
	std::string memstickpath = g_Config.memCardDirectory;
	std::string flash0path = g_Config.flashDirectory;
#endif


	DirectoryFileSystem *memstick = new DirectoryFileSystem(&pspFileSystem, memstickpath);
#ifdef ANDROID
	VFSFileSystem *flash0 = new VFSFileSystem(&pspFileSystem, "flash0");
#else
	DirectoryFileSystem *flash0 = new DirectoryFileSystem(&pspFileSystem, flash0path);
#endif
	pspFileSystem.Mount("ms0:", memstick);
	pspFileSystem.Mount("fatms0:", memstick);
	pspFileSystem.Mount("fatms:", memstick);
	pspFileSystem.Mount("flash0:", flash0);
	
	__KernelListenThreadEnd(&TellFsThreadEnded);
}

void __IoDoState(PointerWrap &p) {
	p.Do(asyncNotifyEvent);
	CoreTiming::RestoreRegisterEvent(asyncNotifyEvent, "IoAsyncNotify", __IoAsyncNotify);
}

void __IoShutdown() {
}

u32 __IoGetFileHandleFromId(u32 id, u32 &outError)
{
	FileNode *f = kernelObjects.Get < FileNode > (id, outError);
	if (!f) {
		return -1;
	}
	return f->handle;
}

u32 sceIoAssign(u32 alias_addr, u32 physical_addr, u32 filesystem_addr, int mode, u32 arg_addr, int argSize) 
{
	std::string alias = Memory::GetCharPointer(alias_addr);
	std::string physical_dev = Memory::GetCharPointer(physical_addr);
	std::string filesystem_dev = Memory::GetCharPointer(filesystem_addr);
	std::string perm;

	switch (mode) {
		case 0:
			perm = "IOASSIGN_RDWR";
			break;
		case 1:
			perm = "IOASSIGN_RDONLY";
			break;
		default:
			perm = "unhandled";
			break;
	}
	DEBUG_LOG(HLE, "sceIoAssign(%s, %s, %s, %s, %08x, %i)", alias.c_str(), physical_dev.c_str(), filesystem_dev.c_str(), perm.c_str(), arg_addr, argSize);
	return 0;
}

u32 sceIoUnassign(const char *alias)  
{
	DEBUG_LOG(HLE, "sceIoUnassign(%s)", alias);
	return 0;
}

u32 sceKernelStdin() {
	DEBUG_LOG(HLE, "3=sceKernelStdin()");
	return 3;
}

u32 sceKernelStdout() {
	DEBUG_LOG(HLE, "1=sceKernelStdout()");
	return 1;
}

u32 sceKernelStderr() {
	DEBUG_LOG(HLE, "2=sceKernelStderr()");
	return 2;
}

void __IoCompleteAsyncIO(SceUID id) {
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		if (f->callbackID) {
			__KernelNotifyCallback(THREAD_CALLBACK_IO, f->callbackID, f->callbackArg);
		}
		f->pendingAsyncResult = false;
	}
}

void __IoCopyDate(ScePspDateTime& date_out, const tm& date_in)
{
	date_out.year = date_in.tm_year+1900;
	date_out.month = date_in.tm_mon+1;
	date_out.day = date_in.tm_mday;
	date_out.hour = date_in.tm_hour;
	date_out.minute = date_in.tm_min;
	date_out.second = date_in.tm_sec;
	date_out.microsecond = 0;
}

void __IoGetStat(SceIoStat *stat, PSPFileInfo &info) {
	memset(stat, 0xfe, sizeof(SceIoStat));
	stat->st_size = (s64) info.size;

	int type, attr;
	if (info.type & FILETYPE_DIRECTORY)
		type = SCE_STM_FDIR, attr = TYPE_DIR;
	else
		type = SCE_STM_FREG, attr = TYPE_FILE;

	stat->st_mode = type | info.access;
	stat->st_attr = attr;
	stat->st_size = info.size;
	__IoCopyDate(stat->st_atime, info.atime);
	__IoCopyDate(stat->st_ctime, info.ctime);
	__IoCopyDate(stat->st_mtime, info.mtime);
	stat->st_private[0] = info.startSector;
}

void __IoSchedAsync(FileNode *f, SceUID fd, int usec) {
	u64 param = ((u64)__KernelGetCurThread()) << 32 | fd;
	CoreTiming::ScheduleEvent(usToCycles(usec), asyncNotifyEvent, param);

	f->pendingAsyncResult = true;
}

u32 sceIoGetstat(const char *filename, u32 addr) {
	// TODO: Improve timing (although this seems normally slow..)
	int usec = 1000;

	SceIoStat stat;
	PSPFileInfo info = pspFileSystem.GetFileInfo(filename);
	if (info.exists) {
		__IoGetStat(&stat, info);
		if (Memory::IsValidAddress(addr)) {
			Memory::WriteStruct(addr, &stat);
			DEBUG_LOG(HLE, "sceIoGetstat(%s, %08x) : sector = %08x", filename, addr, info.startSector);
			return hleDelayResult(0, "io getstat", usec);
		} else {
			ERROR_LOG(HLE, "sceIoGetstat(%s, %08x) : bad address", filename, addr);
			return hleDelayResult(-1, "io getstat", usec);
		}
	} else {
		DEBUG_LOG(HLE, "sceIoGetstat(%s, %08x) : FILE NOT FOUND", filename, addr);
		return hleDelayResult(ERROR_ERRNO_FILE_NOT_FOUND, "io getstat", usec);
	}
}

u32 sceIoChstat(const char *filename, u32 iostatptr, u32 changebits) {
	ERROR_LOG(HLE, "UNIMPL sceIoChstat(%s, %08x, %08x)", filename, iostatptr, changebits);
	if (changebits & SCE_CST_MODE)
		ERROR_LOG(HLE, "sceIoChstat: change mode requested");
	if (changebits & SCE_CST_ATTR)
		ERROR_LOG(HLE, "sceIoChstat: change attr requested");
	if (changebits & SCE_CST_SIZE)
		ERROR_LOG(HLE, "sceIoChstat: change size requested");
	if (changebits & SCE_CST_CT)
		ERROR_LOG(HLE, "sceIoChstat: change creation time requested");
	if (changebits & SCE_CST_AT)
		ERROR_LOG(HLE, "sceIoChstat: change access time requested");
	if (changebits & SCE_CST_MT)
		ERROR_LOG(HLE, "sceIoChstat: change modification time requested");
	if (changebits & SCE_CST_PRVT)
		ERROR_LOG(HLE, "sceIoChstat: change private data requested");
	return 0;
}

u32 npdrmRead(FileNode *f, u8 *data, int size) {
	PGD_DESC *pgd = f->pgdInfo;
	u32 block, offset;
	u32 remain_size, copy_size;

	block  = pgd->file_offset/pgd->block_size;
	offset = pgd->file_offset%pgd->block_size;

	remain_size = size;

	while(remain_size){
	
		if(pgd->current_block!=block){
			pspFileSystem.ReadFile(f->handle, pgd->block_buf, pgd->block_size);
			pgd_decrypt_block(pgd, block);
			pgd->current_block = block;
		}

		if(offset+remain_size>pgd->block_size){
			copy_size = pgd->block_size-offset;
			memcpy(data, pgd->block_buf+offset, copy_size);
			block += 1;
			offset = 0;
		}else{
			copy_size = remain_size;
			memcpy(data, pgd->block_buf+offset, copy_size);
		}

		data += copy_size;
		remain_size -= copy_size;
		pgd->file_offset += copy_size;
	}

	return size;
}

int __IoRead(int id, u32 data_addr, int size) {
	if (id == 3) {
		DEBUG_LOG(HLE, "sceIoRead STDIN");
		return 0; //stdin
	}

	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		if(!(f->openMode & FILEACCESS_READ))
		{
			return ERROR_KERNEL_BAD_FILE_DESCRIPTOR;
		}
		else if (Memory::IsValidAddress(data_addr)) {
			u8 *data = (u8*) Memory::GetPointer(data_addr);
			if(f->npdrm){
				return npdrmRead(f, data, size);
			}else{
				return (int) pspFileSystem.ReadFile(f->handle, data, size);
			}
		} else {
			ERROR_LOG(HLE, "sceIoRead Reading into bad pointer %08x", data_addr);
			// TODO: Returning 0 because it wasn't being sign-extended in async result before.
			// What should this do?
			return 0;
		}
	} else {
		ERROR_LOG(HLE, "sceIoRead ERROR: no file open");
		return error;
	}
}

u32 sceIoRead(int id, u32 data_addr, int size) {
	int result = __IoRead(id, data_addr, size);
	if (result >= 0) {
		DEBUG_LOG(HLE, "%x=sceIoRead(%d, %08x, %x)", result, id, data_addr, size);
		// TODO: Timing is probably not very accurate, low estimate.
		return hleDelayResult(result, "io read", result / 100);
	}
	else
		return result;
}

u32 sceIoReadAsync(int id, u32 data_addr, int size) {
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		f->asyncResult = __IoRead(id, data_addr, size);
		// TODO: Not sure what the correct delay is (and technically we shouldn't read into the buffer yet...)
		__IoSchedAsync(f, id, size / 100);
		DEBUG_LOG(HLE, "%llx=sceIoReadAsync(%d, %08x, %x)", f->asyncResult, id, data_addr, size);
		return 0;
	} else {
		ERROR_LOG(HLE, "sceIoReadAsync: bad file %d", id);
		return error;
	}
}

int __IoWrite(int id, void *data_ptr, int size) {
	if (id == 2) {
		//stderr!
		const char *str = (const char*) data_ptr;
		INFO_LOG(HLE, "stderr: %s", str);
		return size;
	} else if (id == 1) {
		//stdout!
		char *str = (char *) data_ptr;
		char temp = str[size];
		str[size] = 0;
		INFO_LOG(HLE, "stdout: %s", str);
		str[size] = temp;
		return size;
	}
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		if(!(f->openMode & FILEACCESS_WRITE)) {
			return ERROR_KERNEL_BAD_FILE_DESCRIPTOR;
		}
		return (int) pspFileSystem.WriteFile(f->handle, (u8*) data_ptr, size);
	} else {
		ERROR_LOG(HLE, "sceIoWrite ERROR: no file open");
		return (s32) error;
	}
}

u32 sceIoWrite(int id, u32 data_addr, int size) {
	int result = __IoWrite(id, Memory::GetPointer(data_addr), size);
	if (result >= 0) {
		DEBUG_LOG(HLE, "%x=sceIoWrite(%d, %08x, %x)", result, id, data_addr, size);
		// TODO: Timing is probably not very accurate, low estimate.
		return hleDelayResult(result, "io write", result / 100);
	}
	else
		return result;
}

u32 sceIoWriteAsync(int id, u32 data_addr, int size) {
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		f->asyncResult = __IoWrite(id, Memory::GetPointer(data_addr), size);
		// TODO: Not sure what the correct delay is (and technically we shouldn't read into the buffer yet...)
		__IoSchedAsync(f, id, size / 100);
		DEBUG_LOG(HLE, "%llx=sceIoWriteAsync(%d, %08x, %x)", f->asyncResult, id, data_addr, size);
		return 0;
	} else {
		ERROR_LOG(HLE, "sceIoWriteAsync: bad file %d", id);
		return error;
	}
}

u32 sceIoGetDevType(int id) 
{
	ERROR_LOG(HLE, "UNIMPL sceIoGetDevType(%d)", id);
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	int result;
	if (f) 
		result = PSP_DEV_TYPE_ALIAS;
	else {
		ERROR_LOG(HLE, "sceIoGetDevType: unknown id %d", id);
		result = ERROR_KERNEL_BAD_FILE_DESCRIPTOR;
	}

	return result;
}

u32 sceIoCancel(int id) 
{
	ERROR_LOG(HLE, "UNIMPL sceIoCancel(%d)", id);
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		// TODO: Cancel the async operation if possible?
	} else {
		ERROR_LOG(HLE, "sceIoCancel: unknown id %d", id);
		error = ERROR_KERNEL_BAD_FILE_DESCRIPTOR;
	}

	return error;
}

u32 npdrmLseek(FileNode *f, s32 where, FileMove whence)
{
	u32 newPos;

	if(whence==FILEMOVE_BEGIN){
		newPos = where;
	}else if(whence==FILEMOVE_CURRENT){
		newPos = f->pgdInfo->file_offset+where;
	}else{
		newPos = f->pgdInfo->data_size+where;
	}

	if(newPos<0 || newPos>f->pgdInfo->data_size){
		return -EINVAL;
	}

	f->pgdInfo->file_offset = newPos;
	pspFileSystem.SeekFile(f->handle, (s32)f->pgdInfo->data_offset+newPos, whence);

	return newPos;
}

s64 __IoLseek(SceUID id, s64 offset, int whence) {
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		FileMove seek = FILEMOVE_BEGIN;

		s64 newPos = 0;
		switch (whence) {
		case 0:
			newPos = offset;
			break;
		case 1:
			newPos = pspFileSystem.GetSeekPos(f->handle) + offset;
			seek = FILEMOVE_CURRENT;
			break;
		case 2:
			newPos = f->info.size + offset;
			seek = FILEMOVE_END;
			break;
		}
		// Yes, -1 is the correct return code for this case.
		if (newPos < 0)
			return -1;

		if(f->npdrm){
			return npdrmLseek(f, (s32)offset, seek);
		}else{
			return pspFileSystem.SeekFile(f->handle, (s32) offset, seek);
		}
	} else {
		return (s32) error;
	}
}

s64 sceIoLseek(int id, s64 offset, int whence) {
	s64 result = __IoLseek(id, offset, whence);
	if (result >= 0) {
		DEBUG_LOG(HLE, "%lli = sceIoLseek(%d, %llx, %i)", result, id, offset, whence);
		// Educated guess at timing.
		return hleDelayResult(result, "io seek", 100);
	} else {
		ERROR_LOG(HLE, "sceIoLseek(%d, %llx, %i) - ERROR: invalid file", id, offset, whence);
		return result;
	}
}

u32 sceIoLseek32(int id, int offset, int whence) {
	s32 result = (s32) __IoLseek(id, offset, whence);
	if (result >= 0) {
		DEBUG_LOG(HLE, "%i = sceIoLseek(%d, %x, %i)", result, id, offset, whence);
		// Educated guess at timing.
		return hleDelayResult(result, "io seek", 100);
	} else {
		ERROR_LOG(HLE, "sceIoLseek(%d, %x, %i) - ERROR: invalid file", id, offset, whence);
		return result;
	}
}

u32 sceIoLseekAsync(int id, s64 offset, int whence) {
	u32 error;
	FileNode *f = kernelObjects.Get<FileNode>(id, error);
	if (f) {
		f->asyncResult = __IoLseek(id, offset, whence);
		// Educated guess at timing.
		__IoSchedAsync(f, id, 100);
		DEBUG_LOG(HLE, "%lli = sceIoLseekAsync(%d, %llx, %i)", f->asyncResult, id, offset, whence);
		return 0;
	} else {
		ERROR_LOG(HLE, "sceIoLseekAsync(%d, %llx, %i) - ERROR: invalid file", id, offset, whence);
		return error;
	}
	return 0;
}

u32 sceIoLseek32Async(int id, int offset, int whence) {
	u32 error;
	FileNode *f = kernelObjects.Get<FileNode>(id, error);
	if (f) {
		f->asyncResult = __IoLseek(id, offset, whence);
		// Educated guess at timing.
		__IoSchedAsync(f, id, 100);
		DEBUG_LOG(HLE, "%lli = sceIoLseek32Async(%d, %x, %i)", f->asyncResult, id, offset, whence);
		return 0;
	} else {
		ERROR_LOG(HLE, "sceIoLseek32Async(%d, %x, %i) - ERROR: invalid file", id, offset, whence);
		return error;
	}
	return 0;
}

FileNode *__IoOpen(const char* filename, int flags, int mode) {
	//memory stick filename
	int access = FILEACCESS_NONE;
	if (flags & O_RDONLY)
		access |= FILEACCESS_READ;
	if (flags & O_WRONLY)
		access |= FILEACCESS_WRITE;
	if (flags & O_APPEND)
		access |= FILEACCESS_APPEND;
	if (flags & O_CREAT)
		access |= FILEACCESS_CREATE;

	PSPFileInfo info = pspFileSystem.GetFileInfo(filename);
	u32 h = pspFileSystem.OpenFile(filename, (FileAccess) access);
	if (h == 0) {
		return NULL;
	}

	FileNode *f = new FileNode();
	SceUID id = kernelObjects.Create(f);
	f->handle = h;
	f->fullpath = filename;
	f->asyncResult = id;
	f->info = info;
	f->openMode = access;

	f->npdrm = (flags & O_NPDRM)? true: false;

	return f;
}

u32 sceIoOpen(const char* filename, int flags, int mode) {
	FileNode *f = __IoOpen(filename, flags, mode);
	if (f == NULL) {
		ERROR_LOG(HLE, "ERROR_ERRNO_FILE_NOT_FOUND=sceIoOpen(%s, %08x, %08x) - file not found", filename, flags, mode);
		// Timing is not accurate, aiming low for now.
		return hleDelayResult(ERROR_ERRNO_FILE_NOT_FOUND, "file opened", 100);
	}

	SceUID id = f->GetUID();
	DEBUG_LOG(HLE, "%i=sceIoOpen(%s, %08x, %08x)", id, filename, flags, mode);
	// Timing is not accurate, aiming low for now.
	return hleDelayResult(id, "file opened", 100);
}

u32 sceIoClose(int id) {
	u32 error;
	DEBUG_LOG(HLE, "sceIoClose(%d)", id);
	FileNode *f = kernelObjects.Get<FileNode>(id, error);
	if(f && f->npdrm){
		pgd_close(f->pgdInfo);
	}
	// Timing is not accurate, aiming low for now.
	return hleDelayResult(kernelObjects.Destroy<FileNode>(id), "file closed", 100);
}

u32 sceIoRemove(const char *filename) {
	DEBUG_LOG(HLE, "sceIoRemove(%s)", filename);

	// TODO: This timing isn't necessarily accurate, low end for now.
	if(!pspFileSystem.GetFileInfo(filename).exists)
		return hleDelayResult(ERROR_ERRNO_FILE_NOT_FOUND, "file removed", 100);

	pspFileSystem.RemoveFile(filename);
	return hleDelayResult(0, "file removed", 100);
}

u32 sceIoMkdir(const char *dirname, int mode) {
	DEBUG_LOG(HLE, "sceIoMkdir(%s, %i)", dirname, mode);
	// TODO: Improve timing.
	if (pspFileSystem.MkDir(dirname))
		return hleDelayResult(0, "mkdir", 1000);
	else
		return hleDelayResult(ERROR_ERRNO_FILE_ALREADY_EXISTS, "mkdir", 1000);
}

u32 sceIoRmdir(const char *dirname) {
	DEBUG_LOG(HLE, "sceIoRmdir(%s)", dirname);
	// TODO: Improve timing.
	if (pspFileSystem.RmDir(dirname))
		return hleDelayResult(0, "rmdir", 1000);
	else
		return hleDelayResult(ERROR_ERRNO_FILE_NOT_FOUND, "rmdir", 1000);
}

u32 sceIoSync(const char *devicename, int flag) {
	DEBUG_LOG(HLE, "UNIMPL sceIoSync(%s, %i)", devicename, flag);
	return 0;
}

struct DeviceSize {
	u32 maxClusters;
	u32 freeClusters;
	u32 maxSectors;
	u32 sectorSize;
	u32 sectorCount;
};

u32 sceIoDevctl(const char *name, int cmd, u32 argAddr, int argLen, u32 outPtr, int outLen) {
	if (strcmp(name, "emulator:")) {
		DEBUG_LOG(HLE,"sceIoDevctl(\"%s\", %08x, %08x, %i, %08x, %i)", name, cmd, argAddr, argLen, outPtr, outLen);
	}

	// UMD checks
	switch (cmd) {
	case 0x01F20001:  // Get Disc Type.
		if (Memory::IsValidAddress(outPtr + 4)) {
			Memory::Write_U32(0x10, outPtr + 4);  // Game disc
			return 0;
		}
		return -1;
	case 0x01F20002:  // Get last sector number.
	case 0x01F20003:  // Seems identical?
		if (Memory::IsValidAddress(outPtr)) {
			PSPFileInfo info = pspFileSystem.GetFileInfo("umd1:");
			Memory::Write_U32((u32) (info.size / 2048) - 1, outPtr);
			return 0;
		}
		return -1;
	case 0x01F100A3:  // Seek
		// Timing is just a rough guess, probably takes longer.
		return hleDelayResult(0, "dev seek", 100);
	case 0x01F100A4:  // Cache
		return 0;
	case 0x01F300A5:  // Cache + status
		if (Memory::IsValidAddress(outPtr)) {
			Memory::Write_U32(1, outPtr);
			return 0;
		}
		return -1;
	// TODO: What does these do?  Seem to require a u32 in, no output.
	case 0x01F100A6:
	case 0x01F100A8:
	case 0x01F100A9:
	case 0x01F300A7:
		Reporting::ReportMessage("sceIoDevctl(\"%s\", %08x, %08x, %i, %08x, %i)", name, cmd, argAddr, argLen, outPtr, outLen);
		ERROR_LOG(HLE, "UNIMPL sceIoDevctl(\"%s\", %08x, %08x, %i, %08x, %i)", name, cmd, argAddr, argLen, outPtr, outLen);
		return 0;
	}

	// This should really send it on to a FileSystem implementation instead.

	if (!strcmp(name, "mscmhc0:") || !strcmp(name, "ms0:"))
	{
		switch (cmd)
		{
		// does one of these set a callback as well? (see coded arms)
		case 0x02015804:	// Register callback
			if (Memory::IsValidAddress(argAddr) && argLen == 4) {
				u32 cbId = Memory::Read_U32(argAddr);
				if (0 == __KernelRegisterCallback(THREAD_CALLBACK_MEMORYSTICK, cbId)) {
					DEBUG_LOG(HLE, "sceIoDevCtl: Memstick callback %i registered, notifying immediately.", cbId);
					__KernelNotifyCallbackType(THREAD_CALLBACK_MEMORYSTICK, cbId, MemoryStick_State());
					return 0;
				} else {
					return ERROR_MEMSTICK_DEVCTL_BAD_PARAMS;
				}
			}
			break;

		case 0x02025805:	// Unregister callback
			if (Memory::IsValidAddress(argAddr) && argLen == 4) {
				u32 cbId = Memory::Read_U32(argAddr);
				if (0 == __KernelUnregisterCallback(THREAD_CALLBACK_MEMORYSTICK, cbId)) {
					DEBUG_LOG(HLE, "sceIoDevCtl: Unregistered memstick callback %i", cbId);
					return 0;
				} else {
					return ERROR_MEMSTICK_DEVCTL_BAD_PARAMS;
				}
			}
			break;

		case 0x02025801:	// Memstick Driver status?
			if (Memory::IsValidAddress(outPtr) && outLen >= 4) {
				Memory::Write_U32(4, outPtr);  // JPSCP: The right return value is 4 for some reason
				return 0;
			} else {
				return ERROR_MEMSTICK_DEVCTL_BAD_PARAMS;
			}

		case 0x02025806:	// Memory stick inserted?
			if (Memory::IsValidAddress(outPtr) && outLen >= 4) {
				Memory::Write_U32(1, outPtr);
				return 0;
			} else {
				return ERROR_MEMSTICK_DEVCTL_BAD_PARAMS;
			}

		case 0x02425818:  // Get memstick size etc
			// Pretend we have a 2GB memory stick.
			if (Memory::IsValidAddress(argAddr) && argLen >= 4) {  // "Should" be outPtr but isn't
				u32 pointer = Memory::Read_U32(argAddr);
				u32 sectorSize = 0x200;
				u32 memStickSectorSize = 32 * 1024;
				u32 sectorCount = memStickSectorSize / sectorSize;
				u64 freeSize = 1 * 1024 * 1024 * 1024;
				DeviceSize deviceSize;
				deviceSize.maxClusters = (u32)((freeSize  * 95 / 100) / (sectorSize * sectorCount));
				deviceSize.freeClusters = deviceSize.maxClusters;
				deviceSize.maxSectors = deviceSize.maxClusters;
				deviceSize.sectorSize = sectorSize;
				deviceSize.sectorCount = sectorCount;
				Memory::WriteStruct(pointer, &deviceSize);
				DEBUG_LOG(HLE, "Returned memstick size: maxSectors=%i", deviceSize.maxSectors);
				return 0;
			} else {
				ERROR_LOG(HLE, "memstick size query: bad params");
				return ERROR_MEMSTICK_DEVCTL_BAD_PARAMS;
			}

		case 0x02425824:  // Check if write protected
			if (Memory::IsValidAddress(outPtr) && outLen == 4) {
				Memory::Write_U32(0, outPtr);
				return 0;
			} else {
				ERROR_LOG(HLE, "Failed 0x02425824 fat");
				return -1;
			}
		}
	}

	if (!strcmp(name, "fatms0:"))
	{
		switch (cmd) {
		case 0x02415821:  // MScmRegisterMSInsertEjectCallback
			{
				u32 cbId = Memory::Read_U32(argAddr);
				if (0 == __KernelRegisterCallback(THREAD_CALLBACK_MEMORYSTICK_FAT, cbId)) {
					DEBUG_LOG(HLE, "sceIoDevCtl: Memstick FAT callback %i registered, notifying immediately.", cbId);
					__KernelNotifyCallbackType(THREAD_CALLBACK_MEMORYSTICK_FAT, cbId, MemoryStick_FatState());
					return 0;
				} else {
					return -1;
				}
			}
			break;
		case 0x02415822: // MScmUnregisterMSInsertEjectCallback
			{
				u32 cbId = Memory::Read_U32(argAddr);
				if (0 == __KernelUnregisterCallback(THREAD_CALLBACK_MEMORYSTICK_FAT, cbId)) {
					DEBUG_LOG(HLE, "sceIoDevCtl: Unregistered memstick FAT callback %i", cbId);
					return 0;
				} else {
					return -1;
				}
			}

		case 0x02415823:  // Set FAT as enabled
			if (Memory::IsValidAddress(argAddr) && argLen == 4) {
				MemoryStick_SetFatState((MemStickFatState)Memory::Read_U32(argAddr));
				return 0;
			} else {
				ERROR_LOG(HLE, "Failed 0x02415823 fat");
				return -1;
			}
			break;

		case 0x02425823:  // Check if FAT enabled
			if (Memory::IsValidAddress(outPtr) && outLen == 4) {
				Memory::Write_U32(MemoryStick_FatState(), outPtr);
				return 0;
			} else {
				ERROR_LOG(HLE, "Failed 0x02425823 fat");
				return -1;
			}
			break;

		case 0x02425824:  // Check if write protected
			if (Memory::IsValidAddress(outPtr) && outLen == 4) {
				Memory::Write_U32(0, outPtr);
				return 0;
			} else {
				ERROR_LOG(HLE, "Failed 0x02425824 fat");
				return -1;
			}
			break;

		case 0x02425818:  // Get memstick size etc
			// Pretend we have a 2GB memory stick.
			{
				if (Memory::IsValidAddress(argAddr) && argLen >= 4) {  // NOTE: not outPtr
					u32 pointer = Memory::Read_U32(argAddr);
					u32 sectorSize = 0x200;
					u32 memStickSectorSize = 32 * 1024;
					u32 sectorCount = memStickSectorSize / sectorSize;
					u64 freeSize = 1 * 1024 * 1024 * 1024;
					DeviceSize deviceSize;
					deviceSize.maxClusters = (u32)((freeSize  * 95 / 100) / (sectorSize * sectorCount));
					deviceSize.freeClusters = deviceSize.maxClusters;
					deviceSize.maxSectors = deviceSize.maxClusters;
					deviceSize.sectorSize = sectorSize;
					deviceSize.sectorCount = sectorCount;
					Memory::WriteStruct(pointer, &deviceSize);
					return 0;
				} else {
					return ERROR_MEMSTICK_DEVCTL_BAD_PARAMS;
				}
			}
		}
	}

	if (!strcmp(name, "kemulator:") || !strcmp(name, "emulator:"))
	{
		// Emulator special tricks!
		switch (cmd)
		{
		case 1:	// EMULATOR_DEVCTL__GET_HAS_DISPLAY
			if (Memory::IsValidAddress(outPtr))
				Memory::Write_U32(0, outPtr);	 // TODO: Make a headless mode for running tests!
			return 0;
		case 2:	// EMULATOR_DEVCTL__SEND_OUTPUT
			{
				std::string data(Memory::GetCharPointer(argAddr), argLen);
				if (PSP_CoreParameter().printfEmuLog)	{
					host->SendDebugOutput(data.c_str());
				}	else {
					if (PSP_CoreParameter().collectEmuLog) {
						*PSP_CoreParameter().collectEmuLog += data;
					} else {
						DEBUG_LOG(HLE, "%s", data.c_str());
					}
				}
				return 0;
			}
		case 3:	// EMULATOR_DEVCTL__IS_EMULATOR
			if (Memory::IsValidAddress(outPtr))
				Memory::Write_U32(1, outPtr);
			return 0;
		case 4: // EMULATOR_DEVCTL__VERIFY_STATE
			// Note that this is async, and makes sure the save state matches up.
			SaveState::Verify();
			// TODO: Maybe save/load to a file just to be sure?
			return 0;

		case 0x20: // EMULATOR_DEVCTL__EMIT_SCREENSHOT
			u8 *topaddr;
			u32 linesize, pixelFormat;

			__DisplayGetFramebuf(&topaddr, &linesize, &pixelFormat, 0);
			// TODO: Convert based on pixel format / mode / something?
			host->SendDebugScreenshot(topaddr, linesize, 272);
			return 0;
		}

		ERROR_LOG(HLE, "sceIoDevCtl: UNKNOWN PARAMETERS");

		return 0;
	}

	//089c6d1c weird branch
	/*
	089c6bdc ]: HLE: sceKernelCreateCallback(name= MemoryStick Detection ,entry= 089c7484 ) (z_un_089c6bc4)
	089c6c18 ]: HLE: sceIoDevctl("mscmhc0:", 02015804, 09ffb9c0, 4, 00000000, 0) (z_un_089c6bc4)
	089c6c40 ]: HLE: sceKernelCreateCallback(name= MemoryStick Assignment ,entry= 089c7534 ) (z_un_089c6bc4)
	089c6c78 ]: HLE: sceIoDevctl("fatms0:", 02415821, 09ffb9c4, 4, 00000000, 0) (z_un_089c6bc4)
	089c6cac ]: HLE: sceIoDevctl("mscmhc0:", 02025806, 00000000, 0, 09ffb9c8, 4) (z_un_089c6bc4)
	*/
	return SCE_KERNEL_ERROR_UNSUP;
}

u32 sceIoRename(const char *from, const char *to) {
	DEBUG_LOG(HLE, "sceIoRename(%s, %s)", from, to);

	// TODO: Timing isn't terribly accurate.
	if (!pspFileSystem.GetFileInfo(from).exists)
		return hleDelayResult(ERROR_ERRNO_FILE_NOT_FOUND, "file renamed", 1000);

	if (!pspFileSystem.RenameFile(from, to))
		WARN_LOG(HLE, "Could not move %s to %s", from, to);
	return hleDelayResult(0, "file renamed", 1000);
}

u32 sceIoChdir(const char *dirname) {
	DEBUG_LOG(HLE, "sceIoChdir(%s)", dirname);
	pspFileSystem.ChDir(dirname);
	return 0;
}

int sceIoChangeAsyncPriority(int id, int priority)
{
	ERROR_LOG(HLE, "UNIMPL sceIoChangeAsyncPriority(%d, %d)", id, priority);
	return 0;
}

int sceIoCloseAsync(int id)
{
	DEBUG_LOG(HLE, "sceIoCloseAsync(%d)", id);
	u32 error;
	FileNode *f = kernelObjects.Get<FileNode>(id, error);
	if (f)
	{
		f->closePending = true;
		f->asyncResult = 0;
		// TODO: Rough estimate.
		__IoSchedAsync(f, id, 100);
		return 0;
	}
	else
		return error;
}

u32 sceIoSetAsyncCallback(int id, u32 clbckId, u32 clbckArg)
{
	DEBUG_LOG(HLE, "sceIoSetAsyncCallback(%d, %i, %08x)", id, clbckId, clbckArg);

	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f)
	{
		f->callbackID = clbckId;
		f->callbackArg = clbckArg;
		return 0;
	}
	else
	{
		return error;
	}
}

u32 sceIoOpenAsync(const char *filename, int flags, int mode)
{
	FileNode *f = __IoOpen(filename, flags, mode);
	SceUID fd;

	// We have to return an fd here, which may have been destroyed when we reach Wait if it failed.
	if (f == NULL)
	{
		ERROR_LOG(HLE, "ERROR_ERRNO_FILE_NOT_FOUND=sceIoOpenAsync(%s, %08x, %08x) - file not found", filename, flags, mode);

		f = new FileNode();
		fd = kernelObjects.Create(f);
		f->handle = fd;
		f->fullpath = filename;
		f->asyncResult = ERROR_ERRNO_FILE_NOT_FOUND;
		f->closePending = true;
	}
	else
	{
		fd = f->GetUID();
		DEBUG_LOG(HLE, "%x=sceIoOpenAsync(%s, %08x, %08x)", fd, filename, flags, mode);
	}

	// TODO: Timing is very inconsistent.  From ms0, 10ms - 20ms depending on filesize/dir depth?  From umd, can take > 1s.
	// For now let's aim low.
	__IoSchedAsync(f, fd, 100);

	return fd;
}

u32 sceIoGetAsyncStat(int id, u32 poll, u32 address)
{
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f)
	{
		if (f->pendingAsyncResult) {
			if (poll) {
				DEBUG_LOG(HLE, "%lli = sceIoGetAsyncStat(%i, %i, %08x): not ready", f->asyncResult, id, poll, address);
				return 1;
			} else {
				DEBUG_LOG(HLE, "%lli = sceIoGetAsyncStat(%i, %i, %08x): waiting", f->asyncResult, id, poll, address);
				__KernelWaitCurThread(WAITTYPE_IO, id, address, 0, false, "io waited");
			}
		} else {
			DEBUG_LOG(HLE, "%lli = sceIoGetAsyncStat(%i, %i, %08x)", f->asyncResult, id, poll, address);
			Memory::Write_U64((u64) f->asyncResult, address);

			if (f->closePending) {
				kernelObjects.Destroy<FileNode>(id);
			}
		}
		return 0; //completed
	}
	else
	{
		ERROR_LOG(HLE, "ERROR - sceIoGetAsyncStat with invalid id %i", id);
		return -1;
	}
}

int sceIoWaitAsync(int id, u32 address) {
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		if (f->pendingAsyncResult) {
			DEBUG_LOG(HLE, "%lli = sceIoWaitAsync(%i, %08x): waiting", f->asyncResult, id, address);
			__KernelWaitCurThread(WAITTYPE_IO, id, address, 0, false, "io waited");
		} else {
			DEBUG_LOG(HLE, "%lli = sceIoWaitAsync(%i, %08x)", f->asyncResult, id, address);
			Memory::Write_U64((u64) f->asyncResult, address);

			if (f->closePending) {
				kernelObjects.Destroy<FileNode>(id);
			}
		}
		return 0; //completed
	} else {
		ERROR_LOG(HLE, "ERROR - sceIoWaitAsync waiting for invalid id %i", id);
		return -1;
	}
}

int sceIoWaitAsyncCB(int id, u32 address) {
	// Should process callbacks here
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		hleCheckCurrentCallbacks();
		if (f->pendingAsyncResult) {
			DEBUG_LOG(HLE, "%lli = sceIoWaitAsyncCB(%i, %08x): waiting", f->asyncResult, id, address);
			__KernelWaitCurThread(WAITTYPE_IO, id, address, 0, false, "io waited");
		} else {
			DEBUG_LOG(HLE, "%lli = sceIoWaitAsyncCB(%i, %08x)", f->asyncResult, id, address);
			Memory::Write_U64((u64) f->asyncResult, address);

			if (f->closePending) {
				kernelObjects.Destroy<FileNode>(id);
			}
		}
		return 0; //completed
	} else {
		ERROR_LOG(HLE, "ERROR - sceIoWaitAsyncCB waiting for invalid id %i", id);
		return -1;
	}
}

u32 sceIoPollAsync(int id, u32 address) {
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		if (f->pendingAsyncResult) {
			DEBUG_LOG(HLE, "%lli = sceIoPollAsync(%i, %08x): not ready", f->asyncResult, id, address);
			return 1;
		} else {
			DEBUG_LOG(HLE, "%lli = sceIoPollAsync(%i, %08x)", f->asyncResult, id, address);
			Memory::Write_U64((u64) f->asyncResult, address);

			if (f->closePending) {
				kernelObjects.Destroy<FileNode>(id);
			}
			return 0; //completed
		}
	} else {
		ERROR_LOG(HLE, "ERROR - sceIoPollAsync waiting for invalid id %i", id);
		return -1;  // TODO: correct error code
	}
}

class DirListing : public KernelObject {
public:
	const char *GetName() {return name.c_str();}
	const char *GetTypeName() {return "DirListing";}
	static u32 GetMissingErrorCode() { return SCE_KERNEL_ERROR_BADF; }
	int GetIDType() const { return PPSSPP_KERNEL_TMID_DirList; }

	virtual void DoState(PointerWrap &p) {
		p.Do(name);
		p.Do(index);

		// TODO: Is this the right way for it to wake up?
		int count = (int) listing.size();
		p.Do(count);
		listing.resize(count);
		for (int i = 0; i < count; ++i) {
			listing[i].DoState(p);
		}
		p.DoMarker("DirListing");
	}

	std::string name;
	std::vector<PSPFileInfo> listing;
	int index;
};

u32 sceIoDopen(const char *path) {
	DEBUG_LOG(HLE, "sceIoDopen(\"%s\")", path);

	if(!pspFileSystem.GetFileInfo(path).exists)
	{
		return ERROR_ERRNO_FILE_NOT_FOUND;
	}

	DirListing *dir = new DirListing();
	SceUID id = kernelObjects.Create(dir);

	dir->listing = pspFileSystem.GetDirListing(path);
	dir->index = 0;
	dir->name = std::string(path);

	// TODO: The result is delayed only from the memstick, it seems.
	return id;
}

u32 sceIoDread(int id, u32 dirent_addr) {
	u32 error;
	DirListing *dir = kernelObjects.Get<DirListing>(id, error);
	if (dir) {
		SceIoDirEnt *entry = (SceIoDirEnt*) Memory::GetPointer(dirent_addr);

		if (dir->index == (int) dir->listing.size()) {
			DEBUG_LOG(HLE, "sceIoDread( %d %08x ) - end of the line", id, dirent_addr);
			entry->d_name[0] = '\0';
			return 0;
		}

		PSPFileInfo &info = dir->listing[dir->index];
		__IoGetStat(&entry->d_stat, info);

		strncpy(entry->d_name, info.name.c_str(), 256);
		entry->d_name[255] = '\0';
		entry->d_private = 0xC0DEBABE;
		DEBUG_LOG(HLE, "sceIoDread( %d %08x ) = %s", id, dirent_addr, entry->d_name);

		// TODO: Improve timing.  Only happens on the *first* entry read, ms and umd.
		if (dir->index++ == 0) {
			return hleDelayResult(1, "readdir", 1000);
		}
		return 1;
	} else {
		DEBUG_LOG(HLE, "sceIoDread - invalid listing %i, error %08x", id, error);
		return SCE_KERNEL_ERROR_BADF;
	}
}

u32 sceIoDclose(int id) {
	DEBUG_LOG(HLE, "sceIoDclose(%d)", id);
	return kernelObjects.Destroy<DirListing>(id);
}

int __IoIoctl(u32 id, u32 cmd, u32 indataPtr, u32 inlen, u32 outdataPtr, u32 outlen) 
{
	u32 error;
	FileNode *f = kernelObjects.Get<FileNode>(id, error);
	if (error) {
		ERROR_LOG(HLE, "UNIMPL %08x=sceIoIoctl id: %08x, cmd %08x, bad file", error, id, cmd);
		return error;
	}

	//KD Hearts:
	//56:46:434 HLE\sceIo.cpp:886 E[HLE]: UNIMPL 0=sceIoIoctrl id: 0000011f, cmd 04100001, indataPtr 08b313d8, inlen 00000010, outdataPtr 00000000, outLen 0
	//	0000000

	// TODO: This kind of stuff should be moved to the devices (wherever that would be)
	// and does not belong in this file. Same thing with Devctl.

	switch (cmd) {
	// Define decryption key (amctrl.prx DRM)
	case 0x04100001:  
		if (Memory::IsValidAddress(indataPtr) && inlen == 16) {
			u8 keybuf[16];
			u8 pgd_header[0x90];
			u8 pgd_magic[4] = {0x00, 0x50, 0x47, 0x44};
			INFO_LOG(HLE, "Decrypting PGD DRM files");
			memcpy(keybuf, Memory::GetPointer(indataPtr), 16);
			pspFileSystem.ReadFile(f->handle, pgd_header, 0x90);
			f->pgdInfo = pgd_open(pgd_header, 2, keybuf);
			if(f->pgdInfo==NULL){
				ERROR_LOG(HLE, "Not a valid PGD file. Open as normal file.");
				f->npdrm = false;
				pspFileSystem.SeekFile(f->handle, (s32)0, FILEMOVE_BEGIN);
				if(memcmp(pgd_header, pgd_magic, 4)==0){
					// File is PGD file, but key mismatch
					return 0x80510204;
				}else{
					// File is decrypted.
					return 0;
				}
			}else{
				// Everthing OK.
				f->npdrm = true;
				return 0;
			}
		}
		break;

	// Get UMD sector size
	case 0x01020003:
		INFO_LOG(HLE, "sceIoIoCtl: Asked for sector size of file %i", id);
		if (Memory::IsValidAddress(outdataPtr) && outlen == 4) {
			Memory::Write_U32(f->info.sectorSize, outdataPtr);
		}
		break;

	// Get UMD file pointer
	case 0x01020004:
		INFO_LOG(HLE, "sceIoIoCtl: Asked for fpointer of file %i", id);
		if (Memory::IsValidAddress(outdataPtr) && outlen >= 4) {
			Memory::Write_U32(f->info.fpointer, outdataPtr);
		}
		break;

	// Get UMD file start sector .
	case 0x01020006:
		INFO_LOG(HLE, "sceIoIoCtl: Asked for start sector of file %i", id);
		if (Memory::IsValidAddress(outdataPtr) && outlen >= 4) {
			Memory::Write_U32(f->info.startSector, outdataPtr);
		}
		break;

	// Get UMD file size in bytes.
	case 0x01020007:
		INFO_LOG(HLE, "sceIoIoCtl: Asked for size of file %i", id);
		if (Memory::IsValidAddress(outdataPtr) && outlen >= 8) {
			Memory::Write_U64(f->info.size, outdataPtr);
		}
		break;

	default:
		Reporting::ReportMessage("sceIoIoctl(%s, %08x, %x, %08x, %x)", f->fullpath.c_str(), indataPtr, inlen, outdataPtr, outlen);
		ERROR_LOG(HLE, "UNIMPL 0=sceIoIoctl id: %08x, cmd %08x, indataPtr %08x, inlen %08x, outdataPtr %08x, outLen %08x", id,cmd,indataPtr,inlen,outdataPtr,outlen);
		break;
	}

	return 0;
}

u32 sceIoIoctl(u32 id, u32 cmd, u32 indataPtr, u32 inlen, u32 outdataPtr, u32 outlen) 
{
	int result = __IoIoctl(id, cmd, indataPtr, inlen, outdataPtr, outlen);
	// Just a low estimate on timing.
	return hleDelayResult(0, "io ctrl command", 100);
}

u32 sceIoIoctlAsync(u32 id, u32 cmd, u32 indataPtr, u32 inlen, u32 outdataPtr, u32 outlen)
{
	u32 error;
	FileNode *f = kernelObjects.Get < FileNode > (id, error);
	if (f) {
		DEBUG_LOG(HLE, "sceIoIoctlAsync(%08x, %08x, %08x, %08x, %08x, %08x)", id, cmd, indataPtr, inlen, outdataPtr, outlen);
		f->asyncResult = __IoIoctl(id, cmd, indataPtr, inlen, outdataPtr, outlen);
		__IoSchedAsync(f, id, 100);
		return 0;
	} else {
		ERROR_LOG(HLE, "UNIMPL %08x=sceIoIoctl id: %08x, cmd %08x, bad file", error, id, cmd);
		return error;
	}
}

KernelObject *__KernelFileNodeObject() {
	return new FileNode;
}

KernelObject *__KernelDirListingObject() {
	return new DirListing;
}

const HLEFunction IoFileMgrForUser[] = {
	{ 0xb29ddf9c, &WrapU_C<sceIoDopen>, "sceIoDopen" },
	{ 0xe3eb004c, &WrapU_IU<sceIoDread>, "sceIoDread" },
	{ 0xeb092469, &WrapU_I<sceIoDclose>, "sceIoDclose" },
	{ 0xe95a012b, &WrapU_UUUUUU<sceIoIoctlAsync>, "sceIoIoctlAsync" },
	{ 0x63632449, &WrapU_UUUUUU<sceIoIoctl>, "sceIoIoctl" },
	{ 0xace946e8, &WrapU_CU<sceIoGetstat>, "sceIoGetstat" },
	{ 0xb8a740f4, &WrapU_CUU<sceIoChstat>, "sceIoChstat" },
	{ 0x55f4717d, &WrapU_C<sceIoChdir>, "sceIoChdir" },
	{ 0x08bd7374, &WrapU_I<sceIoGetDevType>, "sceIoGetDevType" },
	{ 0xB2A628C1, &WrapU_UUUIUI<sceIoAssign>, "sceIoAssign" },
	{ 0xe8bc6571, &WrapU_I<sceIoCancel>, "sceIoCancel" },
	{ 0xb293727f, &WrapI_II<sceIoChangeAsyncPriority>, "sceIoChangeAsyncPriority" },
	{ 0x810C4BC3, &WrapU_I<sceIoClose>, "sceIoClose" }, //(int fd);
	{ 0xff5940b6, &WrapI_I<sceIoCloseAsync>, "sceIoCloseAsync" },
	{ 0x54F5FB11, &WrapU_CIUIUI<sceIoDevctl>, "sceIoDevctl" }, //(const char *name int cmd, void *arg, size_t arglen, void *buf, size_t *buflen);
	{ 0xcb05f8d6, &WrapU_IUU<sceIoGetAsyncStat>, "sceIoGetAsyncStat" },
	{ 0x27EB27B8, &WrapI64_II64I<sceIoLseek>, "sceIoLseek" }, //(int fd, int offset, int whence);
	{ 0x68963324, &WrapU_III<sceIoLseek32>, "sceIoLseek32" },
	{ 0x1b385d8f, &WrapU_III<sceIoLseek32Async>, "sceIoLseek32Async" },
	{ 0x71b19e77, &WrapU_II64I<sceIoLseekAsync>, "sceIoLseekAsync" },
	{ 0x109F50BC, &WrapU_CII<sceIoOpen>, "sceIoOpen" }, //(const char* file, int mode);
	{ 0x89AA9906, &WrapU_CII<sceIoOpenAsync>, "sceIoOpenAsync" },
	{ 0x06A70004, &WrapU_CI<sceIoMkdir>, "sceIoMkdir" }, //(const char *dir, int mode);
	{ 0x3251ea56, &WrapU_IU<sceIoPollAsync>, "sceIoPollAsync" },
	{ 0x6A638D83, &WrapU_IUI<sceIoRead>, "sceIoRead" }, //(int fd, void *data, int size);
	{ 0xa0b5a7c2, &WrapU_IUI<sceIoReadAsync>, "sceIoReadAsync" },
	{ 0xF27A9C51, &WrapU_C<sceIoRemove>, "sceIoRemove" }, //(const char *file);
	{ 0x779103A0, &WrapU_CC<sceIoRename>, "sceIoRename" }, //(const char *oldname, const char *newname);
	{ 0x1117C65F, &WrapU_C<sceIoRmdir>, "sceIoRmdir" }, //(const char *dir);
	{ 0xA12A0514, &WrapU_IUU<sceIoSetAsyncCallback>, "sceIoSetAsyncCallback" },
	{ 0xab96437f, &WrapU_CI<sceIoSync>, "sceIoSync" },
	{ 0x6d08a871, &WrapU_C<sceIoUnassign>, "sceIoUnassign" },
	{ 0x42EC03AC, &WrapU_IUI<sceIoWrite>, "sceIoWrite" }, //(int fd, void *data, int size);
	{ 0x0facab19, &WrapU_IUI<sceIoWriteAsync>, "sceIoWriteAsync" },
	{ 0x35dbd746, &WrapI_IU<sceIoWaitAsyncCB>, "sceIoWaitAsyncCB" },
	{ 0xe23eec33, &WrapI_IU<sceIoWaitAsync>, "sceIoWaitAsync" },
};

void Register_IoFileMgrForUser() {
	RegisterModule("IoFileMgrForUser", ARRAY_SIZE(IoFileMgrForUser), IoFileMgrForUser);
}


const HLEFunction StdioForUser[] = {
	{ 0x172D316E, &WrapU_V<sceKernelStdin>, "sceKernelStdin" },
	{ 0xA6BAB2E9, &WrapU_V<sceKernelStdout>, "sceKernelStdout" },
	{ 0xF78BA90A, &WrapU_V<sceKernelStderr>, "sceKernelStderr" }, 
};

void Register_StdioForUser() {
	RegisterModule("StdioForUser", ARRAY_SIZE(StdioForUser), StdioForUser);
}
