/*
 * Copyright (c) 2026 Filip Stojanovic <filipembedded@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(b_m2mem_pack1, LOG_LEVEL_INF);

#define FLASH_TEST_REGION_OFFSET 0xff000
#define FLASH_TEST_SECTOR_SIZE   4096U

#define TEST_COUNT     3U
#define TEST_BANNER    "=============================================="
#define TEST_SEPARATOR "----------------------------------------------"

enum test_led_status_t {
	TEST_NEXT = 0,
	TEST_PASS,
	TEST_FAIL,
	TEST_END
};

/**
 * @brief TN1618: Board reference information EEPROM addresses
 */
#define M2MEM_ID_EEPROM_USER_SECTION_BASE 0x00   /* Free to user - size 8 Kbytes */
#define M2MEM_ID_EEPROM_INFO_BASE         0x2000 /* Add-on board information - size 1 Kbyte */
#define M2MEM_ID_EEPROM_MEMA_BASE         0x2400 /* Memory A info (D0 - D3) - size 1 Kbyte */
#define M2MEM_ID_EEPROM_MEMB_BASE         0x2800 /* Memory B info (D4 - D7) - size 1 Kbyte */
#define M2MEM_ID_EEPROM_MEMC_BASE         0x2C00 /* Memory C info (D8 - D11) - size 1 Kbyte */
#define M2MEM_ID_EEPROM_MEMD_BASE         0x3000 /* Memory D info (D12 - D15) - size 1 Kbyte */
#define M2MEM_ID_EEPROM_SPI_BASE          0x3400 /* SPI information - size 1 Kbyte */
#define M2MEM_ID_EEPROM_I2C_BASE          0x3800 /* I2C information - size 1 Kbyte */
#define M2MEM_ID_EEPROM_RESERVED_BASE     0x3C00 /* Reserved - size 1 Kbyte */

/**
 * @brief Board identification and memory configuration record.
 *
 * This structure describes the memory add-on board according to the
 * M.2/SerialMemory EEPROM layout defined by TN1618.
 *
 * All multi-byte values are stored in the EEPROM using the format defined
 * by the M.2/SerialMemory specification.
 */
struct m2mem_info_t {
	uint8_t VerRev[2];            /* Board information - version and revision */
	char CPN[16];                 /* Board order code - card part number */
	char FG[16];                  /* Board version code */
	uint16_t Vmin_Vin_x100;       /* Min acceptable input supply voltage */
	uint16_t Vmax_Vin_x100;       /* Max acceptable input supply voltage */
	uint16_t Vout_x100;           /* Nominal memory supply voltage provided by the board */
	uint8_t PopulatedMemoryCount; /* Number of populated memory devices */
	uint8_t MaxMemoryCount;       /* Maximum number of memory devices supported by board */
	uint8_t JEDEC_MemA[4];        /* Identification data for memory position D0-D3. */
	uint8_t JEDEC_MemB[4];        /* Identification data for memory position D4-D7. */
	uint8_t JEDEC_MemC[4];        /* Identification data for memory position D8-D11. */
	uint8_t JEDEC_MemD[4];        /* Identification data for memory position D12-D15. */
	uint8_t Clock_MHz_Min;        /* Minimum supported memory clock frequency */
	uint8_t Clock_MHz_Max;        /* Maximal supported memory clock frequency */
	uint32_t Reserved1;
	uint32_t Reserved2;
} __packed;

static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(DT_NODELABEL(m2mem_led_1), gpios);
static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(DT_NODELABEL(m2mem_led_2), gpios);
static const struct device *const id_eeprom = DEVICE_DT_GET(DT_NODELABEL(m2mem_id_eeprom));
static const struct device *const flash = DEVICE_DT_GET(DT_NODELABEL(m2mem_flash));

static void print_hex(const char *label, const uint8_t *data, size_t len)
{
	printf("%s", label);

	for (size_t i = 0; i < len; i++) {
		printf(" %02x", data[i]);
	}

	printf("\n");
}

static void print_test_header(unsigned int num, const char *name)
{
	printf("\n" TEST_SEPARATOR "\n");
	printf("TEST %u: %s:\n", num, name);
	printf(TEST_SEPARATOR "\n");
}

static void print_test_result(unsigned int num, int ret)
{
	printf("TEST %u: %s\n", num, (ret == 0) ? "PASS" : "FAIL");
	printf(TEST_SEPARATOR "\n");
}

