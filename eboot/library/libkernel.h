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
#define SceLibKernel_sceKernelWaitSema 0xa951
#define SceLibKernel_sceClibMemcpy 0x1
#define SceLibKernel_sceKernelLoadStartModule 0xa729
#define SceLibKernel_sceKernelCreateThread 0xacc9
#define SceLibKernel_sceKernelWaitThreadEnd 0x16fd
#define SceLibKernel_sceKernelStartThread 0xa789

// Imports

// SceThreadmgr
#define SceLibKernel_sceKernelExitDeleteThread 0x61fc

// SceModulemgr
#define SceLibKernel_sceKernelGetModuleList 0x675c
#define SceLibKernel_sceKernelGetModuleInfo 0x676c

#endif
