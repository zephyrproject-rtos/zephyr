/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "server_internal.h"

#include <string.h>
#include <strings.h>

#include <zephyr/net/http/service.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/eventfd.h>
#include <zephyr/ztest.h>

#define BUFFER_SIZE                    1024
#define SERVER_IPV4_ADDR               "127.0.0.1"
#define SERVER_PORT                    8080
#define TIMEOUT_S                      1

#define UPGRADE_STREAM_ID              1
#define TEST_STREAM_ID_1               3
#define TEST_STREAM_ID_2               5

#define TEST_DYNAMIC_POST_PAYLOAD "Test dynamic POST"
#define TEST_DYNAMIC_GET_PAYLOAD "Test dynamic GET"
#define TEST_STATIC_PAYLOAD "Hello, World!"
#define TEST_STATIC_FS_PAYLOAD "Hello, World from static file!"

/* Random base64 encoded data */
#define TEST_LONG_PAYLOAD_CHUNK_1                                                                  \
	"Z3479c2x8gXgzvDpvt4YuQePsvmsur1J1U+lLKzkyGCQgtWEysRjnO63iZvN/Zaag5YlliAkcaWi"             \
	"Alb8zI4SxK+JB3kfpkcAA6c8m2PfkP6D5+Vrcy9O6ituR8gb0tm8o9CwTeUhf8H6q2kB5BO1ZZxm"             \
	"G9c3VO9BLLTC8LMG8isyzB1wT+EB8YTv4YaNc9mXJmXNt3pycZ4Thg20rPfhZsvleIeUYZZQJArx"             \
	"ufSBYR4v6mAEm/qdFqIwe9k6dtJEfR5guFoAWbR4jMrJreshyvByrZSy+aP1S93Fvob9hNn6ouSc"
#define TEST_LONG_PAYLOAD_CHUNK_2                                                                  \
	"a0UIx0JKhFKvnM23kcavlMzwD+MerSiPUDYKSjtnjhhZmW3GonTpUWMEuDGZNkbrAZ3fbuWRbHi0"             \
	"1GufXYWGw/Jk6H6GV5WWWF9a71dng6gsH21zD1dqYIo46hofi4mfJ8Spo9a4Ch04ARNFSMhuLwYv"             \
	"eOprXUybMUiBVlTansXL2mdH2BgCPu4u65kIyAxcQpiXNGSJ3EjEIGIa"

static const char long_payload[] = TEST_LONG_PAYLOAD_CHUNK_1 TEST_LONG_PAYLOAD_CHUNK_2;
BUILD_ASSERT(sizeof(long_payload) - 1 > CONFIG_HTTP_SERVER_CLIENT_BUFFER_SIZE,
	     "long_payload should be longer than client buffer to test payload being sent to "
	     "application across multiple calls to dynamic resource callback");

/* Individual HTTP2 frames, used to compose requests.
 *
 * Headers and data frames can be composed based on a "real" request by copying the frame from a
 * wireshark capture (Copy --> ...as a hex stream) and formatting into a C array initializer using
 * xxd:
 *
 *   echo "<frame_as_hex_stream>" | xxd -r -p | xxd -i
 *
 * For example:
 *   $ echo "01234567" | xxd -r -p | xxd -i
 *     0x01, 0x23, 0x45, 0x67
 */
#define TEST_HTTP2_MAGIC \
	0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, 0x48, 0x54, 0x54, 0x50, 0x2f, 0x32, \
	0x2e, 0x30, 0x0d, 0x0a, 0x0d, 0x0a, 0x53, 0x4d, 0x0d, 0x0a, 0x0d, 0x0a
#define TEST_HTTP2_SETTINGS \
	0x00, 0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00,	0x00, \
	0x00, 0x03, 0x00, 0x00, 0x00, 0x64, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff
#define TEST_HTTP2_SETTINGS_ACK \
	0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_GOAWAY \
	0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_HEADERS_GET_ROOT_STREAM_1 \
	0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x82, 0x84, 0x86, 0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, \
	0x78, 0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa, \
	0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83
#define TEST_HTTP2_HEADERS_GET_INDEX_STREAM_2 \
	0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_2, \
	0x82, 0x85, 0x86, 0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, \
	0x78, 0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa, \
	0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83
#define TEST_HTTP2_HEADERS_GET_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x2b, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x82, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa
#define TEST_HTTP2_HEADERS_GET_DYNAMIC_STREAM_1_PADDED \
	0x00, 0x00, 0x3d, 0x01, 0x0d, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, 0x11,\
	0x82, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_HEADERS_GET_HEADER_CAPTURE1_STREAM_1 \
	0x00, 0x00, 0x39, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x82, 0x04, 0x8b, 0x62, 0x72, 0x8e, 0x42, 0xd9, 0x11, 0x07, 0x5a, 0x6d, \
	0xb0, 0xbf, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, \
	0x7a, 0x88, 0x25, 0xb6, 0x50, 0xc3, 0xab, 0xbc, 0x15, 0xc1, 0x53, 0x03, \
	0x2a, 0x2f, 0x2a, 0x40, 0x88, 0x49, 0x50, 0x95, 0xa7, 0x28, 0xe4, 0x2d, \
	0x9f, 0x87, 0x49, 0x50, 0x98, 0xbb, 0x8e, 0x8b, 0x4b
#define TEST_HTTP2_HEADERS_GET_HEADER_CAPTURE2_STREAM_1 \
	0x00, 0x00, 0x5a, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x82, 0x04, 0x8b, 0x62, 0x72, 0x8e, 0x42, 0xd9, 0x11, 0x07, 0x5a, 0x6d, \
	0xb0, 0xbf, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, \
	0x7a, 0xa9, 0x18, 0xc6, 0x31, 0x8c, 0x63, 0x18, 0xc6, 0x31, 0x8c, 0x63, \
	0x18, 0xc6, 0x31, 0x8c, 0x63, 0x18, 0xc6, 0x31, 0x8c, 0x63, 0x18, 0xc6, \
	0x31, 0x8c, 0x63, 0x18, 0xc6, 0x31, 0x8c, 0x63, 0x18, 0xc6, 0x31, 0x8c, \
	0x63, 0x18, 0xc6, 0x31, 0x8c, 0x63, 0x1f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, \
	0x40, 0x88, 0x49, 0x50, 0x95, 0xa7, 0x28, 0xe4, 0x2d, 0x9f, 0x87, 0x49, \
	0x50, 0x98, 0xbb, 0x8e, 0x8b, 0x4b
#define TEST_HTTP2_HEADERS_GET_HEADER_CAPTURE3_STREAM_1 \
	0x00, 0x00, 0x4c, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x82, 0x04, 0x8b, 0x62, 0x72, 0x8e, 0x42, 0xd9, 0x11, 0x07, 0x5a, 0x6d, \
	0xb0, 0xbf, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, \
	0x7a, 0x88, 0x25, 0xb6, 0x50, 0xc3, 0xab, 0xbc, 0x15, 0xc1, 0x53, 0x03, \
	0x2a, 0x2f, 0x2a, 0x40, 0x88, 0x49, 0x50, 0x95, 0xa7, 0x28, 0xe4, 0x2d, \
	0x9f, 0x87, 0x49, 0x50, 0x98, 0xbb, 0x8e, 0x8b, 0x4b, 0x40, 0x88, 0x49, \
	0x50, 0x95, 0xa7, 0x28, 0xe4, 0x2d, 0x82, 0x88, 0x49, 0x50, 0x98, 0xbb, \
	0x8e, 0x8b, 0x4a, 0x2f
#define TEST_HTTP2_HEADERS_POST_HEADER_CAPTURE_WITH_TESTHEADER_STREAM_1 \
	0x00, 0x00, 0x4b, 0x01, 0x04, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x83, 0x04, 0x8b, 0x62, 0x72, 0x8e, 0x42, 0xd9, 0x11, 0x07, 0x5a, 0x6d, \
	0xb0, 0xbf, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, \
	0x7a, 0x88, 0x25, 0xb6, 0x50, 0xc3, 0xab, 0xbc, 0x15, 0xc1, 0x53, 0x03, \
	0x2a, 0x2f, 0x2a, 0x40, 0x88, 0x49, 0x50, 0x95, 0xa7, 0x28, 0xe4, 0x2d, \
	0x9f, 0x87, 0x49, 0x50, 0x98, 0xbb, 0x8e, 0x8b, 0x4b, 0x5f, 0x8b, 0x1d, \
	0x75, 0xd0, 0x62, 0x0d, 0x26, 0x3d, 0x4c, 0x74, 0x41, 0xea, 0x0f, 0x0d, \
	0x02, 0x31, 0x30
#define TEST_HTTP2_HEADERS_POST_HEADER_CAPTURE2_NO_TESTHEADER_STREAM_2 \
	0x00, 0x00, 0x39, 0x01, 0x04, 0x00, 0x00, 0x00, TEST_STREAM_ID_2, \
	0x83, 0x04, 0x8b, 0x62, 0x72, 0x8e, 0x42, 0xd9, 0x11, 0x07, 0x5a, 0x6d, \
	0xb0, 0xa2, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, \
	0x7a, 0x88, 0x25, 0xb6, 0x50, 0xc3, 0xab, 0xbc, 0x15, 0xc1, 0x53, 0x03, \
	0x2a, 0x2f, 0x2a, 0x5f, 0x8b, 0x1d, 0x75, 0xd0, 0x62, 0x0d, 0x26, 0x3d, \
	0x4c, 0x74, 0x41, 0xea, 0x0f, 0x0d, 0x02, 0x31, 0x30
#define TEST_HTTP2_HEADERS_GET_RESPONSE_HEADERS_STREAM_1 \
	0x00, 0x00, 0x28, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x82, 0x04, 0x8c, 0x62, 0xc2, 0xa2, 0xb3, 0xd4, 0x82, 0xc5, 0x39, 0x47, \
	0x21, 0x6c, 0x47, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, \
	0xff, 0x7a, 0x88, 0x25, 0xb6, 0x50, 0xc3, 0xab, 0xbc, 0x15, 0xc1, 0x53, \
	0x03, 0x2a, 0x2f, 0x2a
#define TEST_HTTP2_HEADERS_POST_RESPONSE_HEADERS_STREAM_1 \
	0x00, 0x00, 0x28, 0x01, 0x04, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x83, 0x04, 0x8c, 0x62, 0xc2, 0xa2, 0xb3, 0xd4, 0x82, 0xc5, 0x39, 0x47, \
	0x21, 0x6c, 0x47, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, \
	0xff, 0x7a, 0x88, 0x25, 0xb6, 0x50, 0xc3, 0xab, 0xbc, 0x15, 0xc1, 0x53, \
	0x03, 0x2a, 0x2f, 0x2a
