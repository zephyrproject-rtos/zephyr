/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbd.h>
#include <usbd_uac2_macros.h>

/* 48 kHz headset with implicit feedback, allowed on both Full and High-Speed */
static const uint8_t reference_ac_interface_descriptor[] = {
	/* 4.7.2 Class-Specific AC Interface Descriptor */
	0x09,			/* bLength = 9 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x01,			/* bDescriptorSubtype = HEADER */
	0x00, 0x02,		/* bcdADC = 02.00 */
	0x04,			/* bCategory = HEADSET */
	0x6b, 0x00,		/* wTotalLength = 0x6b = 107 */
	0x00,			/* bmControls = Latency Control not present */
};

static const uint8_t reference_ac_clock_source_descriptor[] = {
	/* 4.7.2.1 Clock Source Descriptor */
	0x08,			/* bLength = 8 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x0a,			/* bDescriptorSubtype = CLOCK_SOURCE */
	0x01,			/* bClockID = 1 */
	0x03,			/* bmAttributes = Internal programmable */
	0x03,			/* bmControls = frequency host programmable */
	0x00,			/* bAssocTerminal = 0 (not associated) */
	0x00,			/* iClockSource = 0 (no string descriptor) */
};

static const uint8_t reference_ac_hp_input_terminal_descriptor[] = {
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
};

static const uint8_t reference_ac_hp_feature_unit_descriptor[] = {
	/* 4.7.2.8 Feature Unit Descriptor */
	0x12,			/* bLength = 18 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x06,			/* bDescriptorSubtype = FEATURE_UNIT */
	0x03,			/* bUnitID = 3 */
	0x02,			/* bSourceID = 2 (streaming input) */
	0x03, 0x30, 0x00, 0x00, /* bmaControls(0): Mute and Auto Gain */
	0x00, 0x30, 0x00, 0x00, /* bmaControls(1): Auto Gain */
	0x00, 0x30, 0x00, 0x00, /* bmaControls(2): Auto Gain */
	0x00,			/* iFeature = 0 (no string descriptor)*/
};

static const uint8_t reference_ac_hp_output_terminal_descriptor[] = {
	/* 4.7.2.5 Output Terminal Descriptor */
	0x0c,			/* bLength = 12 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x03,			/* bDescriptorSubtype = OUTPUT_TERMINAL */
	0x04,			/* bTerminalID = 4 */
	0x02, 0x04,		/* wTerminalType = 0x0402 (Headset) */
	0x05,			/* bAssocTerminal = 5 (headset input) */
	0x03,			/* bSourceID = 3 (headphones feature unit) */
	0x01,			/* bCSourceID = 1 (main clock) */
	0x00, 0x00,		/* bmControls = none present */
	0x00,			/* iTerminal = 0 (no string descriptor) */
};

static const uint8_t reference_ac_mic_input_terminal_descriptor[] = {
	/* 4.7.2.4 Input Terminal Descriptor */
	0x11,			/* bLength = 17 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x02,			/* bDescriptorSubtype = INPUT_TERMINAL */
	0x05,			/* bTerminalID = 5 */
	0x02, 0x04,		/* wTerminalType = 0x0402 (Headset) */
	0x04,			/* bAssocTerminal = 4 (headset output) */
	0x01,			/* bCSourceID = 1 (main clock) */
	0x01,			/* bNrChannels = 1 */
	0x01, 0x00, 0x00, 0x00,	/* bmChannelConfig = Front Left */
	0x00,			/* iChannelNames = 0 (all pre-defined) */
	0x00, 0x00,		/* bmControls = none present */
	0x00,			/* iTerminal = 0 (no string descriptor) */
};

static const uint8_t reference_ac_mic_feature_unit_descriptor[] = {
	/* 4.7.2.8 Feature Unit Descriptor */
	0x0e,			/* bLength = 14 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x06,			/* bDescriptorSubtype = FEATURE_UNIT */
	0x06,			/* bUnitID = 6 */
	0x05,			/* bSourceID = 5 (headset input) */
	0x03, 0x00, 0x00, 0x00, /* bmaControls(0): Mute */
	0x00, 0x30, 0x00, 0x00, /* bmaControls(1): Auto Gain */
	0x00,			/* iFeature = 0 (no string descriptor)*/
};

