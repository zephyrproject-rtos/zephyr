/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RFID_CR95HF_H
#define RFID_CR95HF_H

#define CR95HF_SND_BUF_SIZE 50
#define CR95HF_RCV_BUF_SIZE 258

#define CR95HF_ISO_14443_106_KBPS 0x00 /* 106 Kbps */
#define CR95HF_ISO_14443_212_KBPS 0x01 /* 212 Kbps (2) */
#define CR95HF_ISO_14443_424_KBPS 0x02 /* 424 Kbps */

#define CR95HF_READY_TO_READ  0x8
#define CR95HF_READY_TO_WRITE 0x4

#define CR95HF_WFE_TAG_DETECTOR_ARRAY                                                              \
	{0x00, 0x07, 0x0E, 0x0A, 0x21, 0x00, 0x79, 0x01, 0x18,                                     \
	 0x00, 0x20, 0x60, 0x60, 0x64, 0x74, 0x3F, 0x28}

#endif /* RFID_CR95HF_H */