#define TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x30, 0x01, 0x04, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x83, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, 0x0f, 0x0d, 0x02, 0x31, 0x37
#define TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1_PRIORITY \
	0x00, 0x00, 0x35, 0x01, 0x24, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x00, 0x00, 0x00, 0x00, 0x64, \
	0x83, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, 0x0f, 0x0d, 0x02, 0x31, 0x37
#define TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1_PRIORITY_PADDED \
	0x00, 0x00, 0x40, 0x01, 0x2c, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x0a, 0x00, 0x00, 0x00, 0x00, 0xc8,\
	0x83, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, 0x0f, 0x0d, 0x02, 0x31, 0x37, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_PARTIAL_HEADERS_POST_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x83, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a
#define TEST_HTTP2_CONTINUATION_POST_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x10, 0x09, 0x04, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x2f, 0x2a, 0x5f, 0x87, 0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, 0x0f, \
	0x0d, 0x02, 0x31, 0x37
#define TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x11, 0x00, 0x01, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x54, 0x65, 0x73, 0x74, 0x20, 0x64, 0x79, 0x6e, 0x61, 0x6d, 0x69, 0x63, \
	0x20, 0x50, 0x4f, 0x53, 0x54
#define TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_PADDED \
	0x00, 0x00, 0x34, 0x00, 0x09, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, 0x22, \
	0x54, 0x65, 0x73, 0x74, 0x20, 0x64, 0x79, 0x6e, 0x61, 0x6d, 0x69, 0x63, \
	0x20, 0x50, 0x4f, 0x53, 0x54, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_NO_END_STREAM \
	0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x54, 0x65, 0x73, 0x74, 0x20, 0x64, 0x79, 0x6e, 0x61, 0x6d, 0x69, 0x63, \
	0x20, 0x50, 0x4f, 0x53, 0x54
#define TEST_HTTP2_DATA_POST_HEADER_CAPTURE_STREAM_1                                               \
	0x00, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x7b, 0x22, 0x74, 0x65, 0x73, 0x74, 0x22, 0x3a, 0x31, 0x7d
#define TEST_HTTP2_DATA_POST_HEADER_CAPTURE_STREAM_2                                               \
	0x00, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, TEST_STREAM_ID_2, \
	0x7b, 0x22, 0x74, 0x65,	0x73, 0x74, 0x22, 0x3a, 0x31, 0x7d
#define TEST_HTTP2_TRAILING_HEADER_STREAM_1 \
	0x00, 0x00, 0x0c, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x40, 0x84, 0x92, 0xda, 0x69, 0xf5, 0x85, 0x9c, 0xa3, 0x90, 0xb6, 0x7f
#define TEST_HTTP2_RST_STREAM_STREAM_1 \
	0x00, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0xaa, 0xaa, 0xaa, 0xaa
#define TEST_HTTP2_HEADERS_PUT_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x34, 0x01, 0x04, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x42, 0x03, 0x50, 0x55, 0x54, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, \
	0x89, 0x70, 0xff, 0x04, 0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, \
	0x88, 0x25, 0xb6, 0x50, 0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, \
	0x2f, 0x2a, 0x5f, 0x87, 0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, 0x0f, \
	0x0d, 0x02, 0x31, 0x37
#define TEST_HTTP2_HEADERS_PATCH_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x36, 0x01, 0x04, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x42, 0x05, 0x50, 0x41, 0x54, 0x43, 0x48, 0x86, 0x41, 0x87, 0x0b, 0xe2, \
	0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, 0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, \
	0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, 0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, \
	0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, 0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, \
	0xaa, 0x0f, 0x0d, 0x02, 0x31, 0x37
#define TEST_HTTP2_DATA_PUT_DYNAMIC_STREAM_1 TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1
#define TEST_HTTP2_DATA_PATCH_DYNAMIC_STREAM_1 TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1
#define TEST_HTTP2_HEADERS_DELETE_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x32, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x42, 0x06, 0x44, 0x45, 0x4c, 0x45, 0x54, 0x45, 0x86, 0x41, 0x87, 0x0b, \
	0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, 0x86, 0x62, 0x4f, 0x55, 0x0e, \
	0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, 0xc3, 0xcb, 0xbc, 0xb8, 0x3f, \
	0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, 0x49, 0x7c, 0xa5, 0x8a, 0xe8, \
	0x19, 0xaa
#define TEST_HTTP2_HEADERS_POST_ROOT_STREAM_1 \
	0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x83, 0x84, 0x86, 0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, \
	0x78, 0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa, \
	0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83
#define TEST_HTTP2_DATA_POST_ROOT_STREAM_1 TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1

static uint16_t test_http_service_port = SERVER_PORT;
HTTP_SERVICE_DEFINE(test_http_service, SERVER_IPV4_ADDR,
		    &test_http_service_port, 1, 10, NULL, NULL);

static const char static_resource_payload[] = TEST_STATIC_PAYLOAD;
struct http_resource_detail_static static_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.static_data = static_resource_payload,
	.static_data_len = sizeof(static_resource_payload) - 1,
};

HTTP_RESOURCE_DEFINE(static_resource, test_http_service, "/",
		     &static_resource_detail);

static uint8_t dynamic_payload[32];
static size_t dynamic_payload_len = sizeof(dynamic_payload);
static bool dynamic_error;

static int dynamic_cb(struct http_client_ctx *client, enum http_data_status status,
		      const struct http_request_ctx *request_ctx,
		      struct http_response_ctx *response_ctx, void *user_data)
{
	static size_t offset;

	if (status == HTTP_SERVER_DATA_ABORTED) {
		offset = 0;
		return 0;
	}

	if (dynamic_error) {
		return -ENOMEM;
	}

	switch (client->method) {
	case HTTP_GET:
		response_ctx->body = dynamic_payload;
		response_ctx->body_len = dynamic_payload_len;
		response_ctx->final_chunk = true;
		break;
	case HTTP_DELETE:
		response_ctx->body = NULL;
		response_ctx->body_len = 0;
		response_ctx->final_chunk = true;
		break;
	case HTTP_POST:
	case HTTP_PUT:
	case HTTP_PATCH:
		if (request_ctx->data_len + offset > sizeof(dynamic_payload)) {
			return -ENOMEM;
		}

		if (request_ctx->data_len > 0) {
			memcpy(dynamic_payload + offset, request_ctx->data, request_ctx->data_len);
			offset += request_ctx->data_len;
		}

		if (status == HTTP_SERVER_DATA_FINAL) {
			/* All data received, reset progress. */
			dynamic_payload_len = offset;
			offset = 0;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

struct http_resource_detail_dynamic dynamic_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods =
			BIT(HTTP_GET) | BIT(HTTP_DELETE) | BIT(HTTP_POST) |
			BIT(HTTP_PUT) | BIT(HTTP_PATCH),
		.content_type = "text/plain",
	},
	.cb = dynamic_cb,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(dynamic_resource, test_http_service, "/dynamic",
		     &dynamic_detail);

struct test_headers_clone {
	uint8_t buffer[CONFIG_HTTP_SERVER_CAPTURE_HEADER_BUFFER_SIZE];
	struct http_header headers[CONFIG_HTTP_SERVER_CAPTURE_HEADER_COUNT];
	size_t count;
	enum http_header_status status;
};

static int dynamic_request_headers_cb(struct http_client_ctx *client, enum http_data_status status,
				      const struct http_request_ctx *request_ctx,
				      struct http_response_ctx *response_ctx, void *user_data)
{
	ptrdiff_t offset;
	struct http_header *hdrs_src;
	struct http_header *hdrs_dst;
	struct test_headers_clone *clone = (struct test_headers_clone *)user_data;

	if (request_ctx->header_count != 0) {
		/* Copy the captured header info to static buffer for later assertions in testcase.
		 * Don't assume that the buffer inside client context remains valid after return
		 * from the callback. Also need to update pointers within structure with an offset
		 * to point at new buffer.
		 */
		memcpy(clone->buffer, &client->header_capture_ctx, sizeof(clone->buffer));

		clone->count = request_ctx->header_count;
		clone->status = request_ctx->headers_status;

		hdrs_src = request_ctx->headers;
		hdrs_dst = clone->headers;
		offset = clone->buffer - client->header_capture_ctx.buffer;

		for (int i = 0; i < request_ctx->header_count; i++) {
			if (hdrs_src[i].name != NULL) {
				hdrs_dst[i].name = hdrs_src[i].name + offset;
			}

			if (hdrs_src[i].value != NULL) {
				hdrs_dst[i].value = hdrs_src[i].value + offset;
			}
		}
	}

	return 0;
}

/* Define two resources for testing header capture, so that we can check concurrent streams */
static struct test_headers_clone request_headers_clone;
static struct test_headers_clone request_headers_clone2;

struct http_resource_detail_dynamic dynamic_request_headers_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
			.content_type = "text/plain",
		},
	.cb = dynamic_request_headers_cb,
	.user_data = &request_headers_clone,
};

struct http_resource_detail_dynamic dynamic_request_headers_detail2 = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
			.content_type = "text/plain",
		},
	.cb = dynamic_request_headers_cb,
	.user_data = &request_headers_clone2,
};

HTTP_RESOURCE_DEFINE(dynamic_request_headers_resource, test_http_service, "/header_capture",
		     &dynamic_request_headers_detail);

HTTP_RESOURCE_DEFINE(dynamic_request_headers_resource2, test_http_service, "/header_capture2",
		     &dynamic_request_headers_detail2);

HTTP_SERVER_REGISTER_HEADER_CAPTURE(capture_user_agent, "User-Agent");
HTTP_SERVER_REGISTER_HEADER_CAPTURE(capture_test_header, "Test-Header");
HTTP_SERVER_REGISTER_HEADER_CAPTURE(capture_test_header2, "Test-Header2");

enum dynamic_response_headers_variant {
	/* No application defined response code, headers or data */
	DYNAMIC_RESPONSE_HEADERS_VARIANT_NONE,

	/* Send a 422 response code */
	DYNAMIC_RESPONSE_HEADERS_VARIANT_422,

	/* Send an extra header on top of server defaults */
	DYNAMIC_RESPONSE_HEADERS_VARIANT_EXTRA_HEADER,

	/* Override the default Content-Type header */
	DYNAMIC_RESPONSE_HEADERS_VARIANT_OVERRIDE_HEADER,

	/* Send body data combined with header data in a single callback */
	DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_COMBINED,

	/* Send body data in a separate callback to header data */
	DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_SEPARATE,

	/* Long body data split across multiple callbacks */
	DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_LONG,
};

static uint8_t dynamic_response_headers_variant;
static uint8_t dynamic_response_headers_buffer[sizeof(long_payload)];

static int dynamic_response_headers_cb(struct http_client_ctx *client, enum http_data_status status,
				       const struct http_request_ctx *request_ctx,
				       struct http_response_ctx *response_ctx, void *user_data)
{
	static bool request_continuation;
	static size_t offset;

	static const struct http_header extra_headers[] = {
		{.name = "Test-Header", .value = "test_data"},
	};

