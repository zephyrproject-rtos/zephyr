/** @file
 *  @brief Byte order helpers.
 */

/*
 * Copyright (c) 2015-2016, Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

/** @def bswap_16
 *  @brief Change the endianness of a 16-bit integer.
 *
 *  @param x 16-bit integer.
 *
 *  @return 16-bit integer in swapped endianness.
 */
#define bswap_16(x) ((uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

/** @def bswap_32
 *  @brief Change the endianness of a 32-bit integer.
 *
 *  @param x 32-bit integer.
 *
 *  @return 32-bit integer in swapped endianness.
 */
#define bswap_32(x) ((uint32_t) ((((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) \
				| (((x) & 0xff00) << 8) | (((x) & 0xff) << 24)))

/** @def sys_le16_to_cpu
 *  @brief Convert 16-bit integer from little-endian to host endianness.
 *
 *  @param val 16-bit integer in little-endian format.
 *
 *  @return 16-bit integer in host endianness.
 */

/** @def sys_cpu_to_le16
 *  @brief Convert 16-bit integer from host endianness to little-endian.
 *
 *  @param val 16-bit integer in host endianness.
 *
 *  @return 16-bit integer in little-endian format.
 */

/** @def sys_be16_to_cpu
 *  @brief Convert 16-bit integer from big-endian to host endianness.
 *
 *  @param val 16-bit integer in big-endian format.
 *
 *  @return 16-bit integer in host endianness.
 */

/** @def sys_cpu_to_be16
 *  @brief Convert 16-bit integer from host endianness to big-endian.
 *
 *  @param val 16-bit integer in host endianness.
 *
 *  @return 16-bit integer in big-endian format.
 */

/** @def sys_le32_to_cpu
 *  @brief Convert 32-bit integer from little-endian to host endianness.
 *
 *  @param val 32-bit integer in little-endian format.
 *
 *  @return 32-bit integer in host endianness.
 */

/** @def sys_cpu_to_le32
 *  @brief Convert 32-bit integer from host endianness to little-endian.
 *
 *  @param val 32-bit integer in host endianness.
 *
 *  @return 32-bit integer in little-endian format.
 */

/** @def sys_be32_to_cpu
 *  @brief Convert 32-bit integer from big-endian to host endianness.
 *
 *  @param val 32-bit integer in big-endian format.
 *
 *  @return 32-bit integer in host endianness.
 */

/** @def sys_cpu_to_be32
 *  @brief Convert 32-bit integer from host endianness to big-endian.
 *
 *  @param val 32-bit integer in host endianness.
 *
 *  @return 32-bit integer in big-endian format.
 */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define sys_le16_to_cpu(val) (val)
#define sys_cpu_to_le16(val) (val)
#define sys_be16_to_cpu(val) bswap_16(val)
#define sys_cpu_to_be16(val) bswap_16(val)
#define sys_le32_to_cpu(val) (val)
#define sys_cpu_to_le32(val) (val)
#define sys_be32_to_cpu(val) bswap_32(val)
#define sys_cpu_to_be32(val) bswap_32(val)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define sys_le16_to_cpu(val) bswap_16(val)
#define sys_cpu_to_le16(val) bswap_16(val)
#define sys_be16_to_cpu(val) (val)
#define sys_cpu_to_be16(val) (val)
#define sys_le32_to_cpu(val) bswap_32(val)
#define sys_cpu_to_le32(val) bswap_32(val)
#define sys_be32_to_cpu(val) (val)
#define sys_cpu_to_be32(val) (val)
#else
#error "Unknown byte order"
#endif
