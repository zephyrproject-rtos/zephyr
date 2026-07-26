/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#include <bflb_soc.h>
#include <glb_reg.h>

#include "macsw_config.h"
#include "macsw_plat.h"

LOG_MODULE_REGISTER(bflb_wifi_plat, CONFIG_WIFI_LOG_LEVEL);

#define WIFI_DT_NODE      DT_NODELABEL(wifi0)
#define WIFI_IRQ_NUM      DT_IRQ_BY_NAME(WIFI_DT_NODE, wifi, irq)
#define WIFI_IRQ_PRI      DT_IRQ_BY_NAME(WIFI_DT_NODE, wifi, priority)
#define WIFI_IPC_IRQ_NUM  DT_IRQ_BY_NAME(WIFI_DT_NODE, wifi_ipc, irq)
#define WIFI_IPC_IRQ_PRI  DT_IRQ_BY_NAME(WIFI_DT_NODE, wifi_ipc, priority)
#define BFLB_RSSI_INVALID (-100)

extern void wifi_main(void *param);
extern void interrupt0_handler(void);

static K_SEM_DEFINE(wifi_task_sem, 0, 1);
static K_SEM_DEFINE(wifi_task_ready_sem, 0, 1);
static K_THREAD_STACK_DEFINE(wifi_task_stack, CONFIG_BFLB_WIFI_BL61X_TASK_STACK_SIZE);
static struct k_thread wifi_task_thread;
static atomic_t wifi_task_ready;
static void wifi_mac_isr(const void *arg)
{
	ARG_UNUSED(arg);
	interrupt0_handler();
}

static void wifi_task_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	wifi_main(NULL);
}

static void wifi_platform_clock_enable(void)
{
	uint32_t tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	tmp |= GLB_CGEN_S2_WIFI_MSK;
	sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG2_OFFSET);
}

void wifi_task_create(void)
{
	wifi_platform_clock_enable();
	IRQ_CONNECT(WIFI_IRQ_NUM, WIFI_IRQ_PRI, wifi_mac_isr, NULL, 0);
	irq_enable(WIFI_IRQ_NUM);
	IRQ_CONNECT(WIFI_IPC_IRQ_NUM, WIFI_IPC_IRQ_PRI, wifi_mac_isr, NULL, 0);
	irq_enable(WIFI_IPC_IRQ_NUM);

	k_thread_create(&wifi_task_thread, wifi_task_stack, K_THREAD_STACK_SIZEOF(wifi_task_stack),
			wifi_task_entry, NULL, NULL, NULL, CONFIG_BFLB_WIFI_BL61X_TASK_PRIORITY, 0,
			K_NO_WAIT);
	k_thread_name_set(&wifi_task_thread, "wifi_macsw");
}

void wifi_task_suspend(void)
{
	if (!atomic_test_and_set_bit(&wifi_task_ready, 0)) {
		k_sem_give(&wifi_task_ready_sem);
	}
	k_sem_take(&wifi_task_sem, K_FOREVER);
}

int wifi_task_wait_ready(k_timeout_t timeout)
{
	return k_sem_take(&wifi_task_ready_sem, timeout);
}

void wifi_task_resume(bool isr)
{
	ARG_UNUSED(isr);
	k_sem_give(&wifi_task_sem);
}

uint32_t wifi_sys_now_ms(bool isr)
{
	ARG_UNUSED(isr);
	return k_uptime_get_32();
}

__used __printf_like(2, 3) void wifi_syslog(int priority, const char *fmt, ...)
{
	ARG_UNUSED(priority);
	ARG_UNUSED(fmt);
}

/* Low-power stubs */
int bl_lp_get_resume_wifi(void)
{
	return 0;
}

void bl_lp_clear_resume_wifi(void)
{
	/* LP not supported */
}

int bl_lp_get_wake_reason(void)
{
	return 0;
}

void bl_lp_set_resume_wifi(void)
{
	/* LP not supported */
}

int8_t hal_macsw_lp_rssi_restore(void)
{
	return BFLB_RSSI_INVALID;
}

int hal_macsw_lp_get_resume_wifi(void)
{
	return 0;
}

void hal_macsw_lp_set_resume_wifi(void)
{
	/* LP not supported */
}

void hal_macsw_lp_clear_resume_wifi(void)
{
	/* LP not supported */
}

int hal_macsw_lp_is_wake_by_traffic(void)
{
	return 0;
}

int hal_macsw_lp_is_wake_by_ap_existence_check(void)
{
	return 0;
}

int hal_macsw_lp_is_wake_by_ap_disconnected(void)
{
	return 0;
}

int macsw_hook_beacon(const char *f, ...)
{
	ARG_UNUSED(f);
	return 0;
}

int macsw_hook_evt_disconnected(const char *f, ...)
{
	ARG_UNUSED(f);
	return 0;
}
