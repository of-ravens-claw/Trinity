/* pspemu.h -- ScePspemu exports/imports
 *
 * Copyright (C) 2019 TheFloW
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __PSPEMU_H__
#define __PSPEMU_H__

// Imports

// SceRtcUser
#define ScePspemu_sceRtcGetCurrentClockLocalTime 0x671c

// SceDisplay
#define ScePspemu_sceDisplayWaitVblankStart 0x6aec

// SceNet
#define ScePspemu_sceNetRecv 0x70cc
#define ScePspemu_sceNetTerm 0x722c

// SceLibKernel
#define ScePspemu_sceKernelWaitSema 0x732c
#define ScePspemu_sceKernelCreateSema 0x735c

// SceThreadmgr
#define ScePspemu_sceKernelDeleteSema 0x76fc
#define ScePspemu_sceKernelSignalSema 0x770c

// SceWlan
#define ScePspemu_sceWlanGetConfiguration 0x772c

#endif
