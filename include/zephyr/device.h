/*
 * Copyright (c) 2015 Intel Corporation.
 * Copyright (c) 2022 Trackunit A/S
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

#include <device_generated.h>

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type used to represent a "handle" for a device.
 *
 * Every struct device has an associated handle. You can get a pointer
 * to a device structure from its handle and vice versa, but the
 * handle uses less space than a pointer. The device.h API mainly uses
 * handles to store lists of multiple devices in a compact way.
 *
 * The extreme values and zero have special significance. Negative
 * values identify functionality that does not correspond to a Zephyr
 * device, such as the system clock or a SYS_INIT() function.
 *
 * @see device_handle_get()
 * @see device_from_handle()
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
 * @brief Expands to the name of a global device object.
 *
 * @details Return the full name of a device object symbol created by
 * DEVICE_DEFINE(), using the dev_name provided to DEVICE_DEFINE().
 * This is the name of the global variable storing the device
 * structure, not a pointer to the string in the device's @p name
 * field.
 *
 * It is meant to be used for declaring extern symbols pointing to device
 * objects before using the DEVICE_GET macro to get the device object.
 *
 * This macro is normally only useful within device driver source
 * code. In other situations, you are probably looking for
 * device_get_binding().
 *
 * @param name The same @p dev_name token given to DEVICE_DEFINE()
 *
 * @return The full name of the device object defined by DEVICE_DEFINE()
 */
#define DEVICE_NAME_GET(name) _CONCAT(__device_, name)

/* Node paths can exceed the maximum size supported by
 * device_get_binding() in user mode; this macro synthesizes a unique
 * dev_name from a devicetree node while staying within this maximum
 * size.
 *
 * The ordinal used in this name can be mapped to the path by
 * examining zephyr/include/generated/devicetree_generated.h.
 */
#define Z_DEVICE_DT_DEV_NAME(node_id) _CONCAT(dts_ord_, DT_DEP_ORD(node_id))

/* Synthesize a unique name for the device state associated with
 * dev_name.
 */
#define Z_DEVICE_STATE_NAME(dev_name) _CONCAT(__devstate_, dev_name)

/**
 * @brief Utility macro to define and initialize the device state.
 *
 * @param node_id Devicetree node id of the device.
 * @param dev_name Device name.
 */
#define Z_DEVICE_STATE_DEFINE(node_id, dev_name)			\
	static struct device_state Z_DEVICE_STATE_NAME(dev_name)	\
	__attribute__((__section__(".z_devstate")));

/**
 * @brief Create a device object and set it up for boot time initialization.
 *
 * @details This macro defines a <tt>struct device</tt> that is
 * automatically configured by the kernel during system
 * initialization. This macro should only be used when the device is
 * not being allocated from a devicetree node. If you are allocating a
 * device from a devicetree node, use DEVICE_DT_DEFINE() or
 * DEVICE_DT_INST_DEFINE() instead.
 *
 * @param dev_name A unique token which is used in the name of the
 * global device structure as a C identifier.
 *
 * @param drv_name A string name for the device, which will be stored
 * in the device structure's @p name field. This name can be used to
 * look up the device with device_get_binding(). This must be less
 * than Z_DEVICE_MAX_NAME_LEN characters (including terminating NUL)
 * in order to be looked up from user mode.
 *
 * @param init_fn Pointer to the device's initialization function,
 * which will be run by the kernel during system initialization.
 *
 * @param pm_device Pointer to the device's power management
 * resources, a <tt>struct pm_device</tt>, which will be stored in the
 * device structure's @p pm field. Use NULL if the device does not use
 * PM.
 *
 * @param data_ptr Pointer to the device's private mutable data, which
 * will be stored in the device structure's @p data field.
 *
 * @param cfg_ptr Pointer to the device's private constant data, which
 * will be stored in the device structure's @p config field.
 *
 * @param level The device's initialization level. See SYS_INIT() for
 * details.
 *
 * @param prio The device's priority within its initialization level.
 * See SYS_INIT() for details.
 *
 * @param api_ptr Pointer to the device's API structure. Can be NULL.
 */
#define DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_device,		\
		      data_ptr, cfg_ptr, level, prio, api_ptr)		\
	Z_DEVICE_STATE_DEFINE(DT_INVALID_NODE, dev_name) \
	Z_DEVICE_DEFINE(DT_INVALID_NODE, dev_name, drv_name, init_fn,	\
			pm_device,					\
			data_ptr, cfg_ptr, level, prio, api_ptr,	\
			&Z_DEVICE_STATE_NAME(dev_name))

/**
 * @brief Return a string name for a devicetree node.
 *
 * @details This macro returns a string literal usable as a device's
 * @p name field from a devicetree node identifier.
 *
 * @param node_id The devicetree node identifier.
 *
 * @return The value of the node's "label" property, if it has one.
 * Otherwise, the node's full name in "node-name@@unit-address" form.
 */
#define DEVICE_DT_NAME(node_id) \
	DT_PROP_OR(node_id, label, DT_NODE_FULL_NAME(node_id))

/**
 * @brief Create a device object from a devicetree node identifier and
 * set it up for boot time initialization.
 *
 * @details This macro defines a <tt>struct device</tt> that is
 * automatically configured by the kernel during system
 * initialization. The global device object's name as a C identifier
 * is derived from the node's dependency ordinal. The device
 * structure's @p name field is set to
 * <tt>DEVICE_DT_NAME(node_id)</tt>.
 *
 * The device is declared with extern visibility, so a pointer to a
 * global device object can be obtained with
 * <tt>DEVICE_DT_GET(node_id)</tt> from any source file that includes
 * device.h. Before using the pointer, the referenced object should be
 * checked using device_is_ready().
 *
 * @param node_id The devicetree node identifier.
 *
 * @param init_fn Pointer to the device's initialization function,
 * which will be run by the kernel during system initialization.
 *
 * @param pm_device Pointer to the device's power management
 * resources, a <tt>struct pm_device</tt>, which will be stored in the
 * device structure's @p pm field. Use NULL if the device does not use
 * PM.
 *
 * @param data_ptr Pointer to the device's private mutable data, which
 * will be stored in the device structure's @p data field.
 *
 * @param cfg_ptr Pointer to the device's private constant data, which
 * will be stored in the device structure's @p config field.
 *
 * @param level The device's initialization level. See SYS_INIT() for
 * details.
 *
 * @param prio The device's priority within its initialization level.
 * See SYS_INIT() for details.
 *
 * @param api_ptr Pointer to the device's API structure. Can be NULL.
 */
#define DEVICE_DT_DEFINE(node_id, init_fn, pm_device,			\
			 data_ptr, cfg_ptr, level, prio,		\
			 api_ptr, ...)					\
	Z_DEVICE_STATE_DEFINE(node_id, Z_DEVICE_DT_DEV_NAME(node_id)) \
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_NAME(node_id),		\
			DEVICE_DT_NAME(node_id), init_fn,		\
			pm_device,					\
			data_ptr, cfg_ptr, level, prio,			\
			api_ptr,					\
			&Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_NAME(node_id)),	\
			__VA_ARGS__)

/**
 * @brief Like DEVICE_DT_DEFINE(), but uses an instance of a
 * DT_DRV_COMPAT compatible instead of a node identifier.
 *
 * @param inst instance number. The @p node_id argument to
 * DEVICE_DT_DEFINE is set to <tt>DT_DRV_INST(inst)</tt>.
 *
 * @param ... other parameters as expected by DEVICE_DT_DEFINE.
 */
