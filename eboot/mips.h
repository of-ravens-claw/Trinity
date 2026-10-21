#ifndef __MIPS_H__
#define __MIPS_H__

#include "rpc.h"

extern void (*_sceKernelIcacheInvalidateAll)(void);
extern void (*_sceKernelDcacheInvalidateRange)(const void* p, unsigned int size);
extern void (*_sceKernelDcacheWritebackInvalidateAll)(void);

extern int (*_sceMeRequest)(int cmd, ...);

extern int (*_sceKermitRequest)(KermitRequest* request, int mode, int cmd, int argc, int is_callback, u64* resp);
extern int (*_sceKermitMemoryArgument)(KermitRequest* request, int arg, const void* data, int size, int mode);

int exploit_mips_kernel(void);

#endif
