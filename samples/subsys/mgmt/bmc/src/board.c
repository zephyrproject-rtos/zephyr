/*
 * Board glue for the BMC sample.
 *
 * This is the file a product would adapt first: it publishes the sensors the
 * board carries and, when the host cannot be driven over plain GPIOs, replaces
 * the built-in host control backend.
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/host.h>
#include <zephyr/mgmt/bmc/sensor.h>

LOG_MODULE_DECLARE(bmc_sample, LOG_LEVEL_INF);

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(bmc_die_temp))
/*
 * A sensor backed by a Zephyr sensor device only needs the devicetree node and
 * the Redfish metadata; the BMC samples it on demand.
 */
BMC_SENSOR_DT_DEFINE(sensor_die_temp, DT_ALIAS(bmc_die_temp), SENSOR_CHAN_DIE_TEMP, "TempBmc",
		     "BMC Die Temperature", "Temperature", "Cel");
#endif

/*
 * A sensor that is not a Zephyr sensor device is just as easy: supply a read
 * callback. This one reports how long the BMC has been up.
 */
static int uptime_read(const struct bmc_sensor *sensor, struct sensor_value *val)
{
	ARG_UNUSED(sensor);

	val->val1 = (int32_t)(k_uptime_get() / MSEC_PER_SEC);
	val->val2 = 0;

	return 0;
}

BMC_SENSOR_DEFINE(sensor_uptime, .id = "Uptime", .name = "BMC Uptime", .reading_type = "Duration",
		  .units = "s", .physical_context = "ManagementController", .read = uptime_read);

#if !defined(CONFIG_BMC_HOST_GPIO)
/*
 * Without host GPIOs the sample still answers power queries, so that the
 * dashboard and the Redfish resources have something to show. A real product
 * would talk to its host here instead.
 */
static bool host_powered;

static int sim_host_power_set(bool on)
{
	LOG_INF("Host power %s", on ? "on" : "off");
	host_powered = on;

	return 0;
}

static int sim_host_power_get(bool *on)
{
	*on = host_powered;

	return 0;
}

static int sim_host_reset(void)
{
	LOG_INF("Host reset");

	return 0;
}

static const struct bmc_host_ops sim_host_ops = {
	.power_set = sim_host_power_set,
	.power_get = sim_host_power_get,
	.reset = sim_host_reset,
};

static int board_host_init(void)
{
	return bmc_host_ops_register(&sim_host_ops);
}

BMC_COMPONENT_DEFINE(sample_host, BMC_INIT_PHASE_PLATFORM, board_host_init, false);
#endif /* !CONFIG_BMC_HOST_GPIO */
