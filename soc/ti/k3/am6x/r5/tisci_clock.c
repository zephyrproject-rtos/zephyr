#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/tisci_clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc_clock, LOG_LEVEL_DBG);

#define CONSOLE_NODE DT_CHOSEN(zephyr_console)

int soc_clock_set_freq(const struct device *dev, clock_control_subsys_rate_t subsys,
		       uint64_t frequency)
{
	int err = clock_control_set_rate(dev, subsys, &frequency);

	if (err < 0) {
		LOG_ERR("Failed to set clock rate for %s\n", dev->name);
		return 1;
	}

	return 0;
}

#define CHECK_NODE_CLOCK_COND(node)                                                                \
	UTIL_AND(DT_NODE_HAS_PROP(node, clocks), DT_NODE_HAS_PROP(node, clock_frequency))

#define NODE_ENABLE_CLOCK(node)                                                                    \
	{                                                                                          \
		const struct device *dev = TISCI_GET_CLOCK(node);                                  \
		struct tisci_clock_config config = TISCI_GET_CLOCK_DETAILS(node);                  \
		uint64_t freq = DT_PROP(node, clock_frequency);                                    \
		LOG_INF("Setting frequency for " DT_NODE_FULL_NAME(node) ": %lluHz", freq);        \
		error_count += soc_clock_set_freq(dev, &config, freq);                             \
	}

#if CHECK_NODE_CLOCK_COND(CONSOLE_NODE)
int soc_chosen_console_clock_init(void)
{
	int error_count = 0;

	NODE_ENABLE_CLOCK(CONSOLE_NODE);
	return error_count;
}

SYS_INIT(soc_chosen_console_clock_init, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY);
#endif /* CHECK_NODE_CLOCK_COND(zephyr_console) */

int soc_clock_init(void)
{
	int error_count = 0;

#define CHECK_NODE_CLOCK(node)                                                                     \
	COND_CODE_1(CHECK_NODE_CLOCK_COND(node), (NODE_ENABLE_CLOCK(node)), (/* Do nothing */))

	DT_FOREACH_STATUS_OKAY_NODE(CHECK_NODE_CLOCK);

	return error_count;
}
SYS_INIT(soc_clock_init, POST_KERNEL, 1);
