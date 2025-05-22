#include <stdint.h>
#include "anx7533_flash.h"
#include "anx7533_config.h"
#include "anx7533_reg.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Flash write protection range
#define  FLASH_PROTECTION_ALL  (BP4 | BP3 | BP2 | BP1 | BP0)
#define  HW_FLASH_PROTECTION_PATTERN   (SRP0 | FLASH_PROTECTION_ALL)
#define  FLASH_PROTECTION_PATTERN_MASK (SRP0 | BP4 | BP3 | BP2 | BP1 | BP0)

#define  SW_FLASH_PROTECTION_PATTERN   ( FLASH_PROTECTION_ALL )

LOG_MODULE_DECLARE(anx7533, LOG_LEVEL_INF);

static inline void anx7533_read_status_enable(const struct device *dev)
{
          do {
		  uint8_t tmp;

		  anx7533_i2c_read_byte(dev, SLAVEID_SPI, R_DSC_CTRL_0, &tmp);
		  tmp |= READ_STATUS_EN;
		  anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_DSC_CTRL_0, tmp);
	  } while(0);
}

static inline void anx7533_write_general_instruction(const struct device *dev,
					     uint8_t instruction_type)
{
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_STATUS_2, instruction_type);
}

// READ_DELAY_SELECT = 0, GENERAL_INSTRUCTION_EN = 1;
static inline void anx7533_general_instruction_enable(const struct device *dev)
{
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_RW_CTRL, GENERAL_INSTRUCTION_EN);
}

static inline void anx7533_write_status_register(const struct device *dev, uint8_t value)
{
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_STATUS_0, value);
}

// READ_DELAY_SELECT = 0, WRITE_STATUS_EN = 1;
static inline void anx7533_write_status_enable(const struct device *dev)
{
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_RW_CTRL, WRITE_STATUS_EN);
}

static inline void anx7533_flash_write_enable(const struct device *dev)
{
	do {
		anx7533_write_general_instruction(dev, WRITE_ENABLE);
		anx7533_general_instruction_enable(dev);
	} while(0);
}

static inline void anx7533_flash_write_disable(const struct device *dev)
{
	do {
		anx7533_write_general_instruction(dev, WRITE_DISABLE);
		anx7533_general_instruction_enable(dev);
	} while(0);
}

static inline void anx7533_flash_write_status_register(const struct device *dev,
						       uint8_t value)
{
	do {
		anx7533_flash_write_enable(dev);
		anx7533_write_status_register(dev, value);
		anx7533_write_status_enable(dev);
	} while(0);
}

static inline void anx7533_flash_address(const struct device *dev, uint16_t addr)
{
	do {
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_L, addr);
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_H,
			       (uint8_t)(addr >> 8));
	} while(0);
}

// FLASH_ERASE_TYPE = R_FLASH_STATUS_3
static inline void anx7533_flash_erase_type(const struct device *dev, uint16_t type)
{
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_STATUS_3, type);
}

// READ_DELAY_SELECT = 0, FLASH_ERASE_EN = 1
static inline void anx7533_flash_erase_enable(const struct device *dev)
{
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_RW_CTRL, FLASH_ERASE_EN);
}

static inline void anx7533_flash_sector_erase(const struct device *dev, uint16_t addr)
{
	do {
		anx7533_flash_write_enable(dev);
		anx7533_flash_address(dev, addr);
		anx7533_flash_erase_type(dev, SECTOR_ERASE);
		anx7533_flash_erase_enable(dev);
	} while(0);
}

// READ_DELAY_SELECT = 0, FLASH_READ = 1
static inline void anx7533_flash_ocm_read_enable(const struct device *dev)
{
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_RW_CTRL, FLASH_READ);
}

// READ_DELAY_SELECT = 0, FLASH_WRITE = 1
static inline void anx7533_flash_ocm_write_enable(const struct device *dev)
{
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_RW_CTRL, FLASH_WRITE);
}

static unsigned char const OCM_FW_DATA[]= {
	#include "anx7533_ocm_hex.h"
};
static unsigned char write_data_buf[MAX_BYTE_COUNT_PER_RECORD_FLASH];

static void anx7533_flash_wait_until_wip_cleared(const struct device *dev)
{
	uint8_t  tmp;

	anx7533_read_status_enable(dev);
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, R_FLASH_STATUS_4, &tmp);

	while ((tmp & 1) != 0) {
		k_msleep(1);
		anx7533_read_status_enable(dev);
		anx7533_i2c_read_byte(dev, SLAVEID_SPI, R_FLASH_STATUS_4, &tmp);
	};
}

