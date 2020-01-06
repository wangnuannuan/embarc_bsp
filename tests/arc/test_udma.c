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
 * @brief Unit test for arc_udma.c
 */

#include "embARC_debug.h"
#include "embARC_test.h"
#include "arc/arc_udma.h"
#include "arc/arc_cache.h"
#include "arc/arc_timer.h"
#include "stdlib.h"

#ifdef BOARD_EMSDP
#define UDMA_TIMER_ID TIMER_0
#else
#define UDMA_TIMER_ID TIMER_1
#endif

// Notice: Increase heap size in makefile before using multiple channels.
// Warning: With multiple channels in use, the memory usage is increased and it could exceed cache's capability. A significent reduction of cycles for cache operations will be observed.
#define DMA_USE_CHANNEL_NUM 2   // DMA_ALL_CHANNEL_NUM
#define TEST_INCREMENT 1024     // MEMSZ_GAP
#if DMA_USE_CHANNEL_NUM > DMA_ALL_CHANNEL_NUM
#warning Unable to use more channels than the board has!
#undef DMA_USE_CHANNEL_NUM
#define DMA_USE_CHANNEL_NUM DMA_ALL_CHANNEL_NUM
#endif

// define USE_EXTRA_TASK will stop reading of comparison result. By doing so assumes that all memory copy / dma copy are succeeded
#define EXTRA_TASK_NUM 1
#define MEMORY_FENCE() arc_sync()
#define DCACHE_FLUSH_MLINES(addr, size) dcache_flush_mlines((uint32_t)(addr), (uint32_t)(size))
#define DCACHE_INVALIDATE_MLINES(addr, size) dcache_invalidate_mlines((uint32_t)(addr), (uint32_t)(size))

static volatile uint32_t start_test = 0;
static uint32_t perf_id = 0xFF;

static DMA_STATE_T udma;
static DMA_CHANNEL_T dma_chn_test[DMA_USE_CHANNEL_NUM];
static DMA_DESC_T dma_desc[DMA_USE_CHANNEL_NUM];
static DMA_CTRL_T dma_ctrl;
static uint8_t int_error = 0;
static uint32_t dma_prio = DMA_CHN_HIGH_PRIO;

/** performance timer initialization */
static void perf_init(uint32_t id)
{
	if (timer_start(id, TIMER_CTRL_NH, 0xFFFFFFFF) < 0) {
		EMBARC_PRINTF("perf timer init failed\r\n");
		while (1) {
			;
		}
	}
	perf_id = id;
}

/** performance timer start */
static void perf_start(void)
{
	if (timer_current(perf_id, (void *)(&start_test)) < 0) {
		start_test = 0;
	}
}

/** performance timer end, and return the time passed */
static uint32_t perf_end(void)
{
	uint32_t end = 0;

	if (timer_current(perf_id, (void *)(&end)) < 0) {
		return 0;
	}

	if (start_test < end) {
		return (end - start_test);
	} else {
		return (0xFFFFFFFF - start_test + end);
	}
}