	static const struct http_header override_headers[] = {
		{.name = "Content-Type", .value = "application/json"},
	};

	if (status != HTTP_SERVER_DATA_FINAL &&
	    dynamic_response_headers_variant != DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_LONG) {
		/* Long body variant is the only one which needs to take some action before final
		 * data has been received from server
		 */
		return 0;
	}

	switch (dynamic_response_headers_variant) {
	case DYNAMIC_RESPONSE_HEADERS_VARIANT_NONE:
		break;

	case DYNAMIC_RESPONSE_HEADERS_VARIANT_422:
		response_ctx->status = 422;
		response_ctx->final_chunk = true;
		break;

	case DYNAMIC_RESPONSE_HEADERS_VARIANT_EXTRA_HEADER:
		response_ctx->headers = extra_headers;
		response_ctx->header_count = ARRAY_SIZE(extra_headers);
		response_ctx->final_chunk = true;
		break;

	case DYNAMIC_RESPONSE_HEADERS_VARIANT_OVERRIDE_HEADER:
		response_ctx->headers = override_headers;
		response_ctx->header_count = ARRAY_SIZE(extra_headers);
		response_ctx->final_chunk = true;
		break;

	case DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_SEPARATE:
		if (!request_continuation) {
			/* Send headers in first callback */
			response_ctx->headers = extra_headers;
			response_ctx->header_count = ARRAY_SIZE(extra_headers);
			request_continuation = true;
		} else {
			/* Send body in subsequent callback */
			response_ctx->body = TEST_DYNAMIC_GET_PAYLOAD;
			response_ctx->body_len = strlen(response_ctx->body);
			response_ctx->final_chunk = true;
			request_continuation = false;
		}
		break;

	case DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_COMBINED:
		response_ctx->headers = extra_headers;
		response_ctx->header_count = ARRAY_SIZE(extra_headers);
		response_ctx->body = TEST_DYNAMIC_GET_PAYLOAD;
		response_ctx->body_len = strlen(response_ctx->body);
		response_ctx->final_chunk = true;
		break;

	case DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_LONG:
		if (client->method == HTTP_GET) {
			/* Send GET payload split across multiple callbacks */
			size_t send_len = (offset == 0) ? strlen(TEST_LONG_PAYLOAD_CHUNK_1)
							: strlen(TEST_LONG_PAYLOAD_CHUNK_2);

			response_ctx->body = long_payload + offset;
			response_ctx->body_len = send_len;
			offset += send_len;

			if (offset == strlen(long_payload)) {
				offset = 0;
				response_ctx->final_chunk = true;
			}
		} else if (client->method == HTTP_POST) {
			/* Copy POST payload into buffer for later comparison */
			zassert(offset + request_ctx->data_len <=
					sizeof(dynamic_response_headers_buffer),
				"POST data too long for buffer");
			memcpy(dynamic_response_headers_buffer + offset, request_ctx->data,
			       request_ctx->data_len);
			offset += request_ctx->data_len;

			if (status == HTTP_SERVER_DATA_FINAL) {
				offset = 0;
			}
		} else {
			zassert(false, "unexpected HTTP method");
		}
		break;
	}

	return 0;
}

struct http_resource_detail_dynamic dynamic_response_headers_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
		.content_type = "text/plain",
	},
	.cb = dynamic_response_headers_cb,
	.user_data = NULL
};

HTTP_RESOURCE_DEFINE(dynamic_response_headers_resource, test_http_service, "/response_headers",
		     &dynamic_response_headers_detail);

static int client_fd = -1;
static uint8_t buf[BUFFER_SIZE];

/* This function ensures that there's at least as much data as requested in
 * the buffer.
 */
static void test_read_data(size_t *offset, size_t need)
{
	int ret;

	while (*offset < need) {
		ret = zsock_recv(client_fd, buf + *offset, sizeof(buf) - *offset, 0);
		zassert_not_equal(ret, -1, "recv() failed (%d)", errno);
		*offset += ret;
		if (ret == 0) {
			break;
		}
	};

	zassert_true(*offset >= need, "Not all requested data received");
}

/* This function moves the remaining data in the buffer to the beginning. */
static void test_consume_data(size_t *offset, size_t consume)
{
	zassert_true(*offset >= consume, "Cannot consume more data than received");
	*offset -= consume;
	memmove(buf, buf + consume, *offset);
}

static void expect_http1_switching_protocols(size_t *offset)
{
	static const char switching_protocols[] =
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Connection: Upgrade\r\n"
		"Upgrade: h2c\r\n"
		"\r\n";

	test_read_data(offset, sizeof(switching_protocols) - 1);
	zassert_mem_equal(buf, switching_protocols, sizeof(switching_protocols) - 1,
			  "Received data doesn't match expected response");
	test_consume_data(offset, sizeof(switching_protocols) - 1);
}

static void test_get_frame_header(size_t *offset, struct http2_frame *frame)
{
	test_read_data(offset, HTTP2_FRAME_HEADER_SIZE);

	frame->length = sys_get_be24(&buf[HTTP2_FRAME_LENGTH_OFFSET]);
	frame->type = buf[HTTP2_FRAME_TYPE_OFFSET];
	frame->flags = buf[HTTP2_FRAME_FLAGS_OFFSET];
	frame->stream_identifier = sys_get_be32(
				&buf[HTTP2_FRAME_STREAM_ID_OFFSET]);
	frame->stream_identifier &= HTTP2_FRAME_STREAM_ID_MASK;

	test_consume_data(offset, HTTP2_FRAME_HEADER_SIZE);
}

static void expect_http2_settings_frame(size_t *offset, bool ack)
{
	struct http2_frame frame;

	test_get_frame_header(offset, &frame);

	zassert_equal(frame.type, HTTP2_SETTINGS_FRAME, "Expected settings frame");
	zassert_equal(frame.stream_identifier, 0, "Settings frame stream ID must be 0");

	if (ack) {
		zassert_equal(frame.length, 0, "Invalid settings frame length");
		zassert_equal(frame.flags, HTTP2_FLAG_SETTINGS_ACK,
			      "Expected settings ACK flag");
	} else {
		zassert_equal(frame.length % sizeof(struct http2_settings_field), 0,
			      "Invalid settings frame length");
		zassert_equal(frame.flags, 0, "Expected no settings flags");

		/* Consume settings payload */
		test_read_data(offset, frame.length);
		test_consume_data(offset, frame.length);
	}
}

static void expect_contains_header(const uint8_t *buffer, size_t len,
				   const struct http_header *header)
{
	int ret;
	bool found = false;
	struct http_hpack_header_buf header_buf;
	size_t consumed = 0;

	while (consumed < len) {
		ret = http_hpack_decode_header(buffer + consumed, len, &header_buf);
		zassert_true(ret >= 0, "Failed to decode header");
		zassert_true(consumed + ret <= len, "Frame length exceeded");

		if (strncasecmp(header_buf.name, header->name, header_buf.name_len) == 0 &&
		    strncasecmp(header_buf.value, header->value, header_buf.value_len) == 0) {
			found = true;
			break;
		}

		consumed += ret;
	}

	zassert_true(found, "Header '%s: %s' not found", header->name, header->value);
}

static void expect_http2_headers_frame(size_t *offset, int stream_id, uint8_t flags,
				       const struct http_header *headers, size_t headers_count)
{
	struct http2_frame frame;

	test_get_frame_header(offset, &frame);

	zassert_equal(frame.type, HTTP2_HEADERS_FRAME, "Expected headers frame, got frame type %u",
		      frame.type);
	zassert_equal(frame.stream_identifier, stream_id,
		      "Invalid headers frame stream ID");
	zassert_equal(frame.flags, flags, "Unexpected flags received (expected %x got %x)", flags,
		      frame.flags);

	/* Consume headers payload */
	test_read_data(offset, frame.length);

	for (size_t i = 0; i < headers_count; i++) {
		expect_contains_header(buf, frame.length, &headers[i]);
	}

	test_consume_data(offset, frame.length);
}

/* "payload" may be NULL to skip data frame content validation. */
static void expect_http2_data_frame(size_t *offset, int stream_id,
				    const uint8_t *payload, size_t payload_len,
				    uint8_t flags)
{
	struct http2_frame frame;

	test_get_frame_header(offset, &frame);

	zassert_equal(frame.type, HTTP2_DATA_FRAME, "Expected data frame");
	zassert_equal(frame.stream_identifier, stream_id,
		      "Invalid data frame stream ID");
	zassert_equal(frame.flags, flags, "Unexpected flags received");
	if (payload != NULL) {
		zassert_equal(frame.length, payload_len,
			      "Unexpected data frame length");
	}

	/* Verify data payload */
	test_read_data(offset, frame.length);
	if (payload != NULL) {
		zassert_mem_equal(buf, payload, payload_len,
				  "Unexpected data payload");
	}
	test_consume_data(offset, frame.length);
}

static void expect_http2_window_update_frame(size_t *offset, int stream_id)
{
	struct http2_frame frame;

	test_get_frame_header(offset, &frame);

	zassert_equal(frame.type, HTTP2_WINDOW_UPDATE_FRAME,
		      "Expected window update frame");
	zassert_equal(frame.stream_identifier, stream_id,
		      "Invalid window update frame stream ID (expected %d got %d)", stream_id,
		      frame.stream_identifier);
	zassert_equal(frame.flags, 0, "Unexpected flags received");
	zassert_equal(frame.length, sizeof(uint32_t),
		      "Unexpected window update frame length");


	/* Consume window update payload */
	test_read_data(offset, frame.length);
	test_consume_data(offset, frame.length);
}

