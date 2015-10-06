/* byteorder.h - Byte order helpers */

/*
 * Copyright (c) 2015, Intel Corporation.
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

#define bswap_16(x) ((uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

#define bswap_32(x) ((uint32_t) ((((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) \
				| (((x) & 0xff00) << 8) | (((x) & 0xff) << 24)))

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
