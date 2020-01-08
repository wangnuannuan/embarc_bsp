/* ------------------------------------------
 * Copyright (c) 2016, Synopsys, Inc. All rights reserved.

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

/**
 * \file
 * \brief Unit test for arc_exception.c and arc_exception.h
 */

#include "embARC_test.h"
#include "arc/arc_exception.h"

#ifndef INTNO_SWI
#define INTNO_SWI 20
#endif

#define TEST_COUNT_ERROR 0xffff

static volatile uint32_t test_count;
static uint32_t int_handler_length = EMBARC_ARRAY_COUNT(exc_int_handler_table);
static uint32_t exc_entry_length = EMBARC_ARRAY_COUNT(exc_entry_table);

static void sw_exc_trigger(uint8_t exc_no, uint32_t exc_cause, uint32_t exc_parm)
{
	uint32_t val = 0;

	val = exc_parm | (exc_cause << 8) | (exc_no << 16);

	arc_aux_write(AUX_ECR, val);
	Asm(
		"lr %0, [%1]\n"
		"sr %0, [%2]\n"
		"mov %0, %3 \n"
		"kflag %0   \n"
		"mov %0, test_ret_from_exception \n"
		"sr  %0, [%4]  \n"
		"b exc_entry_cpu  \n"
		"test_ret_from_exception:  \n"
		:
		: "r" (val), "i" (AUX_STATUS32), "i" (AUX_ERSTATUS),
		"i" (AUX_STATUS_MASK_AE), "i" (AUX_ERRET));
}

static void exc_entry_test(void)
{
}

static void exc_handler_test(void *exc_frame)
{
	test_count = 1;
}

static void int_handler_test(void *ptr)
{
	test_count = 2;
}

static void check_processor_frame(PROCESSOR_FRAME_T *frame)
{
	INT_EXC_FRAME_T *exc_frame =  &frame->exc_frame;
	CALLEE_FRAME_T *callee_regs = &frame->callee_regs;

	UNIT_TEST_ASSERT_EQUAL((uint32_t)check_processor_frame, exc_frame->r0);
	UNIT_TEST_ASSERT_EQUAL(1, exc_frame->r1);
	UNIT_TEST_ASSERT_EQUAL(2, exc_frame->r2);
	UNIT_TEST_ASSERT_EQUAL(3, exc_frame->r3);
	UNIT_TEST_ASSERT_EQUAL(10, exc_frame->r10);
	UNIT_TEST_ASSERT_EQUAL(11, exc_frame->r11);
	UNIT_TEST_ASSERT_EQUAL(12, exc_frame->r12);
	UNIT_TEST_ASSERT_EQUAL(13, callee_regs->r13);
#ifndef ARC_FEATURE_RF16
	UNIT_TEST_ASSERT_EQUAL(4, exc_frame->r4);
	UNIT_TEST_ASSERT_EQUAL(5, exc_frame->r5);
	UNIT_TEST_ASSERT_EQUAL(6, exc_frame->r6);
	UNIT_TEST_ASSERT_EQUAL(7, exc_frame->r7);
	UNIT_TEST_ASSERT_EQUAL(8, exc_frame->r8);
	UNIT_TEST_ASSERT_EQUAL(9, exc_frame->r9);
	UNIT_TEST_ASSERT_EQUAL(14, callee_regs->r14);
	UNIT_TEST_ASSERT_EQUAL(15, callee_regs->r15);
	UNIT_TEST_ASSERT_EQUAL(16, callee_regs->r16);
	UNIT_TEST_ASSERT_EQUAL(17, callee_regs->r17);
	UNIT_TEST_ASSERT_EQUAL(18, callee_regs->r18);
	UNIT_TEST_ASSERT_EQUAL(19, callee_regs->r19);
	UNIT_TEST_ASSERT_EQUAL(20, callee_regs->r20);
	UNIT_TEST_ASSERT_EQUAL(21, callee_regs->r21);
	UNIT_TEST_ASSERT_EQUAL(22, callee_regs->r22);
	UNIT_TEST_ASSERT_EQUAL(23, callee_regs->r23);
	UNIT_TEST_ASSERT_EQUAL(24, callee_regs->r24);
	UNIT_TEST_ASSERT_EQUAL(25, callee_regs->r25);
#endif
	UNIT_TEST_ASSERT_EQUAL(26, exc_frame->gp);
	UNIT_TEST_ASSERT_EQUAL(27, exc_frame->fp);
	UNIT_TEST_ASSERT_EQUAL(29, exc_frame->ilink);
	UNIT_TEST_ASSERT_EQUAL(30, exc_frame->r30);
	UNIT_TEST_ASSERT_EQUAL(31, exc_frame->blink);
	UNIT_TEST_ASSERT_EQUAL(60, exc_frame->lp_count);
	UNIT_TEST_ASSERT_EQUAL(AUX_LP_START << 2, exc_frame->lp_start);
	UNIT_TEST_ASSERT_EQUAL(AUX_LP_END << 2, exc_frame->lp_end);
#if ARC_FEATURE_CODE_DENSITY
	UNIT_TEST_ASSERT_EQUAL(AUX_EI_BASE << 2, exc_frame->ei);
	UNIT_TEST_ASSERT_EQUAL(AUX_LDI_BASE << 2, exc_frame->ldi);
	UNIT_TEST_ASSERT_EQUAL(AUX_JLI_BASE << 2, exc_frame->jli);
#endif
#if ARC_FEATURE_DSP || ARC_FEATURE_FPU || ARC_FEATURE_MPU_OPTION_NUM > 6
	UNIT_TEST_ASSERT_EQUAL(58, exc_frame->r58);
	UNIT_TEST_ASSERT_EQUAL(59, exc_frame->r59);
#endif
#if ARC_FEATURE_FPU
	FPU_EXT_FRAME_T *fpu_regs;
	fpu_regs = &callee_regs->fpu_ext_regs;
	UNIT_TEST_ASSERT_EQUAL(0, fpu_regs->fpu_status);
	UNIT_TEST_ASSERT_EQUAL(AUX_FPU_CTRL, fpu_regs->fpu_ctrl);
#if ARC_FEATURE_FPU_DA
	UNIT_TEST_ASSERT_EQUAL(AUX_FPU_DPFP2H, fpu_regs->dpfp2h);
	UNIT_TEST_ASSERT_EQUAL(AUX_FPU_DPFP2L, fpu_regs->dpfp2l);
	UNIT_TEST_ASSERT_EQUAL(AUX_FPU_DPFP1H, fpu_regs->dpfp1h);
	UNIT_TEST_ASSERT_EQUAL(AUX_FPU_DPFP1L, fpu_regs->dpfp1l);
#endif
#endif
#if ARC_FEATURE_DSP
	DSP_EXT_FRAME_T *dsp_regs;
	dsp_regs = &callee_regs->dsp_regs;
	 /* When dsp_accshift==full, dsp_regs->dsp_ctrl = 0x1f, else  dsp_regs->dsp_ctrl = 0x16 */
	UNIT_TEST_ASSERT_EQUAL(0x1f, dsp_regs->dsp_ctrl);
	UNIT_TEST_ASSERT_EQUAL(58, dsp_regs->acc0_lo);
	UNIT_TEST_ASSERT_EQUAL(129, dsp_regs->acc0_glo);
	UNIT_TEST_ASSERT_EQUAL(59, dsp_regs->acc0_hi);
	UNIT_TEST_ASSERT_EQUAL(131, dsp_regs->acc0_ghi);
#if ARC_FEATURE_DSP_COMPLEX
	UNIT_TEST_ASSERT_EQUAL(0, dsp_regs->dsp_fft_ctrl);
	UNIT_TEST_ASSERT_EQUAL(AUX_DSP_BFLY0, dsp_regs->dsp_bfly0);
#endif
#endif

}