ZTEST(server_function_tests, test_http2_get_concurrent_streams)
{
	static const uint8_t request_get_2_streams[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_ROOT_STREAM_1,
		TEST_HTTP2_HEADERS_GET_INDEX_STREAM_2,
		TEST_HTTP2_GOAWAY,
	};
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request_get_2_streams,
			 sizeof(request_get_2_streams), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Settings frame is expected twice (server settings + settings ACK) */
	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1, HTTP2_FLAG_END_HEADERS, NULL, 0);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, TEST_STATIC_PAYLOAD,
				strlen(TEST_STATIC_PAYLOAD),
				HTTP2_FLAG_END_STREAM);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_2, HTTP2_FLAG_END_HEADERS, NULL, 0);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_2, NULL, 0,
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http2_static_get)
{
	static const uint8_t request_get_static_simple[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_ROOT_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request_get_static_simple,
			 sizeof(request_get_static_simple), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1, HTTP2_FLAG_END_HEADERS, NULL, 0);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, TEST_STATIC_PAYLOAD,
				strlen(TEST_STATIC_PAYLOAD),
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http1_static_upgrade_get)
{
	static const char http1_request[] =
		"GET / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"Connection: Upgrade, HTTP2-Settings\r\n"
		"Upgrade: h2c\r\n"
		"HTTP2-Settings: AAMAAABkAAQAoAAAAAIAAAAA\r\n"
		"\r\n";
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Verify HTTP1 switching protocols response. */
	expect_http1_switching_protocols(&offset);

	/* Verify HTTP2 frames. */
	expect_http2_settings_frame(&offset, false);
	expect_http2_headers_frame(&offset, UPGRADE_STREAM_ID, HTTP2_FLAG_END_HEADERS, NULL, 0);
	expect_http2_data_frame(&offset, UPGRADE_STREAM_ID, TEST_STATIC_PAYLOAD,
				strlen(TEST_STATIC_PAYLOAD),
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http1_static_get)
{
	static const char http1_request[] =
		"GET / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"\r\n";
	static const char expected_response[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 13\r\n"
		"\r\n"
		TEST_STATIC_PAYLOAD;
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
}

/* Common code to verify POST/PUT/PATCH */
static void common_verify_http2_dynamic_post_request(const uint8_t *request,
						     size_t request_len)
{
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request, request_len, 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, NULL, 0);

	zassert_equal(dynamic_payload_len, strlen(TEST_DYNAMIC_POST_PAYLOAD),
		      "Wrong dynamic resource length");
	zassert_mem_equal(dynamic_payload, TEST_DYNAMIC_POST_PAYLOAD,
			  dynamic_payload_len, "Wrong dynamic resource data");
}

ZTEST(server_function_tests, test_http2_dynamic_post)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

/* Common code to verify POST/PUT/PATCH */
static void common_verify_http1_dynamic_upgrade_post(const uint8_t *method)
{
	static const char http1_request[] =
		" /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Content-Length: 17\r\n"
		"Connection: Upgrade, HTTP2-Settings\r\n"
		"Upgrade: h2c\r\n"
		"HTTP2-Settings: AAMAAABkAAQAoAAAAAIAAAAA\r\n"
		"\r\n"
		TEST_DYNAMIC_POST_PAYLOAD;
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, method, strlen(method), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Verify HTTP1 switching protocols response. */
	expect_http1_switching_protocols(&offset);

	/* Verify HTTP2 frames. */
	expect_http2_settings_frame(&offset, false);
	expect_http2_headers_frame(&offset, UPGRADE_STREAM_ID,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, NULL, 0);

	zassert_equal(dynamic_payload_len, strlen(TEST_DYNAMIC_POST_PAYLOAD),
		      "Wrong dynamic resource length");
	zassert_mem_equal(dynamic_payload, TEST_DYNAMIC_POST_PAYLOAD,
			  dynamic_payload_len, "Wrong dynamic resource data");
}

ZTEST(server_function_tests, test_http1_dynamic_upgrade_post)
{
	common_verify_http1_dynamic_upgrade_post("POST");
}

/* Common code to verify POST/PUT/PATCH */
static void common_verify_http1_dynamic_post(const uint8_t *method)
{
	static const char http1_request[] =
		" /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Content-Length: 17\r\n"
		"\r\n"
		TEST_DYNAMIC_POST_PAYLOAD;
	static const char expected_response[] = "HTTP/1.1 200\r\n"
						"Transfer-Encoding: chunked\r\n"
						"Content-Type: text/plain\r\n"
						"\r\n"
						"0\r\n\r\n";
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, method, strlen(method), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");

	zassert_equal(dynamic_payload_len, strlen(TEST_DYNAMIC_POST_PAYLOAD),
		      "Wrong dynamic resource length");
	zassert_mem_equal(dynamic_payload, TEST_DYNAMIC_POST_PAYLOAD,
			  dynamic_payload_len, "Wrong dynamic resource data");
}

ZTEST(server_function_tests, test_http1_dynamic_post)
{
	common_verify_http1_dynamic_post("POST");
}

static void common_verify_http2_dynamic_get_request(const uint8_t *request,
						    size_t request_len)
{
	size_t offset = 0;
	int ret;

	dynamic_payload_len = strlen(TEST_DYNAMIC_GET_PAYLOAD);
	memcpy(dynamic_payload, TEST_DYNAMIC_GET_PAYLOAD, dynamic_payload_len);

	ret = zsock_send(client_fd, request, request_len, 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1, HTTP2_FLAG_END_HEADERS, NULL, 0);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, TEST_DYNAMIC_GET_PAYLOAD,
				strlen(TEST_DYNAMIC_GET_PAYLOAD), HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http2_dynamic_get)
{
	static const uint8_t request_get_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_get_request(request_get_dynamic,
						sizeof(request_get_dynamic));
}

ZTEST(server_function_tests, test_http1_dynamic_upgrade_get)
{
	static const char http1_request[] =
		"GET /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"Connection: Upgrade, HTTP2-Settings\r\n"
		"Upgrade: h2c\r\n"
		"HTTP2-Settings: AAMAAABkAAQAoAAAAAIAAAAA\r\n"
		"\r\n";
	size_t offset = 0;
	int ret;

	dynamic_payload_len = strlen(TEST_DYNAMIC_GET_PAYLOAD);
	memcpy(dynamic_payload, TEST_DYNAMIC_GET_PAYLOAD, dynamic_payload_len);

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Verify HTTP1 switching protocols response. */
	expect_http1_switching_protocols(&offset);

	/* Verify HTTP2 frames. */
	expect_http2_settings_frame(&offset, false);
	expect_http2_headers_frame(&offset, UPGRADE_STREAM_ID, HTTP2_FLAG_END_HEADERS, NULL, 0);
	expect_http2_data_frame(&offset, UPGRADE_STREAM_ID, TEST_DYNAMIC_GET_PAYLOAD,
				strlen(TEST_DYNAMIC_GET_PAYLOAD), HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http1_dynamic_get)
{
	static const char http1_request[] =
		"GET /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"\r\n";
	static const char expected_response[] = "HTTP/1.1 200\r\n"
						"Transfer-Encoding: chunked\r\n"
						"Content-Type: text/plain\r\n"
						"\r\n"
						"10\r\n" TEST_DYNAMIC_GET_PAYLOAD "\r\n"
						"0\r\n\r\n";
	size_t offset = 0;
	int ret;

	dynamic_payload_len = strlen(TEST_DYNAMIC_GET_PAYLOAD);
	memcpy(dynamic_payload, TEST_DYNAMIC_GET_PAYLOAD, dynamic_payload_len);

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
}

ZTEST(server_function_tests, test_http2_dynamic_put)
{
	static const uint8_t request_put_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_PUT_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_PUT_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_put_dynamic,
						 sizeof(request_put_dynamic));
}

ZTEST(server_function_tests, test_http1_dynamic_upgrade_put)
{
	common_verify_http1_dynamic_upgrade_post("PUT");
}

ZTEST(server_function_tests, test_http1_dynamic_put)
{
	common_verify_http1_dynamic_post("PUT");
}

ZTEST(server_function_tests, test_http2_dynamic_patch)
{
	static const uint8_t request_patch_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_PATCH_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_PATCH_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_patch_dynamic,
						 sizeof(request_patch_dynamic));
}

ZTEST(server_function_tests, test_http1_dynamic_upgrade_patch)
{
	common_verify_http1_dynamic_upgrade_post("PATCH");
}

ZTEST(server_function_tests, test_http1_dynamic_patch)
{
	common_verify_http1_dynamic_post("PATCH");
}

ZTEST(server_function_tests, test_http2_dynamic_delete)
{
	static const uint8_t request_delete_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_DELETE_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request_delete_dynamic,
			 sizeof(request_delete_dynamic), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				   NULL, 0);
}

ZTEST(server_function_tests, test_http1_dynamic_upgrade_delete)
{
	static const char http1_request[] =
		"DELETE /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Connection: Upgrade, HTTP2-Settings\r\n"
		"Upgrade: h2c\r\n"
		"HTTP2-Settings: AAMAAABkAAQAoAAAAAIAAAAA\r\n"
		"\r\n";
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Verify HTTP1 switching protocols response. */
	expect_http1_switching_protocols(&offset);

	/* Verify HTTP2 frames. */
	expect_http2_settings_frame(&offset, false);
	expect_http2_headers_frame(&offset, UPGRADE_STREAM_ID,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				   NULL, 0);
}

ZTEST(server_function_tests, test_http1_dynamic_delete)
{
	static const char http1_request[] =
		"DELETE /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"\r\n";
	static const char expected_response[] = "HTTP/1.1 200\r\n"
						"Transfer-Encoding: chunked\r\n"
						"Content-Type: text/plain\r\n"
						"\r\n"
						"0\r\n\r\n";
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
}

ZTEST(server_function_tests, test_http1_connection_close)
{
	static const char http1_request_1[] =
		"GET / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"\r\n";
	static const char http1_request_2[] =
		"GET / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"Connection: close\r\n"
		"\r\n";
	static const char expected_response[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 13\r\n"
		"\r\n"
		TEST_STATIC_PAYLOAD;
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request_1, strlen(http1_request_1), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
	test_consume_data(&offset, sizeof(expected_response) - 1);

	/* With no connection: close, the server shall serve another request on
	 * the same connection.
	 */
	ret = zsock_send(client_fd, http1_request_2, strlen(http1_request_2), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
	test_consume_data(&offset, sizeof(expected_response) - 1);

	/* Second request included connection: close, so we should expect the
	 * connection to be closed now.
	 */
	ret = zsock_recv(client_fd, buf, sizeof(buf), 0);
	zassert_equal(ret, 0, "Connection should've been closed");
}

ZTEST(server_function_tests, test_http2_post_data_with_padding)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_PADDED,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

ZTEST(server_function_tests, test_http2_post_headers_with_priority)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1_PRIORITY,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_PADDED,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

ZTEST(server_function_tests, test_http2_post_headers_with_priority_and_padding)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1_PRIORITY_PADDED,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_PADDED,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

ZTEST(server_function_tests, test_http2_post_headers_with_continuation)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_PARTIAL_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_CONTINUATION_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

ZTEST(server_function_tests, test_http2_post_missing_continuation)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_PARTIAL_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	size_t offset = 0;
	int ret;

	memset(buf, 0, sizeof(buf));

	ret = zsock_send(client_fd, request_post_dynamic,
			 sizeof(request_post_dynamic), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	/* Expect settings, but processing headers (and lack of continuation
	 * frame) should break the stream, and trigger disconnect.
	 */
	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);

	ret = zsock_recv(client_fd, buf, sizeof(buf), 0);
	zassert_equal(ret, 0, "Connection should've been closed");
}

ZTEST(server_function_tests, test_http2_post_trailing_headers)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_NO_END_STREAM,
		TEST_HTTP2_TRAILING_HEADER_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request_post_dynamic,
			 sizeof(request_post_dynamic), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	/* In this case order is reversed, data frame had not END_STREAM flag.
	 * Because of this, reply will only be sent after processing the final
	 * trailing headers frame, but this will be preceded by window update
	 * after processing the data frame.
	 */
	expect_http2_window_update_frame(&offset, TEST_STREAM_ID_1);
	expect_http2_window_update_frame(&offset, 0);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, NULL, 0);

	zassert_equal(dynamic_payload_len, strlen(TEST_DYNAMIC_POST_PAYLOAD),
		      "Wrong dynamic resource length");
	zassert_mem_equal(dynamic_payload, TEST_DYNAMIC_POST_PAYLOAD,
			  dynamic_payload_len, "Wrong dynamic resource data");
}

