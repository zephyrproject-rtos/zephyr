#include <zephyr/kernel.h>

#define RPMSGFS_OPEN      1
#define RPMSGFS_CLOSE     2
#define RPMSGFS_READ      3
#define RPMSGFS_WRITE     4
#define RPMSGFS_LSEEK     5
#define RPMSGFS_IOCTL     6
#define RPMSGFS_SYNC      7
#define RPMSGFS_DUP       8
#define RPMSGFS_FSTAT     9
#define RPMSGFS_FTRUNCATE 10
#define RPMSGFS_OPENDIR   11
#define RPMSGFS_READDIR   12
#define RPMSGFS_REWINDDIR 13
#define RPMSGFS_CLOSEDIR  14
#define RPMSGFS_STATFS    15
#define RPMSGFS_UNLINK    16
#define RPMSGFS_MKDIR     17
#define RPMSGFS_RMDIR     18
#define RPMSGFS_RENAME    19
#define RPMSGFS_STAT      20
#define RPMSGFS_FCHSTAT   21
#define RPMSGFS_CHSTAT    22

/*
 * RPMSGFS file mode
 */

#define RPMSGFS_O_RDONLY    BIT(0)
#define RPMSGFS_O_WRONLY    BIT(1)
#define RPMSGFS_O_CREAT     BIT(2)
#define RPMSGFS_O_EXCL      BIT(3)
#define RPMSGFS_O_APPEND    BIT(4)
#define RPMSGFS_O_TRUNC     BIT(5)
#define RPMSGFS_O_NONBLOCK  BIT(6)
#define RPMSGFS_O_SYNC      BIT(7)
#define RPMSGFS_O_BINARY    BIT(8)
#define RPMSGFS_O_DIRECT    BIT(9)
#define RPMSGFS_O_DIRECTORY BIT(11)
#define RPMSGFS_O_NOFOLLOW  BIT(12)
#define RPMSGFS_O_LARGEFILE BIT(13)
#define RPMSGFS_O_NOATIME   BIT(18)

/*
 * RPMSGFS file flags
 */

#define RPMSGFS_S_IFIFO  (1 << 12)
#define RPMSGFS_S_IFCHR  (2 << 12)
#define RPMSGFS_S_IFSEM  (3 << 12)
#define RPMSGFS_S_IFDIR  (4 << 12)
#define RPMSGFS_S_IFMQ   (5 << 12)
#define RPMSGFS_S_IFBLK  (6 << 12)
#define RPMSGFS_S_IFREG  (8 << 12)
#define RPMSGFS_S_IFMTD  (9 << 12)
#define RPMSGFS_S_IFLNK  (10 << 12)
#define RPMSGFS_S_IFSOCK (12 << 12)
#define RPMSGFS_S_IFMT   (15 << 12)

#define RPMSGFS_FMODE_READ  0x1
#define RPMSGFS_FMODE_WRITE 0x2

struct iovec {
	void *iov_base; /* Base address of I/O memory region */
	size_t iov_len; /* Size of the memory pointed to by iov_base */
};

struct rpmsgfs_cookie_s {
	struct k_sem sem;
	int result;
	void *data;
};

struct rpmsgfs_header_s {
	uint32_t command;
	int32_t result;
	uint64_t cookie;
} __attribute__((packed));

struct rpmsgfs_open_s {
	struct rpmsgfs_header_s header;
	int32_t flags;
	int32_t mode;
	char pathname[0];
} __attribute__((packed));

struct rpmsgfs_file_descriptor_s {
	struct rpmsgfs_header_s header;
	int32_t fd;
} __attribute__((packed));

struct rpmsgfs_read_write_s {
	struct rpmsgfs_header_s header;
	int32_t fd;
	uint32_t count;
	char buf[0];
} __attribute__((packed));

struct rpmsgfs_lseek_s {
	struct rpmsgfs_header_s header;
	int32_t fd;
	int32_t whence;
	int32_t offset;
} __attribute__((packed));

struct rpmsgfs_ftruncate_s {
	struct rpmsgfs_header_s header;
	int32_t fd;
	int32_t length;
} __attribute__((packed));

struct rpmsgfs_mkdir_s {
	struct rpmsgfs_header_s header;
	int32_t mode;
	uint32_t reserved;
	char pathname[0];
} __attribute__((packed));

struct rpmsgfs_pathname_s {
	struct rpmsgfs_header_s header;
	char pathname[0];
} __attribute__((packed));

struct rpmsgfs_readdir_s {
	struct rpmsgfs_header_s header;
	int32_t fd;
	uint32_t type;
	char name[0];
} __attribute__((packed));

struct rpmsgfs_stat_priv_s {
	uint32_t dev;      /* Device ID of device containing file */
	uint32_t mode;     /* File type, attributes, and access mode bits */
	uint32_t rdev;     /* Device ID (if file is character or block special) */
	uint16_t ino;      /* File serial number */
	uint16_t nlink;    /* Number of hard links to the file */
	int64_t size;      /* Size of file/directory, in bytes */
	int64_t atim_sec;  /* Time of last access, seconds */
	int64_t atim_nsec; /* Time of last access, nanoseconds */
	int64_t mtim_sec;  /* Time of last modification, seconds */
	int64_t mtim_nsec; /* Time of last modification, nanoseconds */
	int64_t ctim_sec;  /* Time of last status change, seconds */
	int64_t ctim_nsec; /* Time of last status change, nanoseconds */
	uint64_t blocks;   /* Number of blocks allocated */
	int16_t uid;       /* User ID of file */
	int16_t gid;       /* Group ID of file */
	int16_t blksize;   /* Block size used for filesystem I/O */
	uint16_t reserved; /* Reserved space */
} __attribute__((packed));

struct rpmsgfs_fstat_s {
	struct rpmsgfs_header_s header;
	struct rpmsgfs_stat_priv_s buf;
	union {
		int32_t fd;
		char pathname[0];
	};
} __attribute__((packed));

struct rpmsgfs_statfs_s {
	struct rpmsgfs_header_s header;
	uint32_t type;     /* Type of filesystem */
	uint32_t reserved; /* Reserved space */
	uint64_t namelen;  /* Maximum length of filenames */
	uint64_t bsize;    /* Optimal block size for transfers */
	uint64_t blocks;   /* Total data blocks in the file system of this size */
	uint64_t bfree;    /* Free blocks in the file system */
	uint64_t bavail;   /* Free blocks avail to non-superuser */
	uint64_t files;    /* Total file nodes in the file system */
	uint64_t ffree;    /* Free file nodes in the file system */
	char pathname[0];
} __attribute__((packed));
