/** @file
 *  @brief Bluetooth subsystem classic core APIs.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_H_

/**
 * @brief Bluetooth APIs
 * @defgroup bluetooth Bluetooth APIs
 * @ingroup connectivity
 * @{
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic Access Profile (GAP)
 * @defgroup bt_gap Generic Access Profile (GAP)
 * @since 1.0
 * @version 1.0.0
 * @ingroup bluetooth
 * @{
 */

/**
 * @private
 * @brief BR/EDR discovery private structure
 */
struct bt_br_discovery_priv {
	/** Clock offset */
	uint16_t clock_offset;
	/** Page scan repetition mode */
	uint8_t pscan_rep_mode;
	/** Resolving remote name*/
	uint8_t resolve_state;
};

/** Maximum size of Extended Inquiry Response */
#define BT_BR_EIR_SIZE_MAX 240

/** @brief BR/EDR discovery result structure */
struct bt_br_discovery_result {
	/** Private data */
	struct bt_br_discovery_priv _priv;

	/** Remote device address */
	bt_addr_t addr;

	/** RSSI from inquiry */
	int8_t rssi;

	/** Class of Device */
	uint8_t cod[3];

	/** Extended Inquiry Response */
	uint8_t eir[BT_BR_EIR_SIZE_MAX];
};

/** BR/EDR discovery parameters */
struct bt_br_discovery_param {
	/** Maximum length of the discovery in units of 1.28 seconds.
	 *  Valid range is 0x01 - 0x30.
	 */
	uint8_t length;

	/** True if limited discovery procedure is to be used. */
	bool limited;
};

/**
 * @brief Start BR/EDR discovery
 *
 * Start BR/EDR discovery (inquiry) and provide results through the specified
 * callback. The discovery results will be notified through callbacks
 * registered by @ref bt_br_discovery_cb_register.
 * If more inquiry results were received during session than
 * fits in provided result storage, only ones with highest RSSI will be
 * reported.
 *
 * @param param Discovery parameters.
 * @param results Storage for discovery results.
 * @param count Number of results in storage. Valid range: 1-255.
 *
 * @return Zero on success or error code otherwise, positive in case
 * of protocol error or negative (POSIX) in case of stack internal error
 */
int bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  struct bt_br_discovery_result *results, size_t count);

/**
 * @brief Stop BR/EDR discovery.
 *
 * Stops ongoing BR/EDR discovery. If discovery was stopped by this call
 * results won't be reported
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_br_discovery_stop(void);

struct bt_br_discovery_cb {

	/**
	 * @brief An inquiry response received callback.
	 *
	 * @param result Storage used for discovery results
	 */
	void (*recv)(const struct bt_br_discovery_result *result);

	/** @brief The inquiry has stopped after discovery timeout.
	 *
	 * @param results Storage used for discovery results
	 * @param count Number of valid discovery results.
	 */
	void (*timeout)(const struct bt_br_discovery_result *results,
				  size_t count);

	sys_snode_t node;
};

/**
 * @brief Register discovery packet callbacks.
 *
 * Adds the callback structure to the list of callback structures that monitors
 * inquiry activity.
 *
 * This callback will be called for all inquiry activity, regardless of what
 * API was used to start the discovery.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_br_discovery_cb_register(struct bt_br_discovery_cb *cb);

/**
 * @brief Unregister discovery packet callbacks.
 *
 * Remove the callback structure from the list of discovery callbacks.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_br_discovery_cb_unregister(struct bt_br_discovery_cb *cb);

struct bt_br_oob {
	/** BR/EDR address. */
	bt_addr_t addr;
};

/**
 * @brief Get BR/EDR local Out Of Band information
 *
 * This function allows to get local controller information that are useful
 * for Out Of Band pairing or connection creation process.
 *
 * @param oob Out Of Band information
 */
int bt_br_oob_get_local(struct bt_br_oob *oob);

