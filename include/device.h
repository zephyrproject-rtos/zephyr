/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICE_H_
#define ZEPHYR_INCLUDE_DEVICE_H_

/**
 * @brief Device Driver APIs
 * @defgroup io_interfaces Device Driver APIs
 * @{
 * @}
 */
/**
 * @brief Miscellaneous Drivers APIs
 * @defgroup misc_interfaces Miscellaneous Drivers APIs
 * @ingroup io_interfaces
 * @{
 * @}
 */
/**
 * @brief Device Model APIs
 * @defgroup device_model Device Model APIs
 * @{
 */

#include <init.h>
#include <pm/device.h>
#include <sys/device_mmio.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Type used to represent devices and functions.
 *
 * The extreme values and zero have special significance.  Negative
 * values identify functionality that does not correspond to a Zephyr
 * device, such as the system clock or a SYS_INIT() function.
 */
typedef int16_t device_handle_t;

/** @brief Flag value used in lists of device handles to separate
 * distinct groups.
 *
 * This is the minimum value for the device_handle_t type.
 */
#define DEVICE_HANDLE_SEP INT16_MIN

/** @brief Flag value used in lists of device handles to indicate the
 * end of the list.
 *
 * This is the maximum value for the device_handle_t type.
 */
#define DEVICE_HANDLE_ENDS INT16_MAX

/** @brief Flag value used to identify an unknown device. */
#define DEVICE_HANDLE_NULL 0

#define Z_DEVICE_MAX_NAME_LEN	48

/**
 * @def DEVICE_NAME_GET
 *
 * @brief Expands to the full name of a global device object
 *
 * @details Return the full name of a device object symbol created by
 * DEVICE_DEFINE(), using the dev_name provided to DEVICE_DEFINE().
 *
 * It is meant to be used for declaring extern symbols pointing on device
 * objects before using the DEVICE_GET macro to get the device object.
 *
 * @param name The same as dev_name provided to DEVICE_DEFINE()
 *
 * @return The expanded name of the device object created by DEVICE_DEFINE()
 */
#define DEVICE_NAME_GET(name) _CONCAT(__device_, name)

/**
 * @def SYS_DEVICE_DEFINE
 *
 * @brief Run an initialization function at boot at specified priority,
 * and define device PM control function.
 *
 * @details Invokes DEVICE_DEFINE() with no power management support
 * (@p pm_control_fn), no API (@p api_ptr), and a device name derived from
 * the @p init_fn name (@p dev_name).
 */
#define SYS_DEVICE_DEFINE(drv_name, init_fn, pm_control_fn, level, prio) \
	DEVICE_DEFINE(Z_SYS_NAME(init_fn), drv_name, init_fn,		\
		      pm_control_fn,					\
		      NULL, NULL, level, prio, NULL)

/**
 * @def DEVICE_DEFINE
 *
 * @brief Create device object and set it up for boot time initialization,
 * with the option to pm_control. In case of Device Idle Power
 * Management is enabled, make sure the device is in suspended state after
 * initialization.
 *
 * @details This macro defines a device object that is automatically
 * configured by the kernel during system initialization. Note that
 * devices set up with this macro will not be accessible from user mode
 * since the API is not specified;
 *
 * @param dev_name Device name. This must be less than Z_DEVICE_MAX_NAME_LEN
 * characters (including terminating NUL) in order to be looked up from user
 * mode with device_get_binding().
 *
 * @param drv_name The name this instance of the driver exposes to
 * the system.
 *
 * @param init_fn Address to the init function of the driver.
 *
 * @param pm_control_fn Pointer to pm_control function.
 * Can be NULL if not implemented.
 *
 * @param data_ptr Pointer to the device's private data.
 *
 * @param cfg_ptr The address to the structure containing the
 * configuration information for this instance of the driver.
 *
 * @param level The initialization level.  See SYS_INIT() for
 * details.
 *
 * @param prio Priority within the selected initialization level. See
 * SYS_INIT() for details.
 *
 * @param api_ptr Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 */
