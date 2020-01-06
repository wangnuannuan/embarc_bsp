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
 * @brief Unit test for arc_cache.c and arc_cache.h
 */
#include "embARC_test.h"
#include "arc/arc_cache.h"

#if ARC_FEATURE_ICACHE_PRESENT && ARC_FEATURE_DCACHE_PRESENT

#define I_CACHE_SIZE (ARC_FEATURE_ICACHE_SIZE)
#define I_CACHE_WAYS (ARC_FEATURE_ICACHE_WAYS)
#define I_CACHE_LINE_SIZE (ARC_FEATURE_ICACHE_LINE_SIZE)
#define I_CACHE_SET_MASK (((I_CACHE_SIZE / I_CACHE_WAYS) - 1) & ~(I_CACHE_LINE_SIZE - 1))

#define D_CACHE_SIZE (ARC_FEATURE_DCACHE_SIZE)
#define D_CACHE_WAYS (ARC_FEATURE_DCACHE_WAYS)
#define D_CACHE_LINE_SIZE (ARC_FEATURE_ICACHE_LINE_SIZE)
#define D_CACHE_SETS (D_CACHE_SIZE / D_CACHE_WAYS / D_CACHE_LINE_SIZE)
#define D_CACHE_SET_MASK (((D_CACHE_SIZE / D_CACHE_WAYS) - 1) & ~(D_CACHE_LINE_SIZE - 1))
#define D_CACHE_TAG_MASK (~((D_CACHE_SIZE / D_CACHE_WAYS) - 1))

static __attribute__ ((aligned(D_CACHE_SIZE)))
uint32_t cache_data[D_CACHE_WAYS][D_CACHE_SETS][D_CACHE_LINE_SIZE >> 2];

extern uint8_t _f_init[];

UNIT_TEST(arc_cache, i_inval){
	UNIT_TEST_ASSERT_EQUAL(-1, icache_invalidate_mlines(0, 0));
	UNIT_TEST_ASSERT_EQUAL(0, icache_invalidate_mlines(0, I_CACHE_SIZE));
	UNIT_TEST_ASSERT_EQUAL(-1, icache_invalidate_mlines(0, I_CACHE_SIZE + 1));
	UNIT_TEST_ASSERT_EQUAL(0, icache_invalidate_mlines(0x80000000, I_CACHE_SIZE));
	UNIT_TEST_ASSERT_EQUAL(0, icache_invalidate_mlines(0xFFFFFFFF, 32));
}

UNIT_TEST(arc_cache, i_lock){
	UNIT_TEST_ASSERT_EQUAL(-1, icache_lock_mlines(0, 0));
	UNIT_TEST_ASSERT_EQUAL(-1, icache_lock_mlines(0, I_CACHE_SIZE + 1));
	icache_invalidate();
	UNIT_TEST_ASSERT_EQUAL(0, icache_lock_mlines((uint32_t)_f_init, I_CACHE_SIZE));
	UNIT_TEST_ASSERT_EQUAL(0, icache_invalidate_mlines((uint32_t)_f_init, I_CACHE_SIZE));
	UNIT_TEST_ASSERT_EQUAL(0, icache_lock_mlines(0x10000000, I_CACHE_SIZE / I_CACHE_WAYS));
	UNIT_TEST_ASSERT_EQUAL(0, icache_lock_mlines(0xFFFFFFFF, I_CACHE_SIZE / 1024));
}

#if ARC_FEATURE_ICACHE_FEATURE == 2
/* only advanced debug features are supported, the following tests are vaid */
UNIT_TEST(arc_cache, i_w_r){
	uint32_t tag;
	uint32_t data;
	uint32_t test_addr = 0;

	icache_access_mode(1);
	UNIT_TEST_ASSERT_EQUAL(-1, icache_direct_write(0, 0, 0));
	UNIT_TEST_ASSERT_EQUAL(-1, icache_direct_read(0, &tag, &data));
	icache_access_mode(0);
	icache_lock_mlines(0, I_CACHE_LINE_SIZE);
	UNIT_TEST_ASSERT_EQUAL(0, icache_direct_write(0, 0x23, 0x25));
	UNIT_TEST_ASSERT_EQUAL(0, icache_direct_read(0, &tag, &data));
	UNIT_TEST_ASSERT_EQUAL(0x25, data);
	UNIT_TEST_ASSERT_EQUAL(0x23 & ~I_CACHE_SET_MASK, tag & ~I_CACHE_SET_MASK);

	icache_access_mode(0);
	UNIT_TEST_ASSERT_EQUAL(-1, icache_indirect_read(0, &tag, &data));
	UNIT_TEST_ASSERT_EQUAL(0, icache_direct_write(0, 0xffff, 0xffff));
	icache_access_mode(1);
	test_addr = (0x12ffff & ~I_CACHE_SET_MASK);
	UNIT_TEST_ASSERT_EQUAL(-1, icache_indirect_read(test_addr, &tag, &data));
	test_addr = (0xffff & ~I_CACHE_SET_MASK);
	UNIT_TEST_ASSERT_EQUAL(0, icache_indirect_read(test_addr, &tag, &data));
	UNIT_TEST_ASSERT_EQUAL(0xffff, data);
	UNIT_TEST_ASSERT_EQUAL(0xffff & ~I_CACHE_SET_MASK, tag & ~I_CACHE_SET_MASK);
}
#endif