#define DEVICE_DT_INST_DEFINE(inst, ...) \
	DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief The name of the global device object for @p node_id
 *
 * @details Returns the name of the global device structure as a C
 * identifier. The device must be allocated using DEVICE_DT_DEFINE()
 * or DEVICE_DT_INST_DEFINE() for this to work.
 *
 * This macro is normally only useful within device driver source
 * code. In other situations, you are probably looking for
 * DEVICE_DT_GET().
 *
 * @param node_id Devicetree node identifier
 *
 * @return The name of the device object as a C identifier
 */
#define DEVICE_DT_NAME_GET(node_id) DEVICE_NAME_GET(Z_DEVICE_DT_DEV_NAME(node_id))

/**
 * @brief Get a <tt>const struct device*</tt> from a devicetree node
 * identifier
 *
 * @details Returns a pointer to a device object created from a
 * devicetree node, if any device was allocated by a driver.
 *
 * If no such device was allocated, this will fail at linker time. If
 * you get an error that looks like <tt>undefined reference to
 * __device_dts_ord_<N></tt>, that is what happened. Check to make
 * sure your device driver is being compiled, usually by enabling the
 * Kconfig options it requires.
 *
 * @param node_id A devicetree node identifier
 * @return A pointer to the device object created for that node
 */
#define DEVICE_DT_GET(node_id) (&DEVICE_DT_NAME_GET(node_id))

/**
 * @brief Get a <tt>const struct device*</tt> for an instance of a
 *        DT_DRV_COMPAT compatible
 *
 * @details This is equivalent to <tt>DEVICE_DT_GET(DT_DRV_INST(inst))</tt>.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return A pointer to the device object created for that instance
 */
#define DEVICE_DT_INST_GET(inst) DEVICE_DT_GET(DT_DRV_INST(inst))

/**
 * @brief Get a <tt>const struct device*</tt> from a devicetree compatible
 *
 * If an enabled devicetree node has the given compatible and a device
 * object was created from it, this returns a pointer to that device.
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
 * @brief Get a <tt>const struct device*</tt> from a devicetree compatible
 *
 * @details If an enabled devicetree node has the given compatible and
 * a device object was created from it, this returns a pointer to that
 * device.
 *
 * If there no such devices, this will fail at compile time.
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
 * @brief Utility macro to obtain an optional reference to a device.
 *
 * @details If the node identifier refers to a node with status
 * "okay", this returns <tt>DEVICE_DT_GET(node_id)</tt>. Otherwise, it
 * returns NULL.
 *
 * @param node_id devicetree node identifier
 *
 * @return a <tt>const struct device*</tt> for the node identifier,
 * which may be NULL.
 */
#define DEVICE_DT_GET_OR_NULL(node_id)					\
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),			\
		    (DEVICE_DT_GET(node_id)), (NULL))

/**
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

/**
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
 * @brief Get a <tt>const struct init_entry*</tt> from a devicetree node
 *
 * @param node_id A devicetree node identifier
 *
 * @return A pointer to the init_entry object created for that node
 */
#define DEVICE_INIT_DT_GET(node_id) (&Z_INIT_ENTRY_NAME(DEVICE_DT_NAME_GET(node_id)))

/**
 * @brief Get a <tt>const struct init_entry*</tt> from a device by name
 *
 * @param name The same as dev_name provided to DEVICE_DEFINE()
 *
 * @return A pointer to the init_entry object created for that device
 */
#define DEVICE_INIT_GET(name) (&Z_INIT_ENTRY_NAME(DEVICE_NAME_GET(name)))

/**
 * @brief Runtime device dynamic structure (in RAM) per driver instance
 *
 * Fields in this are expected to be default-initialized to zero. The
 * kernel driver infrastructure and driver access functions are
 * responsible for ensuring that any non-zero initialization is done
 * before they are accessed.
 */
struct device_state {
	/** Non-negative result of initializing the device.
	 *
	 * The absolute value returned when the device initialization
	 * function was invoked, or `UINT8_MAX` if the value exceeds
	 * an 8-bit integer. If initialized is also set, a zero value
	 * indicates initialization succeeded.
	 */
	unsigned int init_res : 8;

	/** Indicates the device initialization function has been
	 * invoked.
	 */
	bool initialized : 1;
};

struct pm_device;

#ifdef CONFIG_HAS_DYNAMIC_DEVICE_HANDLES
#define Z_DEVICE_HANDLES_CONST
#else
#define Z_DEVICE_HANDLES_CONST const
#endif

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
	struct device_state *state;
	/** Address of the device instance private data */
	void *data;
	/** optional pointer to handles associated with the device.
	 *
	 * This encodes a sequence of sets of device handles that have
	 * some relationship to this node. The individual sets are
	 * extracted with dedicated API, such as
	 * device_required_handles_get().
	 */
	Z_DEVICE_HANDLES_CONST device_handle_t *handles;

