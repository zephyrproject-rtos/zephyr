/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>

/* Maximum supported AD data length, use a value supported by the Controller,
 * Bluetooth Core Specification define minimum of 31 bytes will be supported by
 * all Controllers, can be a maximum of 1650 bytes when supported.
 */
#if defined(CONFIG_BT_CTLR_ADV_DATA_LEN_MAX)
#define BT_AD_DATA_LEN_MAX CONFIG_BT_CTLR_ADV_DATA_LEN_MAX
#else
#define BT_AD_DATA_LEN_MAX 31U
#endif

/* Size of AD data format length field in octets */
#define BT_AD_DATA_FORMAT_LEN_SIZE 1U

/* Size of AD data format type field in octets */
#define BT_AD_DATA_FORMAT_TYPE_SIZE 1U

/* Maximum value of AD data format length field (8-bit) */
#define BT_AD_DATA_FORMAT_LEN_MAX 255U

/* Device name length, size minus one null character */
#define BT_DEVICE_NAME_LEN (sizeof(CONFIG_BT_DEVICE_NAME) - 1U)

/* Device name length in AD data format, 2 bytes for length and type overhead */
#define BT_DEVICE_NAME_AD_DATA_LEN (BT_AD_DATA_FORMAT_LEN_SIZE + \
				    BT_AD_DATA_FORMAT_TYPE_SIZE + \
				    BT_DEVICE_NAME_LEN)

/* Maximum manufacturer data length, considering ad data format overhead and
 * the included device name in ad data format.
 */
#define BT_MFG_DATA_LEN_MAX       (MIN((BT_AD_DATA_FORMAT_LEN_MAX - \
					BT_AD_DATA_FORMAT_TYPE_SIZE), \
				       (BT_AD_DATA_LEN_MAX - \
					BT_AD_DATA_FORMAT_LEN_SIZE - \
					BT_AD_DATA_FORMAT_TYPE_SIZE)))
#define BT_MFG_DATA_LEN           (MIN(BT_MFG_DATA_LEN_MAX, \
				       (BT_AD_DATA_LEN_MAX - \
					BT_AD_DATA_FORMAT_LEN_SIZE - \
					BT_AD_DATA_FORMAT_TYPE_SIZE - \
					BT_DEVICE_NAME_AD_DATA_LEN)))
/*
 * Datalength is an integer, so BT_MFG_DATA_LEN can not be larger than 255.
 * To ensure that we need to chain PDUs we therefore add manufacturer data
 * twice when chaining is enabled
 */
static uint8_t mfg_data[BT_MFG_DATA_LEN] = { 0xFF, 0xFF, };

static const struct bt_data ad_long[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
#if CONFIG_BT_CTLR_ADV_DATA_LEN_MAX > 255
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, sizeof(mfg_data)),
#endif
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data ad_short[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static struct bt_le_ext_adv *adv[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

int broadcaster_multiple(void)
{
	int err;

	/* Create and start Advertising Sets */
	for (int index = 0; index < CONFIG_BT_EXT_ADV_MAX_ADV_SET; index++) {
		struct bt_le_adv_param adv_param = {
			.id = BT_ID_DEFAULT,
			.sid = 0U, /* Supply unique SID when creating advertising set */
			.secondary_max_skip = 0U,
			.options = BT_LE_ADV_OPT_EXT_ADV,
			.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
			.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
			.peer = NULL,
		};
		const struct adv_param_config {
			uint32_t options;
			const struct bt_data *ad;
			size_t ad_size;
		} param_config[] = {
		{ /* Use 1M legacy PDU */
			.options = BT_LE_ADV_OPT_NONE,
			.ad = ad_short,
			.ad_size = ARRAY_SIZE(ad_short),
		},
		{ /* Use 2M auxiliary PDU */
			.options = BT_LE_ADV_OPT_EXT_ADV,
			.ad = ad_long,
			.ad_size = ARRAY_SIZE(ad_long),
		},
		{ /* Use 1M auxiliary PDU */
			.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_NO_2M,
			.ad = ad_long,
			.ad_size = ARRAY_SIZE(ad_long),
		},
		{ /* Use Coded PHY */
			.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CODED,
			.ad = ad_long,
			.ad_size = ARRAY_SIZE(ad_long),
		}};
		const struct bt_data *ad;
		size_t ad_size;

		/* Use advertising set instance index as SID */
		adv_param.sid = index;

		/* Advertising set options, AD and AD array size */
		const struct adv_param_config *config =
			&param_config[index % ARRAY_SIZE(param_config)];

		adv_param.options = config->options;
		ad = config->ad;
		ad_size = config->ad_size;

ext_adv_create_retry:
		/* Create a non-connectable advertising set */
		err = bt_le_ext_adv_create(&adv_param, NULL, &adv[index]);
		if (err) {
			/* Failed creating Coded PHY advertising set? */
			if ((adv_param.options & BT_LE_ADV_OPT_CODED) != 0U) {
				printk("Failed to create advertising set %d with Coded PHY "
				       "(err %d), retry without...\n", index, err);

				/* Retry with non-Coded PHY advertising set */
				adv_param.options &= ~BT_LE_ADV_OPT_CODED;

				goto ext_adv_create_retry;
			}

			printk("Failed to create advertising set %d (err %d)\n", index, err);
			return err;
		}

		/* Set extended advertising data */
		err = bt_le_ext_adv_set_data(adv[index], ad, ad_size, NULL, 0);
		if (err) {
			printk("Failed to set advertising data for set %d "
			       "(err %d)\n", index, err);
			return err;
		}

		/* Start extended advertising set */
		err = bt_le_ext_adv_start(adv[index],
					  BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			printk("Failed to start extended advertising set %d "
			       "(err %d)\n", index, err);
			return err;
		}

		printk("Started Extended Advertising Set %d.\n", index);
	}

	return 0;
}
