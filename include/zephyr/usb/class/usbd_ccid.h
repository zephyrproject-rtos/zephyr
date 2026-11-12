/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2026 TeraTron GmbH
 */

/**
 * @file
 * @brief Backend registration and card-presence notification for USB CCID interfaces.
 * @ingroup usbd_ccid
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_CCID_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_CCID_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB CCID device class API
 * @defgroup usbd_ccid USB CCID device class
 * @ingroup usb
 *
 * Each class instance exposes one card slot and uses an independent backend.
 * The class supports backend-selected protocols at APDU exchange level. It does not interpret APDU
 * contents or depend on a card transport. Card parameters are backend-owned.
 * Each enabled @c zephyr,ccid devicetree node creates an instance. Its
 * @c instance-id property selects the backend API index and the USB stack
 * registration name @c ccid_<instance-id>. IDs must be unique; gaps are allowed.
 * The number of instances and their resources are derived from enabled nodes.
 * The optional @c label property supplies the USB interface string and defaults
 * to "CCID".
 * @{
 */

/** State of the integrated circuit card exposed by the backend. */
enum usbd_ccid_card_state {
	/** No card is present. */
	USBD_CCID_CARD_ABSENT,
	/** A card is present and powered off. */
	USBD_CCID_CARD_INACTIVE,
	/** A card is present and powered on. */
	USBD_CCID_CARD_ACTIVE,
};

/** CCID protocol numbers (not the dwProtocols descriptor bitmask). */
enum usbd_ccid_protocol {
	USBD_CCID_PROTOCOL_T0 = 0,
	USBD_CCID_PROTOCOL_T1 = 1,
};

#define USBD_CCID_PROTOCOL_MASK(protocol) (1UL << (protocol))

/** APDU reader features from CCID 1.1, section 5.1. */
enum usbd_ccid_feature {
	USBD_CCID_AUTO_PARAMETERS = 0x00000002,
	USBD_CCID_AUTO_ACTIVATE = 0x00000004,
	USBD_CCID_AUTO_VOLTAGE = 0x00000008,
	USBD_CCID_AUTO_CLOCK = 0x00000010,
	USBD_CCID_AUTO_DATA_RATE = 0x00000020,
	USBD_CCID_AUTO_NEGOTIATION = 0x00000040,
	USBD_CCID_AUTO_PPS = 0x00000080,
	USBD_CCID_CLOCK_STOP = 0x00000100,
	USBD_CCID_NONZERO_NAD = 0x00000200,
	USBD_CCID_AUTO_IFSD = 0x00000400,
	USBD_CCID_SHORT_APDU = 0x00020000,
	USBD_CCID_EXTENDED_APDU = 0x00040000,
};

/** Power-on voltage selection from CCID 1.1, section 6.1.1. */
enum usbd_ccid_voltage {
	USBD_CCID_VOLTAGE_AUTO = 0,
	USBD_CCID_VOLTAGE_5V = 1,
	USBD_CCID_VOLTAGE_3V = 2,
	USBD_CCID_VOLTAGE_1V8 = 3,
};

#define USBD_CCID_VOLTAGE_MASK(voltage) (1U << ((voltage) - 1U))

/** Clock state reported in CCID bClockStatus (section 6.2.2). */
enum usbd_ccid_clock_status {
	USBD_CCID_CLOCK_RUNNING = 0,
	USBD_CCID_CLOCK_STOPPED_LOW = 1,
	USBD_CCID_CLOCK_STOPPED_HIGH = 2,
	USBD_CCID_CLOCK_STOPPED_UNKNOWN = 3,
};

/** Card clock-stop capability in bClockStop (section 6.2.3). */
enum usbd_ccid_clock_stop {
	USBD_CCID_CLOCK_STOP_NOT_ALLOWED = 0,
	USBD_CCID_CLOCK_STOP_LOW = 1,
	USBD_CCID_CLOCK_STOP_HIGH = 2,
	USBD_CCID_CLOCK_STOP_EITHER = 3,
};

/** Convention bit in bmTCCKST0 and bmTCCKST1. */
enum usbd_ccid_convention {
	USBD_CCID_CONVENTION_DIRECT = 0x00,
	USBD_CCID_CONVENTION_INVERSE = 0x02,
};

