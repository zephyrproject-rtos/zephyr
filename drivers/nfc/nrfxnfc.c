/*
 * Copyright (c) 2023 Sendrato
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/device.h>

#include <zephyr/nfc/nfc_tag.h>

#if CONFIG_NFC_T2T_NRFXLIB
#include <nfc_t2t_lib.h>
#endif
#if CONFIG_NFC_T4T_NRFXLIB
#include <nfc_t4t_lib.h>
#endif

/******************* DEVICE STRUCTURE *******************/

struct nrfxnfc_data {
	const struct device *parent;
	const struct device *dev;
	struct k_work worker_irq;
	/* NFC subsys data */
	nfc_tag_cb_t nfc_tag_cb;
	enum nfc_tag_type tag_type;
};

static uint8_t _nrfxnfc_payload[CONFIG_NFC_NRFX_MAX_PAYLOAD_SIZE];

/******************* NFC SUBSYS DRIVER IMPLEMENTATION *******************/

#if CONFIG_NFC_T2T_NRFXLIB
static void nrfxnfc_t2t_cb(void *context, nfc_t2t_event_t event, const uint8_t *data,
			   size_t data_length)
{
	const struct device *dev = (const struct device *)context;
	struct nrfxnfc_data *dev_data = dev->data;
	enum nfc_tag_event nfc_event = NFC_TAG_EVENT_NONE;

	switch (event) {
	case NFC_T2T_EVENT_NONE:
		nfc_event = NFC_TAG_EVENT_NONE;
		break;

	case NFC_T2T_EVENT_FIELD_ON:
		nfc_event = NFC_TAG_EVENT_FIELD_ON;
		break;

	case NFC_T2T_EVENT_FIELD_OFF:
		nfc_event = NFC_TAG_EVENT_FIELD_OFF;
		break;

	case NFC_T2T_EVENT_DATA_READ:
		nfc_event = NFC_TAG_EVENT_READ_DONE;
		break;

	case NFC_T2T_EVENT_STOPPED:
		nfc_event = NFC_TAG_EVENT_STOPPED;
		break;
	}

	if (dev_data->nfc_tag_cb) {
		dev_data->nfc_tag_cb(dev, nfc_event, NULL, 0);
	}
}
#endif /* CONFIG_NFC_T2T_NRFXLIB */

