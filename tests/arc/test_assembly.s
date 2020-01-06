/* ------------------------------------------
 * Copyright (c) 2017, Synopsys, Inc. All rights reserved.

 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:

 * 1) Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.

 * 3) Neither the name of the Synopsys, Inc., nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
--------------------------------------------- */
#define __ASSEMBLY__
#include "arc/arc.h"
#include "arc/arc_asm_common.h"

	.file "test_assembly.s"

	.text
	.global test_processor_frame
	.align 4
test_processor_frame:
	lr r1, [AUX_STATUS32]
	kflag 0
	EXCEPTION_PROLOGUE
	SAVE_CALLEE_REGS
#if ARC_FEATURE_CODE_DENSITY
	mov r1, (AUX_JLI_BASE << 2)
	sr r1, [AUX_JLI_BASE]
	mov r1, (AUX_LDI_BASE << 2)
	sr r1, [AUX_LDI_BASE]
	mov r1, (AUX_EI_BASE << 2)
	sr r1, [AUX_EI_BASE]
#endif
#if ARC_FEATURE_FPU
	mov r1, AUX_FPU_CTRL
	sr r1, [AUX_FPU_CTRL]
	mov r1, AUX_FPU_STATUS
	sr r1, [AUX_FPU_STATUS]
#endif
#if ARC_FEATURE_FPU_DA
	mov r1, AUX_FPU_DPFP1L
	sr r1, [AUX_FPU_DPFP1L]
	mov r1, AUX_FPU_DPFP1H
	sr r1, [AUX_FPU_DPFP1H]
	mov r1, AUX_FPU_DPFP2L
	sr r1, [AUX_FPU_DPFP2L]
	mov r1, AUX_FPU_DPFP2H
	sr r1, [AUX_FPU_DPFP2H]
#endif
#if ARC_FEATURE_DSP
	mov r1, 0x1f
	sr r1, [AUX_DSP_CTRL]
	mov r1, AUX_ACC0_LO
	sr r1, [AUX_ACC0_LO]
	mov r1, AUX_ACC0_GLO
	sr r1, [AUX_ACC0_GLO]
	mov r1, AUX_ACC0_HI
	sr r1, [AUX_ACC0_HI]
	mov r1, AUX_ACC0_GHI
	sr r1, [AUX_ACC0_GHI]
#if ARC_FEATURE_DSP_COMPLEX
	mov r1, AUX_DSP_BFLY0
	sr r1, [AUX_DSP_BFLY0]
	mov r1, AUX_DSP_FFT_CTRL
	sr r1, [AUX_DSP_FFT_CTRL]
#endif
#endif
	mov r60, 60
	mov r1, (AUX_LP_START << 2)
	sr r1, [AUX_LP_START]
	mov r1, (AUX_LP_END << 2)
	sr r1, [AUX_LP_END]
	mov r1, AUX_ERSTATUS
	sr  r1, [AUX_ERSTATUS]
	mov r1, AUX_ERRET
	sr r1, [AUX_ERRET]
	mov r1, 1
	mov r2, 2
	mov r3, 3
#ifndef ARC_FEATURE_RF16
	mov r4, 4
	mov r5, 5
	mov r6, 6
	mov r7, 7
	mov r8, 8
	mov r9, 9
#endif
	mov r10, 10
	mov r11, 11
	mov r12, 12
	mov r13, 13
	mov r14, 14
	mov r15, 15
#ifndef ARC_FEATURE_RF16
	mov r16, 16
	mov r17, 17
	mov r18, 18
	mov r19, 19
	mov r20, 20
	mov r21, 21
	mov r22, 22
	mov r23, 23
	mov r24, 24
	mov r25, 25
#endif
	mov r26, 26
	mov r27, 27
	mov r29, 29
	mov r30, 30
	mov r31, 31
#if ARC_FEATURE_FPU || ARC_FEATURE_DSP || ARC_FEATURE_MPU_OPTION_NUM > 6
	mov r58, 58
	mov r59, 59
#endif
	EXCEPTION_PROLOGUE
	SAVE_CALLEE_REGS
	mov r1, r0
	mov r0, sp
	jl [r1]
	RESTORE_CALLEE_REGS
	EXCEPTION_EPILOGUE
	RESTORE_CALLEE_REGS
	EXCEPTION_EPILOGUE
	kflag r1
	j [blink]