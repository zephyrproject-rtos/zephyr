/*
 * Copyright (c) 2025 Texas Instruments Incorporated.
 *
 * TI Secureproxy Mailbox header file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_TI_SECURE_PROXY_H_
#define ZEPHYR_INCLUDE_DRIVERS_TI_SECURE_PROXY_H_

#include <stddef.h>
#include <stdint.h>
#define MAILBOX_MBOX_SIZE 60

/**
 * @struct rx_msg
 * @brief Received message details
 * @param seq: Message sequence number
 * @param size: Message size in bytes
 * @param buf: Buffer for message data
 */
struct rx_msg {
	uint8_t seq;
	size_t size;
	uint8_t buf[MAILBOX_MBOX_SIZE];
};

#endif