#if CONFIG_NFC_T4T_NRFXLIB
static void nrfxnfc_t4t_cb(void *context,
			   nfc_t4t_event_t event,
			   const uint8_t *data,
			   size_t data_length,
			   uint32_t flags
{
	const struct device *dev = (const struct device *)context;
	struct nrfxnfc_data *dev_data = dev->data;
	enum nfc_tag_event nfc_event = NFC_TAG_EVENT_NONE;

	switch (event) {
	case NFC_T4T_EVENT_NONE:
		nfc_event = NFC_TAG_EVENT_NONE;
		break;

	case NFC_T4T_EVENT_FIELD_ON:
		nfc_event = NFC_TAG_EVENT_FIELD_ON;
		break;

	case NFC_T4T_EVENT_FIELD_OFF:
		nfc_event = NFC_TAG_EVENT_FIELD_OFF;
		break;

	case NFC_T4T_EVENT_NDEF_READ:
		nfc_event = NFC_TAG_EVENT_READ_DONE;
		break;

	case NFC_T4T_EVENT_NDEF_UPDATED:
		nfc_event = NFC_TAG_EVENT_WRITE_DONE;
		break;

	case NFC_T4T_EVENT_DATA_TRANSMITTED:
		nfc_event = NFC_TAG_EVENT_DATA_TRANSMITTED;
		break;

	case NFC_T4T_EVENT_DATA_IND:
		if (flags & NFC_T4T_DI_FLAG_MORE) {
			nfc_event = NFC_TAG_EVENT_DATA_IND;
		} else {
			nfc_event = NFC_TAG_EVENT_DATA_IND_DONE;
		}
		break;
	}

	if (dev_data->nfc_tag_cb) {
		dev_data->nfc_tag_cb(dev, nfc_event, NULL, 0);
	}
}
#endif /* CONFIG_NFC_T4T_NRFXLIB */


static int nrfxnfc_tag_init(const struct device *dev, nfc_tag_cb_t cb)
{
	struct nrfxnfc_data *data = dev->data;

	/* Setup callback */
	data->nfc_tag_cb = cb;

	return 0;
}


static int nrfxnfc_tag_set_type(const struct device *dev, enum nfc_tag_type type)
{
	int rv = 0;
	struct nrfxnfc_data *data = dev->data;

	if ((type != NFC_TAG_TYPE_T2T) && (type != NFC_TAG_TYPE_T4T)) {
		return -ENOTSUP;
	}

#if CONFIG_NFC_T2T_NRFXLIB
	if (type == NFC_TAG_TYPE_T2T) {
		rv = nfc_t2t_setup(nrfxnfc_t2t_cb, (void *)dev);
		if (rv != 0) {
			return rv;
		}
	}
#endif

#if CONFIG_NFC_T4T_NRFXLIB
	if (type == NFC_TAG_TYPE_T4T) {
		rv = nfc_t4t_setup(nrfxnfc_t4t_cb, (void *)dev);
		if (rv != 0) {
			return rv;
		}
	}
#endif

	/* Type is set */
	data->tag_type = type;
	return 0;
}


static int nrfxnfc_tag_get_type(const struct device *dev,
				enum nfc_tag_type *type)
{
	struct nrfxnfc_data *data = dev->data;
	*type = data->tag_type;
	return 0;
}


static int nrfxnfc_tag_start(const struct device *dev)
{
	struct nrfxnfc_data *data = dev->data;

#if CONFIG_NFC_T2T_NRFXLIB
	if (data->tag_type == NFC_TAG_TYPE_T2T) {
		return nfc_t2t_emulation_start();
	}
#endif

#if CONFIG_NFC_T4T_NRFXLIB
	if (data->tag_type == NFC_TAG_TYPE_T4T) {
		return nfc_t4t_emulation_start();
	}
#endif

	return -ENODEV;
}


static int nrfxnfc_tag_stop(const struct device *dev)
{
	struct nrfxnfc_data *data = dev->data;

#if CONFIG_NFC_T2T_NRFXLIB
	if (data->tag_type == NFC_TAG_TYPE_T2T) {
		return nfc_t2t_emulation_stop();
	}
#endif

#if CONFIG_NFC_T4T_NRFXLIB
	if (data->tag_type == NFC_TAG_TYPE_T4T) {
		return nfc_t4t_emulation_stop();
	}
#endif

	return -ENODEV;
}


static int nrfxnfc_tag_set_ndef(const struct device *dev,
				uint8_t *buf, uint16_t len)
{
	struct nrfxnfc_data *data = dev->data;

	if (len > CONFIG_NFC_NRFX_MAX_PAYLOAD_SIZE) {
		return -ENOMEM;
	}

	/* copy buf to ensure longevity and validity of data */
	(void)memset(_nrfxnfc_payload, 0, CONFIG_NFC_NRFX_MAX_PAYLOAD_SIZE);
	(void)memcpy(_nrfxnfc_payload, buf, len);

#if CONFIG_NFC_T2T_NRFXLIB
	if (data->tag_type == NFC_TAG_TYPE_T2T) {
		return nfc_t2t_payload_set(_nrfxnfc_payload, len);
	}
#endif

#if CONFIG_NFC_T4T_NRFXLIB
	if (data->tag_type == NFC_TAG_TYPE_T4T) {
		return nfc_t4t_ndef_rwpayload_set(_nrfxnfc_payload, len);
	}
#endif

	return -ENODEV;
}


static int nrfxnfc_tag_cmd(const struct device *dev,
			   enum nfc_tag_cmd cmd,
			   uint8_t *buf, uint16_t *buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cmd);
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);
	return 0;
}


static struct nfc_tag_driver_api _nrfxnfc_driver_api = {
	.init       = nrfxnfc_tag_init,
	.set_type   = nrfxnfc_tag_set_type,
	.get_type   = nrfxnfc_tag_get_type,
	.start      = nrfxnfc_tag_start,
	.stop       = nrfxnfc_tag_stop,
	.set_ndef   = nrfxnfc_tag_set_ndef,
	.cmd        = nrfxnfc_tag_cmd,
};


/******************* DEVICE INITIALISATION BY DTS *******************/

static struct nrfxnfc_data _nrfxnfc_driver_data = {0};
static char _nrfxnfc_driver_name[] = CONFIG_NFC_NRFX_DRV_NAME;


static int _nrfxnfc_init(const struct device *dev)
{
	return 0;
}

DEVICE_DEFINE(nrfxnfc,
		_nrfxnfc_driver_name,
		_nrfxnfc_init,
		NULL,
		&_nrfxnfc_driver_data,
		NULL, POST_KERNEL,
		CONFIG_NFC_NRFX_INIT_PRIORITY,
		&_nrfxnfc_driver_api);
