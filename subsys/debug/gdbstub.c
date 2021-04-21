/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <kernel.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(gdbstub);

#include <sys/util.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "gdbstub_backend.h"

#define GDB_PACKET_SIZE 256

/* GDB remote serial protocol does not define errors value properly
 * and handle all error packets as the same the code error is not
 * used. There are informal values used by others gdbstub
 * implementation, like qemu. Lets use the same here.
 */
#define GDB_ERROR_GENERAL   "E01"
#define GDB_ERROR_MEMORY    "E14"
#define GDB_ERROR_OVERFLOW  "E22"

/**
 * Add preamble and termination to the given data.
 *
 * It returns 0 if the packet was acknowledge, -1 otherwise.
 */
static int gdb_send_packet(const uint8_t *data, size_t len)
{
	uint8_t buf[2];
	uint8_t checksum = 0;

	/* Send packet start */
	z_gdb_putchar('$');

	/* Send packet data and calculate checksum */
	while (len-- > 0) {
		checksum += *data;
		z_gdb_putchar(*data++);
	}

	/* Send the checksum */
	z_gdb_putchar('#');

	if (bin2hex(&checksum, 1, buf, sizeof(buf)) == 0) {
		return -1;
	}

	z_gdb_putchar(buf[0]);
	z_gdb_putchar(buf[1]);

	if (z_gdb_getchar() == '+') {
		return 0;
	}

	/* Just got an invalid response */
	return -1;
}

/**
 * Receives a packet
 *
 * Return 0 in case of success, otherwise -1
 */
static int gdb_get_packet(uint8_t *buf, size_t buf_len, size_t *len)
{
	uint8_t ch = '0';
	uint8_t expected_checksum, checksum = 0;
	uint8_t checksum_buf[2];

	/* Wait for packet start */
	checksum = 0;

	/* wait for the start character, ignore the rest */
	while (ch != '$') {
		ch = z_gdb_getchar();
	}

	*len = 0;
	/* Read until receive # or the end of the buffer */
	while (*len < (buf_len - 1)) {
		ch = z_gdb_getchar();

		if (ch == '#') {
			break;
		}

		checksum += ch;
		buf[*len] = ch;
		(*len)++;
	}

	buf[*len] = '\0';

	/* Get checksum now */
	checksum_buf[0] = z_gdb_getchar();
	checksum_buf[1] = z_gdb_getchar();

	if (hex2bin(checksum_buf, 2, &expected_checksum, 1) == 0) {
		return -1;
	}

	/* Verify checksum */
	if (checksum != expected_checksum) {
		LOG_DBG("Bad checksum. Got 0x%x but was expecting: 0x%x",
			checksum, expected_checksum);
		/* NACK packet */
		z_gdb_putchar('-');
		return -1;
	}

	/* ACK packet */
	z_gdb_putchar('+');

	return 0;
}

/**
 * Read data from a given memory.
 *
 * Return 0 in case of success, otherwise -1
 */
static int gdb_mem_read(uint8_t *buf, size_t buf_len,
			uintptr_t addr, size_t len)
{
	uint8_t data;
	size_t pos, count = 0;

	if (len > buf_len) {
		return -1;
	}

	/* Read from system memory */
	for (pos = 0; pos < len; pos++) {
		data = *(uint8_t *)(addr + pos);
		count += bin2hex(&data, 1, buf + count, buf_len - count);
	}

	return count;
}

/**
 * Write data in a given memory.
 *
 * Return 0 in case of success, otherwise -1
 */
static int gdb_mem_write(const uint8_t *buf, uintptr_t addr,
			 size_t len)
{
	uint8_t data;

	while (len > 0) {
		size_t ret = hex2bin(buf, 2, &data, sizeof(data));

		if (ret == 0) {
			return -1;
		}

		*(uint8_t *)addr = data;

		addr++;
		buf += 2;
		len--;
	}

	return 0;
}

/**
 * Send a exception packet "T <value>"
 */
static int gdb_send_exception(uint8_t *buf, size_t len, uint8_t exception)
{
	size_t size;

	*buf = 'T';
	size = bin2hex(&exception, 1, buf + 1, len - 1);
	if (size == 0) {
		return -1;
	}

	/* Related to 'T' */
	size++;

	return gdb_send_packet(buf, size);
}

/**
 * Synchronously communicate with gdb on the host
 */
