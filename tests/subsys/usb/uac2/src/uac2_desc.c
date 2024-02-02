/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>

#include <usbd_uac2_macros.h>

static const uint8_t reference_ac_interface_descriptor[] = {
	/* 4.7.2 Class-Specific AC Interface Descriptor */
	0x09,			/* bLength = 9 */
	0x24,			/* bDescriptorType = CS_INTERFACE */
	0x01,			/* bDescriptorSubtype = HEADER */
	0x00, 0x02,		/* bcdADC = 02.00 */
	0x04,			/* bCategory = HEADSET */
	0x4b, 0x00,		/* wTotalLength = 0x4b = 75 */
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

static const uint8_t reference_ac_hp_output_terminal_descriptor[] = {
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
};

static const uint8_t reference_ac_mic_input_terminal_descriptor[] = {
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
};

static const uint8_t reference_ac_mic_output_terminal_descriptor[] = {
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
static const uint8_t reference_as_in_cs_general_descriptor[] = {
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

DT_FOREACH_CHILD(DT_NODELABEL(uac2_headset), VALIDATE_NODE)

UAC2_DESCRIPTOR_ARRAYS(DT_NODELABEL(uac2_headset))

const static struct usb_desc_header *generated_uac2_descriptor_set[] = {
	UAC2_DESCRIPTOR_PTRS(DT_NODELABEL(uac2_headset))
};

ZTEST(uac2_desc, test_uac2_descriptors)
{
	const struct usb_desc_header **ptr = generated_uac2_descriptor_set;

	const struct usb_association_descriptor *iad;
	const struct usb_if_descriptor *iface;
	const struct usb_ep_descriptor *ep;

	/* Headset has 3 interfaces: 1 AudioControl and 2 AudioStreaming */
	iad = (const struct usb_association_descriptor *)*ptr;
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
	zassert_mem_equal(reference_ac_hp_output_terminal_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_hp_output_terminal_descriptor));
	ptr++;
	zassert_mem_equal(reference_ac_mic_input_terminal_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_mic_input_terminal_descriptor));
	ptr++;
	zassert_mem_equal(reference_ac_mic_output_terminal_descriptor, *ptr,
		ARRAY_SIZE(reference_ac_mic_output_terminal_descriptor));
	ptr++;

	/* AudioStreaming OUT interface Alt 0 without endpoints */
	iface = (const struct usb_if_descriptor *)*ptr;
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
	zassert_equal(ep->bLength, sizeof(struct usb_ep_descriptor));
	zassert_equal(ep->bDescriptorType, USB_DESC_ENDPOINT);
	zassert_equal(ep->bEndpointAddress, FIRST_OUT_EP_ADDR);
	zassert_equal(ptr - generated_uac2_descriptor_set,
		UAC2_DESCRIPTOR_AS_DATA_EP_INDEX(DT_NODELABEL(as_iso_out)));
	zassert_equal(ep->Attributes.transfer, USB_EP_TYPE_ISO);
	zassert_equal(ep->Attributes.synch, 1 /* Asynchronous */);
	zassert_equal(ep->Attributes.usage, 0 /* Data Endpoint */);
	zassert_equal(sys_le16_to_cpu(ep->wMaxPacketSize), 196);
	zassert_equal(ep->bInterval, 1);
	ptr++;

	/* AudioStreaming OUT endpoint descriptor */
	zassert_mem_equal(reference_as_ep_descriptor, *ptr,
		ARRAY_SIZE(reference_as_ep_descriptor));
	ptr++;

	/* AudioStreaming IN interface Alt 0 without endpoints */
	iface = (const struct usb_if_descriptor *)*ptr;
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
	zassert_equal(ep->bLength, sizeof(struct usb_ep_descriptor));
	zassert_equal(ep->bDescriptorType, USB_DESC_ENDPOINT);
	zassert_equal(ep->bEndpointAddress, FIRST_IN_EP_ADDR);
	zassert_equal(ptr - generated_uac2_descriptor_set,
		UAC2_DESCRIPTOR_AS_DATA_EP_INDEX(DT_NODELABEL(as_iso_in)));
	zassert_equal(ep->Attributes.transfer, USB_EP_TYPE_ISO);
	zassert_equal(ep->Attributes.synch, 1 /* Asynchronous */);
	zassert_equal(ep->Attributes.usage, 2 /* Implicit Feedback Data */);
	zassert_equal(sys_le16_to_cpu(ep->wMaxPacketSize), 98);
	zassert_equal(ep->bInterval, 1);
	ptr++;

	/* AudioStreaming IN endpoint descriptor */
	zassert_mem_equal(reference_as_ep_descriptor, *ptr,
		ARRAY_SIZE(reference_as_ep_descriptor));
	ptr++;

	/* Confirm there is no trailing data */
	zassert_equal(*ptr, NULL);
}

ZTEST_SUITE(uac2_desc, NULL, NULL, NULL, NULL, NULL);
