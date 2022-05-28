/*
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_GSM_PPP_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_GSM_PPP_H_

#define GSM_PPP_MDM_MANUFACTURER_LENGTH  10
#define GSM_PPP_MDM_MODEL_LENGTH         16
#define GSM_PPP_MDM_REVISION_LENGTH      64
#define GSM_PPP_MDM_IMEI_LENGTH          16
#define GSM_PPP_MDM_IMSI_LENGTH          16
#define GSM_PPP_MDM_ICCID_LENGTH         32

struct gsm_ppp_modem_info {
	char mdm_manufacturer[GSM_PPP_MDM_MANUFACTURER_LENGTH];
	char mdm_model[GSM_PPP_MDM_MODEL_LENGTH];
	char mdm_revision[GSM_PPP_MDM_REVISION_LENGTH];
	char mdm_imei[GSM_PPP_MDM_IMEI_LENGTH];
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	char mdm_imsi[GSM_PPP_MDM_IMSI_LENGTH];
	char mdm_iccid[GSM_PPP_MDM_ICCID_LENGTH];
#endif
	int  mdm_rssi;
};

struct modem_context;
struct k_sem;
/**
 * @brief Application specific modem setup code.
 *
 * This function is a NOP by default, but weakly linked so that it can be
 * explicitly implemented by the application if they chose, but gets optimized
 * out if they don't (similar to how we do with main()). This is called after
 * the general modem setup, just before the PDP context is initialized. Typical
 * use cases for this would be to disable GSM or customize the bandmasks
 * according to regional differences.
 *
 * @param context pointer to modem info
 * @param sem_response semaphore to be passed to modem_cmd_send
 */
void gsm_ppp_application_setup(struct modem_context *context,
			       struct k_sem *sem_response);

/** @cond INTERNAL_HIDDEN */
struct device;
typedef void (*gsm_modem_power_cb)(const struct device *, void *);

void gsm_ppp_start(const struct device *dev);
void gsm_ppp_stop(const struct device *dev);
/** @endcond */

/**
 * @brief Register functions callbacks for power modem on/off.
 *
 * @param dev: gsm modem device
 * @param modem_on: callback function to
 *		execute during gsm ppp configuring.
 * @param modem_off: callback function to
 *		execute during gsm ppp stopping.
 * @param user_data: user specified data
 */
void gsm_ppp_register_modem_power_callback(const struct device *dev,
					   gsm_modem_power_cb modem_on,
					   gsm_modem_power_cb modem_off,
					   void *user_data);

/**
 * @brief Get GSM modem information.
 *
 * @param dev: GSM modem device.
 *
 * @retval struct gsm_ppp_modem_info * pointer to modem information structure.
 */
const struct gsm_ppp_modem_info *gsm_ppp_modem_info(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_GSM_PPP_H_ */
