/*
 * Copyright (c) 2025 Yoan Dumas
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mbox.h>
#include <zephyr/logging/log.h>
#include "videocore_if.h"

LOG_MODULE_REGISTER(BCM2711_VC_IF, CONFIG_MBOX_LOG_LEVEL);

extern volatile uint32_t *get_mbox_buffer(const struct device *dev);
extern uint32_t bcm2711_mbox_read(const struct device *dev, mbox_channel_id_t channel);

#define MBOX_BCM2711_REQUEST_SUCCESSFUL 0x80000000
#define MBOX_BCM2711_ERROR_RESPONSE     0x80000001
#define MBOX_BCM2711_PROCESS_REQUEST    0x00000000

/**
 * @brief An enum of the RPI->Videocore firmware mailbox property interface.
 * Further details are available from
 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */
typedef enum {
	TAG_END = 0x00000000,

	/* Videocore */
	TAG_GET_FIRMWARE_VERSION,

	/* Hardware */
	TAG_GET_BOARD_REVISION = 0x10002,
	TAG_GET_BOARD_MAC_ADDRESS,
	TAG_GET_BOARD_SERIAL,
	TAG_GET_ARM_MEMORY,
	TAG_GET_VC_MEMORY,
	TAG_GET_CLOCKS,

	/* Config */
	TAG_GET_COMMAND_LINE = 0x50001,

	/* Shared resource management */
	TAG_GET_DMA_CHANNELS = 0x60001,

	/* Power */
	TAG_GET_POWER_STATE = 0x20001,
	TAG_GET_TIMING,
	TAG_SET_POWER_STATE = 0x28001,

	/* Clocks */
	TAG_GET_CLOCK_STATE = 0x30001,
	TAG_SET_CLOCK_STATE = 0x38001,
	TAG_GET_CLOCK_RATE = 0x30002,
	TAG_SET_CLOCK_RATE = 0x38002,
	TAG_GET_MAX_CLOCK_RATE = 0x30004,
	TAG_GET_MIN_CLOCK_RATE = 0x30007,
	TAG_GET_TURBO = 0x30009,
	TAG_SET_TURBO = 0x38009,

	/* Voltage */
	TAG_GET_VOLTAGE = 0x30003,
	TAG_SET_VOLTAGE = 0x38003,
	TAG_GET_MAX_VOLTAGE = 0x30005,
	TAG_GET_MIN_VOLTAGE = 0x30008,
	TAG_GET_TEMPERATURE = 0x30006,
	TAG_GET_MAX_TEMPERATURE = 0x3000A,
	TAG_ALLOCATE_MEMORY = 0x3000C,
	TAG_LOCK_MEMORY = 0x3000D,
	TAG_UNLOCK_MEMORY = 0x3000E,
	TAG_RELEASE_MEMORY = 0x3000F,
	TAG_EXECUTE_CODE = 0x30010,
	TAG_GET_DISPMANX_MEM_HANDLE = 0x30014,
	TAG_GET_EDID_BLOCK = 0x30020,

	/* Framebuffer */
	TAG_ALLOCATE_BUFFER = 0x40001,
	TAG_RELEASE_BUFFER = 0x48001,
	TAG_BLANK_SCREEN = 0x40002,
	TAG_GET_PHYSICAL_SIZE = 0x40003,
	TAG_TEST_PHYSICAL_SIZE = 0x44003,
	TAG_SET_PHYSICAL_SIZE = 0x48003,
	TAG_GET_VIRTUAL_SIZE = 0x40004,
	TAG_TEST_VIRTUAL_SIZE = 0x44004,
	TAG_SET_VIRTUAL_SIZE = 0x48004,
	TAG_GET_DEPTH = 0x40005,
	TAG_TEST_DEPTH = 0x44005,
	TAG_SET_DEPTH = 0x48005,
	TAG_GET_PIXEL_ORDER = 0x40006,
	TAG_TEST_PIXEL_ORDER = 0x44006,
	TAG_SET_PIXEL_ORDER = 0x48006,
	TAG_GET_ALPHA_MODE = 0x40007,
	TAG_TEST_ALPHA_MODE = 0x44007,
	TAG_SET_ALPHA_MODE = 0x48007,
	TAG_GET_PITCH = 0x40008,
	TAG_GET_VIRTUAL_OFFSET = 0x40009,
	TAG_TEST_VIRTUAL_OFFSET = 0x44009,
	TAG_SET_VIRTUAL_OFFSET = 0x48009,
	TAG_GET_OVERSCAN = 0x4000A,
	TAG_TEST_OVERSCAN = 0x4400A,
	TAG_SET_OVERSCAN = 0x4800A,
	TAG_GET_PALETTE = 0x4000B,
	TAG_TEST_PALETTE = 0x4400B,
	TAG_SET_PALETTE = 0x4800B,
	TAG_SET_CURSOR_INFO = 0x8011,
	TAG_SET_CURSOR_STATE = 0x8010
} bcm2711_mailbox_tag_t;