ZTEST(server_function_tests, test_http2_get_headers_with_padding)
{
	static const uint8_t request_get_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_DYNAMIC_STREAM_1_PADDED,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_get_request(request_get_dynamic,
						sizeof(request_get_dynamic));
}

ZTEST(server_function_tests, test_http2_rst_stream)
{
	static const uint8_t request_rst_stream[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_RST_STREAM_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};

	size_t offset = 0;
	int ret;

	memset(buf, 0, sizeof(buf));

	ret = zsock_send(client_fd, request_rst_stream,
			 sizeof(request_rst_stream), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	/* Expect settings, but processing RST_STREAM should close the stream,
	 * so DATA frame should trigger connection error (closed stream) and
	 * disconnect.
	 */
	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);

	ret = zsock_recv(client_fd, buf, sizeof(buf), 0);
	zassert_equal(ret, 0, "Connection should've been closed");
}

static const char http1_header_capture_common_response[] = "HTTP/1.1 200\r\n"
							   "Transfer-Encoding: chunked\r\n"
							   "Content-Type: text/plain\r\n"
							   "\r\n"
							   "0\r\n\r\n";

static void test_http1_header_capture_common(const char *request)
{
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request, strlen(request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	test_read_data(&offset, sizeof(http1_header_capture_common_response) - 1);
	zassert_mem_equal(buf, http1_header_capture_common_response,
			  sizeof(http1_header_capture_common_response) - 1);
}

ZTEST(server_function_tests, test_http1_header_capture)
{
	static const char request[] = "GET /header_capture HTTP/1.1\r\n"
				      "User-Agent: curl/7.68.0\r\n"
				      "Test-Header: test_value\r\n"
				      "Accept: */*\r\n"
				      "Accept-Encoding: deflate, gzip, br\r\n"
				      "\r\n";
	struct http_header *hdrs = request_headers_clone.headers;
	int ret;

	test_http1_header_capture_common(request);

	zassert_equal(request_headers_clone.count, 2,
		      "Didn't capture the expected number of headers");
	zassert_equal(request_headers_clone.status, HTTP_HEADER_STATUS_OK,
		      "Header capture status was not OK");

	zassert_not_equal(hdrs[0].name, NULL, "First header name is NULL");
	zassert_not_equal(hdrs[0].value, NULL, "First header value is NULL");
	zassert_not_equal(hdrs[1].name, NULL, "Second header name is NULL");
	zassert_not_equal(hdrs[1].value, NULL, "Second header value is NULL");

	ret = strcmp(hdrs[0].name, "User-Agent");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[0].value, "curl/7.68.0");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].name, "Test-Header");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].value, "test_value");
	zassert_equal(0, ret, "Header strings did not match");
}

ZTEST(server_function_tests, test_http1_header_too_long)
{
	static const char request[] =
		"GET /header_capture HTTP/1.1\r\n"
		"User-Agent: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n"
		"Test-Header: test_value\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"\r\n";
	struct http_header *hdrs = request_headers_clone.headers;
	int ret;

	test_http1_header_capture_common(request);

	zassert_equal(request_headers_clone.count, 1,
		      "Didn't capture the expected number of headers");
	zassert_equal(request_headers_clone.status, HTTP_HEADER_STATUS_DROPPED,
		      "Header capture status was OK, but should not have been");

	/* First header too long should not stop second header being captured into first slot */
	zassert_not_equal(hdrs[0].name, NULL, "First header name is NULL");
	zassert_not_equal(hdrs[0].value, NULL, "First header value is NULL");

	ret = strcmp(hdrs[0].name, "Test-Header");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[0].value, "test_value");
	zassert_equal(0, ret, "Header strings did not match");
}

ZTEST(server_function_tests, test_http1_header_too_many)
{
	static const char request[] = "GET /header_capture HTTP/1.1\r\n"
				      "User-Agent: curl/7.68.0\r\n"
				      "Test-Header: test_value\r\n"
				      "Test-Header2: test_value2\r\n"
				      "Accept: */*\r\n"
				      "Accept-Encoding: deflate, gzip, br\r\n"
				      "\r\n";
	struct http_header *hdrs = request_headers_clone.headers;
	int ret;

	test_http1_header_capture_common(request);

	zassert_equal(request_headers_clone.count, 2,
		      "Didn't capture the expected number of headers");
	zassert_equal(request_headers_clone.status, HTTP_HEADER_STATUS_DROPPED,
		      "Header capture status OK, but should not have been");

	zassert_not_equal(hdrs[0].name, NULL, "First header name is NULL");
	zassert_not_equal(hdrs[0].value, NULL, "First header value is NULL");
	zassert_not_equal(hdrs[1].name, NULL, "Second header name is NULL");
	zassert_not_equal(hdrs[1].value, NULL, "Second header value is NULL");

	ret = strcmp(hdrs[0].name, "User-Agent");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[0].value, "curl/7.68.0");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].name, "Test-Header");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].value, "test_value");
	zassert_equal(0, ret, "Header strings did not match");
}

static void common_verify_http2_get_header_capture_request(const uint8_t *request,
							   size_t request_len)
{
	size_t offset = 0;
	int ret;

	dynamic_payload_len = strlen(TEST_DYNAMIC_GET_PAYLOAD);
	memcpy(dynamic_payload, TEST_DYNAMIC_GET_PAYLOAD, dynamic_payload_len);

	ret = zsock_send(client_fd, request, request_len, 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, NULL, 0);
}

ZTEST(server_function_tests, test_http2_header_capture)
{
	static const uint8_t request[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_HEADER_CAPTURE1_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	struct http_header *hdrs = request_headers_clone.headers;
	int ret;

	common_verify_http2_get_header_capture_request(request, sizeof(request));

	zassert_equal(request_headers_clone.count, 2,
		      "Didn't capture the expected number of headers");
	zassert_equal(request_headers_clone.status, HTTP_HEADER_STATUS_OK,
		      "Header capture status was not OK");

	zassert_not_equal(hdrs[0].name, NULL, "First header name is NULL");
	zassert_not_equal(hdrs[0].value, NULL, "First header value is NULL");
	zassert_not_equal(hdrs[1].name, NULL, "Second header name is NULL");
	zassert_not_equal(hdrs[1].value, NULL, "Second header value is NULL");

	ret = strcmp(hdrs[0].name, "User-Agent");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[0].value, "curl/7.81.0");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].name, "Test-Header");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].value, "test_value");
	zassert_equal(0, ret, "Header strings did not match");
}

ZTEST(server_function_tests, test_http2_header_too_long)
{
	static const uint8_t request[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_HEADER_CAPTURE2_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	struct http_header *hdrs = request_headers_clone.headers;
	int ret;

	common_verify_http2_get_header_capture_request(request, sizeof(request));

	zassert_equal(request_headers_clone.count, 1,
		      "Didn't capture the expected number of headers");
	zassert_equal(request_headers_clone.status, HTTP_HEADER_STATUS_DROPPED,
		      "Header capture status was OK, but should not have been");

	/* First header too long should not stop second header being captured into first slot */
	zassert_not_equal(hdrs[0].name, NULL, "First header name is NULL");
	zassert_not_equal(hdrs[0].value, NULL, "First header value is NULL");

	ret = strcmp(hdrs[0].name, "Test-Header");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[0].value, "test_value");
	zassert_equal(0, ret, "Header strings did not match");
}

ZTEST(server_function_tests, test_http2_header_too_many)
{
	static const uint8_t request[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_HEADER_CAPTURE3_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	struct http_header *hdrs = request_headers_clone.headers;
	int ret;

	common_verify_http2_get_header_capture_request(request, sizeof(request));

	zassert_equal(request_headers_clone.count, 2,
		      "Didn't capture the expected number of headers");
	zassert_equal(request_headers_clone.status, HTTP_HEADER_STATUS_DROPPED,
		      "Header capture status OK, but should not have been");

	zassert_not_equal(hdrs[0].name, NULL, "First header name is NULL");
	zassert_not_equal(hdrs[0].value, NULL, "First header value is NULL");
	zassert_not_equal(hdrs[1].name, NULL, "Second header name is NULL");
	zassert_not_equal(hdrs[1].value, NULL, "Second header value is NULL");

	ret = strcmp(hdrs[0].name, "User-Agent");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[0].value, "curl/7.81.0");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].name, "Test-Header");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].value, "test_value");
	zassert_equal(0, ret, "Header strings did not match");
}

ZTEST(server_function_tests, test_http2_header_concurrent)
{
	/* Two POST requests which are concurrent, ie. headers1, headers2, data1, data2 */
	static const uint8_t request[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_HEADER_CAPTURE_WITH_TESTHEADER_STREAM_1,
		TEST_HTTP2_HEADERS_POST_HEADER_CAPTURE2_NO_TESTHEADER_STREAM_2,
		TEST_HTTP2_DATA_POST_HEADER_CAPTURE_STREAM_1,
		TEST_HTTP2_DATA_POST_HEADER_CAPTURE_STREAM_2,
		TEST_HTTP2_GOAWAY,
	};

	struct http_header *hdrs = request_headers_clone.headers;
	struct http_header *hdrs2 = request_headers_clone2.headers;
	int ret;
	size_t offset = 0;

	ret = zsock_send(client_fd, request, sizeof(request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	/* Wait for response on both resources before checking captured headers */
	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, NULL, 0);
	expect_http2_window_update_frame(&offset, TEST_STREAM_ID_1);
	expect_http2_window_update_frame(&offset, 0);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_2,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, NULL, 0);

	/* Headers captured on /header_capture path should have two headers including the
	 * Test-Header
	 */
	zassert_equal(request_headers_clone.count, 2,
		      "Didn't capture the expected number of headers");

	zassert_not_equal(hdrs[0].name, NULL, "First header name is NULL");
	zassert_not_equal(hdrs[0].value, NULL, "First header value is NULL");
	zassert_not_equal(hdrs[1].name, NULL, "Second header name is NULL");
	zassert_not_equal(hdrs[1].value, NULL, "Second header value is NULL");

	ret = strcmp(hdrs[0].name, "User-Agent");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[0].value, "curl/7.81.0");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].name, "Test-Header");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs[1].value, "test_value");
	zassert_equal(0, ret, "Header strings did not match");

	/* Headers captured on the /header_capture2 path should have only one header, not including
	 * the Test-Header
	 */
	zassert_equal(request_headers_clone2.count, 1,
		      "Didn't capture the expected number of headers");

	zassert_not_equal(hdrs2[0].name, NULL, "First header name is NULL");
	zassert_not_equal(hdrs2[0].value, NULL, "First header value is NULL");

	ret = strcmp(hdrs2[0].name, "User-Agent");
	zassert_equal(0, ret, "Header strings did not match");
	ret = strcmp(hdrs2[0].value, "curl/7.81.0");
	zassert_equal(0, ret, "Header strings did not match");
}

