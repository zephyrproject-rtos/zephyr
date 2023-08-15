/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio device class driver
 *
 * Driver for USB Audio device class driver
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>
#include <zephyr/usb/class/usb_audio.h>
#include "usb_audio_internal.h"

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/buf.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_audio, CONFIG_USB_AUDIO_LOG_LEVEL);

struct feature_volume {
	int16_t volume_max;
	int16_t volume_min;
	int16_t volume_res;
};

/* Device data structure */
struct usb_audio_dev_data {
	const struct usb_audio_ops *ops;

	uint8_t *controls[2];

	uint8_t ch_cnt[2];

	const struct cs_ac_if_descriptor *desc_hdr;

	struct usb_dev_data common;

	struct net_buf_pool *pool;

	/* Not applicable for Headphones, left with 0 */
	uint16_t in_frame_size;

	/* Not applicable for not support volume feature device */
	struct feature_volume volumes;

	bool rx_enable;
	bool tx_enable;
};

static sys_slist_t usb_audio_data_devlist;

/**
 * @brief Fill the USB Audio descriptor
 *
 * This macro fills USB descriptor for specific type of device
 * (Headphones or Microphone) depending on dev param.
 *
 * @note Feature unit has variable length and only 1st field of
 *	 .bmaControls is filled. Later its fixed in usb_fix_descriptor()
 * @note Audio control and Audio streaming interfaces are numerated starting
 *	 from 0 and are later fixed in usb_fix_descriptor()
 *
 * @param [in] dev	Device type. Must be HP/MIC
 * @param [in] i	Instance of device of current type (dev)
 * @param [in] id	Param for counting logic entities
 * @param [in] link	ID of IN/OUT terminal to which General Descriptor
 *			is linked.
 * @param [in] it_type	Input terminal type
 * @param [in] ot_type	Output terminal type
 */
#define DEFINE_AUDIO_DESCRIPTOR(dev, i, id, link, it_type, ot_type, cb, addr) \
USBD_CLASS_DESCR_DEFINE(primary, audio)					      \
struct dev##_descriptor_##i dev##_desc_##i = {				      \
	USB_AUDIO_IAD(2)						      \
	.std_ac_interface = INIT_STD_IF(USB_AUDIO_AUDIOCONTROL, 0, 0, 0),     \
	.cs_ac_interface = INIT_CS_AC_IF(dev, i, 1),			      \
	.input_terminal = INIT_IN_TERMINAL(dev, i, id, it_type),	      \
	.feature_unit = INIT_FEATURE_UNIT(dev, i, id + 1, id),		      \
	.output_terminal = INIT_OUT_TERMINAL(id + 2, id + 1, ot_type),	      \
	.as_interface_alt_0 = INIT_STD_IF(USB_AUDIO_AUDIOSTREAMING, 1, 0, 0), \
	.as_interface_alt_1 = INIT_STD_IF(USB_AUDIO_AUDIOSTREAMING, 1, 1, 1), \
	.as_cs_interface = INIT_AS_GENERAL(link),			      \
	.format = INIT_AS_FORMAT_I(CH_CNT(dev, i), GET_RES(dev, i)),	      \
	.std_ep = INIT_STD_AS_AD_EP(dev, i, addr),			      \
	.cs_ep = INIT_CS_AS_AD_EP,					      \
};									      \
static struct usb_ep_cfg_data dev##_usb_audio_ep_data_##i[] = {		      \
	INIT_EP_DATA(cb, addr),						      \
}

/**
 * @brief Fill the USB Audio descriptor
 *
 * This macro fills USB descriptor for specific type of device.
 * Macro is used when the device uses 2 audiostreaming interfaces,
 * eg. Headset
 *
 * @note Feature units have variable length and only 1st field of
 *	 .bmaControls is filled. Its fixed in usb_fix_descriptor()
 * @note Audio control and Audio streaming interfaces are numerated starting
 *	 from 0 and are later fixed in usb_fix_descriptor()
 *
 * @param [in] dev	Device type.
 * @param [in] i	Instance of device of current type (dev)
 * @param [in] id	Param for counting logic entities
 */
