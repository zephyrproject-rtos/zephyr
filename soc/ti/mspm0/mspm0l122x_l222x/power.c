#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

static void set_mode_run(uint8_t state)
{
	switch (state) {
	case DL_SYSCTL_POWER_POLICY_RUN_SLEEP0:
		DL_SYSCTL_setPowerPolicyRUN0SLEEP0();
		break;
	case DL_SYSCTL_POWER_POLICY_RUN_SLEEP1:
		DL_SYSCTL_setPowerPolicyRUN1SLEEP1();
		break;
	case DL_SYSCTL_POWER_POLICY_RUN_SLEEP2:
		DL_SYSCTL_setPowerPolicyRUN2SLEEP2();
		break;
	}
}

static void set_mode_stop(uint8_t state)
{
	switch (state) {
	case DL_SYSCTL_POWER_POLICY_STOP0:
		DL_SYSCTL_setPowerPolicySTOP0();
		break;
	case DL_SYSCTL_POWER_POLICY_STOP1:
		DL_SYSCTL_setPowerPolicySTOP1();
		break;
	case DL_SYSCTL_POWER_POLICY_STOP2:
		DL_SYSCTL_setPowerPolicySTOP2();
		break;
	}
}

static void set_mode_standby(uint8_t state)
{
	switch (state) {
	case DL_SYSCTL_POWER_POLICY_STANDBY0:
		DL_SYSCTL_setPowerPolicySTANDBY0();
		break;
	case DL_SYSCTL_POWER_POLICY_STANDBY1:
		DL_SYSCTL_setPowerPolicySTANDBY1();
		break;
	}
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_ACTIVE:
		set_mode_run(substate_id);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop(substate_id);
		break;
	case PM_STATE_STANDBY:
		set_mode_standby(substate_id);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}

	__WFI();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	irq_unlock(0);
}
