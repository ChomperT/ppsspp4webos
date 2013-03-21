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

#include "HLE.h"
#include "../MIPS/MIPS.h"
#include "../MIPS/MIPSCodeUtils.h"
#include "../MIPS/MIPSInt.h"

#include "Common/LogManager.h"
#include "../FileSystems/FileSystem.h"
#include "../FileSystems/MetaFileSystem.h"
#include "../PSPLoaders.h"
#include "../../Core/CoreTiming.h"
#include "../../Core/SaveState.h"
#include "../../Core/System.h"
#include "../../GPU/GPUInterface.h"
#include "../../GPU/GPUState.h"

#include "__sceAudio.h"
#include "sceAtrac.h"
#include "sceAudio.h"
#include "sceCtrl.h"
#include "sceDisplay.h"
#include "sceFont.h"
#include "sceGe.h"
#include "sceIo.h"
#include "sceKernel.h"
#include "sceKernelAlarm.h"
#include "sceKernelInterrupt.h"
#include "sceKernelThread.h"
#include "sceKernelMemory.h"
#include "sceKernelMutex.h"
#include "sceKernelMbx.h"
#include "sceKernelMsgPipe.h"
#include "sceKernelInterrupt.h"
#include "sceKernelSemaphore.h"
#include "sceKernelEventFlag.h"
#include "sceKernelVTimer.h"
#include "sceKernelTime.h"
#include "sceMpeg.h"
#include "scePower.h"
#include "sceUtility.h"
#include "sceUmd.h"
#include "sceSsl.h"
#include "sceSas.h"
#include "scePsmf.h"
#include "sceImpose.h"
#include "sceUsb.h"
#include "scePspNpDrm_user.h"

#include "../Util/PPGeDraw.h"

/*
17: [MIPS32 R4K 00000000 ]: Loader: Type: 1 Vaddr: 00000000 Filesz: 2856816 Memsz: 2856816 
18: [MIPS32 R4K 00000000 ]: Loader: Loadable Segment Copied to 0898dab0, size 002b9770
19: [MIPS32 R4K 00000000 ]: Loader: Type: 1 Vaddr: 002b9770 Filesz: 14964 Memsz: 733156 
20: [MIPS32 R4K 00000000 ]: Loader: Loadable Segment Copied to 08c47220, size 000b2fe4
*/

static bool kernelRunning = false;
KernelObjectPool kernelObjects;
KernelStats kernelStats;

void __KernelInit()
{
	if (kernelRunning)
	{
		ERROR_LOG(HLE, "Can't init kernel when kernel is running");
		return;
	}

	__KernelTimeInit();
	__InterruptsInit();
	__KernelMemoryInit();
	__KernelThreadingInit();
	__KernelAlarmInit();
	__KernelVTimerInit();
	__KernelEventFlagInit();
	__KernelMbxInit();
	__KernelMutexInit();
	__KernelSemaInit();
	__IoInit();
	__AudioInit();
	__SasInit();
	__AtracInit();
	__DisplayInit();
	__GeInit();
	__PowerInit();
	__UtilityInit();
	__UmdInit();
	__MpegInit(PSP_CoreParameter().useMediaEngine);
	__PsmfInit();
	__CtrlInit();
	__SslInit();
	__ImposeInit();
	__UsbInit();
	__FontInit();
	
	SaveState::Init();  // Must be after IO, as it may create a directory

	// "Internal" PSP libraries
	__PPGeInit();

	kernelRunning = true;
	INFO_LOG(HLE, "Kernel initialized.");
}

void __KernelShutdown()
{
	if (!kernelRunning)
	{
		ERROR_LOG(HLE, "Can't shut down kernel - not running");
		return;
	}
	kernelObjects.List();
	INFO_LOG(HLE, "Shutting down kernel - %i kernel objects alive", kernelObjects.GetCount());
	hleCurrentThreadName = NULL;
	kernelObjects.Clear();

	__FontShutdown();

	__MpegShutdown();
	__PsmfShutdown();
	__PPGeShutdown();

	__CtrlShutdown();
	__UtilityShutdown();
	__GeShutdown();
	__SasShutdown();
	__DisplayShutdown();
	__AtracShutdown();
	__AudioShutdown();
	__IoShutdown();
	__KernelMutexShutdown();
	__KernelThreadingShutdown();
	__KernelMemoryShutdown();
	__InterruptsShutdown();

	CoreTiming::ClearPendingEvents();
	CoreTiming::UnregisterAllEvents();

	kernelRunning = false;
}

