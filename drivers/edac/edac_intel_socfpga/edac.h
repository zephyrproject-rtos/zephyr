/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_EDAC_EDAC_INTEL_SOCFPGA_EDAC_H_
#define ZEPHYR_INCLUDE_DRIVERS_EDAC_EDAC_INTEL_SOCFPGA_EDAC_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>

#define INJECT_DBE (1u)
#define INJECT_SBE (2u)

#define DDR_ECC_DBE_STATUS   BIT(1)
#define DDR_OCRAM_DBE_STATUS BIT(0)
/**
 * @brief Get the cold reset boot scratch register 3 value
 *
 * @return cold reset boot scratch register 3 value
 */
uint32_t read_boot_scratch_cold3_reg(const struct device *sysmngr_dev);

/**
 * @brief Define the callback function signature for
 * `set_ecc_error_cb()` function.
 *
 * @param dev       ECC device structure
 * @param dbe		Double bit error status
 * @param sbe		Single Bit Error status
 * @param user_data Pointer to data specified by user
 */
typedef void (*edac_callback_t)(const struct device *dev, bool dbe, bool sbe, void *user_data);

#if defined(CONFIG_EDAC_ERROR_INJECT)
/**
 * @brief to inject a double bit or single bit error
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id HPS peripheral ECC module ID or EMIF interface ID
 * @param error_type type of error to be injected (DBE or SBE).
 * @return  0 Error injection is success
 *			-EINVAL in case of invalid error_type parameter
 *			-ENODEV in case of ECC is not initialised or disabled
 *			-EBUSY in case ecc error injection timeout
 */
typedef int (*edac_inject_ecc_error_t)(const struct device *dev, uint32_t id, uint32_t error_type);
#endif

/**
 * @brief Function to set a callback function for reporting ECC error.
 *        The callback will be called from respective ECC driver ISR if an ECC error occurs.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cb ECC error count, describes ECC info buffer
 * @param user_data Pointer to the ECC info buffer.
 *
 * @return -EINVAL if invalid value passed for input parameter 'cb'.
 */
typedef int (*edac_set_ecc_error_cb_t)(const struct device *dev, edac_callback_t cb,
				       void *user_data);

/**
 * @brief to get the single bit error count
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id HPS peripheral ECC module ID or EMIF interface ID
 * @return  >= 0 Get SB error count success and count value equals to return value
 *			-ENODEV in case of ECC is not initialised or disabled
 */
typedef int (*edac_get_sbe_ecc_error_cnt_t)(const struct device *dev, uint32_t id);


struct edac_ecc_driver_api {
#if defined(CONFIG_EDAC_ERROR_INJECT)
	edac_inject_ecc_error_t inject_ecc_error;
#endif
	edac_set_ecc_error_cb_t set_ecc_error_cb;
	edac_get_sbe_ecc_error_cnt_t get_sbe_ecc_err_cnt;
};

/**
 * @brief to get the single bit error count
 */
void process_serror_for_hps_dbe(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_EDAC_EDAC_INTEL_SOCFPGA_EDAC_H_ */
