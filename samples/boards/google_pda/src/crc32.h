/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CRC32_H_
#define _CRC32_H_

/**
 * @brief Initializes the crc32 module
 */
void crc32_init(void);

/**
 * @brief Creates a hash from the packet and stores the hash internally
 *
 * @param buf pointer to the packet to be hashed
 * @param size size of the packet in bytes
 */
void crc32_hash(const void *buf, int size);

/**
 * @brief Creates a hash of a 32 bit unsigned integer and stores the hash internally
 *
 * @param val value to be hashed
 */
void crc32_hash32(uint32_t val);

/**
 * @brief Creates a hash of a 16 bit unsigned integer and stores the hash internally
 *
 * @param val value to be hashed
 */
void crc32_hash16(uint16_t val);

/**
 * @brief retrieves the stored crc32 hash value
 *
 * @return crc32 hash value
 */
uint32_t crc32_result(void);

#endif /* _CRC32_H_ */