/**
 * @brief Enable/disable set controller in discoverable state.
 *
 * Allows make local controller to listen on INQUIRY SCAN channel and responds
 * to devices making general inquiry. To enable this state it's mandatory
 * to first be in connectable state.
 *
 * If the device enters limited discoverable mode, the controller will leave from discoverable
 * mode after the duration of @kconfig{BT_LIMITED_DISCOVERABLE_DURATION} seconds in the limited
 * discoverable mode.
 *
 * @param enable Value allowing/disallowing controller to become discoverable.
 * @param limited Value allowing/disallowing controller to enter limited discoverable mode.
 *
 * @return Negative if fail set to requested state or requested state has been
 *         already set. Zero if done successfully.
 */
int bt_br_set_discoverable(bool enable, bool limited);

/**
 * @brief Enable/disable set controller in connectable state.
 *
 * Allows make local controller to be connectable. It means the controller
 * start listen to devices requests on PAGE SCAN channel. If disabled also
 * resets discoverability if was set.
 *
 * @param enable Value allowing/disallowing controller to be connectable.
 *
 * @return Negative if fail set to requested state or requested state has been
 *         already set. Zero if done successfully.
 */
int bt_br_set_connectable(bool enable);

#define BT_BR_SCAN_INTERVAL_MIN 0x0012 /* 0x0012, 11.25ms, U:0.625 */
#define BT_BR_SCAN_INTERVAL_MAX 0x1000 /* 0x1000, 2.560s, U:0.625 */

#define BT_BR_SCAN_WINDOW_MIN 0x0011 /* 0x0011, 10.625ms, U:0.625 */
#define BT_BR_SCAN_WINDOW_MAX 0x1000 /* 0x1000, 2.560s, U:0.625 */

/**
 * @name Defined BR/EDR Page Scan Intervals and Windows
 * @{
 */
#define BT_BR_PAGE_SCAN_INTERVAL_R0 0x0800 /* 0x0800, 1.280s, U:0.625 */
#define BT_BR_PAGE_SCAN_WINDOW_R0   0x0800 /* 0x0800, 1.280s, U:0.625 */

#define BT_BR_PAGE_SCAN_FAST_INTERVAL_R1 0x00a0 /* 0x00a0, 100.0ms, U:0.625 */
#define BT_BR_PAGE_SCAN_FAST_WINDOW_R1   0x0011 /* 0x0011, 10.625ms, U:0.625 */

#define BT_BR_PAGE_SCAN_MEDIUM_INTERVAL_R1 0x0800 /* 0x0800, 1.280s, U:0.625 */
#define BT_BR_PAGE_SCAN_MEDIUM_WINDOW_R1   0x0011 /* 0x0011, 10.625ms, U:0.625 */

#define BT_BR_PAGE_SCAN_SLOW_INTERVAL_R1 0x0800 /* 0x0800, 1.280s, U:0.625 */
#define BT_BR_PAGE_SCAN_SLOW_WINDOW_R1   0x0011 /* 0x0011, 10.625ms, U:0.625 */

#define BT_BR_PAGE_SCAN_FAST_INTERVAL_R2 0x1000 /* 0x1000, 2.560s, U:0.625 */
#define BT_BR_PAGE_SCAN_FAST_WINDOW_R2   0x0011 /* 0x0011, 10.625ms, U:0.625 */

#define BT_BR_PAGE_SCAN_SLOW_INTERVAL_R2 0x1000 /* 0x1000, 2.560s, U:0.625 */
#define BT_BR_PAGE_SCAN_SLOW_WINDOW_R2   0x0011 /* 0x0011, 10.625ms, U:0.625 */
/**
 * @}
 */

/** Page scan type. */
enum bt_br_scan_type {
	/** Standard scan (default) */
	BT_BR_SCAN_TYPE_STANDARD = 0,

	/** Interlaced scan (1.2 devices only) */
	BT_BR_SCAN_TYPE_INTERLACED = 1,
};

struct bt_br_page_scan_param {
	/** Page scan interval in 0.625 ms units
	 *  Range: 0x0012 to 0x1000; only even values are valid.
	 */
	uint16_t interval;

	/** Page scan window in 0.625 ms units
	 *  Range: 0x0011 to 0x1000.
	 */
	uint16_t window;

	/** Page scan type. */
	enum bt_br_scan_type type;
};

/**
 * @brief Initialize BR Scan parameters
 *
 * @param _interval  Scan interval
 * @param _window    Scan window
 * @param _type      Scan type
 */