static void print_m2mem_info(const struct m2mem_info_t *info)
{
	printf("\nEEPROM board information\n");
	printf(TEST_SEPARATOR "\n");
	printf("Version and revision:       %u.%u\n", info->VerRev[0], info->VerRev[1]);
	printf("Card part number:           %.*s\n", (int)sizeof(info->CPN), info->CPN);
	printf("Board version code:         %.*s\n", (int)sizeof(info->FG), info->FG);
	printf("Min input supply voltage:   %u.%02u V\n", info->Vmin_Vin_x100 / 100U,
	       info->Vmin_Vin_x100 % 100U);
	printf("Max input supply voltage:   %u.%02u V\n", info->Vmax_Vin_x100 / 100U,
	       info->Vmax_Vin_x100 % 100U);
	printf("Memory supply voltage:      %u.%02u V\n", info->Vout_x100 / 100U,
	       info->Vout_x100 % 100U);
	printf("Populated memory devices:   %u\n", info->PopulatedMemoryCount);
	printf("Supported memory devices:   %u\n", info->MaxMemoryCount);

	if (info->MaxMemoryCount == 4) {
		print_hex("JEDEC ID D0-D3:            ", info->JEDEC_MemA,
			  sizeof(info->JEDEC_MemA));
		print_hex("JEDEC ID D4-D7:            ", info->JEDEC_MemB,
			  sizeof(info->JEDEC_MemB));
		print_hex("JEDEC ID D8-D11:           ", info->JEDEC_MemC,
			  sizeof(info->JEDEC_MemC));
		print_hex("JEDEC ID D12-D15:          ", info->JEDEC_MemD,
			  sizeof(info->JEDEC_MemD));
	} else if (info->MaxMemoryCount == 2) {
		print_hex("JEDEC ID D0-D7:            ", info->JEDEC_MemA,
			  sizeof(info->JEDEC_MemA));
		print_hex("JEDEC ID D8-D15:           ", info->JEDEC_MemC,
			  sizeof(info->JEDEC_MemC));
	}

	printf("Memory clock range:         %u to %u MHz\n", info->Clock_MHz_Min,
	       info->Clock_MHz_Max);
}

static void indicate_test_status(enum test_led_status_t test_led_status)
{
	switch (test_led_status) {
	case TEST_NEXT:
		for (int i = 0; i < 5; i++) {
			gpio_pin_toggle_dt(&red_led);
			gpio_pin_toggle_dt(&green_led);
			k_msleep(150);
		}
		break;

	case TEST_PASS:
		gpio_pin_set_dt(&green_led, 1);
		k_msleep(2000);
		gpio_pin_set_dt(&green_led, 0);
		break;

	case TEST_FAIL:
		gpio_pin_set_dt(&red_led, 1);
		k_msleep(2000);
		gpio_pin_set_dt(&red_led, 0);
		break;

	case TEST_END:
		gpio_pin_set_dt(&red_led, 1);
		gpio_pin_set_dt(&green_led, 1);
		return;

	default:
		printf("%s: Unknown test state.\n", __func__);
	}
	gpio_pin_set_dt(&red_led, 0);
	gpio_pin_set_dt(&green_led, 0);
	k_msleep(500);
}

/**
 * @brief Compare JEDEC ID's from identification EEPROM and from the devicetree
 *
 * @param flash_dev: Pointer to the flash device
 * @param eeprom_info: Pointer to the m2mem_info_t struct
 *
 * @return - 0 if JEDEC ID's match
 *         - -1 if incorrect params passed, or if JEDEC ID's mismatch
 */
int compare_jedec_test(const struct device *flash_dev, const struct m2mem_info_t *eeprom_info)
{
	static const uint8_t dt_jedec_id[] = DT_PROP(DT_NODELABEL(m2mem_flash), jedec_id);
	uint8_t hw_jedec_id[sizeof(dt_jedec_id)];
	int ret;

	if (flash_dev == NULL || eeprom_info == NULL) {
		return -1;
	}

	ret = flash_read_jedec_id(flash_dev, hw_jedec_id);
	if (ret < 0) {
		printf("flash_read_jedec_id failed, error: %d\n", ret);
		return -1;
	}

	print_hex("JEDEC ID - devicetree:      ", dt_jedec_id, sizeof(dt_jedec_id));
	print_hex("JEDEC ID - eeprom:          ", eeprom_info->JEDEC_MemA, sizeof(dt_jedec_id));

	ret = memcmp(dt_jedec_id, eeprom_info->JEDEC_MemA, sizeof(dt_jedec_id));
	if (ret == 0) {
		printf("jedec id stored in eeprom matches one in devicetree.\n");
	} else {
		printf("jedec id stored in eeprom doesn't match one in devicetree.\n");
		ret = -1;
	}

	return ret;
}

/**
 * @brief Erase flash region starting from FLASH_TEST_REGION_OFFSET, and
 *        verify if it succeeded.
 *
 * @param flash_dev: Pointer to the flash device
 * @return - 0 if JEDEC ID's match
 *         - -1 if incorrect params passed, or if JEDEC ID's mismatch
 */
