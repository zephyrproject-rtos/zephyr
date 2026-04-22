/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Audio Class 2 device public header
 *
 * This header describes only class API interaction with application.
 * The audio device itself is modelled with devicetree zephyr,uac2 compatible.
 *
 * This API is currently considered experimental.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_UAC2_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_UAC2_H_

#include <zephyr/device.h>

/** Feature Unit Control Selectors (UAC2 Table A-23) */
enum usb_audio_fucs {
	/** Control undefined (UAC1 and UAC2) */
	USB_AUDIO_FU_CONTROL_UNDEFINED = 0x00,
	/** Mute control (UAC1 and UAC2) */
	USB_AUDIO_FU_MUTE_CONTROL = 0x01,
	/** Volume control (UAC1 and UAC2) */
	USB_AUDIO_FU_VOLUME_CONTROL = 0x02,
	/** Bass control (UAC1 and UAC2) */
	USB_AUDIO_FU_BASS_CONTROL = 0x03,
	/** Mid control (UAC1 and UAC2) */
	USB_AUDIO_FU_MID_CONTROL = 0x04,
	/** Treble control (UAC1 and UAC2) */
	USB_AUDIO_FU_TREBLE_CONTROL = 0x05,
	/** Graphic equalizer control (UAC1 and UAC2) */
	USB_AUDIO_FU_GRAPHIC_EQUALIZER_CONTROL = 0x06,
	/** Automatic gain control (UAC1 and UAC2) */
	USB_AUDIO_FU_AUTOMATIC_GAIN_CONTROL = 0x07,
	/** Delay control (UAC1 and UAC2) */
	USB_AUDIO_FU_DELAY_CONTROL = 0x08,
	/** Bass boost control (UAC1 and UAC2) */
	USB_AUDIO_FU_BASS_BOOST_CONTROL = 0x09,
	/** Loudness control (UAC1 and UAC2) */
	USB_AUDIO_FU_LOUDNESS_CONTROL = 0x0A,
	/** Input gain control (UAC2 only) */
	USB_AUDIO_FU_INPUT_GAIN_CONTROL = 0x0B,
	/** Input gain pad control (UAC2 only) */
	USB_AUDIO_FU_INPUT_GAIN_PAD_CONTROL = 0x0C,
	/** Phase inverter control (UAC2 only) */
	USB_AUDIO_FU_PHASE_INVERTER_CONTROL = 0x0D,
	/** Underflow control (UAC2 only) */
	USB_AUDIO_FU_UNDERFLOW_CONTROL = 0x0E,
	/** Overflow control (UAC2 only) */
	USB_AUDIO_FU_OVERFLOW_CONTROL = 0x0F,
	/** Latency control (UAC2 only) */
	USB_AUDIO_FU_LATENCY_CONTROL = 0x10
};

/**
 * @brief USB Audio Class 2 device API
 * @defgroup uac2_device USB Audio Class 2 device API
 * @ingroup usb
 * @since 3.6
 * @version 0.2.0
 * @{
 */

/**
 * @brief Structure to describe a single sub-range of an audio control.
 */
struct uac2_range_subrange {
	/** Minimum value */
	int32_t min;
	/** Maximum value */
	int32_t max;
	/** Resolution (step size) */
	uint32_t res;
};

/**
 * @brief Structure to describe the full range of a control.
 *
 * The application is responsible for the memory management of this structure.
 * Use @ref UAC2_RANGE_DEFINE define a range with a specific
 * number of subranges.
 */
struct uac2_range {
	/** Number of sub-ranges */
	uint16_t num_subranges;
	/** Array of sub-ranges */
	struct uac2_range_subrange ranges[];
};

/**
 * @brief Define a uac2_range with a specific number of subranges.
 *
 * @param _name Variable name
 * @param _count Number of subranges
 */
#define UAC2_RANGE_DEFINE(_name, _count) \
	struct { \
		uint16_t num_subranges; \
		struct uac2_range_subrange ranges[_count]; \
	} _name

/**
 * @brief Get entity ID
 *
 * @param node node identifier
 */
#define UAC2_ENTITY_ID(node)							\
	({									\
		BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_PARENT(node), zephyr_uac2));	\
		UTIL_INC(DT_NODE_CHILD_IDX(node));				\
	})

/**
 * @brief USB Audio 2 Feature Unit event handlers
 */
