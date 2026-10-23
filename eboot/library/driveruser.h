/* driveruser.h -- SceDriverUser exports
 *
 * Copyright (C) 2019 TheFloW
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __DRIVERUSER_H__
#define __DRIVERUSER_H__

// SceRtcUser
#define SceDriverUser_sceRtcGetCurrentClockLocalTime 0x325d
#define SceDriverUser_sceRtcConvertLocalTimeToUtc 0x326d

// SceDisplayUser
#define SceDriverUser_sceDisplayGetFrameBuf 0x3345
#define SceDriverUser_sceDisplaySetFrameBuf 0x3329

// SceAppMgrUser
#define SceDriverUser_sceAppMgrPspSaveDataGetParams 0x3869

#endif
