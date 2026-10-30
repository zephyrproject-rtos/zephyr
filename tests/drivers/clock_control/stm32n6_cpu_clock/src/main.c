/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <soc.h>
#include <stm32_ll_rcc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <clock_control/clock_stm32_ll_common.h>

#define CPU_RATE_BOOT_HZ DT_PROP(DT_NODELABEL(cpusw), clock_frequency)

static const uint32_t cpu_rates_hz[] = {
	CPU_RATE_BOOT_HZ, 16000000U, 600000000U, 50000000U, 200000000U, CPU_RATE_BOOT_HZ,
};

struct clock_domain_snapshot {
	uint32_t pll1_source;
	uint32_t pll1_m;
	uint32_t pll1_n;
	uint32_t pll1_p1;
	uint32_t pll1_p2;
	uint32_t pll1_fracn;
	uint32_t ic2_source;
	uint32_t ic2_divider;
#if defined(STM32_IC7_ENABLED)
	uint32_t ic7_source;
	uint32_t ic7_divider;
#endif
#if defined(STM32_IC8_ENABLED)
	uint32_t ic8_source;
	uint32_t ic8_divider;
#endif
	uint32_t ahb_prescaler;
	uint32_t apb1_prescaler;
	uint32_t apb2_prescaler;
	uint32_t apb4_prescaler;
	uint32_t apb5_prescaler;
};

static struct k_timer rate_timer;
static atomic_t rate_timer_expirations;
static atomic_t rate_timer_action_started;
static atomic_t rate_timer_action_status;
K_SEM_DEFINE(rate_timer_action_done, 0, 1);

static void rate_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	atomic_inc(&rate_timer_expirations);
	if (atomic_cas(&rate_timer_action_started, 0, 1)) {
		int ret = stm32_clock_control_set_cpu_rate(200000000U);
		int restore_ret;

		if (ret == 0 && (SystemCoreClock != 200000000U || LL_RCC_IC1_GetDivider() != 12U)) {
			ret = -EIO;
		}
		restore_ret = stm32_clock_control_set_cpu_rate(CPU_RATE_BOOT_HZ);
		if (ret == 0) {
			ret = restore_ret;
		}
		if (ret == 0 && (SystemCoreClock != CPU_RATE_BOOT_HZ ||
				 LL_RCC_IC1_GetDivider() != DT_PROP(DT_NODELABEL(ic1), ic_div))) {
			ret = -EIO;
		}
		atomic_set(&rate_timer_action_status, ret);
		k_sem_give(&rate_timer_action_done);
	}
}

static struct clock_domain_snapshot get_clock_domain_snapshot(void)
{
	struct clock_domain_snapshot snapshot;

	snapshot.pll1_source = LL_RCC_PLL1_GetSource();
	snapshot.pll1_m = LL_RCC_PLL1_GetM();
	snapshot.pll1_n = LL_RCC_PLL1_GetN();
	snapshot.pll1_p1 = LL_RCC_PLL1_GetP1();
	snapshot.pll1_p2 = LL_RCC_PLL1_GetP2();
	snapshot.pll1_fracn = LL_RCC_PLL1_GetFRACN();
	snapshot.ic2_source = LL_RCC_IC2_GetSource();
	snapshot.ic2_divider = LL_RCC_IC2_GetDivider();
#if defined(STM32_IC7_ENABLED)
	snapshot.ic7_source = LL_RCC_IC7_GetSource();
	snapshot.ic7_divider = LL_RCC_IC7_GetDivider();
#endif
#if defined(STM32_IC8_ENABLED)
	snapshot.ic8_source = LL_RCC_IC8_GetSource();
	snapshot.ic8_divider = LL_RCC_IC8_GetDivider();
#endif
	snapshot.ahb_prescaler = LL_RCC_GetAHBPrescaler();
	snapshot.apb1_prescaler = LL_RCC_GetAPB1Prescaler();
	snapshot.apb2_prescaler = LL_RCC_GetAPB2Prescaler();
	snapshot.apb4_prescaler = LL_RCC_GetAPB4Prescaler();
	snapshot.apb5_prescaler = LL_RCC_GetAPB5Prescaler();