#define DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_control_fn,	\
		      data_ptr, cfg_ptr, level, prio, api_ptr)		\
	Z_DEVICE_DEFINE(DT_INVALID_NODE, dev_name, drv_name, init_fn,	\
			pm_control_fn,					\
			data_ptr, cfg_ptr, level, prio, api_ptr)

/**
 * @def DEVICE_DT_NAME
 *
 * @brief Return a string name for a devicetree node.
 *
 * @details This macro returns a string literal usable as a device name
 * from a devicetree node. If the node has a "label" property, its value is
 * returned. Otherwise, the node's full "node-name@@unit-address" name is
 * returned.
 *
 * @param node_id The devicetree node identifier.
 */
#define DEVICE_DT_NAME(node_id) \
	DT_PROP_OR(node_id, label, DT_NODE_FULL_NAME(node_id))

/**
 * @def DEVICE_DT_DEFINE
 *
 * @brief Like DEVICE_DEFINE but taking metadata from a devicetree node.
 *
 * @details This macro defines a device object that is automatically
 * configured by the kernel during system initialization.  The device
 * object name is derived from the node identifier (encoding the
 * devicetree path to the node), and the driver name is from the @p
 * label property of the devicetree node.
 *
 * The device is declared with extern visibility, so device objects
 * defined through this API can be obtained directly through
 * DEVICE_DT_GET() using @p node_id.  Before using the pointer the
 * referenced object should be checked using device_is_ready().
 *
 * @param node_id The devicetree node identifier.
 *
 * @param init_fn Address to the init function of the driver.
 *
 * @param pm_control_fn Pointer to pm_control function.
 * Can be NULL if not implemented.
 *
 * @param data_ptr Pointer to the device's private data.
 *
 * @param cfg_ptr The address to the structure containing the
 * configuration information for this instance of the driver.
 *
 * @param level The initialization level.  See SYS_INIT() for
 * details.
 *
 * @param prio Priority within the selected initialization level. See
 * SYS_INIT() for details.
 *
 * @param api_ptr Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 */
#define DEVICE_DT_DEFINE(node_id, init_fn, pm_control_fn,		\
			 data_ptr, cfg_ptr, level, prio,		\
			 api_ptr, ...)					\
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_NAME(node_id),		\
			DEVICE_DT_NAME(node_id), init_fn,		\
			pm_control_fn,					\
			data_ptr, cfg_ptr, level, prio,			\
			api_ptr, __VA_ARGS__)

/**
 * @def DEVICE_DT_INST_DEFINE
 *
 * @brief Like DEVICE_DT_DEFINE for an instance of a DT_DRV_COMPAT compatible
 *
 * @param inst instance number.  This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to DEVICE_DT_DEFINE.
 *
 * @param ... other parameters as expected by DEVICE_DT_DEFINE.
 */
#define DEVICE_DT_INST_DEFINE(inst, ...) \
	DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @def DEVICE_DT_NAME_GET
 *
 * @brief The name of the struct device object for @p node_id
 *
 * @details Return the full name of a device object symbol created by
 * DEVICE_DT_DEFINE(), using the dev_name derived from @p node_id
 *
 * It is meant to be used for declaring extern symbols pointing on device
 * objects before using the DEVICE_DT_GET macro to get the device object.
 *
 * @param node_id The same as node_id provided to DEVICE_DT_DEFINE()
 *
 * @return The expanded name of the device object created by
 * DEVICE_DT_DEFINE()
 */
#define DEVICE_DT_NAME_GET(node_id) DEVICE_NAME_GET(Z_DEVICE_DT_DEV_NAME(node_id))

