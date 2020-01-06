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
 * \brief unit test for dev_gpio.h
 */

#include <string.h>
#include "embARC_test.h"
#include "embARC_error.h"
#include "device/ip_hal/dev_gpio.h"
#include "embARC_debug.h"

#define TEST_GPIO_BIT0_OFS      (0)
#define TEST_GPIO_BIT0_INT_TYPE     (GPIO_INT_LEVEL_TRIG)
#define TEST_GPIO_BIT0_INT_POLARITY (GPIO_INT_ACTIVE_HIGH)
#define TEST_GPIO_BIT0_INT_DEBOUNCE (GPIO_INT_NO_DEBOUNCE)

#define UNIT_TEST_ASSERT_VOID_EQUAL(expected, real) UNIT_TEST_ASSERT_EQUAL((long)(expected), (long)(real))

static void gpio_force_close(DEV_GPIO *gpioobj)
{
	if (gpioobj == 0) {
		return;
	}
	while (gpioobj->gpio_close() != E_OK) {
		;
	}
}

static volatile uint32_t bit_isr_cnt0 = 0;
static volatile uint32_t check_extra = 0;

static void reset_all_cnt(void)
{
	bit_isr_cnt0 = 0;
	check_extra = 0;
}
static void gpio_bit_isr0(void *ptr)
{
	DEV_GPIO *gpio_obj = (DEV_GPIO *)ptr;

	bit_isr_cnt0++;
	UNIT_TEST_ASSERT_EQUAL(E_OK, gpio_obj->gpio_control(GPIO_CMD_DIS_BIT_INT, CONV2VOID(1 << TEST_GPIO_BIT0_OFS)));
	if (check_extra) {
		UNIT_TEST_ASSERT_VOID_EQUAL(gpio_bit_isr0, gpio_obj->gpio_info.extra);
	}
}

static void gpio_bit_isr_all(void *ptr)
{
	DEV_GPIO *gpio_obj = (DEV_GPIO *)ptr;
	uint32_t val_u32;

	UNIT_TEST_ASSERT_EQUAL(E_OK, gpio_obj->gpio_control(GPIO_CMD_GET_BIT_MTHD, CONV2VOID(&val_u32)));
	if (val_u32) {
		UNIT_TEST_ASSERT_EQUAL(0xFFFFFFFF, (val_u32));
	}
	UNIT_TEST_ASSERT_EQUAL(E_OK, gpio_obj->gpio_control(GPIO_CMD_DIS_BIT_INT, CONV2VOID(0xFFFFFFFF)));
	if (check_extra) {
		UNIT_TEST_ASSERT_VOID_EQUAL(gpio_bit_isr0, gpio_obj->gpio_info.extra);
	}
}

