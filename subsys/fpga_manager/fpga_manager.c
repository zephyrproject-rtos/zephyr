/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <zephyr/fpga_manager/fpga_manager.h>

__weak int32_t fpga_get_status_plat(void *status)
{
	ARG_UNUSED(status);
	return -ENOSYS;
}

int32_t fpga_get_status(void *status)
{
	return fpga_get_status_plat(status);
}

__weak int32_t fpga_load_plat(char *image_ptr, uint32_t img_size)
{
	ARG_UNUSED(image_ptr);
	ARG_UNUSED(img_size);
	return -ENOSYS;
}

int32_t fpga_load(char *image_ptr, uint32_t img_size)
{
	return fpga_load_plat(image_ptr, img_size);
}

__weak int32_t fpga_load_file_plat(const char *const filename, const uint32_t config_type)
{
	ARG_UNUSED(filename);
	ARG_UNUSED(config_type);
	return -ENOSYS;
}

int32_t fpga_load_file(const char *const filename, const uint32_t config_type)
{
	return fpga_load_file_plat(filename, config_type);
}

__weak int32_t fpga_get_memory_plat(char **phyaddr, uint32_t *size)
{
	ARG_UNUSED(phyaddr);
	ARG_UNUSED(size);
	return -ENOSYS;
}

int32_t fpga_get_memory(char **phyaddr, uint32_t *size)
{
	return fpga_get_memory_plat(phyaddr, size);
}