extern void test_processor_frame(void *entry);

UNIT_TEST(int_exc, int_exc_frame){
	UNIT_TEST_ASSERT_NOT_EQUAL(0, ARC_PROCESSOR_FRAME_T_SIZE);
	UNIT_TEST_ASSERT_NOT_EQUAL(0, ARC_INT_EXC_FRAME_T_SIZE);
	UNIT_TEST_ASSERT_NOT_EQUAL(0, ARC_CALLEE_FRAME_T_SIZE);
	test_processor_frame(check_processor_frame);
}

UNIT_TEST(int_exc, exc_entry){
	EXC_ENTRY_T old;

	old = exc_entry_get(exc_entry_length + 1);
	UNIT_TEST_ASSERT_NULL(old);
	old = exc_entry_get(EXC_NO_TRAP);
	UNIT_TEST_ASSERT_NOT_NULL(old);
	UNIT_TEST_ASSERT_EQUAL(-1, exc_entry_install(exc_entry_length, exc_entry_test));
	UNIT_TEST_ASSERT_EQUAL(0, exc_entry_install(EXC_NO_TRAP, exc_entry_test));
	UNIT_TEST_ASSERT_EQUAL((uint32_t)exc_entry_test,
			       (uint32_t)exc_entry_get(EXC_NO_TRAP));
	UNIT_TEST_ASSERT_EQUAL(0, exc_entry_install(EXC_NO_TRAP, old));
}

UNIT_TEST(int_exc, exc_handler){
	test_count = 0;
	EXC_HANDLER_T old;
	old = exc_handler_get(exc_entry_length);
	UNIT_TEST_ASSERT_NULL(old);
	old = exc_handler_get(EXC_NO_TRAP);
	UNIT_TEST_ASSERT_NOT_NULL(old);
	UNIT_TEST_ASSERT_EQUAL(-1, exc_handler_install(exc_entry_length, exc_handler_test));
}