static const uint8_t reference_ac_mic_output_terminal_descriptor[] = {
	/* 4.7.2.5 Output Terminal Descriptor */
	0x0c,			/* bLength = 12 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x03,			/* bDescriptorSubtype = OUTPUT_TERMINAL */
	0x07,			/* bTerminalID = 7 */
	0x01, 0x01,		/* wTerminalType = 0x0101 (USB streaming) */
	0x00,			/* bAssocTerminal = 0 (not associated) */
	0x06,			/* bSourceID = 6 (mic feature unit) */
	0x01,			/* bCSourceID = 1 (main clock) */
	0x00, 0x00,		/* bmControls = none present */
	0x00,			/* iTerminal = 0 (no string descriptor) */
};

/* USB IN = Audio device streaming output */
static const uint8_t reference_as_in_cs_general_descriptor[] = {
	/* 4.9.2 Class-Specific AS Interface Descriptor */
	0x10,			/* bLength = 16 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x01,			/* bDescriptorSubtype = AS_GENERAL */
	0x07,			/* bTerminalLink = 7 (USB streaming output) */
	0x00,			/* bmControls = non present */
	0x01,			/* bFormatType = 1 */
	0x01, 0x00, 0x00, 0x00,	/* bmFormats = PCM */
	0x01,			/* bNrChannels = 1 */
	0x01, 0x00, 0x00, 0x00,	/* bmChannelConfig = Front Left */
	0x00,			/* iChannelNames = 0 (all pre-defined) */
};