/** Checksum bit in bmTCCKST1. */
enum usbd_ccid_t1_checksum {
	USBD_CCID_T1_CHECKSUM_LRC = 0x00,
	USBD_CCID_T1_CHECKSUM_CRC = 0x01,
};

/** Fixed T=1 bits in bmTCCKST1, combined with convention and checksum. */
#define USBD_CCID_T1_TCCKS_PROTOCOL           0x10
/** ISO/IEC 7816-3 initial FI=1, DI=1: F=372, D=1. */
#define USBD_CCID_FINDEX_DINDEX_DEFAULT       0x11
/** No extra guard time. */
#define USBD_CCID_GUARD_TIME_DEFAULT          0U
/** ISO/IEC 7816-3 default T=0 WI. */
#define USBD_CCID_T0_WAITING_INTEGER_DEFAULT  10U
/** ISO/IEC 7816-3 default T=1 BWI=4, CWI=13. */
#define USBD_CCID_T1_WAITING_INTEGERS_DEFAULT 0x4D
/** ISO/IEC 7816-3 default card information field size. */
#define USBD_CCID_T1_IFSC_DEFAULT             32U
/** Largest T=1 information field size (ISO/IEC 7816-3). */
#define USBD_CCID_T1_IFS_MAX                  254U
/** Default T=1 node address. */
#define USBD_CCID_T1_NAD_DEFAULT              0U

/** Current card clock and data rate, including backend-negotiated settings. */
struct usbd_ccid_data_rate_and_clock {
	uint32_t clock_khz;
	uint32_t data_rate_bps;
};

/** Reader capabilities, fixed before USB is enabled. */
struct usbd_ccid_reader_config {
	/** CCID dwProtocols bitmask; use USBD_CCID_PROTOCOL_MASK(). */
	uint32_t protocols;
	/** Must be included in protocols. */
	uint8_t default_protocol;
	/** CCID dwFeatures; select exactly one APDU exchange level. */
	uint32_t features;
	/** Voltage capability bits, using USBD_CCID_VOLTAGE_MASK(). */
	uint8_t voltage_support;
	/** Maximum T=1 IFSD, 1..254; zero for a reader without T=1. */
	uint32_t max_ifsd;
	/** Nonempty table of nonzero, strictly ascending clocks in kHz; first is the default. */
	const uint32_t *clock_frequencies;
	uint8_t clock_count;
	/** Nonempty table of nonzero, strictly ascending rates in bit/s; first is the default. */
	const uint32_t *data_rates;
	uint8_t data_rate_count;
	/**
	 * Automatic time-extension interval in milliseconds; zero disables it.
	 * Select an interval shorter than the host's initial WWT/BWT and than
	 * the extended waiting time. Backend callbacks must otherwise finish
	 * within the host's waiting time. This does not extend card-side timers.
	 */
	uint32_t time_extension_interval_ms;
	/** CCID WWT/BWT multiplier, 1..255 when automatic extensions are enabled. */
	uint8_t time_extension_multiplier;
};

/**
 * Protocol number and opaque CCID protocol parameter data.
 *
 * The backend defines and validates the data layout and length. For CCID 1.1,
 * T=0 uses five bytes and T=1 uses seven bytes. Input data is class-owned and
 * valid during the callback. Output data is backend-owned and must remain valid
 * until the next backend callback for this instance; the class copies it into
 * the USB response immediately. Backend callbacks must not return stack storage.
 */
struct usbd_ccid_parameters {
	uint8_t protocol;
	const void *data;
	size_t length;
};

/** Optional T=0 layout helper from CCID 1.1, section 6.2.3. */
struct usbd_ccid_t0_parameters {
	/** FI and DI indices (high and low nibble). */
	uint8_t findex_dindex;
	/** Convention from enum usbd_ccid_convention; all other bits are zero. */
	uint8_t tccks;
	/** Extra guard time in ETU; 0xFF has the same meaning as zero. */
	uint8_t guard_time;
	/** WI used to determine the T=0 work waiting time (WWT). */
	uint8_t waiting_integer;
	/** Clock stop support from enum usbd_ccid_clock_stop. */
	uint8_t clock_stop;
};

