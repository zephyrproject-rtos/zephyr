/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include <bflb_soc.h>
#include <glb_reg.h>

LOG_MODULE_REGISTER(bflb_wifi_plat, CONFIG_WIFI_LOG_LEVEL);

#define WIFI_DT_NODE DT_NODELABEL(wifi0)

#define GLB_WIFI_CFG0_OFFSET      0x3B0U
#define GLB_WIFI_MAC_CORE_DIV_MSK 0xFU
#define GLB_WIFI_MAC_CORE_DIV_40M 1U

#define BFLB_CRC32_STATE_INIT 0xFFFFFFFFU

/* struct wifi_bt_coex_ctx, normally provided by the BT controller blob. */
#define BFLB_COEX_CTX_WORDS 7U

#define PWR_TABLE_INIT(prop) {DT_FOREACH_PROP_ELEM_SEP(WIFI_DT_NODE, prop, DT_PROP_BY_IDX, (,))}

/* CRC32 stream (used by the blob for beacon integrity checks). */
struct utils_crc32_stream {
	uint32_t state;
};

extern void wifi_main(void *arg);
extern void __real_bl_sleep_schedule(void);

extern void trpc_update_power_11b(int8_t *pwr);
extern void trpc_update_power_11g(int8_t *pwr);
extern void trpc_update_power_11n(int8_t *pwr);

void ipc_emb_wait(void);
void ipc_emb_notify(void);
void utils_list_push_back(sys_slist_t *list, sys_snode_t *node);
sys_snode_t *utils_list_pop_front(sys_slist_t *list);

/* Linker anchors the blob walks for its static configuration entries. */
uint8_t _ld_bl_static_cfg_entry_start[0] Z_GENERIC_SECTION(.bl_static_cfg_entry);
uint8_t _ld_bl_static_cfg_entry_end[0] Z_GENERIC_SECTION(.bl_static_cfg_entry);

/* A zeroed coexistence context leaves the WiFi side in charge of the RF. */
uint32_t coex_timing_control_ctx[BFLB_COEX_CTX_WORDS];

static K_KERNEL_STACK_DEFINE(wifi_task_stack, CONFIG_WIFI_BFLB_WIFI4_TASK_STACK_SIZE);
static struct k_thread wifi_task_thread;
static K_SEM_DEFINE(wifi_task_ready_sem, 0, 1);

/* Firmware scheduler sleep/wake on a Zephyr semaphore. */
static K_SEM_DEFINE(ipc_emb_sem, 0, 1);

static void bflb_wifi_clock_enable(void);
static void bflb_wifi_rf_param_init(void);
static void wifi_task_entry(void *p1, void *p2, void *p3);

/* Ungate the WiFi MAC clock, MAC core clock at 40 MHz.  The PHY clock is
 * ungated in soc_early_init_hook().
 */
static void bflb_wifi_clock_enable(void)
{
	uint32_t v;

	v = sys_read32(GLB_BASE + GLB_CGEN_CFG2_OFFSET);
	sys_write32(v | GLB_CGEN_S2_WIFI_MSK, GLB_BASE + GLB_CGEN_CFG2_OFFSET);

	v = sys_read32(GLB_BASE + GLB_WIFI_CFG0_OFFSET);
	v = (v & ~GLB_WIFI_MAC_CORE_DIV_MSK) | GLB_WIFI_MAC_CORE_DIV_40M;
	sys_write32(v, GLB_BASE + GLB_WIFI_CFG0_OFFSET);
}

/* Per-rate TX power targets from devicetree, in dBm. */
static void bflb_wifi_rf_param_init(void)
{
	static int8_t pwr_11b[4] = PWR_TABLE_INIT(pwr_table_11b);
	static int8_t pwr_11g[8] = PWR_TABLE_INIT(pwr_table_11g);
	static int8_t pwr_11n[8] = PWR_TABLE_INIT(pwr_table_11n);

	trpc_update_power_11b(pwr_11b);
	trpc_update_power_11g(pwr_11g);
	trpc_update_power_11n(pwr_11n);
}

static void wifi_task_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	wifi_main(NULL);
}

int bflb_wifi_hw_init(void)
{
	/* WIFI_RAM is a custom memory-region the kernel never zeroes; the
	 * blob's NOLOAD .fw.SHRAM / .fw.SHRAMIPC objects live there.
	 */
	memset((void *)DT_REG_ADDR(DT_NODELABEL(wifi_ram)), 0, DT_REG_SIZE(DT_NODELABEL(wifi_ram)));

	bflb_wifi_clock_enable();
	bflb_wifi_rf_param_init();

	return 0;
}

