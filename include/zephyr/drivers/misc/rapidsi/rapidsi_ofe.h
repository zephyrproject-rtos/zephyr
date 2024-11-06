/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the Rapid Silicon One Flow Engine
 */

#ifndef INCLUDE_ZEPHYR_DRIVEMISC_RAPIDSI_OFE_H_
#define INCLUDE_ZEPHYR_DRIVEMISC_RAPIDSI_OFE_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif


/* *********************************************
 * OFE_RESET_TYPE lists the elements which can
 * be reset by the OFE module.
 * ********************************************/
enum OFE_RESET_SUBSYS_TYPE { OFE_RESET_SUBSYS_FCB = 0, OFE_RESET_SUBSYS_PCB = 1 };

/* *********************************************
 * OFE_STATUS_SUBSYS_TYPE lists the elements whose config
 * status can be read and config done/error can
 * be set.
 * ********************************************/
enum OFE_STATUS_SUBSYS_TYPE { OFE_STATUS_SUBSYS_FCB = 0, OFE_STATUS_SUBSYS_ICB = 1 };

/* *********************************************
 * OFE_CFG_STATUS lists the ofe_cfg_status
 * register elements config_done and config_error to
 * set their value.
 * ********************************************/
enum OFE_CFG_STATUS { OFE_CFG_STATUS_DONE = 0, OFE_CFG_STATUS_ERROR = 1 };

/* *********************************************
 * ofe_cfg_status status of configurations of
 * FCB, ICB and CCB.
 * ********************************************/
struct ofe_cfg_status {
  volatile uint32_t fcb_cfg_status : 1;
  volatile uint32_t icb_cfg_status : 1;
  volatile uint32_t reserved_1 : 2;
  volatile uint32_t pcb_rstn : 1;
  volatile uint32_t reserved_2 : 2;
  volatile uint32_t cfg_done : 1;
  volatile uint32_t reserved_3 : 7;
  volatile uint32_t cfg_error : 1;
  volatile uint32_t rwm_control_fpga : 3;
  volatile uint32_t reserved_4 : 12;
  volatile uint32_t global_reset_fpga : 1;
};

typedef int (*ofe_api_get_xcb_config_status)(
                                                const struct device *dev, 
                                                enum OFE_STATUS_SUBSYS_TYPE inElem, 
                                                bool *status
                                            );
typedef int (*ofe_api_set_config_status)(
                                         const struct device *dev, 
                                         enum OFE_CFG_STATUS inElem, 
                                         bool status
                                        );                                        
typedef int (*ofe_api_reset)(
                                const struct device *dev, 
                                enum OFE_RESET_SUBSYS_TYPE inType,
                                bool reset_value
                            );

__subsystem struct ofe_driver_api {
    ofe_api_get_xcb_config_status get_xcb_config_status;
    ofe_api_set_config_status set_config_status;
    ofe_api_reset   reset;
};

/* ******************************************
 * @brief: ofe_get_config_status will return
 * the status of configuration for FCB or ICB.
 *
 * @param [in]: OFE register element for
 * configuration status; e.g, FCB, ICB etc
 *
 * @return: true or false
 * ******************************************/
static inline int ofe_get_xcb_config_status(
                                            const struct device *dev, 
                                            enum OFE_STATUS_SUBSYS_TYPE inElem, 
                                            bool *status
                                        )
{
    struct ofe_driver_api *api = (struct ofe_driver_api*)dev->api;

    if(api->get_xcb_config_status == NULL) {
        return -ENOSYS;
    }

    return api->get_xcb_config_status(dev, inElem, status);
}                                        

/* ******************************************
 * @brief: ofe_set_config_status is used to
 * either set true or false to the
 * config_done or config_error bits in OFE
 * status register.
 *
 * @param [in]: bool
 *
 * @return: void
 * ******************************************/
static inline int ofe_set_config_status(
                                         const struct device *dev, 
                                         enum OFE_CFG_STATUS inElem, 
                                         bool inValue
                                       )
{
    struct ofe_driver_api *api = (struct ofe_driver_api*)dev->api;

    if(api->set_config_status == NULL) {
        return -ENOSYS;
    }

    return api->set_config_status(dev, inElem, inValue);
}                                        

/* ******************************************
 * @brief: ofe_reset sets or resets the 
 * reset bit in the ofe_status register
 * for the type of supported subsystem reset.
 * 
 * @param [in]: dev device structure
 * @param [in]: bool to set or reset pcb
 *
 * @return: error code
 * ******************************************/
static inline int ofe_reset(
                                const struct device *dev, 
                                enum OFE_RESET_SUBSYS_TYPE inType,
                                bool reset_value
                           )
{
    struct ofe_driver_api *api = (struct ofe_driver_api*)dev->api;

    if(api->reset == NULL) {
        return -ENOSYS;
    }

    return api->reset(dev, inType, reset_value);
}                              


#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZEPHYR_DRIVEMISC_RAPIDSI_OFE_H_ */