static int32_t dma_prepare(uint32_t udma_int_enable, uint32_t dmac_mode)
{
	int32_t dma_channel = DMA_CHN_ANY;
	uint32_t ctrl_op = DMA_SINGLE_TRANSFER;
	uint32_t ctrl_data_type = DMA_MEM2MEM;
	uint32_t ctrl_rt = DMA_AUTO_REQUEST;
	uint32_t ctrl_addr_mode = DMA_AM_SRCINC_DSTINC;

	DMA_CTRL_SET_OP(&dma_ctrl, ctrl_op);
	DMA_CTRL_SET_RT(&dma_ctrl, ctrl_rt);
	DMA_CTRL_SET_DTT(&dma_ctrl, ctrl_data_type);
	DMA_CTRL_SET_DWINC(&dma_ctrl, dmac_mode);
	DMA_CTRL_SET_AM(&dma_ctrl, ctrl_addr_mode);
	DMA_CTRL_SET_ARB(&dma_ctrl, 255);
	DMA_CTRL_SET_INT(&dma_ctrl, udma_int_enable);

	/* Check errors */
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_config_desc(NULL, NULL, NULL, 0, &dma_ctrl));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_init_channel(NULL));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_config_channel(NULL, NULL));
	UNIT_TEST_ASSERT_EQUAL(DMA_CHN_INVALID, dmac_reserve_channel(DMA_CHN_ANY, NULL, DMA_REQ_SOFT));

	for (uint8_t i = 0; i < DMA_USE_CHANNEL_NUM; i++) {
		dmac_config_desc(dma_desc + i, NULL, NULL, 0, &dma_ctrl);
		UNIT_TEST_ASSERT_EQUAL(-1, dmac_desc_add_linked(NULL, NULL));
		dmac_desc_add_linked(dma_desc + i, NULL);

		/* Init and configure dma channel transfer with transfer descriptor */
		dmac_init_channel(dma_chn_test + i);
		UNIT_TEST_ASSERT_EQUAL(-1, dmac_check_channel(NULL));
		UNIT_TEST_ASSERT_EQUAL(-1, dmac_check_channel(dma_chn_test + i));

		dmac_config_channel(dma_chn_test + i, dma_desc + i);
		UNIT_TEST_ASSERT_EQUAL(-1, dmac_check_channel(dma_chn_test + i));
		/* Reserve a channel for use */
		dma_channel = dmac_reserve_channel(DMA_CHN_ANY, dma_chn_test + i, DMA_REQ_SOFT);
		if (dma_channel == DMA_CHN_INVALID) {
			EMBARC_PRINTF("dmac_reserve_channel No.%d failed, ret %d\r\n", i, dma_channel);
		}
	}

	/*Check ERRORS*/
	UNIT_TEST_ASSERT_EQUAL(DMA_CHN_INVALID, dmac_reserve_channel(DMA_CHN_ANY, dma_chn_test, DMA_REQ_SOFT));
	UNIT_TEST_ASSERT_EQUAL(DMA_CHN_INVALID, dmac_reserve_channel(DMA_CHN_INVALID, dma_chn_test, DMA_REQ_SOFT)); ////
	UNIT_TEST_ASSERT_EQUAL(DMA_CHN_INVALID, dmac_reserve_channel(DMA_ALL_CHANNEL_NUM + 1, dma_chn_test, DMA_REQ_SOFT));
	return dma_channel;
}

static void dma_finish(void)
{
	/*Check ERRORS*/
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_stop_channel(NULL));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_clear_channel(NULL));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_release_channel(NULL));

	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, NULL);
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_stop_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_clear_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_release_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, dma_desc + DMA_USE_CHANNEL_NUM - 1);
	/* Release channel resource */
	for (uint8_t i = 0; i < DMA_USE_CHANNEL_NUM; i++) {
		dmac_stop_channel(dma_chn_test + i);
		dmac_clear_channel(dma_chn_test + i);
		dmac_release_channel(dma_chn_test + i);
		UNIT_TEST_ASSERT_EQUAL(-1, dmac_wait_channel(dma_chn_test + i));
		UNIT_TEST_ASSERT_EQUAL(-1, dmac_check_channel(dma_chn_test + i));
	}

	UNIT_TEST_ASSERT_EQUAL(-1, dmac_start_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, NULL, DMA_CHN_HIGH_PRIO));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_stop_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_clear_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_release_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	UNIT_TEST_ASSERT_EQUAL(0, dmac_reserve_channel(0, dma_chn_test, DMA_REQ_SOFT));                         /* g_dmac->dma_chns = NULL */
	UNIT_TEST_ASSERT_EQUAL(DMA_CHN_INVALID, dmac_reserve_channel(0, dma_chn_test, DMA_REQ_SOFT));           /* g_dmac->dma_chns[0] != NULL */
}

static volatile uint32_t task_done_count = 0;
static void dma_xfer_callback(void *param)
{
	task_done_count++;
	EMBARC_PRINTF("%d ", dma_chn_test->channel);
}

static void dma_int_callback(void *param)
{
	EMBARC_PRINTF("DMA callback with interrupt ");
}

static int32_t cmp_data(uint8_t *src, uint8_t *dst, uint32_t size)
{
	for (uint32_t i = 0; i < size; i++) {
		if (src[i] != dst[i]) {
			return -1;
		}
	}
	return 0;
}

static int32_t extra_cpy_task(uint32_t size)
{
	uint8_t *src_ptr = NULL;
	uint8_t *dst_ptr = NULL;

	src_ptr = (uint8_t *)malloc(size);
	if (!src_ptr) {
		EMBARC_PRINTF("extra_cpy_task: Not enough memory for testing, exit this example!\r\n");
		return -1;
	}
	dst_ptr = (uint8_t *)malloc(size);
	if (!dst_ptr) {
		EMBARC_PRINTF("extra_cpy_task: Not enough memory for testing, exit this example!\r\n");
		free(src_ptr);
		return -1;
	}
	for (uint32_t i = 0; i < size; i++) {
		src_ptr[i] = i;
		dst_ptr[i] = 0xFF;
	}
	memcpy(dst_ptr, src_ptr, size);
	if (cmp_data(src_ptr, dst_ptr, size) == 0) {
		arc_lock();
		task_done_count++;
		arc_unlock();
		free(src_ptr);
		free(dst_ptr);
		return 0;
	} else {
		EMBARC_PRINTF("extra_cpy_task: data error!\r\n");
		free(src_ptr);
		free(dst_ptr);
		return -1;
	}
}

