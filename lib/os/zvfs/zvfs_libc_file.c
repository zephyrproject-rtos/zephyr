#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/util.h>

/* in case these are defined as macros */
#undef stdin
#undef stdout
#undef stderr

#define table_stride ROUND_UP(CONFIG_ZVFS_LIBC_FILE_SIZE, CONFIG_ZVFS_LIBC_FILE_ALIGN)
#define table_data   ((uint8_t *)_k_mem_slab_buf_zvfs_libc_file_table)

K_MEM_SLAB_DEFINE_STATIC(zvfs_libc_file_table, CONFIG_ZVFS_LIBC_FILE_SIZE, CONFIG_ZVFS_OPEN_MAX,
			 CONFIG_ZVFS_LIBC_FILE_ALIGN);

FILE *const stdin = (FILE *)(table_data + 0 /* STDIN_FILENO */ * table_stride);
FILE *const stdout = (FILE *)(table_data + 1 /* STDOUT_FILENO */ * table_stride);
FILE *const stderr = (FILE *)(table_data + 2 /* STDERR_FILENO */ * table_stride);

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

	return (FILE *)&_k_mem_slab_buf_zvfs_libc_file_table[table_stride * fd];
}
