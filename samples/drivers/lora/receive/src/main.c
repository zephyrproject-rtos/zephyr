/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * Modifications (c) 2021 Dean Weiten
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/lora.h>
#include <errno.h>
#include <sys/util.h>
#include <zephyr.h>
#include <ctype.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");
#define DEFAULT_RADIO DT_LABEL(DEFAULT_RADIO_NODE)

#define MAX_DATA_LEN	(255)

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lora_receive);

/* 1000 msec = 1 sec */
#define FAIL_SLEEP_TIME_MS	(10)


void main(void)
{
	const struct device *LoraDevice;
	struct lora_modem_config LoraConfig;
	int     IntRetVal;
	int     DataLength;
	uint8_t RawData[MAX_DATA_LEN+10] = {0};
	uint8_t SafeData[MAX_DATA_LEN+10];
	int16_t rssi;
	int8_t  snr;
	int		MessageCounter;
	int		CharCtr;


	printk("\f\n");
	printk("Simple LoRa receive test for board config \"%s\"\n\n", CONFIG_BOARD);

	/* Connect to the LoRa radio */
	LoraDevice = device_get_binding(DEFAULT_RADIO);
	if (!LoraDevice) {
		LOG_ERR("%s Device not found", DEFAULT_RADIO);
		return;
	}

	/*
	 * LoRa configuration settings.
	 *
	 * Generally, we Will only be able to communicate with
	 * other devices with the same frequency, bandwidth and
	 * datarate settings.  In fact, systems with different
	 * bandwidth and datarate are more or less "orthoganal"
	 * to each other, meaning that one system will see the
	 * other system's signal as uncorrelated noise.
	 */
	LoraConfig.frequency = 915000000;	/* In Hz */
	LoraConfig.bandwidth = BW_250_KHZ;	/* Driver can interpret valid Hz or enum */
	LoraConfig.datarate = SF_10;
	LoraConfig.preamble_len = 8;
	LoraConfig.coding_rate = CR_4_5;
	LoraConfig.tx_power = 14;
	LoraConfig.tx = false;

	/* Print LoRa settings in use */
	printk("Settings:\n");
	printk("  Frequency                      = %9d (Hz)\n", LoraConfig.frequency);
	printk("  Bandwidth                      = %9d (Hz or enum)\n", LoraConfig.bandwidth);
	printk("  DataRate (spreading factor/SF) = %9d\n", LoraConfig.datarate);
	printk("  PreambleLen                    = %9d\n", LoraConfig.preamble_len);
	printk("  CodingRate                     = %9d\n", LoraConfig.coding_rate);
	printk("  TxPower                        = %9d\n", LoraConfig.tx_power);
	printk("  Tx                             = %9d\n", LoraConfig.tx);
	printk("\n");

	/* Set the configuration of the LoRa receiver */
	IntRetVal = lora_config(LoraDevice, &LoraConfig);
	if (IntRetVal < 0) {
		LOG_ERR("LoRa config failed");
		return;
	}

	/* Listen for LoRa messages forever. */
	MessageCounter = 0;
	while (1) {
		/* Block until data arrives */
		DataLength = lora_recv(LoraDevice, RawData, MAX_DATA_LEN, K_FOREVER,
				&rssi, &snr);
		if (DataLength < 0) {
			/* If receive error, log the error, wait a short time, then try again. */
			LOG_ERR("LoRa receive failed");
			k_msleep(FAIL_SLEEP_TIME_MS);
		} else {
			/* If packet received, count it, then print it. */

			/* Counter can probably do more, but we'll wrap at 65,536 */
			MessageCounter++;
			if  (MessageCounter > 0xFFFF)
				MessageCounter = 0;

			/*
			 * Changed from LOG_INF to give more flexibility in printout.
			 *
			 * LOG_INF("Received data 0x%4.4X: %s (RSSI:%ddBm, SNR:%ddBm)",
			 *	MessageCounter, log_strdup(SafeData), rssi, snr);
			 */

			/* Some data received is not string-safe, so sanitize string for printing */
			for (CharCtr = 0;
					((CharCtr < DataLength) && (CharCtr < MAX_DATA_LEN));
						CharCtr++) {
				if  (isprint(RawData[CharCtr])) {
					SafeData[CharCtr] = RawData[CharCtr];
				} else {
					SafeData[CharCtr] = '.';
				}
			}
			SafeData[CharCtr] = '\0';

			/* Print statistics */
			printk("\n");
			printk("Msg 0x%4.4X of length %3d received (RSSI:%ddBm, SNR:%ddBm):\n",
					MessageCounter, DataLength, rssi, snr);

			/* Print sanitized string */
			printk("As string: %s\n", SafeData);

			/* Print hex dump of byte data */
			printk("As hex bytes:");
			for (CharCtr = 0;
					((CharCtr < DataLength) && (CharCtr < MAX_DATA_LEN));
						CharCtr++) {
				switch (CharCtr & 0x0F) {
					case  0:	/* A line is 16 bytes.  Put "address" within packet at beginning of line */
						printk("\n  0x%4.4X - ", CharCtr);
						break;

					case  4:	/* Small visual cue at the quarter line marks - 4th and 12th byte */
					case 12:
						printk(" ");
						break;

					case  8:	/* Larger visual cue at the half line mark - 8th byte */
						printk("- ");
						break;
				}
				printk("%2.2X ", RawData[CharCtr]);
			}

			/* Blank space before next message */
			printk("\n\n");
		}
	}
}