/**
 * @def DEVICE_DT_GET
 *
 * @brief Obtain a pointer to a device object by @p node_id
 *
 * @details Return the address of a device object created by
 * DEVICE_DT_INIT(), using the dev_name derived from @p node_id
 *
 * @param node_id The same as node_id provided to DEVICE_DT_DEFINE()
 *
 * @return A pointer to the device object created by DEVICE_DT_DEFINE()
 */
#define DEVICE_DT_GET(node_id) (&DEVICE_DT_NAME_GET(node_id))

/** @def DEVICE_DT_INST_GET
 *
 * @brief Obtain a pointer to a device object for an instance of a
 *        DT_DRV_COMPAT compatible
 *
 * @param inst instance number
 */
#define DEVICE_DT_INST_GET(inst) DEVICE_DT_GET(DT_DRV_INST(inst))

/**
 * @def DEVICE_DT_GET_ANY
 *
 * @brief Obtain a pointer to a device object by devicetree compatible
 *
 * If any enabled devicetree node has the given compatible and a
 * device object was created from it, this returns that device.
 *
 * If there no such devices, this returns NULL.
 *
 * If there are multiple, this returns an arbitrary one.
 *
 * If this returns non-NULL, the device must be checked for readiness
 * before use, e.g. with device_is_ready().
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @return a pointer to a device, or NULL
 */
#define DEVICE_DT_GET_ANY(compat)					    \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),			    \
		    (DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(compat))), \
		    (NULL))

/**
 * @def DEVICE_DT_GET_ONE
 *
 * @brief Obtain a pointer to a device object by devicetree compatible
 *
 * If any enabled devicetree node has the given compatible and a
 * device object was created from it, this returns that device.
 *
 * If there no such devices, this throws a compilation error.
 *
 * If there are multiple, this returns an arbitrary one.
 *
 * If this returns non-NULL, the device must be checked for readiness
 * before use, e.g. with device_is_ready().
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @return a pointer to a device
 */
#define DEVICE_DT_GET_ONE(compat)					    \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),			    \
		    (DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(compat))), \
		    (ZERO_OR_COMPILE_ERROR(0)))

/**
 * @def DEVICE_GET
 *
 * @brief Obtain a pointer to a device object by name
 *
 * @details Return the address of a device object created by
 * DEVICE_DEFINE(), using the dev_name provided to DEVICE_DEFINE().
 *
 * @param name The same as dev_name provided to DEVICE_DEFINE()
 *
 * @return A pointer to the device object created by DEVICE_DEFINE()
 */
#define DEVICE_GET(name) (&DEVICE_NAME_GET(name))

/** @def DEVICE_DECLARE
 *
 * @brief Declare a static device object
 *
 * This macro can be used at the top-level to declare a device, such
 * that DEVICE_GET() may be used before the full declaration in
 * DEVICE_DEFINE().
 *
 * This is often useful when configuring interrupts statically in a
 * device's init or per-instance config function, as the init function
 * itself is required by DEVICE_DEFINE() and use of DEVICE_GET()
 * inside it creates a circular dependency.
 *
 * @param name Device name
 */
#define DEVICE_DECLARE(name) static const struct device DEVICE_NAME_GET(name)

/**
 * @brief Runtime device dynamic structure (in RAM) per driver instance
 *
 * Fields in this are expected to be default-initialized to zero.  The
 * kernel driver infrastructure and driver access functions are
 * responsible for ensuring that any non-zero initialization is done
 * before they are accessed.
 */
struct device_state {
	/** Non-negative result of initializing the device.
	 *
	 * The absolute value returned when the device initialization
	 * function was invoked, or `UINT8_MAX` if the value exceeds
	 * an 8-bit integer.  If initialized is also set, a zero value
	 * indicates initialization succeeded.
	 */
	unsigned int init_res : 8;

	/** Indicates the device initialization function has been
	 * invoked.
	 */
	bool initialized : 1;

#ifdef CONFIG_PM_DEVICE
	/* Power management data */
	struct pm_device pm;
#endif /* CONFIG_PM_DEVICE */
};

