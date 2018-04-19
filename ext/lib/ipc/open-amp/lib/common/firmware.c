/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       firmware.c
 *
 * COMPONENT
 *
 *         OpenAMP stack.
 *
 * DESCRIPTION
 *
 *
 **************************************************************************/

#include <string.h>
#include <openamp/firmware.h>

/**
 * config_get_firmware
 *
 * Searches the given firmware in firmware table list and provides
 * it to caller.
 *
 * @param fw_name    - name of the firmware
 * @param start_addr - pointer t hold start address of firmware
 * @param size       - pointer to hold size of firmware
 *
 * returns -  status of function execution
 *
 */

extern struct firmware_info fw_table[];
extern int fw_table_size;

int config_get_firmware(char *fw_name, uintptr_t *start_addr,
			unsigned int *size)
{
	int idx;
	for (idx = 0; idx < fw_table_size; idx++) {
		if (!strncmp((char *)fw_table[idx].name, fw_name, sizeof(fw_table[idx].name))) {
			*start_addr = fw_table[idx].start_addr;
			*size =
			    fw_table[idx].end_addr - fw_table[idx].start_addr +
			    1;
			return 0;
		}
	}
	return -1;
}
