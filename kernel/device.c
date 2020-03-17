/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <device.h>
#include <sys/atomic.h>
#include <syscall_handler.h>

extern const struct init_entry __init_start[];
extern const struct init_entry __init_PRE_KERNEL_1_start[];
extern const struct init_entry __init_PRE_KERNEL_2_start[];
extern const struct init_entry __init_POST_KERNEL_start[];
extern const struct init_entry __init_APPLICATION_start[];
extern const struct init_entry __init_end[];

#ifdef CONFIG_SMP
extern const struct init_entry __init_SMP_start[];
#endif

extern struct device __device_start[];
extern struct device __device_end[];

extern struct device_context __device_context_start[];

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
extern u32_t __device_busy_start[];
extern u32_t __device_busy_end[];
#define DEVICE_BUSY_SIZE (__device_busy_end - __device_busy_start)
#endif

/**
 * @brief Execute all the init entry initialization functions at a given level
 *
 * @details Invokes the initialization routine for each init entry object
 * created by the INIT_ENTRY_DEFINE() macro using the specified level.
 * The linker script places the init entry objects in memory in the order
 * they need to be invoked, with symbols indicating where one level leaves
 * off and the next one begins.
 *
 * @param level init level to run.
 */
void z_sys_init_run_level(s32_t level)
{
	static const struct init_entry *levels[] = {
		__init_PRE_KERNEL_1_start,
		__init_PRE_KERNEL_2_start,
		__init_POST_KERNEL_start,
		__init_APPLICATION_start,
#ifdef CONFIG_SMP
		__init_SMP_start,
#endif
		/* End marker */
		__init_end,
	};
	const struct init_entry *entry;

	for (entry = levels[level]; entry < levels[level+1]; entry++) {
		struct device *dev = entry->dev;
		int retval;

		if (dev != NULL) {
			z_object_init(dev);
		}

		retval = entry->init(dev);
		if (retval != 0) {
			if (dev) {
				/* Initialization failed. Clear the API struct
				 * so that device_get_binding() will not succeed
				 * for it.
				 */
				dev->driver_api = NULL;
			}
		}
	}
}

struct device *z_impl_device_get_binding(const char *name)
{
	struct device *dev;

	/* Split the search into two loops: in the common scenario, where
	 * device names are stored in ROM (and are referenced by the user
	 * with CONFIG_* macros), only cheap pointer comparisons will be
	 * performed. Reserve string comparisons for a fallback.
	 */
	for (dev = __device_start; dev != __device_end; dev++) {
		if ((dev->driver_api != NULL) &&
		    (dev->name == name)) {
			return dev;
		}
	}

	for (dev = __device_start; dev != __device_end; dev++) {
		if ((dev->driver_api != NULL) &&
		    (strcmp(name, dev->name) == 0)) {
			return dev;
		}
	}

	return NULL;
}

#ifdef CONFIG_USERSPACE
static inline struct device *z_vrfy_device_get_binding(const char *name)
{
	char name_copy[Z_DEVICE_MAX_NAME_LEN];

	if (z_user_string_copy(name_copy, (char *)name, sizeof(name_copy))
	    != 0) {
		return 0;
	}

	return z_impl_device_get_binding(name_copy);
}
#include <syscalls/device_get_binding_mrsh.c>
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
int device_pm_control_nop(struct device *unused_device,
		       u32_t unused_ctrl_command,
		       void *unused_context,
		       device_pm_cb cb,
		       void *unused_arg)
{
	return 0;
}

void device_list_get(struct device **device_list, int *device_count)
{

	*device_list = __device_start;
	*device_count = __device_end - __device_start;
}


int device_any_busy_check(void)
{
	int i = 0;

	for (i = 0; i < DEVICE_BUSY_SIZE; i++) {
		if (__device_busy_start[i] != 0U) {
			return -EBUSY;
		}
	}
	return 0;
}

int device_busy_check(struct device *chk_dev)
{
	if (atomic_test_bit((const atomic_t *)__device_busy_start,
			    (chk_dev - __device_start))) {
		return -EBUSY;
	}
	return 0;
}

#endif

void device_busy_set(struct device *busy_dev)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	atomic_set_bit((atomic_t *) __device_busy_start,
		       (busy_dev - __device_start));
#else
	ARG_UNUSED(busy_dev);
#endif
}

void device_busy_clear(struct device *busy_dev)
{
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	atomic_clear_bit((atomic_t *) __device_busy_start,
			 (busy_dev - __device_start));
#else
	ARG_UNUSED(busy_dev);
#endif
}

#if defined(CONFIG_DEVICE_STATUS_REPORT)
static void device_call_status(struct device *dev, bool init, int status)
{
	struct device_context *dc =
		(struct device_context *)__device_context_start +
		(dev - __device_start);

	if (status) {
		if (init) {
			dc->status.init = false;
		} else {
			dc->status.call = false;
		}

		if (status == -EIO) {
			dc->status.hw_fault = true;
		}
	} else {
		if (init) {
			dc->status.init = true;
		} else {
			dc->status.call = true;
		}
	}

	DEVICE_STATUS_SET_STATUS_CODE(dc, status);
}

