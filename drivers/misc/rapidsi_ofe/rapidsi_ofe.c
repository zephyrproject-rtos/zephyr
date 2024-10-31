/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/misc/rapidsi/rapidsi_ofe.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

#define LOG_LEVEL CONFIG_RAPIDSI_OFE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rapidsi_ofe);

#if DT_HAS_COMPAT_STATUS_OKAY(rapidsi_ofe)
  #define DT_DRV_COMPAT rapidsi_ofe
  static volatile struct ofe_cfg_status *s_OFE_Cfg_Status = NULL;
#else
  #error Rapid Silicon OFE IP is not enabled in the Device Tree
#endif

struct ofe_config {
	uint32_t base;
	const struct pinctrl_dev_config *pcfg;
};

int get_xcb_config_status(
							const struct device *dev, 
							enum OFE_STATUS_SUBSYS_TYPE inElem, 
							bool *status
						)
{
	switch (inElem) {
		case (OFE_STATUS_SUBSYS_FCB): {
			*status = s_OFE_Cfg_Status->fcb_cfg_status;
		} break;
		case (OFE_STATUS_SUBSYS_ICB): {
			*status = s_OFE_Cfg_Status->icb_cfg_status;
		} break;
		default: {
			LOG_ERR("%s(%d) Invalid Element Type:%d\r\n", __func__, __LINE__, inElem);
			return -ENOSYS;
		} break;
  	}
  	return 0;
}	

int set_config_status(
						const struct device *dev, 
						enum OFE_CFG_STATUS inElem, 
						bool status
					 )
{
  struct ofe_cfg_status lv_ofe_cfg_status = {0};

  // Read
  lv_ofe_cfg_status = *s_OFE_Cfg_Status;

  // Modify
  switch (inElem) {
    case (OFE_CFG_STATUS_DONE): {
      lv_ofe_cfg_status.cfg_done = status;
    } break;
    case (OFE_CFG_STATUS_ERROR): {
      lv_ofe_cfg_status.cfg_error = status;
    } break;
	default: {
		LOG_ERR("%s(%d) Not Implemented Element Type:%d\r\n", __func__, __LINE__, inElem);
		return -ENOSYS;
	} break;	
  }

  // Write
  sys_write32 (*(uint32_t*)&lv_ofe_cfg_status, (mem_addr_t)s_OFE_Cfg_Status);

  if(sys_read32((mem_addr_t)s_OFE_Cfg_Status) != *(uint32_t*)&lv_ofe_cfg_status) {

	return -EINVAL;
  }

  return 0;
}	

int reset(
			const struct device *dev, 
			enum OFE_RESET_SUBSYS_TYPE inType,
            bool reset_value
		 )
{
  struct ofe_cfg_status lv_ofe_cfg_status = {0};

  // Read
  lv_ofe_cfg_status = *s_OFE_Cfg_Status;

  	switch (inType) {
		case (OFE_RESET_SUBSYS_FCB): {
			// Modify
			lv_ofe_cfg_status.global_reset_fpga = reset_value;

			// Write
			sys_write32 (*(uint32_t*)&lv_ofe_cfg_status, (mem_addr_t)s_OFE_Cfg_Status);

			if (s_OFE_Cfg_Status->global_reset_fpga != reset_value) {
				LOG_ERR("%s(%d) Error Setting global fpga reset bit: %d != %d\r\n", \
				__func__, __LINE__, s_OFE_Cfg_Status->global_reset_fpga, reset_value);
				return -EINVAL;
			}
		} break;
		case (OFE_RESET_SUBSYS_PCB): {
			// Modify
			lv_ofe_cfg_status.pcb_rstn = reset_value;

			// Write
			sys_write32 (*(uint32_t*)&lv_ofe_cfg_status, (mem_addr_t)s_OFE_Cfg_Status);

			if (s_OFE_Cfg_Status->pcb_rstn != reset_value) {
				LOG_ERR("%s(%d) Error Setting pcb reset bit: %d != %d\r\n", \
				__func__, __LINE__, s_OFE_Cfg_Status->pcb_rstn, reset_value);
				return -EINVAL;
			}
		} break;
		default: {
			LOG_ERR("%s(%d) Not Implemented Element Type:%d\r\n", __func__, __LINE__, inType);
			return -ENOSYS;
		} break;
  	}

	return 0;
}	

static int ofe_init(const struct device *dev)
{
	int error = 0;
	const struct ofe_config *lvConfig = (struct ofe_config*)dev->config;
	s_OFE_Cfg_Status = ((struct ofe_cfg_status *)(lvConfig->base));
	// Initialize the pad controller pins for Config Done and Error Status
	error = pinctrl_apply_state(lvConfig->pcfg, PINCTRL_STATE_DEFAULT);
	return error;
}

static const struct ofe_driver_api s_ofe_api = {
	.get_xcb_config_status = get_xcb_config_status,
	.reset = reset,
	.set_config_status = set_config_status
};

#define OFE_DEVICE_DT_DEFINE(node_id)						\
	PINCTRL_DT_DEFINE(node_id);								\
															\
	static const struct ofe_config s_ofe_config = {			\
		.base = DT_REG_ADDR(DT_NODELABEL(ofe)),				\
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(node_id)			\
	};														\
															\
	DEVICE_DT_DEFINE(										\
						node_id,		 					\
						ofe_init, 							\
						NULL, 								\
						NULL, 								\
						&s_ofe_config, 						\
						POST_KERNEL, 						\
						CONFIG_RAPIDSI_OFE_INIT_PRIORITY, 	\
						&s_ofe_api							\
					)										\

DT_FOREACH_STATUS_OKAY(DT_DRV_COMPAT, OFE_DEVICE_DT_DEFINE)