/** Optional T=1 layout helper from CCID 1.1, section 6.2.3. */
struct usbd_ccid_t1_parameters {
	/** FI and DI indices (high and low nibble). */
	uint8_t findex_dindex;
	/** USBD_CCID_T1_TCCKS_PROTOCOL combined with checksum and convention. */
	uint8_t tccks;
	/** Extra guard time in ETU; 0xFF reduces the normal guard time by one. */
	uint8_t guard_time;
	/** BWI and CWI (high and low nibble). */
	uint8_t waiting_integers;
	/** Clock stop support from enum usbd_ccid_clock_stop. */
	uint8_t clock_stop;
	/** Card receive information-field size, in bytes. */
	uint8_t ifsc;
	uint8_t nad;
};

/**
 * Initializer for T=0 with initial ISO/IEC 7816-3 parameters, direct
 * convention and no clock-stop support. This is a starting point for a backend,
 * not a universal card profile: adapt it to the ATR and negotiated settings.
 */
#define USBD_CCID_T0_PARAMETERS_DEFAULT                                                            \
	{                                                                                          \
		.findex_dindex = USBD_CCID_FINDEX_DINDEX_DEFAULT,                                  \
		.tccks = USBD_CCID_CONVENTION_DIRECT,                                              \
		.guard_time = USBD_CCID_GUARD_TIME_DEFAULT,                                        \
		.waiting_integer = USBD_CCID_T0_WAITING_INTEGER_DEFAULT,                           \
		.clock_stop = USBD_CCID_CLOCK_STOP_NOT_ALLOWED,                                    \
	}

/**
 * Initializer for T=1 with initial ISO/IEC 7816-3 parameters, direct
 * convention, LRC and no clock-stop support. Adapt it to the ATR and negotiated
 * settings; selecting T=1 alone does not establish a complete card profile.
 */
#define USBD_CCID_T1_PARAMETERS_DEFAULT                                                            \
	{                                                                                          \
		.findex_dindex = USBD_CCID_FINDEX_DINDEX_DEFAULT,                                  \
		.tccks = USBD_CCID_T1_TCCKS_PROTOCOL | USBD_CCID_CONVENTION_DIRECT |               \
			 USBD_CCID_T1_CHECKSUM_LRC,                                                \
		.guard_time = USBD_CCID_GUARD_TIME_DEFAULT,                                        \
		.waiting_integers = USBD_CCID_T1_WAITING_INTEGERS_DEFAULT,                         \
		.clock_stop = USBD_CCID_CLOCK_STOP_NOT_ALLOWED,                                    \
		.ifsc = USBD_CCID_T1_IFSC_DEFAULT,                                                 \
		.nad = USBD_CCID_T1_NAD_DEFAULT,                                                   \
	}

/** Offending field offsets returned by the @c set_parameters callback. */
enum usbd_ccid_parameter_error {
	USBD_CCID_PARAMETER_ERROR_LENGTH = 1,
	USBD_CCID_PARAMETER_ERROR_PROTOCOL = 7,
	USBD_CCID_PARAMETER_ERROR_FINDEX_DINDEX = 10,
	USBD_CCID_PARAMETER_ERROR_TCCKS = 11,
	USBD_CCID_PARAMETER_ERROR_GUARD_TIME = 12,
	USBD_CCID_PARAMETER_ERROR_WAITING_INTEGERS = 13,
	USBD_CCID_PARAMETER_ERROR_CLOCK_STOP = 14,
	USBD_CCID_PARAMETER_ERROR_IFSC = 15,
	USBD_CCID_PARAMETER_ERROR_NAD = 16,
};

struct usbd_ccid_card_ops;

/**
 * Backend instance bound to one CCID interface.
 *
 * Embed this object in an application-specific structure and use CONTAINER_OF()
 * in callbacks to access its private state. The object, operation table and
 * capability tables must remain valid for the class lifetime. Do not change
 * ops or config after registration; the class references them without copying.
 * One backend object can be registered with only one interface.
 */
struct usbd_ccid_backend {
	const struct usbd_ccid_card_ops *ops;
	struct usbd_ccid_reader_config config;
};

