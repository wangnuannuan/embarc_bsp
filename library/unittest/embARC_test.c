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

/* Copyright 2011,2012 Bas van den Berg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @ingroup EMBARC_TEST
 * @brief A light unit test framework based on CTest and Contiki App
 */

#include <setjmp.h>
#include <string.h>
#include "embARC_test.h"
#include "embARC_debug.h"

#define UNIT_TEST_ERR(fmt, ...) do {		   \
		EMBARC_PRINTF(fmt, ##__VA_ARGS__); \
		longjmp(unit_test_err, 1);	   \
} while (0)

typedef int32_t (*TEST_FILTER_FUNC_T)(struct unit_test *);

static jmp_buf unit_test_err;
static const char *suite_name;

/**
 * @brief Default test suite filter
 *
 * @param t	Pointer to test case
 *
 * @return 1
 */
static int32_t suite_all(struct unit_test *t)
{
	return 1;
}

/**
 * @brief a Test suite filter by name
 *
 * @param t Pinter to test case
 *
 * @return 1 in the suite, 0 not in the suite
 */
static int32_t suite_filter(struct unit_test *t)
{
	return strncmp(suite_name, t->ssname, strlen(suite_name)) == 0;
}

/**
 * @brief Assert when the real string isn't the expected string
 *
 * @param expect	Expected string
 * @param real	Real string
 * @param caller The caller file name string
 * @param line	Line number in the caller file
 */
void unit_test_assert_str(const char *expect, const char *real, const char *caller, int32_t line)
{
	if ((expect == NULL && real != NULL) ||
	    (expect != NULL && real == NULL) ||
	    (expect && real && strcmp(expect, real) != 0)) {
		UNIT_TEST_ERR("%s:%d  expected '%s', got '%s'\n", caller, line, expect, real);
	}
}

/**
 * @brief Assert when the real data area doesn't equal the expected data area
 *
 * @param expect		Expected data area
 * @param expsize	Data size of expected data area
 * @param real		Real data area
 * @param realsize	Data size of real data area
 * @param caller	The caller file
 * @param line		Line number in the caller file
 */
void unit_test_assert_data(			\
	const uint8_t *expect, int32_t expsize,	\
	const uint8_t *real, int32_t realsize,	\
	const char *caller, int32_t line)
{
	int32_t i;

	if (expsize != realsize) {
		UNIT_TEST_ERR("%s:%d  expected %d bytes, got %d\n", \
			      caller, line, expsize, realsize);
	}
	for (i = 0; i < expsize; i++) {
		if (expect[i] != real[i]) {
			UNIT_TEST_ERR("%s:%d expected 0x%02x at offset %d got 0x%02x\n", \
				      caller, line, expect[i], i, real[i]);
		}
	}
}

/**
 * @brief Assert when the real value doesn't equal the expected value
 *
 * @param expect	Expected value
 * @param real	Real value
 * @param caller	The caller file
 * @param line		Line number in the caller file
 */
void unit_test_assert_equal(int32_t expect, int32_t real, const char *caller, int32_t line)
{
	if ((expect) != (real)) {
		UNIT_TEST_ERR("%s:%d  expected %ld, got %ld\n", caller, line, expect, real);
	}
}

/**
 * @brief Assert when the real value equal the expected value
 *
 * @param expect	Expected value
 * @param real	Real value
 * @param caller	The caller file
 * @param line		Line number in the caller file
 */
void unit_test_assert_not_equal(int32_t expect, int32_t real, const char *caller, int32_t line)
{
	if ((expect) == (real)) {
		UNIT_TEST_ERR("%s:%d  should not be %ld\n", caller, line, real);
	}
}

/**
 * @brief Assert when the real value is not null
 *
 * @param real	Real value
 * @param caller	The caller file
 * @param line		Line number in the caller file
 */
void unit_test_assert_null(void *real, const char *caller, int32_t line)
{
	if ((real) != NULL) {
		UNIT_TEST_ERR("%s:%d  should be NULL\n", caller, line);
	}
}