static void anx7533_flash_wait_until_flash_sm_done(const struct device *dev)
{
	uint8_t  tmp;

	anx7533_i2c_read_byte(dev, SLAVEID_SPI, R_RAM_CTRL, &tmp);

	while ((tmp & FLASH_DONE) == 0) {
		k_msleep(1);
		anx7533_i2c_read_byte(dev, SLAVEID_SPI, R_RAM_CTRL, &tmp);
	};
}

static void anx7533_flash_hw_write_protection_enable(const struct device *dev)
{
	uint8_t data;

	anx7533_i2c_read_byte(dev, SLAVEID_SPI, GPIO_STATUS_1, &data);
	data |= FLASH_WP;
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, GPIO_STATUS_1, data);

	data = HW_FLASH_PROTECTION_PATTERN;
	anx7533_flash_wait_until_flash_sm_done(dev);
	anx7533_flash_write_status_register(dev, data);

	anx7533_flash_wait_until_wip_cleared(dev);

	anx7533_i2c_read_byte(dev, SLAVEID_SPI, GPIO_STATUS_1, &data);
	data &= ~FLASH_WP;
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, GPIO_STATUS_1, data);

	anx7533_flash_wait_until_flash_sm_done(dev);
	anx7533_read_status_enable(dev);

	anx7533_i2c_read_byte(dev, SLAVEID_SPI, R_FLASH_STATUS_4, &data);
	anx7533_flash_wait_until_flash_sm_done(dev);

	if ((data & FLASH_PROTECTION_PATTERN_MASK) == HW_FLASH_PROTECTION_PATTERN) {
		LOG_DBG("Flash hardware write protection enabled.\n");
	} else {
		LOG_ERR("Enabling protection FAILED! = 0x%02X\n", data);
	}
}

static void anx7533_flash_write_protection_disable(const struct device *dev)
{
	uint8_t data;

	// WP# pin of Flash die = high, not hardware write protected
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, GPIO_STATUS_1, &data);
	data |= FLASH_WP;
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, GPIO_STATUS_1, data);

	data = 0;
	anx7533_flash_wait_until_flash_sm_done(dev);
	anx7533_flash_write_status_register(dev, data);

	anx7533_flash_wait_until_wip_cleared(dev);

	anx7533_flash_wait_until_flash_sm_done(dev);
	anx7533_read_status_enable(dev);
	// STATUS_REGISTER
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, R_FLASH_STATUS_4, &data);
	anx7533_flash_wait_until_flash_sm_done(dev);

	if ((data & FLASH_PROTECTION_PATTERN_MASK) == 0) {
		LOG_DBG("Flash write protection disabled.\n");
	} else {
		LOG_ERR("Disable protection FAILED! = 0x%02X\n", data);
	}
}

static void anx7533_flash_actual_write(const struct device *dev)
{
	anx7533_flash_wait_until_wip_cleared(dev);  /* This is waiting for previous flash_write_enable() is done, i.e. writing status register is done */
	anx7533_flash_ocm_write_enable(dev);
	anx7533_flash_wait_until_wip_cleared(dev);

	anx7533_flash_wait_until_flash_sm_done(dev);
}

void anx7533_flash_command_erase_partition(const struct device *dev, uint8_t part_id)
{
	// Flash address, any address inside the sector is a valid address for the Sector Erase (SE) command
	uint16_t flash_addr;
	uint16_t base_addr;
	uint16_t end_addr;
	uint8_t *str[PARTITION_ID_MAX] = {
				"Main OCM FW",
				"Secure OCM FW",
				"HDCP 1.4 & 2.2 key",
				"chip Id"};

	if (part_id >= PARTITION_ID_MAX) {
		LOG_ERR("Partition ID is invalid!! = %d\n", part_id);
		return;
	}

	anx7533_flash_write_protection_disable(dev);

	switch (part_id) {
		case MAIN_OCM:
			base_addr = MAIN_OCM_FW_ADDR_BASE;
			end_addr = MAIN_OCM_FW_ADDR_END;
			break;
		case SECURE_OCM:
			base_addr = SECURE_OCM_FW_ADDR_BASE;
			end_addr = SECURE_OCM_FW_ADDR_END;
			break;
		case HDCP_14_22_KEY:
			base_addr = HDCP_14_22_KEY_ADDR_BASE;
			end_addr = HDCP_14_22_KEY_ADDR_END;
			break;
		case CHIP_ID:
			base_addr = CHIP_ID_ADDR_BASE;
			end_addr = CHIP_ID_ADDR_END;
			break;
		default:
			break;
	}

	for (flash_addr = base_addr; flash_addr <= end_addr; flash_addr += FLASH_SECTOR_SIZE) {
		anx7533_flash_sector_erase(dev, flash_addr);
		anx7533_flash_wait_until_wip_cleared(dev);
		anx7533_flash_wait_until_flash_sm_done(dev);
	}

	LOG_INF("%s erased.\n", str[part_id]);

	anx7533_flash_hw_write_protection_enable(dev);
}

