/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/dma.h>
#include <scu.h>

#define ATTR_INF	"\x1b[32;1m" // ANSI_COLOR_GREEN
#define ATTR_ERR	"\x1b[31;1m" // ANSI_COLOR_RED
#define ATTR_RST	"\x1b[37;1m" // ANSI_COLOR_RESET

#define FLASH_RW_SIZE 255
#define FLASH_BASE_ADDR 0xB0000000

void Flash_Test(const struct device *flash, const struct device *dma, uint32_t FLASH_ADDR, uint8_t EVEN_ODD_MUL, uint8_t FORMATTER) {
	int errorcode = 0;
	uint8_t flash_data_write[FLASH_RW_SIZE] = {0};
	uint8_t flash_data_read[FLASH_RW_SIZE] = {0};
	errorcode = flash_read(flash, FLASH_ADDR, flash_data_read, FLASH_RW_SIZE);
	if(errorcode < 0) {
		printf("%s Error reading from flash with code:%d%s\n", ATTR_ERR, errorcode,ATTR_RST);
	} 
	else {
		printf("%s Reading Back Before Erasing Flash%s\n", ATTR_INF,ATTR_RST);
		// for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
		// 	printf("%s %d%s", ATTR_INF, flash_data_read[i],i%FORMATTER==0?"\n":"");
		// } printf("\n");
	}
	errorcode = flash_erase(flash, FLASH_ADDR, 0x1000);
	if(errorcode < 0) {
		printf("%s\nError Erasing the flash at 0x%08x offset errorcode:%d%s\n", ATTR_ERR, FLASH_ADDR, errorcode, ATTR_RST);
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
			// for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
			// 	printf("%s 0x%02x%s", ATTR_INF, flash_data_read[i],i%FORMATTER==0?"\n":"");			
			// } printf("\n");
		} else {
			printf("%s\nSuccessfully performed erase to flash with code:%d%s\n", ATTR_INF, errorcode,ATTR_RST);
		}
		errorcode = 0;
	}

	if(errorcode == 0) {
		printf("%s Writing the following data After Erasing Flash%s\n", ATTR_INF,ATTR_RST);
		for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) { 
			flash_data_write[i] = ((rand() % (FLASH_RW_SIZE - 1 + 1)) + 1)+((rand() % (FLASH_RW_SIZE - 1 + 1)) + 1);
			printf("%s %d%s", ATTR_INF, flash_data_write[i],i%FORMATTER==0?"\n":"");
		} 
		printf("\n");
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

	uint8_t *MemMapReadAddr = (uint8_t*)(FLASH_BASE_ADDR + FLASH_ADDR);
	uint8_t MemMapReadDestAddr[FLASH_RW_SIZE];
	{		
		printf("CPU Memory Mapped Reading Test from address:%p\n", MemMapReadAddr);
		bool memmaptestfail = false;
		for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) { 
			if(MemMapReadAddr[i] != flash_data_write[i]) {
				printf("%s %d%s", ATTR_ERR, MemMapReadAddr[i],i%FORMATTER==0?"\n":"");
				memmaptestfail = true;
			} 
			else {
				printf("%s %d%s", ATTR_INF, MemMapReadAddr[i],i%FORMATTER==0?"\n":"");
			}
		} printf("\n");
		if(memmaptestfail) {
			printf("%s CPU Memory mapped read failed\n", ATTR_ERR);	
		} else {
			printf("%s CPU Memory mapped read passed\n", ATTR_INF);	
		}
	} 

	if(dma != NULL) {
		printf("%s DMA Memory Mapped Reading Test from address:%p\n", ATTR_RST, MemMapReadAddr);
		// Configure DMA Block
		struct dma_block_config lv_dma_block_config = {
			.block_size = FLASH_RW_SIZE,
			.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
			.dest_address = (uint32_t)&MemMapReadDestAddr[0],
			.dest_reload_en = 0,
			.dest_scatter_count = 0,
			.dest_scatter_en = 0,
			.dest_scatter_interval = 0,
			.fifo_mode_control = 0,
			.flow_control_mode = 0,
			.next_block = NULL,
			.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
			.source_address = (uint32_t)&MemMapReadAddr[0],
			.source_gather_count = 0,
			.source_gather_en = 0,
			.source_gather_interval = 0,
			.source_reload_en = 0,
			._reserved = 0
		};
		// Configure DMA
		struct dma_config lv_Dma_Config = {
			.block_count = 1,
			.channel_direction = MEMORY_TO_MEMORY,
			.channel_priority = 1,
			.complete_callback_en = 0,
			.cyclic = 0,
			.dest_burst_length = 1,
			.dest_chaining_en = 0,
			.dest_data_size = 1,
			.dest_handshake = 1,
			.dma_callback = NULL,
			.dma_slot = 0,
			.error_callback_dis = 1,
			.head_block = &lv_dma_block_config,
			.linked_channel = 0,
			.source_burst_length = 1,
			.source_chaining_en = 0,
			.source_data_size = 1,
			.user_data = NULL
		};		
		struct dma_status stat = {0};
		printf("%s DMA Configuration...\n", ATTR_RST);
		int dma_error = dma_config(dma, 1, &lv_Dma_Config);
		k_msleep(10);
		if(dma_error) {
			printf("%s Error %d Configuration...\n", ATTR_RST, dma_error);
		} else {			
			// Call DMA Read
			printf("%s DMA Starting...\n", ATTR_INF);
			dma_error = dma_start(dma, 1);
			k_msleep(10);
		}
		(void)dma_get_status(dma, 1, &stat);
		while(stat.pending_length > 1) {
			dma_error = dma_get_status(dma, 1, &stat);
			if(dma_error) {
				printf("Error %d dma_status...\n", dma_error);
				break;
			} else {
				printf("dma_pending_length:%d \n", stat.pending_length);
			}
		}

		if(!dma_error) {
			bool memmaptestfail = false;
			for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) { 
				if(MemMapReadDestAddr[i] != flash_data_write[i]) {
					printf("%s %d%s", ATTR_ERR, MemMapReadDestAddr[i],i%FORMATTER==0?"\n":"");
					memmaptestfail = true;
				} 
				else {
					printf("%s %d%s", ATTR_INF, MemMapReadDestAddr[i],i%FORMATTER==0?"\n":"");
				}
			} 
			printf("\n");
			if(memmaptestfail) {
				printf("%s DMA Memory mapped read failed\n", ATTR_ERR);	
			} else {
				printf("%s DMA Memory mapped read passed\n", ATTR_INF);	
			}
		} else {
			printf("Error %d DMA Start...\n", dma_error);
		}
	}
}

