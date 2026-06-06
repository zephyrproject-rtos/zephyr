/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL60x WiFi platform support: AON power rails, GLB clock ungate, PHY/RF
 * parameter init, firmware task bring-up and the linker-wrap glue the
 * wifi4 MAC firmware blob needs to run on Zephyr.
 */

#include <stdarg.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include <bflb_soc.h>
#include <aon_reg.h>
#include <glb_reg.h>

#include <bouffalolab/bl60x/wifi/utils_list.h>

LOG_MODULE_REGISTER(bflb_wifi_plat, CONFIG_WIFI_LOG_LEVEL);

#define WIFI_DT_NODE    DT_NODELABEL(wifi0)
#define RF_TOP0_IRQ_NUM DT_IRQ_BY_NAME(WIFI_DT_NODE, rf_top0, irq)
#define RF_TOP0_IRQ_PRI DT_IRQ_BY_NAME(WIFI_DT_NODE, rf_top0, priority)
#define RF_TOP1_IRQ_NUM DT_IRQ_BY_NAME(WIFI_DT_NODE, rf_top1, irq)
#define RF_TOP1_IRQ_PRI DT_IRQ_BY_NAME(WIFI_DT_NODE, rf_top1, priority)

/* RF/AON tuning constants. */
#define BFLB_AON_DELAY_SHORT_US   20U
#define BFLB_AON_DELAY_MED_US     60U
#define BFLB_AON_DELAY_LONG_US    100U
#define BFLB_AHB_GATE_WIFI_MAC    BIT(6)
#define BFLB_AHB_GATE_WIFI_PHY    BIT(7)
#define BFLB_XTAL_CAPCODE_DEFAULT 50U

/* MAC HW GEN-status registers (DMA-dead recovery). */
#define MAC_HW_GEN_RAW_REG     0x44B08074U
#define MAC_HW_GEN_MASK_REG    0x44B0806CU
#define MAC_HW_GEN_LOAD_REG    0x44B08180U
#define MAC_HW_GEN_RXDMA_DEAD  BIT(8)
#define MAC_HW_GEN_HD_DMA_DEAD BIT(16)
#define MAC_HW_GEN_TXDMA_DEAD  BIT(24)
#define MAC_HW_GEN_LOAD_PD_DMA 0x0C000000U
#define MAC_HW_GEN_LOAD_HD_DMA 0x08000000U
#define MAC_HW_GEN_LOAD_TX_DMA 0x04000000U

/* MAC controller / sysctrl registers used by the wifi_main bring-up. */
#define GLB_SWRST_CFG0        0x40000010U
#define GLB_SWRST_S20_BIT     BIT(4)
#define MAC_CTRL_REG          0x44B00400U
#define MAC_CTRL_EN_BIT       BIT(0)
#define MAC_CTRL_DIS_BIT      BIT(5)
#define MAC_CTRL_RESET_VALUE  0x68U
#define MAC_PHY_ABI_FREQ_REG  0x44B00404U
#define MAC_PHY_ABI_FREQ_VAL  0x24F037U
#define MAC_HW_TIMER_REG      0x44B00120U
#define MAC_HW_TIMER_TICK_BIT BIT(31)
#define MAC_SLEEP_GATE_REG    0x44900084U
#define MAC_SLEEP_GATE_BIT    BIT(0)
#define WIFI_SUBSYS_CLK_REG   0x44920004U
#define WIFI_SUBSYS_CLK_VAL   0x5010001FU
#define RF_XTAL_HZ            40000000U
#define MAC_RESET_PULSE_US    100U

/* RF tunables. */
#define BFLB_RF_PWR_AVG_REAL_CALL_LIMIT 50
#define BFLB_RF_PWR_AVG_STUB_VALUE      1000

/* Entry points into the firmware/PHY blobs. */
extern void wifi_main(void *arg);
extern int wifi_hosal_rf_turn_on(void *arg);
extern int rfc_init(uint32_t xtal);
extern void mpif_clk_init(void);
extern void sysctrl_init(void);
extern void intc_init(void);
extern void ipc_emb_init(void);
extern void bl_init(void);
extern void bl_pm_ops_register(void);
extern void bl_sleep_schedule(void);
extern void ke_evt_schedule(void);
extern uint32_t ke_env;

/* Linker anchors the blob walks for its static configuration entries. */
uint8_t _ld_bl_static_cfg_entry_start[0] Z_GENERIC_SECTION(.bl_static_cfg_entry);
uint8_t _ld_bl_static_cfg_entry_end[0] Z_GENERIC_SECTION(.bl_static_cfg_entry);