#define BT_BR_SCAN_INIT(_interval, _window, _type) \
{ \
	.interval = (_interval), \
	.window = (_window), \
	.type = (_type) \
}

/**
 * Helper to declare BR/EDR page scan parameters inline
 *
 * @param _interval page scan interval, N * 0.625 milliseconds
 * @param _window   page scan window, N * 0.625 milliseconds
 * @param _type    BT_BR_SCAN_TYPE_STANDARD or BT_BR_SCAN_TYPE_INTERLACED
 */

#define BT_BR_PAGE_SCAN_PARAM(_interval, _window, _type) \
	((const struct bt_br_page_scan_param[]) { \
		BT_BR_SCAN_INIT(_interval, _window, _type) \
	})

/**
 * @brief Default page scan parameters for R0
 *
 * Page scan interval and window are set to 1.280 seconds (0x0800 in 0.625 ms units).
 * The scan type is set to standard.
 */
#define BT_BR_PAGE_SCAN_PARAM_R0 BT_BR_PAGE_SCAN_PARAM(BT_BR_PAGE_SCAN_INTERVAL_R0, \
						       BT_BR_PAGE_SCAN_WINDOW_R0, \
						       BT_BR_SCAN_TYPE_STANDARD)

/**
 * @brief Fast page scan parameters for R1
 *
 * Page scan interval is set to 100 ms (0x00A0 in 0.625 ms units), and the
 * page scan window is set to 10.240 seconds (0x27FF in 1 ms units).
 * The scan type is set to interlaced.
 */
#define BT_BR_PAGE_SCAN_PARAM_FAST_R1 \
	BT_BR_PAGE_SCAN_PARAM(BT_BR_PAGE_SCAN_FAST_INTERVAL_R1, \
	BT_BR_PAGE_SCAN_FAST_WINDOW_R1, \
	BT_BR_SCAN_TYPE_INTERLACED)

/**
 * @brief Medium page scan parameters for R1
 *
 * Page scan interval and window are set to 1.280 seconds (0x0800 in 0.625 ms units).
 * The scan type is set to standard.
 */
#define BT_BR_PAGE_SCAN_PARAM_MEDIUM_R1 \
	BT_BR_PAGE_SCAN_PARAM( \
	BT_BR_PAGE_SCAN_MEDIUM_INTERVAL_R1, \
	BT_BR_PAGE_SCAN_MEDIUM_WINDOW_R1, \
	BT_BR_SCAN_TYPE_INTERLACED)

/**
 * @brief Slow page scan parameters for R1
 *
 * Page scan interval and window are set to 1.280 seconds (0x0800 in 0.625 ms units).
 * The scan type is set to standard.
 */
#define BT_BR_PAGE_SCAN_PARAM_SLOW_R1 \
	BT_BR_PAGE_SCAN_PARAM( \
	BT_BR_PAGE_SCAN_SLOW_INTERVAL_R1, \
	BT_BR_PAGE_SCAN_SLOW_WINDOW_R1, \
	BT_BR_SCAN_TYPE_STANDARD)

/**
 * @brief Fast page scan parameters for R2
 *
 * Page scan interval is set to 2.560 seconds (0x1000 in 0.625 ms units), and the
 * page scan window is set to 10.240 seconds (0x27FF in 1 ms units).
 * The scan type is set to standard.
 */
#define BT_BR_PAGE_SCAN_PARAM_FAST_R2 \
	BT_BR_PAGE_SCAN_PARAM( \
	BT_BR_PAGE_SCAN_FAST_INTERVAL_R2, \
	BT_BR_PAGE_SCAN_FAST_WINDOW_R2, \
	BT_BR_SCAN_TYPE_INTERLACED)

/**
 * @brief Slow page scan parameters for R2
 *
 * Page scan interval and window are set to 2.560 seconds (0x1000 in 0.625 ms units).
 * The scan type is set to standard.
 */