#ifdef CONFIG_PM_DEVICE
	/** Reference to the device PM resources. */
	struct pm_device *pm;
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

	if ((dev_handle > 0) && ((size_t)dev_handle <= numdev)) {
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
 *
 * @see device_required_foreach()
 * @see device_supported_foreach()
 */
typedef int (*device_visitor_callback_t)(const struct device *dev, void *context);

/**
 * @brief Get the device handles for devicetree dependencies of this device.
 *
 * This function returns a pointer to an array of device handles. The
 * length of the array is stored in the @p count parameter.
 *
 * The array contains a handle for each device that @p dev requires
 * directly, as determined from the devicetree. This does not include
 * transitive dependencies; you must recursively determine those.
 *
 * @param dev the device for which dependencies are desired.
 *
 * @param count pointer to where this function should store the length
 * of the returned array. No value is stored if the call returns a
 * null pointer. The value may be set to zero if the device has no
 * devicetree dependencies.
 *
 * @return a pointer to a sequence of @p *count device handles, or a null
 * pointer if @p dev does not have any dependency data.
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
 * @brief Get the device handles for injected dependencies of this device.
 *
 * This function returns a pointer to an array of device handles. The
 * length of the array is stored in the @p count parameter.
 *
 * The array contains a handle for each device that @p dev manually injected
 * as a dependency, via providing extra arguments to Z_DEVICE_DEFINE. This does
 * not include transitive dependencies; you must recursively determine those.
 *
 * @param dev the device for which injected dependencies are desired.
 *
 * @param count pointer to where this function should store the length
 * of the returned array. No value is stored if the call returns a
 * null pointer. The value may be set to zero if the device has no
 * devicetree dependencies.
 *
 * @return a pointer to a sequence of @p *count device handles, or a null
 * pointer if @p dev does not have any dependency data.
 */
static inline const device_handle_t *
device_injected_handles_get(const struct device *dev,
			    size_t *count)
{
	const device_handle_t *rv = dev->handles;
	size_t region = 0;
	size_t i = 0;

	if (rv != NULL) {
		/* Fast forward to injected devices */
		while (region != 1) {
			if (*rv == DEVICE_HANDLE_SEP) {
				region++;
			}
			rv++;
		}
		while ((rv[i] != DEVICE_HANDLE_ENDS)
		       && (rv[i] != DEVICE_HANDLE_SEP)) {
			++i;
		}
		*count = i;
	}

	return rv;
}

/**
 * @brief Get the set of handles that this device supports.
 *
 * This function returns a pointer to an array of device handles. The
 * length of the array is stored in the @p count parameter.
 *
 * The array contains a handle for each device that @p dev "supports"
 * -- that is, devices that require @p dev directly -- as determined
 * from the devicetree. This does not include transitive dependencies;
 * you must recursively determine those.
 *
 * @param dev the device for which supports are desired.
 *
 * @param count pointer to where this function should store the length
 * of the returned array. No value is stored if the call returns a
 * null pointer. The value may be set to zero if nothing in the
 * devicetree depends on @p dev.
 *
 * @return a pointer to a sequence of @p *count device handles, or a null
 * pointer if @p dev does not have any dependency data.
 */
static inline const device_handle_t *
device_supported_handles_get(const struct device *dev,
			     size_t *count)
{
	const device_handle_t *rv = dev->handles;
	size_t region = 0;
	size_t i = 0;

	if (rv != NULL) {
		/* Fast forward to supporting devices */
		while (region != 2) {
			if (*rv == DEVICE_HANDLE_SEP) {
				region++;
			}
			rv++;
		}
		/* Count supporting devices */
		while (rv[i] != DEVICE_HANDLE_ENDS) {
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
 * controller for communication. Required devices can derive from
 * statically-defined devicetree relationships or dependencies registered
 * at runtime.
 *
 * This API supports operating on the set of required devices. Example uses
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
 * @param dev a device of interest. The devices that this device depends on
 * will be used as the set of devices to visit. This parameter must not be
 * null.
 *
 * @param visitor_cb the function that should be invoked on each device in the
 * dependency set. This parameter must not be null.
 *
 * @param context state that is passed through to the visitor function. This
 * parameter may be null if @p visitor tolerates a null @p context.
 *
 * @return The number of devices that were visited if all visits succeed, or
 * the negative value returned from the first visit that did not succeed.
 */
int device_required_foreach(const struct device *dev,
			  device_visitor_callback_t visitor_cb,
			  void *context);

/**
 * @brief Visit every device that @p dev directly supports.
 *
 * Zephyr maintains information about which devices are directly supported by
 * another device; for example an I2C controller will support an I2C-based
 * sensor driver. Supported devices can derive from statically-defined
 * devicetree relationships.
 *
 * This API supports operating on the set of supported devices. Example uses
 * include iterating over the devices connected to a regulator when it is
 * powered on.
 *
 * There is no guarantee on the order in which required devices are visited.
 *
 * If the @p visitor function returns a negative value iteration is halted,
 * and the returned value from the visitor is returned from this function.
 *
 * @note This API is not available to unprivileged threads.
 *
 * @param dev a device of interest. The devices that this device supports
 * will be used as the set of devices to visit. This parameter must not be
 * null.
 *
 * @param visitor_cb the function that should be invoked on each device in the
 * support set. This parameter must not be null.
 *
 * @param context state that is passed through to the visitor function. This
 * parameter may be null if @p visitor tolerates a null @p context.
 *
 * @return The number of devices that were visited if all visits succeed, or
 * the negative value returned from the first visit that did not succeed.
 */
int device_supported_foreach(const struct device *dev,
			     device_visitor_callback_t visitor_cb,
			     void *context);

/**
 * @brief Get a <tt>const struct device*</tt> from its @p name field
 *
 * @details This function iterates through the devices on the system.
 * If a device with the given @p name field is found, and that device
 * initialized successfully at boot time, this function returns a
 * pointer to the device.
 *
 * If no device has the given name, this function returns NULL.
 *
 * This function also returns NULL when a device is found, but it
 * failed to initialize successfully at boot time. (To troubleshoot
 * this case, set a breakpoint on your device driver's initialization
 * function.)
 *
 * @param name device name to search for. A null pointer, or a pointer
 * to an empty string, will cause NULL to be returned.
 *
 * @return pointer to device structure with the given name; NULL if
 * the device is not found or if the device with that name's
 * initialization function failed.
 */
__syscall const struct device *device_get_binding(const char *name);

/** @brief Get access to the static array of static devices.
 *
 * @param devices where to store the pointer to the array of
 * statically allocated devices. The array must not be mutated
 * through this pointer.
 *
 * @return the number of statically allocated devices.
 */
size_t z_device_get_all_static(const struct device * *devices);

/**
 * @brief Verify that a device is ready for use.
 *
 * This is the implementation underlying device_is_ready(), without the overhead
 * of a syscall wrapper.
 *
 * @param dev pointer to the device in question.
 *
 * @retval true If the device is ready for use.
 * @retval false If the device is not ready for use or if a NULL device pointer
 * is passed as argument.
 *
 * @see device_is_ready()
 */
bool z_device_is_ready(const struct device *dev);

/** @brief Verify that a device is ready for use.
 *
 * Indicates whether the provided device pointer is for a device known to be
 * in a state where it can be used with its standard API.
 *
 * This can be used with device pointers captured from DEVICE_DT_GET(), which
 * does not include the readiness checks of device_get_binding(). At minimum
 * this means that the device has been successfully initialized.
 *
 * @param dev pointer to the device in question.
 *
 * @retval true If the device is ready for use.
 * @retval false If the device is not ready for use or if a NULL device pointer
 * is passed as argument.
 */
__syscall bool device_is_ready(const struct device *dev);

static inline bool z_impl_device_is_ready(const struct device *dev)
{
	return z_device_is_ready(dev);
}

/**
 * @}
 */

/* Synthesize the name of the object that holds device ordinal and
 * dependency data. If the object doesn't come from a devicetree
 * node, use dev_name.
 */
#define Z_DEVICE_HANDLE_NAME(node_id, dev_name)				\
	_CONCAT(__devicehdl_,						\
		COND_CODE_1(DT_NODE_EXISTS(node_id),			\
			    (node_id),					\
			    (dev_name)))

#define Z_DEVICE_EXTRA_HANDLES(...)				\
	FOR_EACH_NONEMPTY_TERM(IDENTITY, (,), __VA_ARGS__)

/*
 * Utility macro to define and initialize the device state.
 *
 * @param node_id Devicetree node id of the device.
 * @param dev_name Device name.
 */
#define Z_DEVICE_STATE_DEFINE(node_id, dev_name)			\
	static struct device_state Z_DEVICE_STATE_NAME(dev_name)	\
	__attribute__((__section__(".z_devstate")));

/* Construct objects that are referenced from struct device. These
 * include power management and dependency handles.
 */
#define Z_DEVICE_DEFINE_PRE(node_id, dev_name, ...)			\
	Z_DEVICE_DEFINE_HANDLES(node_id, dev_name, __VA_ARGS__)

/* Initial build provides a record that associates the device object
 * with its devicetree ordinal, and provides the dependency ordinals.
 * These are provided as weak definitions (to prevent the reference
 * from being captured when the original object file is compiled), and
 * in a distinct pass1 section (which will be replaced by
 * postprocessing).
 *
 * Before processing in gen_handles.py, the array format is:
 * {
 *     DEVICE_ORDINAL (or DEVICE_HANDLE_NULL if not a devicetree node),
 *     List of devicetree dependency ordinals (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of injected dependency ordinals (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of devicetree supporting ordinals (if any),
 * }
 *
 * After processing in gen_handles.py, the format is updated to:
 * {
 *     List of existing devicetree dependency handles (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of injected devicetree dependency handles (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of existing devicetree support handles (if any),
 *     DEVICE_HANDLE_NULL
 * }
 *
 * It is also (experimentally) necessary to provide explicit alignment
 * on each object. Otherwise x86-64 builds will introduce padding
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
	extern Z_DEVICE_HANDLES_CONST device_handle_t			\
		Z_DEVICE_HANDLE_NAME(node_id, dev_name)[];		\
	Z_DEVICE_HANDLES_CONST device_handle_t				\
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
			DEVICE_HANDLE_SEP,				\
	COND_CODE_1(DT_NODE_EXISTS(node_id),				\
			(DT_SUPPORTS_DEP_ORDS(node_id)), ())		\
		};

#define Z_DEVICE_DEFINE_INIT(node_id, dev_name)				\
	.handles = Z_DEVICE_HANDLE_NAME(node_id, dev_name),

/* Like DEVICE_DEFINE but takes a node_id AND a dev_name, and trailing
 * dependency handles that come from outside devicetree.
 */
#define Z_DEVICE_DEFINE(node_id, dev_name, drv_name, init_fn, pm_device,\
			data_ptr, cfg_ptr, level, prio, api_ptr, state_ptr, ...)	\
	Z_DEVICE_DEFINE_PRE(node_id, dev_name, __VA_ARGS__)		\
	COND_CODE_1(DT_NODE_EXISTS(node_id), (), (static))		\
		const Z_DECL_ALIGN(struct device)			\
		DEVICE_NAME_GET(dev_name) __used			\
	__attribute__((__section__(".z_device_" #level STRINGIFY(prio)"_"))) = { \
		.name = drv_name,					\
		.config = (cfg_ptr),					\
		.api = (api_ptr),					\
		.state = (state_ptr),					\
		.data = (data_ptr),					\
		COND_CODE_1(CONFIG_PM_DEVICE, (.pm = pm_device,), ())	\
		Z_DEVICE_DEFINE_INIT(node_id, dev_name)			\
	};								\
	BUILD_ASSERT(sizeof(Z_STRINGIFY(drv_name)) <= Z_DEVICE_MAX_NAME_LEN, \
		     Z_STRINGIFY(DEVICE_NAME_GET(drv_name)) " too long"); \
	Z_INIT_ENTRY_DEFINE(DEVICE_NAME_GET(dev_name), init_fn,		\
		(&DEVICE_NAME_GET(dev_name)), level, prio)

#ifdef CONFIG_LEGACY_DEVICE_MODEL

/*
 * The legacy device model can be used with the the new DEVICE_NEW_DEFINE() and
 * DEVICE_DT_NEW_DEFINE() macros, which will expand to the legacy DEVICE_DEFINE()
 * and DEVICE_DT_DEFINE() macros.
 *
 * This feature will allow for updating drivers to use the new multi API device
 * model without breaking compatibility with the legacy device model, given that
 * the device drivers only support a single API.
 *
 * Example from uart driver before update to new device model:
 *
 *     DEVICE_DT_DEFINE(node_id, ..., &uart_api_impl);
 *
 * Example from uart driver after update to new device model:
 *
 *     DEVICE_DT_NEW_DEFINE(node_id, ..., DEVICE_API(&uart_api_impl, uart));
 *
 * Using the legacy device model, both the legacy DEVICE_DT_DEFINE() and new
 * driver DEVICE_DT_NEW_DEFINE() will expand identically. The device instances
 * can be obtained using DEVICE_DT_GET(). The macros DEVICE_API_GET() and
 * DEVICE_DT_API_GET() and their INST variants are not available. The new
 * properties files and accompanying macros like DEVICE_DT_API_FOREACH(),
 * DEVICE_DT_PROPERTY() etc. are available.
 */

#define DEVICE_NEW_DEFINE(dev_name, drv_name, init_fn, pm_dev_ptr, data_ptr,        \
			cfg_ptr, init_level, init_prio, api_spec)                               \
	DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_dev_ptr, data_ptr, cfg_ptr,       \
		init_level, init_prio, Z_DEVICE_API_API_PTR(api_spec))

#define DEVICE_DT_NEW_DEFINE(node_id, init_fn, pm_dev_ptr, data_ptr, cfg_ptr,       \
			init_level, init_prio, api_spec, ...)                                   \
	DEVICE_DT_DEFINE(node_id, init_fn, pm_dev_ptr, data_ptr, cfg_ptr, init_level,   \
		init_prio, Z_DEVICE_API_API_PTR(api_spec), __VA_ARGS__)

#define DEVICE_DT_INST_NEW_DEFINE(inst, ...) \
	DEVICE_DT_NEW_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

#endif /* CONFIG_LEGACY_DEVICE_MODEL */

/**
 * @brief Multi API Device Model APIs
 * @defgroup device_model_multi_api Multi API Device Model APIs
 *
 * @note The multi API device model doesn't support device handles or
 * runtime device lookup using device_get_binding(). Use devicetree node
 * identifiers with the DT and DEVICE_DT macros to look up and listify
 * devices at compile time instead.
 *
 * @{
 */

/**
 * @def DEVICE_API
 *
 * @brief Named tuple containing device API parameters used during device
 * definition.
 *
 * @details The API type specified using this macro exist in the properties
 * file associated with the device driver using the compatible property. An
 * example which attributes the APIs "uart" and "spi" to the device driver
 * with the compatible "dummy" follows:
 *
 *   compatible: "dummy"
 *   api: ["uart", "spi"]
 *
 * These properties in the properties file allow for defining the following:
 *
 *   DEVICE_DT_NEW_DEFINE(node_id, ...,
 *       DEVICE_API(&dymmy_uart_api_impl, uart),
 *       DEVICE_API(&dymmy_spi_api_impl, spi));
 *
 * @param api_ptr Pointer to the device's API structure.
 *
 * @param api_type API type
 */
#define DEVICE_API(api_ptr, api_type) (api_ptr, api_type)

#ifndef CONFIG_LEGACY_DEVICE_MODEL

/**
 * @def DEVICE_NEW_DEFINE
 *
 * @brief Define required <tt>struct device</tt> objects for device and
 * set it up for boot time initialization.
 *
 * @details A <tt>struct device</tt> is created for each specified API.
 *
 * The following example shows how to define three <tt>struct device</tt>
 * objects for a device which implements the uart, i2c and spi APIs.
 *
 * DEVICE_NEW_DEFINE(dev_name, drv_name, ...,
 *     DEVICE_API(&uart_ptr, uart),
 *     DEVICE_API(&i2c_ptr, i2c),
 *     DEVICE_API(&spi_ptr, spi));
 *
 * See DEVICE_API_GET() to get a pointer to a <tt>struct device</tt>
 * defined using this macro.
 *
 * @param dev_name A unique token which is used as the base name for
 * the <tt>struct device</tt> objects C identifiers. The API type of
 * the <tt>struct device</tt> object is appended to the C identifier.
 *
 * @param drv_name A string base name for the <tt>struct device</tt>
 * objects, which will be stored in the <tt>struct device</tt>
 * object's @p name field. The API type of the <tt>struct device</tt>
 * is appended to the name.
 *
 * @param init_fn Pointer to the device's initialization function,
 * which will be run by the kernel during system initialization.
 *
 * @param pm_dev_ptr Pointer to the device's power management
 * resources, a <tt>struct pm_device</tt>, which will be stored in all
 * <tt>struct device</tt> objects @p pm field. Set to NULL if the
 * device does not use PM.
 *
 * @param data_ptr Pointer to the device's private mutable data, which
 * is stored in the <tt>struct device</tt> objects @p data field.
 *
 * @param cfg_ptr Pointer to the device's private constant data, which
 * is stored in the <tt>struct device</tt> objects @p config field.
 *
 * @param init_level The device's initialization level. See SYS_INIT() for
 * details.
 *
 * @param init_prio The device's priority within its initialization level.
 * See SYS_INIT() for details.
 *
 * @param api_spec The device's primary API specification, defined using
 * DEVICE_API().
 *
 * @param ... List of further unique API specifications.
 */
#define DEVICE_NEW_DEFINE(dev_name, drv_name, init_fn, pm_dev_ptr, data_ptr,        \
			cfg_ptr, init_level, init_prio, api_spec, ...)                          \
	Z_DEVICE_NEW_STATE_DEFINE(dev_name)                                             \
	                                                                                \
	Z_DEVICE_NEW_DEFINE(dev_name, drv_name, init_fn, pm_dev_ptr, data_ptr,          \
		cfg_ptr, init_level, init_prio, Z_DEVICE_API_API_PTR(api_spec),             \
		Z_DEVICE_API_API_TYPE(api_spec),                                            \
		&Z_DEVICE_NEW_STATE_NAME(dev_name))                                         \
	                                                                                \
	Z_DEVICE_ASSIGN_ADDITIONAL_APIS(                                                \
		Z_DEVICE_DEV_TUPLE(dev_name, drv_name,                                      \
			Z_DEVICE_API_API_TYPE(api_spec), pm_dev_ptr, data_ptr, cfg_ptr),        \
		__VA_ARGS__)

/**
 * @def DEVICE_API_GET
 *
 * @brief Get a const struct device pointer from a device defined using
 * DEVICE_API_DEFINE().
 *
 * @details The following example shows how to get a pointer to three
 * <tt>struct device</tt> objects specified by API type, defined using
 * DEVICE_API_DEFINE().
 *
 * static const struct device uart_dev = DEVICE_API_GET(dev_name, uart);
 * static const struct device i2c_dev = DEVICE_API_GET(dev_name, i2c);
 * static const struct device spi_dev = DEVICE_API_GET(dev_name, spi);
 *
 * @note DEVICE_API_GET() is only useful within the source file which
 * defined the struct devices since they are static.
 *
 * @param dev_name The device name used when defining the device using
 * DEVICE_API_DEFINE().
 *
 * @param api_type Specifies which <tt>struct device</tt> object to get.
 *
 * @return A pointer to the <tt>struct device</tt> object.
 */
#define DEVICE_API_GET(dev_name, api_type) \
	(&Z_DEVICE_API_NAME(dev_name, api_type))

/**
 * @def DEVICE_DT_NEW_DEFINE
 *
 * @brief Define required <tt>struct device</tt> objects for device
 * declared in devicetree and set it up for boot time initialization.
 *
 * @details A <tt>struct device</tt> is created for each specified API.
 *
 * The following example shows how to define three <tt>struct device</tt>
 * objects for a device which implements the uart, i2c and spi APIs.
 *
 * DEVICE_DT_NEW_DEFINE(node_id, ...,
 *     DEVICE_API(&uart_ptr, uart),
 *     DEVICE_API(&i2c_ptr, i2c),
 *     DEVICE_API(&spi_ptr, spi));
 *
 * See DEVICE_DT_API_GET() to get a pointer to a <tt>struct device</tt>
 * defined using this macro.
 *
 * @param node_id The devicetree node identifier which the struct devices
 * belong to.
 *
 * @param init_fn Pointer to the device's initialization function,
 * which will be run by the kernel during system initialization.
 *
 * @param pm_dev_ptr Pointer to the device's power management
 * resources, a <tt>struct pm_device</tt>, which will be stored in all
 * <tt>struct device</tt> objects @p pm field. Set to NULL if the
 * device does not use PM.
 *
 * @param data_ptr Pointer to the device's private mutable data, which
 * is stored in the <tt>struct device</tt> objects @p data field.
 *
 * @param cfg_ptr Pointer to the device's private constant data, which
 * is stored in the <tt>struct device</tt> objects @p config field.
 *
 * @param init_level The device's initialization level. See SYS_INIT() for
 * details.
 *
 * @param init_prio The device's priority within its initialization level.
 * See SYS_INIT() for details.
 *
 * @param api_spec The device's primary API specification, defined using
 * DEVICE_API().
 *
 * @param ... List of further unique API specifications.
 */
#define DEVICE_DT_NEW_DEFINE(node_id, init_fn, pm_dev_ptr, data_ptr, cfg_ptr,       \
			init_level, init_prio, api_spec, ...)                                   \
	Z_DEVICE_NEW_STATE_DEFINE(Z_DEVICE_DT_NAME_FROM_NODE(node_id))                  \
	                                                                                \
	Z_DEVICE_DT_NEW_DEFINE(node_id, init_fn, pm_dev_ptr, data_ptr,                  \
		cfg_ptr, init_level, init_prio, Z_DEVICE_API_API_PTR(api_spec),             \
		Z_DEVICE_API_API_TYPE(api_spec),                                            \
		&Z_DEVICE_NEW_STATE_NAME(Z_DEVICE_DT_NAME_FROM_NODE(node_id)))              \
	                                                                                \
	Z_DEVICE_DT_ASSIGN_ADDITIONAL_APIS(                                             \
		Z_DEVICE_DT_DEV_TUPLE(node_id, Z_DEVICE_API_API_TYPE(api_spec),             \
			pm_dev_ptr, data_ptr, cfg_ptr),                                         \
		__VA_ARGS__)

