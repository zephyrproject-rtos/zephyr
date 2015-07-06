#include <string.h>
#include <device.h>
#include <misc/util.h>

extern struct device __initconfig_start[];
extern struct device __initconfig0_start[];
extern struct device __initconfig1_start[];
extern struct device __initconfig2_start[];
extern struct device __initconfig3_start[];
extern struct device __initconfig4_start[];
extern struct device __initconfig5_start[];
extern struct device __initconfig6_start[];
extern struct device __initconfig7_start[];
extern struct device __initconfig_end[];

static struct device *config_levels[] = {
	__initconfig0_start,
	__initconfig1_start,
	__initconfig2_start,
	__initconfig3_start,
	__initconfig4_start,
	__initconfig5_start,
	__initconfig6_start,
	__initconfig7_start,
	__initconfig_end,
};

/**
 * @brief Execute all the driver init functions at a given level
 *
 * @details Driver init objects are created with the
 * __define_initconfig() macro and are placed in RAM by the linker
 * script. The {nano|micro}kernel code will execute the init level at
 * the appropriate time.
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