static void test_http1_dynamic_response_headers(const char *request, const char *expected_response)
{
	int ret;
	size_t offset = 0;

	ret = zsock_send(client_fd, request, strlen(request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	test_read_data(&offset, strlen(expected_response));
	zassert_mem_equal(buf, expected_response, strlen(expected_response));
}

static void test_http1_dynamic_response_headers_default(const char *expected_response, bool post)
{
	static const char http1_get_response_headers_request[] =
		"GET /response_headers HTTP/1.1\r\n"
		"Accept: */*\r\n"
		"\r\n";
	static const char http1_post_response_headers_request[] =
		"POST /response_headers HTTP/1.1\r\n"
		"Accept: */*\r\n"
		"Content-Length: 17\r\n"
		"\r\n" TEST_DYNAMIC_POST_PAYLOAD;
	const char *request =
		post ? http1_post_response_headers_request : http1_get_response_headers_request;

	test_http1_dynamic_response_headers(request, expected_response);
}

static void test_http2_dynamic_response_headers(const uint8_t *request, size_t request_len,
						const struct http_header *expected_headers,
						size_t expected_headers_count, bool end_stream,
						size_t *offset)
{
	int ret;
	const uint8_t expected_flags = end_stream ? HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM
						  : HTTP2_FLAG_END_HEADERS;

	ret = zsock_send(client_fd, request, request_len, 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	expect_http2_settings_frame(offset, false);
	expect_http2_settings_frame(offset, true);
	expect_http2_headers_frame(offset, TEST_STREAM_ID_1, expected_flags, expected_headers,
				   expected_headers_count);
}

static void test_http2_dynamic_response_headers_default(const struct http_header *expected_headers,
							size_t expected_headers_count, bool post,
							bool end_stream, size_t *offset)
{
	const uint8_t http2_get_response_headers_request[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_RESPONSE_HEADERS_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	const uint8_t http2_post_response_headers_request[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_RESPONSE_HEADERS_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	const uint8_t *request =
		post ? http2_post_response_headers_request : http2_get_response_headers_request;
	size_t request_len = post ? sizeof(http2_post_response_headers_request)
				  : sizeof(http2_get_response_headers_request);

	test_http2_dynamic_response_headers(request, request_len, expected_headers,
					    expected_headers_count, end_stream, offset);
}

static void test_http1_dynamic_response_header_none(bool post)
{
	static const char response[] = "HTTP/1.1 200\r\n"
				       "Transfer-Encoding: chunked\r\n"
				       "Content-Type: text/plain\r\n"
				       "\r\n"
				       "0\r\n\r\n";

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_NONE;

	test_http1_dynamic_response_headers_default(response, post);
}

ZTEST(server_function_tests, test_http1_dynamic_get_response_header_none)
{
	test_http1_dynamic_response_header_none(false);
}

ZTEST(server_function_tests, test_http1_dynamic_post_response_header_none)
{
	test_http1_dynamic_response_header_none(true);
}

static void test_http2_dynamic_response_header_none(bool post)
{
	size_t offset = 0;
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "200"},
		{.name = "content-type", .value = "text/plain"},
	};

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_NONE;

	test_http2_dynamic_response_headers_default(expected_headers, ARRAY_SIZE(expected_headers),
						    post, true, &offset);
}

ZTEST(server_function_tests, test_http2_dynamic_get_response_header_none)
{
	test_http2_dynamic_response_header_none(false);
}

ZTEST(server_function_tests, test_http2_dynamic_post_response_header_none)
{
	test_http2_dynamic_response_header_none(true);
}

static void test_http1_dynamic_response_header_422(bool post)
{
	static const char response[] = "HTTP/1.1 422\r\n"
				       "Transfer-Encoding: chunked\r\n"
				       "Content-Type: text/plain\r\n"
				       "\r\n"
				       "0\r\n\r\n";

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_422;

	test_http1_dynamic_response_headers_default(response, post);
}

ZTEST(server_function_tests, test_http1_dynamic_get_response_header_422)
{
	test_http1_dynamic_response_header_422(false);
}

ZTEST(server_function_tests, test_http1_dynamic_post_response_header_422)
{
	test_http1_dynamic_response_header_422(true);
}

static void test_http2_dynamic_response_header_422(bool post)
{
	size_t offset = 0;
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "422"},
		{.name = "content-type", .value = "text/plain"},
	};

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_422;

	test_http2_dynamic_response_headers_default(expected_headers, ARRAY_SIZE(expected_headers),
						    post, true, &offset);
}

ZTEST(server_function_tests, test_http2_dynamic_get_response_header_422)
{
	test_http2_dynamic_response_header_422(false);
}

ZTEST(server_function_tests, test_http2_dynamic_post_response_header_422)
{
	test_http2_dynamic_response_header_422(true);
}

static void test_http1_dynamic_response_header_extra(bool post)
{
	static const char response[] = "HTTP/1.1 200\r\n"
				       "Transfer-Encoding: chunked\r\n"
				       "Test-Header: test_data\r\n"
				       "Content-Type: text/plain\r\n"
				       "\r\n"
				       "0\r\n\r\n";

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_EXTRA_HEADER;

	test_http1_dynamic_response_headers_default(response, post);
}

ZTEST(server_function_tests, test_http1_dynamic_get_response_header_extra)
{
	test_http1_dynamic_response_header_extra(false);
}

ZTEST(server_function_tests, test_http1_dynamic_post_response_header_extra)
{
	test_http1_dynamic_response_header_extra(true);
}

static void test_http2_dynamic_response_header_extra(bool post)
{
	size_t offset = 0;
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "200"},
		{.name = "content-type", .value = "text/plain"},
		{.name = "test-header", .value = "test_data"},
	};

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_EXTRA_HEADER;

	test_http2_dynamic_response_headers_default(expected_headers, ARRAY_SIZE(expected_headers),
						    post, true, &offset);
}

ZTEST(server_function_tests, test_http2_dynamic_get_response_header_extra)
{
	test_http2_dynamic_response_header_extra(false);
}

ZTEST(server_function_tests, test_http2_dynamic_post_response_header_extra)
{
	test_http2_dynamic_response_header_extra(true);
}

static void test_http1_dynamic_response_header_override(bool post)
{
	static const char response[] = "HTTP/1.1 200\r\n"
				       "Transfer-Encoding: chunked\r\n"
				       "Content-Type: application/json\r\n"
				       "\r\n"
				       "0\r\n\r\n";

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_OVERRIDE_HEADER;

	test_http1_dynamic_response_headers_default(response, post);
}

ZTEST(server_function_tests, test_http1_dynamic_get_response_header_override)
{
	test_http1_dynamic_response_header_override(false);
}

ZTEST(server_function_tests, test_http1_dynamic_post_response_header_override)
{
	test_http1_dynamic_response_header_override(true);
}

static void test_http2_dynamic_response_header_override(bool post)
{
	size_t offset = 0;
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "200"},
		{.name = "content-type", .value = "application/json"},
	};

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_OVERRIDE_HEADER;

	test_http2_dynamic_response_headers_default(expected_headers, ARRAY_SIZE(expected_headers),
						    post, true, &offset);
}

ZTEST(server_function_tests, test_http2_dynamic_get_response_header_override)
{
	test_http2_dynamic_response_header_override(false);
}

ZTEST(server_function_tests, test_http2_dynamic_post_response_header_override)
{
	test_http2_dynamic_response_header_override(true);
}

static void test_http1_dynamic_response_header_separate(bool post)
{
	static const char response[] = "HTTP/1.1 200\r\n"
				       "Transfer-Encoding: chunked\r\n"
				       "Test-Header: test_data\r\n"
				       "Content-Type: text/plain\r\n"
				       "\r\n"
				       "10\r\n" TEST_DYNAMIC_GET_PAYLOAD "\r\n"
				       "0\r\n\r\n";

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_SEPARATE;

	test_http1_dynamic_response_headers_default(response, post);
}

ZTEST(server_function_tests, test_http1_dynamic_get_response_header_separate)
{
	test_http1_dynamic_response_header_separate(false);
}

ZTEST(server_function_tests, test_http1_dynamic_post_response_header_separate)
{
	test_http1_dynamic_response_header_separate(true);
}

static void test_http2_dynamic_response_header_separate(bool post)
{
	size_t offset = 0;
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "200"},
		{.name = "test-header", .value = "test_data"},
		{.name = "content-type", .value = "text/plain"},
	};

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_SEPARATE;

	test_http2_dynamic_response_headers_default(expected_headers, ARRAY_SIZE(expected_headers),
						    post, false, &offset);
}

ZTEST(server_function_tests, test_http2_dynamic_get_response_header_separate)
{
	test_http2_dynamic_response_header_separate(false);
}

ZTEST(server_function_tests, test_http2_dynamic_post_response_header_separate)
{
	test_http2_dynamic_response_header_separate(true);
}

static void test_http1_dynamic_response_header_combined(bool post)
{
	static const char response[] = "HTTP/1.1 200\r\n"
				       "Transfer-Encoding: chunked\r\n"
				       "Test-Header: test_data\r\n"
				       "Content-Type: text/plain\r\n"
				       "\r\n"
				       "10\r\n" TEST_DYNAMIC_GET_PAYLOAD "\r\n"
				       "0\r\n\r\n";

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_COMBINED;

	test_http1_dynamic_response_headers_default(response, post);
}

ZTEST(server_function_tests, test_http1_dynamic_get_response_header_combined)
{
	test_http1_dynamic_response_header_combined(false);
}

ZTEST(server_function_tests, test_http1_dynamic_post_response_header_combined)
{
	test_http1_dynamic_response_header_combined(true);
}

static void test_http2_dynamic_response_header_combined(bool post)
{
	size_t offset = 0;
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "200"},
		{.name = "test-header", .value = "test_data"},
		{.name = "content-type", .value = "text/plain"},
	};

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_COMBINED;

	test_http2_dynamic_response_headers_default(expected_headers, ARRAY_SIZE(expected_headers),
						    post, false, &offset);
}

ZTEST(server_function_tests, test_http2_dynamic_get_response_header_combined)
{
	test_http2_dynamic_response_header_combined(false);
}

ZTEST(server_function_tests, test_http2_dynamic_post_response_header_combined)
{
	test_http2_dynamic_response_header_combined(true);
}

