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
 * @details Driver config object are created via the
 * DECLARE_DEVICE_INIT_CONFIG() macro and placed in ROM by the
 * linker. If a driver needs to bind to another driver it will use
 * this function to retrieve the device structure of the lower level
 * driver by the name the driver exposes to the system.
 *
 * @param name driver name to search for.
 */
struct device* device_get_binding(char *name)
{
	struct device *info;
	struct device *found = 0;
	int level;

	for (level = 0; level < ARRAY_SIZE(config_levels) - 1; level++) {
		for (info = config_levels[level];
		     info < config_levels[level+1]; info++) {
			struct device_config *device = info->config;
			if (!strcmp(name, device->name)) {
				return info;
			}
		}
	}
	return found;
}
