#ifndef __ANX7533_FLASH_H__
#define __ANX7533_FLASH_H__

#include <zephyr/device.h>

#define FALSH_READ_BACK
#define FLASH_SECTOR_SIZE	(4 * 1024)

#define MAIN_OCM		0
#define SECURE_OCM		1
#define HDCP_14_22_KEY		2
#define CHIP_ID			3
#define PARTITION_ID_MAX	4

#define MAIN_OCM_FW_ADDR_BASE		0x1000
#define MAIN_OCM_FW_ADDR_END		0x8FFF

#define SECURE_OCM_FW_ADDR_BASE		0xA000
#define SECURE_OCM_FW_ADDR_END		0xCFFF

#define HDCP_14_22_KEY_ADDR_BASE	0x9000
#define HDCP_14_22_KEY_ADDR_END		0x9FFF

#define CHIP_ID_ADDR_BASE		0xD000
#define CHIP_ID_ADDR_END		0xDFFF

#define	FLASH_START_ADDRESS		0x0020
#define	FLASH_END_ADDRESS		0xFFFF


#define VERSION_ADDR			0x0100

#define HEX_LINE_SIZE			16

/******************************************************************************
Function Prototype
******************************************************************************/
void anx7533_flash_command_erase_partition(const struct device *dev,
					   uint8_t part_id);
int anx7533_flash_burn_hex_auto(const struct device *dev);

int anx7533_i2c_write_byte(const struct device *dev, uint8_t slave_id,
			   uint16_t offset_addr, uint8_t data);

int anx7533_i2c_read_byte(const struct device *dev, uint8_t slave_id,
			  uint16_t offset_addr, uint8_t *p_data);

void anx7533_chip_poweron(const struct device *dev);

void anx7533_chip_powerdown(const struct device *dev);

#endif

