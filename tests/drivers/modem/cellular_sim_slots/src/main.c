/*
 * Copyright (c) 2026 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SIM slot selection of the modem_cellular driver. The modem instances sit on
 * emulated UARTs that never answer, so the driver stays in its power-up states
 * until it is suspended. The SIM switch is a gpio-mux on an emulated GPIO,
 * whose output tells which slot the modem is routed to.
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/pm/device.h>

#define SIM_MUX_GPIO_NODE DT_GPIO_CTLR(DT_NODELABEL(sim_mux), mux_gpios)
#define SIM_MUX_GPIO_PIN  DT_GPIO_PIN(DT_NODELABEL(sim_mux), mux_gpios)

static const struct device *const modem = DEVICE_DT_GET(DT_NODELABEL(modem));
static const struct device *const single_sim_modem = DEVICE_DT_GET(DT_NODELABEL(single_sim_modem));
static const struct device *const sim_mux_gpio = DEVICE_DT_GET(SIM_MUX_GPIO_NODE);

/* SIM switch output and active slot right after boot, before any test runs */
static int boot_mux_output;
static uint8_t boot_active;

static enum modem_cellular_state modem_state(void)
{
	const struct modem_cellular_data *data = modem->data;

	return data->state;
}

static int sim_mux_output(void)
{
	return gpio_emul_output_get(sim_mux_gpio, SIM_MUX_GPIO_PIN);
}

static void modem_suspend(void)
{
	int ret = pm_device_action_run(modem, PM_DEVICE_ACTION_SUSPEND);

	zassert_true((ret == 0) || (ret == -EALREADY), "Failed to suspend the modem (%d)", ret);
	zassert_equal(modem_state(), MODEM_CELLULAR_STATE_IDLE, "Modem not idle once suspended");
}

static void modem_resume(void)
{
	int ret = pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME);

	zassert_true((ret == 0) || (ret == -EALREADY), "Failed to resume the modem (%d)", ret);

	/* The resume is handled by the driver's state machine on the system work queue */
	zassert_true(WAIT_FOR(modem_state() != MODEM_CELLULAR_STATE_IDLE, 1000000, k_msleep(1)),
		     "Modem did not leave the idle state on resume");
}

static uint8_t active_sim_slot(void)
{
	uint8_t count;
	uint8_t active;

	zassert_ok(cellular_get_sim_slots(modem, &count, &active));
	zassert_equal(count, 2, "Unexpected SIM slot count %u", count);

	return active;
}

static void *test_setup(void)
{
	uint8_t count;

	zassert_true(device_is_ready(modem), "Modem not ready");
	zassert_true(device_is_ready(single_sim_modem), "Single SIM modem not ready");

	/* Let the state machine handle the resume issued at init */
	zassert_true(WAIT_FOR(modem_state() != MODEM_CELLULAR_STATE_IDLE, 1000000, k_msleep(1)),
		     "Modem did not start");

	boot_mux_output = sim_mux_output();
	zassert_ok(cellular_get_sim_slots(modem, &count, &boot_active));

	return NULL;
}

/* Every test starts with the modem running on the default SIM slot, 1 */
static void test_before(void *fixture)
{
	int ret;

	ARG_UNUSED(fixture);

	modem_suspend();

	ret = cellular_select_sim(modem, 1);
	zassert_true((ret == 0) || (ret == -EALREADY), "Failed to select SIM slot 1 (%d)", ret);

	modem_resume();
}

ZTEST(cellular_sim_slots, test_default_slot_selected_at_boot)
{
	zassert_equal(boot_active, 1, "default-sim slot not active at boot");
	zassert_equal(boot_mux_output, 1, "SIM switch not routed to the default-sim slot");
}

ZTEST(cellular_sim_slots, test_select_rejected_while_running)
{
	zassert_equal(cellular_select_sim(modem, 0), -EBUSY,
		      "SIM slot switched while the modem is running");
	zassert_equal(active_sim_slot(), 1, "Active SIM slot changed");
	zassert_equal(sim_mux_output(), 1, "SIM switch moved while the modem is running");
}

