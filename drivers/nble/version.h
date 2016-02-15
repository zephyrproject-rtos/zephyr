/*
 * Copyright (c) 2016 Intel Corporation
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

/**
 * Define the content of a 48-bytes binary version header.
 *
 * The binary version header allows to uniquely identify the binary used. Note:
 *  - a device may include more than one binary, each having its own binary
 *    version header.
 *  - the position of this struct is usually defined in the linker script and
 *    its content is overwritten after the build (in a special post-build script).
 * It therefore doesn't need to be initialized at compile-time (excepted magic
 * and version), yet can be used in the code at runtime.
 *
 * The binary version header is usually localized at the beginning of
 * the payload, but in the case of e.g. a bootloader, it can also be stored
 * at the end of the payload, or even anywhere within the payload.
 *
 * Major, Minor, Patch are the following the usual definition, e.g. 1.0.0
 */
struct version_header {
	/** Always equal to $B!N */
	uint8_t magic[4];

	/** Header format version */
	uint8_t version;
	uint8_t major;
	uint8_t minor;
	uint8_t patch;

	/**
	 * Human-friendly version string, free format (not NULL terminated)
	 * Advised format is: PPPPXXXXXX-YYWWTBBBB
	 *  - PPPP  : product code, e.g ATP1
	 *  - XXXXXX: binary info. Usually contains information such as the
	 *    binary type (bootloader, application), build variant (unit tests,
	 *    debug, release), release/branch name
	 *  - YY    : year last 2 digits
	 *  - WW    : work week number
	 *  - T     : build type, e.g. [W]eekly, [L]atest, [R]elease, [P]rod,
	 *    [F]actory, [C]ustom
	 *  - BBBB  : build number, left padded with zeros
	 * Examples:
	 *  - ATP1BOOT01-1503W0234
	 *  - CLRKAPP123-1502R0013
	 */
	char version_string[20];

	/**
	 * Micro-SHA1 (first 4 bytes of the SHA1) of the binary payload excl
	 * this header. It allows to uniquely identify the exact binary used.
	 * In the case the header is located in the middle of the payload, the
	 * SHA1 has to be computed from two disjoint buffers.
	 */
	uint8_t hash[4];

	/** Position of the payload start relative to the address of struct */
	int32_t offset;

	/** Filled with zeros, can be eventually used for 64 bits support */
	uint8_t reserved_1[4];

	/** Size of the payload in bytes, including this header */
	uint32_t size;

	/** Filled with zeros, can be eventually used for 64 bits support */
	uint8_t reserved_2[4];
} __packed;