/* RF power-on: WB, MBG, LDO15RF, SFREG rails plus WiFi AHB clock ungate
 * and a default XTAL capcode (without it the crystal can be off enough
 * that RX never hears beacons).
 */
static void bflb_wifi_rf_power_on(void)
{
	uint32_t v;

	v = sys_read32(AON_BASE + AON_RF_TOP_AON_OFFSET);
	v |= BIT(AON_PU_MBG_AON_POS);
	sys_write32(v, AON_BASE + AON_RF_TOP_AON_OFFSET);
	k_busy_wait(BFLB_AON_DELAY_SHORT_US);

	v |= BIT(AON_PU_LDO15RF_AON_POS);
	sys_write32(v, AON_BASE + AON_RF_TOP_AON_OFFSET);
	k_busy_wait(BFLB_AON_DELAY_MED_US);

	v |= BIT(AON_PU_SFREG_AON_POS);
	sys_write32(v, AON_BASE + AON_RF_TOP_AON_OFFSET);
	k_busy_wait(BFLB_AON_DELAY_SHORT_US);

	v = sys_read32(AON_BASE + AON_MISC_OFFSET);
	v |= BIT(AON_SW_WB_EN_AON_POS);
	sys_write32(v, AON_BASE + AON_MISC_OFFSET);
	k_busy_wait(BFLB_AON_DELAY_LONG_US);

	v = sys_read32(GLB_BASE + GLB_CGEN_CFG0_OFFSET);
	v |= BFLB_AHB_GATE_WIFI_MAC | BFLB_AHB_GATE_WIFI_PHY;
	sys_write32(v, GLB_BASE + GLB_CGEN_CFG0_OFFSET);
	k_busy_wait(BFLB_AON_DELAY_LONG_US);

	v = sys_read32(AON_BASE + AON_XTAL_CFG_OFFSET);
	v &= AON_XTAL_CAPCODE_IN_AON_UMSK & AON_XTAL_CAPCODE_OUT_AON_UMSK;
	v |= (BFLB_XTAL_CAPCODE_DEFAULT << AON_XTAL_CAPCODE_IN_AON_POS) |
	     (BFLB_XTAL_CAPCODE_DEFAULT << AON_XTAL_CAPCODE_OUT_AON_POS);
	sys_write32(v, AON_BASE + AON_XTAL_CFG_OFFSET);
	k_busy_wait(BFLB_AON_DELAY_LONG_US);
}

/* Swallow RF-top interrupts during calibration: the PHY lib polls RF
 * registers internally and doesn't need these ISRs to do work.
 */
static void rf_top_isr(const void *arg)
{
	ARG_UNUSED(arg);
}

/* RF parameter init: no RFTLV blob in flash, feed the PHY the power
 * tables from devicetree (quarter-dBm).
 */
extern void trpc_update_power_11b(int8_t *pwr);
extern void trpc_update_power_11g(int8_t *pwr);
extern void trpc_update_power_11n(int8_t *pwr);

#define PWR_TABLE_INIT(prop) {DT_FOREACH_PROP_ELEM_SEP(WIFI_DT_NODE, prop, DT_PROP_BY_IDX, (,))}

int32_t rfparam_init(uint32_t base_addr, void *rf_para, uint32_t apply_flag)
{
	static int8_t pwr_11b[4] = PWR_TABLE_INIT(pwr_table_11b);
	static int8_t pwr_11g[8] = PWR_TABLE_INIT(pwr_table_11g);
	static int8_t pwr_11n[8] = PWR_TABLE_INIT(pwr_table_11n);

	ARG_UNUSED(base_addr);
	ARG_UNUSED(rf_para);
	ARG_UNUSED(apply_flag);

	trpc_update_power_11b(pwr_11b);
	trpc_update_power_11g(pwr_11g);
	trpc_update_power_11n(pwr_11n);

	return 0;
}

extern void rfc_config_channel(uint32_t channel_freq);

/* Weak: newer phyrf library revisions export rf_set_channel themselves. */
__weak void rf_set_channel(uint8_t bandwidth, uint16_t channel_freq)
{
	ARG_UNUSED(bandwidth);
	rfc_config_channel(channel_freq);
}

int bflb_wifi_hw_init(void)
{
	bflb_wifi_rf_power_on();

	if (rfparam_init(0, NULL, 0) != 0) {
		LOG_ERR("rfparam_init failed");
		return -EIO;
	}

	IRQ_CONNECT(RF_TOP0_IRQ_NUM, RF_TOP0_IRQ_PRI, rf_top_isr, NULL, 0);
	IRQ_CONNECT(RF_TOP1_IRQ_NUM, RF_TOP1_IRQ_PRI, rf_top_isr, NULL, 0);
	irq_enable(RF_TOP0_IRQ_NUM);
	irq_enable(RF_TOP1_IRQ_NUM);

	return 0;
}

