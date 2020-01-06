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
 * @brief Unit test for dev_uart.h
 */

#include <string.h>
#include "embARC_test.h"
#include "embARC_error.h"
#include "device/ip_hal/dev_uart.h"

#ifdef BOARD_EMSK
#define BOARD_CONSOLE_UART_ID 1
#endif /* BOARD_EMSK */

/** nsim related definition */
#ifdef BOARD_NSIM
#define BOARD_CONSOLE_UART_ID 0
#endif /* BOARD_NSIM */

#ifdef BOARD_AXS
#define BOARD_CONSOLE_UART_ID 2
#endif /* BOARD_AXS */

#ifdef BOARD_HSDK
#define BOARD_CONSOLE_UART_ID 3
#endif /* BOARD_HSDK */

#ifdef BOARD_IOTDK
#define BOARD_CONSOLE_UART_ID 0
#endif /* BOARD_IOTDK */

#ifdef BOARD_EMSDP
#define BOARD_CONSOLE_UART_ID 0
#endif /* BOARD_EMDK */

#define TEST_UART_ID_0  (BOARD_CONSOLE_UART_ID == 0 ? 1 : 0)
#define TEST_UART_ID_1  (BOARD_CONSOLE_UART_ID == 1 ? 0 : 1)
#define TEST_UART_ID_2  (BOARD_CONSOLE_UART_ID == 2 ? 0 : 2)

static void uart_force_close(DEV_UART *uartobj)
{
	if (uartobj == 0) {
		return;
	}
	while (uartobj->uart_close() != E_OK) {
		;
	}
}

static volatile uint32_t tx_int_cnt = 0;
static volatile uint32_t rx_int_cnt = 0;
static volatile uint32_t err_int_cnt = 0;
#define TX_BUF_CNT  4
static uint8_t tx_buffer[TX_BUF_CNT];
#define RX_BUF_CNT  4
static uint8_t rx_buffer[RX_BUF_CNT];

static void reset_all_cnt(void)
{
	tx_int_cnt = 0;
	rx_int_cnt = 0;
	err_int_cnt = 0;
}
static void tx_callback(void *ptr)
{
	tx_int_cnt++;
}
static void rx_callback(void *ptr)
{
	rx_int_cnt++;
}
static void err_callback(void *ptr)
{
	err_int_cnt++;
}

/* all uart info related variable check functions */
static DEV_BUFFER devbuf_all_empty = { NULL, 0, 0 };
#define UNIT_TEST_ASSERT_VOID_EQUAL(expected, real) UNIT_TEST_ASSERT_EQUAL(*((uint32_t *)(expected)), *((uint32_t *)(real)))