/** Card backend operations for a single-slot CCID reader. */
struct usbd_ccid_card_ops {
	/**
	 * @brief Return the current card state without blocking.
	 *
	 * May run concurrently with worker callbacks and cancellation. Publish
	 * state changes atomically; do not take a lock held by a blocking callback.
	 *
	 * @param backend Object supplied during backend registration.
	 * @return Current card state.
	 */
	enum usbd_ccid_card_state (*get_state)(struct usbd_ccid_backend *backend);

	/**
	 * @brief Power on or reset the card and return its answer to reset (ATR).
	 *
	 * This callback runs in the CCID worker thread and may block.
	 *
	 * @param backend Object supplied during backend registration.
	 * @param[out] atr Class-owned buffer for the ATR, valid during this call.
	 * @param voltage Host-selected power voltage (including automatic selection).
	 * @param[in,out] atr_length Buffer capacity in bytes on entry; number of ATR
	 *                          bytes written on success, at most the capacity.
	 * @retval 0 on success.
	 * @return Negative errno on failure.
	 */
	int (*power_on)(struct usbd_ccid_backend *backend, enum usbd_ccid_voltage voltage,
			uint8_t *atr, size_t *atr_length);

	/**
	 * @brief Power off the card.
	 *
	 * This callback runs in the CCID worker thread and may block.
	 *
	 * @param backend Object supplied during backend registration.
	 * @retval 0 on success.
	 * @return Negative errno on failure.
	 */
	int (*power_off)(struct usbd_ccid_backend *backend);

	/**
	 * @brief Exchange one APDU with the card.
	 *
	 * On entry, @p response_length contains the capacity of @p response. On
	 * success it shall contain the number of response bytes produced.
	 * This callback runs in the CCID worker thread and may block.
	 *
	 * @param backend Object supplied during backend registration.
	 * @param[in] command Class-owned command buffer, valid during this call.
	 * @param command_length Command length in bytes.
	 * @param[out] response Class-owned response buffer, valid during this call.
	 * @param[in,out] response_length Response capacity in bytes on entry; number
	 *                               of bytes written on success, at most the capacity.
	 * @retval 0 on success.
	 * @return Negative errno on failure.
	 */
	int (*transceive)(struct usbd_ccid_backend *backend, const uint8_t *command,
			  size_t command_length, uint8_t *response, size_t *response_length);

	/**
	 * @brief Return the currently selected protocol and its parameters.
	 *
	 * The backend owns the parameters and keeps them consistent with its ATR.
	 * All parameter callbacks run in the instance's CCID worker and may block.
	 * Parameter data lifetime is described by struct usbd_ccid_parameters.
	 *
	 * @param backend Object supplied during backend registration.
	 * @param[out] parameters Current parameters; fill every field on success.
	 * @retval 0 on success.
	 * @return Negative errno on failure; -ETIMEDOUT if no card is present.
	 */
	int (*get_parameters)(struct usbd_ccid_backend *backend,
			      struct usbd_ccid_parameters *parameters);

	/**
	 * @brief Validate and apply a requested protocol and parameters atomically.
	 *
	 * On success return the effective parameters, including the convention
	 * determined by the ATR. For T=0/T=1 ignore bit 1 of bmTCCKS (CCID 1.1,
	 * section 6.1.7). On failure leave all current parameters unchanged.
	 * A fixed-profile backend accepts matching values and rejects changes.
	 *
	 * @param backend Object supplied during backend registration.
	 * @param[in] requested Parameters requested by the host.
	 * @param[out] parameters Effective parameters; fill every field on success.
	 * @retval 0 on success.
	 * @return Positive CCID bError (1..255), such as a field offset from
	 *         @ref usbd_ccid_parameter_error, or negative errno on an operation
	 *         failure. On a positive error the class queries get_parameters to
	 *         report the unchanged current settings.
	 */
	int (*set_parameters)(struct usbd_ccid_backend *backend,
			      const struct usbd_ccid_parameters *requested,
			      struct usbd_ccid_parameters *parameters);

	/**
	 * @brief Restore the card's default protocol and parameters.
	 *
	 * @param backend Object supplied during backend registration.
	 * @param[out] parameters Restored parameters; fill every field on success.
	 * @retval 0 on success.
	 * @return Negative errno on failure; -ETIMEDOUT if no card is present.
	 */
	int (*reset_parameters)(struct usbd_ccid_backend *backend,
				struct usbd_ccid_parameters *parameters);

