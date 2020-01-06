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
   --------------------------------------------- */

/**
 * @file
 * @brief Unit test for arc_mpu.c and arc_mpu.h
 */
#include "embARC_test.h"
#include "arc/arc_mpu.h"

UNIT_TEST(arc_mpu, arc_mpu){
	uint32_t r_size_lshift;
	uint32_t r_addr_start;

	UNIT_TEST_ASSERT_EQUAL(0x00000000, arc_aux_read(AUX_MPU_EN)); /* Reset */
	/* arc_mpu_enable();  MPU is always enabled*/

	arc_mpu_default(0);
	UNIT_TEST_ASSERT_EQUAL(0x00000000, arc_aux_read(AUX_MPU_EN)); /* MPU enable bit - 0 */

	arc_mpu_disable();
	UNIT_TEST_ASSERT_EQUAL(0x00000000, arc_aux_read(AUX_MPU_EN) & AUX_MPU_EN_ENABLE); /* MPU enable bit - 0 */
	UNIT_TEST_ASSERT_EQUAL(0, arc_mpu_in_region(0, 0x00000000, 32 * 8));

	arc_mpu_enable();
	UNIT_TEST_ASSERT_EQUAL(AUX_MPU_EN_ENABLE, arc_aux_read(AUX_MPU_EN) & AUX_MPU_EN_ENABLE); /* MPU enable bit - 1 */

	/* Check error*/
	arc_mpu_region_config(ARC_FEATURE_MPU_REGIONS, 0, 0, 0);
	arc_mpu_region_config(ARC_FEATURE_MPU_REGIONS - 1, 0, 0, 0);
	arc_mpu_region_config(0, 0, 2049, 0); /* 2^n < size < 2^n+1 bits=n+1 MPU_RDP*/

	for (uint32_t i = 0; i < ARC_FEATURE_MPU_REGIONS; i++) {
		arc_mpu_region_config(0, i, 32 * 8, ARC_MPU_REGION_ALL_ATTR); /* AUX_MPU_RDP0 */
		r_addr_start = arc_aux_read(AUX_MPU_RDB0 + 2 * i) & (~AUX_MPU_VALID_MASK);
		r_size_lshift = arc_aux_read(AUX_MPU_RDP0 + 2 * i) & AUX_MPU_ATTR_MASK;
		r_size_lshift = (r_size_lshift & 0x3) | ((r_size_lshift >> 7) & 0x1C);
		UNIT_TEST_ASSERT_EQUAL(1, arc_mpu_in_region(i, r_addr_start, r_addr_start));
		UNIT_TEST_ASSERT_EQUAL(0, arc_mpu_in_region(i, r_addr_start, r_addr_start  + (1 << (r_size_lshift + 1)) + 1));
	}

	UNIT_TEST_ASSERT_EQUAL(0, arc_mpu_probe(0x00000000));
	UNIT_TEST_ASSERT_EQUAL(-1, arc_mpu_probe(0x0004096));
}

UNIT_TEST(arc_mpu, api){
	uint32_t r_size_lshift;
	uint32_t r_addr_start;

	arc_mpu_region_config(0, 0, REGION_32B, ARC_MPU_REGION_ALL_ATTR); /* AUX_MPU_RDP0 */
	r_addr_start = arc_aux_read(AUX_MPU_RDB0 + 2 * 0) & (~AUX_MPU_VALID_MASK);
	r_size_lshift = arc_aux_read(AUX_MPU_RDP0 + 2 * 0) & AUX_MPU_ATTR_MASK;
	r_size_lshift = (r_size_lshift & 0x3) | ((r_size_lshift >> 7) & 0x1C);

	volatile uint32_t *const test_data = (uint32_t *) r_addr_start;
	*test_data = 1;                         /* Write test: The region is writable when in user or kernal model */
	uint32_t read_data = *test_data;
	UNIT_TEST_ASSERT_EQUAL(1, read_data);   /* Read test: The region is readable when in user or kernal model */
}