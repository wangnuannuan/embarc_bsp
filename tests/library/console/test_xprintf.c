/**
 * @file
 * @brief Unit test for xprintf.h
 */

#include "embARC_test.h"
#include "console/xprintf/xprintf.h"
#include "console/console_io.h"

static uint8_t def_xfunc_in(void)
{
	static uint32_t i = 0;

	i++;
	uint8_t buffer[40] = { ' ',  '.', 2, 3, 4, 5, 6, 7, '\b', '\r' };
	return buffer[i];
}

static uint8_t def_xfunc_in_null(void)
{
	return 0;
}

UNIT_TEST(xprintf, put_dump){
	char buffer[40] = { 1, 2, 3, 4, 5, 6, 7, 'g', 'k', '.' };

	buffer[36] = 'G';

	uint16_t *buffer_16;
	uint32_t *buffer_32;
	buffer_32 = ( uint32_t * ) buffer;
	buffer_16 = ( uint16_t * ) buffer;

	put_dump(buffer, 0, sizeof(buffer), sizeof(uint64_t));
	put_dump(buffer, 0, sizeof(buffer), DW_8BIT);
	put_dump(buffer_16, 0, sizeof(buffer) >> 1, DW_16BIT);
	put_dump(buffer_32, 0, sizeof(buffer) >> 2, DW_32BIT);
}

UNIT_TEST(xprintf, xfgets){
	char buffer[40];

	UNIT_TEST_ASSERT_EQUAL(0, xfgets(NULL, NULL, 0));
	UNIT_TEST_ASSERT_EQUAL(1, xfgets(def_xfunc_in, buffer, sizeof(buffer)));
	UNIT_TEST_ASSERT_EQUAL(0, xfgets(def_xfunc_in_null, buffer, sizeof(buffer)));

	UNIT_TEST_ASSERT_EQUAL(1, xgets(buffer, sizeof(buffer)));
}

UNIT_TEST(xprintf, xatoi){
	int32_t res;
	char Line[256] = "123 -5   0x3ff 0b1111 0377  w ";
	char *ptr;

	ptr = Line;
	xatoi(&ptr, &res);
}

UNIT_TEST(xprintf, xsprintf){
	xsprintf("test", "%c");

	xprintf("%d", 1234);
	xprintf("%6d,%3d%%", -200, 5);
	xprintf("%-6u", 100);
	xprintf("%ld", 12345678L);
	xprintf("%04x", 0xA3);
	xprintf("%08LX", 0x123ABC);
	xprintf("%016b", 0x550F);
	xprintf("%s", "String");
	xprintf("%-4s", "abc");
	xprintf("%4s", "abc");
	xprintf("%c", 'a');
	xprintf("%f", 10.0);

	xfprintf(xputc, "%c", 'a');
}

static void test_xvprintf(const char *fmt, ...)
{
	va_list arp;

	va_start(arp, fmt);

	xvprintf(fmt, arp);

	va_end(arp);
}

UNIT_TEST_SKIP(xprintf, xfputs){
	uint8_t data[2] = { 0, 1 };

	xfputs(NULL, NULL);
	xputs(NULL);
	test_xvprintf("xvprintf");

	UNIT_TEST_ASSERT_DATA(data, 2, data, 2);
	UNIT_TEST_ASSERT_FALSE(0);
	UNIT_TEST_ASSERT_STR("hello", "hello");
	UNIT_TEST_ASSERT_TRUE(1);
	UNIT_TEST_ASSERT_FAIL();
}