static int device_check_status(struct device *dev)
{
	struct device_context *dc =
		(struct device_context *)__device_context_start +
		(dev - __device_start);

	if (!dc->status.init ||
	    (IS_ENABLED(CONFIG_DEVICE_STRICT_CHECK) &&
	     (dc->status.hw_fault))) {
		return DEVICE_STATUS_GET_STATUS_CODE(dc);
	}

	return 0;
}

#else /* CONFIG_DEVICE_STATUS_REPORT */

static void device_call_status(struct device *dev, bool init, int status)
{
	struct device_context *dc =
		(struct device_context *)__device_context_start +
		(dev - __device_start);

	if (init) {
		if (status) {
			dc->status.init = false;
		} else {
			dc->status.init = true;
		}
	}

	DEVICE_STATUS_SET_STATUS_CODE(dc, status);
}

static int device_check_status(struct device *dev)
{
	struct device_context *dc =
		(struct device_context *)__device_context_start +
		(dev - __device_start);

	return dc->status.init ? 0 : -EIO;
}
#endif /* CONFIG_DEVICE_STATUS_REPORT */

int device_init_done(struct device *dev, int status)
{
	device_call_status(dev, true, status);

	return status;
}

#ifdef CONFIG_DEVICE_CALL_TIMEOUT
static void device_call_set_timeout(struct device *dev, k_timeout_t timeout)
{
	struct device_context *dc =
		(struct device_context *)__device_context_start +
		(dev - __device_start);

	dc->timeout = timeout;
}

static void device_call_cancel(struct device *dev)
{
	if (dev->cancel != NULL) {
		dev->cancel(dev);
	}

	device_call_status(dev, false, -ECANCELED);
}
#else
#define device_call_set_timeout(...)
#define device_call_cancel(...)
#endif

int device_lock_timeout(struct device *dev, k_timeout_t timeout)
{
	int status;
#if defined(CONFIG_DEVICE_CONCURRENT_ACCESS)
	struct device_context *dc =
		(struct device_context *)__device_context_start +
		(dev - __device_start);
#endif

#ifdef CONFIG_DEVICE_CONCURRENT_ACCESS
	k_timeout_t lock_timeout = timeout;

#ifdef CONFIG_DEVICE_CALL_TIMEOUT
#ifdef CONFIG_DEVICE_NO_LOCK_TIMEOUT
	lock_timeout = K_FOREVER;
#else
	u32_t time_spent;

	if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		time_spent = k_uptime_get_32();
	}
#endif
#endif /* CONFIG_DEVICE_CALL_TIMEOUT */

	if (k_sem_take(&dc->lock, lock_timeout) != 0) {
		return -EAGAIN;
	}

#if defined(CONFIG_DEVICE_CALL_TIMEOUT) &&	\
	!defined(CONFIG_DEVICE_NO_LOCK_TIMEOUT)

	if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		if (k_uptime_get_32() < time_spent) {
			time_spent = k_uptime_get_32() +
				(UINT32_MAX - time_spent);
		} else {
			time_spent = k_uptime_get_32() + time_spent;
		}

		timeout = K_MSEC(k_ticks_to_ms_floor32(tiemout) - time_spent);
	}
#endif /* CONFIG_DEVICE_CALL_TIMEOUT */

#endif /* CONFIG_DEVICE_CONCURRENT_ACCESS */

	device_call_set_timeout(dev, timeout);

	status = device_check_status(dev);
#ifdef CONFIG_DEVICE_CONCURRENT_ACCESS
	if (status != 0) {
		k_sem_give(&dc->lock);
	}
#endif
	return status;
}

void device_call_complete(struct device *dev, int status)
{
	struct device_context *dc =
		(struct device_context *)__device_context_start +
		(dev - __device_start);

	(void)device_call_status(dev, false, status);

	k_sem_give(&dc->sync);
}

int device_release(struct device *dev)
{
	struct device_context *dc =
		(struct device_context *)__device_context_start +
		(dev - __device_start);
	int status;

#ifdef CONFIG_DEVICE_CALL_TIMEOUT
	if (k_sem_take(&dc->sync, dc->timeout) == -EAGAIN) {
		device_call_cancel(dev);
	}
#else
	k_sem_take(&dc->sync, K_FOREVER);
#endif /* CONFIG_DEVICE_CALL_TIMEOUT */

	status = DEVICE_STATUS_GET_STATUS_CODE(dc);

#ifdef CONFIG_DEVICE_CONCURRENT_ACCESS
	k_sem_give(&dc->lock);
#endif
	return status;
}