	return snapshot;
}

static void assert_clock_domain_unchanged(const struct clock_domain_snapshot *before)
{
	struct clock_domain_snapshot after = get_clock_domain_snapshot();

	zassert_equal(after.pll1_source, before->pll1_source, "PLL1 source changed");
	zassert_equal(after.pll1_m, before->pll1_m, "PLL1 M divider changed");
	zassert_equal(after.pll1_n, before->pll1_n, "PLL1 N multiplier changed");
	zassert_equal(after.pll1_p1, before->pll1_p1, "PLL1 P1 divider changed");
	zassert_equal(after.pll1_p2, before->pll1_p2, "PLL1 P2 divider changed");
	zassert_equal(after.pll1_fracn, before->pll1_fracn, "PLL1 fraction changed");
	zassert_equal(after.ic2_source, before->ic2_source, "IC2 source changed");
	zassert_equal(after.ic2_divider, before->ic2_divider, "IC2 divider changed");
#if defined(STM32_IC7_ENABLED)
	zassert_equal(after.ic7_source, before->ic7_source, "audio IC7 source changed");
	zassert_equal(after.ic7_divider, before->ic7_divider, "audio IC7 divider changed");
#endif
#if defined(STM32_IC8_ENABLED)
	zassert_equal(after.ic8_source, before->ic8_source, "audio IC8 source changed");
	zassert_equal(after.ic8_divider, before->ic8_divider, "audio IC8 divider changed");
#endif
	zassert_equal(after.ahb_prescaler, before->ahb_prescaler, "AHB prescaler changed");
	zassert_equal(after.apb1_prescaler, before->apb1_prescaler, "APB1 prescaler changed");
	zassert_equal(after.apb2_prescaler, before->apb2_prescaler, "APB2 prescaler changed");
	zassert_equal(after.apb4_prescaler, before->apb4_prescaler, "APB4 prescaler changed");
	zassert_equal(after.apb5_prescaler, before->apb5_prescaler, "APB5 prescaler changed");
}

static void set_and_check_cpu_rate(uint32_t expected_hz)
{
	struct stm32_pclken ic1_clock = {
		.bus = STM32_SRC_IC1,
		.div = 0U,
		.enr = 0U,
	};
	uint32_t ic1_rate;
	int ret;

	ret = stm32_clock_control_set_cpu_rate(expected_hz);
	zassert_ok(ret, "failed to set CPU rate to %u Hz", expected_hz);
	ret = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(rcc)),
				     (clock_control_subsys_t)&ic1_clock, &ic1_rate);
	zassert_ok(ret, "failed to read live IC1 rate");
	zassert_equal(ic1_rate, expected_hz, "IC1 rate mismatch");
	zassert_equal(HAL_RCC_GetCpuClockFreq(), expected_hz, "CPU clock rate mismatch");
	zassert_equal(SystemCoreClock, expected_hz, "SystemCoreClock mismatch");
	zassert_equal(sys_clock_hw_cycles_per_sec(), CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
		      "fixed system timer frequency changed");
	zassert_equal(LL_RCC_IC1_GetDivider(), 2400000000U / expected_hz, "IC1 divider mismatch");
}

static void *test_setup(void)
{
	zassert_equal(SystemCoreClock, CPU_RATE_BOOT_HZ, "initial CPU clock metadata mismatch");
	k_timer_init(&rate_timer, rate_timer_expiry, NULL);
	atomic_set(&rate_timer_action_started, 0);
	atomic_set(&rate_timer_action_status, -EINPROGRESS);
	k_sem_reset(&rate_timer_action_done);
	return NULL;
}

static void test_teardown(void *fixture)
{
	int ret;

	ARG_UNUSED(fixture);
	k_timer_stop(&rate_timer);
	ret = stm32_clock_control_set_cpu_rate(CPU_RATE_BOOT_HZ);
	zassert_ok(ret, "failed to restore CPU boot rate");
}