int z_gdb_main_loop(struct gdb_ctx *ctx, bool start)
{
	uint8_t buf[GDB_PACKET_SIZE];
	enum loop_state {
		RECEIVING,
		CONTINUE,
		FAILED
	} state;

	state = RECEIVING;

	if (start == false) {
		gdb_send_exception(buf, sizeof(buf), ctx->exception);
	}

#define CHECK_FAILURE(condition)		\
	{					\
		if ((condition)) {		\
			state = FAILED;	\
			break;			\
		}				\
	}

#define CHECK_SYMBOL(c)					\
	{							\
		CHECK_FAILURE(ptr == NULL || *ptr != (c));	\
		ptr++;						\
	}

#define CHECK_INT(arg)							\
	{								\
		arg = strtol((const char *)ptr, (char **)&ptr, 16);	\
		CHECK_FAILURE(ptr == NULL);				\
	}

	while (state == RECEIVING) {
		uint8_t *ptr;
		size_t data_len, pkt_len;
		uintptr_t addr;
		int ret;

		ret = gdb_get_packet(buf, sizeof(buf), &pkt_len);
		CHECK_FAILURE(ret == -1);

		if (pkt_len == 0) {
			continue;
		}

		ptr = buf;

		switch (*ptr++) {

		/**
		 * Read from the memory
		 * Format: m addr,length
		 */
		case 'm':
			CHECK_INT(addr);
			CHECK_SYMBOL(',');
			CHECK_INT(data_len);

			/* Read Memory */

			/*
			 * GDB ask the guest to read parameters when
			 * the user request backtrace. If the
			 * parameter is a NULL pointer this will cause
			 * a fault. Just send a packet informing that
			 * this address is invalid
			 */
			if (addr == 0L) {
				gdb_send_packet(GDB_ERROR_MEMORY, 3);
				break;
			}
			ret = gdb_mem_read(buf, sizeof(buf), addr, data_len);
			CHECK_FAILURE(ret == -1);
			gdb_send_packet(buf, ret);
			break;

		/**
		 * Write to memory
		 * Format: M addr,length:val
		 */
		case 'M':
			CHECK_INT(addr);
			CHECK_SYMBOL(',');
			CHECK_INT(data_len);
			CHECK_SYMBOL(':');

			if (addr == 0L) {
				gdb_send_packet(GDB_ERROR_MEMORY, 3);
				break;
			}

			/* Write Memory */
			pkt_len = gdb_mem_write(ptr, addr, data_len);
			CHECK_FAILURE(pkt_len == -1);
			gdb_send_packet("OK", 2);
			break;

		/*
		 * Continue ignoring the optional address
		 * Format: c addr
		 */
		case 'c':
			arch_gdb_continue();
			state = CONTINUE;
			break;

		/*
		 * Step one instruction ignoring the optional address
		 * s addr..addr
		 */
		case 's':
			arch_gdb_step();
			state = CONTINUE;
			break;

		/*
		 * Read all registers
		 * Format: g
		 */
		case 'g':
			pkt_len = bin2hex((const uint8_t *)&(ctx->registers),
				  sizeof(ctx->registers), buf, sizeof(buf));
			CHECK_FAILURE(pkt_len == 0);
			gdb_send_packet(buf, pkt_len);
			break;

		/**
		 * Write the value of the CPU registers
		 * Fromat: G XX...
		 */
		case 'G':
			pkt_len = hex2bin(ptr, pkt_len - 1,
					 (uint8_t *)&(ctx->registers),
					 sizeof(ctx->registers));
			CHECK_FAILURE(pkt_len == 0);
			gdb_send_packet("OK", 2);
			break;

		/**
		 * Read the value of a register
		 * Format: p n
		 */
		case 'p':
			CHECK_INT(addr);
			CHECK_FAILURE(addr >= ARCH_GDB_NUM_REGISTERS);

			/* Read Register */
			pkt_len = bin2hex(
				(const uint8_t *)&(ctx->registers[addr]),
				sizeof(ctx->registers[addr]),
				buf, sizeof(buf));
			CHECK_FAILURE(pkt_len == 0);
			gdb_send_packet(buf, pkt_len);
			break;

		/**
		 * Write data into a specific register
		 * Format: P register=value
		 */
		case 'P':
			CHECK_INT(addr);
			CHECK_SYMBOL('=');

			/*
			 * GDB requires orig_eax that seems to be
			 * Linux specific. Unfortunately if we just
			 * return "E01" gdb will stop.  So, we just
			 * send "OK" and ignore it.
			 */
			if (addr < ARCH_GDB_NUM_REGISTERS) {
				pkt_len = hex2bin(ptr, strlen(ptr),
					  (uint8_t *)&(ctx->registers[addr]),
					  sizeof(ctx->registers[addr]));
				CHECK_FAILURE(pkt_len == 0);
			}
			gdb_send_packet("OK", 2);
			break;


		/* What cause the pause  */
		case '?':
			gdb_send_exception(buf, sizeof(buf),
					   ctx->exception);
			break;

		/*
		 * Not supported action
		 */
		default:
			gdb_send_packet(NULL, 0);
			break;
		}
	}

	if (state == FAILED) {
		gdb_send_packet(GDB_ERROR_GENERAL, 3);
		return -1;
	}

#undef CHECK_FAILURE
#undef CHECK_INT
#undef CHECK_SYMBOL

	return 0;
}

int gdb_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	if (z_gdb_backend_init() == -1) {
		LOG_ERR("Could not initialize gdbstub backend.");
		return -1;
	}

	arch_gdb_init();
	return 0;
}

SYS_INIT(gdb_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
