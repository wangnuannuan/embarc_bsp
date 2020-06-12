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
 * @file
 * @brief Unit test for arc_timer.c and arc_timer.h
 */

#include "embARC_test.h"
#include "arc/arc_timer.h"

UNIT_TEST(arc_timer, present) {
	UNIT_TEST_ASSERT_EQUAL(1, arc_timer_present(TIMER_0));
	UNIT_TEST_ASSERT_EQUAL(1, arc_timer_present(TIMER_1));
	UNIT_TEST_ASSERT_EQUAL(1, arc_timer_present(TIMER_RTC));
	UNIT_TEST_ASSERT_EQUAL(0, arc_timer_present(3));
	UNIT_TEST_ASSERT_EQUAL(0, arc_timer_present(0xffffffff));
}

UNIT_TEST(arc_timer, start) {
	if (arc_timer_present(TIMER_0)) {
		UNIT_TEST_ASSERT_EQUAL(0,
				       arc_timer_start(TIMER_0, 0xffffffff, 0xffffffff));
	}
	if (arc_timer_present(TIMER_1)) {
		UNIT_TEST_ASSERT_EQUAL(0,
				       arc_timer_start(TIMER_1, 0xffffffff, 0xffffffff));
	}
	if (arc_timer_present(TIMER_RTC)) {
		UNIT_TEST_ASSERT_EQUAL(0,
				       arc_timer_start(TIMER_RTC, 0xffffffff, 0xffffffff));
	}
	UNIT_TEST_ASSERT_EQUAL(-1, arc_timer_start(3, 0, 0));
	UNIT_TEST_ASSERT_EQUAL(-1, arc_timer_start(0xffffffff, 0, 0));
}

UNIT_TEST(arc_timer, stop) {
	if (arc_timer_present(TIMER_0)) {
		UNIT_TEST_ASSERT_EQUAL(0, arc_timer_stop(TIMER_0));
	}
	if (arc_timer_present(TIMER_1)) {
		UNIT_TEST_ASSERT_EQUAL(0, arc_timer_stop(TIMER_1));
	}
	if (arc_timer_present(TIMER_RTC)) {
		UNIT_TEST_ASSERT_EQUAL(0, arc_timer_stop(TIMER_RTC));
	}
	UNIT_TEST_ASSERT_EQUAL(-1, arc_timer_stop(3));
	UNIT_TEST_ASSERT_EQUAL(-1, arc_timer_stop(0xffffffff));
}

UNIT_TEST(arc_timer, current) {
	uint32_t val = 0xffffffff;

	if (arc_timer_present(TIMER_0)) {
		arc_timer_stop(TIMER_0);
		UNIT_TEST_ASSERT_EQUAL(0, arc_timer_current(TIMER_0, &val));
		UNIT_TEST_ASSERT_NOT_EQUAL(0, val);
	}
	if (arc_timer_present(TIMER_1)) {
		arc_timer_stop(TIMER_1);
		UNIT_TEST_ASSERT_EQUAL(0, arc_timer_current(TIMER_1, &val));
		UNIT_TEST_ASSERT_NOT_EQUAL(0, val);
	}
	if (arc_timer_present(TIMER_RTC)) {
		uint64_t val1 = 0xffffffff;
		arc_timer_stop(TIMER_RTC);
		UNIT_TEST_ASSERT_EQUAL(0, arc_timer_current(TIMER_RTC, &val1));
		UNIT_TEST_ASSERT_EQUAL(0, val1);
	}
	UNIT_TEST_ASSERT_EQUAL(-1, arc_timer_current(0xffffffff, &val));
}

UNIT_TEST(arc_timer, int_clear) {
	if (arc_timer_present(TIMER_0)) {
		UNIT_TEST_ASSERT_EQUAL(0, arc_timer_int_clear(TIMER_0));
	}
	if (arc_timer_present(TIMER_1)) {
		UNIT_TEST_ASSERT_EQUAL(0, arc_timer_int_clear(TIMER_1));
	}
	UNIT_TEST_ASSERT_EQUAL(-1, arc_timer_int_clear(0xffffffff));
}

#if defined(ARC_FEATURE_SEC_TIMER1_PRESENT) \
	|| defined(ARC_FEATURE_SEC_TIMER0_PRESENT)

UNIT_TEST(arc_timer, s_start) {
	if (arc_secure_timer_present(SECURE_TIMER_0)) {
		UNIT_TEST_ASSERT_EQUAL(0,
				       arc_secure_timer_start(SECURE_TIMER_0, 0xffffffff, 0xffffffff));
	}
	if (arc_secure_timer_present(SECURE_TIMER_1)) {
		UNIT_TEST_ASSERT_EQUAL(0,
				       arc_secure_timer_start(SECURE_TIMER_1, 0xffffffff, 0xffffffff));
	}
	UNIT_TEST_ASSERT_EQUAL(-1,
			       arc_secure_timer_start(TIMER_RTC, 0xffffffff, 0xffffffff));
}

UNIT_TEST(arc_timer, s_current) {
	uint32_t val = 0xffffffff;

	secure_timer_init();
	if (arc_secure_timer_present(SECURE_TIMER_0)) {
		secure_timer_stop(SECURE_TIMER_0);
		UNIT_TEST_ASSERT_EQUAL(0, arc_secure_timer_current(SECURE_TIMER_0, &val));
		UNIT_TEST_ASSERT_NOT_EQUAL(0, val);
		UNIT_TEST_ASSERT_EQUAL(0, arc_secure_timer_int_clear(SECURE_TIMER_0));
	}
	if (arc_secure_timer_present(SECURE_TIMER_1)) {
		secure_timer_stop(SECURE_TIMER_1);
		UNIT_TEST_ASSERT_EQUAL(0, arc_secure_timer_current(SECURE_TIMER_1, &val));
		UNIT_TEST_ASSERT_NOT_EQUAL(0, val);
		UNIT_TEST_ASSERT_EQUAL(0, arc_secure_timer_int_clear(SECURE_TIMER_1));
	}
	UNIT_TEST_ASSERT_EQUAL(0, arc_secure_timer_present(0xffffffff));
	UNIT_TEST_ASSERT_EQUAL(-1, secure_timer_stop(0xffffffff));
	UNIT_TEST_ASSERT_EQUAL(-1, arc_secure_timer_current(0xffffffff, &val));
	UNIT_TEST_ASSERT_EQUAL(-1, arc_secure_timer_int_clear(0xffffffff));
}
#endif

UNIT_TEST(arc_timer, calibrate) {
	UNIT_TEST_ASSERT_NOT_EQUAL(-1, arc_timer_calibrate_delay(1000000));
}
