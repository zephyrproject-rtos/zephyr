#include <string.h>
#include <device.h>
#include <misc/util.h>

extern struct device __device_init_start[];
extern struct device __device_PRIMARY_start[];
extern struct device __device_SECONDARY_start[];
extern struct device __device_NANOKERNEL_start[];
extern struct device __device_MICROKERNEL_start[];
extern struct device __device_APPLICATION_start[];
extern struct device __device_init_end[];

static struct device *config_levels[] = {
	__device_PRIMARY_start,
	__device_SECONDARY_start,
	__device_NANOKERNEL_start,
	__device_MICROKERNEL_start,
	__device_APPLICATION_start,
	__device_init_end,
};

/**
 * @brief Execute all the device initialization functions at a given level
 *
 * @details Invokes the initialization routine for each device object
 * created by the SYS_DEFINE_DEVICE() macro using the specified level.
 * The linker script places the device objects in memory in the order
 * they need to be invoked, with symbols indicating where one level leaves
 * off and the next one begins.
 *
 * @param level init level to run.
 */
void _sys_device_do_config_level(int level)
{
	struct device *info;

	for (info = config_levels[level]; info < config_levels[level+1]; info++) {
		struct device_config *device = info->config;

		device->init(info);
	}
}

/**
 * @brief Retrieve the device structure for a driver by name
 *
 * @details Device objects are created via the SYS_DEFINE_DEVICE() macro and
 * placed in memory by the linker. If a driver needs to bind to another driver
 * it can use this function to retrieve the device structure of the lower level
 * driver by the name the driver exposes to the system.
 *
 * @param name device name to search for.
 *
 * @return pointer to device structure, or NULL if not found.
 */
struct device *device_get_binding(char *name)
{
	struct device *info;

	for (info = __device_init_start; info != __device_init_end; info++) {
		if (!strcmp(name, info->config->name)) {
			return info;
		}
	}

	return NULL;
}
