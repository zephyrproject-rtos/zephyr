/*
 * Copyright (c) 2025 Sequans Communications
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup hwspinlock_interface
 * @brief Main header file for hardware spinlock driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_
#define ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_

/**
 * @brief Interfaces for hardware spinlocks.
 * @defgroup hwspinlock_interface Hardware Spinlock
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HW spinlock controller runtime context
 */
struct hwspinlock_context {
	/**
	 * @internal
	 * Per HW spinlock lock
	 * @note HW spinlock protects resources across clusters, but we need to protect the
	 * access to HW spinlock inside of the same cluster, so a single thread may claim the
	 * lock at a time.
	 */
	struct k_spinlock lock;
};

/**
 * @brief Opaque type to represent a hwspinlock runtime context.
 *
 * This type is not meant to be inspected by application code.
 */
typedef struct hwspinlock_context hwspinlock_ctx_t;

/**
 * @brief Complete hardware spinlock DT information
 */
struct hwspinlock_dt_spec {
	/** HW spinlock device */
	const struct device *dev;
	/** HW spinlock id */
	uint32_t id;
	/** Runtime context */
	hwspinlock_ctx_t ctx;
};

/**
 * @brief Initializer for a hwspinlock_ctx_t
 *
 * @note We must declare each field individually because a struct k_spinlock might have no field
 * depending on Kconfig options, and gcc requires struct without any field to be initialized
 * explicitly, instead of just being able to do `{0}`.
 */
#define HWSPINLOCK_CTX_INITIALIZER                                                                 \
	{                                                                                          \
		.lock = {},                                                                        \
	}

/**
 * @brief Structure initializer for struct hwspinlock_dt_spec from devicetree by index
 *
 * This helper macro expands to a static initializer for a struct hwspinlock_dt_spec
 * by reading the relevant device controller and id number from the devicetree.
 *
 * Example devicetree fragment:
 *
 * @code{.devicetree}
 *     n: node {
 *             hwlocks = <&hwlock 8>,
 *                       <&hwlock 9>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     const struct hwspinlock_dt_spec spec = HWSPINLOCK_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n), 0);
 * @endcode
 *
 * @param node_id Devicetree node identifier for the HWSPINLOCK device
 * @param idx Index of hwlocks element
 *
 * @return static initializer for a struct hwspinlock_dt_spec
 */
#define HWSPINLOCK_DT_SPEC_GET_BY_IDX(node_id, idx)                                                \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_HWSPINLOCK_CTRL_BY_IDX(node_id, idx)),                     \
		.id = DT_HWSPINLOCK_ID_BY_IDX(node_id, idx),                                       \
		.ctx = HWSPINLOCK_CTX_INITIALIZER,                                                 \
	}

/**
 * @brief Structure initializer for struct hwspinlock_dt_spec from devicetree by name
 *
 * This helper macro expands to a static initializer for a struct hwspinlock_dt_spec
 * by reading the relevant device controller and id number from the devicetree.
 *
 * Example devicetree fragment:
 *
 * @code{.devicetree}
 *     n: node {
 *             hwlocks = <&hwlock 8>,
 *                       <&hwlock 9>;
 *             hwlock-names = "rd", "wr";
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     const struct hwspinlock_dt_spec spec = HWSPINLOCK_DT_SPEC_GET_BY_NAME(DT_NODELABEL(n), wr);
 * @endcode
 *
 * @param node_id Devicetree node identifier for the HWSPINLOCK device
 * @param name lowercase-and-underscores name of the hwlocks element
 *
 * @return static initializer for a struct hwspinlock_dt_spec
 */
#define HWSPINLOCK_DT_SPEC_GET_BY_NAME(node_id, name)                                              \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_HWSPINLOCK_CTRL_BY_NAME(node_id, name)),                   \
		.id = DT_HWSPINLOCK_ID_BY_NAME(node_id, name),                                     \
		.ctx = HWSPINLOCK_CTX_INITIALIZER,                                                 \
	}

/**
 * @brief Structure initializer for struct hwspinlock_dt_spec from devicetree.
 *
 * This is equivalent to HWSPINLOCK_DT_SPEC_GET_BY_IDX(node_id, 0)
 *
 * @param node_id Devicetree node identifier for the HWSPINLOCK device
 * @see HWSPINLOCK_DT_SPEC_GET_BY_IDX()
 */