/**
 * @def DEVICE_DT_INST_NEW_DEFINE
 *
 * @brief Wrapper macro for DEVICE_DT_NEW_DEFINE() using the compatible
 * instance identifier instead of the node_id.
 *
 * @details This is equivalent to DEVICE_DT_NEW_DEFINE(DT_DRV_INST(inst), ...).
 *
 * @param inst DT_DRV_COMPAT instance index
 */
#define DEVICE_DT_INST_NEW_DEFINE(inst, ...) \
	DEVICE_DT_NEW_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @def DEVICE_DT_API_GET
 *
 * @brief Get a const struct device* from a devicetree node
 * identifier which implements the specified API type
 *
 * @details The following example shows how to get a pointer to three
 * <tt>struct device</tt> objects specified by API type, defined using
 * DEVICE_DT_NEW_DEFINE().
 *
 * const struct device uart_dev = DEVICE_DT_API_GET(node_id, uart);
 * const struct device i2c_dev = DEVICE_DT_API_GET(node_id, i2c);
 * const struct device spi_dev = DEVICE_DT_API_GET(node_id, spi);
 *
 * If a specific <tt>struct device</tt> object has not been allocated,
 * this will fail at linker time. If you get an error that looks like
 * undefined reference to __api_device_dts_ord_<N>, that is what happened.
 * Check to make sure your device driver is being compiled, usually by
 * enabling the Kconfig options it requires.
 *
 * @param node_id The devicetree node identifier which the
 * <tt>struct device</tt> object belongs to.
 *
 * @param api_type Specifies which <tt>struct device</tt> object to get.
 *
 * @return A pointer to the <tt>struct device</tt> object.
 */
