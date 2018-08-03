/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <mmustructs.h>
#include <arch/x86/arch.h>
#include <linker/linker-defs.h>
#include <arch/x86/arch.h>
#include <ztest.h>
#include <kernel_internal.h>

#define SKIP_SIZE 5
#define BUFF_SIZE 10

static int status;

#define BUFF_READABLE ((u32_t) 0x0)
#define BUFF_WRITEABLE ((u32_t) 0x1)
#define BUFF_USER ((u32_t) 0x2)

int _arch_buffer_validate(void *addr, size_t size, int write);
void reset_flag(void);
void reset_multi_pte_page_flag(void);
void reset_multi_pde_flag(void);

#define ADDR_PAGE_1 ((void *)__bss_start + SKIP_SIZE * MMU_PAGE_SIZE)
#define ADDR_PAGE_2 ((void *)__bss_start + (SKIP_SIZE + 1) * MMU_PAGE_SIZE)
#define PRESET_PAGE_1_VALUE (X86_MMU_GET_PTE(ADDR_PAGE_1)->p = 1)
#define PRESET_PAGE_2_VALUE (X86_MMU_GET_PTE(ADDR_PAGE_2)->p = 1)


/* if Failure occurs
 * _arch_buffer_validate return -EPERM
 * else return 0.
 * Below conditions will be tested accordingly
 *
 */

/* read write testing */
static int buffer_rw_read(void)
{
	PRESET_PAGE_1_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ,
			   MMU_PDE_RW_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       BUFF_SIZE,
				       BUFF_WRITEABLE);

	if (status != -EPERM) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_flag();
	return TC_PASS;
}

static int buffer_writeable_write(void)
{
	PRESET_PAGE_1_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE,
			   MMU_PDE_RW_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       BUFF_SIZE,
				       BUFF_WRITEABLE);
	if (status != 0) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_flag();
	return TC_PASS;
}

static int buffer_readable_read(void)
{
	PRESET_PAGE_1_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ,
			   MMU_PDE_RW_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       BUFF_SIZE,
				       BUFF_READABLE);
	if (status != 0) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_flag();
	return TC_PASS;
}

static int buffer_readable_write(void)
{
	PRESET_PAGE_1_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE,
			   MMU_PDE_RW_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       BUFF_SIZE,
				       BUFF_READABLE);

	if (status != 0) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_flag();
	return TC_PASS;
}
/* supervisor test */

static int buffer_supervisor_rw(void)
{
	PRESET_PAGE_1_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       BUFF_SIZE,
				       BUFF_READABLE | BUFF_USER);

	if (status != -EPERM) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_flag();
	return TC_PASS;
}

static int buffer_supervisor_w(void)
{
	PRESET_PAGE_1_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       BUFF_SIZE,
				       BUFF_WRITEABLE);

	if (status != -EPERM) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_flag();
	return TC_PASS;
}

static int buffer_user_rw_user(void)
{
	PRESET_PAGE_1_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_USER,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);
	status = _arch_buffer_validate(ADDR_PAGE_1,
				       BUFF_SIZE,
				       BUFF_WRITEABLE | BUFF_USER);
	if (status != 0) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_flag();
	return TC_PASS;
}

static int buffer_user_rw_supervisor(void)
{
	PRESET_PAGE_1_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);
	status = _arch_buffer_validate(ADDR_PAGE_1,
				       BUFF_SIZE,
				       BUFF_WRITEABLE | BUFF_USER);
	if (status != -EPERM) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_flag();
	return TC_PASS;
}

/* Check buffer with multiple pages*/
static int multi_page_buffer_user(void)
{
	PRESET_PAGE_1_VALUE;
	PRESET_PAGE_2_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       2 * MMU_PAGE_SIZE,
				       BUFF_WRITEABLE | BUFF_USER);
	if (status != -EPERM) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_multi_pte_page_flag();
	return TC_PASS;
}

static int multi_page_buffer_write_user(void)
{
	PRESET_PAGE_1_VALUE;
	PRESET_PAGE_2_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       2 * MMU_PAGE_SIZE,
				       BUFF_WRITEABLE);
	if (status != -EPERM) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_multi_pte_page_flag();
	return TC_PASS;
}

static int multi_page_buffer_read_user(void)
{
	PRESET_PAGE_1_VALUE;
	PRESET_PAGE_2_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       2 * MMU_PAGE_SIZE,
				       BUFF_READABLE | BUFF_USER);
	if (status != -EPERM) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_multi_pte_page_flag();
	return TC_PASS;
}

static int multi_page_buffer_read(void)
{
	PRESET_PAGE_1_VALUE;
	PRESET_PAGE_2_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ | MMU_ENTRY_SUPERVISOR,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       2 * MMU_PAGE_SIZE,
				       BUFF_WRITEABLE);
	if (status != -EPERM) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_multi_pte_page_flag();
	return TC_PASS;
}

static int multi_pde_buffer_rw(void)
{
	PRESET_PAGE_1_VALUE;
	PRESET_PAGE_2_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ,
			   MMU_PDE_RW_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ,
			   MMU_PDE_RW_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       2 * MMU_PAGE_SIZE,
				       BUFF_WRITEABLE);

	if (status != -EPERM) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_multi_pde_flag();
	return TC_PASS;
}