void __KernelDoState(PointerWrap &p)
{
	p.Do(kernelRunning);
	kernelObjects.DoState(p);
	p.DoMarker("KernelObjects");

	__InterruptsDoState(p);
	// Memory needs to be after kernel objects, which may free kernel memory.
	__KernelMemoryDoState(p);
	__KernelThreadingDoState(p);
	__KernelAlarmDoState(p);
	__KernelVTimerDoState(p);
	__KernelEventFlagDoState(p);
	__KernelMbxDoState(p);
	__KernelModuleDoState(p);
	__KernelMutexDoState(p);
	__KernelSemaDoState(p);
	__KernelTimeDoState(p);

	__AtracDoState(p);
	__AudioDoState(p);
	__CtrlDoState(p);
	__DisplayDoState(p);
	__FontDoState(p);
	__GeDoState(p);
	__ImposeDoState(p);
	__IoDoState(p);
	__MpegDoState(p);
	__PowerDoState(p);
	__PsmfDoState(p);
	__PsmfPlayerDoState(p);
	__SasDoState(p);
	__SslDoState(p);
	__UmdDoState(p);
	__UtilityDoState(p);
	__UsbDoState(p);

	__PPGeDoState(p);

	__InterruptsDoStateLate(p);
	__KernelThreadingDoStateLate(p);
}

bool __KernelIsRunning() {
	return kernelRunning;
}

void sceKernelExitGame()
{
	INFO_LOG(HLE,"sceKernelExitGame");
	if (!PSP_CoreParameter().headLess)
		PanicAlert("Game exited");
	__KernelSwitchOffThread("game exited");
	Core_Stop();
}

void sceKernelExitGameWithStatus()
{
	INFO_LOG(HLE,"sceKernelExitGameWithStatus");
	if (!PSP_CoreParameter().headLess)
		PanicAlert("Game exited (with status)");
	__KernelSwitchOffThread("game exited");
	Core_Stop();
}

u32 sceKernelRegisterExitCallback(u32 cbId)
{
	DEBUG_LOG(HLE,"NOP sceKernelRegisterExitCallback(%i)", cbId);
	return 0;
}


u32 sceKernelDevkitVersion()
{
	int firmwareVersion = 150;
	int major = firmwareVersion / 100;
	int minor = (firmwareVersion / 10) % 10;
	int revision = firmwareVersion % 10;
	int devkitVersion = (major << 24) | (minor << 16) | (revision << 8) | 0x10;
	DEBUG_LOG(HLE,"sceKernelDevkitVersion (%i) ", devkitVersion);
	return devkitVersion;
}

u32 sceKernelRegisterKprintfHandler()
{
	ERROR_LOG(HLE,"UNIMPL sceKernelRegisterKprintfHandler()");
	return 0;
}
void sceKernelRegisterDefaultExceptionHandler()
{
	ERROR_LOG(HLE,"UNIMPL sceKernelRegisterDefaultExceptionHandler()");
	RETURN(0);
}

void sceKernelSetGPO(u32 ledAddr)
{
	// Sets debug LEDs.
	DEBUG_LOG(HLE,"sceKernelSetGPO(%02x)", ledAddr);
}

u32 sceKernelGetGPI()
{
	// Always returns 0 on production systems.
	DEBUG_LOG(HLE,"0=sceKernelGetGPI()");
	return 0;
}

// #define LOG_CACHE

// Don't even log these by default, they're spammy and we probably won't
// need to emulate them. Useful for invalidating cached textures though,
// and in the future display lists (although hashing takes care of those
// for now).
int sceKernelDcacheInvalidateRange(u32 addr, int size)
{
#ifdef LOG_CACHE
	NOTICE_LOG(HLE,"sceKernelDcacheInvalidateRange(%08x, %i)", addr, size);
#endif
	if (size < 0 || (int) addr + size < 0)
		return SCE_KERNEL_ERROR_ILLEGAL_ADDR;

	if (size > 0)
	{
		if ((addr % 64) != 0 || (size % 64) != 0)
			return SCE_KERNEL_ERROR_CACHE_ALIGNMENT;

		if (addr != 0)
			gpu->InvalidateCache(addr, size);
	}
	return 0;
}

int sceKernelIcacheInvalidateRange(u32 addr, int size) {
	DEBUG_LOG(HLE,"sceKernelIcacheInvalidateRange(%08x, %i)", addr, size);
	// TODO: Make the JIT hash and compare the touched blocks.
	return 0;
}

int sceKernelDcacheWritebackAll()
{
#ifdef LOG_CACHE
	NOTICE_LOG(HLE,"sceKernelDcacheWritebackAll()");
#endif
	// Some games seem to use this a lot, it doesn't make sense
	// to zap the whole texture cache.
	gpu->InvalidateCacheHint(0, -1);
	return 0;
}

