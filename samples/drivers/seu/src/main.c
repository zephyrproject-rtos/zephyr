/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * A sample application using seu_subsystem to read/write seu errors.
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/seu/seu.h>

static struct k_sem semaphore;

#ifndef SEU_SECTOR_ADDR
#define SEU_SECTOR_ADDR (5)
#endif

#ifndef SEU_INJECT_SINGLE_BIT
#define SEU_INJECT_SINGLE_BIT (0)
#endif

#ifndef SEU_CRAM_SEL_0
#define SEU_CRAM_SEL_0 (0)
#endif

#ifndef SEU_CRAM_SEL_1
#define SEU_CRAM_SEL_1 (1)
#endif

#ifndef NUMBER_OF_INJECTION
#define NUMBER_OF_INJECTION (0)
#endif

#ifndef ECC_INJECT_SINGLE_BIT
#define ECC_INJECT_SINGLE_BIT (1)
#endif

#ifndef ECC_SECTOR_ADDR
#define ECC_SECTOR_ADDR (0xFF)
#endif

#ifndef ECC_RAM_ID
#define ECC_RAM_ID (1)
#endif

#define SEU_DEV DEVICE_DT_GET(DT_ALIAS(seu));

void seu_callback(void *data)
{
	if (data == NULL) {
		printk("Callback received data not valid\n");
		return;
	}

	struct seu_err_data *seu_data = (struct seu_err_data *)data;

	printk("The SEU Error Type: %d:\n", seu_data->sub_error_type);
	printk("The Sector Address: %d\n", seu_data->sector_addr);
	printk("The Correction status: %d\n", seu_data->correction_status);
	printk("The row frame index: %d\n", seu_data->row_frame_index);
	printk("The bit position: %d\n", seu_data->bit_position);

	k_sem_give(&semaphore);
}

void ecc_callback(void *data)
{
	if (data == NULL) {
		printk("Callback received data not valid\n");
		return;
	}

	struct ecc_err_data *ecc_data = (struct ecc_err_data *)data;

	printk("The ECC Error Type: %d:\n", ecc_data->sub_error_type);
	printk("The Sector Address: %d\n", ecc_data->sector_addr);
	printk("The Correction status: %d\n", ecc_data->correction_status);
	printk("Ram Id: %d\n", ecc_data->ram_id_error);

	k_sem_give(&semaphore);
}

int inject_safe_seu_error(const struct device *seu_dev)
{
	int ret;
	struct inject_safe_seu_error_frame seu_inject_safe_error;

	seu_inject_safe_error.sector_address = SEU_SECTOR_ADDR;
	seu_inject_safe_error.inject_type = SEU_INJECT_SINGLE_BIT;
	seu_inject_safe_error.number_of_injection = NUMBER_OF_INJECTION;
	seu_inject_safe_error.cram_sel_0 = SEU_CRAM_SEL_0;
	seu_inject_safe_error.cram_sel_1 = SEU_CRAM_SEL_0;

	ret = insert_safe_seu_error(seu_dev, &seu_inject_safe_error);

	return ret;
}

int read_seu_statistics_information(const struct device *seu_dev)
{
	int ret;
	struct seu_statistics_data seu_read_statistics;

	ret = read_seu_statistics(seu_dev, SEU_SECTOR_ADDR, &seu_read_statistics);

	if (ret != 0) {
		printk("SEU read statistics failed %x\n", ret);
		return ret;
	}
	printk("The value of t_seu_cycle : 0x%x\n", seu_read_statistics.t_seu_cycle);
	printk("The value of t_seu_detect : 0x%x\n", seu_read_statistics.t_seu_detect);
	printk("The value of t_seu_correct : 0x%x\n", seu_read_statistics.t_seu_correct);
	printk("The value of t_seu_inject_detect : 0x%x\n",
	       seu_read_statistics.t_seu_inject_detect);
	printk("The value of t_sdm_seu_poll_interval : 0x%x\n",
	       seu_read_statistics.t_sdm_seu_poll_interval);
	printk("The value of t_sdm_seu_pin_toggle_overhead : 0x%x\n",
	       seu_read_statistics.t_sdm_seu_pin_toggle_overhead);

	return 0;
}

int inject_ecc_error(const struct device *seu_dev)
{
	int ret;
	struct inject_ecc_error_frame ecc_err;

	ecc_err.inject_type = ECC_INJECT_SINGLE_BIT;
	ecc_err.ram_id = ECC_RAM_ID;
	ecc_err.sector_address = ECC_SECTOR_ADDR;

	ret = insert_ecc_error(seu_dev, &ecc_err);

	return ret;
}

int main(void)
{
	int ret;
	const struct device *seu_dev;
	uint32_t client_no_seu, client_no_ecc;

	seu_dev = SEU_DEV;

	if (!device_is_ready(seu_dev)) {
		printk("SEU driver is not ready\n");
		return -ENODEV;
	}

	ret = k_sem_init(&semaphore, 0, 1);
	if (ret != 0) {
		printk("Semaphore initialization failed\n");
		return ret;
	}
	printk("SEU Test Started\n");
	ret = seu_callback_function_register(seu_dev, seu_callback, SEU_ERROR_MODE, &client_no_seu);

	if (ret != 0) {
		printk("SEU callback function register failed\n");
		return ret;
	}

	printk("The Client No is %d\n", client_no_seu);

	ret = seu_callback_function_enable(seu_dev, client_no_seu);

	if (ret != 0) {
		printk("SEU callback function register enable failed\n");
		return ret;
	}

	ret = seu_callback_function_register(seu_dev, ecc_callback, ECC_ERROR_MODE, &client_no_ecc);

	if (ret != 0) {
		printk("SEU callback function register failed\n");
		return ret;
	}
	printk("The Client No is %d\n", client_no_ecc);

	if (ret != 0) {
		printk("SEU callback function register enable failed\n");
		return ret;
	}

	ret = seu_callback_function_enable(seu_dev, client_no_ecc);

	if (ret != 0) {
		printk("SEU callback function register enable failed\n");
		return ret;
	}

	printk("SEU Safe Error Insert Test Started\n");

	ret = inject_safe_seu_error(seu_dev);
	if (ret != 0) {
		printk("SEU safe injection failed %x\n", ret);
		return ret;
	}
	k_sem_take(&semaphore, K_FOREVER);
	printk("SEU Safe Error Insert Test Completed\n");
	printk("Read SEU Statistics Test Started\n");

	ret = read_seu_statistics_information(seu_dev);
	if (ret != 0) {
		printk("SEU read failed %x\n", ret);
		return -1;
	}
	printk("Read SEU Statistics Test Completed\n");
	printk("Read ECC Error Test Started\n");

	ret = inject_ecc_error(seu_dev);

	if (ret != 0) {
		printk("ECC injection failed %x\n", ret);
		return ret;
	}

	k_sem_take(&semaphore, K_FOREVER);
	printk("Read ECC Error Test Completed\n");

	ret = seu_callback_function_disable(seu_dev, client_no_ecc);

	if (ret != 0) {
		printk("SEU callback function register disable failed\n");
		return ret;
	}

	ret = seu_callback_function_disable(seu_dev, client_no_seu);

	if (ret != 0) {
		printk("SEU callback function register disable failed\n");
		return ret;
	}

	printk("SEU Test Completed\n");
	return 0;
}