static int multi_pde_buffer_writeable_write(void)
{
	PRESET_PAGE_1_VALUE;
	PRESET_PAGE_2_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE,
			   MMU_PDE_RW_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE,
			   MMU_PDE_RW_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       2 * MMU_PAGE_SIZE,
				       BUFF_WRITEABLE);
	if (status != 0) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_multi_pde_flag();
	return TC_PASS;
}

static int multi_pde_buffer_readable_read(void)
{
	PRESET_PAGE_1_VALUE;
	PRESET_PAGE_2_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ,
			   MMU_PDE_RW_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_READ,
			   MMU_PDE_RW_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       2 * MMU_PAGE_SIZE,
				       BUFF_READABLE);
	if (status != 0) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_multi_pde_flag();
	return TC_PASS;
}

static int multi_pde_buffer_readable_write(void)
{
	PRESET_PAGE_1_VALUE;
	PRESET_PAGE_2_VALUE;

	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE,
			   MMU_PDE_RW_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE,
			   MMU_PDE_RW_MASK);

	status = _arch_buffer_validate(ADDR_PAGE_1,
				       2 * MMU_PAGE_SIZE,
				       BUFF_READABLE);

	if (status != 0) {
		TC_PRINT("%s failed\n", __func__);
		return TC_FAIL;
	}
	reset_multi_pde_flag();
	return TC_PASS;
}

void reset_flag(void)
{
	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_USER,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);
}

void reset_multi_pte_page_flag(void)
{
	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_USER,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_USER,
			   MMU_PTE_RW_MASK | MMU_PTE_US_MASK);
}

void reset_multi_pde_flag(void)
{
	_x86_mmu_set_flags(ADDR_PAGE_1,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_USER,
			   MMU_PDE_RW_MASK | MMU_PDE_US_MASK);

	_x86_mmu_set_flags(ADDR_PAGE_2,
			   MMU_PAGE_SIZE,
			   MMU_ENTRY_WRITE | MMU_ENTRY_USER,
			   MMU_PDE_RW_MASK | MMU_PDE_US_MASK);
}

/**
 * @brief Verify read from multiple pages of buffer with write access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_multi_pde_buffer_readable_write(void)
{
	zassert_true(multi_pde_buffer_readable_write() == TC_PASS, NULL);
}

/**
 * @brief Verify read to multiple pages of buffer with read access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see  _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_multi_pde_buffer_readable_read(void)
{
	zassert_true(multi_pde_buffer_readable_read() == TC_PASS, NULL);
}

/**
 * @brief Verify write to 2 pages of buffer with write access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see  _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_multi_pde_buffer_writeable_write(void)
{
	zassert_true(multi_pde_buffer_writeable_write() == TC_PASS, NULL);
}

/**
 * @brief Read from multiple pages from buffer with write access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see  _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_multi_pde_buffer_rw(void)
{
	zassert_true(multi_pde_buffer_rw() == TC_PASS, NULL);
}

/**
 * @brief Test to write to buffer which has read access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_buffer_rw_read(void)
{
	zassert_true(buffer_rw_read() == TC_PASS, NULL);
}

/**
 * @brief Test to write to buffer which has write access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_buffer_writeable_write(void)
{
	zassert_true(buffer_writeable_write() == TC_PASS, NULL);
}

/**
 * @brief Test to read from buffer with read access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_buffer_readable_read(void)
{
	zassert_true(buffer_readable_read() == TC_PASS, NULL);
}

/**
 * @brief Test to read from a buffer with write access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_buffer_readable_write(void)
{
	zassert_true(buffer_readable_write() == TC_PASS, NULL);
}

/**
 * @brief Verify read as user from buffer which has write access to supervisor
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_buffer_supervisor_rw(void)
{
	zassert_true(buffer_supervisor_rw() == TC_PASS, NULL);
}

/**
 * @brief Verify write to buffer which has write access to supervisor
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_buffer_supervisor_w(void)
{
	zassert_true(buffer_supervisor_w() == TC_PASS, NULL);
}

/**
 * @brief Verify write as user to buffer with write permission
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_buffer_user_rw_user(void)
{
	zassert_true(buffer_user_rw_user() == TC_PASS, NULL);
}

/**
 * @brief Verify write as user to buffer which has write from supervisor
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_buffer_user_rw_supervisor(void)
{
	zassert_true(buffer_user_rw_supervisor() == TC_PASS, NULL);
}

/**
 * @brief Verify write/user to buffer with 2 pages having write/supervisor
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_multi_page_buffer_user(void)
{
	zassert_true(multi_page_buffer_user() == TC_PASS, NULL);
}

/**
 * @brief Verify write to buffer with 2 pages having write/supervisor
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_multi_page_buffer_write_user(void)
{
	zassert_true(multi_page_buffer_write_user() == TC_PASS, NULL);
}

/**
 * @brief Verify read as user to buffer with read/supervisor access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_multi_page_buffer_read_user(void)
{
	zassert_true(multi_page_buffer_read_user() == TC_PASS, NULL);
}

/**
 * @brief Verify write to buffer with read/supervisor access
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see _arch_buffer_validate(), _x86_mmu_set_flags()
 */
void test_multi_page_buffer_read(void)
{
	zassert_true(multi_page_buffer_read() == TC_PASS, NULL);
}
