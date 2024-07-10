/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/flash.h>
#include <scu.h>

#define ATTR_INF	"\x1b[32;1m" // ANSI_COLOR_GREEN
#define ATTR_ERR	"\x1b[31;1m" // ANSI_COLOR_RED
#define ATTR_RST	"\x1b[37;1m" // ANSI_COLOR_RESET

#define FLASH_RW_SIZE 255

void Flash_Test(const struct device *flash, uint32_t FLASH_ADDR, uint8_t EVEN_ODD_MUL, uint8_t FORMATTER) {
	int errorcode = 0;
	uint8_t flash_data_write[FLASH_RW_SIZE] = {0};
	uint8_t flash_data_read[FLASH_RW_SIZE] = {0};
	errorcode = flash_read(flash, FLASH_ADDR, flash_data_read, FLASH_RW_SIZE);
	if(errorcode < 0) {
		printf("%s Error reading from flash with code:%d%s\n", ATTR_ERR, errorcode,ATTR_RST);
	} 
	else {
		printf("%s Reading Back Before Erasing Flash%s\n", ATTR_INF,ATTR_RST);
		for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
			printf("%s %d%s", ATTR_INF, flash_data_read[i],i%FORMATTER==0?"\n":"");
		} printf("\n");
	}
	errorcode = flash_erase(flash, FLASH_ADDR, 0x1000);
	if(errorcode < 0) {
		printf("%s\nError Erasing the flash at 0x%08x offset errorcode:%d%s\n", ATTR_ERR, FLASH_ADDR, errorcode,ATTR_RST);
	} else {
		printf("%s\nSuccessfully Erased Flash\n", ATTR_RST);
		errorcode = flash_read(flash, FLASH_ADDR, flash_data_read, FLASH_RW_SIZE);
		if(errorcode < 0) {
			printf("%s Error reading from flash with code:%d%s\n", ATTR_ERR, errorcode,ATTR_RST);
		} else {
			printf("%s Reading Back After Erasing Area of Flash%s\n", ATTR_INF,ATTR_RST);
			for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
				if(flash_data_read[i] != 0xff) {
					errorcode = -1;
				}
			}
		}
		if(errorcode == -1) {
			printf("%s\nFlash erase at 0x%08x did not produce correct results%s\n", ATTR_ERR, FLASH_ADDR,ATTR_RST);
			for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
				printf("%s 0x%02x%s", ATTR_INF, flash_data_read[i],i%FORMATTER==0?"\n":"");			
			} printf("\n");
		} else {
			printf("%s\nSuccessfully performed erase to flash with code:%d%s\n", ATTR_INF, errorcode,ATTR_RST);
		}
		errorcode = 0;
	}

	if(errorcode == 0) {
		printf("%s Writing the following data After Erasing Flash%s\n", ATTR_INF,ATTR_RST);
		for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) { 
			flash_data_write[i] = (rand() % (FLASH_RW_SIZE - 1 + 1)) + 1;
			printf("%s %d%s", ATTR_INF, flash_data_write[i],i%FORMATTER==0?"\n":"");
		} printf("\n");
		errorcode = flash_write(flash, FLASH_ADDR, flash_data_write, FLASH_RW_SIZE);
	}

	if(errorcode < 0) {
		printf("%s \nError writing to flash with code:%d%s\n", ATTR_ERR, errorcode,ATTR_RST);
	} else {
		printf("%s \nSuccessfully written to flash with code:%d Resetting the reading buffer....%s\n", ATTR_INF, errorcode,ATTR_RST);
		memset(flash_data_read, 0, FLASH_RW_SIZE);
		errorcode = flash_read(flash, FLASH_ADDR, flash_data_read, FLASH_RW_SIZE);
		if(errorcode < 0) {
			printf("%s Error reading from flash with code:%d%s\n", ATTR_ERR, errorcode,ATTR_RST);
		} else {
			printf("%s Successfully Read from flash with code:%d%s\n", ATTR_INF, errorcode,ATTR_RST);
			bool Data_Validated = true; uint8_t Mismatch_count = 0;
			for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
				if(flash_data_read[i] != flash_data_write[i]) {
					Data_Validated = false; Mismatch_count++;
					printf("%s %d - Read:%d != Write:%d%s\n", ATTR_ERR, i, flash_data_read[i], flash_data_write[i],ATTR_RST);
				} 
			}
			if(!Data_Validated) {
				printf("%s Flash Integrity Check Failed With %d Mismatched Entries%s\n", ATTR_ERR,Mismatch_count,ATTR_RST);
			} else{
				printf("%s Flash Integrity Check Passed!!!%s\n", ATTR_INF,ATTR_RST);
			}
		}
	}
}

int main(void)
{
	int Cnt = 0;
	struct sensor_value lvTemp = {0}, lvVolt = {0};
	uint8_t chip_id = 0, vendor_id = 0;
	int errorcode = 0;


	soc_get_id(&chip_id, &vendor_id);

	const struct device *pvt = DEVICE_DT_GET(DT_NODELABEL(pvt0));
	const struct device *flash = DEVICE_DT_GET(DT_NODELABEL(m25p32));

	if((pvt == NULL) || (!device_is_ready(pvt))) {
		printf("%s pvt has status disabled or driver is not initialized...%s\n", ATTR_ERR, ATTR_RST);
	} else {
		printf("%s pvt Object is Created %s\n", ATTR_INF, ATTR_RST);
				errorcode = sensor_channel_get(pvt, SENSOR_CHAN_DIE_TEMP, &lvTemp);
		if(errorcode == 0) {
			printf("%s Error fetching temperature value. Error code:%u%s\n", ATTR_ERR, errorcode,ATTR_RST);
		}

		errorcode = sensor_channel_get(pvt, SENSOR_CHAN_VOLTAGE, &lvVolt);
		if(errorcode == 0) {
			printf("%s Error fetching Voltage value. Error code:%u%s\n", ATTR_ERR, errorcode,ATTR_RST);
		}

		printf("%s Die Temperature:%d Voltage:%d%s\n", ATTR_INF, lvTemp.val1, lvVolt.val1,ATTR_RST);
	}

	if((flash == NULL) || (!device_is_ready(flash))) {
		printf("%s flash has status disabled or driver is not initialized...%s\n", ATTR_ERR,ATTR_RST);
	} else {
		printf("%s flash Object is Created\n", ATTR_INF);
		Flash_Test(flash, 0x1000, 0, 20);
	}

	while(true) {		
		printf(
				"%s%d - %s [CHIP_ID:0x%02x VENDOR_ID:0x%02x] Die[Temp:%d Volt:%d] mTimerClock = %d Hz\r", 
				ATTR_RST, Cnt++, 
				CONFIG_BOARD_TARGET, 
				chip_id, vendor_id, lvTemp.val1, lvVolt.val1,
				CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
			  );
		k_msleep(1000);
	}

	return 0;
}
