/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ brief tests for the mem_probe functionalities
 */

#include <stdio.h>
#include <misc/util.h>
#include <linker/linker-defs.h>
#include <debug/mem_safe.h>
#include <tc_util.h>
#include <cache.h>

#define MY_DATA_SIZE 16
const char __aligned(4) real_rodata[MY_DATA_SIZE] = "0123456789abcdef";
char *rodata = (char *)real_rodata;
char __aligned(4) rwdata[MY_DATA_SIZE+1];
char __aligned(4) buffer[MY_DATA_SIZE+1];

#if MY_DATA_SIZE != 16
  #error never verified with values other than 16!
#endif

#define ROM_START ((u32_t)&_image_rom_start)
#define ROM_END   ((u32_t)&_image_rom_end)
#define RAM_START ((u32_t)&_image_ram_start)
#define RAM_END   ((u32_t)&_image_ram_end)

char * const p_image_rom_start = (char *)ROM_START;
char * const p_image_rom_end   = (char *)ROM_END;
char * const p_image_ram_start = (char *)RAM_START;
char * const p_image_ram_end   = (char *)RAM_END;

char *rw_data_after_image = (char *)(RAM_END + KB(1));
char *rw_data_after_image_end;
char *ro_data_after_image = (char *)(RAM_END + KB(3));
char *ro_data_after_image_end;

int foo;

#define PROBE_BUFFER_SIZE 32
char top_of_ram[PROBE_BUFFER_SIZE] __in_section(top_of_image_ram, 0, 0);
char bottom_of_ram[PROBE_BUFFER_SIZE] __in_section(bottom_of_image_ram, 0, 0);

static void update_rv(int *rv, int last_result)
{
	*rv = *rv == TC_FAIL ? *rv : last_result;
	if (last_result == TC_FAIL) {
		TC_PRINT("FAIL\n");
	} else {
		TC_PRINT("PASS\n");
	}
}

#define RO 0
#define RW 1
#define INVALID -1
static int mem_range_check(const void *p)
{
	u32_t addr = (u32_t)p;

	if (addr >= ROM_START && addr < ROM_END) {
		return RO;
	} else if (addr >= RAM_START && addr < RAM_END) {
		return RW;
	} else {
		return INVALID;
	}
}

static int test_width(char *mem, int perm, int width, int expected)
{
	char *rights_str =  perm == SYS_MEM_SAFE_READ ? "READ" :
						perm == SYS_MEM_SAFE_WRITE ? "WRITE" :
						"INVALID ACCESS";
	int mem_type = mem_range_check(mem);
	char *mem_range_str = mem_type == RO ? "RO" :
						  mem_type == RW ? "RW" : "out-of-image";

	TC_PRINT("testing %s of %s on %s memory with width %d.......",
			expected == 0 ? "SUCCESS" : "FAILURE",
			rights_str, mem_range_str, width);

	int rv = _mem_probe(mem, perm, width, buffer);

	return rv == expected ? TC_PASS : TC_FAIL;
}

static int test_width_read(char *mem, int width, int expected)
{
	return test_width(mem, SYS_MEM_SAFE_READ, width, expected);
}

static int test_width_write(char *mem, int width, int expected)
{
	return test_width(mem, SYS_MEM_SAFE_WRITE, width, expected);
}

typedef int (*access_func)(void *, char *, size_t, int);
static int test_mem_safe_access(void *p, char *buf, int size,
									int width, int perm)
{
	int rc;
	char *func_str = (perm == SYS_MEM_SAFE_WRITE) ? "write" : "read";

	access_func func = (perm == SYS_MEM_SAFE_WRITE) ?
			_mem_safe_write : _mem_safe_read;

	TC_PRINT("testing SUCCESS of _mem_safe_%s(size: %d, width: %d).......",
				func_str, size, width);
	rc = func(p, buf, size, width);
	if (rc < 0) {
		TC_PRINT("(%d)", rc);
		return TC_FAIL;
	}

	if (memcmp(p, buf, size) != 0) {
		TC_PRINT("(bad data)");
		return TC_FAIL;
	}

	return TC_PASS;
}