UNIT_TEST(arc_cache, d_inval){
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_invalidate_mlines(0, 0));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_invalidate_mlines(0, 1));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_invalidate_mlines(0, D_CACHE_SIZE));
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_invalidate_mlines(0, D_CACHE_SIZE + 1));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_invalidate_mlines(0x80000000, D_CACHE_SIZE));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_invalidate_mlines(0xFFFFFFFF, D_CACHE_SIZE / 1024));
}

UNIT_TEST(arc_cache, d_lock){
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_lock_mlines(0, 0));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_lock_mlines(0, 1));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_invalidate_mlines(0, 1));
	dcache_invalidate();
	UNIT_TEST_ASSERT_EQUAL(0, dcache_lock_mlines(0, D_CACHE_SIZE));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_invalidate_mlines(0, D_CACHE_SIZE));
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_lock_mlines(0, D_CACHE_SIZE + 1));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_lock_mlines(0x10000000, D_CACHE_SIZE / 2));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_lock_mlines(0xFFFFFFFF, D_CACHE_SIZE / 1024));
}

UNIT_TEST(arc_cache, d_w_r){
	uint32_t tag;
	uint32_t data;

	/* direct access */
	dcache_access_mode(1);
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_direct_write(0, 0, 0));
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_direct_read(0, &tag, &data));
	dcache_access_mode(0);
	dcache_lock_mlines(0, D_CACHE_LINE_SIZE);
	UNIT_TEST_ASSERT_EQUAL(0, dcache_direct_write(0, 0xffff, 0xffff));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_direct_read(0, &tag, &data));
	UNIT_TEST_ASSERT_EQUAL(0xffff, data);
	UNIT_TEST_ASSERT_EQUAL(0xffff & D_CACHE_TAG_MASK, tag & D_CACHE_TAG_MASK);

	/* indirect access */
	dcache_access_mode(0);
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_indirect_read(0, &tag, &data));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_direct_write(0, 0xffff, 0xffff));
	dcache_access_mode(1);
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_indirect_read((0x12ffff & D_CACHE_TAG_MASK), &tag, &data));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_indirect_read((0xffff & D_CACHE_TAG_MASK), &tag, &data));
	UNIT_TEST_ASSERT_EQUAL(0xffff, data);
	UNIT_TEST_ASSERT_EQUAL(0xffff & D_CACHE_TAG_MASK, tag & D_CACHE_TAG_MASK);
}

UNIT_TEST(arc_cache, d_flush){
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_flush_mlines(0, 0));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_flush_mlines(0, D_CACHE_SIZE));
	UNIT_TEST_ASSERT_EQUAL(-1, dcache_flush_mlines(0, D_CACHE_SIZE + 1));
	UNIT_TEST_ASSERT_EQUAL(0, dcache_flush_mlines(0xFFFFFFFF, D_CACHE_SIZE));
}

static void write_cache_data(uint32_t data)
{
	int32_t way, set, offset;

	for (way = 0; way < D_CACHE_WAYS; way++) {
		for (set = 0; set < D_CACHE_SETS; set++) {
			for (offset = 0; offset < (D_CACHE_LINE_SIZE >> 2); offset++) {
				cache_data[way][set][offset] = data;
			}
		}
	}

}

UNIT_TEST(arc_cache, d_opts){

	dcache_invalidate();
	dcache_disable();

	write_cache_data(0x55555555);

	UNIT_TEST_ASSERT_EQUAL(arc_read_cached_32(cache_data), arc_read_uncached_32(cache_data));
	dcache_enable(DC_CTRL_DISABLE_FLUSH_LOCKED | DC_CTRL_INDIRECT_ACCESS | DC_CTRL_INVALID_FLUSH);

	write_cache_data(0xaaaaaaaa);

	/** UNIT_TEST_ASSERT_NOT_EQUAL(arc_read_cached_32(cache_data), arc_read_uncached_32(cache_data)); */
	UNIT_TEST_ASSERT_NOT_EQUAL(arc_read_cached_32(cache_data[0][D_CACHE_SETS - 1]), arc_read_uncached_32(cache_data[0][D_CACHE_SETS - 1]));
	UNIT_TEST_ASSERT_EQUAL(dcache_lock_mlines((uint32_t)cache_data, D_CACHE_SETS * D_CACHE_LINE_SIZE), 0);
	UNIT_TEST_ASSERT_EQUAL(dcache_flush_mlines((uint32_t)cache_data, D_CACHE_SETS * D_CACHE_LINE_SIZE), 0);
	UNIT_TEST_ASSERT_NOT_EQUAL(arc_read_cached_32(cache_data), arc_read_uncached_32(cache_data));
	UNIT_TEST_ASSERT_EQUAL(dcache_invalidate_mlines((uint32_t)cache_data, D_CACHE_SETS * D_CACHE_LINE_SIZE), 0);
	UNIT_TEST_ASSERT_EQUAL(arc_read_cached_32(cache_data), arc_read_uncached_32(cache_data));
}
#else
#error test_cache.c is only for targets with cache
#endif
