/* opp_common.h - Shared declarations for the OPP board-to-board test firmware */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPP_TEST_COMMON_H_
#define OPP_TEST_COMMON_H_

#include <zephyr/bluetooth/classic/opp.h>

/*
 * A minimal vCard 2.1 object used as the pushed object and as the server's
 * default Get Object. It fits within a single OBEX packet.
 */
#define OPP_TEST_VCARD                                                                             \
	"BEGIN:VCARD\r\n"                                                                          \
	"VERSION:2.1\r\n"                                                                          \
	"N:Zephyr;OPP\r\n"                                                                         \
	"FN:OPP Zephyr\r\n"                                                                        \
	"TEL;CELL:+10000000000\r\n"                                                                \
	"END:VCARD\r\n"

/* OBEX Name header value (UTF-16BE) for the pushed object: "test.vcf". */
#define OPP_TEST_OBJECT_NAME_UTF16                                                                 \
	{0x00, 't', 0x00, 'e', 0x00, 's', 0x00, 't', 0x00, '.', 0x00, 'v', 0x00, 'c', 0x00, 'f',   \
	 0x00, 0x00}

/* Default maximum OBEX packet length proposed by both roles in this test. */
#define OPP_TEST_MOPL 0xFFFFU

#endif /* OPP_TEST_COMMON_H_ */
