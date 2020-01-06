/* ------------------------------------------
 * Copyright (c) 2019, Synopsys, Inc. All rights reserved.

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
   --------------------------------------------- */
#include "embARC_test.h"
#include "arc/arc_mp.h"

static EMBARC_ALIGNED(4) uint8_t test_mp_core0_stack[16];
static uint32_t test_mp_result;

static void test_mp_core0_func(uint32_t core, void *arg)
{
	if (core == 0 && arg == NULL && arc_cpu_sp == &test_mp_core0_stack[16]) {
		test_mp_result = 1;
	}  else {
		test_mp_result = 0;
	}
}

UNIT_TEST(arc_mp, mp){
	ARC_SPINLOCK_T lock;

	arc_spinlock_get(&lock);
	arc_spinlock_release(&lock);
	arc_spinlock_try(&lock);
	arc_spinlock_release(&lock);
	arc_cpu_wake_flag = 0;
	test_mp_result = 0;
	arc_start_slave_cpu(0, test_mp_core0_stack, 16, test_mp_core0_func, NULL);
	arc_slave_start(0);
	UNIT_TEST_ASSERT_EQUAL(test_mp_result, 1);
}