/**
 * @brief Runtime device structure (in ROM) per driver instance
 */
struct device {
	/** Name of the device instance */
	const char *name;
	/** Address of device instance config information */
	const void *config;
	/** Address of the API structure exposed by the device instance */
	const void *api;
	/** Address of the common device state */
	struct device_state * const state;
	/** Address of the device instance private data */
	void * const data;
	/** optional pointer to handles associated with the device.
	 *
	 * This encodes a sequence of sets of device handles that have
	 * some relationship to this node.  The individual sets are
	 * extracted with dedicated API, such as
	 * device_required_handles_get().
	 */
	const device_handle_t *const handles;
#ifdef CONFIG_PM_DEVICE
	/** Power Management function */
	int (*pm_control)(const struct device *dev, uint32_t command,
			  enum pm_device_state *state);
	/** Pointer to device instance power management data */
	struct pm_device * const pm;
#endif
};

/**
 * @brief Get the handle for a given device
 *
 * @param dev the device for which a handle is desired.
 *
 * @return the handle for the device, or DEVICE_HANDLE_NULL if the
 * device does not have an associated handle.
 */
static inline device_handle_t
device_handle_get(const struct device *dev)
{
	device_handle_t ret = DEVICE_HANDLE_NULL;
	extern const struct device __device_start[];

	/* TODO: If/when devices can be constructed that are not part of the
	 * fixed sequence we'll need another solution.
	 */
	if (dev != NULL) {
		ret = 1 + (device_handle_t)(dev - __device_start);
	}

	return ret;
}

/**
 * @brief Get the device corresponding to a handle.
 *
 * @param dev_handle the device handle
 *
 * @return the device that has that handle, or a null pointer if @p
 * dev_handle does not identify a device.
 */
static inline const struct device *
device_from_handle(device_handle_t dev_handle)
{
	extern const struct device __device_start[];
	extern const struct device __device_end[];
	const struct device *dev = NULL;
	size_t numdev = __device_end - __device_start;

	if ((dev_handle > 0) && ((size_t)dev_handle < numdev)) {
		dev = &__device_start[dev_handle - 1];
	}

	return dev;
}

/**
 * @brief Prototype for functions used when iterating over a set of devices.
 *
 * Such a function may be used in API that identifies a set of devices and
 * provides a visitor API supporting caller-specific interaction with each
 * device in the set.
 *
 * The visit is said to succeed if the visitor returns a non-negative value.
 *
 * @param dev a device in the set being iterated
 *
 * @param context state used to support the visitor function
 *
 * @return A non-negative number to allow walking to continue, and a negative
 * error code to case the iteration to stop.
 */
typedef int (*device_visitor_callback_t)(const struct device *dev, void *context);

/**
 * @brief Get the set of handles for devicetree dependencies of this device.
 *
 * These are the device dependencies inferred from devicetree.
 *
 * @param dev the device for which dependencies are desired.
 *
 * @param count pointer to a place to store the number of devices provided at
 * the returned pointer.  The value is not set if the call returns a null
 * pointer.  The value may be set to zero.
 *
 * @return a pointer to a sequence of @p *count device handles, or a null
 * pointer if @p dh does not provide dependency information.
 */
static inline const device_handle_t *
device_required_handles_get(const struct device *dev,
			    size_t *count)
{
	const device_handle_t *rv = dev->handles;

	if (rv != NULL) {
		size_t i = 0;

		while ((rv[i] != DEVICE_HANDLE_ENDS)
		       && (rv[i] != DEVICE_HANDLE_SEP)) {
			++i;
		}
		*count = i;
	}

	return rv;
}

