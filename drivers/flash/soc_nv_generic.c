/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/flash.h>
#include <string.h>

/* This driver supports multi-instance and also covers the soc-nv-null device
 * which is additional compatible to be placed on soc-nv-flash with no controller,
 * as for example happens in x86 QEMU device.
 */
#define DT_DRV_COMPAT soc_nv_flash

/* In case when there is only single device with null-controller as parent, then
 * there is nothing to build.
 * Unfortunatley it is not possible to filter this in CMakeLists.txt.
 */
#if !(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1 &&                             \
	DT_NODE_HAS_COMPAT(DT_PARENT(DT_INST(0, DT_DRV_COMPAT)), zephyr_null_controller))

#define SOC_NVM_PARENT_API(dev) ((struct flash_driver_api *)SOC_NVM_PARENT(dev)->api)

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#define SOC_NVM_CONFIG(dev) ((const struct soc_nv_flash_config *)(dev)->config)
#define SOC_NVM_PARENT(dev) ((SOC_NVM_CONFIG(dev)->parent))
#define SOC_NVM_DEV_TOUCH(dev)
#else
/* Single instance */
#define SOC_NV_FLASH_NODE DT_INST(0, DT_DRV_COMPAT)
#define SOC_NVM_CONFIG(dev) NULL
#define SOC_NVM_PARENT(dev) DEVICE_DT_GET(DT_PARENT(SOC_NV_FLASH_NODE))
#define SOC_NVM_DEV_TOUCH(dev) ARG_UNUSED(dev)
#endif

struct soc_nv_flash_config {
	const struct device *parent;
};

static int soc_nv_mem_read(const struct device *dev, off_t addr, void *data, size_t len)
{
	SOC_NVM_DEV_TOUCH(dev);

	return SOC_NVM_PARENT_API(dev)->read(SOC_NVM_PARENT(dev), addr, data, len);
}

static int soc_nv_mem_write(const struct device *dev, off_t addr,
			    const void *data, size_t len)
{
	SOC_NVM_DEV_TOUCH(dev);

	return SOC_NVM_PARENT_API(dev)->write(SOC_NVM_PARENT(dev), addr, data, len);
}

static int soc_nv_mem_erase(const struct device *dev, off_t addr, size_t size)
{
	SOC_NVM_DEV_TOUCH(dev);

	return SOC_NVM_PARENT_API(dev)->erase(SOC_NVM_PARENT(dev), addr, size);
}

static const struct flash_parameters *soc_nv_mem_parameters(const struct device *dev)
{
	SOC_NVM_DEV_TOUCH(dev);

	return SOC_NVM_PARENT_API(dev)->get_parameters(SOC_NVM_PARENT(dev));
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void soc_nv_mem_layout(const struct device *dev,
			   const struct flash_pages_layout **layout,
			   size_t *layout_size)
{
	SOC_NVM_DEV_TOUCH(dev);

	SOC_NVM_PARENT_API(dev)->page_layout(SOC_NVM_PARENT(dev), layout, layout_size);
}
#endif

static const struct flash_driver_api soc_nrf_flash_api = {
	.read = soc_nv_mem_read,
	.write = soc_nv_mem_write,
	.erase = soc_nv_mem_erase,
	.get_parameters = soc_nv_mem_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = soc_nv_mem_layout,
#endif
};

/* In case when there is more than one instance we need to store parent, controller,
 * in config of a instance as each instance may have different controller.
 */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#define DEFINE_SOC_NVM_FLASH_INSTANCE(n)						\
	const static struct soc_nv_flash_config soc_nv_flash_config_##n = {		\
		.parent = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),			\
	};										\
	DEVICE_DT_DEFINE_SUB(DT_DRV_INST(n), NULL, NULL,				\
			     &soc_nv_flash_config_##n,					\
			     POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,			\
			     &soc_nrf_flash_api);

/* Only devices that have controller will be defined, because otherwise there is no
 * parent to call. So when you do DEVICE_DT_GET on node that does not have controller
 * you will get linker error as no device will be provided for it.
 */
#define SOC_NV_ONLY_WITH_CONTROLLER(inst)							\
	COND_CODE_1(										\
		DT_NODE_HAS_COMPAT(DT_PARENT(DT_DRV_INST(inst)), zephyr_null_controller),	\
		(),										\
		(DEFINE_SOC_NVM_FLASH_INSTANCE(inst)))

DT_INST_FOREACH_STATUS_OKAY(SOC_NV_ONLY_WITH_CONTROLLER);
#else
/* With single instance device there is no config provided. */
DEVICE_DT_DEFINE_SUB(DT_DRV_INST(0), NULL, NULL, NULL, POST_KERNEL,
		     CONFIG_FLASH_INIT_PRIORITY, &soc_nrf_flash_api);
#endif

#endif