/* all gpio info related variable check functions */
static inline void check_gpio_info_direction(DEV_GPIO *gpioobj, uint32_t expected)
{
	uint32_t val_u32;

	UNIT_TEST_ASSERT_EQUAL(expected, gpioobj->gpio_info.direction);
	UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_DIR, CONV2VOID(&val_u32)));
	UNIT_TEST_ASSERT_EQUAL(expected, val_u32);
}
static inline void check_gpio_info_method(DEV_GPIO *gpioobj, uint32_t expected)
{
	uint32_t val_u32;

	UNIT_TEST_ASSERT_EQUAL(expected, gpioobj->gpio_info.method);
	UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_MTHD, CONV2VOID(&val_u32)));
	UNIT_TEST_ASSERT_EQUAL(expected, val_u32);
}
static inline void check_gpio_info_opn_cnt(DEV_GPIO *gpioobj, uint32_t expected)
{
	UNIT_TEST_ASSERT_EQUAL(expected, gpioobj->gpio_info.opn_cnt);
}
static inline void check_gpio_info_bit_int_cfg(DEV_GPIO *gpioobj, void *expected)
{
	DEV_GPIO_INT_CFG int_cfg, *p_int_cfg;

	p_int_cfg = (DEV_GPIO_INT_CFG *)expected;
	int_cfg.int_bit_mask = p_int_cfg->int_bit_mask;

	UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_INT_CFG, CONV2VOID(&int_cfg)));
	p_int_cfg->int_bit_type &= p_int_cfg->int_bit_mask;
	p_int_cfg->int_bit_polarity &= p_int_cfg->int_bit_mask;
	p_int_cfg->int_bit_debounce &= p_int_cfg->int_bit_mask;
	uint32_t val_u32 = memcmp(expected, &(int_cfg), sizeof(DEV_GPIO_INT_CFG));
	UNIT_TEST_ASSERT_EQUAL(0, val_u32);
}
static inline void check_gpio_info_bit_isr(DEV_GPIO *gpioobj, void *expected)
{
	DEV_GPIO_BIT_ISR bit_isr, *p_bit_isr;

	p_bit_isr = (DEV_GPIO_BIT_ISR *)expected;
	bit_isr.int_bit_ofs = p_bit_isr->int_bit_ofs;

	UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_ISR, CONV2VOID(&bit_isr)));
	uint32_t val_u32 = memcmp(expected, &(bit_isr), sizeof(DEV_GPIO_BIT_ISR));
	UNIT_TEST_ASSERT_EQUAL(0, val_u32);
}
static inline void check_gpio_info_extra(DEV_GPIO *gpioobj, void *expected)
{
	UNIT_TEST_ASSERT_VOID_EQUAL(expected, gpioobj->gpio_info.extra);
}

static inline void check_gpio_open(DEV_GPIO *gpioobj)
{
	gpio_force_close(gpioobj);
	if (gpioobj) {
		/** 0. test open gpio and check its predefined direction */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0xf0f));
		check_gpio_info_direction(gpioobj, 0xf0f);
		check_gpio_info_method(gpioobj, 0);
		check_gpio_info_opn_cnt(gpioobj, 1);
		check_gpio_info_bit_int_cfg(gpioobj, CONV2VOID(&gpio_int_cfg_default));
		check_gpio_info_extra(gpioobj, NULL);
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());

		/** 1. test open gpio with different direction */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0xA5A5));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0xA5A5));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());

		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(10000000));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());

		/** 2. test open 2 times with same direction */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0xA5A5));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0xA5A5));
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, gpioobj->gpio_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());

		/** 3. test open 2 times with different direction */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0xA5A5));
		check_gpio_info_direction(gpioobj, 0xA5A5);
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, gpioobj->gpio_open(0xA5A));
		check_gpio_info_direction(gpioobj, 0xA5A5);
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, gpioobj->gpio_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
	}
}

static inline void check_gpio_close(DEV_GPIO *gpioobj)
{
	gpio_force_close(gpioobj);

	if (gpioobj) {
		/** 1. test open 3 times, then close device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0xA5A5));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0xA5A5));
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, gpioobj->gpio_close());
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, gpioobj->gpio_open(0xA5A));
		UNIT_TEST_ASSERT_EQUAL(E_OPNED, gpioobj->gpio_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());

		/** 2. test close device directly without open it */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
	}
}