#define DEVICE_DT_API_GET(node_id, api_type) \
	(&Z_DEVICE_DT_API_NAME(node_id, api_type))

/**
 * @def DEVICE_DT_INST_API_GET
 *
 * @brief Get a const struct device* for an instance of a
 * DT_DRV_COMPAT compatible.
 *
 * @details This is equivalent to
 * DEVICE_DT_API_GET(DT_DRV_INST(inst), ...).
 *
 * @param inst Compatible instance index.
 *
 * @return A pointer to the <tt>struct device</tt> object.
 */
#define DEVICE_DT_INST_API_GET(inst, ...) \
	DEVICE_DT_API_GET(DT_DRV_INST(inst), __VA_ARGS__)

#endif /* CONFIG_LEGACY_DEVICE_MODEL */

/**
 * @def DEVICE_DT_NEW_SUPPORTED
 *
 * @brief Test if device supports API.
 *
 * @param node_id Devicetree node identifier to test.
 *
 * @param api_type API type to test for.
 */
#define DEVICE_DT_API_SUPPORTED(node_id, api_type)                                  \
	IS_ENABLED(_CONCAT(_CONCAT(_CONCAT(_CONCAT(DEVICE_, node_id), _API_),           \
		api_type), _EXISTS))

/**
 * @def DEVICE_DT_INST_API_SUPPORTED
 *
 * @brief Test if device supports API
 *
 * @param inst Compatible instance id of device to test
 *
 * @param api_type API type to test for
 */