struct uac2_feature_unit_ops {
	/**
	 * @brief Callback for host SET CUR requests on a Feature Unit.
	 * @param[in] buf Raw buffer from the host containing the new value.
	 * @param dev USB Audio 2 device
	 * @param entity_id Feature Unit ID
	 * @param control_selector Control selector (mute, volume, etc.)
	 * @param channel_num Channel number (0 = master, 1+ = individual channels)
	 * @param buf Buffer containing the new control value
	 * @param user_data Opaque user data pointer
	 * @return 0 on success, negative value on error
	 */
	int (*set_cur_cb)(const struct device *dev, uint8_t entity_id,
			  enum usb_audio_fucs control_selector, uint8_t channel_num,
			  const struct net_buf *buf, void *user_data);

	/**
	 * @brief Callback for host GET CUR requests on a Feature Unit.
	 * @param dev USB Audio 2 device
	 * @param entity_id Feature Unit ID
	 * @param control_selector Control selector (mute, volume, etc.)
	 * @param channel_num Channel number (0 = master, 1+ = individual channels)
	 * @param[out] value Pointer to be filled with the current control value.
	 * @param user_data Opaque user data pointer
	 * @return 0 on success, negative value on error
	 */
	int (*get_cur_cb)(const struct device *dev, uint8_t entity_id,
			  enum usb_audio_fucs control_selector, uint8_t channel_num,
			  uint32_t *value, void *user_data);

	/**
	 * @brief Callback for host GET RANGE requests on a Feature Unit.
	 *
	 * The application is responsible for the memory management of the
	 * returned uac2_range structure. The pointer returned can point to
	 * a structure in read-only memory (ROM) to save RAM.
	 *
	 * @param dev USB Audio 2 device
	 * @param entity_id Feature Unit ID
	 * @param control_selector Control selector (mute, volume, etc.)
	 * @param channel_num Channel number (0 = master, 1+ = individual channels)
	 * @param user_data Opaque user data pointer
	 *
	 * @return Pointer to the uac2_range struct on success, NULL on error.
	 */
	const struct uac2_range *(*get_range_cb)(const struct device *dev,
						 uint8_t entity_id,
						 enum usb_audio_fucs control_selector,
						 uint8_t channel_num,
						 void *user_data);
};

/**
 * @brief USB Audio 2 application event handlers
 */