ZTEST(stm32n6_cpu_clock, test_invalid_rates_preserve_clock_state)
{
	static const uint32_t invalid_rates_hz[] = {
		0U,
		123456789U,
		CPU_RATE_BOOT_HZ + 1U,
		1000000U,
	};

	uint32_t system_core_clock = SystemCoreClock;
	uint32_t runtime_rate = sys_clock_hw_cycles_per_sec();
	uint32_t ic1_rate;
	uint32_t ic1_divider = LL_RCC_IC1_GetDivider();
	struct clock_domain_snapshot domains = get_clock_domain_snapshot();

	struct stm32_pclken ic1_clock = {
		.bus = STM32_SRC_IC1,
		.div = 0U,
		.enr = 0U,
	};
	zassert_ok(clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(rcc)),
					  (clock_control_subsys_t)&ic1_clock, &ic1_rate),
		   "failed to read live IC1 rate");

	for (size_t i = 0U; i < ARRAY_SIZE(invalid_rates_hz); i++) {
		int ret = stm32_clock_control_set_cpu_rate(invalid_rates_hz[i]);

		zassert_equal(ret, -EINVAL, "rate %u returned %d", invalid_rates_hz[i], ret);
		zassert_equal(LL_RCC_IC1_GetDivider(), ic1_divider,
			      "invalid request changed IC1 divider");
		zassert_equal(SystemCoreClock, system_core_clock,
			      "invalid request changed SystemCoreClock");
		zassert_equal(sys_clock_hw_cycles_per_sec(), runtime_rate,
			      "invalid request changed runtime timer frequency");
		zassert_equal(HAL_RCC_GetCpuClockFreq(), ic1_rate,
			      "invalid request changed CPU frequency");
	}
	assert_clock_domain_unchanged(&domains);
}

ZTEST(stm32n6_cpu_clock, test_same_rate_is_idempotent)
{
	uint32_t divider = LL_RCC_IC1_GetDivider();

	set_and_check_cpu_rate(CPU_RATE_BOOT_HZ);
	zassert_equal(LL_RCC_IC1_GetDivider(), divider, "same-rate request changed divider");
}

ZTEST(stm32n6_cpu_clock, test_rate_sweep_keeps_timer_and_other_clocks_running)
{
	struct clock_domain_snapshot domains = get_clock_domain_snapshot();
	int64_t uptime_before = k_uptime_get();

	atomic_set(&rate_timer_expirations, 0);
	k_timer_start(&rate_timer, K_MSEC(5), K_MSEC(5));
	zassert_ok(k_sem_take(&rate_timer_action_done, K_MSEC(100)),
		   "rate changes from timer ISR did not complete");
	zassert_equal(atomic_get(&rate_timer_action_status), 0,
		      "rate changes from timer ISR failed");
	set_and_check_cpu_rate(CPU_RATE_BOOT_HZ);

	for (size_t i = 0U; i < ARRAY_SIZE(cpu_rates_hz); i++) {
		set_and_check_cpu_rate(cpu_rates_hz[i]);
		k_sleep(K_MSEC(20));
	}

	k_timer_stop(&rate_timer);
	zassert_true(k_uptime_get() - uptime_before >= 100,
		     "uptime did not advance across the rate sweep");
	zassert_true(atomic_get(&rate_timer_expirations) >= 10,
		     "periodic timer did not continue expiring during rate changes");
	assert_clock_domain_unchanged(&domains);
}

#if CONFIG_TEST_STM32N6_CPU_CLOCK_STRESS_ITERATIONS > 0
ZTEST(stm32n6_cpu_clock, test_rate_change_stress)
{
	for (uint32_t i = 0U; i < CONFIG_TEST_STM32N6_CPU_CLOCK_STRESS_ITERATIONS; i++) {
		uint32_t rate = cpu_rates_hz[1U + i % (ARRAY_SIZE(cpu_rates_hz) - 1U)];
		int ret = stm32_clock_control_set_cpu_rate(rate);

		zassert_ok(ret, "stress iteration %u failed for %u Hz", i, rate);
	}
	set_and_check_cpu_rate(CPU_RATE_BOOT_HZ);
}
#endif

ZTEST_SUITE(stm32n6_cpu_clock, NULL, test_setup, NULL, test_teardown, NULL);