#define DEVICE_DT_INST_API_SUPPORTED(inst, api_type) \
	DEVICE_DT_API_SUPPORTED(DT_DRV_INST(inst), api_type)

/**
 * @def DEVICE_DT_API_SUPPORTED_ANY
 *
 * @brief Test if any enabled device supports provided API
 *
 * @param api_type API type to test for
 */
#define DEVICE_DT_API_SUPPORTED_ANY(api_type) \
	IS_ENABLED(_CONCAT(_CONCAT(DEVICE_DT_API_, api_type), _FOREACH_EXISTS))

/**
 * @def DEVICE_DT_API_FOREACH
 *
 * @brief Invoke fn macro for each node which implements the provided
 * API type. The fn macro must take a single argument, which is the
 * node identifier.
 *
 * @param fn Macro which is invoked for each node which supports provided API type.
 *
 * @param api_type API type which specifies which nodes to invoke fn macro for.
 */
#define DEVICE_DT_API_FOREACH(fn, api_type)                                         \
	COND_CODE_1(                                                                    \
		IS_ENABLED(_CONCAT(_CONCAT(DEVICE_DT_API_, api_type), _FOREACH_EXISTS)),    \
		(_CONCAT(_CONCAT(DEVICE_DT_API_, api_type), _FOREACH)(fn)),                 \
		())

/**
 * @def DEVICE_DT_API_FOREACH_VARGS
 *
 * @brief Invoke fn macro for each node which implements the provided
 * API type. The fn macro must take multiple arguments, the first argument
 * is the node identifier.
 *
 * @param fn Macro which is invoked for each node which supports provided API type.
 *
 * @param api_type API type which specifies which nodes to invoke fn macro for.
 *
 * @param ... Additional arguments to pass to fn macro.
 */
#define DEVICE_DT_API_FOREACH_VARGS(fn, api_type, ...)                              \
	COND_CODE_1(                                                                    \
		IS_ENABLED(_CONCAT(_CONCAT(DEVICE_DT_API_, api_type), _FOREACH_EXISTS)),    \
		(_CONCAT(_CONCAT(DEVICE_DT_API_, api_type), _FOREACH_VARGS)                 \
			(fn, __VA_ARGS__)),                                                     \
		())

/**
 * @def DEVICE_DT_PROPERTY
 *
 * @brief Get a device property using a node identifier
 *
 * @details Device properties are defined in properties files. An example which
 * defines the property vendor which will expand to "dummy" follows:
 *
 *   properties:
 *     vendor:
 *       value: "dummy"
 *
 * The properties files are tied to device nodes using the compatible property. To
 * fetch the vendor property at compile time for a node, use the following macro:
 *
 *   DEVICE_DT_PROPERTY(node_id, vendor)
 *
 * @param node_id Devicetree node identifier to test.
 *
 * @param property Property to fetch.
 */
#define DEVICE_DT_PROPERTY(node_id, property) \
	_CONCAT(_CONCAT(_CONCAT(DEVICE_, node_id), _PROPERTY_), property)

/**
 * @def DEVICE_DT_INST_PROPERTY
 *
 * @brief Get a device property using a compatible instance index
 *
 * @details See DEVICE_DT_PROPERTY() for details.
 *
 * @param inst Compatible instance index of the device
 *
 * @param property Property to fetch.
 */
#define DEVICE_DT_INST_PROPERTY(inst, property) \
	DEVICE_DT_PROPERTY(DT_DRV_INST(inst), property)

/**
 * @def DEVICE_DT_PROPERTY_OR
 *
 * @brief Get a device property using a node identifier with fallback to default
 * value if the device property doesn't exist.
 *
 * @details See DEVICE_DT_PROPERTY() for details.
 *
 * @param node_id Devicetree node identifier to test.
 *
 * @param property Property to fetch.
 *
 * @param default Default value used if device property doesn't exist.
 */
#define DEVICE_DT_PROPERTY_OR(node_id, property, default)                           \
	COND_CODE_1(                                                                    \
		IS_ENABLED(_CONCAT(_CONCAT(_CONCAT(DEVICE_DT_, node_id), _PROPERTY_),       \
			property)),                                                             \
		(_CONCAT(_CONCAT(_CONCAT(DEVICE_, node_id), _PROPERTY_), property)),        \
		(default))

/**
 * @def DEVICE_DT_INST_PROPERTY_OR
 *
 * @brief Get a device property using a compatible instance index with fallback to
 * default value if the device property doesn't exist.
 *
 * @details See DEVICE_DT_PROPERTY() for details.
 *
 * @param inst Compatible instance index of the device
 *
 * @param property Property to fetch.
 *
 * @param default Default value used if device property doesn't exist.
 */
#define DEVICE_DT_INST_PROPERTY_OR(inst, property, default) \
	DEVICE_DT_PROPERTY_OR(DT_DRV_INST(inst), property, default)

/**
 * @}
 */

/*
 * Private Multi API Device Model APIs shared between static and devicetree devices
 */

/* Helper which extracts API pointer from DEVICE_API */
#define Z_DEVICE_API_API_PTR_(api_ptr, api_type) api_ptr
#define Z_DEVICE_API_API_PTR(api_spec) Z_DEVICE_API_API_PTR_ api_spec

#ifndef CONFIG_LEGACY_DEVICE_MODEL

/* Helper which extracts API type from DEVICE_API */
#define Z_DEVICE_API_API_TYPE_(api_ptr, api_type) api_type
#define Z_DEVICE_API_API_TYPE(api_spec) Z_DEVICE_API_API_TYPE_ api_spec

/* Helper which generates device name from name and API type */
#define Z_DEVICE_API_NAME(name, api_type)                                           \
	_CONCAT(_CONCAT(_CONCAT(__api_device_, name), _), api_type)

/* Helper which generates device name from devicetree node id and API type */
#define Z_DEVICE_DT_API_NAME(node_id, api_type)                                     \
	Z_DEVICE_API_NAME(Z_DEVICE_DT_NAME_FROM_NODE(node_id), api_type)