static uint32_t cycles_cnt[15];
static uint32_t cycles_idx = 0;
static int32_t dma_copy(void *dst, void *src, uint32_t size, uint32_t udma_int_enable)
{
	int32_t ercd = -1;

	cycles_idx = 0;
	DMA_CALLBACK_T xfer_cb = NULL;
	uint8_t ch_idx;
	uint32_t ch_dst, ch_src, ch_size;

	ch_dst = *((uint32_t *)dst);
	ch_src = *((uint32_t *)src);
	ch_size = size / DMA_USE_CHANNEL_NUM;
	cycles_cnt[cycles_idx++] = perf_end();
	for (ch_idx = 0; ch_idx < DMA_USE_CHANNEL_NUM; ch_idx++) {
		/* udma_int_enable == DMA_INT_ENABLE && int_error == 1 */
		dmac_config_desc(dma_desc + ch_idx, (uint32_t *)ch_src, (uint32_t *)ch_dst, ch_size, &dma_ctrl);
		ch_src += ch_size;
		ch_dst += ch_size;
	}
	cycles_cnt[cycles_idx++] = perf_end();

	MEMORY_FENCE();
	DCACHE_FLUSH_MLINES((uint32_t *)src, size);
	DCACHE_FLUSH_MLINES((uint32_t *)dst, size);

	cycles_cnt[cycles_idx++] = perf_end();

	/* Start channel transfer with priority, without callback  */
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_start_channel(NULL, NULL, dma_prio));

	/* Check errors */
	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, NULL);
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_start_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, NULL, dma_prio));
	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, dma_desc + DMA_USE_CHANNEL_NUM - 1);

	for (ch_idx = 0; ch_idx < DMA_USE_CHANNEL_NUM; ch_idx++) {
		if (udma_int_enable == DMA_INT_ENABLE && int_error == 0) {
			xfer_cb = dma_xfer_callback;
		} else if (udma_int_enable == DMA_INT_ENABLE && int_error == 1) {

			xfer_cb = dma_int_callback;
		} else {
			xfer_cb = NULL;
		}
		ercd = dmac_start_channel(dma_chn_test + ch_idx, xfer_cb, dma_prio);
		if (ercd != 0) {
			EMBARC_PRINTF("dma_copy: start channel failed, index %d ret %d\r\n", ch_idx, ercd);
		}
		if (xfer_cb) {
			UNIT_TEST_ASSERT_EQUAL(DMA_BUSY, dmac_check_channel(dma_chn_test + ch_idx));
		} else {
			UNIT_TEST_ASSERT_EQUAL(DMA_IDLE, dmac_wait_channel(dma_chn_test + ch_idx));
			UNIT_TEST_ASSERT_EQUAL(DMA_IDLE, dmac_check_channel(dma_chn_test + ch_idx));
		}
		if (dma_prio == DMA_CHN_HIGH_PRIO) { /* Change prio */
			dma_prio = DMA_CHN_NORM_PRIO;
		} else {
			dma_prio = DMA_CHN_HIGH_PRIO;
		}
	}

	cycles_cnt[cycles_idx++] = perf_end();
	ercd = extra_cpy_task(size);
	/** EMBARC_PRINTF("dma_copy: extra_cpy_task ret %d\r\n", ercd); */
	/* Check errors */
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_wait_channel(NULL));
	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, NULL);
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_wait_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, dma_desc + DMA_USE_CHANNEL_NUM - 1);
	/* Wait until transfer is done */
	if (xfer_cb && int_error == 0) {
		while (task_done_count < DMA_USE_CHANNEL_NUM + EXTRA_TASK_NUM) {
			UNIT_TEST_ASSERT_EQUAL(DMA_BUSY, dmac_wait_channel(dma_chn_test));
			dmac_stop_channel(dma_chn_test + ch_idx);
		}
	}
	cycles_cnt[cycles_idx++] = perf_end();

	MEMORY_FENCE();
	DCACHE_INVALIDATE_MLINES((uint32_t *)src, size);
	DCACHE_INVALIDATE_MLINES((uint32_t *)dst, size);
	cycles_cnt[cycles_idx++] = perf_end();

	return ercd;
}

static void init_data(uint8_t *src, uint8_t *dst, uint32_t size)
{
	for (uint32_t i = 0; i < size; i++) {
		src[i] = i;
		dst[i] = 0xFF;
	}
}

