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
 * @brief Unit test for for arc_udma.c
 */

#include "embARC.h"
#include "embARC_debug.h"
#include "embARC_test.h"
#if ARC_FEATURE_DMAC_PRESENT
#include "stdlib.h"

#ifdef BOARD_EMSDP
#define UDMA_TIMER_ID TIMER_0
#else
#define UDMA_TIMER_ID TIMER_1
#endif

// Notice: Increase heap size in makefile before using multiple channels.
// Warning: With multiple channels in use, the memory usage is increased and it could exceed cache's capability. A significent reduction of cycles for cache operations will be observed.
#define DMA_USE_CHANNEL_NUM     4               // DMA_ALL_CHANNEL_NUM
#define TEST_INCREMENT                  256     // MEMSZ_GAP
#if DMA_USE_CHANNEL_NUM > DMA_ALL_CHANNEL_NUM
#warning Unable to use more channels than the board has!
#undef DMA_USE_CHANNEL_NUM
#define DMA_USE_CHANNEL_NUM             DMA_ALL_CHANNEL_NUM
#endif

#define MEMORY_FENCE()                          arc_sync()
#define DCACHE_FLUSH_MLINES(addr, size) dcache_flush_mlines((uint32_t)(addr), (uint32_t)(size))
#define DCACHE_INVALIDATE_MLINES(addr, size)    dcache_invalidate_mlines((uint32_t)(addr), (uint32_t)(size))
#define ICACHE_INVALIDATE_MLINES(addr, size)    icache_invalidate_mlines((uint32_t)(addr), (uint32_t)(size))

static volatile uint32_t task_done_count = 0;
static DMA_STATE_T udma;
static DMA_CHANNEL_T dma_chn_test[DMA_USE_CHANNEL_NUM];
static DMA_DESC_T dma_desc[DMA_USE_CHANNEL_NUM];
static DMA_CTRL_T dma_ctrl;
static uint32_t dma_prio = DMA_CHN_HIGH_PRIO;

static int32_t dma_prepare(uint32_t dmac_mode)
{
	int32_t dma_channel = DMA_CHN_ANY;

	DMA_CTRL_SET_OP(&dma_ctrl, DMA_SINGLE_TRANSFER);
	DMA_CTRL_SET_RT(&dma_ctrl, DMA_AUTO_REQUEST);
	DMA_CTRL_SET_DTT(&dma_ctrl, DMA_MEM2MEM);
	DMA_CTRL_SET_DWINC(&dma_ctrl, dmac_mode);
	DMA_CTRL_SET_AM(&dma_ctrl, DMA_AM_SRCINC_DSTINC);
	DMA_CTRL_SET_ARB(&dma_ctrl, 255);
	DMA_CTRL_SET_INT(&dma_ctrl, DMA_INT_ENABLE);

	/* Check errors */
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_init_channel(NULL));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_config_desc(NULL, NULL, NULL, 0, &dma_ctrl));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_config_channel(NULL, NULL));

	for (uint8_t i = 0; i < DMA_USE_CHANNEL_NUM; i++) {
		dmac_config_desc(dma_desc + i, NULL, NULL, 0, &dma_ctrl);
		dmac_desc_add_linked(dma_desc + i, NULL);

		/* Init and configure dma channel transfer with transfer descriptor */
		dmac_init_channel(dma_chn_test + i);
		dmac_config_channel(dma_chn_test + i, dma_desc + i);
		/* Reserve a channel for use */
		dma_channel = dmac_reserve_channel(DMA_CHN_ANY, dma_chn_test + i, DMA_REQ_SOFT);
		if (dma_channel == DMA_CHN_INVALID) {
			EMBARC_PRINTF("dmac_reserve_channel No.%d failed, ret %d\r\n", i, dma_channel);
		}
	}
	/*Check ERRORS*/
	UNIT_TEST_ASSERT_EQUAL(DMA_CHN_INVALID, dmac_reserve_channel(DMA_CHN_ANY, NULL, DMA_REQ_SOFT));
	UNIT_TEST_ASSERT_EQUAL(DMA_CHN_INVALID, dmac_reserve_channel(DMA_CHN_ANY, dma_chn_test, DMA_REQ_SOFT));
	UNIT_TEST_ASSERT_EQUAL(DMA_CHN_INVALID, dmac_reserve_channel(DMA_CHN_INVALID, dma_chn_test, DMA_REQ_SOFT));
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

	/*Check ERRORS*/
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_start_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, NULL, DMA_CHN_HIGH_PRIO));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_stop_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_clear_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_release_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	UNIT_TEST_ASSERT_EQUAL(0, dmac_reserve_channel(0, dma_chn_test, DMA_REQ_SOFT));                              /* g_dmac->dma_chns = NULL */
	UNIT_TEST_ASSERT_EQUAL(DMA_CHN_INVALID, dmac_reserve_channel(0, dma_chn_test, DMA_REQ_SOFT));                /* g_dmac->dma_chns[0] != NULL */
}

static void dma_xfer_callback(void *param)
{
	arc_lock();
	task_done_count++;
	arc_unlock();
}

