/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <usbd_uac2_macros.h>

static const uint8_t reference_ac_descriptors[] = {
	/* 4.7.2 Class-Specific AC Interface Descriptor */
	0x09,			/* bLength = 9 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x01,			/* bDescriptorSubtype = HEADER */
	0x00, 0x02,		/* bcdADC = 02.00 */
	0x04,			/* bCategory = HEADSET */
	0x4b, 0x00,		/* wTotalLength = 0x4b = 75 */
	0x00,			/* bmControls = Latency Control not present */
	/* 4.7.2.1 Clock Source Descriptor */
	0x08,			/* bLength = 8 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x0a,			/* bDescriptorSubtype = CLOCK_SOURCE */
	0x01,			/* bClockID = 1 */
	0x03,			/* bmAttributes = Internal programmable */
	0x03,			/* bmControls = frequency host programmable */
	0x00,			/* bAssocTerminal = 0 (not associated) */
	0x00,			/* iClockSource = 0 (no string descriptor) */
	/* 4.7.2.4 Input Terminal Descriptor */
	0x11,			/* bLength = 17 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x02,			/* bDescriptorSubtype = INPUT_TERMINAL */
	0x02,			/* bTerminalID = 2 */
	0x01, 0x01,		/* wTerminalType = 0x0101 (USB streaming) */
	0x00,			/* bAssocTerminal = 0 (not associated) */
	0x01,			/* bCSourceID = 1 (main clock) */
	0x02,			/* bNrChannels = 2 */
	0x03, 0x00, 0x00, 0x00,	/* bmChannelConfig = Front Left, Front Right */
	0x00,			/* iChannelNames = 0 (all pre-defined) */
	0x00, 0x00,		/* bmControls = none present */
	0x00,			/* iTerminal = 0 (no string descriptor) */
	/* 4.7.2.5 Output Terminal Descriptor */
	0x0c,			/* bLength = 12 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x03,			/* bDescriptorSubtype = OUTPUT_TERMINAL */
	0x03,			/* bTerminalID = 3 */
	0x02, 0x04,		/* wTerminalType = 0x0402 (Headset) */
	0x04,			/* bAssocTerminal = 4 (headset input) */
	0x02,			/* bSourceID = 2 (streaming input) */
	0x01,			/* bCSourceID = 1 (main clock) */
	0x00, 0x00,		/* bmControls = none present */
	0x00,			/* iTerminal = 0 (no string descriptor) */
	/* 4.7.2.4 Input Terminal Descriptor */
	0x11,			/* bLength = 17 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x02,			/* bDescriptorSubtype = INPUT_TERMINAL */
	0x04,			/* bTerminalID = 4 */
	0x02, 0x04,		/* wTerminalType = 0x0402 (Headset) */
	0x03,			/* bAssocTerminal = 3 (headset output) */
	0x01,			/* bCSourceID = 1 (main clock) */
	0x01,			/* bNrChannels = 1 */
	0x01, 0x00, 0x00, 0x00,	/* bmChannelConfig = Front Left */
	0x00,			/* iChannelNames = 0 (all pre-defined) */
	0x00, 0x00,		/* bmControls = none present */
	0x00,			/* iTerminal = 0 (no string descriptor) */
	/* 4.7.2.5 Output Terminal Descriptor */
	0x0c,			/* bLength = 12 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x03,			/* bDescriptorSubtype = OUTPUT_TERMINAL */
	0x05,			/* bTerminalID = 5 */
	0x01, 0x01,		/* wTerminalType = 0x0101 (USB streaming) */
	0x00,			/* bAssocTerminal = 0 (not associated) */
	0x04,			/* bSourceID = 4 (headset input) */
	0x01,			/* bCSourceID = 1 (main clock) */
	0x00, 0x00,		/* bmControls = none present */
	0x00,			/* iTerminal = 0 (no string descriptor) */
};

/* USB IN = Audio device streaming output */
static const uint8_t reference_as_in_descriptors[] = {
	/* 4.9.2 Class-Specific AS Interface Descriptor */
	0x10,			/* bLength = 16 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x01,			/* bDescriptorSubtype = AS_GENERAL */
	0x05,			/* bTerminalLink = 5 (USB streaming output) */
	0x00,			/* bmControls = non present */
	0x01,			/* bFormatType = 1 */
	0x01, 0x00, 0x00, 0x00,	/* bmFormats = PCM */
	0x01,			/* bNrChannels = 1 */
	0x01, 0x00, 0x00, 0x00,	/* bmChannelConfig = Front Left */
	0x00,			/* iChannelNames = 0 (all pre-defined) */
	/* Universal Serial Bus Device Class Definition for Audio Data Formats
	 * Release 2.0, May 31, 2006. 2.3.1.6 Type I Format Type Descriptor
	 */
	0x06,			/* bLength = 6 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x02,			/* bDescriptorSubtype = FORMAT_TYPE */
	0x01,			/* bFormatType = 1 */
	0x02,			/* bSubslotSize = 2 */
	0x10,			/* bBitResolution = 16 */
};

/* USB OUT = Audio device streaming input */
static const uint8_t reference_as_out_descriptors[] = {
	/* 4.9.2 Class-Specific AS Interface Descriptor */
	0x10,			/* bLength = 16 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x01,			/* bDescriptorSubtype = AS_GENERAL */
	0x02,			/* bTerminalLink = 2 (USB streaming input) */
	0x00,			/* bmControls = non present */
	0x01,			/* bFormatType = 1 */
	0x01, 0x00, 0x00, 0x00,	/* bmFormats = PCM */
	0x02,			/* bNrChannels = 2 */
	0x03, 0x00, 0x00, 0x00,	/* bmChannelConfig = Front Left, Front Right */
	0x00,			/* iChannelNames = 0 (all pre-defined) */
	/* Universal Serial Bus Device Class Definition for Audio Data Formats
	 * Release 2.0, May 31, 2006. 2.3.1.6 Type I Format Type Descriptor
	 */
	0x06,			/* bLength = 6 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x02,			/* bDescriptorSubtype = FORMAT_TYPE */
	0x01,			/* bFormatType = 1 */
	0x02,			/* bSubslotSize = 2 */
	0x10,			/* bBitResolution = 16 */
};

DT_FOREACH_CHILD(DT_NODELABEL(uac2_headset), VALIDATE_NODE)

static const uint8_t generated_ac_descriptors[] = {
	AC_INTERFACE_HEADER_DESCRIPTOR(DT_NODELABEL(uac2_headset))
	ENTITY_HEADERS(DT_NODELABEL(uac2_headset))
};

static const uint8_t generated_as_in_descriptors[] = {
	AUDIO_STREAMING_INTERFACE_DESCRIPTORS(DT_NODELABEL(as_iso_in))
};

static const uint8_t generated_as_out_descriptors[] = {
	AUDIO_STREAMING_INTERFACE_DESCRIPTORS(DT_NODELABEL(as_iso_out))
};

ZTEST(uac2_desc, test_audiocontrol_descriptors)
{
	zassert_mem_equal(reference_ac_descriptors, generated_ac_descriptors,
		ARRAY_SIZE(reference_ac_descriptors));
}

ZTEST(uac2_desc, test_audiostreaming_descriptors)
{
	zassert_mem_equal(reference_as_in_descriptors,
		generated_as_in_descriptors,
		ARRAY_SIZE(reference_as_in_descriptors));

	zassert_mem_equal(reference_as_out_descriptors,
		generated_as_out_descriptors,
		ARRAY_SIZE(reference_as_out_descriptors));
}

ZTEST_SUITE(uac2_desc, NULL, NULL, NULL, NULL, NULL);
