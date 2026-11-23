/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Meta
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define REG_ADDR  0x1000U
#define REG_VALUE 0x0123456789abcdefULL
#define LO_WORD   0x89abcdefU
#define HI_WORD   0x01234567U

#ifdef CONFIG_BIG_ENDIAN
#define LO_ADDR (REG_ADDR + 4U)
#define HI_ADDR REG_ADDR
#else
#define LO_ADDR REG_ADDR
#define HI_ADDR (REG_ADDR + 4U)
#endif

struct access {
	bool write;
	mem_addr_t addr;
	uint32_t data;
};

static struct access accesses[2];
static size_t access_count;

static void record(bool write, mem_addr_t addr, uint32_t data)
{
	zassert_true(access_count < ARRAY_SIZE(accesses), "too many accesses");
	accesses[access_count] = (struct access){.write = write, .addr = addr, .data = data};
	access_count++;
}

static uint32_t mock_read32(mem_addr_t addr)
{
	uint32_t data = (addr == LO_ADDR) ? LO_WORD : HI_WORD;

	record(false, addr, data);
	return data;
}

static void mock_write32(uint32_t data, mem_addr_t addr)
{
	record(true, addr, data);
}

/* Route the helpers' 32-bit accesses to the mocks. */
#define sys_read32(addr)        mock_read32(addr)
#define sys_write32(data, addr) mock_write32(data, addr)

#include <zephyr/sys/sys_io_non_atomic.h>

static void check_accesses(bool write, mem_addr_t first_addr, uint32_t first_data,
			   mem_addr_t second_addr, uint32_t second_data)
{
	zassert_equal(access_count, 2U);
	zassert_equal(accesses[0].write, write);
	zassert_equal(accesses[0].addr, first_addr);
	zassert_equal(accesses[0].data, first_data);
	zassert_equal(accesses[1].write, write);
	zassert_equal(accesses[1].addr, second_addr);
	zassert_equal(accesses[1].data, second_data);
}

ZTEST(sys_io_non_atomic, test_read64_lo_hi)
{
	zassert_equal(sys_read64_lo_hi(REG_ADDR), REG_VALUE);
	check_accesses(false, LO_ADDR, LO_WORD, HI_ADDR, HI_WORD);
}

ZTEST(sys_io_non_atomic, test_read64_hi_lo)
{
	zassert_equal(sys_read64_hi_lo(REG_ADDR), REG_VALUE);
	check_accesses(false, HI_ADDR, HI_WORD, LO_ADDR, LO_WORD);
}

ZTEST(sys_io_non_atomic, test_write64_lo_hi)
{
	sys_write64_lo_hi(REG_VALUE, REG_ADDR);
	check_accesses(true, LO_ADDR, LO_WORD, HI_ADDR, HI_WORD);
}

ZTEST(sys_io_non_atomic, test_write64_hi_lo)
{
	sys_write64_hi_lo(REG_VALUE, REG_ADDR);
	check_accesses(true, HI_ADDR, HI_WORD, LO_ADDR, LO_WORD);
}

static void reset_accesses(void *fixture)
{
	ARG_UNUSED(fixture);

	access_count = 0U;
}

ZTEST_SUITE(sys_io_non_atomic, NULL, NULL, reset_accesses, NULL, NULL);
