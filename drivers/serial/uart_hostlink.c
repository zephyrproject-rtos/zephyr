/*
 * Copyright (c) 2023 Synopsys Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <string.h>

#define DT_DRV_COMPAT snps_hostlink_uart

/* Only supported by HW and nSIM targets */
BUILD_ASSERT(!IS_ENABLED(CONFIG_QEMU_TARGET));
/* Only supported by ARC targets */
BUILD_ASSERT(IS_ENABLED(CONFIG_ARC));

#define HL_SYSCALL_OPEN		0
#define HL_SYSCALL_CLOSE	1
#define HL_SYSCALL_READ		2
#define HL_SYSCALL_WRITE	3
#define HL_SYSCALL_LSEEK	4
#define HL_SYSCALL_UNLINK	5
#define HL_SYSCALL_ISATTY	6
#define HL_SYSCALL_TMPNAM	7
#define HL_SYSCALL_GETENV	8
#define HL_SYSCALL_CLOCK	9
#define HL_SYSCALL_TIME		10
#define HL_SYSCALL_RENAME	11
#define HL_SYSCALL_ARGC		12
#define HL_SYSCALL_ARGV		13
#define HL_SYSCALL_RETCODE	14
#define HL_SYSCALL_ACCESS	15
#define HL_SYSCALL_GETPID	16
#define HL_SYSCALL_GETCWD	17
#define HL_SYSCALL_USER		18

#ifndef __noinline
#define __noinline __attribute__((noinline))
#endif /* __noinline */

#define HL_VERSION 1

/* "No message here" mark.  */
#define HL_NOADDRESS 0xFFFFFFFF

/* TODO: if we want to carve some additional space we can use the actual maximum processor cache
 * line size here (i.e 128)
 */
#define HL_MAX_DCACHE_LINE 256

/* Hostlink gateway structure.  */
struct hl_hdr {
	uint32_t version;		/* Current version is 1.  */
	uint32_t target2host_addr;	/* Packet address from target to host.  */
	uint32_t host2target_addr;	/* Packet address from host to target.  */
	uint32_t buf_addr;		/* Address for host to write answer.  */
	uint32_t payload_size;		/* Buffer size without packet header.  */
	uint32_t options;		/* For future use.  */
	uint32_t break_to_mon_addr;	/* For future use.  */
} __packed;

/* Hostlink packet header.  */
struct hl_pkt_hdr {
	uint32_t packet_id;	/* Packet id.  Always set to 1 here.  */
	uint32_t total_size;	/* Size of packet including header.  */
	uint32_t priority;	/* For future use.  */
	uint32_t type;		/* For future use.  */
	uint32_t checksum;	/* For future use.  */
} __packed;

struct hl_packed_int {
	volatile uint16_t type;
	volatile uint16_t size;
	volatile int32_t value;
} __packed;

struct hl_packed_short_buff {
	volatile uint16_t type;
	volatile uint16_t size;
	volatile uint8_t payload_short[4];
} __packed;

BUILD_ASSERT(sizeof(struct hl_packed_int) == sizeof(struct hl_packed_short_buff));

struct hl_pkt_write_char_put {
	struct hl_packed_int syscall_nr;
	struct hl_packed_int fd;
	struct hl_packed_short_buff buff;
	struct hl_packed_int nbyte;
} __packed;

struct hl_pkt_write_char_get {
	struct hl_packed_int byte_written;
	struct hl_packed_int host_errno;
} __packed;

#define MAX_PKT_SZ MAX(sizeof(struct hl_pkt_write_char_put), sizeof(struct hl_pkt_write_char_get))
#define HL_HEADERS_SZ (sizeof(struct hl_hdr) + sizeof(struct hl_pkt_hdr))
BUILD_ASSERT(HL_HEADERS_SZ + MAX_PKT_SZ < HL_MAX_DCACHE_LINE);

union payload_u {
	struct hl_pkt_write_char_put pkt_write_char_put;
	struct hl_pkt_write_char_get pkt_write_char_get;
	char reserved[HL_MAX_DCACHE_LINE - HL_HEADERS_SZ];
} __packed;

BUILD_ASSERT(sizeof(union payload_u) % 4 == 0);

/* Main hostlink structure.  */
struct hl {
	/* General hostlink information.  */
	volatile struct hl_hdr hdr;
	/* Start of the hostlink buffer.  */
	volatile struct hl_pkt_hdr pkt_hdr;
	/* Payload buffer */
	volatile union payload_u payload;
} __aligned(HL_MAX_DCACHE_LINE) __packed;