#define BT_BR_PAGE_SCAN_PARAM_SLOW_R2 \
	BT_BR_PAGE_SCAN_PARAM( \
	BT_BR_PAGE_SCAN_SLOW_INTERVAL_R2, \
	BT_BR_PAGE_SCAN_SLOW_WINDOW_R2, \
	BT_BR_SCAN_TYPE_STANDARD)

/**
 * @brief Update BR/EDR page scan parameters.
 *
 * This function updates the page scan parameters of the local BR/EDR controller.
 * Page scan parameters determine how the controller listens for incoming
 * connection requests from remote devices.
 *
 * The function validates the provided parameters, including the interval,
 * window, and scan type, and sends the appropriate HCI commands to update
 * the controller's page scan activity and scan type.
 *
 * The user can set custom page scan parameters using the helper macro
 * `BT_BR_PAGE_SCAN_PARAM(interval, window, type)` to define their own values.
 * Alternatively, the user can use predefined standard parameters as defined
 * in the Bluetooth specification:
 * - `BT_BR_PAGE_SCAN_PARAM_R0`: Default page scan parameters.
 * - `BT_BR_PAGE_SCAN_PARAM_FAST_R1`: Fast page scan parameters for R1.
 * - `BT_BR_PAGE_SCAN_PARAM_MEDIUM_R1`: Medium page scan parameters for R1.
 * - `BT_BR_PAGE_SCAN_PARAM_SLOW_R1`: Slow page scan parameters for R1.
 * - `BT_BR_PAGE_SCAN_PARAM_FAST_R2`: Fast page scan parameters for R2.
 * - `BT_BR_PAGE_SCAN_PARAM_SLOW_R2`: Slow page scan parameters for R2.
 *
 * These predefined parameters are designed to meet common use cases and
 * ensure compliance with the Bluetooth specification.
 *
 * @param param Page scan parameters, including:
 *              - interval: Time between consecutive page scans (in 0.625 ms units).
 *                          Must be in the range [0x0012, 0x1000].
 *              - window: Duration of a single page scan (in 0.625 ms units).
 *                        Must be in the range [0x0011, 0x1000].
 *              - type: Page scan type (e.g., standard or interlaced).
 *
 * @return 0 on success.
 * @return -EINVAL if the provided parameters are invalid.
 * @return -EAGAIN if the device is not ready.
 * @return -ENOBUFS if memory allocation for HCI commands fails.
 * @return Other negative error codes for internal failures.
 */
int bt_br_page_scan_update_param(const struct bt_br_page_scan_param *param);

/**
 * @name Defined BR Class of Device (CoD) values
 * @{
 */

/* Major Service Classes */
#define BT_COD_MAJOR_SVC_CLASS_LIMITED_DISCOVER BIT(13)
#define BT_COD_MAJOR_SVC_CLASS_LE_AUDIO         BIT(14)
#define BT_COD_MAJOR_SVC_CLASS_RESERVED         BIT(15)
#define BT_COD_MAJOR_SVC_CLASS_POSITIONING      BIT(16)
#define BT_COD_MAJOR_SVC_CLASS_NETWORKING       BIT(17)
#define BT_COD_MAJOR_SVC_CLASS_RENDERING        BIT(18)
#define BT_COD_MAJOR_SVC_CLASS_CAPTURING        BIT(19)
#define BT_COD_MAJOR_SVC_CLASS_OBJECT_TRANSFER  BIT(20)
#define BT_COD_MAJOR_SVC_CLASS_AUDIO            BIT(21)
#define BT_COD_MAJOR_SVC_CLASS_TELEPHONY        BIT(22)
#define BT_COD_MAJOR_SVC_CLASS_INFORMATION      BIT(23)

/* Major Device Class */
#define BT_COD_MAJOR_DEVICE_CLASS_MISCELLANEOUS 0x00
#define BT_COD_MAJOR_DEVICE_CLASS_COMPUTER      0x01
#define BT_COD_MAJOR_DEVICE_CLASS_PHONE         0x02
#define BT_COD_MAJOR_DEVICE_CLASS_LAN_NETWORK   0x03
#define BT_COD_MAJOR_DEVICE_CLASS_AUDIO_VIDEO   0x04
#define BT_COD_MAJOR_DEVICE_CLASS_PERIPHERAL    0x05
#define BT_COD_MAJOR_DEVICE_CLASS_IMAGING       0x06
#define BT_COD_MAJOR_DEVICE_CLASS_WEARABLE      0x07
#define BT_COD_MAJOR_DEVICE_CLASS_TOY           0x08
#define BT_COD_MAJOR_DEVICE_CLASS_HEALTH        0x09
#define BT_COD_MAJOR_DEVICE_CLASS_UNCATEGORIZED 0x1F