static int32_t perf_dmacpy(uint8_t *src, uint8_t *dst, uint32_t size, uint32_t udma_int_enable)
{
	uint32_t cycles;

	task_done_count = 0;
	init_data(src, dst, size);
	perf_init(UDMA_TIMER_ID);
	perf_start();
	dma_copy(dst, src, size, udma_int_enable);
	cycles = perf_end();
	if (cmp_data(src, dst, size) == 0) {
		return cycles;
	} else {
		return -1;
	}
}

static int32_t perf_overhead(void)
{
	uint32_t cycles;

	perf_init(UDMA_TIMER_ID);
	perf_start();
	cycles = perf_end();
	return cycles;
}

static uint32_t test_dma_int_enable(uint32_t udma_int_enable, uint32_t dmac_mode)
{
	uint32_t test_sz = TEST_INCREMENT;
	uint8_t *src_ptr = NULL;
	uint8_t *dst_ptr = NULL;
	int32_t dmacpy_cycles;
	int32_t ret;

	/** Must init uDMA before use it */
	EMBARC_PRINTF("DMA test: DMA has %d channels, now uses %d channels\r\n", DMA_ALL_CHANNEL_NUM, DMA_USE_CHANNEL_NUM);
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_init(NULL));
	dmac_init(&udma);
	ret = dma_prepare(udma_int_enable, dmac_mode);
	if (ret == DMA_CHN_INVALID) {
		EMBARC_PRINTF("dma channel failed, exit.\r\n");
		return -1;
	}
	EMBARC_PRINTF("XFER_SIZE DESC_CONF CACHE_FLUSH XFER_START XFER_WAIT CACHE_INV DMACPY MEMCPY\r\n");
	do {
		if (test_sz % DMA_USE_CHANNEL_NUM != 0) {
			EMBARC_PRINTF("dma_copy: test_sz unable to be divided by %d channels, skip\r\n", DMA_USE_CHANNEL_NUM);
		} else {
			src_ptr = (uint8_t *)malloc(test_sz);
			if (!src_ptr) {
				EMBARC_PRINTF("extra_cpy_task: Not enough memory for testing, exit this example!\r\n");
				return -1;
			}
			dst_ptr = (uint8_t *)malloc(test_sz);
			if (!dst_ptr) {
				EMBARC_PRINTF("extra_cpy_task: Not enough memory for testing, exit this example!\r\n");
				free(src_ptr);
				return -1;
			}
			dmacpy_cycles = perf_dmacpy(src_ptr, dst_ptr, test_sz, udma_int_enable);

			if (dmacpy_cycles == -1) {
				EMBARC_PRINTF("DMACPY API Test failed!\r\n");
				break;
			}
			EMBARC_PRINTF("%u ", test_sz);
			for (uint32_t i = 1; i < cycles_idx; i++) {
				EMBARC_PRINTF("%u ", cycles_cnt[i] - cycles_cnt[i - 1]);
			}
			EMBARC_PRINTF("%u \r\n", (uint32_t)dmacpy_cycles);

			free(src_ptr);
			free(dst_ptr);
		}
		test_sz += TEST_INCREMENT;
		if (test_sz > 1024 * 2 * DMA_USE_CHANNEL_NUM) {
			EMBARC_PRINTF("DMA only support 8K cell transfer\r\n");
			break;
		}
	} while (1);

	if (src_ptr != NULL) {
		free(src_ptr);
	}
	if (dst_ptr != NULL) {
		free(dst_ptr);
	}
	dma_finish();
	dmac_close();
	return 0;
}

UNIT_TEST(arc_udma, int_disable){
	uint32_t ctrl_int;
	uint32_t ctrl_dw_inc;

	ctrl_int = DMA_INT_DISABLE;
	ctrl_dw_inc = DMA_DW4INC4;
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(ctrl_int, ctrl_dw_inc));
}

UNIT_TEST(arc_udma, int_error){
	uint32_t ctrl_int;

	ctrl_int = DMA_INT_ENABLE;
	int_error = 1;
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(ctrl_int, DMA_DW4INC4));
}

UNIT_TEST(arc_udma, mode_d1inc1){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW1INC1));
}

UNIT_TEST(arc_udma, int_enable){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW4INC4));
}

UNIT_TEST(arc_udma, mode_d1inc2){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW1INC2));
}

UNIT_TEST(arc_udma, mode_d1inc4){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW1INC4));
}

UNIT_TEST(arc_udma, mode_no){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, 8));
}

/* exception EV_Misaligned
   UNIT_TEST(arc_udma, mode_d2inc4) {
    UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW2INC4));
   }

   UNIT_TEST(arc_udma, mode_d2inc2) {
    UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW2INC2));
   }
 */

#ifdef BOARD_HSDK
UNIT_TEST(arc_udma, mode_d8inc8){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW8INC8));
}
#endif