UNIT_TEST(int_exc, int_handler){
	test_count = 0;
	INT_HANDLER_T old;

	old = int_handler_get(int_handler_length + 1);
	UNIT_TEST_ASSERT_NULL(old);
	old = int_handler_get(NUM_EXC_CPU - 1);
	UNIT_TEST_ASSERT_NULL(old);
	old = int_handler_get(INTNO_SWI);
	UNIT_TEST_ASSERT_NOT_NULL(old);
	UNIT_TEST_ASSERT_EQUAL(-1,
			       int_handler_install(0, int_handler_test));
	UNIT_TEST_ASSERT_EQUAL(0, int_handler_install(INTNO_SWI, int_handler_test));
	UNIT_TEST_ASSERT_EQUAL((uint32_t)int_handler_test,
			       (uint32_t)int_handler_get(INTNO_SWI));
	cpu_lock();
	UNIT_TEST_ASSERT_EQUAL(-1, int_enable(int_handler_length));
	int_pri_set(INTNO_SWI, INT_PRI_MIN);
	int_enable(INTNO_SWI);
	int_sw_trigger(INTNO_SWI);
	Asm("nop_s");
	Asm("nop_s");
	UNIT_TEST_ASSERT_EQUAL(0, test_count);
	cpu_unlock();
	Asm("nop_s");
	Asm("nop_s");
	Asm("nop_s");
	UNIT_TEST_ASSERT_EQUAL(2, test_count);
}

UNIT_TEST(int_exc, inline_fs){
	UNIT_TEST_ASSERT_EQUAL(0, int_level_config(INTNO_SWI, 0));
	UNIT_TEST_ASSERT_EQUAL(0, int_level_get(INTNO_SWI));
	UNIT_TEST_ASSERT_EQUAL(0, int_enable(INTNO_SWI));
	UNIT_TEST_ASSERT_EQUAL(1, int_enabled(INTNO_SWI));
	UNIT_TEST_ASSERT_EQUAL(0, int_disable(INTNO_SWI));
	UNIT_TEST_ASSERT_EQUAL(0, int_enabled(INTNO_SWI));

	UNIT_TEST_ASSERT_EQUAL(-1, int_ipm_set(INT_PRI_MIN - 1));
	UNIT_TEST_ASSERT_EQUAL(0, int_ipm_set(INT_PRI_MIN + 1));
	UNIT_TEST_ASSERT_EQUAL(INT_PRI_MIN + 1, int_ipm_get());

	UNIT_TEST_ASSERT_EQUAL(0, int_pri_set(INTNO_SWI, -2));
	UNIT_TEST_ASSERT_EQUAL(-2, int_pri_get(INTNO_SWI));
	UNIT_TEST_ASSERT_EQUAL(0, int_secure_set(INTNO_SWI, 0));
	UNIT_TEST_ASSERT_EQUAL(-1, int_secure_set(int_handler_length + 1, 0));
	int_disable(INTNO_SWI);
	UNIT_TEST_ASSERT_EQUAL(0, int_probe(INTNO_SWI));

	UNIT_TEST_ASSERT_EQUAL(-1, int_level_config(int_handler_length, 0));
	UNIT_TEST_ASSERT_EQUAL(-1, int_level_get(int_handler_length));
	UNIT_TEST_ASSERT_EQUAL(-1, int_pri_set(int_handler_length, -2));
	UNIT_TEST_ASSERT_EQUAL(0, int_pri_get(int_handler_length));
	UNIT_TEST_ASSERT_EQUAL(-1, int_probe(int_handler_length));
	UNIT_TEST_ASSERT_EQUAL(-1, int_disable(int_handler_length));

	int_sw_trigger(INTNO_SWI);
	UNIT_TEST_ASSERT_EQUAL(1, int_probe(INTNO_SWI));
	UNIT_TEST_ASSERT_EQUAL(-1, int_enabled(int_handler_length));

	UNIT_TEST_ASSERT_EQUAL(-1, int_sw_trigger(int_handler_length));
}

UNIT_TEST(int_exc, arc_excs){
	uint32_t exception_causes[14] =
	{ 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x4, 0x8, 0x10, 0x11, 0x12, 0x13, 0x80 };
	uint32_t exception_parameters[11] =
	{ 0x1, 0x2, 0x3, 0x4, 0x8, 0x10, 0x20, 0x24, 0x40, 0x44, 0x80 };

	for (uint32_t i = 0; i < NUM_EXC_CPU; i++) {
		for (uint32_t j = 0; j < 14; j++) {
			for (uint32_t m = 0; m < 11; m++) {
				sw_exc_trigger(i, exception_causes[j], exception_parameters[m]);
			}
		}
	}
	UNIT_TEST_ASSERT_EQUAL(0,
			       exc_handler_install(EXC_NO_TRAP, exc_handler_test));
	UNIT_TEST_ASSERT_EQUAL((uint32_t)exc_handler_test,
			       (uint32_t)exc_handler_get(EXC_NO_TRAP));
}

UNIT_TEST(int_exc, firq) {
	arc_firq_stack_set(NULL);
}