/* Helper which places API device struct in correct section in correct order */
#define Z_DEVICE_API_PLACE_IN_SECTION(api_type)                                     \
	__attribute__((__section__(".z_api_device")))

/* Helper which creates a name from devicetree node id */
#define Z_DEVICE_DT_NAME_FROM_NODE(node_id)                                         \
	_CONCAT(dts_ord_, DT_DEP_ORD(node_id))

/* Helper which creates a name for the API device state struct */
#define Z_DEVICE_NEW_STATE_NAME(name)                                               \
	_CONCAT(__api_devstate_, name)

/* Define a device state struct and place it in correct section */
#define Z_DEVICE_NEW_STATE_DEFINE(name)                                             \
	static struct device_state Z_DEVICE_NEW_STATE_NAME(name)                        \
		__attribute__((__section__(".z_devstate")));

/* Helper which creates name string from driver name and API type */
#define Z_DEVICE_API_FULL_NAME(drv_name, api_type)                                  \
	(drv_name "_" STRINGIFY(api_type))

/* Helper which creates name string from devicetree node and API type */
#define Z_DEVICE_DT_API_FULL_NAME(node_id, api_type)                                \
	(DT_NODE_FULL_NAME(node_id) "_" STRINGIFY(api_type))

/*
 * Private Multi API Device Model APIs specific to static devices
 */

/* Named tuple containing device parameters used when assigning API devices */
#define Z_DEVICE_DEV_TUPLE(dev_name, drv_name, api_type, pm_dev_ptr, data_ptr,      \
	cfg_ptr) (dev_name, drv_name, api_type, pm_dev_ptr, data_ptr, cfg_ptr)

/* Helper which extracts device name from Z_DEVICE_DEV_TUPLE */
#define Z_DEVICE_DEV_TUPLE_DEV_NAME_(dev_name, drv_name, api_type, pm_dev_ptr,      \
	data_ptr, cfg_ptr) dev_name

#define Z_DEVICE_DEV_TUPLE_DEV_NAME(dev_spec)                                       \
	Z_DEVICE_DEV_TUPLE_DEV_NAME_ dev_spec

/* Helper which extracts driver name from Z_DEVICE_DEV_TUPLE */
#define Z_DEVICE_DEV_TUPLE_DRV_NAME_(dev_name, drv_name, api_type, pm_dev_ptr,      \
	data_ptr, cfg_ptr) drv_name

#define Z_DEVICE_DEV_TUPLE_DRV_NAME(dev_spec)                                       \
	Z_DEVICE_DEV_TUPLE_DRV_NAME_ dev_spec

/* Helper which extracts API type from Z_DEVICE_DEV_TUPLE */
#define Z_DEVICE_DEV_TUPLE_API_TYPE_(dev_name, drv_name, api_type, pm_dev_ptr,      \
	data_ptr, cfg_ptr) api_type

#define Z_DEVICE_DEV_TUPLE_API_TYPE(dev_spec)                                       \
	Z_DEVICE_DEV_TUPLE_API_TYPE_ dev_spec

/* Helper which extracts PM device pointer from Z_DEVICE_DEV_TUPLE */
#define Z_DEVICE_DEV_TUPLE_PM_DEV_PTR_(dev_name, drv_name, api_type,                \
	pm_dev_ptr, data_ptr, cfg_ptr) pm_dev_ptr

#define Z_DEVICE_DEV_TUPLE_PM_DEV_PTR(dev_spec)                                     \
	Z_DEVICE_DEV_TUPLE_PM_DEV_PTR_ dev_spec

/* Helper which extracts data pointer from Z_DEVICE_DEV_TUPLE */
#define Z_DEVICE_DEV_TUPLE_DATA_PTR_(dev_name, drv_name, api_type, pm_dev_ptr,      \
	data_ptr, cfg_ptr) data_ptr

#define Z_DEVICE_DEV_TUPLE_DATA_PTR(dev_spec)                                       \
	Z_DEVICE_DEV_TUPLE_DATA_PTR_ dev_spec

/* Helper which extracts config pointer from Z_DEVICE_DEV_TUPLE */
#define Z_DEVICE_DEV_TUPLE_CFG_PTR_(dev_name, drv_name, api_type, pm_dev_ptr,       \
	data_ptr, cfg_ptr) cfg_ptr

#define Z_DEVICE_DEV_TUPLE_CFG_PTR(dev_spec)                                        \
	Z_DEVICE_DEV_TUPLE_CFG_PTR_ dev_spec

/*
 * Helper which defines an API device and its dependencies, places it in the correct
 * section, and sets it up for initialization on boot.
 */
#define Z_DEVICE_NEW_DEFINE(dev_name, drv_name, init_fn, pm_dev_ptr, data_ptr,      \
			cfg_ptr, init_level, init_prio, api_ptr, api_type, state_ptr)           \
	static const Z_DECL_ALIGN(struct device) Z_DEVICE_API_NAME(dev_name, api_type)  \
		Z_DEVICE_API_PLACE_IN_SECTION(api_type) = {                                 \
		.name = drv_name,                                                           \
		.config = (cfg_ptr),                                                        \
		.api = (api_ptr),                                                           \
		.state = (state_ptr),                                                       \
		.data = (data_ptr),                                                         \
		COND_CODE_1(CONFIG_PM_DEVICE, (.pm = pm_dev_ptr,), ())                      \
	};                                                                              \
                                                                                    \
	Z_INIT_ENTRY_DEFINE(Z_DEVICE_API_NAME(dev_name, api_type), init_fn,             \
		&(Z_DEVICE_API_NAME(dev_name, api_type)), init_level, init_prio);

/*
 * Helper which creates a device object which shares all members excluding the name
 * and API pointer.
 */
#define Z_DEVICE_API_ASSIGN(api_spec, dev_spec)                                     \
	const Z_DECL_ALIGN(struct device)                                               \
		Z_DEVICE_API_NAME(Z_DEVICE_DEV_TUPLE_DEV_NAME(dev_spec),                    \
			Z_DEVICE_API_API_TYPE(api_spec))                                        \
		Z_DEVICE_API_PLACE_IN_SECTION(Z_DEVICE_API_API_TYPE(api_spec)) = {          \
		.name = Z_DEVICE_API_FULL_NAME(                                             \
			Z_DEVICE_DEV_TUPLE_DRV_NAME(dev_spec),                                  \
			Z_DEVICE_API_API_TYPE(api_spec)),                                       \
		.config = Z_DEVICE_DEV_TUPLE_CFG_PTR(dev_spec),                             \
		.api = Z_DEVICE_API_API_PTR(api_spec),                                      \
		.state = &Z_DEVICE_NEW_STATE_NAME(                                          \
			Z_DEVICE_DEV_TUPLE_DEV_NAME(dev_spec)),                                 \
		.data = Z_DEVICE_DEV_TUPLE_DATA_PTR(dev_spec),                              \
		COND_CODE_1(CONFIG_PM_DEVICE,                                               \
			(.pm = Z_DEVICE_DEV_TUPLE_PM_DEV_PTR(dev_spec)), ())                    \
	}

/* Helper which assigns every api_spec passed in VA_ARGS to dev_spec */
#define Z_DEVICE_ASSIGN_ADDITIONAL_APIS(dev_spec, ...)                              \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(LIST_DROP_EMPTY(__VA_ARGS__, _)),                \
		(),                                                                         \
		(FOR_EACH_FIXED_ARG(Z_DEVICE_API_ASSIGN, (;), dev_spec,                     \
			LIST_DROP_EMPTY(__VA_ARGS__))))