static inline void check_uart_info_baudrate(DEV_UART *uartobj, uint32_t expected)
{
	UNIT_TEST_ASSERT_EQUAL(expected, uartobj->uart_info.baudrate);
}
static inline void check_uart_info_status(DEV_UART *uartobj, uint32_t expected)
{
	UNIT_TEST_ASSERT_EQUAL(expected, uartobj->uart_info.status);
}
static inline void check_uart_info_opn_cnt(DEV_UART *uartobj, uint32_t expected)
{
	UNIT_TEST_ASSERT_EQUAL(expected, uartobj->uart_info.opn_cnt);
}
static inline void check_uart_info_dps_format(DEV_UART *uartobj, UART_DPS_FORMAT *expected)
{
	DEV_UART_INFO_PTR info_ptr = &(uartobj->uart_info);

	UNIT_TEST_ASSERT_EQUAL(expected->databits, info_ptr->dps_format.databits);
	UNIT_TEST_ASSERT_EQUAL(expected->parity, info_ptr->dps_format.parity);
	UNIT_TEST_ASSERT_EQUAL(expected->stopbits, info_ptr->dps_format.stopbits);
}
static inline void check_uart_info_hwfc(DEV_UART *uartobj, uint32_t expected)
{
	UNIT_TEST_ASSERT_EQUAL(expected, uartobj->uart_info.hwfc);
}
static inline void check_uart_info_tx_buf(DEV_UART *uartobj, DEV_BUFFER *expected)
{
	DEV_BUFFER *buf = expected;
	DEV_UART_INFO_PTR info_ptr = &(uartobj->uart_info);

	UNIT_TEST_ASSERT_VOID_EQUAL(buf->buf, (info_ptr->tx_buf.buf));
	UNIT_TEST_ASSERT_VOID_EQUAL(buf->len, (info_ptr->tx_buf.len));
	UNIT_TEST_ASSERT_VOID_EQUAL(buf->ofs, (info_ptr->tx_buf.ofs));
}
static inline void check_uart_info_rx_buf(DEV_UART *uartobj, void *expected)
{
	DEV_BUFFER *buf = expected;

	UNIT_TEST_ASSERT_VOID_EQUAL(buf->buf, uartobj->uart_info.rx_buf.buf);
	UNIT_TEST_ASSERT_VOID_EQUAL(buf->len, uartobj->uart_info.rx_buf.len);
	UNIT_TEST_ASSERT_VOID_EQUAL(buf->ofs, uartobj->uart_info.rx_buf.ofs);
}
static inline void check_uart_info_uart_cbs_tx_cb(DEV_UART *uartobj, void *expected)
{
	DEV_UART_INFO_PTR info_ptr = &(uartobj->uart_info);
	DEV_UART_CBS_PTR cbs_ptr = &(info_ptr->uart_cbs);

	UNIT_TEST_ASSERT_VOID_EQUAL(expected, cbs_ptr->tx_cb);
}
static inline void check_uart_info_uart_cbs_rx_cb(DEV_UART *uartobj, void *expected)
{
	UNIT_TEST_ASSERT_VOID_EQUAL(expected, uartobj->uart_info.uart_cbs.rx_cb);
}
static inline void check_uart_info_uart_cbs_err_cb(DEV_UART *uartobj, void *expected)
{
	UNIT_TEST_ASSERT_VOID_EQUAL(expected, uartobj->uart_info.uart_cbs.err_cb);
}
static inline void check_uart_info_extra(DEV_UART *uartobj, void *expected)
{
	UNIT_TEST_ASSERT_VOID_EQUAL(expected, uartobj->uart_info.extra);
}

static inline void check_uart_open(DEV_UART *uartobj)
{
	uart_force_close(uartobj);

	UART_DPS_FORMAT dps_format_default;
	UART_DPS_FORMAT_DEFAULT(dps_format_default);
	if (uartobj) {
		/** 0. test open uart and check its pre baudrate */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		check_uart_info_baudrate(uartobj, UART_BAUDRATE_115200);
		check_uart_info_status(uartobj, DEV_ENABLED);
		check_uart_info_opn_cnt(uartobj, 1);
		check_uart_info_dps_format(uartobj, &dps_format_default);
		check_uart_info_hwfc(uartobj, UART_FC_DEFAULT);
		/* tx buffer check */
		check_uart_info_tx_buf(uartobj, &devbuf_all_empty);
		/* rx buffer check */
		check_uart_info_rx_buf(uartobj, &devbuf_all_empty);

		/* callbacks check */
		check_uart_info_uart_cbs_tx_cb(uartobj, NULL);
		check_uart_info_uart_cbs_rx_cb(uartobj, NULL);
		check_uart_info_uart_cbs_err_cb(uartobj, NULL);

		check_uart_info_extra(uartobj, NULL);
		DEV_UART_INFO_SET_EXTRA_OBJECT(&(uartobj->uart_info), NULL);
		UNIT_TEST_ASSERT_NULL(DEV_UART_INFO_GET_EXTRA_OBJECT(&(uartobj->uart_info)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());

		/** 1. test open uart with different baudrates */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_open(0));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());

		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(10000000));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());

		/** 2. test open 2 times with same baudrate */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, uartobj->uart_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());

		/** 3. test open 2 times with different baudrate */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		check_uart_info_baudrate(uartobj, UART_BAUDRATE_115200);
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, uartobj->uart_open(UART_BAUDRATE_57600));
		check_uart_info_baudrate(uartobj, UART_BAUDRATE_115200);
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, uartobj->uart_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
	}
}