ZTEST(server_function_tests, test_http1_dynamic_get_response_header_long)
{
	static const char response[] = "HTTP/1.1 200\r\n"
				       "Transfer-Encoding: chunked\r\n"
				       "Content-Type: text/plain\r\n"
				       "\r\n"
				       "130\r\n" TEST_LONG_PAYLOAD_CHUNK_1 "\r\n"
				       "d0\r\n" TEST_LONG_PAYLOAD_CHUNK_2 "\r\n"
				       "0\r\n\r\n";

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_LONG;

	test_http1_dynamic_response_headers_default(response, false);
}

ZTEST(server_function_tests, test_http1_dynamic_post_response_header_long)
{
	static const char request[] = "POST /response_headers HTTP/1.1\r\n"
				      "Accept: */*\r\n"
				      "Content-Length: 512\r\n"
				      "\r\n" TEST_LONG_PAYLOAD_CHUNK_1 TEST_LONG_PAYLOAD_CHUNK_2;
	static const char response[] = "HTTP/1.1 200\r\n"
				       "Transfer-Encoding: chunked\r\n"
				       "Content-Type: text/plain\r\n"
				       "\r\n"
				       "0\r\n\r\n";

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_LONG;

	test_http1_dynamic_response_headers(request, response);
	zassert_mem_equal(dynamic_response_headers_buffer, long_payload, strlen(long_payload));
}

ZTEST(server_function_tests, test_http2_dynamic_get_response_header_long)
{
	size_t offset = 0;
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "200"},
		{.name = "content-type", .value = "text/plain"},
	};

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_LONG;

	test_http2_dynamic_response_headers_default(expected_headers, ARRAY_SIZE(expected_headers),
						    false, false, &offset);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, TEST_LONG_PAYLOAD_CHUNK_1,
				strlen(TEST_LONG_PAYLOAD_CHUNK_1), 0);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, TEST_LONG_PAYLOAD_CHUNK_2,
				strlen(TEST_LONG_PAYLOAD_CHUNK_2), HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http2_dynamic_post_response_header_long)
{
	size_t offset = 0;
	size_t req_offset = 0;

	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "200"},
		{.name = "content-type", .value = "text/plain"},
	};

	const uint8_t request_part1[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_RESPONSE_HEADERS_STREAM_1,
		/* Data frame header */
		0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, TEST_STREAM_ID_1,
	};

	const uint8_t request_part3[] = {
		TEST_HTTP2_GOAWAY,
	};

	static uint8_t
		request[sizeof(request_part1) + (sizeof(long_payload) - 1) + sizeof(request_part3)];

	BUILD_ASSERT(sizeof(long_payload) - 1 == 0x200, "Length field in data frame header must "
							"match length of long_payload");

	memcpy(request + req_offset, request_part1, sizeof(request_part1));
	req_offset += sizeof(request_part1);
	memcpy(request + req_offset, long_payload, sizeof(long_payload) - 1);
	req_offset += sizeof(long_payload) - 1;
	memcpy(request + req_offset, request_part3, sizeof(request_part3));
	req_offset += sizeof(request_part3);

	dynamic_response_headers_variant = DYNAMIC_RESPONSE_HEADERS_VARIANT_BODY_LONG;

	test_http2_dynamic_response_headers(request, req_offset, expected_headers,
					    ARRAY_SIZE(expected_headers), true, &offset);
	zassert_mem_equal(dynamic_response_headers_buffer, long_payload, strlen(long_payload));
}

ZTEST(server_function_tests, test_http1_409_method_not_allowed)
{
	static const char http1_request[] =
		"POST / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 13\r\n\r\n"
		TEST_STATIC_PAYLOAD;
	static const char expected_response[] =
		"HTTP/1.1 405 Method Not Allowed\r\n";
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
}

ZTEST(server_function_tests, test_http1_upgrade_409_method_not_allowed)
{
	static const char http1_request[] =
		"POST / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 13\r\n"
		"Connection: Upgrade, HTTP2-Settings\r\n"
		"Upgrade: h2c\r\n"
		"HTTP2-Settings: AAMAAABkAAQAoAAAAAIAAAAA\r\n\r\n"
		TEST_STATIC_PAYLOAD;
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "405"}
	};
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Verify HTTP1 switching protocols response. */
	expect_http1_switching_protocols(&offset);

	/* Verify HTTP2 frames. */
	expect_http2_settings_frame(&offset, false);
	expect_http2_headers_frame(&offset, UPGRADE_STREAM_ID,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				   expected_headers, 1);
}

ZTEST(server_function_tests, test_http2_409_method_not_allowed)
{
	static const uint8_t request_post_static[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_ROOT_STREAM_1,
		TEST_HTTP2_DATA_POST_ROOT_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "405"}
	};
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request_post_static,
			 sizeof(request_post_static), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				   expected_headers, 1);
}

ZTEST(server_function_tests, test_http1_500_internal_server_error)
{
	static const char http1_request[] =
		"GET /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"\r\n";
	static const char expected_response[] =
		"HTTP/1.1 500 Internal Server Error\r\n";
	size_t offset = 0;
	int ret;

	dynamic_error = true;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
}

ZTEST(server_function_tests, test_http1_upgrade_500_internal_server_error)
{
	static const char http1_request[] =
		"GET /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"Connection: Upgrade, HTTP2-Settings\r\n"
		"Upgrade: h2c\r\n"
		"HTTP2-Settings: AAMAAABkAAQAoAAAAAIAAAAA\r\n"
		"\r\n";
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "500"}
	};
	size_t offset = 0;
	int ret;

	dynamic_error = true;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Verify HTTP1 switching protocols response. */
	expect_http1_switching_protocols(&offset);

	/* Verify HTTP2 frames. */
	expect_http2_settings_frame(&offset, false);
	expect_http2_headers_frame(&offset, UPGRADE_STREAM_ID,
				   HTTP2_FLAG_END_HEADERS,
				   expected_headers, 1);
	/* Expect data frame with reason but don't check the content as it may
	 * depend on libc being used (i. e. string returned by strerror()).
	 */
	expect_http2_data_frame(&offset, UPGRADE_STREAM_ID, NULL, 0,
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http2_500_internal_server_error)
{
	static const uint8_t request_get_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	const struct http_header expected_headers[] = {
		{.name = ":status", .value = "500"}
	};
	size_t offset = 0;
	int ret;

	dynamic_error = true;

	ret = zsock_send(client_fd, request_get_dynamic,
			 sizeof(request_get_dynamic), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS,
				   expected_headers, 1);
	/* Expect data frame with reason but don't check the content as it may
	 * depend on libc being used (i. e. string returned by strerror()).
	 */
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, NULL, 0,
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests_no_init, test_http_server_start_stop)
{
	struct sockaddr_in sa = { 0 };
	int ret;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);

	ret = zsock_inet_pton(AF_INET, SERVER_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_equal(1, ret, "inet_pton() failed to convert %s", SERVER_IPV4_ADDR);

	zassert_ok(http_server_start(), "Failed to start the server");
	zassert_not_ok(http_server_start(), "Server start should report na error.");

	zassert_ok(http_server_stop(), "Failed to stop the server");
	zassert_not_ok(http_server_stop(), "Server stop should report na error.");

	zassert_ok(http_server_start(), "Failed to start the server");

	/* Server should be listening now. */
	ret = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_not_equal(ret, -1, "failed to create client socket (%d)", errno);
	client_fd = ret;

	zassert_ok(zsock_connect(client_fd, (struct sockaddr *)&sa, sizeof(sa)),
		   "failed to connect to the server (%d)", errno);
	zassert_ok(zsock_close(client_fd), "close() failed on the client fd (%d)", errno);
	client_fd = -1;

	/* Check if the server can be restarted again after client connected. */
	zassert_ok(http_server_stop(), "Failed to stop the server");
	zassert_ok(http_server_start(), "Failed to start the server");

	/* Let the server thread run. */
	k_msleep(CONFIG_HTTP_SERVER_RESTART_DELAY + 10);

	ret = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_not_equal(ret, -1, "failed to create client socket (%d)", errno);
	client_fd = ret;

	zassert_ok(zsock_connect(client_fd, (struct sockaddr *)&sa, sizeof(sa)),
		   "failed to connect to the server (%d)", errno);
	zassert_ok(zsock_close(client_fd), "close() failed on the client fd (%d)", errno);
	client_fd = -1;

	zassert_ok(http_server_stop(), "Failed to stop the server");
}

ZTEST(server_function_tests_no_init, test_get_frame_type_name)
{
	zassert_str_equal(get_frame_type_name(HTTP2_DATA_FRAME), "DATA",
			  "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_HEADERS_FRAME),
			  "HEADERS", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_PRIORITY_FRAME),
			  "PRIORITY", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_RST_STREAM_FRAME),
			  "RST_STREAM", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_SETTINGS_FRAME),
			  "SETTINGS", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_PUSH_PROMISE_FRAME),
			  "PUSH_PROMISE", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_PING_FRAME), "PING",
			  "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_GOAWAY_FRAME),
			  "GOAWAY", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_WINDOW_UPDATE_FRAME),
			  "WINDOW_UPDATE", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_CONTINUATION_FRAME),
			  "CONTINUATION", "Unexpected frame type");
}

ZTEST(server_function_tests_no_init, test_parse_http_frames)
{
	static struct http_client_ctx ctx_client1;
	static struct http_client_ctx ctx_client2;
	struct http2_frame *frame;

	unsigned char buffer1[] = {
		0x00, 0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x64, 0x00,
		0x04, 0x00, 0x00, 0xff, 0xff, 0x00
	};
	unsigned char buffer2[] = {
		0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00,
		0x01, 0x82, 0x84, 0x86, 0x41, 0x8a, 0x0b, 0xe2,
		0x5c, 0x0b, 0x89, 0x70, 0xdc, 0x78, 0x0f, 0x03,
		0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a,
		0xaa, 0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68,
		0x0b, 0x83
	};

	memcpy(ctx_client1.buffer, buffer1, sizeof(buffer1));
	memcpy(ctx_client2.buffer, buffer2, sizeof(buffer2));

	ctx_client1.cursor = ctx_client1.buffer;
	ctx_client1.data_len = ARRAY_SIZE(buffer1);

	ctx_client2.cursor = ctx_client2.buffer;
	ctx_client2.data_len = ARRAY_SIZE(buffer2);

	/* Test: Buffer with the first frame */
	int parser1 = parse_http_frame_header(&ctx_client1, ctx_client1.cursor,
					      ctx_client1.data_len);

	zassert_equal(parser1, 0, "Failed to parse the first frame");

	frame = &ctx_client1.current_frame;

	/* Validate frame details for the 1st frame */
	zassert_equal(frame->length, 0x0C, "Expected length for the 1st frame doesn't match");
	zassert_equal(frame->type, 0x04, "Expected type for the 1st frame doesn't match");
	zassert_equal(frame->flags, 0x00, "Expected flags for the 1st frame doesn't match");
	zassert_equal(frame->stream_identifier, 0x00,
		      "Expected stream_identifier for the 1st frame doesn't match");

	/* Test: Buffer with the second frame */
	int parser2 = parse_http_frame_header(&ctx_client2, ctx_client2.cursor,
					      ctx_client2.data_len);

	zassert_equal(parser2, 0, "Failed to parse the second frame");

	frame = &ctx_client2.current_frame;

	/* Validate frame details for the 2nd frame */
	zassert_equal(frame->length, 0x21, "Expected length for the 2nd frame doesn't match");
	zassert_equal(frame->type, 0x01, "Expected type for the 2nd frame doesn't match");
	zassert_equal(frame->flags, 0x05, "Expected flags for the 2nd frame doesn't match");
	zassert_equal(frame->stream_identifier, 0x01,
		      "Expected stream_identifier for the 2nd frame doesn't match");
}

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_ram_disk)

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);