#define VERSION_ADDR 0x0100
static void anx7533_read_hex_ver(const struct device *dev, uint8_t *pData)
{

	pData[0] = OCM_FW_DATA[VERSION_ADDR]; // main version
	pData[1] = OCM_FW_DATA[VERSION_ADDR + 1] ; // minor version
	pData[2] = OCM_FW_DATA[VERSION_ADDR + 2] ; // build version
	pData[0] = pData[0]&0x0F;
	pData[1] = pData[1]&0x0F;
}

int anx7533_flash_burn_hex_auto(const struct device *dev)
{
	static uint8_t i;
	uint8_t update_flag;
	uint8_t reg_temp;
	uint8_t current_version[3];
	uint8_t hex_version[3];
	uint8_t write_length;
	uint16_t hex_index;
	uint16_t hex_total;
	uint16_t last_data;
	uint16_t flash_addr;
	uint16_t total_write_data;

	// RESET and POWER UP Chicago
	anx7533_chip_poweron(dev);
	// delay for OCM get version
	k_msleep(50);

	// stop main OCM
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, OCM_DEBUG_CTRL, &reg_temp);
	reg_temp |= OCM_RESET;
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, OCM_DEBUG_CTRL, reg_temp);
	// stop secure OCM to avoid buffer access conflict
	anx7533_i2c_read_byte(dev, SLAVEID_DP_IP, ADDR_HDCP2_CTRL, &reg_temp);
	reg_temp &= (~HDCP2_FW_EN);
	anx7533_i2c_write_byte(dev, SLAVEID_DP_IP, ADDR_HDCP2_CTRL, reg_temp);

	// can read I2C or not
	if (0 != anx7533_i2c_read_byte(dev, SLAVEID_SPI, R_VERSION, &reg_temp)) {
		LOG_ERR("I2C err, auto-flash FAIL!!!\n");
		return VALUE_FAILURE;
	}

	// read current OCM version
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, OCM_VERSION_MAJOR, &reg_temp);
	current_version[0] = (reg_temp >> 4) & 0x0F;
	current_version[1] = (reg_temp) & 0x0F;
	anx7533_i2c_read_byte(dev, SLAVEID_SPI, OCM_BUILD_NUM, &reg_temp);
	current_version[2] = reg_temp;

	anx7533_read_hex_ver(dev, &hex_version[0]);

	LOG_INF("OCM ver:%1X.%1X.%02X, HEX ver:%1X.%1X.%02X...",
			current_version[0],current_version[1],current_version[2],
			hex_version[0],hex_version[1],hex_version[2]);

	update_flag = 0;
	// check version
	for (i = 0; i < 3; i++) {
		if (current_version[i] != hex_version[i]) {
			if (current_version[i]>hex_version[i]) {
				update_flag = 0;
				break;
			}else{ // current < hex
				update_flag = 1;
				break;
			}
		}
	}

	if (0 == update_flag) {
		LOG_INF("No need to flash\n");
		anx7533_chip_powerdown(dev);
		return VALUE_SUCCESS;
	}

	LOG_INF("Start to flash\n");
	// delay to avoid Flash unstable
	k_msleep(50);
	// Erase OCM first
	anx7533_flash_command_erase_partition(dev, MAIN_OCM);
	// delay to avoid Flash unstable
	k_msleep(100);

	total_write_data = 0;
	hex_index = 0;
	hex_total = (sizeof(OCM_FW_DATA))/FLASH_WRITE_MAX_LENGTH;
	last_data = (sizeof(OCM_FW_DATA))%FLASH_WRITE_MAX_LENGTH;

	LOG_INF("Flash total=%d, last=%d\n", hex_total, last_data);

	if ((0 == last_data) && (hex_total > 0)) {
		// The last 32 bytes include CRC data
		hex_total--;
		last_data = FLASH_WRITE_MAX_LENGTH;
	}

	anx7533_flash_write_protection_disable(dev);
	anx7533_flash_wait_until_flash_sm_done(dev);

	// **************************** Write 16/32 bytes data , begin
	for (hex_index = 0; hex_index <= hex_total; hex_index++) {
		if (hex_index == hex_total) {
			if (last_data > (FLASH_WRITE_MAX_LENGTH/2)) {
				// one more 16 bytes data
				total_write_data += (FLASH_WRITE_MAX_LENGTH/2);
				write_length = (FLASH_WRITE_MAX_LENGTH/2);
			} else {
				// write CRC data
				break;
			}
		} else {
			total_write_data += FLASH_WRITE_MAX_LENGTH;
			write_length = FLASH_WRITE_MAX_LENGTH;
		}

		// print prograss status
		if (hex_index % 0x20 == 0) {
			LOG_INF("flashing...%02d%s\n", ((hex_index*100)/hex_total),"%");
		}

		// Copy data to buffer
		memcpy(&write_data_buf[0],&OCM_FW_DATA[hex_index*FLASH_WRITE_MAX_LENGTH],write_length);
		flash_addr = MAIN_OCM_FW_ADDR_BASE+(hex_index*FLASH_WRITE_MAX_LENGTH);

		anx7533_flash_write_enable(dev);

		// Write address register
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_H, (uint8_t)(flash_addr >> 8));
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_L, (uint8_t)(flash_addr & 0x00FF));

		// Write length register
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_LEN_H, 0);
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_LEN_L, (uint8_t)(FLASH_WRITE_MAX_LENGTH - 1));

		// Write data register
		for (i = 0; i < write_length; i++) {
			anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_0 + i, write_data_buf[i]);
		}

		if (i != FLASH_WRITE_MAX_LENGTH) {
			for (; i < FLASH_WRITE_MAX_LENGTH; i++) {
				anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_0 + i, 0xFF);
			}
		}

		// Write to Flash
		anx7533_flash_actual_write(dev);

		/* =============== Reads 32 bytes =============== */
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_H,
			       (uint8_t)(flash_addr >> 8));
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_L,
			       (uint8_t)(flash_addr & 0x00FF));

		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_LEN_H, 0);
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_LEN_L,
			       (uint8_t)(FLASH_READ_MAX_LENGTH-1));

		anx7533_flash_ocm_read_enable(dev);
		anx7533_flash_wait_until_flash_sm_done(dev);

		/* =============== Checking 32 bytes =============== */
		for (i = 0; i < write_length; i++) {
			anx7533_i2c_read_byte(dev, SLAVEID_SPI, FLASH_READ_D0 + i, &reg_temp);
			if (reg_temp!=write_data_buf[i]) {
				LOG_ERR("R err=%02X %02X", reg_temp,write_data_buf[i]);
				goto flash_error;
			}
		}
	}

	// **************************** Write 16/32 bytes data , end

	// **************************** Write CRC data data , begin
	memcpy(&write_data_buf[0],&OCM_FW_DATA[sizeof(OCM_FW_DATA)-HEX_LINE_SIZE],HEX_LINE_SIZE);
	flash_addr = MAIN_OCM_FW_ADDR_END-(FLASH_WRITE_MAX_LENGTH)+1;

	anx7533_flash_write_enable(dev);

	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_H, flash_addr >> 8);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_L, flash_addr & 0xFF);

	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_LEN_H, 0);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_LEN_L, FLASH_WRITE_MAX_LENGTH- 1);

	for (i = 0; i < (FLASH_READ_MAX_LENGTH-HEX_LINE_SIZE); i++) {
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_0 + i, 0xFF);
	}

	for (; i < FLASH_READ_MAX_LENGTH; i++) {
		anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_0 + i,
			       write_data_buf[i-HEX_LINE_SIZE]);
	}
	anx7533_flash_actual_write(dev);

	/* =============== Reads 32 bytes =============== */
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_H, flash_addr >> 8);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_ADDR_L, flash_addr & 0xFF);

	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_LEN_H, 0);
	anx7533_i2c_write_byte(dev, SLAVEID_SPI, R_FLASH_LEN_L, FLASH_READ_MAX_LENGTH - 1);

	anx7533_flash_ocm_read_enable(dev);
	anx7533_flash_wait_until_flash_sm_done(dev);

	/* =============== Checking 16 bytes =============== */
	for (i = HEX_LINE_SIZE; i < FLASH_READ_MAX_LENGTH; i++) {
		anx7533_i2c_read_byte(dev, SLAVEID_SPI, FLASH_READ_D0 + i, &reg_temp);
		if (reg_temp != write_data_buf[i-HEX_LINE_SIZE]) {
			goto flash_error;
		}
	}
	// **************************** Write CRC data data , end

	anx7533_flash_hw_write_protection_enable(dev);
	LOG_INF("flash %d bytes done.\n\n", total_write_data);
	// delay to avoid Flash unstable
	k_msleep(100);
	anx7533_chip_powerdown(dev);
	// delay to avoid Flash unstable
	k_msleep(100);

	return 0;

flash_error:
	anx7533_flash_hw_write_protection_enable(dev);
	LOG_ERR("Flash ERROR!!\n");
	anx7533_chip_powerdown(dev);

	return VALUE_FAILURE2;

}