ZTEST(cellular_sim_slots, test_select_while_suspended)
{
	struct modem_cellular_data *data = modem->data;
	char iccid[MODEM_CELLULAR_DATA_ICCID_LEN];

	modem_suspend();

	/* SIM identity as read from slot 1 */
	strcpy(data->iccid, "8955000000000000001");
	strcpy(data->imsi, "724000000000001");

	zassert_ok(cellular_select_sim(modem, 0));
	zassert_equal(active_sim_slot(), 0, "SIM slot 0 not active");
	zassert_equal(sim_mux_output(), 0, "SIM switch not routed to slot 0");

	zassert_ok(cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_SIM_ICCID, iccid,
					   sizeof(iccid)));
	zassert_str_equal(iccid, "", "ICCID of the previous SIM slot kept");
	zassert_str_equal(data->imsi, "", "IMSI of the previous SIM slot kept");
}

ZTEST(cellular_sim_slots, test_selected_slot_applied_on_resume)
{
	modem_suspend();
	zassert_ok(cellular_select_sim(modem, 0));

	/* Move the switch behind the driver's back, as a MUX that resets its state would */
	zassert_ok(gpio_pin_set_raw(sim_mux_gpio, SIM_MUX_GPIO_PIN, 1));

	modem_resume();

	zassert_equal(sim_mux_output(), 0, "SIM switch not routed to slot 0 on resume");
	zassert_equal(active_sim_slot(), 0, "SIM slot 0 not kept across resume");
}

ZTEST(cellular_sim_slots, test_select_invalid_and_active_slot)
{
	zassert_equal(cellular_select_sim(modem, 2), -EINVAL, "Unknown SIM slot accepted");
	zassert_equal(cellular_select_sim(modem, 1), -EALREADY, "Active SIM slot selected again");

	modem_suspend();

	zassert_equal(cellular_select_sim(modem, 2), -EINVAL, "Unknown SIM slot accepted");
	zassert_equal(cellular_select_sim(modem, 1), -EALREADY, "Active SIM slot selected again");
	zassert_equal(sim_mux_output(), 1, "SIM switch moved");
}

ZTEST(cellular_sim_slots, test_modem_without_sim_slots)
{
	uint8_t count;
	uint8_t active;

	zassert_ok(cellular_get_sim_slots(single_sim_modem, &count, &active));
	zassert_equal(count, 1, "Unexpected SIM slot count %u", count);
	zassert_equal(active, 0, "Unexpected active SIM slot %u", active);
	zassert_equal(cellular_select_sim(single_sim_modem, 0), -ENOTSUP,
		      "SIM slot selected on a modem without sim-slots");
}

#if defined(CONFIG_MODEM_CELLULAR_SHELL)
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

static int shell_run(const char *cmd, const char **output)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	size_t size;
	int ret;

	shell_backend_dummy_clear_output(sh);
	ret = shell_execute_cmd(sh, cmd);
	*output = shell_backend_dummy_get_output(sh, &size);

	return ret;
}

ZTEST(cellular_sim_slots, test_shell_sim)
{
	char cmd[64];
	const char *output;

	snprintk(cmd, sizeof(cmd), "modem_cellular sim %s", modem->name);
	zassert_ok(shell_run(cmd, &output));
	zassert_not_null(strstr(output, "SIM slots: 2, active: 1"), "Unexpected output: %s",
			 output);

	snprintk(cmd, sizeof(cmd), "modem_cellular sim %s 0", modem->name);
	zassert_equal(shell_run(cmd, &output), -EBUSY, "SIM slot switched while running");
	zassert_not_null(strstr(output, "pm suspend"), "Unexpected output: %s", output);

	modem_suspend();

	zassert_ok(shell_run(cmd, &output));
	zassert_equal(sim_mux_output(), 0, "SIM switch not routed to slot 0");

	snprintk(cmd, sizeof(cmd), "modem_cellular sim %s 256", modem->name);
	zassert_equal(shell_run(cmd, &output), -EINVAL, "Out of range SIM slot accepted");
}
#endif /* CONFIG_MODEM_CELLULAR_SHELL */

ZTEST_SUITE(cellular_sim_slots, NULL, test_setup, test_before, NULL, NULL);