static const uint8_t reference_as_in_cs_format_descriptor[] = {
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
static const uint8_t reference_as_out_cs_general_descriptor[] = {
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
};

static const uint8_t reference_as_out_cs_format_descriptor[] = {
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

static const uint8_t reference_as_ep_descriptor[] = {
	/* 4.10.1.2 Class-Specific AS Isochronous Audio Data Endpoint Descriptor
	 */
	0x08,			/* bLength = 8 */
	0x25,			/* bDescriptorType = CS_ENDPOINT */
	0x01,			/* bDescriptorSubtype = EP_GENERAL */
	0x00,			/* bmAttributes = MaxPacketsOnly not set */
	0x00,			/* bmControls = non present */
	0x00,			/* bLockDelayUnits = 0 (Undefined) */
	0x00, 0x00,		/* wLockDelay = 0 */
};

VALIDATE_INSTANCE(DT_NODELABEL(uac2_headset))

UAC2_DESCRIPTOR_ARRAYS(DT_NODELABEL(uac2_headset))

const static struct usb_desc_header *uac2_fs_descriptor_set[] =
	UAC2_FS_DESCRIPTOR_PTRS_ARRAY(DT_NODELABEL(uac2_headset));

const static struct usb_desc_header *uac2_hs_descriptor_set[] =
	UAC2_HS_DESCRIPTOR_PTRS_ARRAY(DT_NODELABEL(uac2_headset));

VALIDATE_INSTANCE(DT_NODELABEL(uac2_headset))

/* 192 kHz 24-bit stereo headphones allowed on High-Speed only */
static const uint8_t reference_hs_ac_interface_descriptor[] = {
	/* 4.7.2 Class-Specific AC Interface Descriptor */
	0x09,			/* bLength = 9 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x01,			/* bDescriptorSubtype = HEADER */
	0x00, 0x02,		/* bcdADC = 02.00 */
	0xff,			/* bCategory = OTHER */
	0x2e, 0x00,		/* wTotalLength = 0x2e = 46 */
	0x00,			/* bmControls = Latency Control not present */
};

static const uint8_t reference_hs_ac_clock_source_descriptor[] = {
	/* 4.7.2.1 Clock Source Descriptor */
	0x08,			/* bLength = 8 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x0a,			/* bDescriptorSubtype = CLOCK_SOURCE */
	0x01,			/* bClockID = 1 */
	0x03,			/* bmAttributes = Internal programmable */
	0x03,			/* bmControls = frequency host programmable */
	0x00,			/* bAssocTerminal = 0 (not associated) */
	0x00,			/* iClockSource = 0 (no string descriptor) */
};

static const uint8_t reference_hs_ac_input_terminal_descriptor[] = {
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
};

static const uint8_t reference_hs_ac_output_terminal_descriptor[] = {
	/* 4.7.2.5 Output Terminal Descriptor */
	0x0c,			/* bLength = 12 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x03,			/* bDescriptorSubtype = OUTPUT_TERMINAL */
	0x03,			/* bTerminalID = 3 */
	0x02, 0x03,		/* wTerminalType = 0x0302 (Headphones) */
	0x00,			/* bAssocTerminal = 0 (none) */
	0x02,			/* bSourceID = 2 (streaming input) */
	0x01,			/* bCSourceID = 1 (main clock) */
	0x00, 0x00,		/* bmControls = none present */
	0x00,			/* iTerminal = 0 (no string descriptor) */
};

static const uint8_t reference_hs_as_cs_general_descriptor[] = {
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
};

static const uint8_t reference_hs_as_cs_format_descriptor[] = {
	/* Universal Serial Bus Device Class Definition for Audio Data Formats
	 * Release 2.0, May 31, 2006. 2.3.1.6 Type I Format Type Descriptor
	 */
	0x06,			/* bLength = 6 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x02,			/* bDescriptorSubtype = FORMAT_TYPE */
	0x01,			/* bFormatType = 1 */
	0x03,			/* bSubslotSize = 3 */
	0x18,			/* bBitResolution = 24 */
};

VALIDATE_INSTANCE(DT_NODELABEL(hs_uac2_headphones))

UAC2_DESCRIPTOR_ARRAYS(DT_NODELABEL(hs_uac2_headphones))

const static struct usb_desc_header *fs_headphones_descriptor_set[] =
	UAC2_FS_DESCRIPTOR_PTRS_ARRAY(DT_NODELABEL(hs_uac2_headphones));

const static struct usb_desc_header *hs_headphones_descriptor_set[] =
	UAC2_HS_DESCRIPTOR_PTRS_ARRAY(DT_NODELABEL(hs_uac2_headphones));

static void test_uac2_descriptors(const struct usb_desc_header **descriptors,
				  enum usbd_speed speed)
{
	const struct usb_desc_header **ptr = descriptors;

	const struct usb_association_descriptor *iad;
	const struct usb_if_descriptor *iface;
	const struct usb_ep_descriptor *ep;

	/* Headset has 3 interfaces: 1 AudioControl and 2 AudioStreaming */
	iad = (const struct usb_association_descriptor *)*ptr;
	zassert_not_null(iad);
	zassert_equal(iad->bLength, sizeof(struct usb_association_descriptor));
	zassert_equal(iad->bDescriptorType, USB_DESC_INTERFACE_ASSOC);
	zassert_equal(iad->bFirstInterface, FIRST_INTERFACE_NUMBER);
	zassert_equal(iad->bInterfaceCount, 3);
	zassert_equal(iad->bFunctionClass, AUDIO_FUNCTION);
	zassert_equal(iad->bFunctionSubClass, FUNCTION_SUBCLASS_UNDEFINED);
	zassert_equal(iad->bFunctionProtocol, AF_VERSION_02_00);
	zassert_equal(iad->iFunction, 0);
	ptr++;

	/* AudioControl interface goes first */
	iface = (const struct usb_if_descriptor *)*ptr;
	zassert_not_null(iface);
	zassert_equal(iface->bLength, sizeof(struct usb_if_descriptor));
	zassert_equal(iface->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(iface->bInterfaceNumber, FIRST_INTERFACE_NUMBER);
	zassert_equal(iface->bAlternateSetting, 0);
	zassert_equal(iface->bNumEndpoints, 0);
	zassert_equal(iface->bInterfaceClass, AUDIO);
	zassert_equal(iface->bInterfaceSubClass, AUDIOCONTROL);
	zassert_equal(iface->bInterfaceProtocol, IP_VERSION_02_00);
	zassert_equal(iface->iInterface, 0);
	ptr++;

	/* AudioControl class-specific descriptors */
	zassert_mem_equal(reference_ac_interface_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_interface_descriptor));
	ptr++;
	zassert_mem_equal(reference_ac_clock_source_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_clock_source_descriptor));
	ptr++;
	zassert_mem_equal(reference_ac_hp_input_terminal_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_hp_input_terminal_descriptor));
	ptr++;
	zassert_mem_equal(reference_ac_hp_feature_unit_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_hp_feature_unit_descriptor));
	ptr++;
	zassert_mem_equal(reference_ac_hp_output_terminal_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_hp_output_terminal_descriptor));
	ptr++;
	zassert_mem_equal(reference_ac_mic_input_terminal_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_mic_input_terminal_descriptor));
	ptr++;
	zassert_mem_equal(reference_ac_mic_feature_unit_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_mic_feature_unit_descriptor));
	ptr++;
	zassert_mem_equal(reference_ac_mic_output_terminal_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_mic_output_terminal_descriptor));
	ptr++;

	/* AudioStreaming OUT interface Alt 0 without endpoints */
	iface = (const struct usb_if_descriptor *)*ptr;
	zassert_not_null(iface);
	zassert_equal(iface->bLength, sizeof(struct usb_if_descriptor));
	zassert_equal(iface->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(iface->bInterfaceNumber, FIRST_INTERFACE_NUMBER + 1);
	zassert_equal(iface->bAlternateSetting, 0);
	zassert_equal(iface->bNumEndpoints, 0);
	zassert_equal(iface->bInterfaceClass, AUDIO);
	zassert_equal(iface->bInterfaceSubClass, AUDIOSTREAMING);
	zassert_equal(iface->bInterfaceProtocol, IP_VERSION_02_00);
	zassert_equal(iface->iInterface, 0);
	ptr++;

	/* AudioStreaming OUT interface Alt 1 with endpoint */
	iface = (const struct usb_if_descriptor *)*ptr;
	zassert_not_null(iface);
	zassert_equal(iface->bLength, sizeof(struct usb_if_descriptor));
	zassert_equal(iface->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(iface->bInterfaceNumber, FIRST_INTERFACE_NUMBER + 1);
	zassert_equal(iface->bAlternateSetting, 1);
	zassert_equal(iface->bNumEndpoints, 1);
	zassert_equal(iface->bInterfaceClass, AUDIO);
	zassert_equal(iface->bInterfaceSubClass, AUDIOSTREAMING);
	zassert_equal(iface->bInterfaceProtocol, IP_VERSION_02_00);
	zassert_equal(iface->iInterface, 0);
	ptr++;

	/* AudioStreaming OUT class-specific descriptors */
	zassert_mem_equal(reference_as_out_cs_general_descriptor, *ptr,
		ARRAY_SIZE(reference_as_out_cs_general_descriptor));
	ptr++;
	zassert_mem_equal(reference_as_out_cs_format_descriptor, *ptr,
		ARRAY_SIZE(reference_as_out_cs_format_descriptor));
	ptr++;

	/* Isochronous OUT endpoint descriptor */
	ep = (const struct usb_ep_descriptor *)*ptr;
	zassert_not_null(ep);
	zassert_equal(ep->bLength, sizeof(struct usb_ep_descriptor));
	zassert_equal(ep->bDescriptorType, USB_DESC_ENDPOINT);
	zassert_equal(ep->bEndpointAddress, FIRST_OUT_EP_ADDR);
	zassert_equal(ptr - descriptors,
		UAC2_DESCRIPTOR_AS_DATA_EP_INDEX(DT_NODELABEL(as_iso_out)));
	zassert_equal(ep->Attributes.transfer, USB_EP_TYPE_ISO);
	zassert_equal(ep->Attributes.synch, 1 /* Asynchronous */);
	zassert_equal(ep->Attributes.usage, 0 /* Data Endpoint */);
	if (speed == USBD_SPEED_FS) {
		zassert_equal(sys_le16_to_cpu(ep->wMaxPacketSize), 196);
	} else {
		zassert_equal(speed, USBD_SPEED_HS);
		zassert_equal(sys_le16_to_cpu(ep->wMaxPacketSize), 28);
	}
	zassert_equal(ep->bInterval, 1);
	ptr++;

	/* AudioStreaming OUT endpoint descriptor */
	zassert_mem_equal(reference_as_ep_descriptor, *ptr,
		ARRAY_SIZE(reference_as_ep_descriptor));
	ptr++;

	/* AudioStreaming IN interface Alt 0 without endpoints */
	iface = (const struct usb_if_descriptor *)*ptr;
	zassert_not_null(iface);
	zassert_equal(iface->bLength, sizeof(struct usb_if_descriptor));
	zassert_equal(iface->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(iface->bInterfaceNumber, FIRST_INTERFACE_NUMBER + 2);
	zassert_equal(iface->bAlternateSetting, 0);
	zassert_equal(iface->bNumEndpoints, 0);
	zassert_equal(iface->bInterfaceClass, AUDIO);
	zassert_equal(iface->bInterfaceSubClass, AUDIOSTREAMING);
	zassert_equal(iface->bInterfaceProtocol, IP_VERSION_02_00);
	zassert_equal(iface->iInterface, 0);
	ptr++;

	/* AudioStreaming IN interface Alt 1 with endpoint */
	iface = (const struct usb_if_descriptor *)*ptr;
	zassert_not_null(iface);
	zassert_equal(iface->bLength, sizeof(struct usb_if_descriptor));
	zassert_equal(iface->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(iface->bInterfaceNumber, FIRST_INTERFACE_NUMBER + 2);
	zassert_equal(iface->bAlternateSetting, 1);
	zassert_equal(iface->bNumEndpoints, 1);
	zassert_equal(iface->bInterfaceClass, AUDIO);
	zassert_equal(iface->bInterfaceSubClass, AUDIOSTREAMING);
	zassert_equal(iface->bInterfaceProtocol, IP_VERSION_02_00);
	zassert_equal(iface->iInterface, 0);
	ptr++;

	/* AudioStreaming IN class-specific descriptors */
	zassert_mem_equal(reference_as_in_cs_general_descriptor, *ptr,
		ARRAY_SIZE(reference_as_in_cs_general_descriptor));
	ptr++;
	zassert_mem_equal(reference_as_in_cs_format_descriptor, *ptr,
		ARRAY_SIZE(reference_as_in_cs_format_descriptor));
	ptr++;

	/* Isochronous IN endpoint descriptor */
	ep = (const struct usb_ep_descriptor *)*ptr;
	zassert_not_null(ep);
	zassert_equal(ep->bLength, sizeof(struct usb_ep_descriptor));
	zassert_equal(ep->bDescriptorType, USB_DESC_ENDPOINT);
	zassert_equal(ep->bEndpointAddress, FIRST_IN_EP_ADDR);
	zassert_equal(ptr - descriptors,
		UAC2_DESCRIPTOR_AS_DATA_EP_INDEX(DT_NODELABEL(as_iso_in)));
	zassert_equal(ep->Attributes.transfer, USB_EP_TYPE_ISO);
	zassert_equal(ep->Attributes.synch, 1 /* Asynchronous */);
	zassert_equal(ep->Attributes.usage, 2 /* Implicit Feedback Data */);
	if (speed == USBD_SPEED_FS) {
		zassert_equal(sys_le16_to_cpu(ep->wMaxPacketSize), 98);
	} else {
		zassert_equal(speed, USBD_SPEED_HS);
		zassert_equal(sys_le16_to_cpu(ep->wMaxPacketSize), 14);
	}
	zassert_equal(ep->bInterval, 1);
	ptr++;

	/* AudioStreaming IN endpoint descriptor */
	zassert_mem_equal(reference_as_ep_descriptor, *ptr,
		ARRAY_SIZE(reference_as_ep_descriptor));
	ptr++;

	/* Confirm there is no trailing data */
	zassert_equal(*ptr, NULL);
}

ZTEST(uac2_desc, test_fs_uac2_descriptors)
{
	test_uac2_descriptors(uac2_fs_descriptor_set, USBD_SPEED_FS);
}

ZTEST(uac2_desc, test_hs_uac2_descriptors)
{
	test_uac2_descriptors(uac2_hs_descriptor_set, USBD_SPEED_HS);
}

ZTEST(uac2_desc, test_fs_hs_iface_and_ep_descriptors_not_shared)
{
	const struct usb_desc_header **fs_ptr = uac2_fs_descriptor_set;
	const struct usb_desc_header **hs_ptr = uac2_hs_descriptor_set;

	while (*fs_ptr && *hs_ptr) {
		const struct usb_desc_header *fs = *fs_ptr;
		const struct usb_desc_header *hs = *hs_ptr;

		zassert_equal(fs->bDescriptorType, hs->bDescriptorType);

		/* IAD, Interface and Endpoint descriptors must not be shared
		 * because there can be different number of interfaces between
		 * the speeds and thus descriptor fixup can assign different
		 * interface numbers and/or endpoint numbers at runtime.
		 */
		if (fs->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			zassert_not_equal(fs, hs, "Shared IAD descriptor");
		} else if (fs->bDescriptorType == USB_DESC_INTERFACE) {
			zassert_not_equal(fs, hs, "Shared interface descriptor");
		} else if (fs->bDescriptorType == USB_DESC_ENDPOINT) {
			zassert_not_equal(fs, hs, "Shared endpoint descriptor");
		} else {
			zassert_equal(fs, hs);
		}

		fs_ptr++;
		hs_ptr++;
	}
}

ZTEST(uac2_desc, test_hs_only_headset)
{
	const struct usb_desc_header **ptr = hs_headphones_descriptor_set;

	const struct usb_association_descriptor *iad;
	const struct usb_if_descriptor *iface;
	const struct usb_ep_descriptor *ep;

	/* Allowed only at High-Speed so Full-Speed should be NULL */
	zassert_equal(*fs_headphones_descriptor_set, NULL);

	/* Headset has 3 interfaces: 1 AudioControl and 2 AudioStreaming */
	iad = (const struct usb_association_descriptor *)*ptr;
	zassert_not_null(iad);
	zassert_equal(iad->bLength, sizeof(struct usb_association_descriptor));
	zassert_equal(iad->bDescriptorType, USB_DESC_INTERFACE_ASSOC);
	zassert_equal(iad->bFirstInterface, FIRST_INTERFACE_NUMBER);
	zassert_equal(iad->bInterfaceCount, 2);
	zassert_equal(iad->bFunctionClass, AUDIO_FUNCTION);
	zassert_equal(iad->bFunctionSubClass, FUNCTION_SUBCLASS_UNDEFINED);
	zassert_equal(iad->bFunctionProtocol, AF_VERSION_02_00);
	zassert_equal(iad->iFunction, 0);
	ptr++;

	/* AudioControl interface goes first */
	iface = (const struct usb_if_descriptor *)*ptr;
	zassert_not_null(iface);
	zassert_equal(iface->bLength, sizeof(struct usb_if_descriptor));
	zassert_equal(iface->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(iface->bInterfaceNumber, FIRST_INTERFACE_NUMBER);
	zassert_equal(iface->bAlternateSetting, 0);
	zassert_equal(iface->bNumEndpoints, 0);
	zassert_equal(iface->bInterfaceClass, AUDIO);
	zassert_equal(iface->bInterfaceSubClass, AUDIOCONTROL);
	zassert_equal(iface->bInterfaceProtocol, IP_VERSION_02_00);
	zassert_equal(iface->iInterface, 0);
	ptr++;

	/* AudioControl class-specific descriptors */
	zassert_mem_equal(reference_hs_ac_interface_descriptor, *ptr,
		ARRAY_SIZE(reference_hs_ac_interface_descriptor));
	ptr++;
	zassert_mem_equal(reference_hs_ac_clock_source_descriptor, *ptr,
		ARRAY_SIZE(reference_hs_ac_clock_source_descriptor));
	ptr++;
	zassert_mem_equal(reference_hs_ac_input_terminal_descriptor, *ptr,
		ARRAY_SIZE(reference_hs_ac_input_terminal_descriptor));
	ptr++;
	zassert_mem_equal(reference_hs_ac_output_terminal_descriptor, *ptr,
		ARRAY_SIZE(reference_hs_ac_output_terminal_descriptor));
	ptr++;

	/* AudioStreaming OUT interface Alt 0 without endpoints */
	iface = (const struct usb_if_descriptor *)*ptr;
	zassert_not_null(iface);
	zassert_equal(iface->bLength, sizeof(struct usb_if_descriptor));
	zassert_equal(iface->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(iface->bInterfaceNumber, FIRST_INTERFACE_NUMBER + 1);
	zassert_equal(iface->bAlternateSetting, 0);
	zassert_equal(iface->bNumEndpoints, 0);
	zassert_equal(iface->bInterfaceClass, AUDIO);
	zassert_equal(iface->bInterfaceSubClass, AUDIOSTREAMING);
	zassert_equal(iface->bInterfaceProtocol, IP_VERSION_02_00);
	zassert_equal(iface->iInterface, 0);
	ptr++;

	/* AudioStreaming OUT interface Alt 1 with endpoints */
	iface = (const struct usb_if_descriptor *)*ptr;
	zassert_not_null(iface);
	zassert_equal(iface->bLength, sizeof(struct usb_if_descriptor));
	zassert_equal(iface->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(iface->bInterfaceNumber, FIRST_INTERFACE_NUMBER + 1);
	zassert_equal(iface->bAlternateSetting, 1);
	zassert_equal(iface->bNumEndpoints, 2);
	zassert_equal(iface->bInterfaceClass, AUDIO);
	zassert_equal(iface->bInterfaceSubClass, AUDIOSTREAMING);
	zassert_equal(iface->bInterfaceProtocol, IP_VERSION_02_00);
	zassert_equal(iface->iInterface, 0);
	ptr++;

	/* AudioStreaming OUT class-specific descriptors */
	zassert_mem_equal(reference_hs_as_cs_general_descriptor, *ptr,
		ARRAY_SIZE(reference_hs_as_cs_general_descriptor));
	ptr++;
	zassert_mem_equal(reference_hs_as_cs_format_descriptor, *ptr,
		ARRAY_SIZE(reference_hs_as_cs_format_descriptor));
	ptr++;

	/* Isochronous OUT endpoint descriptor */
	ep = (const struct usb_ep_descriptor *)*ptr;
	zassert_not_null(ep);
	zassert_equal(ep->bLength, sizeof(struct usb_ep_descriptor));
	zassert_equal(ep->bDescriptorType, USB_DESC_ENDPOINT);
	zassert_equal(ep->bEndpointAddress, FIRST_OUT_EP_ADDR);
	zassert_equal(ptr - hs_headphones_descriptor_set,
		UAC2_DESCRIPTOR_AS_DATA_EP_INDEX(DT_NODELABEL(hs_as_iso_out)));
	zassert_equal(ep->Attributes.transfer, USB_EP_TYPE_ISO);
	zassert_equal(ep->Attributes.synch, 1 /* Asynchronous */);
	zassert_equal(ep->Attributes.usage, 0 /* Data Endpoint */);
	zassert_equal(sys_le16_to_cpu(ep->wMaxPacketSize), 150);
	zassert_equal(ep->bInterval, 1);
	ptr++;

	/* AudioStreaming OUT endpoint descriptor */
	zassert_mem_equal(reference_as_ep_descriptor, *ptr,
		ARRAY_SIZE(reference_as_ep_descriptor));
	ptr++;

	/* Isochronous IN explicit feedback endpoint descriptor */
	ep = (const struct usb_ep_descriptor *)*ptr;
	zassert_not_null(ep);
	zassert_equal(ep->bLength, sizeof(struct usb_ep_descriptor));
	zassert_equal(ep->bDescriptorType, USB_DESC_ENDPOINT);
	zassert_equal(ep->bEndpointAddress, FIRST_IN_EP_ADDR);
	zassert_equal(ptr - hs_headphones_descriptor_set,
		UAC2_DESCRIPTOR_AS_FEEDBACK_EP_INDEX(DT_NODELABEL(hs_as_iso_out)));
	zassert_equal(ep->Attributes.transfer, USB_EP_TYPE_ISO);
	zassert_equal(ep->Attributes.synch, 0 /* No Synchronization */);
	zassert_equal(ep->Attributes.usage, 1 /* Explicit Feedback-Endpoint */);
	zassert_equal(sys_le16_to_cpu(ep->wMaxPacketSize), 4);
	zassert_equal(ep->bInterval, 1);
	ptr++;

	/* Confirm there is no trailing data */
	zassert_equal(*ptr, NULL);
}

ZTEST_SUITE(uac2_desc, NULL, NULL, NULL, NULL, NULL);