int sceKernelDcacheWritebackRange(u32 addr, int size)
{
#ifdef LOG_CACHE
	NOTICE_LOG(HLE,"sceKernelDcacheWritebackRange(%08x, %i)", addr, size);
#endif
	if (size < 0)
		return SCE_KERNEL_ERROR_INVALID_SIZE;

	if (size > 0 && addr != 0) {
		gpu->InvalidateCache(addr, size);
	}
	return 0;
}
int sceKernelDcacheWritebackInvalidateRange(u32 addr, int size)
{
#ifdef LOG_CACHE
	NOTICE_LOG(HLE,"sceKernelDcacheInvalidateRange(%08x, %i)", addr, size);
#endif
	if (size < 0)
		return SCE_KERNEL_ERROR_INVALID_SIZE;

	if (size > 0 && addr != 0) {
		gpu->InvalidateCache(addr, size);
	}
	return 0;
}
int sceKernelDcacheWritebackInvalidateAll()
{
#ifdef LOG_CACHE
	NOTICE_LOG(HLE,"sceKernelDcacheInvalidateAll()");
#endif
	gpu->InvalidateCacheHint(0, -1);
	return 0;
}


u32 sceKernelIcacheInvalidateAll()
{
#ifdef LOG_CACHE
	NOTICE_LOG(CPU, "Icache invalidated - should clear JIT someday");
#endif
	return 0;
}


u32 sceKernelIcacheClearAll()
{
#ifdef LOG_CACHE
	NOTICE_LOG(CPU, "Icache cleared - should clear JIT someday");
#endif
	DEBUG_LOG(CPU, "Icache cleared - should clear JIT someday");
	return 0;
}

KernelObjectPool::KernelObjectPool()
{
	memset(occupied, 0, sizeof(bool)*maxCount);
}

SceUID KernelObjectPool::Create(KernelObject *obj, int rangeBottom, int rangeTop)
{
	if (rangeTop > maxCount)
		rangeTop = maxCount;
	for (int i = rangeBottom; i < rangeTop; i++)
	{
		if (!occupied[i])
		{
			occupied[i] = true;
			pool[i] = obj;
			pool[i]->uid = i + handleOffset;
			return i + handleOffset;
		}
	}
	_dbg_assert_(HLE, 0);
	return 0;
}

bool KernelObjectPool::IsValid(SceUID handle)
{
	int index = handle - handleOffset;
	if (index < 0)
		return false;
	if (index >= maxCount)
		return false;

	return occupied[index];
}

void KernelObjectPool::Clear()
{
	for (int i=0; i<maxCount; i++)
	{
		//brutally clear everything, no validation
		if (occupied[i])
			delete pool[i];
		occupied[i]=false;
	}
	memset(pool, 0, sizeof(KernelObject*)*maxCount);
}

KernelObject *&KernelObjectPool::operator [](SceUID handle)
{
	_dbg_assert_msg_(HLE, IsValid(handle), "GRABBING UNALLOCED KERNEL OBJ");
	return pool[handle - handleOffset];
}

void KernelObjectPool::List()
{
	for (int i = 0; i < maxCount; i++)
	{
		if (occupied[i])
		{
			char buffer[256];
			if (pool[i])
			{
				pool[i]->GetQuickInfo(buffer,256);
				INFO_LOG(HLE, "KO %i: %s \"%s\": %s", i + handleOffset, pool[i]->GetTypeName(), pool[i]->GetName(), buffer);
			}
			else
			{
				strcpy(buffer,"WTF? Zero Pointer");
			}
		}
	}
}

int KernelObjectPool::GetCount()
{
	int count = 0;
	for (int i=0; i<maxCount; i++)
	{
		if (occupied[i])
			count++;
	}
	return count;
}

void KernelObjectPool::DoState(PointerWrap &p)
{
	int _maxCount = maxCount;
	p.Do(_maxCount);

	if (_maxCount != maxCount)
		ERROR_LOG(HLE, "Unable to load state: different kernel object storage.");

	if (p.mode == p.MODE_READ)
	{
		hleCurrentThreadName = NULL;
		kernelObjects.Clear();
	}

	p.DoArray(occupied, maxCount);
	for (int i = 0; i < maxCount; ++i)
	{
		if (!occupied[i])
			continue;

		int type;
		if (p.mode == p.MODE_READ)
		{
			p.Do(type);
			pool[i] = CreateByIDType(type);
			pool[i]->uid = i + handleOffset;

			// Already logged an error.
			if (pool[i] == NULL)
				return;
		}
		else
		{
			type = pool[i]->GetIDType();
			p.Do(type);
		}
		pool[i]->DoState(p);
	}
	p.DoMarker("KernelObjectPool");
}