/* WiFi firmware thread: the wrapped wifi_main() below is a cooperative
 * scheduler that never returns.
 */
static K_KERNEL_STACK_DEFINE(wifi_task_stack, CONFIG_BFLB_WIFI_BL60X_TASK_STACK_SIZE);
static struct k_thread wifi_task_thread;
static K_SEM_DEFINE(wifi_task_ready_sem, 0, 1);

static void wifi_task_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	wifi_main(NULL);
}

void wifi_task_create(void)
{
	k_thread_create(&wifi_task_thread, wifi_task_stack, K_THREAD_STACK_SIZEOF(wifi_task_stack),
			wifi_task_entry, NULL, NULL, NULL,
			K_PRIO_PREEMPT(CONFIG_BFLB_WIFI_BL60X_TASK_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&wifi_task_thread, "bl60x_wifi_fw");
}

int wifi_task_wait_ready(k_timeout_t timeout)
{
	return k_sem_take(&wifi_task_ready_sem, timeout);
}

/* Firmware scheduler sleep/wake on a Zephyr semaphore. */
static K_SEM_DEFINE(ipc_emb_sem, 0, 1);

void __wrap_ipc_emb_wait(void)
{
	k_sem_take(&ipc_emb_sem, K_MSEC(1));
}

void __wrap_ipc_emb_notify(void)
{
	k_sem_give(&ipc_emb_sem);
}

void ipc_emb_wait(void);
void ipc_emb_notify(void);

static void wifi_mac_reset_pulse(void)
{
	unsigned int key = irq_lock();
	uint32_t v = sys_read32(GLB_SWRST_CFG0);

	sys_write32(v | GLB_SWRST_S20_BIT, GLB_SWRST_CFG0);
	k_busy_wait(MAC_RESET_PULSE_US);
	sys_write32(v & ~GLB_SWRST_S20_BIT, GLB_SWRST_CFG0);
	k_busy_wait(MAC_RESET_PULSE_US);

	irq_unlock(key);
}

/* MAC controller R-M-W cycle that hal_machw_init also performs later during
 * the MM_RESET handler.  Doing it early puts the MAC in the correct state
 * before bl_init's task scheduler starts processing IPC.
 */
static void wifi_mac_ctrl_init_cycle(void)
{
	uint32_t v;

	v = sys_read32(MAC_CTRL_REG);
	sys_write32(v | MAC_CTRL_EN_BIT, MAC_CTRL_REG);

	sys_write32(MAC_CTRL_RESET_VALUE, MAC_CTRL_REG);

	v = sys_read32(MAC_CTRL_REG);
	sys_write32(v | MAC_CTRL_EN_BIT, MAC_CTRL_REG);

	v = sys_read32(MAC_CTRL_REG);
	sys_write32(v & ~MAC_CTRL_DIS_BIT, MAC_CTRL_REG);
}

static void wifi_mac_sleep_gate_update(void)
{
	uint32_t hwt = sys_read32(MAC_HW_TIMER_REG);
	uint32_t reg = sys_read32(MAC_SLEEP_GATE_REG);

	if ((hwt & MAC_HW_TIMER_TICK_BIT) != 0U) {
		sys_write32(reg | MAC_SLEEP_GATE_BIT, MAC_SLEEP_GATE_REG);
	} else {
		sys_write32(reg & ~MAC_SLEEP_GATE_BIT, MAC_SLEEP_GATE_REG);
	}
}

/* Replacement for the blob's wifi_main(): bring the MAC out of reset, run
 * the same sysctrl / clk / IPC init the blob does, then enter the
 * cooperative scheduler.  The MAC HW reset pulse on SWRST_S20 and the MAC
 * controller R-M-W cycle are required: without the full sequence RX wedges
 * with HW_IDLE and rx_dma_hdrdesc stays at 0xbaadf00d.
 */
void __wrap_wifi_main(void *arg)
{
	ARG_UNUSED(arg);

	wifi_mac_reset_pulse();
	(void)wifi_hosal_rf_turn_on(NULL);
	(void)rfc_init(RF_XTAL_HZ);

	sys_write32(sys_read32(MAC_CTRL_REG) | MAC_CTRL_EN_BIT, MAC_CTRL_REG);

	mpif_clk_init();
	sysctrl_init();
	intc_init();
	ipc_emb_init();
	bl_init();
	bl_pm_ops_register();

	sys_write32(MAC_PHY_ABI_FREQ_VAL, MAC_PHY_ABI_FREQ_REG);
	wifi_mac_ctrl_init_cycle();
	sys_write32(WIFI_SUBSYS_CLK_VAL, WIFI_SUBSYS_CLK_REG);

	LOG_DBG("wifi_main: scheduler running");
	k_sem_give(&wifi_task_ready_sem);

	for (;;) {
		wifi_mac_sleep_gate_update();
		bl_sleep_schedule();
		if (*(volatile uint32_t *)&ke_env == 0U) {
			ipc_emb_wait();
		}
		ke_evt_schedule();
	}
}

/* MAC HW GEN-status DMA-dead recovery: pulse the LOAD bits so MAC HW
 * re-reads its DMA pointer registers and resumes.
 */
extern void __real_hal_machw_gen_handler(void);

void __wrap_hal_machw_gen_handler(void)
{
	uint32_t pending = sys_read32(MAC_HW_GEN_RAW_REG) & sys_read32(MAC_HW_GEN_MASK_REG);

	__real_hal_machw_gen_handler();

	if ((pending & MAC_HW_GEN_TXDMA_DEAD) != 0U) {
		sys_write32(MAC_HW_GEN_LOAD_TX_DMA, MAC_HW_GEN_LOAD_REG);
	}
	if ((pending & MAC_HW_GEN_HD_DMA_DEAD) != 0U) {
		sys_write32(MAC_HW_GEN_LOAD_HD_DMA, MAC_HW_GEN_LOAD_REG);
	}
	if ((pending & MAC_HW_GEN_RXDMA_DEAD) != 0U) {
		sys_write32(MAC_HW_GEN_LOAD_PD_DMA, MAC_HW_GEN_LOAD_REG);
	}
}

/* Some RF calibration paths poll ADC registers with tight timing;
 * preempting between register accesses leaves the MAC/PHY bus wedged.
 * Run the real calibration with IRQs locked and pre-init the cal memory.
 */
extern int __real_rfc_init(uint32_t xtal);
extern void rf_pri_init_calib_mem(void);

static volatile bool rf_cal_active;

int __wrap_rfc_init(uint32_t xtal)
{
	unsigned int key;
	int r;

	rf_pri_init_calib_mem();

	rf_cal_active = true;
	key = irq_lock();
	r = __real_rfc_init(xtal);
	irq_unlock(key);
	rf_cal_active = false;

	return r;
}

/* The PHY's pm_pwr_avg spins on a power-meter read.  After the tmx_cs
 * sweep the result no longer matters; short-circuit with a stub value.
 */
extern int __real_rf_pri_pm_pwr_avg(int arg);

int __wrap_rf_pri_pm_pwr_avg(int arg)
{
	static int count;

	count++;
	if (count < BFLB_RF_PWR_AVG_REAL_CALL_LIMIT) {
		return __real_rf_pri_pm_pwr_avg(arg);
	}
	return BFLB_RF_PWR_AVG_STUB_VALUE;
}

/* Drop the PHY library's bare printf while calibration runs (UART latency
 * there breaks PHY timing); forward everything else so the wrap does not
 * disable printf for the rest of the image.
 */
int __wrap_printf(const char *fmt, ...)
{
	va_list ap;
	int r;

	if (rf_cal_active) {
		return 0;
	}

	va_start(ap, fmt);
	r = vprintf(fmt, ap);
	va_end(ap);

	return r;
}

/* Timing primitives the blobs import. */
void arch_delay_us(uint32_t us)
{
	k_busy_wait(us);
}

void BL602_Delay_US(uint32_t us)
{
	k_busy_wait(us);
}

void BL602_Delay_MS(uint32_t ms)
{
	k_msleep(ms);
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

/* Singly-linked list used by the firmware TX descriptor queues. */
void utils_list_push_back(struct utils_list *l, struct utils_list_hdr *h)
{
	h->next = NULL;
	if (l->last != NULL) {
		((struct utils_list_hdr *)l->last)->next = h;
	} else {
		l->first = h;
	}
	l->last = h;
}

struct utils_list_hdr *utils_list_pop_front(struct utils_list *l)
{
	struct utils_list_hdr *h = (struct utils_list_hdr *)l->first;

	if (h != NULL) {
		l->first = h->next;
		if (l->first == NULL) {
			l->last = NULL;
		}
	}
	return h;
}

void utils_list_init(struct utils_list *l)
{
	l->first = NULL;
	l->last = NULL;
}

/* CRC32 stream (used by the blob for beacon integrity checks). */
struct utils_crc32_stream {
	uint32_t state;
};

#define BFLB_CRC32_STATE_INIT 0xFFFFFFFFU

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