#define TEST_PARTITION		storage_partition
#define TEST_PARTITION_ID	FIXED_PARTITION_ID(TEST_PARTITION)

#define LFS_MNTP		"/littlefs"
#define TEST_FILE		"static_file.html"
#define TEST_DIR		"/files"
#define TEST_DIR_PATH		LFS_MNTP TEST_DIR

static struct http_resource_detail_static_fs static_file_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC_FS,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_type = "text/html",
		},
	.fs_path = TEST_DIR_PATH,
};

HTTP_RESOURCE_DEFINE(static_file_resource, test_http_service, "/static_file.html",
		     &static_file_resource_detail);

struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)TEST_PARTITION_ID,
	.mnt_point = LFS_MNTP,
};

void test_clear_flash(void)
{
	int rc;
	const struct flash_area *fap;

	rc = flash_area_open(TEST_PARTITION_ID, &fap);
	zassert_equal(rc, 0, "Opening flash area for erase [%d]\n", rc);

	rc = flash_area_flatten(fap, 0, fap->fa_size);
	zassert_equal(rc, 0, "Erasing flash area [%d]\n", rc);
}

static int test_mount(void)
{
	int ret;

	ret = fs_mount(&littlefs_mnt);
	if (ret < 0) {
		TC_PRINT("Error mounting fs [%d]\n", ret);
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_unmount(void)
{
	int ret;

	ret = fs_unmount(&littlefs_mnt);
	if (ret < 0 && ret != -EINVAL) {
		TC_PRINT("Error unmounting fs [%d]\n", ret);
		return TC_FAIL;
	}

	return TC_PASS;
}

#ifndef PATH_MAX
#define PATH_MAX 64
#endif

int check_file_dir_exists(const char *fpath)
{
	int res;
	struct fs_dirent entry;

	res = fs_stat(fpath, &entry);

	return !res;
}

int test_file_write(struct fs_file_t *filep, const char *test_str)
{
	ssize_t brw;
	int res;

	TC_PRINT("\nWrite tests:\n");

	/* Verify fs_seek() */
	res = fs_seek(filep, 0, FS_SEEK_SET);
	if (res) {
		TC_PRINT("fs_seek failed [%d]\n", res);
		fs_close(filep);
		return res;
	}

	TC_PRINT("Data written:\"%s\"\n\n", test_str);

	/* Verify fs_write() */
	brw = fs_write(filep, (char *)test_str, strlen(test_str));
	if (brw < 0) {
		TC_PRINT("Failed writing to file [%zd]\n", brw);
		fs_close(filep);
		return brw;
	}

	if (brw < strlen(test_str)) {
		TC_PRINT("Unable to complete write. Volume full.\n");
		TC_PRINT("Number of bytes written: [%zd]\n", brw);
		fs_close(filep);
		return TC_FAIL;
	}

	TC_PRINT("Data successfully written!\n");

	return res;
}

int test_mkdir(const char *dir_path, const char *file)
{
	int res;
	struct fs_file_t filep;
	char file_path[PATH_MAX] = { 0 };

	fs_file_t_init(&filep);
	res = sprintf(file_path, "%s/%s", dir_path, file);
	__ASSERT_NO_MSG(res < sizeof(file_path));

	if (check_file_dir_exists(dir_path)) {
		TC_PRINT("Dir %s exists\n", dir_path);
		return TC_FAIL;
	}

	TC_PRINT("Creating new dir %s\n", dir_path);

	/* Verify fs_mkdir() */
	res = fs_mkdir(dir_path);
	if (res) {
		TC_PRINT("Error creating dir[%d]\n", res);
		return res;
	}

	res = fs_open(&filep, file_path, FS_O_CREATE | FS_O_RDWR);
	if (res) {
		TC_PRINT("Failed opening file [%d]\n", res);
		return res;
	}

	TC_PRINT("Testing write to file %s\n", file_path);
	res = test_file_write(&filep, TEST_STATIC_FS_PAYLOAD);
	if (res) {
		fs_close(&filep);
		return res;
	}

	res = fs_close(&filep);
	if (res) {
		TC_PRINT("Error closing file [%d]\n", res);
		return res;
	}

	TC_PRINT("Created dir %s!\n", dir_path);

	return res;
}

static int setup_fs(const char *file_ending)
{
	char filename_buf[sizeof(TEST_FILE)+5] = TEST_FILE;

	strcat(filename_buf, file_ending);
	test_clear_flash();

	zassert_equal(test_unmount(), TC_PASS, "Failed to unmount fs");
	zassert_equal(test_mount(), TC_PASS, "Failed to mount fs");

	return test_mkdir(TEST_DIR_PATH, filename_buf);
}

ZTEST(server_function_tests, test_http1_static_fs)
{
	static const char http1_request[] =
		"GET /static_file.html HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"\r\n";
	static const char expected_response[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 30\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		TEST_STATIC_FS_PAYLOAD;
	size_t offset = 0;
	int ret;

	ret = setup_fs("");
	zassert_equal(ret, TC_PASS, "Failed to mount fs");

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
}

ZTEST(server_function_tests, test_http1_static_fs_compression)
{
#define HTTP1_COMPRESSION_REQUEST                                                                  \
	"GET /static_file.html HTTP/1.1\r\n"                                                       \
	"Host: 127.0.0.1:8080\r\n"                                                                 \
	"User-Agent: curl/7.68.0\r\n"                                                              \
	"Accept: */*\r\n"                                                                          \
	"Accept-Encoding: %s\r\n"                                                                  \
	"\r\n"
#define HTTP1_COMPRESSION_RESPONSE                                                                 \
	"HTTP/1.1 200 OK\r\n"                                                                      \
	"Content-Length: 30\r\n"                                                                   \
	"Content-Type: text/html\r\n"                                                              \
	"Content-Encoding: %s\r\n"                                                                 \
	"\r\n" TEST_STATIC_FS_PAYLOAD

	static const char mixed_compression_str[] = "gzip, deflate, br";
	static char http1_request[sizeof(HTTP1_COMPRESSION_REQUEST) +
				  ARRAY_SIZE(mixed_compression_str)] = {0};
	static char expected_response[sizeof(HTTP1_COMPRESSION_RESPONSE) +
				      HTTP_COMPRESSION_MAX_STRING_LEN] = {0};
	static const char *const file_ending_map[] = {[HTTP_GZIP] = ".gz",
						      [HTTP_COMPRESS] = ".lzw",
						      [HTTP_DEFLATE] = ".zz",
						      [HTTP_BR] = ".br",
						      [HTTP_ZSTD] = ".zst"};
	size_t offset;
	int ret;
	int expected_response_size;

	for (enum http_compression i = 0; compression_value_is_valid(i); ++i) {
		offset = 0;

		if (i == HTTP_NONE) {
			continue;
		}
		TC_PRINT("Testing %s compression...\n", http_compression_text(i));
		zassert(i < ARRAY_SIZE(file_ending_map) && &file_ending_map[i] != NULL,
			"No file ending defined for compression");

		sprintf(http1_request, HTTP1_COMPRESSION_REQUEST, http_compression_text(i));
		expected_response_size = sprintf(expected_response, HTTP1_COMPRESSION_RESPONSE,
						 http_compression_text(i));

		ret = setup_fs(file_ending_map[i]);
		zassert_equal(ret, TC_PASS, "Failed to mount fs");

		ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
		zassert_not_equal(ret, -1, "send() failed (%d)", errno);

		memset(buf, 0, sizeof(buf));

		test_read_data(&offset, expected_response_size);
		zassert_mem_equal(buf, expected_response, expected_response_size,
				  "Received data doesn't match expected response");
	}

	offset = 0;
	TC_PRINT("Testing mixed compression...\n");
	sprintf(http1_request, HTTP1_COMPRESSION_REQUEST, mixed_compression_str);
	expected_response_size = sprintf(expected_response, HTTP1_COMPRESSION_RESPONSE,
					 http_compression_text(HTTP_BR));
	ret = setup_fs(file_ending_map[HTTP_BR]);
	zassert_equal(ret, TC_PASS, "Failed to mount fs");

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, expected_response_size);
	zassert_mem_equal(buf, expected_response, expected_response_size,
			  "Received data doesn't match expected response");
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(zephyr_ram_disk) */

static void http_server_tests_before(void *fixture)
{
	struct sockaddr_in sa;
	struct timeval optval = {
		.tv_sec = TIMEOUT_S,
		.tv_usec = 0,
	};
	int ret;

	ARG_UNUSED(fixture);

	memset(dynamic_payload, 0, sizeof(dynamic_payload));
	memset(dynamic_response_headers_buffer, 0, sizeof(dynamic_response_headers_buffer));
	memset(&request_headers_clone, 0, sizeof(request_headers_clone));
	memset(&request_headers_clone2, 0, sizeof(request_headers_clone2));
	dynamic_payload_len = 0;
	dynamic_error = false;

	ret = http_server_start();
	if (ret < 0) {
		printk("Failed to start the server\n");
		return;
	}

	ret = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ret < 0) {
		printk("Failed to create client socket (%d)\n", errno);
		return;
	}
	client_fd = ret;

	ret = zsock_setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &optval,
			       sizeof(optval));
	if (ret < 0) {
		printk("Failed to set timeout (%d)\n", errno);
		return;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);

	ret = zsock_inet_pton(AF_INET, SERVER_IPV4_ADDR, &sa.sin_addr.s_addr);
	if (ret != 1) {
		printk("inet_pton() failed to convert %s\n", SERVER_IPV4_ADDR);
		return;
	}

	ret = zsock_connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		printk("Failed to connect (%d)\n", errno);
	}
}

static void http_server_tests_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (client_fd >= 0) {
		(void)zsock_close(client_fd);
		client_fd = -1;
	}

	(void)http_server_stop();

	k_yield();
}

ZTEST_SUITE(server_function_tests, NULL, NULL, http_server_tests_before,
	    http_server_tests_after, NULL);
ZTEST_SUITE(server_function_tests_no_init, NULL, NULL, NULL,
	    http_server_tests_after, NULL);