KernelObject *KernelObjectPool::CreateByIDType(int type)
{
	// Used for save states.  This is ugly, but what other way is there?
	switch (type)
	{
	case SCE_KERNEL_TMID_Alarm:
		return __KernelAlarmObject();
	case SCE_KERNEL_TMID_EventFlag:
		return __KernelEventFlagObject();
	case SCE_KERNEL_TMID_Mbox:
		return __KernelMbxObject();
	case SCE_KERNEL_TMID_Fpl:
		return __KernelMemoryFPLObject();
	case SCE_KERNEL_TMID_Vpl:
		return __KernelMemoryVPLObject();
	case PPSSPP_KERNEL_TMID_PMB:
		return __KernelMemoryPMBObject();
	case PPSSPP_KERNEL_TMID_Module:
		return __KernelModuleObject();
	case SCE_KERNEL_TMID_Mpipe:
		return __KernelMsgPipeObject();
	case SCE_KERNEL_TMID_Mutex:
		return __KernelMutexObject();
	case SCE_KERNEL_TMID_LwMutex:
		return __KernelLwMutexObject();
	case SCE_KERNEL_TMID_Semaphore:
		return __KernelSemaphoreObject();
	case SCE_KERNEL_TMID_Callback:
		return __KernelCallbackObject();
	case SCE_KERNEL_TMID_Thread:
		return __KernelThreadObject();
	case SCE_KERNEL_TMID_VTimer:
		return __KernelVTimerObject();
	case PPSSPP_KERNEL_TMID_File:
		return __KernelFileNodeObject();
	case PPSSPP_KERNEL_TMID_DirList:
		return __KernelDirListingObject();

	default:
		ERROR_LOG(COMMON, "Unable to load state: could not find object type %d.", type);
		return NULL;
	}
}

struct SystemStatus {
	SceSize size;
	SceUInt status;
	SceUInt clockPart1;
	SceUInt clockPart2;
	SceUInt perfcounter1;
	SceUInt perfcounter2;
	SceUInt perfcounter3;
};

int sceKernelReferSystemStatus(u32 statusPtr) {
	DEBUG_LOG(HLE, "sceKernelReferSystemStatus(%08x)", statusPtr);
	if (Memory::IsValidAddress(statusPtr)) {
		SystemStatus status;
		memset(&status, 0, sizeof(SystemStatus));
		status.size = sizeof(SystemStatus);
		// TODO: Fill in the struct!
		Memory::WriteStruct(statusPtr, &status);
	}
	return 0;
}

struct DebugProfilerRegs {
	u32 enable;
	u32 systemck;
	u32 cpuck;
	u32 internal;
	u32 memory;
	u32 copz;
	u32 vfpu;
	u32 sleep;
	u32 bus_access;
	u32 uncached_load;
	u32 uncached_store;
	u32 cached_load;
	u32 cached_store;
	u32 i_miss;
	u32 d_miss;
	u32 d_writeback;
	u32 cop0_inst;
	u32 fpu_inst;
	u32 vfpu_inst;
	u32 local_bus;
};

u32 sceKernelReferThreadProfiler(u32 statusPtr) {
	ERROR_LOG(HLE, "FAKE sceKernelReferThreadProfiler()");

	// Can we confirm that the struct above is the right struct?
	// If so, re-enable this code.
	//DebugProfilerRegs regs;
	//memset(&regs, 0, sizeof(regs));
	// TODO: fill the struct.
	//if (Memory::IsValidAddress(statusPtr)) {
	//	Memory::WriteStruct(statusPtr, &regs);
	//}
	return 0;
}

int sceKernelReferGlobalProfiler(u32 statusPtr) {
	DEBUG_LOG(HLE, "UNIMPL sceKernelReferGlobalProfiler(%08x)", statusPtr);
	// Ignore for now
	return 0;
}

