#include "zephyr/pm/device_runtime.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc_power, LOG_LEVEL_DBG);

#define CONSOLE_NODE DT_CHOSEN(zephyr_console)

#define CHECK_NODE_POWER_COND(node) DT_NODE_HAS_STATUS_OKAY(DT_PHANDLE(node, power_domains))
#define DEVICE_DT_GET_PD(node)      DEVICE_DT_GET(DT_PHANDLE(node, power_domains))

int soc_power_domain_turn_on(const struct device *dev)
{
	int err = pm_device_runtime_get(dev);

	LOG_INF("Turning on power domain: %s", dev->name);
	if (err < 0) {
		LOG_INF("Failed to get power domain: %s", dev->name);
		return 1;
	}

	return 0;
}

#if CHECK_NODE_POWER_COND(CONSOLE_NODE)
#define CONSOLE_POWER_ON_PRIORITY 45
BUILD_ASSERT(CONSOLE_POWER_ON_PRIORITY < CONFIG_SERIAL_INIT_PRIORITY);
int soc_chosen_console_power_init(void)
{
	return soc_power_domain_turn_on(DEVICE_DT_GET_PD(CONSOLE_NODE));
}
SYS_INIT(soc_chosen_console_power_init, PRE_KERNEL_1, CONSOLE_POWER_ON_PRIORITY);
#endif /* CHECK_NODE_POWER_COND(CONSOLE_NODE) */

int soc_power_domain_init(void)
{
	int error_count = 0;

#define CHECK_NODE_POWER_DOMAIN(node)                                                              \
	COND_CODE_1(CHECK_NODE_POWER_COND(node), (                                                 \
		error_count += soc_power_domain_turn_on(DEVICE_DT_GET_PD(node));                   \
	), (/* Do nothing */))

	DT_FOREACH_STATUS_OKAY_NODE(CHECK_NODE_POWER_DOMAIN);

	return error_count;
}
SYS_INIT(soc_power_domain_init, POST_KERNEL, 0);