#define HWSPINLOCK_DT_SPEC_GET(node_id) \
	HWSPINLOCK_DT_SPEC_GET_BY_IDX(node_id, 0)

/**
 * @brief Instance version of HWSPINLOCK_DT_SPEC_GET_BY_IDX()
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx Index of the hwlocks element
 *
 * @see HWSPINLOCK_DT_SPEC_GET_BY_IDX()
 */
#define HWSPINLOCK_DT_SPEC_INST_GET_BY_IDX(inst, idx)                                              \
	HWSPINLOCK_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Instance version of HWSPINLOCK_DT_SPEC_GET_BY_NAME()
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of the hwlocks element
 *
 * @see HWSPINLOCK_DT_SPEC_GET_BY_NAME()
 */
#define HWSPINLOCK_DT_SPEC_INST_GET_BY_NAME(inst, name)                                            \
	HWSPINLOCK_DT_SPEC_GET_BY_NAME(DT_DRV_INST(inst), name)
/**
 * @brief Instance version of HWSPINLOCK_DT_SPEC_GET()
 *
 * @param inst DT_DRV_COMPAT instance number
 *
 * @see HWSPINLOCK_DT_SPEC_GET()
 */
#define HWSPINLOCK_DT_SPEC_INST_GET(inst) \
	HWSPINLOCK_DT_SPEC_GET(DT_DRV_INST(inst))

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Callback API for trying to lock HW spinlock
 *
 * This callback is optional. If not implemented, hw_spin_trylock() will return -ENOSYS.
 *
 * @see hw_spin_trylock
 */
typedef int (*hwspinlock_api_trylock)(const struct device *dev, uint32_t id);

/**
 * @brief Callback API to lock HW spinlock
 *
 * This callback must be implemented.
 *
 * @see hw_spin_lock
 */
typedef void (*hwspinlock_api_lock)(const struct device *dev, uint32_t id);

/**
 * @brief Callback API to unlock HW spinlock
 *
 * This callback must be implemented.
 *
 * @see hw_spin_unlock
 */
typedef void (*hwspinlock_api_unlock)(const struct device *dev, uint32_t id);

/**
 * @brief Callback API to get HW spinlock max ID
 *
 * This callback must be implemented.
 *
 * @see hw_spinlock_get_max_id
 */
typedef uint32_t (*hwspinlock_api_get_max_id)(const struct device *dev);

__subsystem struct hwspinlock_driver_api {
	hwspinlock_api_trylock trylock;
	hwspinlock_api_lock lock;
	hwspinlock_api_unlock unlock;
	hwspinlock_api_get_max_id get_max_id;
};

/**
 * @endcond
 */

/**
 * @brief Try to lock HW spinlock
 *
 * This function is used when trying to lock an HW spinlock. If the spinlock is already locked by
 * another cluster, exits with -EBUSY.
 *
 * @see hw_spin_lock
 *
 * @param dev HW spinlock device instance.
 * @param ctx HW spinlock runtime context.
 * @param id Spinlock identifier.
 * @param[out] key A pointer to the spinlock key.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the operation is not implemented.
 * @retval -EINVAL If HW spinlock id is invalid.
 * @retval -EBUSY If HW spinlock is already locked by someone else.
 */
static inline int hw_spin_trylock(const struct device *dev, hwspinlock_ctx_t *ctx, uint32_t id,
				  k_spinlock_key_t *key)
{
	const struct hwspinlock_driver_api *api = (const struct hwspinlock_driver_api *)dev->api;
	int ret;

	if (api->trylock == NULL) {
		return -ENOSYS;
	}

	ret = k_spin_trylock(&ctx->lock, key);
	if (ret) {
		return ret;
	}
	return api->trylock(dev, id);
}

/**
 * @brief Lock HW spinlock
 *
 * This function is used when locking an HW spinlock. If the spinlock is already locked by
 * another party, waits for it to be released.
 * This is useful when protecting a resource which is shared between multiple clusters.
 *
 * @note The goal is not to replace regular spinlocks, but to protect shared resources between
 * clusters. However, because we don't want another thread to take the same HW spinlock twice, the
 * locking mechanism acts as a regular spinlock as well.
 *
 * Because this uses a regular zephyr spinlock in conjunction with a hwspinlock, the same rules
 * applies. Separate hwspinlock may be nested. It is legal to lock another (unlocked) hwspinlock
 * while holding a lock. However, an attempt to acquire a hwspinlock that the CPU already holds
 * will deadlock.
 * @see k_spin_lock
 *
 * @param dev HW spinlock device instance.
 * @param ctx HW spinlock runtime context.
 * @param id Spinlock identifier.
 *
 * @return A key value that must be passed to hw_spin_unlock() when the
 *         lock is released.
 */