const HLEFunction ThreadManForUser[] =
{
	{0x55C20A00,&WrapI_CUUU<sceKernelCreateEventFlag>,         "sceKernelCreateEventFlag"},
	{0x812346E4,&WrapU_IU<sceKernelClearEventFlag>,            "sceKernelClearEventFlag"},
	{0xEF9E4C70,&WrapU_I<sceKernelDeleteEventFlag>,            "sceKernelDeleteEventFlag"},
	{0x1fb15a32,&WrapU_IU<sceKernelSetEventFlag>,              "sceKernelSetEventFlag"},
	{0x402FCF22,&WrapI_IUUUU<sceKernelWaitEventFlag>,          "sceKernelWaitEventFlag"},
	{0x328C546A,&WrapI_IUUUU<sceKernelWaitEventFlagCB>,        "sceKernelWaitEventFlagCB"},
	{0x30FD48F0,&WrapI_IUUUU<sceKernelPollEventFlag>,          "sceKernelPollEventFlag"},
	{0xCD203292,&WrapU_IUU<sceKernelCancelEventFlag>,          "sceKernelCancelEventFlag"},
	{0xA66B0120,&WrapU_IU<sceKernelReferEventFlagStatus>,      "sceKernelReferEventFlagStatus"},

	{0x8FFDF9A2,&WrapI_IIU<sceKernelCancelSema>,               "sceKernelCancelSema"},
	{0xD6DA4BA1,&WrapI_CUIIU<sceKernelCreateSema>,             "sceKernelCreateSema"},
	{0x28b6489c,&WrapI_I<sceKernelDeleteSema>,                 "sceKernelDeleteSema"},
	{0x58b1f937,&WrapI_II<sceKernelPollSema>,                  "sceKernelPollSema"},
	{0xBC6FEBC5,&WrapI_IU<sceKernelReferSemaStatus>,           "sceKernelReferSemaStatus"},
	{0x3F53E640,&WrapI_II<sceKernelSignalSema>,                "sceKernelSignalSema"},
	{0x4E3A1105,&WrapI_IIU<sceKernelWaitSema>,                 "sceKernelWaitSema"},
	{0x6d212bac,&WrapI_IIU<sceKernelWaitSemaCB>,               "sceKernelWaitSemaCB"},

	{0x60107536,&WrapI_U<sceKernelDeleteLwMutex>,              "sceKernelDeleteLwMutex"},
	{0x19CFF145,&WrapI_UCUIU<sceKernelCreateLwMutex>,          "sceKernelCreateLwMutex"},
	{0x4C145944,&WrapI_IU<sceKernelReferLwMutexStatusByID>,    "sceKernelReferLwMutexStatusByID"},
	// NOTE: LockLwMutex, UnlockLwMutex, and ReferLwMutexStatus are in Kernel_Library, see sceKernelInterrupt.cpp.

	{0xf8170fbe,&WrapI_I<sceKernelDeleteMutex>,                "sceKernelDeleteMutex"},
	{0xB011B11F,&WrapI_IIU<sceKernelLockMutex>,                "sceKernelLockMutex"},
	{0x5bf4dd27,&WrapI_IIU<sceKernelLockMutexCB>,              "sceKernelLockMutexCB"},
	{0x6b30100f,&WrapI_II<sceKernelUnlockMutex>,               "sceKernelUnlockMutex"},
	{0xb7d098c6,&WrapI_CUIU<sceKernelCreateMutex>,             "sceKernelCreateMutex"},
	{0x0DDCD2C9,&WrapI_II<sceKernelTryLockMutex>,              "sceKernelTryLockMutex"},
	{0xA9C2CB9A,&WrapI_IU<sceKernelReferMutexStatus>,          "sceKernelReferMutexStatus"},

	{0xFCCFAD26,sceKernelCancelWakeupThread,"sceKernelCancelWakeupThread"},
	{0xea748e31,sceKernelChangeCurrentThreadAttr,"sceKernelChangeCurrentThreadAttr"},
	{0x71bc9871,sceKernelChangeThreadPriority,"sceKernelChangeThreadPriority"},
	{0x446D8DE6,WrapI_CUUIUU<sceKernelCreateThread>,"sceKernelCreateThread"},
	{0x9fa03cd3,WrapI_I<sceKernelDeleteThread>,"sceKernelDeleteThread"},
	{0xBD123D9E,sceKernelDelaySysClockThread,"sceKernelDelaySysClockThread"},
	{0x1181E963,sceKernelDelaySysClockThreadCB,"sceKernelDelaySysClockThreadCB"},
	{0xceadeb47,WrapI_U<sceKernelDelayThread>,"sceKernelDelayThread"},
	{0x68da9e36,WrapI_U<sceKernelDelayThreadCB>,"sceKernelDelayThreadCB"},
	{0xaa73c935,sceKernelExitThread,"sceKernelExitThread"},
	{0x809ce29b,sceKernelExitDeleteThread,"sceKernelExitDeleteThread"},
	{0x94aa61ee,sceKernelGetThreadCurrentPriority,"sceKernelGetThreadCurrentPriority"},
	{0x293b45b8,sceKernelGetThreadId,"sceKernelGetThreadId"},
	{0x3B183E26,sceKernelGetThreadExitStatus,"sceKernelGetThreadExitStatus"},
	{0x52089CA1,sceKernelGetThreadStackFreeSize,"sceKernelGetThreadStackFreeSize"},
	{0xFFC36A14,WrapU_UU<sceKernelReferThreadRunStatus>,"sceKernelReferThreadRunStatus"},
	{0x17c1684e,WrapU_UU<sceKernelReferThreadStatus>,"sceKernelReferThreadStatus"},
	{0x2C34E053,WrapI_I<sceKernelReleaseWaitThread>,"sceKernelReleaseWaitThread"},
	{0x75156e8f,sceKernelResumeThread,"sceKernelResumeThread"},
	{0x3ad58b8c,&WrapU_V<sceKernelSuspendDispatchThread>,"sceKernelSuspendDispatchThread"},
	{0x27e22ec2,&WrapU_U<sceKernelResumeDispatchThread>,"sceKernelResumeDispatchThread"},
	{0x912354a7,&WrapI_I<sceKernelRotateThreadReadyQueue>,"sceKernelRotateThreadReadyQueue"},
	{0x9ACE131E,sceKernelSleepThread,"sceKernelSleepThread"},
	{0x82826f70,sceKernelSleepThreadCB,"sceKernelSleepThreadCB"},
	{0xF475845D,&WrapI_IUU<sceKernelStartThread>,"sceKernelStartThread"},
	{0x9944f31f,sceKernelSuspendThread,"sceKernelSuspendThread"},
	{0x616403ba,WrapI_I<sceKernelTerminateThread>,"sceKernelTerminateThread"},
	{0x383f7bcc,WrapI_I<sceKernelTerminateDeleteThread>,"sceKernelTerminateDeleteThread"},
	{0x840E8133,WrapI_IU<sceKernelWaitThreadEndCB>,"sceKernelWaitThreadEndCB"},
	{0xd13bde95,sceKernelCheckThreadStack,"sceKernelCheckThreadStack"},

	{0x94416130,WrapU_UUUU<sceKernelGetThreadmanIdList>,"sceKernelGetThreadmanIdList"},
	{0x57CF62DD,WrapU_U<sceKernelGetThreadmanIdType>,"sceKernelGetThreadmanIdType"},
	{0xBC80EC7C,WrapU_UUUU<sceKernelExtendThreadStack>, "sceKernelExtendThreadStack"},

	{0x82BC5777,WrapU64_V<sceKernelGetSystemTimeWide>,"sceKernelGetSystemTimeWide"},
	{0xdb738f35,WrapI_U<sceKernelGetSystemTime>,"sceKernelGetSystemTime"},
	{0x369ed59d,WrapU_V<sceKernelGetSystemTimeLow>,"sceKernelGetSystemTimeLow"},

	{0x8218B4DD,WrapI_U<sceKernelReferGlobalProfiler>,"sceKernelReferGlobalProfiler"},
	{0x627E6F3A,WrapI_U<sceKernelReferSystemStatus>,"sceKernelReferSystemStatus"},
	{0x64D4540E,WrapU_U<sceKernelReferThreadProfiler>,"sceKernelReferThreadProfiler"},

	//Fifa Street 2 uses alarms
	{0x6652b8ca,WrapI_UUU<sceKernelSetAlarm>,"sceKernelSetAlarm"},
	{0xB2C25152,WrapI_UUU<sceKernelSetSysClockAlarm>,"sceKernelSetSysClockAlarm"},
	{0x7e65b999,WrapI_I<sceKernelCancelAlarm>,"sceKernelCancelAlarm"},
	{0xDAA3F564,WrapI_IU<sceKernelReferAlarmStatus>,"sceKernelReferAlarmStatus"},

	{0xba6b92e2,WrapI_UUU<sceKernelSysClock2USec>,"sceKernelSysClock2USec"},
	{0x110dec9a,WrapI_UU<sceKernelUSec2SysClock>,"sceKernelUSec2SysClock"},
	{0xC8CD158C,WrapU64_U<sceKernelUSec2SysClockWide>,"sceKernelUSec2SysClockWide"},
	{0xE1619D7C,WrapI_UUUU<sceKernelSysClock2USecWide>,"sceKernelSysClock2USecWide"},

	{0x278C0DF5,WrapI_IU<sceKernelWaitThreadEnd>,"sceKernelWaitThreadEnd"},
	{0xd59ead2f,sceKernelWakeupThread,"sceKernelWakeupThread"}, //AI Go, audio?

	{0x0C106E53,0,"sceKernelRegisterThreadEventHandler"},
	{0x72F3C145,0,"sceKernelReleaseThreadEventHandler"},
	{0x369EEB6B,0,"sceKernelReferThreadEventHandlerStatus"},

	{0x349d6d6c,sceKernelCheckCallback,"sceKernelCheckCallback"},
	{0xE81CAF8F,sceKernelCreateCallback,"sceKernelCreateCallback"},
	{0xEDBA5844,sceKernelDeleteCallback,"sceKernelDeleteCallback"},
	{0xC11BA8C4,sceKernelNotifyCallback,"sceKernelNotifyCallback"},
	{0xBA4051D6,sceKernelCancelCallback,"sceKernelCancelCallback"},
	{0x2A3D44FF,sceKernelGetCallbackCount,"sceKernelGetCallbackCount"},
	{0x730ED8BC,sceKernelReferCallbackStatus,"sceKernelReferCallbackStatus"},

	{0x8125221D,&WrapI_CUU<sceKernelCreateMbx>,"sceKernelCreateMbx"},
	{0x86255ADA,&WrapI_I<sceKernelDeleteMbx>,"sceKernelDeleteMbx"},
	{0xE9B3061E,&WrapI_IU<sceKernelSendMbx>,"sceKernelSendMbx"},
	{0x18260574,&WrapI_IUU<sceKernelReceiveMbx>,"sceKernelReceiveMbx"},
	{0xF3986382,&WrapI_IUU<sceKernelReceiveMbxCB>,"sceKernelReceiveMbxCB"},
	{0x0D81716A,&WrapI_IU<sceKernelPollMbx>,"sceKernelPollMbx"},
	{0x87D4DD36,&WrapI_IU<sceKernelCancelReceiveMbx>,"sceKernelCancelReceiveMbx"},
	{0xA8E8C846,&WrapI_IU<sceKernelReferMbxStatus>,"sceKernelReferMbxStatus"},

	{0x7C0DC2A0,sceKernelCreateMsgPipe,"sceKernelCreateMsgPipe"},
	{0xF0B7DA1C,sceKernelDeleteMsgPipe,"sceKernelDeleteMsgPipe"},
	{0x876DBFAD,sceKernelSendMsgPipe,"sceKernelSendMsgPipe"},
	{0x7C41F2C2,sceKernelSendMsgPipeCB,"sceKernelSendMsgPipeCB"},
	{0x884C9F90,sceKernelTrySendMsgPipe,"sceKernelTrySendMsgPipe"},
	{0x74829B76,sceKernelReceiveMsgPipe,"sceKernelReceiveMsgPipe"},
	{0xFBFA697D,sceKernelReceiveMsgPipeCB,"sceKernelReceiveMsgPipeCB"},
	{0xDF52098F,sceKernelTryReceiveMsgPipe,"sceKernelTryReceiveMsgPipe"},
	{0x349B864D,sceKernelCancelMsgPipe,"sceKernelCancelMsgPipe"},
	{0x33BE4024,sceKernelReferMsgPipeStatus,"sceKernelReferMsgPipeStatus"},

	{0x56C039B5,WrapI_CIUUU<sceKernelCreateVpl>,"sceKernelCreateVpl"},
	{0x89B3D48C,WrapI_I<sceKernelDeleteVpl>,"sceKernelDeleteVpl"},
	{0xBED27435,WrapI_IUUU<sceKernelAllocateVpl>,"sceKernelAllocateVpl"},
	{0xEC0A693F,WrapI_IUUU<sceKernelAllocateVplCB>,"sceKernelAllocateVplCB"},
	{0xAF36D708,WrapI_IUU<sceKernelTryAllocateVpl>,"sceKernelTryAllocateVpl"},
	{0xB736E9FF,WrapI_IU<sceKernelFreeVpl>,"sceKernelFreeVpl"},
	{0x1D371B8A,WrapI_IU<sceKernelCancelVpl>,"sceKernelCancelVpl"},
	{0x39810265,WrapI_IU<sceKernelReferVplStatus>,"sceKernelReferVplStatus"},

	{0xC07BB470,sceKernelCreateFpl,"sceKernelCreateFpl"},
	{0xED1410E0,sceKernelDeleteFpl,"sceKernelDeleteFpl"},
	{0xD979E9BF,sceKernelAllocateFpl,"sceKernelAllocateFpl"},
	{0xE7282CB6,sceKernelAllocateFplCB,"sceKernelAllocateFplCB"},
	{0x623AE665,sceKernelTryAllocateFpl,"sceKernelTryAllocateFpl"},
	{0xF6414A71,sceKernelFreeFpl,"sceKernelFreeFpl"},
	{0xA8AA591F,sceKernelCancelFpl,"sceKernelCancelFpl"},
	{0xD8199E4C,sceKernelReferFplStatus,"sceKernelReferFplStatus"},

	{0x20fff560,WrapU_CU<sceKernelCreateVTimer>,"sceKernelCreateVTimer"},
	{0x328F9E52,WrapU_U<sceKernelDeleteVTimer>,"sceKernelDeleteVTimer"},
	{0xc68d9437,WrapU_U<sceKernelStartVTimer>,"sceKernelStartVTimer"},
	{0xD0AEEE87,WrapU_U<sceKernelStopVTimer>,"sceKernelStopVTimer"},
	{0xD2D615EF,WrapU_U<sceKernelCancelVTimerHandler>,"sceKernelCancelVTimerHandler"},
	{0xB3A59970,WrapU_UU<sceKernelGetVTimerBase>,"sceKernelGetVTimerBase"},
	{0xB7C18B77,WrapU64_U<sceKernelGetVTimerBaseWide>,"sceKernelGetVTimerBaseWide"},
	{0x034A921F,WrapU_UU<sceKernelGetVTimerTime>,"sceKernelGetVTimerTime"},
	{0xC0B3FFD2,WrapU64_U<sceKernelGetVTimerTimeWide>,"sceKernelGetVTimerTimeWide"},
	{0x5F32BEAA,WrapU_UU<sceKernelReferVTimerStatus>,"sceKernelReferVTimerStatus"},
	{0x542AD630,WrapU_UU<sceKernelSetVTimerTime>,"sceKernelSetVTimerTime"},
	{0xFB6425C3,WrapU_UU64<sceKernelSetVTimerTimeWide>,"sceKernelSetVTimerTimeWide"},
	{0xd8b299ae,WrapU_UUUU<sceKernelSetVTimerHandler>,"sceKernelSetVTimerHandler"},
	{0x53B00E9A,WrapU_UU64UU<sceKernelSetVTimerHandlerWide>,"sceKernelSetVTimerHandlerWide"},

	// Not sure if these should be hooked up. See below.
	{0x0E927AED, _sceKernelReturnFromTimerHandler, "_sceKernelReturnFromTimerHandler"},
	{0x532A522E, _sceKernelExitThread,"_sceKernelExitThread"},


	// Shouldn't hook this up. No games should import this function manually and call it.
	// {0x6E9EA350, _sceKernelReturnFromCallback,"_sceKernelReturnFromCallback"},
};