void wifi_task_create(void)
{
	k_thread_create(&wifi_task_thread, wifi_task_stack, K_THREAD_STACK_SIZEOF(wifi_task_stack),
			wifi_task_entry, NULL, NULL, NULL,
			K_PRIO_PREEMPT(CONFIG_WIFI_BFLB_WIFI4_TASK_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&wifi_task_thread, "bl808_wifi_fw");
}

int wifi_task_wait_ready(k_timeout_t timeout)
{
	return k_sem_take(&wifi_task_ready_sem, timeout);
}

/* The scheduler loop in wifi_main() starts with this call, so its first
 * run means the firmware bring-up is complete.
 */
void __wrap_bl_sleep_schedule(void)
{
	static bool ready;

	if (!ready) {
		ready = true;
		k_sem_give(&wifi_task_ready_sem);
	}

	__real_bl_sleep_schedule();
}

void __wrap_ipc_emb_wait(void)
{
	k_sem_take(&ipc_emb_sem, K_MSEC(1));
}

void __wrap_ipc_emb_notify(void)
{
	k_sem_give(&ipc_emb_sem);
}

/* The station is marked valid in sta_mgmt_register() before me_init_rate()
 * gives it a rate-control block, so a transmit confirmation landing in
 * that window reaches rc_update_counters() with a NULL block and trips a
 * fatal assert.  Skip the un-initialised stations.
 */
#define RC_STA_INFO_STRIDE  368U
#define RC_STA_INFO_VALID   39U
#define RC_STA_INFO_STATS   324U
#define BFLB_STA_INFO_COUNT 5U

extern uint8_t sta_info_tab[];
extern void __real_rc_update_counters(uint32_t sta_idx, uint32_t a, uint32_t b);

void __wrap_rc_update_counters(uint32_t sta_idx, uint32_t a, uint32_t b)
{
	const uint8_t *sta;

	if (sta_idx >= BFLB_STA_INFO_COUNT) {
		return;
	}

	sta = &sta_info_tab[sta_idx * RC_STA_INFO_STRIDE];
	if ((sta[RC_STA_INFO_VALID] != 0xffU) &&
	    (*(const uint32_t *)&sta[RC_STA_INFO_STATS] == 0U)) {
		return;
	}

	__real_rc_update_counters(sta_idx, a, b);
}

/* The rate the firmware last programmed for a station lives in its TX
 * policy table: sta_info_tab[idx] + 320 points at the policy, and the
 * first entry of the retry chain at +20 is the rate it transmits at, with
 * the retry count in its top bits.  Only the non-HT format is decoded;
 * any other format reports unknown.
 */
#define RC_STA_INFO_POLICY 320U
#define POLICY_RATE_CHAIN  20U
#define RATE_INFO_FMT_MASK GENMASK(13, 11)
#define RATE_INFO_IDX_MASK 0x7fU

const uint16_t bflb_wifi_legacy_rates_100kbps[] = {
	10, 20, 55, 110, 60, 90, 120, 180, 240, 360, 480, 540,
};

int bflb_wifi_phy_rate_kbps(uint8_t sta_idx)
{
	const uint8_t *sta;
	const uint8_t *policy;
	uint32_t info;

	if (sta_idx >= BFLB_STA_INFO_COUNT) {
		return 0;
	}

	sta = &sta_info_tab[sta_idx * RC_STA_INFO_STRIDE];
	if (sta[RC_STA_INFO_VALID] == 0xffU) {
		return 0;
	}

	policy = (const uint8_t *)*(const uint32_t *)&sta[RC_STA_INFO_POLICY];
	if (policy == NULL) {
		return 0;
	}

	info = *(const uint32_t *)&policy[POLICY_RATE_CHAIN];
	if (((info & RATE_INFO_FMT_MASK) != 0U) ||
	    ((info & RATE_INFO_IDX_MASK) >= ARRAY_SIZE(bflb_wifi_legacy_rates_100kbps))) {
		return 0;
	}

	return (int)bflb_wifi_legacy_rates_100kbps[info & RATE_INFO_IDX_MASK] * 100;
}

/* Removing an interface deletes its station again after the disconnect
 * already did, and the second call reads the station's now-invalid
 * interface number (0xff) as an index into vif_info_tab.  Deleting a
 * station that is already gone is a no-op, so stop before it indexes.
 */
extern void __real_sta_mgmt_unregister(uint32_t sta_idx);

void __wrap_sta_mgmt_unregister(uint32_t sta_idx)
{
	if (sta_idx >= BFLB_STA_INFO_COUNT) {
		return;
	}

	if (sta_info_tab[(sta_idx * RC_STA_INFO_STRIDE) + RC_STA_INFO_VALID] == 0xffU) {
		return;
	}

	__real_sta_mgmt_unregister(sta_idx);
}

/* Timing primitives the blobs import. */
void arch_delay_us(uint32_t us)
{
	k_busy_wait(us);
}

uint32_t bl_os_clock_gettime_ms(void)
{
	return k_uptime_get_32();
}

/* Host symbols the firmware blob imports but that need no work here. */
void bl_main_event_handle(int param, void *tx_fc_field)
{
	ARG_UNUSED(param);
	ARG_UNUSED(tx_fc_field);
}

int bl_supplicant_init(void *arg)
{
	ARG_UNUSED(arg);
	return 0;
}

void bl_utils_dump(void)
{
	/* Stub */
}

int bl_printf(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
	return 0;
}

int blob_puts(const char *s)
{
	ARG_UNUSED(s);
	return 0;
}

int blob_putchar(int c)
{
	return c;
}

/* Coexistence debug GPIOs, not wired on supported boards. */
void bl_gpio_enable_output(uint8_t pin, uint8_t pullup, uint8_t pulldown)
{
	ARG_UNUSED(pin);
	ARG_UNUSED(pullup);
	ARG_UNUSED(pulldown);
}

void bl_gpio_output_set(uint8_t pin, uint8_t value)
{
	ARG_UNUSED(pin);
	ARG_UNUSED(value);
}

/* Singly-linked list the firmware queues run on; the blob imports these
 * entry points and its list layout is sys_slist_t.  The TX lists are
 * shared with the host, which may run between the firmware's list stores.
 */
void utils_list_push_back(sys_slist_t *list, sys_snode_t *node)
{
	unsigned int key = irq_lock();

	sys_slist_append(list, node);
	irq_unlock(key);
}

sys_snode_t *utils_list_pop_front(sys_slist_t *list)
{
	unsigned int key = irq_lock();
	sys_snode_t *node = sys_slist_get(list);

	irq_unlock(key);
	return node;
}

int utils_crc32_stream_init(struct utils_crc32_stream *s)
{
	if (s != NULL) {
		s->state = BFLB_CRC32_STATE_INIT;
	}
	return 0;
}

int utils_crc32_stream_feed_block(struct utils_crc32_stream *s, const void *data, uint32_t len)
{
	if (s == NULL) {
		return -EINVAL;
	}
	s->state = crc32_ieee_update(s->state, data, len);
	return 0;
}

/* Single struct-ptr signature: callers only set a0, a two-arg form would
 * read garbage in a1 and trash random memory.
 */
uint32_t utils_crc32_stream_results(struct utils_crc32_stream *s)
{
	return (s != NULL) ? ~s->state : BFLB_CRC32_STATE_INIT;
}

/* TLV util (RF parameters) -- no RF blob TLV in flash. */
int utils_tlv_bl_unpack_auto(void *tlv, uint32_t len, void *out)
{
	ARG_UNUSED(tlv);
	ARG_UNUSED(len);
	ARG_UNUSED(out);
	return 0;
}

/* HOSAL power-management hooks -- all no-op. */
int wifi_hosal_rf_turn_on(void *a)
{
	ARG_UNUSED(a);
	return 0;
}

int wifi_hosal_rf_turn_off(void *a)
{
	ARG_UNUSED(a);
	return 0;
}

int wifi_hosal_pm_state_run(void)
{
	return 0;
}

int wifi_hosal_pm_post_event(int ev, uint32_t code, uint32_t *r)
{
	ARG_UNUSED(ev);
	ARG_UNUSED(code);
	ARG_UNUSED(r);
	return 0;
}

int wifi_hosal_pm_event_register(int ev, uint32_t code, uint32_t cap_bit, uint16_t prio, void *ops,
				 void *arg, int enable)
{
	ARG_UNUSED(ev);
	ARG_UNUSED(code);
	ARG_UNUSED(cap_bit);
	ARG_UNUSED(prio);
	ARG_UNUSED(ops);
	ARG_UNUSED(arg);
	ARG_UNUSED(enable);
	return 0;
}