static int32_t dma_copy(void *dst, void *src, uint32_t size, uint32_t udma_int_enable)
{
	int32_t ercd = -1;
	DMA_CALLBACK_T xfer_cb = NULL;
	uint8_t ch_idx;
	uint32_t ch_dst, ch_src, ch_size, ch_status;

	if (udma_int_enable == DMA_INT_ENABLE) {
		xfer_cb = dma_xfer_callback;
	} else {
		xfer_cb = NULL;
	}
	ch_dst = *((uint32_t *)dst);
	ch_src = *((uint32_t *)src);
	ch_size = size / DMA_USE_CHANNEL_NUM;
	for (ch_idx = 0; ch_idx < DMA_USE_CHANNEL_NUM; ch_idx++) {
		dmac_config_desc(dma_desc + ch_idx, (uint32_t *)ch_src, (uint32_t *)ch_dst, ch_size, &dma_ctrl);
		ch_src += ch_size;
		ch_dst += ch_size;
	}

	MEMORY_FENCE();
	DCACHE_INVALIDATE_MLINES((uint32_t *)src, size);
	DCACHE_INVALIDATE_MLINES((uint32_t *)dst, size);

	/*Check ERRORS*/
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_start_channel(NULL, NULL, dma_prio));
	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, NULL);
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_start_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, NULL, dma_prio));
	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, dma_desc + DMA_USE_CHANNEL_NUM - 1);

	/* Start channel transfer with priority, without callback  */
	for (ch_idx = 0; ch_idx < DMA_USE_CHANNEL_NUM; ch_idx++) {
		ercd = dmac_start_channel(dma_chn_test + ch_idx, xfer_cb, dma_prio);
		if (ercd != 0) {
			EMBARC_PRINTF("dma_copy: start channel failed, index %d ret %d\r\n", ch_idx, ercd);
		}
		ch_status = dmac_check_channel(dma_chn_test + ch_idx);
		if (ch_status) {
			dmac_wait_channel(dma_chn_test + ch_idx);
		}
		if (dma_prio == DMA_CHN_HIGH_PRIO) { /* Change prio */
			dma_prio = DMA_CHN_NORM_PRIO;
		} else {
			dma_prio = DMA_CHN_HIGH_PRIO;
		}

	}

	/* Check errors */
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_wait_channel(NULL));
	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, NULL);
	UNIT_TEST_ASSERT_EQUAL(-1, dmac_wait_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1));
	dmac_config_channel(dma_chn_test + DMA_USE_CHANNEL_NUM - 1, dma_desc + DMA_USE_CHANNEL_NUM - 1);

	/* Wait until transfer is done */
	if (xfer_cb) {
		while (task_done_count < DMA_USE_CHANNEL_NUM) {
			;
		}
	}
	MEMORY_FENCE();
	DCACHE_INVALIDATE_MLINES((uint32_t *)src, size);
	DCACHE_INVALIDATE_MLINES((uint32_t *)dst, size);

	return ercd;
}

#endif /* ARC_FEATURE_DMAC_PRESENT */

static uint32_t test_dma_int_enable(uint32_t udma_int_enable, uint32_t dmac_mode)
{
#if ARC_FEATURE_DMAC_PRESENT
	cpu_unlock();
	uint32_t test_sz = TEST_INCREMENT;
	uint8_t src_ptr[TEST_INCREMENT];
	uint8_t dst_ptr[TEST_INCREMENT];
	int32_t ret;

	/** Must init uDMA before use it */
	dmac_init(&udma);
	ret = dma_prepare(dmac_mode);
	if (ret == DMA_CHN_INVALID) {
		EMBARC_PRINTF("dma channel failed, exit.\r\n");
		return -1;
	}
	do {
		if (test_sz % DMA_USE_CHANNEL_NUM != 0) {
			EMBARC_PRINTF("dma_copy: test_sz unable to be divided by %d channels, skip\r\n", DMA_USE_CHANNEL_NUM);
		} else {
			task_done_count = 0;
			for (uint32_t i = 0; i < test_sz; i++) {
				src_ptr[i] = i;
				dst_ptr[i] = 0xFF;
			}
			dma_copy(dst_ptr, src_ptr, test_sz, udma_int_enable);
			EMBARC_PRINTF("%-11u ", test_sz);
		}
		test_sz += TEST_INCREMENT;
		if (test_sz > 1024) {
			break;
		}
	} while (1);
	dma_finish();
	dmac_close();
#else /* ARC_FEATURE_DMAC_PRESENT */
	EMBARC_PRINTF("This example is not support under current configures \r\n");
#endif /* ARC_FEATURE_DMAC_PRESENT */
	return 0;
}

UNIT_TEST(arc_udma, int_disable){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_DISABLE, DMA_DW4INC4));
}

UNIT_TEST(arc_udma, mode_d1inc1){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW1INC1));
}

UNIT_TEST(arc_udma, mode_d1inc2){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW1INC2));
}

UNIT_TEST(arc_udma, mode_d1inc4){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW1INC4));
}

/* exception EV_Misaligned */
UNIT_TEST(arc_udma, mode_d2inc2) {
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW2INC2));
}

UNIT_TEST(arc_udma, mode_d2inc4) {
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW2INC4));
}

UNIT_TEST(arc_udma, mode_d4inc4) {
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW4INC4));
}

UNIT_TEST(arc_udma, model_clear){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DWINC_CLR));
}

#ifdef BOARD_HSDK
UNIT_TEST(arc_udma, mode_d8inc8){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, DMA_DW8INC8));
}
#endif

UNIT_TEST(arc_udma, mode_no){
	UNIT_TEST_ASSERT_EQUAL(0, test_dma_int_enable(DMA_INT_ENABLE, 8));
}