/* Minor Device Class - Computer Major Class */
#define BT_COD_MINOR_DEVICE_CLASS_COMPUTER_UNCATEGORIZED 0x00
#define BT_COD_MINOR_DEVICE_CLASS_DESKTOP_WORKSTATION    0x01
#define BT_COD_MINOR_DEVICE_CLASS_SERVER                 0x02
#define BT_COD_MINOR_DEVICE_CLASS_LAPTOP                 0x03
#define BT_COD_MINOR_DEVICE_CLASS_HANDHELD_PC_PDA        0x04
#define BT_COD_MINOR_DEVICE_CLASS_PALM_SIZED_PC_PDA      0x05
#define BT_COD_MINOR_DEVICE_CLASS_WEARABLE_COMPUTER      0x06
#define BT_COD_MINOR_DEVICE_CLASS_TABLET                 0x07

/* Minor Device Class - Phone Major Class */
#define BT_COD_MINOR_DEVICE_CLASS_PHONE_UNCATEGORIZED 0x00
#define BT_COD_MINOR_DEVICE_CLASS_CELLULAR            0x01
#define BT_COD_MINOR_DEVICE_CLASS_CORDLESS            0x02
#define BT_COD_MINOR_DEVICE_CLASS_SMARTPHONE          0x03
#define BT_COD_MINOR_DEVICE_CLASS_WIRED_MODEM         0x04
#define BT_COD_MINOR_DEVICE_CLASS_COMMON_ISDN_ACCESS  0x05

/* Minor Device Class - LAN/Network Major Class - Service Utilization */
#define BT_COD_MINOR_DEVICE_CLASS_LAN_FULLY_AVAILABLE      0x00
#define BT_COD_MINOR_DEVICE_CLASS_LAN_1_17_UTILIZED        0x01
#define BT_COD_MINOR_DEVICE_CLASS_LAN_17_33_UTILIZED       0x02
#define BT_COD_MINOR_DEVICE_CLASS_LAN_33_50_UTILIZED       0x03
#define BT_COD_MINOR_DEVICE_CLASS_LAN_50_67_UTILIZED       0x04
#define BT_COD_MINOR_DEVICE_CLASS_LAN_67_83_UTILIZED       0x05
#define BT_COD_MINOR_DEVICE_CLASS_LAN_83_99_UTILIZED       0x06
#define BT_COD_MINOR_DEVICE_CLASS_LAN_NO_SERVICE_AVAILABLE 0x07

/* Minor Device Class - LAN/Network Major Class - Device Type */
#define BT_COD_MINOR_DEVICE_CLASS_LAN_UNCATEGORIZED 0x00

/* Minor Device Class - Audio/Video Major Class */
#define BT_COD_MINOR_DEVICE_CLASS_AV_UNCATEGORIZED          0x00
#define BT_COD_MINOR_DEVICE_CLASS_WEARABLE_HEADSET          0x01
#define BT_COD_MINOR_DEVICE_CLASS_HANDSFREE                 0x02
#define BT_COD_MINOR_DEVICE_CLASS_MICROPHONE                0x04
#define BT_COD_MINOR_DEVICE_CLASS_LOUDSPEAKER               0x05
#define BT_COD_MINOR_DEVICE_CLASS_HEADPHONES                0x06
#define BT_COD_MINOR_DEVICE_CLASS_PORTABLE_AUDIO            0x07
#define BT_COD_MINOR_DEVICE_CLASS_CAR_AUDIO                 0x08
#define BT_COD_MINOR_DEVICE_CLASS_SETTOP_BOX                0x09
#define BT_COD_MINOR_DEVICE_CLASS_HIFI_AUDIO                0x0A
#define BT_COD_MINOR_DEVICE_CLASS_VCR                       0x0B
#define BT_COD_MINOR_DEVICE_CLASS_VIDEO_CAMERA              0x0C
#define BT_COD_MINOR_DEVICE_CLASS_CAMCORDER                 0x0D
#define BT_COD_MINOR_DEVICE_CLASS_VIDEO_MONITOR             0x0E
#define BT_COD_MINOR_DEVICE_CLASS_VIDEO_DISPLAY_LOUDSPEAKER 0x0F
#define BT_COD_MINOR_DEVICE_CLASS_VIDEO_CONFERENCING        0x10
#define BT_COD_MINOR_DEVICE_CLASS_GAMING_TOY                0x12