static inline void check_gpio_control(DEV_GPIO *gpioobj)
{
	uint32_t val_u32;
	uint32_t *val_u32_ptr = &val_u32;
	uint8_t *val_8_ptr;
	int i;
	DEV_GPIO_INT_CFG int_cfg;
	DEV_GPIO_BIT_ISR bit_isr;

	gpio_force_close(gpioobj);

	val_8_ptr = (uint8_t *)((uint32_t)(&val_u32) + 1);

	if (gpioobj) {
		/** Open it before control device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0x0));
		/** 1. test control device without open it */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
		for (i = 0; i < 100; i++) {
			UNIT_TEST_ASSERT_EQUAL(E_CLSED, gpioobj->gpio_control(i, NULL));
			UNIT_TEST_ASSERT_EQUAL(E_CLSED, gpioobj->gpio_control(i, (void *)(i << 2)));
		}

		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0x12345678));
		UNIT_TEST_ASSERT_EQUAL(E_NOSPT, gpioobj->gpio_control(DEV_SET_SYSCMD(0xffffffff), NULL));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0x0));

		/** 2. test GPIO_CMD_SET_BIT_DIR_INPUT command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_DIR_INPUT, CONV2VOID(0)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_DIR_INPUT, CONV2VOID(0xAFA)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_DIR, CONV2VOID(val_u32_ptr)));
		UNIT_TEST_ASSERT_EQUAL(0, val_u32 & 0xAFA);

		/** 3. test GPIO_CMD_SET_BIT_DIR_OUTPUT command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_DIR_OUTPUT, CONV2VOID(0)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_DIR_OUTPUT, CONV2VOID(0xa5a5a5)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_DIR, CONV2VOID(val_u32_ptr)));
		UNIT_TEST_ASSERT_EQUAL(0xa5a5a5, val_u32 & 0xa5a5a5);

		/** 4. test GPIO_CMD_GET_BIT_DIR command */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_GET_BIT_DIR, CONV2VOID(val_8_ptr)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0xabcd));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_DIR, CONV2VOID(val_u32_ptr)));
		UNIT_TEST_ASSERT_EQUAL(0xabcd, val_u32);
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0x0));

		/** 5. test GPIO_CMD_SET_BIT_INT_CFG command */
		int_cfg.int_bit_mask = 0x76543210;
		int_cfg.int_bit_type = 0x12345678;
		int_cfg.int_bit_polarity = 0x76543210;
		int_cfg.int_bit_debounce = 0xa5a5feda;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, CONV2VOID(&int_cfg)));
		check_gpio_info_bit_int_cfg(gpioobj, CONV2VOID(&int_cfg));
		int_cfg.int_bit_mask = 0xffffffff;
		int_cfg.int_bit_type = 0x12345678;
		int_cfg.int_bit_polarity = 0x76543210;
		int_cfg.int_bit_debounce = 0xa5a5feda;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, CONV2VOID(&int_cfg)));
		check_gpio_info_bit_int_cfg(gpioobj, CONV2VOID(&int_cfg));
		int_cfg.int_bit_mask = 0x0;
		int_cfg.int_bit_type = 0x12345678;
		int_cfg.int_bit_polarity = 0x76543210;
		int_cfg.int_bit_debounce = 0xa5a5feda;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, CONV2VOID(&int_cfg)));
		check_gpio_info_bit_int_cfg(gpioobj, CONV2VOID(&int_cfg));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, CONV2VOID(val_8_ptr)));

		/** 6. test GPIO_CMD_GET_BIT_INT_CFG command */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_GET_BIT_INT_CFG, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_GET_BIT_INT_CFG, CONV2VOID(val_8_ptr)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, CONV2VOID(&gpio_int_cfg_default)));
		int_cfg.int_bit_mask = gpio_int_cfg_default.int_bit_mask;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_INT_CFG, CONV2VOID(&int_cfg)));
		UNIT_TEST_ASSERT_EQUAL(0, memcmp((void *)(&gpio_int_cfg_default), (void *)(&int_cfg), sizeof(DEV_GPIO_INT_CFG)));

		/** 7. test GPIO_CMD_SET_BIT_ISR command */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_SET_BIT_ISR, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_SET_BIT_ISR, CONV2VOID(val_8_ptr)));
		bit_isr.int_bit_ofs = 1;
		bit_isr.int_bit_handler = gpio_bit_isr0;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_ISR, CONV2VOID(&bit_isr)));
		bit_isr.int_bit_ofs = 32;
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_SET_BIT_ISR, CONV2VOID(&bit_isr)));
		bit_isr.int_bit_ofs = 256;
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_SET_BIT_ISR, CONV2VOID(&bit_isr)));
		bit_isr.int_bit_ofs = 1;
		check_gpio_info_bit_isr(gpioobj, CONV2VOID(&bit_isr));

		/** 8. test GPIO_CMD_GET_BIT_ISR command */
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_GET_BIT_ISR, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_GET_BIT_ISR, CONV2VOID(val_8_ptr)));
		bit_isr.int_bit_ofs = 0xffff;
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_GET_BIT_ISR, CONV2VOID(&bit_isr)));
		bit_isr.int_bit_ofs = 2;
		bit_isr.int_bit_handler = gpio_bit_isr0;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_ISR, CONV2VOID(&bit_isr)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_ISR, CONV2VOID(&bit_isr)));
		check_gpio_info_bit_isr(gpioobj, CONV2VOID(&bit_isr));

		/** 9. test GPIO_CMD_ENA_BIT_INT command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0x0));
		int_cfg.int_bit_mask = 1 << TEST_GPIO_BIT0_OFS;
		int_cfg.int_bit_type = TEST_GPIO_BIT0_INT_TYPE << TEST_GPIO_BIT0_OFS;
		int_cfg.int_bit_polarity = TEST_GPIO_BIT0_INT_POLARITY << TEST_GPIO_BIT0_OFS;
		int_cfg.int_bit_debounce = TEST_GPIO_BIT0_INT_DEBOUNCE << TEST_GPIO_BIT0_OFS;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, CONV2VOID(&int_cfg)));
		check_gpio_info_bit_int_cfg(gpioobj, CONV2VOID(&int_cfg));
		bit_isr.int_bit_ofs = TEST_GPIO_BIT0_OFS;
		bit_isr.int_bit_handler = gpio_bit_isr0;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_ISR, CONV2VOID(&bit_isr)));
		DEV_GPIO_INFO_SET_EXTRA_OBJECT(&(gpioobj->gpio_info), gpio_bit_isr0);
		reset_all_cnt();
		check_extra = 1;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_DIR_INPUT, CONV2VOID(1 << TEST_GPIO_BIT0_OFS)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_ENA_BIT_INT, CONV2VOID(1 << TEST_GPIO_BIT0_OFS)));
		// board_delay_ms(10000);
		UNIT_TEST_ASSERT_EQUAL(1, (bit_isr_cnt0 > 0));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_DIS_BIT_INT, CONV2VOID(1 << TEST_GPIO_BIT0_OFS)));

		/** 10. test GPIO_CMD_DIS_BIT_INT command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_DIS_BIT_INT, CONV2VOID(0)));
		int_cfg.int_bit_mask = 1 << TEST_GPIO_BIT0_OFS;
		int_cfg.int_bit_type = TEST_GPIO_BIT0_INT_TYPE << TEST_GPIO_BIT0_OFS;
		int_cfg.int_bit_polarity = TEST_GPIO_BIT0_INT_POLARITY << TEST_GPIO_BIT0_OFS;
		int_cfg.int_bit_debounce = TEST_GPIO_BIT0_INT_DEBOUNCE << TEST_GPIO_BIT0_OFS;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_INT_CFG, CONV2VOID(&int_cfg)));
		bit_isr.int_bit_ofs = TEST_GPIO_BIT0_OFS;
		bit_isr.int_bit_handler = gpio_bit_isr0;
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_ISR, CONV2VOID(&bit_isr)));
		reset_all_cnt();
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_ENA_BIT_INT, CONV2VOID(1 << TEST_GPIO_BIT0_OFS)));
		UNIT_TEST_ASSERT_EQUAL(1, (bit_isr_cnt0 > 0));
		reset_all_cnt();
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_ENA_BIT_INT, CONV2VOID(1 << TEST_GPIO_BIT0_OFS)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_DIS_BIT_INT, CONV2VOID(0)));
		UNIT_TEST_ASSERT_EQUAL(1, (bit_isr_cnt0 > 0));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_DIS_BIT_INT, CONV2VOID(1 << TEST_GPIO_BIT0_OFS)));
		reset_all_cnt();
		UNIT_TEST_ASSERT_EQUAL(0, (bit_isr_cnt0));

		/** 11. test GPIO_CMD_GET_BIT_MTHD command */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_DIS_BIT_INT, CONV2VOID(0xFFFFFFFF)));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_GET_BIT_MTHD, CONV2VOID(val_8_ptr)));
		UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_control(GPIO_CMD_GET_BIT_MTHD, CONV2VOID(NULL)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_GET_BIT_MTHD, CONV2VOID(val_u32_ptr)));
		UNIT_TEST_ASSERT_EQUAL(0, (val_u32));
		for (i = 0; i < 32; i++) {
			bit_isr.int_bit_ofs = i;
			bit_isr.int_bit_handler = gpio_bit_isr_all;
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_SET_BIT_ISR, CONV2VOID(&bit_isr)));
		}
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_ENA_BIT_INT, CONV2VOID(0xFFFFFFFF)));
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_DIS_BIT_INT, CONV2VOID(0xFFFFFFFF)));

		/** Close device */
		UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
	}
}