int main(void)
{
	int Cnt = 0;
	uint8_t chip_id = 0, vendor_id = 0;

	printf(
		"%s SPI_(%d)_IRQ_Reg_Val:[0x%08x]; DMA_(%d)_IRQ_Reg_Val:[0x%08x]\n", ATTR_RST, \
		IRQ_ID_SPI, scu_get_irq_reg_val(IRQ_ID_SPI), \
		IRQ_ID_SYSTEM_DMA, scu_get_irq_reg_val(IRQ_ID_SYSTEM_DMA)
	);

	soc_get_id(&chip_id, &vendor_id);

	int errorcode = 0;
	struct sensor_value lvTemp = {0}, lvVolt = {0};
	const struct device *pvt = DEVICE_DT_GET(DT_NODELABEL(pvt0));
	const struct device *spi = DEVICE_DT_GET(DT_NODELABEL(spi0));
	const struct device *flash = DEVICE_DT_GET(DT_NODELABEL(m25p32));
	const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma0));

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

	if(spi == NULL) {
		printf("%s spi has status disabled...%s\n", ATTR_ERR,ATTR_RST);
	} else {			
		printf("%s spi Object is Created. Test Via DMA\n", ATTR_INF);				
		if(flash == NULL) {
			printf("%s flash has status disabled...%s\n", ATTR_ERR,ATTR_RST);
		} else {
			printf("%s flash Object is Created. Initialize to Test Via DMA%s\n", ATTR_INF,ATTR_RST);						
			if(flash != NULL) {
				Flash_Test(flash, dma, 0x1000, 0, 20);
			} else {
				printf("%s Error Initializing DMA Device%s\n", ATTR_ERR,ATTR_RST);
				Flash_Test(flash, NULL, 0x1000, 0, 20);
			}
		}
	}

	while(true) {		
		printf(
				"%s%d - %s [CHIP_ID:0x%02x VENDOR_ID:0x%02x] Build[Date:%s Time:%s]\r", 
				ATTR_RST, Cnt++, 
				CONFIG_BOARD_TARGET, 
				chip_id, vendor_id,
				__DATE__, __TIME__
			  );
		k_msleep(1000);
	}

	return 0;
}