/**
 * @brief Assert when the real value is null
 *
 * @param real	Real value
 * @param caller	The caller file
 * @param line		Line number in the caller file
 */
void unit_test_assert_not_null(const void *real, const char *caller, int32_t line)
{
	if ((real) == NULL) {
		UNIT_TEST_ERR("%s:%d  should not be NULL\n", caller, line);
	}
}

/**
 * @brief Assert when the real value is not true
 *
 * @param real	Real value
 * @param caller	The caller file
 * @param line		Line number in the caller file
 */
void unit_test_assert_true(int32_t real, const char *caller, int32_t line)
{
	if ((real) == 0) {
		UNIT_TEST_ERR("%s:%d  should be true\n", caller, line);
	}
}

/**
 * @brief Assert when the real value is not false
 *
 * @param real	Real value
 * @param caller	The caller file
 * @param line		Line number in the caller file
 */
void unit_test_assert_false(int32_t real, const char *caller, int32_t line)
{
	if ((real) != 0) {
		UNIT_TEST_ERR("%s:%d  should be false\n", caller, line);
	}
}

/**
 * @brief Assert when fail unconditionally
 *
 * @param caller	The caller file
 * @param line		Line number in the caller file
 */
void unit_test_assert_fail(const char *caller, int32_t line)
{
	UNIT_TEST_ERR("%s:%d  shouldn't come here\n", caller, line);
}

extern struct unit_test _f_embarc_unittest[];
extern struct unit_test _e_embarc_unittest[];
/**
 * @brief Run unit test
 *
 * @param test_suite The test suite need to run, NULL for all
 *
 * @return The number of failed test cases
 */
int32_t unit_test_run(const char *test_suite)
{
	int32_t total = 0;
	int32_t num_ok = 0;
	int32_t num_fail = 0;
	int32_t num_skip = 0;
	int32_t index = 1;
	TEST_FILTER_FUNC_T filter = suite_all;
	struct unit_test *unit_test_begin = _f_embarc_unittest;
	struct unit_test *unit_test_end = _e_embarc_unittest;
	struct unit_test *test;

	if (test_suite != NULL) {
		filter = suite_filter;
		suite_name = test_suite;
	} else {
		return -1;
	}

	for (test = unit_test_begin; (uint32_t)test < (uint32_t)unit_test_end; test++) {
		if (test->magic != US_UNIT_TEST_MAGIC) {
			EMBARC_PRINTF("embarc_unittest section corrupted:start:%x, end:%x, test:%x\n", \
				      unit_test_begin, unit_test_end, test);
			return -1;
		}
		if (filter(test)) {
			total++;
		}
	}
	EMBARC_PRINTF("----embARC unit tests----\n");
	EMBARC_PRINTF("start:0x%x, end:0x%x, total:%d\n", \
		      unit_test_begin, unit_test_end, total);
	for (test = unit_test_begin; (uint32_t)test < (uint32_t)unit_test_end; test++) {
		if (filter(test)) {
			EMBARC_PRINTF("TEST %d/%d %s:%s\n ", \
				      index, total, test->ssname, test->ttname);
			if (test->skip) {
				EMBARC_PRINTF("[SKIPPED]\n");
				num_skip++;
			} else {
				int32_t result = setjmp(unit_test_err);
				if (result == 0) {
					if (test->setup) {
						test->setup(test->data);
					}
					if (test->data) {
						test->run(test->data);
					} else {
						test->run();
					}
					if (test->teardown) {
						test->teardown(test->data);
					}
					EMBARC_PRINTF("[OK]\n");
					num_ok++;
				} else {
					EMBARC_PRINTF("[FAIL]\n");
					num_fail++;
				}
			}
			index++;
		}
	}
	EMBARC_PRINTF("RESULTS: %d tests (%d ok, %d failed, %d skipped)\n", \
		      total, num_ok, num_fail, num_skip);

	return num_fail;
}