UNIT_TEST(dev_gpio, get_dev){
	DEV_ID common_dev_id = DEV_ID_0;

	gpio_get_dev(common_dev_id);
	for (int i = 0; i < 25; i++) {
		gpio_get_dev(i);
	}
	UNIT_TEST_ASSERT_NOT_NULL(gpio_get_dev(0));
	UNIT_TEST_ASSERT_NULL(gpio_get_dev(-1));
	UNIT_TEST_ASSERT_NULL(gpio_get_dev(-100));
	UNIT_TEST_ASSERT_NULL(gpio_get_dev(100));
}

UNIT_TEST(dev_gpio, open){
	DEV_GPIO *gpioobj;

	gpioobj = gpio_get_dev(0);
	if (gpioobj) {
		check_gpio_open(gpioobj);
	}
}

UNIT_TEST(dev_gpio, close){
	DEV_GPIO *gpioobj;

	for (int i = 0; i < 25; i++) {
		gpioobj = gpio_get_dev(i);
		if (gpioobj) {
			check_gpio_close(gpioobj);
		}
	}

}

UNIT_TEST(dev_gpio, control){
	DEV_GPIO *gpioobj;

	gpioobj = gpio_get_dev(0);
	if (gpioobj) {
		check_gpio_control(gpioobj);
	}

}

