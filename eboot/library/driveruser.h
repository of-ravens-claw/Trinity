/* driveruser.h -- SceDriverUser exports/imports
 *
 * Copyright (C) 2019 TheFloW
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __DRIVERUSER_H__
#define __DRIVERUSER_H__

// Exports

// SceRtcUser
#define SceDriverUser_sceRtcGetCurrentClockLocalTime 0x41d1
#define SceDriverUser_sceRtcConvertLocalTimeToUtc 0x41e1

// SceDisplayUser
#define SceDriverUser_sceDisplayGetFrameBuf 0x42a9
#define SceDriverUser_sceDisplaySetFrameBuf 0x428d

// SceAppMgrUser
#define SceDriverUser_sceAppMgrPspSaveDataGetParams 0x47f1

#endif
