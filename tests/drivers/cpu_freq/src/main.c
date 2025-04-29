#include <zephyr/ztest.h>
#include <zephyr/drivers/cpu_freq.h>

static const struct device *cpu_freq_dev;

ZTEST(cpu_freq, test_freq_get_cfg_id_invalid)
{
	unsigned int cfg_id;
	int status;

	/* Check return value on invalid CPU ID */

	status = cpu_freq_get_cfg_id(cpu_freq_dev, CONFIG_MP_MAX_NUM_CPUS,
				     &cfg_id);

	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for invalid CPU ID, got %d", status);

	/* Check return value on NULL cfg_id pointer */

	status = cpu_freq_get_cfg_id(cpu_freq_dev, 0, NULL);

	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for NULL cfg_id pointer, got %d", status);
}

ZTEST(cpu_freq, test_set_by_cfg_id_invalid)
{
	int status;

	/* Check return value on invalid CPU bitmask (too high) */

	status = cpu_freq_set_by_cfg_id(cpu_freq_dev,
					BIT(CONFIG_MP_MAX_NUM_CPUS), 0);
	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for invalid CPU bitmask, got %d", status);

	/* Check return value when no CPUs are selected */

	status = cpu_freq_set_by_cfg_id(cpu_freq_dev, 0, 0);
	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for no CPUs selected, got %d", status);

	/* Check return value on invalid invalid cfg ID */

	status = cpu_freq_set_by_cfg_id(cpu_freq_dev, 1, cpu_freq_num_cfgs());
	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for invalid cfg ID, got %d", status);
}

ZTEST(cpu_freq, test_set_by_load_invalid)
{
	int status;

	/* app.overlay specifies 3 thresholds: 75, 50, and 25 percent */

	/* Check return value on invalid CPU bitmask (too high) */

	status = cpu_freq_set_by_load(cpu_freq_dev,
				      BIT(CONFIG_MP_MAX_NUM_CPUS), 75);
	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for invalid CPU bitmask, got %d", status);

	/* Check return value when no CPUs are selected */

	status = cpu_freq_set_by_load(cpu_freq_dev, 0, 75);
	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for no CPUs selected, got %d", status);

	/* Check return value on invalid load percentage (too low) */

	status = cpu_freq_set_by_load(cpu_freq_dev, BIT(0), 0);
	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for invalid load percentage, got %d", status);

	/* Check return value on invalid load percentage (too high) */
	status = cpu_freq_set_by_load(cpu_freq_dev, BIT(0), 101);
	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for invalid load percentage, got %d", status);
}

ZTEST(cpu_freq, test_get_cfg_invalid)
{
	int status;

	/* Check return value on invalid config ID */
	status = cpu_freq_get_cfg(cpu_freq_dev, cpu_freq_num_cfgs(), NULL);
	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for invalid config ID, got %d", status);

	/* Check return value on NULL config pointer */
	status = cpu_freq_get_cfg(cpu_freq_dev, 0, NULL);
	zassert_equal(status, -EINVAL,
		      "Expected -EINVAL for NULL config pointer, got %d", status);
}

ZTEST(cpu_freq, test_num_cfgs)
{
	unsigned int num_cfgs;

	/* The stub driver has 3 configurations */

	num_cfgs = cpu_freq_num_cfgs();
	zassert_equal(num_cfgs, 3,
		      "Expected 3 CPU frequency configurations, got %u", num_cfgs);
}

ZTEST(cpu_freq, test_config_data_validate)
{
	struct cpu_freq_cfg cfg;
	int status;

	/* Verify that the configuration data was initialized correctly */

	status = cpu_freq_get_cfg(cpu_freq_dev, 0, &cfg);
	zassert_equal(status, 0,
		      "Expected success for valid config ID 0, got %d", status);
	zassert_equal(cfg.threshold, 75,
		      "Expected threshold 75 for config ID 0, got %llu", cfg.threshold);
	zassert_equal(cfg.frequency, 1000000000,
		      "Expected frequency 1000000000 for config ID 0, got %llu", cfg.frequency);

	status = cpu_freq_get_cfg(cpu_freq_dev, 1, &cfg);
	zassert_equal(status, 0,
		      "Expected success for valid config ID 0, got %d", status);
	zassert_equal(cfg.threshold, 50,
		      "Expected threshold 50 for config ID 1, got %llu", cfg.threshold);
	zassert_equal(cfg.frequency, 800000000,
		      "Expected frequency 800000000 for config ID 1, got %llu", cfg.frequency);

	status = cpu_freq_get_cfg(cpu_freq_dev, 2, &cfg);
	zassert_equal(status, 0,
		      "Expected success for valid config ID 0, got %d", status);
	zassert_equal(cfg.threshold, 25,
		      "Expected threshold 25 for config ID 2, got %llu", cfg.threshold);
	zassert_equal(cfg.frequency, 300000000,
		      "Expected frequency 300000000 for config ID 2, got %llu", cfg.frequency);
}

ZTEST(cpu_freq, test_set_by_cfg_id_validate)
{
	int status;

	/* Set all CPUs to use config ID 2 */

	status = cpu_freq_set_by_cfg_id(cpu_freq_dev,
					BIT(CONFIG_MP_MAX_NUM_CPUS) - 1, 2);
	zassert_equal(status, 0,
		      "Expected success for setting all CPUs to config ID 2, got %d", status);

	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		unsigned int cfg_id;

		status = cpu_freq_get_cfg_id(cpu_freq_dev, i, &cfg_id);
		zassert_equal(status, 0,
			      "Expected success for getting config ID for CPU%u, got %d",
			      i, status);
		zassert_equal(cfg_id, 2,
			      "Expected config ID 2 for CPU%u, got %u", i, cfg_id);
	}
}

ZTEST(cpu_freq, test_set_by_load_validate)
{
	int status;

	/* Set all CPUs to use config ID 1 for 50% load */

	status = cpu_freq_set_by_load(cpu_freq_dev,
				      BIT(CONFIG_MP_MAX_NUM_CPUS) - 1, 50);
	zassert_equal(status, 0,
		      "Expected success for setting all CPUs to 50%% load, got %d", status);

	for (unsigned int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		unsigned int cfg_id;

		status = cpu_freq_get_cfg_id(cpu_freq_dev, i, &cfg_id);
		zassert_equal(status, 0,
			      "Expected success for getting config ID for CPU%u, got %d",
			      i, status);
		zassert_equal(cfg_id, 1,
			      "Expected config ID 1 for CPU%u, got %u", i, cfg_id);
	}
}

static void *setup_fn(void)
{
	cpu_freq_dev = DEVICE_DT_GET(DT_NODELABEL(cpu_freq));

	return NULL;
}

ZTEST_SUITE(cpu_freq, NULL, setup_fn, NULL, NULL, NULL);
