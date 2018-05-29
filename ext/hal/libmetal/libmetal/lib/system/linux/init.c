/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/init.c
 * @brief	Linux libmetal initialization.
 */

#include <sys/types.h>

#include <metal/sys.h>
#include <metal/utilities.h>

struct metal_state _metal;

extern int metal_linux_irq_init();
extern void metal_linux_irq_shutdown();

/** Sort function for page size array. */
static int metal_pagesize_compare(const void *_a, const void *_b)
{
	const struct metal_page_size *a = _a, *b = _b;
	long diff = a->page_size - b->page_size;
	return metal_sign(diff);
}

static int metal_add_page_size(const char *path, int shift, int mmap_flags)
{
	int index = _metal.num_page_sizes;
	unsigned long size = 1UL << shift;

	if (index >= MAX_PAGE_SIZES) {
		metal_log(METAL_LOG_WARNING, "skipped page size %ld - overflow\n",
			  size);
		return -EOVERFLOW;
	}

	if (!path || shift <= 0) {
		metal_log(METAL_LOG_WARNING, "skipped page size %ld - invalid args\n",
			  size);
		return -EINVAL;
	}

	_metal.page_sizes[index].page_shift = shift;
	_metal.page_sizes[index].page_size = size;
	_metal.page_sizes[index].mmap_flags = mmap_flags;
	strncpy(_metal.page_sizes[index].path, path, PATH_MAX);
	_metal.num_page_sizes ++;

	metal_log(METAL_LOG_DEBUG, "added page size %ld @%s\n", size, path);

	return 0;
}

static int metal_init_page_sizes(void)
{
	const int max_sizes = MAX_PAGE_SIZES - 1;
	long sizes[max_sizes];

	/* Determine system page size. */
	sizes[0] = getpagesize();
	if (sizes[0] <= 0) {
		metal_log(METAL_LOG_ERROR, "failed to get page size\n");
		return -ENOSYS;
	}
	_metal.page_size  = sizes[0];
	_metal.page_shift = metal_log2(sizes[0]);
	metal_add_page_size(_metal.tmp_path, _metal.page_shift, 0);

#ifdef HAVE_HUGETLBFS_H
#ifndef MAP_HUGE_SHIFT
	/* System does not support multiple huge page sizes. */
	sizes[0] = gethugepagesize();
	if (sizes[0] > 0) {
		metal_add_page_size(hugetlbfs_find_path(),
				    metal_log2(sizes[0]),
				    MAP_HUGETLB);
	}
#else
	if (gethugepagesize() >= 0) {
		int i, count;

		/* System supports multiple huge page sizes. */
		count = gethugepagesizes(sizes, max_sizes);
		for (i = 0; i < count; i++) {
			int shift = metal_log2(sizes[i]);
			if ((shift & MAP_HUGE_MASK) != shift)
				continue;
			metal_add_page_size(
				hugetlbfs_find_path_for_size(sizes[i]),
				shift, (MAP_HUGETLB |
				(shift << MAP_HUGE_SHIFT)));
		}
	}
#endif
#endif

	/* Finally sort the resulting array by size. */
	qsort(_metal.page_sizes, _metal.num_page_sizes,
	      sizeof(struct metal_page_size), metal_pagesize_compare);

	return 0;
}

int metal_sys_init(const struct metal_init_params *params)
{
	static char sysfs_path[SYSFS_PATH_MAX];
	const char *tmp_path;
	unsigned int seed;
	FILE* urandom;
	int result;

	/* Determine sysfs mount point. */
	result = sysfs_get_mnt_path(sysfs_path, sizeof(sysfs_path));
	if (result) {
		metal_log(METAL_LOG_ERROR, "failed to get sysfs path (%s)\n",
			  strerror(-result));
		return result;
	}
	_metal.sysfs_path = sysfs_path;

	/* Find the temporary directory location. */
	tmp_path = getenv("TMPDIR");
	if (!tmp_path)
		tmp_path = "/tmp";
	_metal.tmp_path = tmp_path;

	/* Initialize the pseudo-random number generator. */
	urandom = fopen("/dev/urandom", "r");
	if (!urandom) {
		metal_log(METAL_LOG_ERROR, "failed to open /dev/urandom (%s)\n",
			  strerror(errno));
		return -errno;
	}
	if (sizeof(int) != fread(&seed, sizeof(int), 1, urandom)) {
		metal_log(METAL_LOG_DEBUG, "Failed fread /dev/urandom\n");
	}
	fclose(urandom);
	srand(seed);

	result = metal_init_page_sizes();
	if (result < 0)
		return result;

	result = metal_linux_bus_init();
	if (result < 0)
		return result;

	result = open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
	if (result < 0) {
		metal_log(METAL_LOG_DEBUG, "Failed pagemap open - %s\n",
			  strerror(errno));
	}
	_metal.pagemap_fd = result;

	metal_unused(params);

	/* Initialize IRQ handling */
	metal_linux_irq_init();
	return 0;
}

void metal_sys_finish(void)
{

	/* Shutdown IRQ handling */
	metal_linux_irq_shutdown();
	metal_linux_bus_finish();
	close(_metal.pagemap_fd);

}