	/**
	 * @brief Report the current clock and data rate selected by the backend.
	 *
	 * Called in the worker after power-on and for clock/data-rate commands.
	 * Both values must be present in the configured capability tables.
	 * @param backend Registered backend object.
	 * @param[out] settings Current clock and data rate.
	 * @return Zero or negative errno; -ETIMEDOUT for an absent card.
	 */
	int (*get_data_rate_and_clock)(struct usbd_ccid_backend *backend,
				       struct usbd_ccid_data_rate_and_clock *settings);

	/**
	 * @brief Apply a supported clock/data-rate pair atomically.
	 *
	 * Optional when the reader only has fixed/automatically selected settings.
	 * Runs in the worker. Leave settings unchanged on failure. On success
	 * return the actually applied values in settings.
	 * @param backend Registered backend object.
	 * @param[in,out] settings Requested pair, replaced by effective values.
	 * @return Zero, negative errno or a positive CCID bError.
	 */
	int (*set_data_rate_and_clock)(struct usbd_ccid_backend *backend,
				       struct usbd_ccid_data_rate_and_clock *settings);

	/**
	 * @brief Start/stop the physical card clock.
	 *
	 * Required when USBD_CCID_CLOCK_STOP is advertised. Runs in the worker.
	 * @param backend Registered backend object.
	 * @param stop True to stop, false to restart.
	 * @param[out] status CCID bClockStatus from enum usbd_ccid_clock_status.
	 * @return Zero, negative errno or a positive CCID bError.
	 */
	int (*set_clock)(struct usbd_ccid_backend *backend, bool stop, uint8_t *status);

	/**
	 * @brief Request cancellation of a running operation.
	 *
	 * This callback is invoked from USB request context and must not block.
	 * It may run concurrently with a worker callback. It is also called on USB
	 * disable/suspend to request cancellation. Set to NULL if the backend does
	 * not support cancellation; ABORT then waits for the callback to return.
	 *
	 * @param backend Object supplied during backend registration.
	 * @retval 0 if cancellation was requested.
	 * @return Negative errno on failure.
	 */
	int (*abort)(struct usbd_ccid_backend *backend);
};

/**
 * @brief Register operations for an independently addressable CCID interface.
 *
 * Registration must be completed before usbd_init(), with no concurrent
 * backend registration. All callbacks except
 * @c abort, @c set_clock and @c set_data_rate_and_clock are mandatory.
 * The application owns the backend object and its operation table.
 *
 * @param instance Explicit devicetree @c instance-id of an enabled CCID node.
 *                 This is independent of devicetree enumeration order and the
 *                 USB interface number.
 * @param backend Non-NULL backend object. Its configuration must match the
 *                card's ATR and negotiated settings. The object, operation
 *                table and referenced capability tables must remain valid for
 *                the class lifetime; ops/config must not change after binding.
 *
 * @retval 0 on success.
 * @retval -EINVAL for an invalid backend, invalid capabilities or missing mandatory callbacks.
 * @retval -EBUSY if USB is already enabled for the instance.
 * @retval -EALREADY if the instance or backend object is already bound.
 * @retval -ENODEV if no enabled devicetree node has this instance ID.
 */
int usbd_ccid_register_backend(unsigned int instance, struct usbd_ccid_backend *backend);

/**
 * @brief Queue or coalesce a presence notification for one CCID interface.
 *
 * The notification is sent only while the class is enabled. Calls made while
 * the interrupt endpoint is busy are coalesced. The backend must update its
 * card state before calling this function. May be called from an ISR.
 * Allocation and transfer errors are retried by the class work item.
 *
 * @param instance Explicit devicetree @c instance-id of an enabled CCID node.
 *
 * @retval 0 if the notification was queued or coalesced.
 * @retval -ENODEV if the ID has no enabled devicetree node or no registered backend.
 * @retval -EACCES if the class is not enabled.
 * @return Negative work-queue error if notification work could not be queued.
 */
int usbd_ccid_card_changed(unsigned int instance);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_CCID_H_ */