static inline k_spinlock_key_t hw_spin_lock(const struct device *dev, hwspinlock_ctx_t *ctx,
					    uint32_t id)
{
	const struct hwspinlock_driver_api *api = (const struct hwspinlock_driver_api *)dev->api;
	k_spinlock_key_t k;

	__ASSERT(api->lock != NULL, "hwspinlock lock callback must be implemented");

	k = k_spin_lock(&ctx->lock);
	api->lock(dev, id);

	return k;
}

/**
 * @brief Unlock HW spinlock
 *
 * This function to unlock an HW spinlock
 *
 * @param dev HW spinlock device instance.
 * @param ctx HW spinlock runtime context.
 * @param id Spinlock identifier.
 * @param key The value returned from hw_spin_lock() when this lock was
 *        acquired
 */
static inline void hw_spin_unlock(const struct device *dev, hwspinlock_ctx_t *ctx, uint32_t id,
				  k_spinlock_key_t key)
{
	const struct hwspinlock_driver_api *api = (const struct hwspinlock_driver_api *)dev->api;

	__ASSERT(api->unlock != NULL, "hwspinlock unlock callback must be implemented");

	api->unlock(dev, id);
	k_spin_unlock(&ctx->lock, key);
}

/**
 * @brief Get HW spinlock max ID
 *
 * This function is used to get the HW spinlock maximum ID. It should
 * be called before attempting to lock/unlock a specific HW spinlock.
 *
 * @param dev HW spinlock device instance.
 *
 * @retval HW spinlock max ID.
 */
static inline uint32_t hw_spinlock_get_max_id(const struct device *dev)
{
	const struct hwspinlock_driver_api *api = (const struct hwspinlock_driver_api *)dev->api;

	__ASSERT(api->get_max_id != NULL, "hwspinlock get_max_id callback must be implemented");

	return api->get_max_id(dev);
}

/**
 * @brief Try to lock HW spinlock from a struct hwspinlock_dt_spec
 *
 * This is the dt_spec equivalent of hw_spin_trylock()
 * @see hw_spin_trylock
 *
 * @param spec HWSPINLOCK specification from devicetree
 * @param[out] key A pointer to the spinlock key.
 *
 * @returns See return values from hw_spin_trylock()
 */
static inline int hw_spin_trylock_dt(struct hwspinlock_dt_spec *spec, k_spinlock_key_t *key)
{
	return hw_spin_trylock(spec->dev, &spec->ctx, spec->id, key);
}

/**
 * @brief Lock HW spinlock from a struct hwspinlock_dt_spec
 *
 * This is the dt_spec equivalent of hw_spin_lock()
 * @see hw_spin_lock
 *
 * @param spec HWSPINLOCK specification from devicetree
 *
 * @returns See return values from hw_spin_lock()
 */
static inline k_spinlock_key_t hw_spin_lock_dt(struct hwspinlock_dt_spec *spec)
{
	return hw_spin_lock(spec->dev, &spec->ctx, spec->id);
}

/**
 * @brief Unlock HW spinlock from a struct hwspinlock_dt_spec
 *
 * This is the dt_spec equivalent of hw_spin_unlock()
 * @see hw_spin_unlock
 *
 * @param spec HWSPINLOCK specification from devicetree
 * @param key The value returned from hw_spin_lock() when this lock was
 *        acquired
 */
static inline void hw_spin_unlock_dt(struct hwspinlock_dt_spec *spec, k_spinlock_key_t key)
{
	hw_spin_unlock(spec->dev, &spec->ctx, spec->id, key);
}

/**
 * @brief Get HW spinlock max ID from a struct hwspinlock_dt_spec
 *
 * This is the dt_spec equivalent of hw_spinlock_get_max_id()
 * @see hw_spinlock_get_max_id
 *
 * @param spec HWSPINLOCK specification from devicetree
 *
 * @returns See return values from hw_spinlock_get_max_id()
 */
static inline uint32_t hw_spinlock_get_max_id_dt(struct hwspinlock_dt_spec *spec)
{
	return hw_spinlock_get_max_id(spec->dev);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_ */