struct uac2_ops {
	/**
	 * @brief Start of Frame callback
	 *
	 * Notifies application about SOF event on the bus.
	 * This callback is mandatory to register.
	 *
	 * @param dev USB Audio 2 device
	 * @param user_data Opaque user data pointer
	 */
	void (*sof_cb)(const struct device *dev, void *user_data);
	/**
	 * @brief Terminal update callback
	 *
	 * Notifies application that host has enabled or disabled a terminal.
	 * This callback is mandatory to register.
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Terminal ID linked to AudioStreaming interface
	 * @param enabled True if host enabled terminal, False otherwise
	 * @param microframes True if USB connection speed uses microframes
	 * @param user_data Opaque user data pointer
	 */
	void (*terminal_update_cb)(const struct device *dev, uint8_t terminal,
				   bool enabled, bool microframes,
				   void *user_data);
	/**
	 * @brief Get receive buffer address
	 *
	 * USB stack calls this function to obtain receive buffer address for
	 * AudioStreaming interface. The buffer is owned by USB stack until
	 * @ref data_recv_cb callback is called. The buffer must be sufficiently
	 * aligned and otherwise suitable for use by UDC driver.
	 * This callback is mandatory to register for devices receiving USB audio from the USB host.
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Input Terminal ID linked to AudioStreaming interface
	 * @param size Maximum number of bytes USB stack will write to buffer.
	 * @param user_data Opaque user data pointer
	 */
	void *(*get_recv_buf)(const struct device *dev, uint8_t terminal,
			      uint16_t size, void *user_data);
	/**
	 * @brief Data received
	 *
	 * This function releases buffer obtained in @ref get_recv_buf after USB
	 * has written data to the buffer and/or no longer needs it.
	 * This callback is mandatory to register for devices receiving USB audio from the USB host.
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Input Terminal ID linked to AudioStreaming interface
	 * @param buf Buffer previously obtained via @ref get_recv_buf
	 * @param size Number of bytes written to buffer
	 * @param user_data Opaque user data pointer
	 */
	void (*data_recv_cb)(const struct device *dev, uint8_t terminal,
			     void *buf, uint16_t size, void *user_data);
	/**
	 * @brief Transmit buffer release callback
	 *
	 * This function releases buffer provided in @ref usbd_uac2_send when
	 * the class no longer needs it.
	 * This callback is mandatory to register if calling @ref usbd_uac2_send.
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Output Terminal ID linked to AudioStreaming interface
	 * @param buf Buffer previously provided via @ref usbd_uac2_send
	 * @param user_data Opaque user data pointer
	 */
	void (*buf_release_cb)(const struct device *dev, uint8_t terminal,
			       void *buf, void *user_data);
	/**
	 * @brief Get Explicit Feedback value
	 *
	 * Explicit feedback value format depends terminal connection speed.
	 * If device is High-Speed capable, it must use Q16.16 format if and
	 * only if the @ref terminal_update_cb was called with microframes
	 * parameter set to true. On Full-Speed only devices, or if High-Speed
	 * capable device is operating at Full-Speed (microframes was false),
	 * the format is Q10.14 stored on 24 least significant bits (i.e. 8 most
	 * significant bits are ignored).
	 * This callback is mandatory to register if there is USB Audio Streaming interface linked
	 * to Input Terminal clocked from asynchronous clock (i.e. clock source without
	 * sof-synchronized;) and there is no implicit-feedback; on the interface.
	 *
	 * @param dev USB Audio 2 device
	 * @param terminal Input Terminal ID whose feedback should be returned
	 * @param user_data Opaque user data pointer
	 */
	uint32_t (*feedback_cb)(const struct device *dev, uint8_t terminal,
				void *user_data);
	/**
	 * @brief Get active sample rate
	 *
	 * USB stack calls this function when the host asks for active sample
	 * rate if the Clock Source entity supports more than one sample rate.
	 * This function won't ever be called (should be NULL) if all Clock
	 * Source entities support only one sample rate.
	 *
	 * @param dev USB Audio 2 device
	 * @param clock_id Clock Source ID whose sample rate should be returned
	 * @param user_data Opaque user data pointer
	 *
	 * @return Active sample rate in Hz
	 */
	uint32_t (*get_sample_rate)(const struct device *dev, uint8_t clock_id,
				    void *user_data);
	/**
	 * @brief Set active sample rate
	 *
	 * USB stack calls this function when the host sets active sample rate.
	 * This callback may be NULL if all Clock Source entities have only one
	 * sample rate. USB stack sanitizes the sample rate to closest valid
	 * rate for given Clock Source entity.
	 *
	 * @param dev USB Audio 2 device
	 * @param clock_id Clock Source ID whose sample rate should be set
	 * @param rate Sample rate in Hz
	 * @param user_data Opaque user data pointer
	 *
	 * @return 0 on success, negative value on error
	 */
	int (*set_sample_rate)(const struct device *dev, uint8_t clock_id,
			       uint32_t rate, void *user_data);

	/**
	 * @brief Feature Unit operations callback structure
	 *
	 * Pointer to feature unit operations. Set to NULL if feature units
	 * are not used or not supported by the application.
	 */
	const struct uac2_feature_unit_ops *feature_unit_ops;
};

/**
 * @brief Register USB Audio 2 application callbacks.
 *
 * @param dev USB Audio 2 device instance
 * @param ops USB Audio 2 callback structure
 * @param user_data Opaque user data to pass to ops callbacks
 */
void usbd_uac2_set_ops(const struct device *dev,
		       const struct uac2_ops *ops, void *user_data);

/**
 * @brief Send audio data to output terminal
 *
 * Data buffer must be sufficiently aligned and otherwise suitable for use by
 * UDC driver.
 *
 * @note Buffer ownership is transferred to the stack in case of success, in
 * case of an error the caller retains the ownership of the buffer.
 *
 * @param dev USB Audio 2 device
 * @param terminal Output Terminal ID linked to AudioStreaming interface
 * @param data Buffer containing outgoing data
 * @param size Number of bytes to send
 *
 * @return 0 on success, negative value on error
 */
int usbd_uac2_send(const struct device *dev, uint8_t terminal,
		   void *data, uint16_t size);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_UAC2_H_ */
