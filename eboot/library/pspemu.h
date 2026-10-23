/* pspemu.h -- ScePspemu imports
 *
 * Copyright (C) 2019 TheFloW
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __PSPEMU_H__
#define __PSPEMU_H__

// SceRtcUser
#define ScePspemu_sceRtcGetCurrentClockLocalTime 0x19fc4

// SceDisplay
#define ScePspemu_sceDisplayWaitVblankStart 0x1a394

// SceNet
#define ScePspemu_sceNetRecv 0x1a934
#define ScePspemu_sceNetTerm 0x1aa94

// SceLibKernel
#define ScePspemu_sceKernelWaitSema 0x1ab94
#define ScePspemu_sceKernelCreateSema 0x1abc4

// SceThreadmgr
#define ScePspemu_sceKernelDeleteSema 0x1af54
#define ScePspemu_sceKernelSignalSema 0x1af64

// SceWlan
#define ScePspemu_sceWlanGetConfiguration 0x1af84

#endif