void Register_ThreadManForUser()
{
	RegisterModule("ThreadManForUser", ARRAY_SIZE(ThreadManForUser), ThreadManForUser);
}


const HLEFunction LoadExecForUser[] =
{
	{0x05572A5F,&WrapV_V<sceKernelExitGame>, "sceKernelExitGame"}, //()
	{0x4AC57943,&WrapU_U<sceKernelRegisterExitCallback>,"sceKernelRegisterExitCallback"},
	{0xBD2F1094,&WrapI_CU<sceKernelLoadExec>,"sceKernelLoadExec"},
	{0x2AC9954B,&WrapV_V<sceKernelExitGameWithStatus>,"sceKernelExitGameWithStatus"},
};

void Register_LoadExecForUser()
{
	RegisterModule("LoadExecForUser", ARRAY_SIZE(LoadExecForUser), LoadExecForUser);
}



const HLEFunction ExceptionManagerForKernel[] =
{
	{0x3FB264FC, 0, "sceKernelRegisterExceptionHandler"},
	{0x5A837AD4, 0, "sceKernelRegisterPriorityExceptionHandler"},
	{0x565C0B0E, sceKernelRegisterDefaultExceptionHandler, "sceKernelRegisterDefaultExceptionHandler"},
	{0x1AA6CFFA, 0, "sceKernelReleaseExceptionHandler"},
	{0xDF83875E, 0, "sceKernelGetActiveDefaultExceptionHandler"},
	{0x291FF031, 0, "sceKernelReleaseDefaultExceptionHandler"},
	{0x15ADC862, 0, "sceKernelRegisterNmiHandler"},
	{0xB15357C9, 0, "sceKernelReleaseNmiHandler"},
};

void Register_ExceptionManagerForKernel()
{
	RegisterModule("ExceptionManagerForKernel", ARRAY_SIZE(ExceptionManagerForKernel), ExceptionManagerForKernel);
}
