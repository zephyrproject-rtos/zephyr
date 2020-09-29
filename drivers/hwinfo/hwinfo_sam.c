/*
 * Copyright (c) 2019 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/hwinfo.h>
#include <init.h>
#include <soc.h>
#include <string.h>

static uint8_t sam_uid[16];

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	if (length > sizeof(sam_uid)) {
		length = sizeof(sam_uid);
	}

	memcpy(buffer, sam_uid, length);

	return length;
}

/* On the Atmel SAM SoC series, the device id is located in the flash
 * controller. The controller can either present the flash area containing
 * the code, the unique identifier or the user signature area at the flash
 * location. Therefore the function reading the device id must be executed
 * from RAM with the interrupts disabled. To avoid executing this complex
 * code each time the device id is requested, we do this at boot time at save
 * the 128-bit value into RAM.
 */
__ramfunc static void hwinfo_sam_read_device_id(void)
{
	Efc *efc = (Efc *)DT_REG_ADDR(DT_INST(0, atmel_sam_flash_controller));
	uint8_t *flash = (uint8_t *)CONFIG_FLASH_BASE_ADDRESS;
	int i;

	/* Switch the flash controller to the unique identifier area. The flash
	 * is not available anymore, hence we have to wait for it to be *NOT*
	 * ready.
	 */
	efc->EEFC_FCR = EEFC_FCR_FKEY_PASSWD | EEFC_FCR_FCMD_STUI;
	while ((efc->EEFC_FSR & EEFC_FSR_FRDY) == EEFC_FSR_FRDY) {
		/* Wait */
	}

	/* Copy the 128-bit unique ID. We cannot use memcpy as it would
	 * execute code from flash.
	 */
	for (i = 0; i < sizeof(sam_uid); i++) {
		sam_uid[i] = flash[i];
	}

	/* Switch back the controller to the flash area and wait for it to
	 * be ready.
	 */
	efc->EEFC_FCR = EEFC_FCR_FKEY_PASSWD | EEFC_FCR_FCMD_SPUI;
	while ((efc->EEFC_FSR & EEFC_FSR_FRDY) != EEFC_FSR_FRDY) {
		/* Wait */
	}
}

static int hwinfo_sam_init(const struct device *arg)
{
	Efc *efc = (Efc *)DT_REG_ADDR(DT_INST(0, atmel_sam_flash_controller));
	uint32_t fmr;
	int key;

	/* Disable interrupts. */
	key = irq_lock();

	/* Disable code loop optimization and sequential code optimization. */
	fmr = efc->EEFC_FMR;

#ifndef CONFIG_SOC_SERIES_SAM3X
	efc->EEFC_FMR = (fmr & (~EEFC_FMR_CLOE)) | EEFC_FMR_SCOD;
#else
	/* SAM3x does not have loop optimization (EEFC_FMR_CLOE) */
	efc->EEFC_FMR |= EEFC_FMR_SCOD;
#endif

	/* Read the device ID using code in RAM */
	hwinfo_sam_read_device_id();

	/* Restore code optimization settings. */
	efc->EEFC_FMR = fmr;

	/* Re-enable interrupts */
	irq_unlock(key);

	return 0;
}

SYS_INIT(hwinfo_sam_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