/**
 * @brief Visit every device that @p dev directly requires.
 *
 * Zephyr maintains information about which devices are directly required by
 * another device; for example an I2C-based sensor driver will require an I2C
 * controller for communication.  Required devices can derive from
 * statically-defined devicetree relationships or dependencies registered
 * at runtime.
 *
 * This API supports operating on the set of required devices.  Example uses
 * include making sure required devices are ready before the requiring device
 * is used, and releasing them when the requiring device is no longer needed.
 *
 * There is no guarantee on the order in which required devices are visited.
 *
 * If the @p visitor function returns a negative value iteration is halted,
 * and the returned value from the visitor is returned from this function.
 *
 * @note This API is not available to unprivileged threads.
 *
 * @param dev a device of interest.  The devices that this device depends on
 * will be used as the set of devices to visit.  This parameter must not be
 * null.
 *
 * @param visitor_cb the function that should be invoked on each device in the
 * dependency set.  This parameter must not be null.
 *
 * @param context state that is passed through to the visitor function.  This
 * parameter may be null if @p visitor tolerates a null @p context.
 *
 * @return The number of devices that were visited if all visits succeed, or
 * the negative value returned from the first visit that did not succeed.
 */
int device_required_foreach(const struct device *dev,
			  device_visitor_callback_t visitor_cb,
			  void *context);

/**
 * @brief Retrieve the device structure for a driver by name
 *
 * @details Device objects are created via the DEVICE_DEFINE() macro and
 * placed in memory by the linker. If a driver needs to bind to another driver
 * it can use this function to retrieve the device structure of the lower level
 * driver by the name the driver exposes to the system.
 *
 * @param name device name to search for.  A null pointer, or a pointer to an
 * empty string, will cause NULL to be returned.
 *
 * @return pointer to device structure; NULL if not found or cannot be used.
 */
__syscall const struct device *device_get_binding(const char *name);

/** @brief Get access to the static array of static devices.
 *
 * @param devices where to store the pointer to the array of
 * statically allocated devices.  The array must not be mutated
 * through this pointer.
 *
 * @return the number of statically allocated devices.
 */
size_t z_device_get_all_static(const struct device * *devices);

/** @brief Determine whether a device has been successfully initialized.
 *
 * @param dev pointer to the device in question.
 *
 * @return true if and only if the device is available for use.
 */
bool z_device_ready(const struct device *dev);

/** @brief Determine whether a device is ready for use
 *
 * This is the implementation underlying `device_usable_check()`, without the
 * overhead of a syscall wrapper.
 *
 * @param dev pointer to the device in question.
 *
 * @return a non-positive integer as documented in device_usable_check().
 */
static inline int z_device_usable_check(const struct device *dev)
{
	return z_device_ready(dev) ? 0 : -ENODEV;
}

/** @brief Determine whether a device is ready for use.
 *
 * This checks whether a device can be used, returning 0 if it can, and
 * distinct error values that identify the reason if it cannot.
 *
 * @retval 0 if the device is usable.
 * @retval -ENODEV if the device has not been initialized, the device pointer
 * is NULL or the initialization failed.
 * @retval other negative error codes to indicate additional conditions that
 * make the device unusable.
 */
__syscall int device_usable_check(const struct device *dev);

static inline int z_impl_device_usable_check(const struct device *dev)
{
	return z_device_usable_check(dev);
}

/** @brief Verify that a device is ready for use.
 *
 * Indicates whether the provided device pointer is for a device known to be
 * in a state where it can be used with its standard API.
 *
 * This can be used with device pointers captured from DEVICE_DT_GET(), which
 * does not include the readiness checks of device_get_binding().  At minimum
 * this means that the device has been successfully initialized, but it may
 * take on further conditions (e.g. is not powered down).
 *
 * @param dev pointer to the device in question.
 *
 * @retval true if the device is ready for use.
 * @retval false if the device is not ready for use or if a NULL device pointer
 * is passed as argument.
 */
static inline bool device_is_ready(const struct device *dev)
{
	return device_usable_check(dev) == 0;
}

/**
 * @brief Indicate that the device is in the middle of a transaction
 *
 * Called by a device driver to indicate that it is in the middle of a
 * transaction.
 *
 * @param dev Pointer to device structure of the driver instance.
 */
