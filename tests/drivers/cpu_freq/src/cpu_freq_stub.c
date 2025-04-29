#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/cpu_freq.h>

static const unsigned long long raw_data[CPU_FREQ_CFG_NUM_RAW_ELEMENTS] = {
	LISTIFY(CPU_FREQ_CFG_NUM_RAW_ELEMENTS, CPU_FREQ_CFG_RAW_DATA, \
		(,), CPU_FREQ_NODE)
};
static const struct cpu_freq_cfg *cfg_data = (const struct cpu_freq_cfg *)
					     raw_data;

/* Configuration IDs in use by each CPU */
static unsigned int cpu_freq_cfg_id[CONFIG_MP_MAX_NUM_CPUS] = {0};

static int stub_cpu_freq_init(const struct device *dev)
{
	/* Nothing to do during initialization */
	return 0;
}

static int stub_cpu_freq_get_cfg_id(const struct device *dev,
				    unsigned int cpu_id, unsigned int *cfg_id)
{
	*cfg_id = cpu_freq_cfg_id[cpu_id];

	return 0;
}

static int stub_cpu_freq_set_by_cfg_id(const struct device *dev,
				       unsigned int cpu_bitmask,
				       unsigned int cfg_id)
{
	unsigned int i;

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if ((cpu_bitmask & BIT(i)) != 0) {
			cpu_freq_cfg_id[i] = cfg_id;
		}
	}

	return 0;
}

static int stub_cpu_freq_set_by_load(const struct device *dev,
				     unsigned int cpu_bitmask,
				     unsigned int load)
{
	unsigned int i;
	unsigned int cfg_id;

	for (i = 0; i < CPU_FREQ_CFG_NUM_ENTRIES; i++) {
		if (load >= cfg_data[i].threshold) {
			cfg_id = i;
			break;
		}
	}

	if (i >= CPU_FREQ_CFG_NUM_ENTRIES) {
		return -EINVAL;   /* No suitable load found */
	}

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if ((cpu_bitmask & BIT(i)) != 0) {
			cpu_freq_cfg_id[i] = cfg_id;
		}
	}

	return 0;
}

static int stub_cpu_freq_get_cfg(const struct device *dev,
				 unsigned int cfg_id,
				 struct cpu_freq_cfg *cfg)
{
	cfg->threshold = cfg_data[cfg_id].threshold;
	cfg->frequency = cfg_data[cfg_id].frequency;

	return 0;
}

static DEVICE_API(cpu_freq, stub_cpu_freq_api) = {
	.get_cfg_id = stub_cpu_freq_get_cfg_id,
	.set_by_cfg_id = stub_cpu_freq_set_by_cfg_id,
	.set_by_load = stub_cpu_freq_set_by_load,
	.get_cfg = stub_cpu_freq_get_cfg,
};

DEVICE_DT_DEFINE(DT_NODELABEL(cpu_freq), stub_cpu_freq_init,
		 NULL, NULL, NULL, POST_KERNEL,
		 CONFIG_CPU_FREQ_INIT_PRIORITY, &stub_cpu_freq_api);
