/*
 * Copyright (c) 2025 Yoan Dumas
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include "board_rev.h"

LOG_MODULE_REGISTER(RPI_VC_IF_BOARD_REV, CONFIG_MBOX_LOG_LEVEL);

/**
 * Documentation on how to decode the board revision could be found here:
 * https://github.com/raspberrypi/documentation/blob/develop/documentation/asciidoc/computers/raspberry-pi/revision-codes.adoc
 */

const char *processors[] = {"BCM2835", "BCM2836", "BCM2837", "BCM2711"};

const char *rpi_types[] = {"1A",   "1B",  "1A+",  "1B+",    "2B",  "ALPHA", "CM1", "{7}",  "3B",
			   "Zero", "CM3", "{11}", "Zero W", "3B+", "3A+",   "-",   "CM3+", "4B"};

const char *rpi_memories[] = {"256MB", "512MB", "1GiB", "2GiB", "4GiB", "8GiB"};

const char *rpi_manufacturers[] = {"Sony UK",    "Egoman", "Embest",
				   "Sony Japan", "Embest", "Stadium"};

const char *rpi_models[] = {
	"-",
	"-",
	"RPI1B 1.0 256MB Egoman",
	"RPI1B 1.0 256MB Egoman",
	"RPI1B 2.0 256MB Sony UK",
	"RPI1B 2.0 256MB Qisda",
	"RPI1B 2.0 256MB Egoman",
	"RPI1A 2.0 256MB Egoman",
	"RPI1A 2.0 256MB Sony UK",
	"RPI1A 2.0 256MB Qisda",
	"RPI1B 2.0 512MB Egoman",
	"RPI1B 2.0 512MB Sony UK",
	"RPI1B 2.0 512MB Egoman",
	"RPI1B+ 1.2 512MB Sony UK",
	"CM1 1.0 512MB Sony UK",
	"RPI1A+ 1.1 256MB Sony UK",
	"RPI1B+ 1.2 512MB Embest",
	"CM1 1.0 512MB Embest",
	"RPI1A+ 1.1 256MB/512MB Embest",
};

void log_board_revision(uint32_t board_rev)
{
	if (board_rev & (1 << 23)) {
		/* New style revision code */
		LOG_INF("Model: rpi-%s, processor: %s, memory: %s, manufacturer: %s",
			rpi_types[(board_rev & (0xFF << 4)) >> 4],
			processors[(board_rev & (0xF << 12)) >> 12],
			rpi_memories[(board_rev & (0x7 << 20)) >> 20],
			rpi_manufacturers[(board_rev & (0xF << 16)) >> 16]);
	} else {
		/* old style revision code */
		LOG_INF("%s", rpi_models[board_rev]);
	}
}
