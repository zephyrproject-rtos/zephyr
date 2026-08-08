/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BMC_H_
#define ZEPHYR_INCLUDE_MGMT_BMC_H_

/**
 * @file
 * @brief Baseboard Management Controller (BMC) core API.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Baseboard Management Controller
 * @defgroup bmc_api BMC API
 * @ingroup os_services
 * @{
 */

/**
 * @brief BMC bring-up phases.
 *
 * Components registered with BMC_COMPONENT_DEFINE() are initialised phase by
 * phase, in ascending phase order. The order of components within one phase is
 * unspecified, so a component that depends on another one must be placed in a
 * later phase.
 */
enum bmc_init_phase {
	/** Persistent configuration storage. */
	BMC_INIT_PHASE_STORAGE,
	/** Board level facilities: identity, clocks, host control, sensors. */
	BMC_INIT_PHASE_PLATFORM,
	/** Network bring-up. Runs after the configuration is readable. */
	BMC_INIT_PHASE_NETWORK,
	/** Network services such as HTTP, Redfish and the console bridges. */
	BMC_INIT_PHASE_SERVICE,
	/** Application components. Nothing in the BMC core uses this phase. */
	BMC_INIT_PHASE_APP,
	/** Number of phases. */
	BMC_INIT_PHASE_COUNT,
};

/**
 * @brief A unit of BMC functionality that needs initialisation.
 *
 * Do not populate this structure directly, use BMC_COMPONENT_DEFINE().
 */
struct bmc_component {
	/** Human readable name, used for logging. */
	const char *name;
	/** Initialisation function. Returns 0 on success, negative errno otherwise. */
	int (*init)(void);
	/** Phase this component belongs to. */
	uint8_t phase;
	/** If true, an initialisation failure is logged but does not abort boot. */
	bool optional;
};

/**
 * @brief Register a component with the BMC bring-up sequence.
 *
 * Applications use this to hook their own initialisation into bmc_init()
 * without modifying the BMC core. Use BMC_INIT_PHASE_APP unless the component
 * has to run earlier.
 *
 * @param _sym Unique C symbol name for the component.
 * @param _phase One of @ref bmc_init_phase.
 * @param _init_fn Initialisation function of type `int (*)(void)`.
 * @param _optional Whether a failure of @p _init_fn should abort BMC bring-up.
 */
#define BMC_COMPONENT_DEFINE(_sym, _phase, _init_fn, _optional)                                    \
	static const STRUCT_SECTION_ITERABLE(bmc_component, _sym) = {                              \
		.name = STRINGIFY(_sym),                                                           \
		.init = (_init_fn),                                                                \
		.phase = (_phase),                                                                 \
		.optional = (_optional),                                                           \
	}

/** @brief Events reported to registered BMC callbacks. */
enum bmc_event {
	/** BMC bring-up has completed. No event data. */
	BMC_EVENT_BOOT_DONE = BIT(0),
	/** Networking is up and configured. No event data. */
	BMC_EVENT_NET_READY = BIT(1),
	/** A configuration item changed. Data is a `const char *` setting name. */
	BMC_EVENT_CONFIG_CHANGED = BIT(2),
	/** Host power state changed. Data is a `bool *` holding the new state. */
	BMC_EVENT_HOST_POWER_CHANGED = BIT(3),
};

/**
 * @brief BMC event callback registration.
 *
 * The caller owns the storage and must keep it valid for as long as the
 * callback is registered.
 */
struct bmc_callback {
	/** Internal list node. */
	sys_snode_t node;
	/** Bitmask of @ref bmc_event values this callback is interested in. */
	uint32_t event_mask;
	/**
	 * @brief Event handler.
	 *
	 * @param event The @ref bmc_event that occurred.
	 * @param data Event specific data, may be NULL.
	 * @param len Length of @p data in bytes.
	 *
	 * @return 0 on success, negative errno to log an error. The return
	 *         value never stops the event from reaching other callbacks.
	 */
	int (*handler)(uint32_t event, void *data, size_t len);
};

/**
 * @brief Register a BMC event callback.
 *
 * @param cb Callback to register. Must not be registered already.
 */
void bmc_callback_register(struct bmc_callback *cb);

/**
 * @brief Unregister a BMC event callback.
 *
 * @param cb Previously registered callback.
 */
void bmc_callback_unregister(struct bmc_callback *cb);

/**
 * @brief Notify all interested callbacks about an event.
 *
 * Mostly used internally by the BMC core, but applications may raise their own
 * notifications too.
 *
 * @param event The @ref bmc_event that occurred.
 * @param data Event specific data, may be NULL.
 * @param len Length of @p data in bytes.
 */
void bmc_event_notify(uint32_t event, void *data, size_t len);

/**
 * @brief Initialise the BMC.
 *
 * Runs every registered @ref bmc_component in phase order. On success the BMC
 * services are running and bmc_is_boot_finished() returns true.
 *
 * @return 0 on success, negative errno otherwise.
 */
int bmc_init(void);

/**
 * @brief Check whether BMC bring-up has completed.
 *
 * @return true once bmc_init() has run to completion.
 */
bool bmc_is_boot_finished(void);

/** @brief Reboot the BMC itself. Does not return. */
FUNC_NORETURN void bmc_reboot(void);

/** @brief Power off the BMC itself. Does not return. */
FUNC_NORETURN void bmc_poweroff(void);

/**
 * @brief Get the UUID identifying this BMC.
 *
 * Derived from the hardware device identifier when CONFIG_BMC_UUID is enabled,
 * otherwise a fixed placeholder.
 *
 * @return The UUID in string form. Never NULL.
 */
const char *bmc_uuid_get(void);

/**
 * @brief Get the BMC firmware version string.
 *
 * @return The version reported over Redfish and by the `bmc vpd show` shell
 *         command. Never NULL.
 */
const char *bmc_firmware_version(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_BMC_H_ */