/* Minor Device Class - Peripheral Major Class - Input Device Type */
#define BT_COD_MINOR_DEVICE_CLASS_PERIPHERAL_NO_KEYBOARD_NO_POINTING 0x00
#define BT_COD_MINOR_DEVICE_CLASS_KEYBOARD                           0x01
#define BT_COD_MINOR_DEVICE_CLASS_POINTING_DEVICE                    0x02
#define BT_COD_MINOR_DEVICE_CLASS_COMBO_KEYBOARD_POINTING            0x03

/* Minor Device Class - Peripheral Major Class - Device Type */
#define BT_COD_MINOR_DEVICE_CLASS_PERIPHERAL_UNCATEGORIZED 0x00
#define BT_COD_MINOR_DEVICE_CLASS_JOYSTICK                 0x01
#define BT_COD_MINOR_DEVICE_CLASS_GAMEPAD                  0x02
#define BT_COD_MINOR_DEVICE_CLASS_REMOTE_CONTROL           0x03
#define BT_COD_MINOR_DEVICE_CLASS_SENSING_DEVICE           0x04
#define BT_COD_MINOR_DEVICE_CLASS_DIGITIZER_TABLET         0x05
#define BT_COD_MINOR_DEVICE_CLASS_CARD_READER              0x06
#define BT_COD_MINOR_DEVICE_CLASS_DIGITAL_PEN              0x07
#define BT_COD_MINOR_DEVICE_CLASS_HANDHELD_SCANNER         0x08
#define BT_COD_MINOR_DEVICE_CLASS_HANDHELD_GESTURAL_INPUT  0x09

/* Minor Device Class - Imaging Major Class */
#define BT_COD_MINOR_DEVICE_CLASS_IMAGING_DISPLAY       0x01
#define BT_COD_MINOR_DEVICE_CLASS_IMAGING_CAMERA        0x02
#define BT_COD_MINOR_DEVICE_CLASS_IMAGING_SCANNER       0x04
#define BT_COD_MINOR_DEVICE_CLASS_IMAGING_PRINTER       0x08
#define BT_COD_MINOR_DEVICE_CLASS_IMAGING_UNCATEGORIZED 0x01

/* Minor Device Class - Wearable Major Class */
#define BT_COD_MINOR_DEVICE_CLASS_WEARABLE_WRISTWATCH 0x01
#define BT_COD_MINOR_DEVICE_CLASS_WEARABLE_PAGER      0x02
#define BT_COD_MINOR_DEVICE_CLASS_WEARABLE_JACKET     0x03
#define BT_COD_MINOR_DEVICE_CLASS_WEARABLE_HELMET     0x04
#define BT_COD_MINOR_DEVICE_CLASS_WEARABLE_GLASSES    0x05

/* Minor Device Class - Toy Major Class */
#define BT_COD_MINOR_DEVICE_CLASS_TOY_ROBOT      0x01
#define BT_COD_MINOR_DEVICE_CLASS_TOY_VEHICLE    0x02
#define BT_COD_MINOR_DEVICE_CLASS_TOY_DOLL       0x03
#define BT_COD_MINOR_DEVICE_CLASS_TOY_CONTROLLER 0x04
#define BT_COD_MINOR_DEVICE_CLASS_TOY_GAME       0x05