/*
 * Private Multi API Device Model APIs specific to devicetree device
 */

/* Named tuple containing device parameters used when assigning API devices */
#define Z_DEVICE_DT_DEV_TUPLE(node_id, api_type, pm_dev_ptr, data_ptr, cfg_ptr)     \
	(node_id, api_type, pm_dev_ptr, data_ptr, cfg_ptr)

/* Helper which extracts node id from Z_DEVICE_DT_DEV_TUPLE */
#define Z_DEVICE_DT_DEV_TUPLE_NODE_ID_(node_id, api_type, pm_dev_ptr, data_ptr,     \
	cfg_ptr) node_id

#define Z_DEVICE_DT_DEV_TUPLE_NODE_ID(dev_dt_spec)                                  \
	Z_DEVICE_DT_DEV_TUPLE_NODE_ID_ dev_dt_spec

/* Helper which extracts API type from Z_DEVICE_DT_DEV_TUPLE */
#define Z_DEVICE_DT_DEV_TUPLE_API_TYPE_(node_id, api_type, pm_dev_ptr,              \
	data_ptr, cfg_ptr) api_type

#define Z_DEVICE_DT_DEV_TUPLE_API_TYPE(dev_dt_spec)                                 \
	Z_DEVICE_DT_DEV_TUPLE_API_TYPE_ dev_dt_spec

/* Helper which extracts PM device pointer from Z_DEVICE_DT_DEV_TUPLE */
#define Z_DEVICE_DT_DEV_TUPLE_PM_DEV_PTR_(node_id, api_type, pm_dev_ptr,            \
	data_ptr, cfg_ptr) pm_dev_ptr

#define Z_DEVICE_DT_DEV_TUPLE_PM_DEV_PTR(dev_dt_spec)                               \
	Z_DEVICE_DT_DEV_TUPLE_PM_DEV_PTR_ dev_dt_spec

/* Helper which extracts data pointer from Z_DEVICE_DT_DEV_TUPLE */
#define Z_DEVICE_DT_DEV_TUPLE_DATA_PTR_(node_id, api_type, pm_dev_ptr, data_ptr,    \
	cfg_ptr) data_ptr

#define Z_DEVICE_DT_DEV_TUPLE_DATA_PTR(dev_dt_spec)                                 \
	Z_DEVICE_DT_DEV_TUPLE_DATA_PTR_ dev_dt_spec

/* Helper which extracts config pointer from Z_DEVICE_DT_DEV_TUPLE */
#define Z_DEVICE_DT_DEV_TUPLE_CFG_PTR_(node_id, api_type, pm_dev_ptr, data_ptr,     \
	cfg_ptr) cfg_ptr

#define Z_DEVICE_DT_DEV_TUPLE_CFG_PTR(dev_dt_spec)                                  \
	Z_DEVICE_DT_DEV_TUPLE_CFG_PTR_ dev_dt_spec

/*
 * Helper which defines an API device and its dependencies, places it in the correct
 * section, and sets it up for initialization on boot, using information from the
 * devicetree.
 */
#define Z_DEVICE_DT_NEW_DEFINE(node_id, init_fn, pm_dev_ptr, data_ptr, cfg_ptr,     \
			init_level, init_prio, api_ptr, api_type, state_ptr)                    \
	const Z_DECL_ALIGN(struct device) Z_DEVICE_DT_API_NAME(node_id, api_type)       \
		Z_DEVICE_API_PLACE_IN_SECTION(api_type) = {                                 \
		.name = Z_DEVICE_DT_API_FULL_NAME(node_id, api_type),                       \
		.config = (cfg_ptr),                                                        \
		.api = api_ptr,                                                             \
		.state = (state_ptr),                                                       \
		.data = (data_ptr),                                                         \
		COND_CODE_1(CONFIG_PM_DEVICE, (.pm = pm_dev_ptr,), ())                      \
	};                                                                              \
	                                                                                \
	Z_INIT_ENTRY_DEFINE(Z_DEVICE_DT_API_NAME(node_id, api_type), init_fn,           \
		&(Z_DEVICE_DT_API_NAME(node_id, api_type)), init_level, init_prio);

/*
 * Helper which creates a device object which shares all members excluding the name
 * and API pointer.
 */
#define Z_DEVICE_DT_API_ASSIGN(api_spec, dev_dt_spec)                               \
	const Z_DECL_ALIGN(struct device)                                               \
		Z_DEVICE_DT_API_NAME(Z_DEVICE_DT_DEV_TUPLE_NODE_ID(dev_dt_spec),            \
			Z_DEVICE_API_API_TYPE(api_spec))                                        \
		Z_DEVICE_API_PLACE_IN_SECTION(Z_DEVICE_API_API_TYPE(api_spec)) = {          \
		.name = Z_DEVICE_DT_API_FULL_NAME(                                          \
			Z_DEVICE_DT_DEV_TUPLE_NODE_ID(dev_dt_spec),                             \
			Z_DEVICE_API_API_TYPE(api_spec)),                                       \
		.config = Z_DEVICE_DT_DEV_TUPLE_CFG_PTR(dev_dt_spec),                       \
		.api = Z_DEVICE_API_API_PTR(api_spec),                                      \
		.state = &Z_DEVICE_NEW_STATE_NAME(Z_DEVICE_DT_NAME_FROM_NODE(               \
			Z_DEVICE_DT_DEV_TUPLE_NODE_ID(dev_dt_spec))),                           \
		.data = Z_DEVICE_DT_DEV_TUPLE_DATA_PTR(dev_dt_spec),                        \
		COND_CODE_1(CONFIG_PM_DEVICE,                                               \
			(.pm = Z_DEVICE_DT_DEV_TUPLE_PM_DEV_PTR(dev_dt_spec)), ())              \
	}

/* Helper which assigns every api_spec passed in VA_ARGS to dev_dt_spec */
#define Z_DEVICE_DT_ASSIGN_ADDITIONAL_APIS(dev_dt_spec, ...)                        \
	COND_CODE_0(NUM_VA_ARGS_LESS_1(LIST_DROP_EMPTY(__VA_ARGS__, _)),                \
		(),                                                                         \
		(FOR_EACH_FIXED_ARG(Z_DEVICE_DT_API_ASSIGN, (;), dev_dt_spec,               \
			LIST_DROP_EMPTY(__VA_ARGS__))))

#endif /* CONFIG_LEGACY_DEVICE_MODEL */

#ifdef CONFIG_HAS_DTS
/*
 * Declare a device for each status "okay" devicetree node. (Disabled
 * nodes should not result in devices, so not predeclaring these keeps
 * drivers honest.)
 *
 * This is only "maybe" a device because some nodes have status "okay",
 * but don't have a corresponding struct device allocated. There's no way
 * to figure that out until after we've built the zephyr image,
 * though.
 */
#define Z_MAYBE_DEVICE_DECLARE_INTERNAL(node_id) \
	extern const struct device DEVICE_DT_NAME_GET(node_id);

DT_FOREACH_STATUS_OKAY_NODE(Z_MAYBE_DEVICE_DECLARE_INTERNAL)
#endif /* CONFIG_HAS_DTS */

#ifdef __cplusplus
}
#endif


#include <syscalls/device.h>

#endif /* ZEPHYR_INCLUDE_DEVICE_H_ */