void device_busy_set(const struct device *dev);

/**
 * @brief Indicate that the device has completed its transaction
 *
 * Called by a device driver to indicate the end of a transaction.
 *
 * @param dev Pointer to device structure of the driver instance.
 */
void device_busy_clear(const struct device *dev);

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Check if any device is in the middle of a transaction
 *
 * Called by an application to see if any device is in the middle
 * of a critical transaction that cannot be interrupted.
 *
 * @retval 0 if no device is busy
 * @retval -EBUSY if any device is busy
 */
int device_any_busy_check(void);

/**
 * @brief Check if a specific device is in the middle of a transaction
 *
 * Called by an application to see if a particular device is in the
 * middle of a critical transaction that cannot be interrupted.
 *
 * @param chk_dev Pointer to device structure of the specific device driver
 * the caller is interested in.
 * @retval 0 if the device is not busy
 * @retval -EBUSY if the device is busy
 */
int device_busy_check(const struct device *chk_dev);

#endif

/**
 * @}
 */

/* Node paths can exceed the maximum size supported by device_get_binding() in user mode,
 * so synthesize a unique dev_name from the devicetree node.
 *
 * The ordinal used in this name can be mapped to the path by
 * examining zephyr/include/generated/device_extern.h header.  If the
 * format of this conversion changes, gen_defines should be updated to
 * match it.
 */
#define Z_DEVICE_DT_DEV_NAME(node_id) _CONCAT(dts_ord_, DT_DEP_ORD(node_id))

/* Synthesize a unique name for the device state associated with
 * dev_name.
 */
#define Z_DEVICE_STATE_NAME(dev_name) _CONCAT(__devstate_, dev_name)

/** Synthesize the name of the object that holds device ordinal and
 * dependency data.  If the object doesn't come from a devicetree
 * node, use dev_name.
 */
#define Z_DEVICE_HANDLE_NAME(node_id, dev_name)				\
	_CONCAT(__devicehdl_,						\
		COND_CODE_1(DT_NODE_EXISTS(node_id),			\
			    (node_id),					\
			    (dev_name)))

#define Z_DEVICE_EXTRA_HANDLES(...)				\
	FOR_EACH_NONEMPTY_TERM(IDENTITY, (,), __VA_ARGS__)

/* If device power management is enabled, this macro defines a pointer to a
 * device in the z_pm_device_slots region. When invoked for each device, this
 * will effectively result in a device pointer array with the same size of the
 * actual devices list. This is used internally by the device PM subsystem to
 * keep track of suspended devices during system power transitions.
 */
#if CONFIG_PM_DEVICE
#define Z_DEVICE_DEFINE_PM_SLOT(dev_name)				\
	static const Z_DECL_ALIGN(struct device *)			\
	_CONCAT(__pm_device_slot_, DEVICE_NAME_GET(dev_name)) __used	\
	__attribute__((__section__(".z_pm_device_slots")));
#else
#define Z_DEVICE_DEFINE_PM_SLOT(dev_name)
#endif

/* Construct objects that are referenced from struct device.  These
 * include power management and dependency handles.
 */
#define Z_DEVICE_DEFINE_PRE(node_id, dev_name, ...)			\
	Z_DEVICE_DEFINE_HANDLES(node_id, dev_name, __VA_ARGS__)		\
	Z_DEVICE_DEFINE_PM_SLOT(dev_name)


/* Initial build provides a record that associates the device object
 * with its devicetree ordinal, and provides the dependency ordinals.
 * These are provided as weak definitions (to prevent the reference
 * from being captured when the original object file is compiled), and
 * in a distinct pass1 section (which will be replaced by
 * postprocessing).
 *
 * It is also (experimentally) necessary to provide explicit alignment
 * on each object.  Otherwise x86-64 builds will introduce padding
 * between objects in the same input section in individual object
 * files, which will be retained in subsequent links both wasting
 * space and resulting in aggregate size changes relative to pass2
 * when all objects will be in the same input section.
 *
 * The build assert will fail if device_handle_t changes size, which
 * means the alignment directives in the linker scripts and in
 * `gen_handles.py` must be updated.
 */