/* In general we must exactly fit into one or multiple cache lines as we shouldn't share hostlink
 * buffer (which is uncached) with any cached data
 */
BUILD_ASSERT(sizeof(struct hl) % HL_MAX_DCACHE_LINE == 0);
/* However, with current supported functionality we fit into one MAX cache line. If we add
 * some features which require bigger payload buffer this might become not true.
 */
BUILD_ASSERT(sizeof(struct hl) == HL_MAX_DCACHE_LINE);

/* Main structure. Do not rename as nSIM simulator / MDB debugger looks for the '__HOSTLINK__'
 * symbol. We need to keep it initialized so it won't be put into BSS (so we won't write with
 * regular cached access in it).
 */
volatile struct hl __HOSTLINK__ = {
	.hdr = {
		.version = HL_VERSION,
		.target2host_addr = HL_NOADDRESS
	}
};

BUILD_ASSERT(sizeof(__HOSTLINK__) % HL_MAX_DCACHE_LINE == 0);

#if defined(__CCAC__)
#define HL_HAS_C_ACCESSORS 0
#elif defined(CONFIG_ISA_ARCV3)
#define HL_HAS_C_ACCESSORS 0
#else
#define HL_HAS_C_ACCESSORS 1
#endif

#if HL_HAS_C_ACCESSORS

#ifndef __uncached
#define __uncached __attribute__((uncached))
#endif /* __uncached */

static inline void hl_write32(volatile void *addr, uint32_t val)
{
	*(volatile __uncached uint32_t *)addr = val;
}

static inline void hl_write16(volatile void *addr, uint16_t val)
{
	*(volatile __uncached uint16_t *)addr = val;
}

static inline void hl_write8(volatile void *addr, uint8_t val)
{
	*(volatile __uncached uint8_t *)addr = val;
}

static inline uint32_t hl_read32(volatile void *addr)
{
	return *(volatile __uncached uint32_t *)addr;
}

static inline uint16_t hl_read16(volatile void *addr)
{
	return *(volatile __uncached uint16_t *)addr;
}
#else
static inline void hl_write32(volatile void *addr, uint32_t val)
{
	__asm__ __volatile__("st.di %0, [%1]" :: "r" (val), "r" (addr) : "memory");
}

static inline void hl_write16(volatile void *addr, uint16_t val)
{
	__asm__ __volatile__("stb.di %0, [%1]" :: "r" (val), "r" (addr) : "memory");
}

static inline void hl_write8(volatile void *addr, uint8_t val)
{
	__asm__ __volatile__("sth.di %0, [%1]" :: "r" (val), "r" (addr) : "memory");
}

static inline uint32_t hl_read32(volatile void *addr)
{
	uint32_t w;

	__asm__ __volatile__("ld.di %0, [%1]" : "=r" (w) : "r" (addr) : "memory");

	return w;
}

static inline uint16_t hl_read16(volatile void *addr)
{
	uint16_t w;

	__asm__ __volatile__("ld.di %0, [%1]" : "=r" (w) : "r" (addr) : "memory");

	return w;
}
#endif /* HL_HAS_C_ACCESSORS */

/* Get hostlink payload size (iochunk + reserved space).  */
static uint32_t hl_payload_size(void)
{
	return sizeof(__HOSTLINK__.payload);
}

#define ALIGN(x, y) (((x) + ((y) - 1)) & ~((y) - 1))

/* Fill hostlink packet header.  */
static void hl_pkt_init(volatile struct hl_pkt_hdr *pkt, int size)
{
	hl_write32(&pkt->packet_id, 1);
	hl_write32(&pkt->total_size, ALIGN(size, 4) + sizeof(struct hl_pkt_hdr));
	hl_write32(&pkt->priority, 0);
	hl_write32(&pkt->type, 0);
	hl_write32(&pkt->checksum, 0);
}

/* Send hostlink packet to the host.  */
static void hl_static_send(size_t payload_used)
{
	/* We are OK to cast pointer to uint32_t even on 64bit platforms as we support to build
	 * Zephyr on ARCv3 64bit only to lower 4GiB. Still we need to cast via uintptr_t to avoid
	 * compiler warnings.
	 */
	uint32_t buf_addr = (uint32_t)(uintptr_t)(&__HOSTLINK__.pkt_hdr);

	hl_pkt_init(&__HOSTLINK__.pkt_hdr, payload_used);

	hl_write32(&__HOSTLINK__.hdr.buf_addr, buf_addr);
	hl_write32(&__HOSTLINK__.hdr.payload_size, hl_payload_size());
	hl_write32(&__HOSTLINK__.hdr.host2target_addr, HL_NOADDRESS);
	hl_write32(&__HOSTLINK__.hdr.version, HL_VERSION);
	hl_write32(&__HOSTLINK__.hdr.options, 0);
	hl_write32(&__HOSTLINK__.hdr.break_to_mon_addr, 0);

	compiler_barrier();

	/* This tells the debugger we have a command.
	 * It is responsibility of debugger to set this back to HL_NOADDRESS
	 * after receiving the packet.
	 * Please note that we don't wait here because some implementations
	 * use hl_blockedPeek() function as a signal that we send a message.
	 */
	hl_write32(&__HOSTLINK__.hdr.target2host_addr, buf_addr);

	compiler_barrier();
}