/* Minor Device Class - Health Major Class */
#define BT_COD_MINOR_DEVICE_CLASS_HEALTH_UNDEFINED          0x00
#define BT_COD_MINOR_DEVICE_CLASS_BLOOD_PRESSURE_MONITOR    0x01
#define BT_COD_MINOR_DEVICE_CLASS_THERMOMETER               0x02
#define BT_COD_MINOR_DEVICE_CLASS_WEIGHING_SCALE            0x03
#define BT_COD_MINOR_DEVICE_CLASS_GLUCOSE_METER             0x04
#define BT_COD_MINOR_DEVICE_CLASS_PULSE_OXIMETER            0x05
#define BT_COD_MINOR_DEVICE_CLASS_HEART_PULSE_MONITOR       0x06
#define BT_COD_MINOR_DEVICE_CLASS_HEALTH_DATA_DISPLAY       0x07
#define BT_COD_MINOR_DEVICE_CLASS_STEP_COUNTER              0x08
#define BT_COD_MINOR_DEVICE_CLASS_BODY_COMPOSITION_ANALYZER 0x09
#define BT_COD_MINOR_DEVICE_CLASS_PEAK_FLOW_MONITOR         0x0A
#define BT_COD_MINOR_DEVICE_CLASS_MEDICATION_MONITOR        0x0B
#define BT_COD_MINOR_DEVICE_CLASS_KNEE_PROSTHESIS           0x0C
#define BT_COD_MINOR_DEVICE_CLASS_ANKLE_PROSTHESIS          0x0D
#define BT_COD_MINOR_DEVICE_CLASS_GENERIC_HEALTH_MANAGER    0x0E
#define BT_COD_MINOR_DEVICE_CLASS_PERSONAL_MOBILITY_DEVICE  0x0F

/**
 * @}
 */

/**
 * @brief Set the Class of Device configuration parameter of the local
 *        BR/EDR Controller.
 *
 * This function writes the Class of Device (COD) value to the BR/EDR
 * controller. The COD is used by remote devices during the discovery
 * process to identify the type of device and the services it provides.
 *
 * @note The limited discoverable mode bit (BT_COD_MAJOR_SVC_CLASS_LIMITED_DISCOVER)
 *       cannot be set directly through this function. Use bt_br_set_discoverable()
 *       with the limited parameter to enable limited discoverable mode, which will
 *       automatically set the corresponding COD bit.
 *
 * @param cod Class of Device value to set. This should be a combination of:
 *            - Major Service Class bits (BT_COD_MAJOR_SVC_CLASS_*)
 *            - Major Device Class bits (BT_COD_MAJOR_DEVICE_CLASS_*)
 *            - Minor Device Class bits (BT_COD_MINOR_DEVICE_CLASS_*)
 *
 * @retval 0 on success
 * @retval -EAGAIN if the Bluetooth device is not ready
 * @retval -EINVAL if the provided COD value attempts to set the limited
 *         discoverable bit directly, which is not permitted
 * @retval Other negative error codes from underlying HCI command failures
 *
 * @see bt_br_set_discoverable()
 * @see bt_br_get_class_of_device()
 */
int bt_br_set_class_of_device(uint32_t cod);

/**
 * @brief Get the Class of Device configuration parameter of the local
 *        BR/EDR Controller.
 *
 * @param cod Class of Device value.
 *
 * @return Negative if fail set to requested state or requested state has been
 *         already set. Zero if done successfully.
 */
int bt_br_get_class_of_device(uint32_t *cod);

/** @brief Check if a Bluetooth classic device address is bonded.
 *
 *  @param addr Bluetooth classic device address.
 *
 *  @return true if @p addr is bonded
 */
bool bt_br_bond_exists(const bt_addr_t *addr);

/**
 * @brief Clear classic pairing information .
 *
 * @param addr  Remote address, NULL or BT_ADDR_ANY to clear all remote devices.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_br_unpair(const bt_addr_t *addr);

/** Information about a bond with a remote device. */
struct bt_br_bond_info {
	/** Address of the remote device. */
	bt_addr_t addr;
};

/**
 * @brief Iterate through all existing bonds of Classic.
 *
 * @param func       Function to call for each bond.
 * @param user_data  Data to pass to the callback function.
 */
void bt_br_foreach_bond(void (*func)(const struct bt_br_bond_info *info, void *user_data),
			void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_H_ */