static inline void check_uart_close(DEV_UART *uartobj)
{
	uart_force_close(uartobj);
	if (uartobj) {
		/** 1. test open 3 times, then close device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, uartobj->uart_close());
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, uartobj->uart_open(UART_BAUDRATE_57600));
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, uartobj->uart_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());

		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		DEV_BUFFER devbuf = (DEV_BUFFER) { tx_buffer, 2, 1 };
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT_BUF, CONV2VOID(&devbuf)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
		/** 2. test close device directly without open it */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
	}
}

static inline void check_uart_control(DEV_UART *uartobj)
{
	uint32_t val_u32;
	uint32_t *val_u32_ptr = &val_u32;
	uint8_t *val_u8_ptr;
	uint32_t i;
	UART_DPS_FORMAT dps;
	DEV_BUFFER devbuf;

	val_u8_ptr = (uint8_t *)((uint32_t)(val_u32_ptr) + 1);
	uart_force_close(uartobj);
	if (uartobj) {
		/** Open it before control device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		/** 1. test control device without open it */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
		for (i = 0; i < 100; i++) {
			UNIT_TEST_ASSERT_EQUAL(E_CLSED, uartobj->uart_control(i, NULL));
			UNIT_TEST_ASSERT_EQUAL(E_CLSED, uartobj->uart_control(i, (uint32_t *)(i << 2)));
		}
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));

		UNIT_TEST_ASSERT_EQUAL(E_NOSPT, uartobj->uart_control(-1, CONV2VOID(0)));

		/** 2. test UART_CMD_SET_BAUD command */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_BAUD, CONV2VOID(0)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_57600)));
		check_uart_info_baudrate(uartobj, UART_BAUDRATE_57600);
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_115200)));
		check_uart_info_baudrate(uartobj, UART_BAUDRATE_115200);

		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_110)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_300)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_600)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_1200)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_2400)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_4800)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_9600)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_14400)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_19200)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_38400)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_230400)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_460800)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_BAUD, (uint32_t *)(UART_BAUDRATE_921600)));

		/** 3. test UART_CMD_GET_STATUS command */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_GET_STATUS, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_GET_STATUS, CONV2VOID(&val_u32)));
		UNIT_TEST_ASSERT_EQUAL(DEV_ENABLED & val_u32, DEV_ENABLED);
		check_uart_info_status(uartobj, val_u32);
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_GET_STATUS, CONV2VOID(val_u8_ptr)));

		/** 4. test UART_CMD_ENA_DEV command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_ENA_DEV, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_GET_STATUS, CONV2VOID(&val_u32)));
		UNIT_TEST_ASSERT_EQUAL(DEV_ENABLED & val_u32, DEV_ENABLED);

		/** 5. test UART_CMD_DIS_DEV command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_DIS_DEV, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_GET_STATUS, CONV2VOID(&val_u32)));
		UNIT_TEST_ASSERT_EQUAL(DEV_ENABLED & val_u32, DEV_DISABLED);
		for (i = 0; i < 100; i++) {
			if ((i != UART_CMD_GET_STATUS) && (i != UART_CMD_DIS_DEV) && (i != UART_CMD_ENA_DEV)) {
				UNIT_TEST_ASSERT_EQUAL(E_SYS, uartobj->uart_control(i, NULL));
				UNIT_TEST_ASSERT_EQUAL(E_SYS, uartobj->uart_control(i, (uint32_t *)(i << 2)));
			} else {
				UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_ENA_DEV, CONV2VOID(&val_u32)));
			}
			UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_DIS_DEV, CONV2VOID(NULL)));
		}
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_ENA_DEV, CONV2VOID(NULL)));

		/** 6. test UART_CMD_FLUSH_OUTPUT command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_FLUSH_OUTPUT, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_FLUSH_OUTPUT, CONV2VOID(&val_u32)));

		/** 7. test UART_CMD_GET_RXAVAIL command */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_GET_RXAVAIL, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_GET_RXAVAIL, CONV2VOID(&val_u32)));
		UNIT_TEST_ASSERT_EQUAL(0, val_u32);

		/** 8. test UART_CMD_GET_TXAVAIL command */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_GET_TXAVAIL, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_GET_TXAVAIL, CONV2VOID(&val_u32)));
		UNIT_TEST_ASSERT_EQUAL(1, (val_u32 > 0));

		/** 9. test UART_CMD_BREAK_SET command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_BREAK_SET, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_BREAK_SET, CONV2VOID(&val_u32)));

		/** 10. test UART_CMD_BREAK_CLR command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_BREAK_CLR, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_BREAK_CLR, CONV2VOID(&val_u32)));

		/** 11. test UART_CMD_SET_DPS_FORMAT command */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_DPS_FORMAT, CONV2VOID(NULL)));
		dps = (UART_DPS_FORMAT){ 8, UART_PARITY_ODD, UART_STPBITS_TWO };
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_DPS_FORMAT, CONV2VOID(&dps)));
		check_uart_info_dps_format(uartobj, &dps);
		dps = (UART_DPS_FORMAT){ 0, UART_PARITY_ODD, UART_STPBITS_TWO };
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_DPS_FORMAT, CONV2VOID(&dps)));
		dps = (UART_DPS_FORMAT){ 9, UART_PARITY_ODD, UART_STPBITS_TWO };
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_DPS_FORMAT, CONV2VOID(&dps)));
		dps = (UART_DPS_FORMAT){ 8, UART_PARITY_SPACE + 1, UART_STPBITS_TWO };
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_DPS_FORMAT, CONV2VOID(&dps)));
		dps = (UART_DPS_FORMAT){ 8, UART_PARITY_SPACE, UART_STPBITS_TWO + 1 };
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_DPS_FORMAT, CONV2VOID(&dps)));
		dps = (UART_DPS_FORMAT){ 8, UART_PARITY_NONE, UART_STPBITS_ONE };
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_DPS_FORMAT, CONV2VOID(&dps)));
		check_uart_info_dps_format(uartobj, &dps);

		/** 12. test UART_CMD_SET_HWFC command */
		UART_HW_FLOW_CONTROL uart_flow_control;
		for (uart_flow_control = UART_FC_NONE; uart_flow_control <= UART_FC_BOTH; uart_flow_control++) {
			UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_HWFC, (&uart_flow_control)));
			check_uart_info_hwfc(uartobj, uart_flow_control);
		}
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_HWFC, (&uart_flow_control)));
		check_uart_info_hwfc(uartobj, UART_FC_BOTH);
		uart_flow_control = UART_FC_NONE;
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_HWFC, (&uart_flow_control)));
		check_uart_info_hwfc(uartobj, UART_FC_NONE);

		/** 13. test UART_CMD_SET_TXCB command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXCB, CONV2VOID(NULL)));
		check_uart_info_uart_cbs_tx_cb(uartobj, NULL);
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_TXCB, (uint32_t *)(3)));
		check_uart_info_uart_cbs_tx_cb(uartobj, NULL);
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXCB, CONV2VOID(&val_u32)));
		check_uart_info_uart_cbs_tx_cb(uartobj, CONV2VOID(&val_u32));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXCB, CONV2VOID(tx_callback)));
		check_uart_info_uart_cbs_tx_cb(uartobj, CONV2VOID(tx_callback));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXCB, CONV2VOID(NULL)));
		check_uart_info_uart_cbs_tx_cb(uartobj, CONV2VOID(NULL));

		/** 14. test UART_CMD_SET_RXCB command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXCB, CONV2VOID(NULL)));
		check_uart_info_uart_cbs_rx_cb(uartobj, NULL);
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_RXCB, (uint32_t *)(3)));
		check_uart_info_uart_cbs_rx_cb(uartobj, NULL);
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXCB, CONV2VOID(&val_u32)));
		check_uart_info_uart_cbs_rx_cb(uartobj, CONV2VOID(&val_u32));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXCB, CONV2VOID(rx_callback)));
		check_uart_info_uart_cbs_rx_cb(uartobj, CONV2VOID(rx_callback));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXCB, CONV2VOID(NULL)));
		check_uart_info_uart_cbs_rx_cb(uartobj, CONV2VOID(NULL));

		/** 15. test UART_CMD_SET_ERRCB command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_ERRCB, CONV2VOID(NULL)));
		check_uart_info_uart_cbs_err_cb(uartobj, NULL);
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_ERRCB, (uint32_t *)(3)));
		check_uart_info_uart_cbs_err_cb(uartobj, NULL);
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_ERRCB, CONV2VOID(&val_u32)));
		check_uart_info_uart_cbs_err_cb(uartobj, CONV2VOID(&val_u32));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_ERRCB, CONV2VOID(err_callback)));
		check_uart_info_uart_cbs_err_cb(uartobj, CONV2VOID(err_callback));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_ERRCB, CONV2VOID(NULL)));
		check_uart_info_uart_cbs_err_cb(uartobj, NULL);

		/** 16. test UART_CMD_SET_TXINT_BUF command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT_BUF, CONV2VOID(NULL)));
		check_uart_info_tx_buf(uartobj, &devbuf_all_empty);
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_TXINT_BUF, (uint32_t *)(3)));
		check_uart_info_tx_buf(uartobj, &devbuf_all_empty);
		devbuf = (DEV_BUFFER) { tx_buffer, 2, 1 };
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT_BUF, CONV2VOID(&devbuf)));
		devbuf.ofs = 0;
		check_uart_info_tx_buf(uartobj, CONV2VOID(&devbuf));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT_BUF, CONV2VOID(&devbuf)));
		check_uart_info_tx_buf(uartobj, CONV2VOID(&devbuf));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT_BUF, CONV2VOID(NULL)));
		check_uart_info_tx_buf(uartobj, &devbuf_all_empty);

		/** 17. test UART_CMD_SET_RXINT_BUF command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT_BUF, CONV2VOID(NULL)));
		check_uart_info_rx_buf(uartobj, &devbuf_all_empty);
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_control(UART_CMD_SET_RXINT_BUF, (uint32_t *)(3)));
		check_uart_info_rx_buf(uartobj, &devbuf_all_empty);
		devbuf = (DEV_BUFFER) { rx_buffer, 2, 1 };
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT_BUF, CONV2VOID(&devbuf)));
		devbuf.ofs = 0;
		check_uart_info_rx_buf(uartobj, CONV2VOID(&devbuf));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT_BUF, CONV2VOID(&devbuf)));
		check_uart_info_rx_buf(uartobj, CONV2VOID(&devbuf));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT_BUF, CONV2VOID(NULL)));
		check_uart_info_rx_buf(uartobj, &devbuf_all_empty);

		/** 18. test UART_CMD_SET_TXINT command */
		reset_all_cnt();
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXCB, CONV2VOID(tx_callback)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT, (uint32_t *)(1)));
		UNIT_TEST_ASSERT_EQUAL(1, (tx_int_cnt > 0));
		reset_all_cnt();
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT, CONV2VOID(0)));
		UNIT_TEST_ASSERT_EQUAL(0, tx_int_cnt);
		reset_all_cnt();
		devbuf = (DEV_BUFFER) { tx_buffer, TX_BUF_CNT, 1 };
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT_BUF, CONV2VOID(&devbuf)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT, (uint32_t *)(1)));
		UNIT_TEST_ASSERT_EQUAL(1, tx_int_cnt); /* interrupt should only enter 1 time and exit, then the buffer will be cleared */
		check_uart_info_tx_buf(uartobj, &devbuf_all_empty);
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT, CONV2VOID(0)));

		/** 19. test UART_CMD_SET_RXINT command, receive interrupt handling need to cooperate with other uart */
		reset_all_cnt();
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXCB, CONV2VOID(rx_callback)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT, (uint32_t *)(1)));
		/* UNIT_TEST_ASSERT_EQUAL(1, (rx_int_cnt >= 0)); unsigned integer never falls below 0 */
		reset_all_cnt();
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT, CONV2VOID(0)));
		UNIT_TEST_ASSERT_EQUAL(0, rx_int_cnt);
		reset_all_cnt();
		devbuf = (DEV_BUFFER) { rx_buffer, RX_BUF_CNT, 1 };
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT_BUF, CONV2VOID(&devbuf)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT, (uint32_t *)(1)));
		/* UNIT_TEST_ASSERT_EQUAL(1, (rx_int_cnt >= 0)); unsigned integer never falls below 0 */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT, CONV2VOID(0)));

		/** 20. test UART_CMD_ABORT_TX command */
		reset_all_cnt(); /* test abort interrupt transmit when tx interrupt is disabled */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT, CONV2VOID(0)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXCB, CONV2VOID(tx_callback)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_ABORT_TX, CONV2VOID(tx_callback)));
		UNIT_TEST_ASSERT_EQUAL(0, tx_int_cnt);

		reset_all_cnt(); /* test abort interrupt transmit when tx interrupt is enabled */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXCB, CONV2VOID(tx_callback)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT, (uint32_t *)(1)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_ABORT_TX, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_GET_STATUS, CONV2VOID(&val_u32)));
		UNIT_TEST_ASSERT_EQUAL(0, val_u32 & DEV_IN_TX_ABRT);
		UNIT_TEST_ASSERT_EQUAL(1, tx_int_cnt >= 1);
		reset_all_cnt();
		UNIT_TEST_ASSERT_EQUAL(0, tx_int_cnt);

		/** 21. test UART_CMD_ABORT_RX command */
		reset_all_cnt(); /* test abort interrupt transmit when tx interrupt is disabled */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT, CONV2VOID(0)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXCB, CONV2VOID(rx_callback)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_ABORT_RX, CONV2VOID(rx_callback)));
		UNIT_TEST_ASSERT_EQUAL(0, rx_int_cnt);

		reset_all_cnt(); /* test abort interrupt transmit when tx interrupt is enabled */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXCB, CONV2VOID(rx_callback)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT, (uint32_t *)(1)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_ABORT_RX, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_GET_STATUS, CONV2VOID(&val_u32)));
		UNIT_TEST_ASSERT_EQUAL(0, val_u32 & DEV_IN_RX_ABRT);
		UNIT_TEST_ASSERT_EQUAL(1, rx_int_cnt >= 1);
		reset_all_cnt();
		UNIT_TEST_ASSERT_EQUAL(0, rx_int_cnt);

		/** Close device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
	}
}

static inline void check_uart_write(DEV_UART *uartobj)
{
	uart_force_close(uartobj);
	if (uartobj) {
		/** Open it before control device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));

		/** 1. Test uart write parameters */
		UNIT_TEST_ASSERT_EQUAL(TX_BUF_CNT, uartobj->uart_write(tx_buffer, TX_BUF_CNT));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_write(NULL, 2));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_write(NULL, 0));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_write(tx_buffer, 0));

		/** 2. Test uart write when device is not opened */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
		UNIT_TEST_ASSERT_EQUAL(E_CLSED, uartobj->uart_write(tx_buffer, 2));

		/** Close device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
	}
}

static inline void check_uart_read(DEV_UART *uartobj)
{
	uart_force_close(uartobj);
	if (uartobj) {
		/** Open it before control device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));

		/** 1. Test uart read parameters */
		/** UNIT_TEST_ASSERT_EQUAL(RX_BUF_CNT, uartobj->uart_read(rx_buf, RX_BUF_CNT)); need cooperate with other uart */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_read(NULL, 2));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_read(NULL, 0));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, uartobj->uart_read(rx_buffer, 0));

		UNIT_TEST_ASSERT_EQUAL(2, uartobj->uart_write(tx_buffer, 2));
		UNIT_TEST_ASSERT_EQUAL(2, uartobj->uart_read(rx_buffer, 2));

		/* test abort interrupt transmit when tx interrupt is enabled */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_open(UART_BAUDRATE_115200));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXCB, CONV2VOID(tx_callback)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXCB, CONV2VOID(rx_callback)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_RXINT, (uint32_t *)(1)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_control(UART_CMD_SET_TXINT, (uint32_t *)(1)));
		uartobj->uart_write(tx_buffer, 1);

		/** 2. Test uart read when device is not opened */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
		UNIT_TEST_ASSERT_EQUAL(E_CLSED, uartobj->uart_read(rx_buffer, 2));

		/** Close device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, uartobj->uart_close());
	}
}

UNIT_TEST(dev_uart, get_dev){

	UNIT_TEST_ASSERT_NOT_NULL(uart_get_dev(TEST_UART_ID_0));

	UNIT_TEST_ASSERT_NOT_NULL(uart_get_dev(TEST_UART_ID_1));

	UNIT_TEST_ASSERT_NOT_NULL(uart_get_dev(TEST_UART_ID_2));
	UNIT_TEST_ASSERT_NULL(uart_get_dev(-1));
}

UNIT_TEST(dev_uart, open){
	DEV_UART *uartobj;

	uartobj = uart_get_dev(TEST_UART_ID_0);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_open(uartobj);

	uartobj = uart_get_dev(TEST_UART_ID_1);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_open(uartobj);

	uartobj = uart_get_dev(TEST_UART_ID_2);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_open(uartobj);
}

UNIT_TEST(dev_uart, close){
	DEV_UART *uartobj;

	uartobj = uart_get_dev(TEST_UART_ID_0);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_close(uartobj);

	uartobj = uart_get_dev(TEST_UART_ID_1);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_close(uartobj);

	uartobj = uart_get_dev(TEST_UART_ID_2);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_close(uartobj);
}

UNIT_TEST(dev_uart, control){
	DEV_UART *uartobj;

	uartobj = uart_get_dev(TEST_UART_ID_0);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_control(uartobj);

	uartobj = uart_get_dev(TEST_UART_ID_1);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_control(uartobj);

	uartobj = uart_get_dev(TEST_UART_ID_2);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_control(uartobj);
}

UNIT_TEST(dev_uart, write){
	DEV_UART *uartobj;

	uartobj = uart_get_dev(TEST_UART_ID_0); /* emsk Pmod1[4:1] are connected to DW UART0 signals */
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_write(uartobj);

	uartobj = uart_get_dev(TEST_UART_ID_1);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_write(uartobj);

	uartobj = uart_get_dev(TEST_UART_ID_2);
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_write(uartobj);
}

UNIT_TEST(dev_uart, read){
	DEV_UART *uartobj;

	uartobj = uart_get_dev(TEST_UART_ID_0); /* emsk Pmod1[4:1] are connected to DW UART0 signals */
	UNIT_TEST_ASSERT_NOT_NULL(uartobj);
	check_uart_read(uartobj);
}
