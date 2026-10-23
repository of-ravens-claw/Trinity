/* libkernel.h -- SceLibKernel exports/imports
 *
 * Copyright (C) 2019 TheFloW
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __LIBKERNEL_H__
#define __LIBKERNEL_H__

// Exports

// SceLibKernel
#define SceLibKernel_sceKernelWaitSema 0x9bf9
#define SceLibKernel_sceClibMemcpy 0x1
#define SceLibKernel_sceKernelLoadStartModule 0x99d1
#define SceLibKernel_sceKernelCreateThread 0x9f71
#define SceLibKernel_sceKernelWaitThreadEnd 0x1735
#define SceLibKernel_sceKernelStartThread 0x9a31

// Imports

// SceThreadmgr
#define SceLibKernel_sceKernelExitDeleteThread 0x55dc

// SceModulemgr
#define SceLibKernel_sceKernelGetModuleList 0x5b3c
#define SceLibKernel_sceKernelGetModuleInfo 0x5b4c

#endif