int erase_flash_test(const struct device *flash_dev)
{
	int ret;

	if (flash_dev == NULL) {
		return -1;
	}

	ret = flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET, FLASH_TEST_SECTOR_SIZE);
	if (ret != 0) {
		printf("flash_erase failed, error: %d\n", ret);
	} else {
		printf("flash_erase success.\n");
	}

	return ret;
}

/**
 * @brief Write data to flash and verify written.
 *
 * @param flash_dev: Pointer to the flash device
 * @return - 0 if JEDEC ID's match
 *         - -1 if incorrect params passed, or if JEDEC ID's mismatch
 */
int write_flash_test(const struct device *flash_dev)
{
	int ret;
	static const uint8_t expected_data[] = {0x13, 0x12, 0x67, 0x69, 0x12};
	uint8_t read_buf[sizeof(expected_data)] = {0};

	if (flash_dev == NULL) {
		return -1;
	}

	ret = flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, expected_data,
			  sizeof(expected_data));
	if (ret != 0) {
		printf("flash_write failed, error: %d\n", ret);
		return -1;
	}

	ret = flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, read_buf, sizeof(read_buf));
	if (ret != 0) {
		printf("flash_read failed, error: %d\n", ret);
		return -1;
	}

	ret = memcmp(expected_data, read_buf, sizeof(expected_data));
	if (ret == 0) {
		printf("Data written matches data read.\n");
	} else {
		printf("Data written don't match data read.\n");
		ret = -1;
	}

	return ret;
}

int main(void)
{
	if (!gpio_is_ready_dt(&green_led)) {
		printf("green_led not ready!\n");
		return -1;
	}

	if (!gpio_is_ready_dt(&red_led)) {
		printf("red_led not ready!\n");
		return -1;
	}

	if (!device_is_ready(id_eeprom)) {
		printf("id_eeprom not ready!\n");
		return -1;
	}

	if (!device_is_ready(flash)) {
		printf("flash not ready!\n");
		return -1;
	}

	int ret = gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_INACTIVE);

	if (ret != 0) {
		printf("green_led configure failed\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		printf("red_led configure failed\n");
		return -1;
	}

	printf("\n" TEST_BANNER "\n");
	printf(" B-M2MEM-PACK1 shield test\n");
	printf(TEST_BANNER "\n");

	size_t id_eeprom_size = eeprom_get_size(id_eeprom);

	printf("ID EEPROM size: %zu bytes\n", id_eeprom_size);

	struct m2mem_info_t m2mem_info = {0};
	unsigned int passed = 0;

	ret = eeprom_read(id_eeprom, M2MEM_ID_EEPROM_INFO_BASE, &m2mem_info, sizeof(m2mem_info));
	if (ret != 0) {
		printf("id_eeprom read failed, error: %d!\n", ret);
		return -1;
	}

	printf("Board: %s\n", CONFIG_BOARD_TARGET);
	print_m2mem_info(&m2mem_info);

	/*
	 * TEST1: Read JEDEC ID in the runtime and check if it matches with
	 * one written in the devicetree.
	 */
	print_test_header(1, "Compare JEDEC ID's");
	indicate_test_status(TEST_NEXT);
	ret = compare_jedec_test(flash, &m2mem_info);
	print_test_result(1, ret);
	if (ret != 0) {
		indicate_test_status(TEST_FAIL);
	} else {
		passed++;
		indicate_test_status(TEST_PASS);
	}

	/*
	 * TEST2: Erase test - erase some section and check if it succeeded.
	 */
	print_test_header(2, "Erase flash section and verify erased");
	indicate_test_status(TEST_NEXT);
	ret = erase_flash_test(flash);
	print_test_result(2, ret);
	if (ret != 0) {
		printf("Aborting next tests as they depend on this test.\n");
		indicate_test_status(TEST_FAIL);
		return -1;
	}

	passed++;
	indicate_test_status(TEST_PASS);

	/*
	 * TEST3: Flash write test - write expected value and check if it matches,
	 * blink red if fail, blink green if success.
	 */
	print_test_header(3, "Write flash section and verify written");
	indicate_test_status(TEST_NEXT);
	ret = write_flash_test(flash);
	print_test_result(3, ret);
	if (ret != 0) {
		indicate_test_status(TEST_FAIL);
	} else {
		passed++;
		indicate_test_status(TEST_PASS);
	}

	printf("\n" TEST_BANNER "\n");
	printf(" All tests completed: %u/%u passed\n", passed, TEST_COUNT);
	printf(TEST_BANNER "\n");
	indicate_test_status(TEST_END);

	return 0;
}