UNIT_TEST(dev_gpio, write){
	DEV_GPIO *gpioobj;

	for (int i = 0; i < 25; i++) {
		gpioobj = gpio_get_dev(i);
		if (gpioobj) {
			gpio_force_close(gpioobj);
			/** Open it before control device */
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0x1ff));
			gpioobj->gpio_control(GPIO_CMD_SET_BIT_DIR_INPUT, CONV2VOID(0x1ff));
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_control(GPIO_CMD_TOGGLE_BITS, CONV2VOID(0x1ff)));
			gpioobj->gpio_control(GPIO_CMD_DIS_BIT_INT, CONV2VOID(0x1ff));

			/** 1. Test gpio write parameters */
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_write(0xFFFFFFFF, 0x1ff));
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_write(0x00000000, 0x1ff));
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_write(0x00000000, 0x1ff));
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_write(0x00000000, 0x1ff));

			/** 2. Test gpio write when device is not opened */
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
			UNIT_TEST_ASSERT_EQUAL(E_CLSED, gpioobj->gpio_write(0x1, 0x1ff));

			/** Close device */
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
		}
	}

}

UNIT_TEST(dev_gpio, read){
	DEV_GPIO *gpioobj;
	uint32_t val_u32;

	for (int i = 0; i < 25; i++) {
		gpioobj = gpio_get_dev(i);
		if (gpioobj) {
			/** Open it before control device */
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_open(0x0));

			/** 1. Test gpio read  */
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_read(&val_u32, 0xf));
			UNIT_TEST_ASSERT_EQUAL(E_PAR, gpioobj->gpio_read(NULL, 0xf));

			/** 2. Test gpio read when device is not opened */
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
			UNIT_TEST_ASSERT_EQUAL(E_CLSED, gpioobj->gpio_read(&val_u32, 0xf));

			/** Close device */
			UNIT_TEST_ASSERT_EQUAL(E_OK, gpioobj->gpio_close());
		}
	}
}