#define BOARD_REVISION_DATA_SIZE   4
#define BOARD_REVISION_BUFFER_SIZE (7 * sizeof(uint32_t))
#define FIRMWARE_VERSION_DATA_SIZE 4
#define FIRMWARE_VERSION_BUFFER_SIZE (7 * sizeof(uint32_t))

static struct mbox_dt_spec properties_channel = MBOX_DT_SPEC_GET(DT_ALIAS(mbox0), vc);

uint32_t get_vc_firmware_version(void)
{
	struct mbox_msg msg;
	int answer_addr;
	uint32_t *answer;
	volatile uint32_t *mbox_buffer = get_mbox_buffer(DEVICE_DT_GET(DT_ALIAS(mbox0)));

	mbox_buffer[0] = FIRMWARE_VERSION_BUFFER_SIZE; /* size of the message */
	mbox_buffer[1] = MBOX_BCM2711_PROCESS_REQUEST;
	mbox_buffer[2] = TAG_GET_FIRMWARE_VERSION;
	mbox_buffer[3] = FIRMWARE_VERSION_DATA_SIZE; /* size of the data in bytes */
	mbox_buffer[4] = MBOX_BCM2711_PROCESS_REQUEST;
	mbox_buffer[5] = 0; /* Slot for answer */
	mbox_buffer[6] = TAG_END;

	msg.data = (void *)mbox_buffer;
	msg.size = 4;
	mbox_send_dt(&properties_channel, &msg);

	answer_addr = bcm2711_mbox_read(DEVICE_DT_GET(DT_MBOX_CTLR_BY_NAME(DT_ALIAS(mbox0), vc)),
					DT_MBOX_CHANNEL_BY_NAME(DT_ALIAS(mbox0), vc));
	answer = (uint32_t *)((uintptr_t)answer_addr);
	if (answer[1] != MBOX_BCM2711_REQUEST_SUCCESSFUL) {
		LOG_ERR("Error: 0x%08X", answer[1]);
		if (answer[1] == MBOX_BCM2711_ERROR_RESPONSE) {
			LOG_ERR("Error parsing request buffer (partial response)\n");
		}
		return 0;
	}
	if (answer[2] != TAG_GET_FIRMWARE_VERSION) {
		LOG_ERR("Error, we didn't get an answer for firmware version, but: 0x%08X\n",
			answer[2]);
		return 0;
	}
	if (answer[3] != FIRMWARE_VERSION_DATA_SIZE) {
		LOG_ERR("Error, wrong firmware version size: 0x%08X (expected: 0x%08X)\n",
			answer[3], FIRMWARE_VERSION_DATA_SIZE);
		return 0;
	}
	if ((answer[4] & MBOX_BCM2711_REQUEST_SUCCESSFUL) == 0) {
		LOG_ERR("We didn't get an answer !\n");
		return 0;
	}
	return answer[5];
}

uint32_t get_board_revision(void)
{
	uint32_t *answer;
	struct mbox_msg msg;
	int answer_addr;
	volatile uint32_t *mbox_buffer = get_mbox_buffer(DEVICE_DT_GET(DT_ALIAS(mbox0)));

	mbox_buffer[0] = BOARD_REVISION_BUFFER_SIZE; /* size of the message */
	mbox_buffer[1] = MBOX_BCM2711_PROCESS_REQUEST;
	mbox_buffer[2] = TAG_GET_BOARD_REVISION;
	mbox_buffer[3] = BOARD_REVISION_DATA_SIZE; /* size of the data in bytes */
	mbox_buffer[4] = MBOX_BCM2711_PROCESS_REQUEST;
	mbox_buffer[5] = 0; /* Slot for answer */
	mbox_buffer[6] = TAG_END;

	msg.data = (void *)mbox_buffer;
	msg.size = 4;
	mbox_send_dt(&properties_channel, &msg);

	answer_addr = bcm2711_mbox_read(DEVICE_DT_GET(DT_MBOX_CTLR_BY_NAME(DT_ALIAS(mbox0), vc)),
					DT_MBOX_CHANNEL_BY_NAME(DT_ALIAS(mbox0), vc));
	answer = (uint32_t *)((uintptr_t)answer_addr);
	if (answer[1] != MBOX_BCM2711_REQUEST_SUCCESSFUL) {
		LOG_ERR("Error: 0x%08X", answer[1]);
		if (answer[1] == MBOX_BCM2711_ERROR_RESPONSE) {
			LOG_ERR("Error parsing request buffer (partial response)\n");
		}
		return 0;
	}
	if (answer[2] != TAG_GET_BOARD_REVISION) {
		LOG_ERR("Error, we didn't get an answer for the board revision, but: 0x%08X\n",
			answer[2]);
		return 0;
	}
	if (answer[3] != BOARD_REVISION_DATA_SIZE) {
		LOG_ERR("Error, wrong board revision size: 0x%08X (expected: 0x%08X)\n", answer[3],
			BOARD_REVISION_DATA_SIZE);
		return 0;
	}
	if ((answer[4] & MBOX_BCM2711_REQUEST_SUCCESSFUL) == 0) {
		LOG_ERR("We didn't get an answer !\n");
		return 0;
	}
	return answer[5];
}