/*
 * Wait for host response and return pointer to hostlink payload.
 * Symbol hl_blockedPeek() is used by the simulator as message signal.
 */
static void __noinline _hl_blockedPeek(void)
{
	while (hl_read32(&__HOSTLINK__.hdr.host2target_addr) == HL_NOADDRESS) {
		/* TODO: Timeout.  */
	}
}

static void hl_static_recv(void)
{
	compiler_barrier();
	_hl_blockedPeek();
	compiler_barrier();
}

/* Mark hostlink buffer as "No message here".  */
static void hl_delete(void)
{
	hl_write32(&__HOSTLINK__.hdr.target2host_addr, HL_NOADDRESS);
}

/* Parameter types.  */
#define PAT_CHAR	1
#define PAT_SHORT	2
#define PAT_INT		3
#define PAT_STRING	4
/* For future use.  */
#define PAT_INT64	5

static void hl_static_pack_int(volatile struct hl_packed_int *pack, int32_t value)
{
	hl_write16(&pack->type, PAT_INT);
	hl_write16(&pack->size, 4);
	hl_write32(&pack->value, value);
}

static void hl_static_pack_char(volatile struct hl_packed_short_buff *pack, unsigned char c)
{
	hl_write16(&pack->type, PAT_STRING);
	hl_write16(&pack->size, 1);
	hl_write8(&pack->payload_short, c);
}

static int hl_static_unpack_int(volatile struct hl_packed_int *pack, int32_t *value)
{
	uint16_t type = hl_read16(&pack->type);
	uint16_t size = hl_read16(&pack->size);

	if (type != PAT_INT) {
		return -1;
	}

	if (size != 4)  {
		return -1;
	}

	*value = hl_read32(&pack->value);

	return 0;
}

static inline int32_t hl_write_char(int fd, const char c)
{
	/*
	 * Format:
	 * in, int -> syscall (HL_SYSCALL_WRITE)
	 * in, int -> file descriptor
	 * in, ptr -> buffer
	 * in, int -> bytes number
	 * out, int -> bytes written
	 * out, int, host errno
	 */

	hl_static_pack_int(&__HOSTLINK__.payload.pkt_write_char_put.syscall_nr, HL_SYSCALL_WRITE);

	hl_static_pack_int(&__HOSTLINK__.payload.pkt_write_char_put.fd, fd);

	hl_static_pack_char(&__HOSTLINK__.payload.pkt_write_char_put.buff, c);

	hl_static_pack_int(&__HOSTLINK__.payload.pkt_write_char_put.nbyte, 1);

	hl_static_send(sizeof(struct hl_pkt_write_char_put));
	hl_static_recv();

	int32_t bwr = 0;
	int ret = hl_static_unpack_int(&__HOSTLINK__.payload.pkt_write_char_get.byte_written, &bwr);

	/* we can get host errno here with:
	 * hl_static_unpack_int(&__HOSTLINK__.pkt_write_char_get.host_errno, &host_errno);
	 * but we don't need it for UART emulation.
	 */

	if (bwr <= 0) {
		ret = -1;
	}

	hl_delete();

	return ret;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_hostlink_poll_in(const struct device *dev, unsigned char *c)
{
	ARG_UNUSED(dev);

	/* We plan to use hostlink for logging, so no much sense in poll_in implementation */
	return -1;
}

/**
 * @brief Output a character in polled mode.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_hostlink_poll_out(const struct device *dev, unsigned char c)
{
	ARG_UNUSED(dev);

	hl_write_char(1, c);
}

static const struct uart_driver_api uart_hostlink_driver_api = {
	.poll_in = uart_hostlink_poll_in,
	.poll_out = uart_hostlink_poll_out,
};

DEVICE_DT_DEFINE(DT_NODELABEL(hostlink), NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_SERIAL_INIT_PRIORITY, &uart_hostlink_driver_api);
