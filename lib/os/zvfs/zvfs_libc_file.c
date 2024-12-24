#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/util.h>

K_MEM_SLAB_DEFINE_STATIC(zvfs_libc_file_table, CONFIG_ZVFS_LIBC_FILE_SIZE, CONFIG_ZVFS_OPEN_MAX,
			 CONFIG_ZVFS_LIBC_FILE_ALIGN);

int zvfs_libc_file_alloc(int fd, const char *mode, FILE **fp, k_timeout_t timeout)
{
	int ret;

	__ASSERT_NO_MSG(mode != NULL);
	__ASSERT_NO_MSG(fp != NULL);

	ret = k_mem_slab_alloc(&zvfs_libc_file_table, (void **)fp, timeout);
	if (ret < 0) {
		return ret;
	}

	zvfs_libc_file_alloc_cb(fd, mode, *fp);

	return 0;
}

void zvfs_libc_file_free(FILE *fp)
{
	if (fp == NULL) {
		return;
	}

	k_mem_slab_free(&zvfs_libc_file_table, (void *)fp);
}

FILE *zvfs_libc_file_from_fd(int fd)
{
	if ((fd < 0) || (fd >= CONFIG_ZVFS_OPEN_MAX)) {
		return NULL;
	}

	return (FILE *)&_k_mem_slab_buf_zvfs_libc_file_table[CONFIG_ZVFS_LIBC_FILE_SIZE * fd];
}