BUILD_ASSERT(sizeof(device_handle_t) == 2, "fix the linker scripts");
#define Z_DEVICE_DEFINE_HANDLES(node_id, dev_name, ...)			\
	extern const device_handle_t					\
		Z_DEVICE_HANDLE_NAME(node_id, dev_name)[];		\
	const device_handle_t						\
	__aligned(sizeof(device_handle_t))				\
	__attribute__((__weak__,					\
		       __section__(".__device_handles_pass1")))		\
	Z_DEVICE_HANDLE_NAME(node_id, dev_name)[] = {			\
	COND_CODE_1(DT_NODE_EXISTS(node_id), (				\
			DT_DEP_ORD(node_id),				\
			DT_REQUIRES_DEP_ORDS(node_id)			\
		), (							\
			DEVICE_HANDLE_NULL,				\
		))							\
			DEVICE_HANDLE_SEP,				\
			Z_DEVICE_EXTRA_HANDLES(__VA_ARGS__)		\
			DEVICE_HANDLE_ENDS,				\
		};

#define Z_DEVICE_DEFINE_INIT(node_id, dev_name, pm_control_fn)		\
		.handles = Z_DEVICE_HANDLE_NAME(node_id, dev_name),	\
		Z_DEVICE_DEFINE_PM_INIT(dev_name, pm_control_fn)

/* Like DEVICE_DEFINE but takes a node_id AND a dev_name, and trailing
 * dependency handles that come from outside devicetree.
 */
#define Z_DEVICE_DEFINE(node_id, dev_name, drv_name, init_fn, pm_control_fn, \
			data_ptr, cfg_ptr, level, prio, api_ptr, ...)	\
	static struct device_state Z_DEVICE_STATE_NAME(dev_name);	\
	Z_DEVICE_DEFINE_PRE(node_id, dev_name, __VA_ARGS__)		\
	COND_CODE_1(DT_NODE_EXISTS(node_id), (), (static))		\
		const Z_DECL_ALIGN(struct device)			\
		DEVICE_NAME_GET(dev_name) __used			\
	__attribute__((__section__(".z_device_" #level STRINGIFY(prio)"_"))) = { \
		.name = drv_name,					\
		.config = (cfg_ptr),					\
		.api = (api_ptr),					\
		.state = &Z_DEVICE_STATE_NAME(dev_name),		\
		.data = (data_ptr),					\
		Z_DEVICE_DEFINE_INIT(node_id, dev_name, pm_control_fn)	\
	};								\
	BUILD_ASSERT(sizeof(Z_STRINGIFY(drv_name)) <= Z_DEVICE_MAX_NAME_LEN, \
		     Z_STRINGIFY(DEVICE_NAME_GET(drv_name)) " too long"); \
	Z_INIT_ENTRY_DEFINE(DEVICE_NAME_GET(dev_name), init_fn,		\
		(&DEVICE_NAME_GET(dev_name)), level, prio)

#ifdef CONFIG_PM_DEVICE
#define Z_DEVICE_DEFINE_PM_INIT(dev_name, pm_control_fn)		\
	.pm_control = (pm_control_fn),				\
	.pm = &Z_DEVICE_STATE_NAME(dev_name).pm,
#else
#define Z_DEVICE_DEFINE_PM_INIT(dev_name, pm_control_fn)
#endif

#ifdef __cplusplus
}
#endif

/* device_extern is generated based on devicetree nodes */
#include <device_extern.h>

#include <syscalls/device.h>

#endif /* ZEPHYR_INCLUDE_DEVICE_H_ */