void main(void)
{
	/* reference symbols so that they get included */
	top_of_ram[0] = 'a';
	bottom_of_ram[PROBE_BUFFER_SIZE - 1] = 'z';

	int rv = TC_PASS;
	int rc; /* temporary return code */

	buffer[MY_DATA_SIZE] = '\0';
	rwdata[MY_DATA_SIZE] = '\0';

	TC_START("safe memory access routines\n");

	/****
	 *  _mem_probe()
	 */

	/* test access perm */

	update_rv(&rv, test_width_read(rodata, 1, 0));
	update_rv(&rv, test_width_read(rodata, 2, 0));
	update_rv(&rv, test_width_read(rodata, 4, 0));

	update_rv(&rv, test_width_write(rodata, 1, -EFAULT));
	update_rv(&rv, test_width_write(rodata, 2, -EFAULT));
	update_rv(&rv, test_width_write(rodata, 4, -EFAULT));

	update_rv(&rv, test_width_read(rwdata, 1, 0));
	update_rv(&rv, test_width_read(rwdata, 2, 0));
	update_rv(&rv, test_width_read(rwdata, 4, 0));

	update_rv(&rv, test_width_write(rwdata, 1, 0));
	update_rv(&rv, test_width_write(rwdata, 2, 0));
	update_rv(&rv, test_width_write(rwdata, 4, 0));

	const int invalid_access_right = 3;

	update_rv(&rv, test_width(rwdata, invalid_access_right, 4, -EINVAL));

	/* test alignments constraints */

	update_rv(&rv, test_width_read(rodata, 0, -EINVAL));
	update_rv(&rv, test_width_read(rodata, 1, 0));
	update_rv(&rv, test_width_read(rodata, 2, 0));
	update_rv(&rv, test_width_read(rodata, 3, -EINVAL));
	update_rv(&rv, test_width_read(rodata, 4, 0));
	update_rv(&rv, test_width_read(rodata, 5, -EINVAL));
	update_rv(&rv, test_width_read(rodata, 8, -EINVAL));

	/* test image limits */

	update_rv(&rv, test_width_read(p_image_rom_start, 1, 0));
	update_rv(&rv, test_width_read(p_image_rom_end - 1, 1, 0));
	update_rv(&rv, test_width_read(p_image_ram_start, 1, 0));
	update_rv(&rv, test_width_read(p_image_ram_end - 1, 1, 0));

	update_rv(&rv, test_width_write(p_image_rom_start, 1, -EFAULT));
	update_rv(&rv, test_width_write(p_image_rom_end - 1, 1, -EFAULT));
	update_rv(&rv, test_width_write(p_image_ram_start, 1, 0));
	update_rv(&rv, test_width_write(p_image_ram_end - 1, 1, 0));

	update_rv(&rv, test_width_read(p_image_rom_start - 1, 1, -EFAULT));
	update_rv(&rv, test_width_read(p_image_ram_end, 1, -EFAULT));

	/* test out-of-image valid regions */

	rw_data_after_image_end = rw_data_after_image + KB(1);
	ro_data_after_image_end = ro_data_after_image + KB(1);

	TC_PRINT("testing SUCCESS of adding extra RO region.......");
	int region_add_rc = _mem_safe_region_add(ro_data_after_image, KB(1),
							SYS_MEM_SAFE_READ);
	if (region_add_rc < 0) {
		update_rv(&rv, TC_FAIL);
		TC_PRINT("FAIL (%d)\n", region_add_rc);
	} else {
		TC_PRINT("PASS\n");
	}

	TC_PRINT("testing SUCCESS of adding extra RW region.......");
	region_add_rc = _mem_safe_region_add(rw_data_after_image, KB(1),
							SYS_MEM_SAFE_WRITE);
	if (region_add_rc < 0) {
		update_rv(&rv, TC_FAIL);
		TC_PRINT("FAIL (%d)\n", region_add_rc);
	} else {
		TC_PRINT("PASS\n");
	}

	TC_PRINT("testing FAILURE of adding extra region that won't fit.......");
	region_add_rc = _mem_safe_region_add(rw_data_after_image, KB(1),
							SYS_MEM_SAFE_WRITE);
	if (region_add_rc < 0) {
		TC_PRINT("PASS\n");
	} else {
		TC_PRINT("FAIL\n");
	}

	update_rv(&rv, test_width_read(ro_data_after_image, 1, 0));
	update_rv(&rv, test_width_read(ro_data_after_image_end - 1, 1, 0));
	update_rv(&rv, test_width_read(rw_data_after_image, 1, 0));
	update_rv(&rv, test_width_read(rw_data_after_image_end - 1, 1, 0));

	update_rv(&rv, test_width_write(ro_data_after_image, 1, -EFAULT));
	update_rv(&rv, test_width_write(ro_data_after_image_end - 1, 1, -EFAULT));
	update_rv(&rv, test_width_write(rw_data_after_image, 1, 0));
	update_rv(&rv, test_width_write(rw_data_after_image_end - 1, 1, 0));

	update_rv(&rv, test_width_read(ro_data_after_image - 1, 1, -EFAULT));
	update_rv(&rv, test_width_read(ro_data_after_image_end, 1, -EFAULT));
	update_rv(&rv, test_width_read(rw_data_after_image - 1, 1, -EFAULT));
	update_rv(&rv, test_width_read(rw_data_after_image_end, 1, -EFAULT));

	/*
	 * Test the dividing line between rom and ram, even in non-xip images:
	 * it might hit ROM or invalid memory, but never RAM.
	 */
	update_rv(&rv, test_width_write(p_image_ram_start - 1, 1, -EFAULT));

	TC_PRINT("testing SUCCESS of _mem_probe() reading RO values.......");
	(void)_mem_probe(rodata, SYS_MEM_SAFE_READ, 4, buffer);
	(void)_mem_probe(rodata+4, SYS_MEM_SAFE_READ, 4, buffer+4);
	(void)_mem_probe(rodata+8, SYS_MEM_SAFE_READ, 2, buffer+8);
	(void)_mem_probe(rodata+10, SYS_MEM_SAFE_READ, 2, buffer+10);
	(void)_mem_probe(rodata+12, SYS_MEM_SAFE_READ, 1, buffer+12);
	(void)_mem_probe(rodata+13, SYS_MEM_SAFE_READ, 1, buffer+13);
	(void)_mem_probe(rodata+14, SYS_MEM_SAFE_READ, 1, buffer+14);
	(void)_mem_probe(rodata+15, SYS_MEM_SAFE_READ, 1, buffer+15);

	if (memcmp(rodata, buffer, 16) != 0) {
		TC_PRINT("FAIL\n");
		update_rv(&rv, TC_FAIL);
	} else {
		TC_PRINT("PASS\n");
	}

	memcpy(rwdata, rodata, MY_DATA_SIZE);
	memset(buffer, '-', MY_DATA_SIZE);

	TC_PRINT("testing SUCCESS of _mem_probe() reading RW values.......");
	(void)_mem_probe(rwdata, SYS_MEM_SAFE_READ, 4, buffer);
	(void)_mem_probe(rwdata+4, SYS_MEM_SAFE_READ, 4, buffer+4);
	(void)_mem_probe(rwdata+8, SYS_MEM_SAFE_READ, 2, buffer+8);
	(void)_mem_probe(rwdata+10, SYS_MEM_SAFE_READ, 2, buffer+10);
	(void)_mem_probe(rwdata+12, SYS_MEM_SAFE_READ, 1, buffer+12);
	(void)_mem_probe(rwdata+13, SYS_MEM_SAFE_READ, 1, buffer+13);
	(void)_mem_probe(rwdata+14, SYS_MEM_SAFE_READ, 1, buffer+14);
	(void)_mem_probe(rwdata+15, SYS_MEM_SAFE_READ, 1, buffer+15);

	if (memcmp(rwdata, buffer, 16) != 0) {
		TC_PRINT("FAIL\n");
		update_rv(&rv, TC_FAIL);
	} else {
		TC_PRINT("PASS\n");
	}

	memcpy(buffer, rodata, MY_DATA_SIZE);
	memset(rwdata, '-', MY_DATA_SIZE);

	TC_PRINT("testing SUCCESS of _mem_probe() writing values.......");
	(void)_mem_probe(rwdata, SYS_MEM_SAFE_WRITE, 4, buffer);
	(void)_mem_probe(rwdata+4, SYS_MEM_SAFE_WRITE, 4, buffer+4);
	(void)_mem_probe(rwdata+8, SYS_MEM_SAFE_WRITE, 2, buffer+8);
	(void)_mem_probe(rwdata+10, SYS_MEM_SAFE_WRITE, 2, buffer+10);
	(void)_mem_probe(rwdata+12, SYS_MEM_SAFE_WRITE, 1, buffer+12);
	(void)_mem_probe(rwdata+13, SYS_MEM_SAFE_WRITE, 1, buffer+13);
	(void)_mem_probe(rwdata+14, SYS_MEM_SAFE_WRITE, 1, buffer+14);
	(void)_mem_probe(rwdata+15, SYS_MEM_SAFE_WRITE, 1, buffer+15);

	if (memcmp(rwdata, buffer, 16) != 0) {
		TC_PRINT("FAIL\n");
		update_rv(&rv, TC_FAIL);
	} else {
		TC_PRINT("PASS\n");
	}

	/*****
	 * _mem_safe_read()
	 */

	memset(buffer, '-', MY_DATA_SIZE);

	update_rv(&rv, test_mem_safe_access(rodata, buffer, MY_DATA_SIZE,
								0, SYS_MEM_SAFE_READ));

	update_rv(&rv, test_mem_safe_access(rodata, buffer, MY_DATA_SIZE,
								4, SYS_MEM_SAFE_READ));

	update_rv(&rv, test_mem_safe_access(rodata, buffer, MY_DATA_SIZE-2,
								2, SYS_MEM_SAFE_READ));

	update_rv(&rv, test_mem_safe_access(rodata, buffer, MY_DATA_SIZE-1,
								1, SYS_MEM_SAFE_READ));

	TC_PRINT("testing FAILURE of _mem_safe_read() with bad params.......");
	rc = _mem_safe_read(rodata+1, buffer, MY_DATA_SIZE-1, 2);
	if (rc == 0) {
		TC_PRINT("FAIL\n");
		update_rv(&rv, TC_FAIL);
	} else {
		TC_PRINT("PASS (%d)\n", rc);
	}


	/*****
	 * _mem_safe_write()
	 */

	memcpy(buffer, rodata, MY_DATA_SIZE);
	memset(rwdata, '-', MY_DATA_SIZE);
	update_rv(&rv, test_mem_safe_access(rwdata, buffer, MY_DATA_SIZE,
								0, SYS_MEM_SAFE_WRITE));

	memcpy(buffer, rodata, MY_DATA_SIZE);
	memset(rwdata, '-', MY_DATA_SIZE);
	update_rv(&rv, test_mem_safe_access(rwdata, buffer, MY_DATA_SIZE,
								4, SYS_MEM_SAFE_WRITE));

	memcpy(buffer, rodata, MY_DATA_SIZE);
	memset(rwdata, '-', MY_DATA_SIZE);
	update_rv(&rv, test_mem_safe_access(rwdata, buffer, MY_DATA_SIZE - 2,
								2, SYS_MEM_SAFE_WRITE));

	memcpy(buffer, rodata, MY_DATA_SIZE);
	memset(rwdata, '-', MY_DATA_SIZE);
	update_rv(&rv, test_mem_safe_access(rwdata, buffer, MY_DATA_SIZE - 1,
								1, SYS_MEM_SAFE_WRITE));

	memcpy(buffer, rodata, MY_DATA_SIZE);
	memset(rwdata, '-', MY_DATA_SIZE);

	TC_PRINT("testing FAILURE of _mem_safe_write() with bad params.......");
	rc = _mem_safe_write(rwdata+1, buffer, MY_DATA_SIZE-1, 2);
	if (rc == 0) {
		TC_PRINT("FAIL\n");
		update_rv(&rv, TC_FAIL);
	} else {
		TC_PRINT("PASS (%d)\n", rc);
	}

#if !defined(CONFIG_XIP)
	/*****
	 * _mem_safe_write_to_text_section()
	 */

	extern void add_ten_to_foo(void);
	foo = 0;
	memset(buffer, 0x90, 7);

	TC_PRINT("testing FAILURE of _mem_safe_write_to_text_section(&data).......");
	if (_mem_safe_write_to_text_section(&foo, buffer, 1) == 0) {
		TC_PRINT("FAIL\n");
	} else {
		TC_PRINT("PASS\n");
	}

	TC_PRINT("testing SUCCESS of _mem_safe_write_to_text_section(&text).......");
	add_ten_to_foo();
	if (foo != 10) {
		TC_PRINT("FAIL\n");
	} else {
		if (_mem_safe_write_to_text_section((void *)add_ten_to_foo,
								buffer, 7) < 0) {
			TC_PRINT("FAIL\n");
		} else {

			sys_cache_flush((vaddr_t)add_ten_to_foo, 7);
			add_ten_to_foo();
			if (foo != 10) {
				TC_PRINT("FAIL\n");
			} else {
				TC_PRINT("PASS\n");
			}
		}
	}
#endif

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