#define DEFINE_AUDIO_DESCRIPTOR_BIDIR(dev, i, id)			  \
USBD_CLASS_DESCR_DEFINE(primary, audio)					  \
struct dev##_descriptor_##i dev##_desc_##i = {				  \
	USB_AUDIO_IAD(3)						  \
	.std_ac_interface = INIT_STD_IF(USB_AUDIO_AUDIOCONTROL, 0, 0, 0), \
	.cs_ac_interface = INIT_CS_AC_IF_BIDIR(dev, i, 2),		  \
	.input_terminal_0 = INIT_IN_TERMINAL(dev##_MIC, i, id,		  \
						USB_AUDIO_IO_HEADSET),	  \
	.feature_unit_0 = INIT_FEATURE_UNIT(dev##_MIC, i, id+1, id),	  \
	.output_terminal_0 = INIT_OUT_TERMINAL(id+2, id+1,		  \
					USB_AUDIO_USB_STREAMING),	  \
	.input_terminal_1 = INIT_IN_TERMINAL(dev##_HP, i, id+3,		  \
					USB_AUDIO_USB_STREAMING),	  \
	.feature_unit_1 = INIT_FEATURE_UNIT(dev##_HP, i, id+4, id+3),	  \
	.output_terminal_1 = INIT_OUT_TERMINAL(id+5, id+4,		  \
						USB_AUDIO_IO_HEADSET),	  \
	.as_interface_alt_0_0 = INIT_STD_IF(USB_AUDIO_AUDIOSTREAMING,	  \
						1, 0, 0),		  \
	.as_interface_alt_0_1 = INIT_STD_IF(USB_AUDIO_AUDIOSTREAMING,	  \
						1, 1, 1),		  \
		.as_cs_interface_0 = INIT_AS_GENERAL(id+2),		  \
		.format_0 = INIT_AS_FORMAT_I(CH_CNT(dev##_MIC, i),	  \
					     GET_RES(dev##_MIC, i)),	  \
		.std_ep_0 = INIT_STD_AS_AD_EP(dev##_MIC, i,		  \
						   AUTO_EP_IN),		  \
		.cs_ep_0 = INIT_CS_AS_AD_EP,				  \
	.as_interface_alt_1_0 = INIT_STD_IF(USB_AUDIO_AUDIOSTREAMING,	  \
						2, 0, 0),		  \
	.as_interface_alt_1_1 = INIT_STD_IF(USB_AUDIO_AUDIOSTREAMING,	  \
						2, 1, 1),		  \
		.as_cs_interface_1 = INIT_AS_GENERAL(id+3),		  \
		.format_1 = INIT_AS_FORMAT_I(CH_CNT(dev##_HP, i),	  \
					     GET_RES(dev##_HP, i)),	  \
		.std_ep_1 = INIT_STD_AS_AD_EP(dev##_HP, i,		  \
						   AUTO_EP_OUT),	  \
		.cs_ep_1 = INIT_CS_AS_AD_EP,				  \
};									  \
static struct usb_ep_cfg_data dev##_usb_audio_ep_data_##i[] = {		  \
	INIT_EP_DATA(usb_transfer_ep_callback, AUTO_EP_IN),		  \
	INIT_EP_DATA(audio_receive_cb, AUTO_EP_OUT),			  \
}

#define DEFINE_AUDIO_DEV_DATA(dev, i, __out_pool, __in_pool_size)   \
	static uint8_t dev##_controls_##i[FEATURES_SIZE(dev, i)] = {0};\
	static struct usb_audio_dev_data dev##_audio_dev_data_##i =	\
		{ .pool = __out_pool,					\
		  .in_frame_size = __in_pool_size,			\
		  .controls = {dev##_controls_##i, NULL},		\
		  .ch_cnt = {(CH_CNT(dev, i) + 1), 0},			\
		  .volumes.volume_max = GET_VOLUME(dev, i, volume_max), \
		  .volumes.volume_min = GET_VOLUME(dev, i, volume_min), \
		  .volumes.volume_res = GET_VOLUME(dev, i, volume_res), \
		}

#define DEFINE_AUDIO_DEV_DATA_BIDIR(dev, i, __out_pool, __in_pool_size)	   \
	static uint8_t dev##_controls0_##i[FEATURES_SIZE(dev##_MIC, i)] = {0};\
	static uint8_t dev##_controls1_##i[FEATURES_SIZE(dev##_HP, i)] = {0}; \
	static struct usb_audio_dev_data dev##_audio_dev_data_##i =	   \
		{ .pool = __out_pool,					   \
		  .in_frame_size = __in_pool_size,			   \
		  .controls = {dev##_controls0_##i, dev##_controls1_##i},  \
		  .ch_cnt = {(CH_CNT(dev##_MIC, i) + 1),		   \
			     (CH_CNT(dev##_HP, i) + 1)},		   \
		  .volumes.volume_max = GET_VOLUME(dev, i, volume_max),	   \
		  .volumes.volume_min = GET_VOLUME(dev, i, volume_min),	   \
		  .volumes.volume_res = GET_VOLUME(dev, i, volume_res),	   \
		}

/**
 * Helper function for getting channel number directly from the
 * feature unit descriptor.
 */
static uint8_t get_num_of_channels(const struct feature_unit_descriptor *fu)
{
	return (fu->bLength - FU_FIXED_ELEMS_SIZE)/sizeof(uint16_t);
}

/**
 * Helper function for getting supported controls directly from
 * the feature unit descriptor.
 */
static uint16_t get_controls(const struct feature_unit_descriptor *fu)
{
	return sys_get_le16((uint8_t *)&fu->bmaControls[0]);
}

/**
 * Helper function for getting the device streaming direction
 */
static enum usb_audio_direction get_fu_dir(
				const struct feature_unit_descriptor *fu)
{
	const struct output_terminal_descriptor *ot =
		(struct output_terminal_descriptor *)
		((uint8_t *)fu + fu->bLength);
	enum usb_audio_direction dir;

	if (ot->wTerminalType == USB_AUDIO_USB_STREAMING) {
		dir = USB_AUDIO_IN;
	} else {
		dir = USB_AUDIO_OUT;
	}

	return dir;
}

/**
 * Helper function for fixing controls in feature units descriptors.
 */
static void fix_fu_descriptors(struct usb_if_descriptor *iface)
{
	struct cs_ac_if_descriptor *header;
	struct feature_unit_descriptor *fu;

	header = (struct cs_ac_if_descriptor *)
			((uint8_t *)iface + USB_PASSIVE_IF_DESC_SIZE);

	fu = (struct feature_unit_descriptor *)((uint8_t *)header +
						header->bLength +
						INPUT_TERMINAL_DESC_SIZE);

	/* start from 1 as elem 0 is filled when descriptor is declared */
	for (int i = 1; i < get_num_of_channels(fu); i++) {
		(void)memcpy(&fu->bmaControls[i],
			     &fu->bmaControls[0],
			     sizeof(uint16_t));
	}

	if (header->bInCollection == 2) {
		fu = (struct feature_unit_descriptor *)((uint8_t *)fu +
			fu->bLength +
			INPUT_TERMINAL_DESC_SIZE +
			OUTPUT_TERMINAL_DESC_SIZE);
		for (int i = 1; i < get_num_of_channels(fu); i++) {
			(void)memcpy(&fu->bmaControls[i],
				     &fu->bmaControls[0],
				     sizeof(uint16_t));
		}
	}
}

/**
 * Helper function for getting pointer to feature unit descriptor.
 * This is needed in order to address audio specific requests to proper
 * controls struct.
 */
static struct feature_unit_descriptor *get_feature_unit(
				struct usb_audio_dev_data *audio_dev_data,
				uint8_t *device, uint8_t fu_id)
{
	struct feature_unit_descriptor *fu;

	fu = (struct feature_unit_descriptor *)
		((uint8_t *)audio_dev_data->desc_hdr +
		audio_dev_data->desc_hdr->bLength +
		INPUT_TERMINAL_DESC_SIZE);

	if (fu->bUnitID == fu_id) {
		*device = 0;
		return fu;
	}
	/* skip to the next Feature Unit */
	fu = (struct feature_unit_descriptor *)
			((uint8_t *)fu + fu->bLength +
			INPUT_TERMINAL_DESC_SIZE +
			OUTPUT_TERMINAL_DESC_SIZE);
	*device = 1;

	return fu;
}

/**
 * @brief This is a helper function user to inform the user about
 * possibility to write the data to the device.
 */
static void audio_dc_sof(struct usb_cfg_data *cfg,
			 struct usb_audio_dev_data *dev_data)
{
	uint8_t ep_addr;

	/* In endpoint always at index 0 */
	ep_addr = cfg->endpoint[0].ep_addr;
	if ((ep_addr & USB_EP_DIR_MASK) && (dev_data->tx_enable)) {
		if (dev_data->ops && dev_data->ops->data_request_cb) {
			dev_data->ops->data_request_cb(
				dev_data->common.dev);
		}
	}
}

static void audio_interface_config(struct usb_desc_header *head,
				   uint8_t bInterfaceNumber)
{
	struct usb_if_descriptor *iface = (struct usb_if_descriptor *)head;
	struct cs_ac_if_descriptor *header;

	struct usb_association_descriptor *iad =
		(struct usb_association_descriptor *)
		((char *)iface - sizeof(struct usb_association_descriptor));
	iad->bFirstInterface = bInterfaceNumber;
	fix_fu_descriptors(iface);

	/* Audio Control Interface */
	iface->bInterfaceNumber = bInterfaceNumber;
	header = (struct cs_ac_if_descriptor *)
		 ((uint8_t *)iface + iface->bLength);
	header->baInterfaceNr[0] = bInterfaceNumber + 1;

	/* Audio Streaming Interface Passive */
	iface = (struct usb_if_descriptor *)
			  ((uint8_t *)header + header->wTotalLength);
	iface->bInterfaceNumber = bInterfaceNumber + 1;

	/* Audio Streaming Interface Active */
	iface = (struct usb_if_descriptor *)
			  ((uint8_t *)iface + iface->bLength);
	iface->bInterfaceNumber = bInterfaceNumber + 1;

	if (header->bInCollection == 2) {
		header->baInterfaceNr[1] = bInterfaceNumber + 2;
		/* Audio Streaming Interface Passive */
		iface = (struct usb_if_descriptor *)
			((uint8_t *)iface + USB_ACTIVE_IF_DESC_SIZE);
		iface->bInterfaceNumber = bInterfaceNumber + 2;

		/* Audio Streaming Interface Active */
		iface = (struct usb_if_descriptor *)
			((uint8_t *)iface + USB_PASSIVE_IF_DESC_SIZE);
		iface->bInterfaceNumber = bInterfaceNumber + 2;
	}
}

static void audio_cb_usb_status(struct usb_cfg_data *cfg,
			 enum usb_dc_status_code cb_status,
			 const uint8_t *param)
{
	struct usb_audio_dev_data *audio_dev_data;
	struct usb_dev_data *dev_data;

	dev_data = usb_get_dev_data_by_cfg(&usb_audio_data_devlist, cfg);

	if (dev_data == NULL) {
		LOG_ERR("Device data not found for cfg %p", cfg);
		return;
	}

	audio_dev_data = CONTAINER_OF(dev_data, struct usb_audio_dev_data,
				      common);

	switch (cb_status) {
	case USB_DC_SOF:
		audio_dc_sof(cfg, audio_dev_data);
		break;
	default:
		break;
	}
}

/**
 * @brief Helper function for checking if particular entity is a part of
 *	  the audio device.
 *
 * This function checks if given entity is a part of given audio device.
 * If so then true is returned and audio_dev_data is considered correct device
 * data.
 *
 * @note For now this function searches through feature units only. The
 *	 descriptors are known and are not using any other entity type.
 *	 If there is a need to add other units to audio function then this
 *	 must be reworked.
 *
 * @param [in]      audio_dev_data USB audio device data.
 * @param [in, out] entity	   USB Audio entity.
 *				   .id      [in]  id of searched entity
 *				   .subtype [out] subtype of entity (if found)
 *
 * @return true if entity matched audio_dev_data, false otherwise.
 */
static bool is_entity_valid(struct usb_audio_dev_data *audio_dev_data,
			    struct usb_audio_entity *entity)
{
	const struct cs_ac_if_descriptor *header;
	const struct feature_unit_descriptor *fu;

	header = audio_dev_data->desc_hdr;
	fu = (struct feature_unit_descriptor *)((uint8_t *)header +
						header->bLength +
						INPUT_TERMINAL_DESC_SIZE);
	if (fu->bUnitID == entity->id) {
		entity->subtype = fu->bDescriptorSubtype;
		return true;
	}

	if (header->bInCollection == 2) {
		fu = (struct feature_unit_descriptor *)((uint8_t *)fu +
			fu->bLength +
			INPUT_TERMINAL_DESC_SIZE +
			OUTPUT_TERMINAL_DESC_SIZE);
		if (fu->bUnitID == entity->id) {
			entity->subtype = fu->bDescriptorSubtype;
			return true;
		}
	}

	return false;
}

/**
 * @brief Helper function for getting the audio_dev_data by the entity number.
 *
 * This function searches through all audio devices the one with given
 * entity number and return the audio_dev_data structure for this entity.
 *
 * @param [in, out] entity USB Audio entity addressed by the request.
 *			   .id      [in]  id of searched entity
 *			   .subtype [out] subtype of entity (if found)
 *
 * @return audio_dev_data for given entity, NULL if not found.
 */
static struct usb_audio_dev_data *get_audio_dev_data_by_entity(
					struct usb_audio_entity *entity)
{
	struct usb_dev_data *dev_data;
	struct usb_audio_dev_data *audio_dev_data;

	SYS_SLIST_FOR_EACH_CONTAINER(&usb_audio_data_devlist, dev_data, node) {
		audio_dev_data = CONTAINER_OF(dev_data,
					      struct usb_audio_dev_data,
					      common);

		if (is_entity_valid(audio_dev_data, entity)) {
			return audio_dev_data;
		}
	}
	return NULL;
}

/**
 * @brief Helper function for checking if particular interface is a part of
 *	  the audio device.
 *
 * This function checks if given interface is a part of given audio device.
 * If so then true is returned and audio_dev_data is considered correct device
 * data.
 *
 * @param [in] audio_dev_data USB audio device data.
 * @param [in] interface      USB Audio interface number.
 *
 * @return true if interface matched audio_dev_data, false otherwise.
 */
static bool is_interface_valid(struct usb_audio_dev_data *audio_dev_data,
			       uint8_t interface)
{
	const struct cs_ac_if_descriptor *header;

	header = audio_dev_data->desc_hdr;
	uint8_t desc_iface = 0;

	for (size_t i = 0; i < header->bInCollection; i++) {
		desc_iface = header->baInterfaceNr[i];
		if (desc_iface == interface) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Helper function for getting the audio_dev_data by the interface
 *	  number.
 *
 * This function searches through all audio devices the one with given
 * interface number and returns the audio_dev_data structure for this device.
 *
 * @param [in] interface USB Audio interface addressed by the request.
 *
 * @return audio_dev_data for given interface, NULL if not found.
 */
static struct usb_audio_dev_data *get_audio_dev_data_by_iface(uint8_t interface)
{
	struct usb_dev_data *dev_data;
	struct usb_audio_dev_data *audio_dev_data;

	SYS_SLIST_FOR_EACH_CONTAINER(&usb_audio_data_devlist, dev_data, node) {
		audio_dev_data = CONTAINER_OF(dev_data,
					      struct usb_audio_dev_data,
					      common);

		if (is_interface_valid(audio_dev_data, interface)) {
			return audio_dev_data;
		}
	}
	return NULL;
}

/**
 * @brief Handler for feature unit mute control requests.
 *
 * This function handles feature unit mute control request.
 *
 * @param audio_dev_data USB audio device data.
 * @param setup          Information about the executed request.
 * @param len		 Size of the buffer.
 * @param data		 Buffer containing the request result.
 * @param evt		 Feature Unit Event info.
 * @param device	 Device part that has been addressed. Applicable for
 *			 bidirectional device.
 *
 * @return 0 if successful, negative errno otherwise.
 */
static int handle_fu_mute_req(struct usb_audio_dev_data *audio_dev_data,
			      struct usb_setup_packet *setup,
			      int32_t *len, uint8_t **data,
			      struct usb_audio_fu_evt *evt,
			      uint8_t device)
{
	uint8_t ch = (setup->wValue) & 0xFF;
	uint8_t ch_cnt = audio_dev_data->ch_cnt[device];
	uint8_t *controls = audio_dev_data->controls[device];
	uint8_t *control_val = &controls[POS(MUTE, ch, ch_cnt)];

	if (usb_reqtype_is_to_device(setup)) {
		/* Check if *len has valid value */
		if (*len != LEN(1, MUTE)) {
			return -EINVAL;
		}

		if (setup->bRequest == USB_AUDIO_SET_CUR) {
			evt->val = control_val;
			evt->val_len = *len;
			memcpy(control_val, *data, *len);
			return 0;
		}
	} else {
		if (setup->bRequest == USB_AUDIO_GET_CUR) {
			*data = control_val;
			*len = LEN(1, MUTE);
			return 0;
		}
	}

	return -EINVAL;
}


static int handle_fu_volume_req(struct usb_audio_dev_data *audio_dev_data,
				struct usb_setup_packet *setup, int32_t *len, uint8_t **data,
				struct usb_audio_fu_evt *evt, uint8_t device)
{
	uint8_t ch = (setup->wValue) & 0xFF;
	uint8_t ch_cnt = audio_dev_data->ch_cnt[device];
	uint8_t *controls = audio_dev_data->controls[device];
	uint8_t *control_val = &controls[POS(VOLUME, ch, ch_cnt)];
	int16_t target_vol = 0;
	int16_t temp_vol = 0;

	if (usb_reqtype_is_to_device(setup)) {
		/* Check if *len has valid value */
		if (*len != LEN(1, VOLUME)) {
			LOG_ERR("*len: %d, LEN(1, VOLUME): %d", *len, LEN(1, VOLUME));
			return -EINVAL;
		}
		if (setup->bRequest == USB_AUDIO_SET_CUR) {
			target_vol = *((int16_t *)*data);
			if (!IN_RANGE(target_vol, audio_dev_data->volumes.volume_min,
				      audio_dev_data->volumes.volume_max)) {
				LOG_ERR("Volume out of range: %d", target_vol);
				return -EINVAL;
			}
			if (target_vol % audio_dev_data->volumes.volume_res != 0) {
				target_vol = ROUND_UP(target_vol,
					audio_dev_data->volumes.volume_res);
			}
			evt->val = control_val;
			evt->val_len = *len;
			*((int16_t *)evt->val) = sys_le16_to_cpu(target_vol);
			return 0;
		}
	} else {
		if (setup->bRequest == USB_AUDIO_GET_CUR) {
			*len = LEN(ch_cnt, VOLUME);
			temp_vol = sys_cpu_to_le16(*(int16_t *)control_val);
			memcpy(*data, &temp_vol, *len);
			return 0;
		} else if (setup->bRequest == USB_AUDIO_GET_MIN) {
			*len = sizeof(audio_dev_data->volumes.volume_min);
			temp_vol = sys_cpu_to_le16(audio_dev_data->volumes.volume_min);
			memcpy(*data, &temp_vol, *len);
			return 0;
		} else if (setup->bRequest == USB_AUDIO_GET_MAX) {
			*len = sizeof(audio_dev_data->volumes.volume_max);
			temp_vol = sys_cpu_to_le16(audio_dev_data->volumes.volume_max);
			memcpy(*data, &temp_vol, *len);
			return 0;
		} else if (setup->bRequest == USB_AUDIO_GET_RES) {
			*len = sizeof(audio_dev_data->volumes.volume_res);
			temp_vol = sys_cpu_to_le16(audio_dev_data->volumes.volume_res);
			memcpy(*data, &temp_vol, *len);
			return 0;
		}
	}

	return -EINVAL;
}

/**
 * @brief Handler for feature unit requests.
 *
 * This function handles feature unit specific requests.
 * If request is properly served 0 is returned. Negative errno
 * is returned in case of an error. This leads to setting stall on IN EP0.
 *
 * @param audio_dev_data USB audio device data.
 * @param pSetup         Information about the executed request.
 * @param len            Size of the buffer.
 * @param data           Buffer containing the request result.
 *
 * @return 0 if successful, negative errno otherwise.
 */
static int handle_feature_unit_req(struct usb_audio_dev_data *audio_dev_data,
				   struct usb_setup_packet *pSetup,
				   int32_t *len, uint8_t **data)
{
	const struct feature_unit_descriptor *fu;
	struct usb_audio_fu_evt evt;
	enum usb_audio_fucs cs;
	uint8_t device;
	uint8_t fu_id;
	uint8_t ch_cnt;
	uint8_t ch;
	int ret;

	fu_id = ((pSetup->wIndex) >> 8) & 0xFF;
	fu = get_feature_unit(audio_dev_data, &device, fu_id);
	ch = (pSetup->wValue) & 0xFF;
	cs = ((pSetup->wValue) >> 8) & 0xFF;
	ch_cnt = audio_dev_data->ch_cnt[device];

	LOG_DBG("CS: %d, CN: %d, len: %d", cs, ch, *len);

	/* Error checking */
	if (!(BIT(cs) & (get_controls(fu) << 1))) {
		/* Feature not supported by this FU */
		return -EINVAL;
	} else if (ch >= ch_cnt) {
		/* Invalid ch */
		return -EINVAL;
	}

	switch (cs) {
	case USB_AUDIO_FU_MUTE_CONTROL:
		ret = handle_fu_mute_req(audio_dev_data, pSetup,
					 len, data, &evt, device);
		break;
	case USB_AUDIO_FU_VOLUME_CONTROL:
		ret = handle_fu_volume_req(audio_dev_data, pSetup,
					 len, data, &evt, device);
		break;
	default:
		return -ENOTSUP;
	}

	if (ret) {
		return ret;
	}

	/* Inform the app */
	if (audio_dev_data->ops && audio_dev_data->ops->feature_update_cb) {
		if (usb_reqtype_is_to_device(pSetup) &&
		    pSetup->bRequest == USB_AUDIO_SET_CUR) {
			evt.cs = cs;
			evt.channel = ch;
			evt.dir = get_fu_dir(fu);
			audio_dev_data->ops->feature_update_cb(
					audio_dev_data->common.dev, &evt);
		}
	}

	return 0;
}

/**
 * @brief Handler called for class specific interface request.
 *
 * This function handles all class specific interface requests to a usb audio
 * device. If request is properly server then 0 is returned. Returning negative
 * value will lead to set stall on IN EP0.
 *
 * @param pSetup    Information about the executed request.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
static int handle_interface_req(struct usb_setup_packet *pSetup,
				int32_t *len,
				uint8_t **data)
{
	struct usb_audio_dev_data *audio_dev_data;
	struct usb_audio_entity entity;

	/* parse wIndex for interface request */
	uint8_t entity_id = ((pSetup->wIndex) >> 8) & 0xFF;

	entity.id = entity_id;

	/** Normally there should be a call to usb_get_dev_data_by_iface()
	 * and addressed interface should be read from wIndex low byte.
	 *
	 * uint8_t interface = (pSetup->wIndex) & 0xFF;
	 *
	 * However, Linux is using special form of Audio Requests
	 * which always left wIndex low byte 0 no matter which device and
	 * entity is addressed. Because of that there is a need to obtain
	 * this information from the device descriptor using entity id.
	 */
	audio_dev_data = get_audio_dev_data_by_entity(&entity);

	if (audio_dev_data == NULL) {
		LOG_ERR("Device data not found for entity %u", entity.id);
		return -ENODEV;
	}

	switch (entity.subtype) {
	case USB_AUDIO_FEATURE_UNIT:
		return handle_feature_unit_req(audio_dev_data,
					       pSetup, len, data);
	default:
		LOG_INF("Currently not supported");
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Custom callback for USB Device requests.
 *
 * This callback is called when set/get interface request is directed
 * to the device. This is Zephyr way to address those requests.
 * It's not possible to do that in the core stack as common USB device
 * stack does not know the amount of devices that has alternate interfaces.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return 0 on success, positive value if request is intended to be handled
 *	   by the core USB stack. Negative error code on fail.
 */
static int audio_custom_handler(struct usb_setup_packet *pSetup, int32_t *len,
				uint8_t **data)
{
	const struct cs_ac_if_descriptor *header;
	struct usb_audio_dev_data *audio_dev_data;
	const struct usb_if_descriptor *if_desc;
	const struct usb_ep_descriptor *ep_desc;

	uint8_t iface = (pSetup->wIndex) & 0xFF;

	if (pSetup->RequestType.recipient != USB_REQTYPE_RECIPIENT_INTERFACE ||
	    usb_reqtype_is_to_host(pSetup)) {
		return -EINVAL;
	}

	audio_dev_data = get_audio_dev_data_by_iface(iface);
	if (audio_dev_data == NULL) {
		return -EINVAL;
	}

	/* Search for endpoint associated to addressed interface
	 * Endpoint is searched in order to know the direction of
	 * addressed interface.
	 */
	header = audio_dev_data->desc_hdr;

	/* Skip to the first interface */
	if_desc = (struct usb_if_descriptor *)((uint8_t *)header +
						header->wTotalLength +
						USB_PASSIVE_IF_DESC_SIZE);

	if (if_desc->bInterfaceNumber == iface) {
		ep_desc = (struct usb_ep_descriptor *)((uint8_t *)if_desc +
						USB_PASSIVE_IF_DESC_SIZE +
						USB_AC_CS_IF_DESC_SIZE +
						USB_FORMAT_TYPE_I_DESC_SIZE);
	} else {
		/* In case first interface address is not the one addressed
		 * we can be sure the second one is because
		 * get_audio_dev_data_by_iface() found the device. It
		 * must be the second interface associated with the device.
		 */
		if_desc = (struct usb_if_descriptor *)((uint8_t *)if_desc +
						USB_ACTIVE_IF_DESC_SIZE);
		ep_desc = (struct usb_ep_descriptor *)((uint8_t *)if_desc +
						USB_PASSIVE_IF_DESC_SIZE +
						USB_AC_CS_IF_DESC_SIZE +
						USB_FORMAT_TYPE_I_DESC_SIZE);
	}

	if (pSetup->bRequest == USB_SREQ_SET_INTERFACE) {
		if (ep_desc->bEndpointAddress & USB_EP_DIR_MASK) {
			audio_dev_data->tx_enable = pSetup->wValue;
		} else {
			audio_dev_data->rx_enable = pSetup->wValue;
		}
	}

	return -EINVAL;
}

/**
 * @brief Handler called for Class requests not handled by the USB stack.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
static int audio_class_handle_req(struct usb_setup_packet *pSetup,
				  int32_t *len, uint8_t **data)
{
	LOG_DBG("bmRT 0x%02x, bR 0x%02x, wV 0x%04x, wI 0x%04x, wL 0x%04x",
		pSetup->bmRequestType, pSetup->bRequest, pSetup->wValue,
		pSetup->wIndex, pSetup->wLength);

	switch (pSetup->RequestType.recipient) {
	case USB_REQTYPE_RECIPIENT_INTERFACE:
		return handle_interface_req(pSetup, len, data);
	default:
		LOG_ERR("Request recipient invalid");
		return -EINVAL;
	}
}

static int usb_audio_device_init(const struct device *dev)
{
	LOG_DBG("Init Audio Device: dev %p (%s)", dev, dev->name);

	return 0;
}

static void audio_write_cb(uint8_t ep, int size, void *priv)
{
	struct usb_dev_data *dev_data;
	struct usb_audio_dev_data *audio_dev_data;
	struct net_buf *buffer = priv;

	dev_data = usb_get_dev_data_by_ep(&usb_audio_data_devlist, ep);
	audio_dev_data = dev_data->dev->data;

	LOG_DBG("Written %d bytes on ep 0x%02x, *audio_dev_data %p",
		size, ep, audio_dev_data);

	/* Ask installed callback to process the data.
	 * User is responsible for freeing the buffer.
	 * In case no callback is installed free the buffer.
	 */
	if (audio_dev_data->ops && audio_dev_data->ops->data_written_cb) {
		audio_dev_data->ops->data_written_cb(dev_data->dev,
						     buffer, size);
	} else {
		/* Release net_buf back to the pool */
		net_buf_unref(buffer);
	}
}

int usb_audio_send(const struct device *dev, struct net_buf *buffer,
		   size_t len)
{
	struct usb_audio_dev_data *audio_dev_data = dev->data;
	struct usb_cfg_data *cfg = (void *)dev->config;
	/* EP ISO IN is always placed first in the endpoint table */
	uint8_t ep = cfg->endpoint[0].ep_addr;

	if (!(ep & USB_EP_DIR_MASK)) {
		LOG_ERR("Wrong device");
		return -EINVAL;
	}

	if (!audio_dev_data->tx_enable) {
		LOG_DBG("sending dropped -> Host chose passive interface");
		return -EAGAIN;
	}

	if (len > buffer->size) {
		LOG_ERR("Cannot send %d bytes, to much data", len);
		return -EINVAL;
	}

	/** buffer passed to *priv because completion callback
	 * needs to release it to the pool
	 */
	return usb_transfer(ep, buffer->data, len, USB_TRANS_WRITE | USB_TRANS_NO_ZLP,
		     audio_write_cb, buffer);
}

size_t usb_audio_get_in_frame_size(const struct device *dev)
{
	struct usb_audio_dev_data *audio_dev_data = dev->data;

	return audio_dev_data->in_frame_size;
}

static void audio_receive_cb(uint8_t ep, enum usb_dc_ep_cb_status_code status)
{
	struct usb_audio_dev_data *audio_dev_data;
	struct usb_dev_data *common;
	struct net_buf *buffer;
	int ret_bytes;
	int ret;

	__ASSERT(status == USB_DC_EP_DATA_OUT, "Invalid ep status");

	common = usb_get_dev_data_by_ep(&usb_audio_data_devlist, ep);
	if (common == NULL) {
		return;
	}

	audio_dev_data = CONTAINER_OF(common, struct usb_audio_dev_data,
				      common);

	/** Check if active audiostreaming interface is selected
	 * If no there is no point to read the data. Return from callback
	 */
	if (!audio_dev_data->rx_enable) {
		return;
	}

	/* Check if application installed callback and process the data.
	 * In case no callback is installed do not alloc the buffer at all.
	 */
	if (audio_dev_data->ops && audio_dev_data->ops->data_received_cb) {
		buffer = net_buf_alloc(audio_dev_data->pool, K_NO_WAIT);
		if (!buffer) {
			LOG_ERR("Failed to allocate data buffer");
			return;
		}

		ret = usb_read(ep, buffer->data, buffer->size, &ret_bytes);

		if (ret) {
			LOG_ERR("ret=%d ", ret);
			net_buf_unref(buffer);
			return;
		}

		if (!ret_bytes) {
			net_buf_unref(buffer);
			return;
		}
		audio_dev_data->ops->data_received_cb(common->dev,
						      buffer, ret_bytes);
	}
}

void usb_audio_register(const struct device *dev,
			const struct usb_audio_ops *ops)
{
	struct usb_audio_dev_data *audio_dev_data = dev->data;
	const struct usb_cfg_data *cfg = dev->config;
	const struct std_if_descriptor *iface_descr =
		cfg->interface_descriptor;
	const struct cs_ac_if_descriptor *header =
		(struct cs_ac_if_descriptor *)
		((uint8_t *)iface_descr + USB_PASSIVE_IF_DESC_SIZE);

	audio_dev_data->ops = ops;
	audio_dev_data->common.dev = dev;
	audio_dev_data->rx_enable = false;
	audio_dev_data->tx_enable = false;
	audio_dev_data->desc_hdr = header;

	sys_slist_append(&usb_audio_data_devlist, &audio_dev_data->common.node);

	LOG_DBG("Device dev %p dev_data %p cfg %p added to devlist %p",
		dev, audio_dev_data, dev->config,
		&usb_audio_data_devlist);
}

#define DEFINE_AUDIO_DEVICE(dev, i)					  \
	USBD_DEFINE_CFG_DATA(dev##_audio_config_##i) = {		  \
		.usb_device_description	= NULL,				  \
		.interface_config = audio_interface_config,		  \
		.interface_descriptor = &dev##_desc_##i.std_ac_interface, \
		.cb_usb_status = audio_cb_usb_status,			  \
		.interface = {						  \
			.class_handler = audio_class_handle_req,	  \
			.custom_handler = audio_custom_handler,		  \
			.vendor_handler = NULL,				  \
		},							  \
		.num_endpoints = ARRAY_SIZE(dev##_usb_audio_ep_data_##i), \
		.endpoint = dev##_usb_audio_ep_data_##i,		  \
	};								  \
	DEVICE_DT_DEFINE(DT_INST(i, COMPAT_##dev),			  \
			    &usb_audio_device_init,			  \
			    NULL,					  \
			    &dev##_audio_dev_data_##i,			  \
			    &dev##_audio_config_##i, POST_KERNEL,	  \
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		  \
			    DUMMY_API)

#define DEFINE_BUF_POOL(name, size) \
	NET_BUF_POOL_FIXED_DEFINE(name, 5, size, 4, net_buf_destroy)

#define UNIDIR_DEVICE(dev, i, out_pool, in_size, it_type, ot_type, cb, addr) \
	UTIL_EXPAND( \
	DEFINE_AUDIO_DEV_DATA(dev, i, out_pool, in_size); \
	DECLARE_DESCRIPTOR(dev, i, 1); \
	DEFINE_AUDIO_DESCRIPTOR(dev, i, dev##_ID(i), dev##_LINK(i), \
				it_type, ot_type, cb, addr); \
	DEFINE_AUDIO_DEVICE(dev, i))

#define HEADPHONES_DEVICE(i, dev) UTIL_EXPAND( \
	DEFINE_BUF_POOL(audio_data_pool_hp_##i, EP_SIZE(dev, i));  \
	UNIDIR_DEVICE(dev, i, &audio_data_pool_hp_##i, 0, \
		      USB_AUDIO_USB_STREAMING, USB_AUDIO_OUT_HEADPHONES, \
		      audio_receive_cb, AUTO_EP_OUT);)

#define MICROPHONE_DEVICE(i, dev) UTIL_EXPAND( \
	UNIDIR_DEVICE(dev, i, NULL, EP_SIZE(dev, i), \
		      USB_AUDIO_IN_MICROPHONE, USB_AUDIO_USB_STREAMING, \
		      usb_transfer_ep_callback, AUTO_EP_IN);)

#define HEADSET_DEVICE(i, dev) UTIL_EXPAND( \
	DEFINE_BUF_POOL(audio_data_pool_hs_##i, EP_SIZE(dev##_HP, i)); \
	DEFINE_AUDIO_DEV_DATA_BIDIR(dev, i, &audio_data_pool_hs_##i, \
				    EP_SIZE(dev##_MIC, i)); \
	DECLARE_DESCRIPTOR_BIDIR(dev, i, 2); \
	DEFINE_AUDIO_DESCRIPTOR_BIDIR(dev, i, dev##_ID(i)); \
	DEFINE_AUDIO_DEVICE(dev, i);)

LISTIFY(HEADPHONES_DEVICE_COUNT, HEADPHONES_DEVICE, (), HP)
LISTIFY(MICROPHONE_DEVICE_COUNT, MICROPHONE_DEVICE, (), MIC)
LISTIFY(HEADSET_DEVICE_COUNT, HEADSET_DEVICE, (), HS)
