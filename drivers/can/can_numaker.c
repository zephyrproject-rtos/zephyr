/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker

#include <soc.h>
#include <zephyr/drivers/can.h>
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#endif
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(can_numaker, CONFIG_CAN_LOG_LEVEL);

#include "can_mcan.h"

#include "NuMicro.h"

/* Implementation notes
 *
 * 1. Use Bosch M_CAN driver (m_can) as backend
 *    NOTE: m_can only supports single configuration of max element numbers for all instances
 * 2. Need to modify can_numaker_get_core_clock() for new SOC support
 */

struct can_numaker_config {
    CANFD_T *   canfd_base;
    uint32_t    id_rst;
    uint32_t    clk_modidx;
    uint32_t    clk_src;
    uint32_t    clk_div;
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
    const struct device *               clkctrl_dev;
#endif
    void (*irq_config_func)(const struct device *dev);
#ifdef CONFIG_PINCTRL
    const struct pinctrl_dev_config *   pincfg;
#endif
};

struct can_numaker_data {
    /* Avoid zero length struct */
    uint32_t resv0;
};

static int can_numaker_get_core_clock(const struct device *dev, uint32_t *rate)
{
    const struct can_mcan_config *mcan_config = dev->config;
    const struct can_numaker_config *config = mcan_config->custom;
    int rc = 0;
    uint32_t clksrc_rate_idx;
    uint32_t clksrc_rate;
    uint32_t clkdiv_divider;

    /* Initialize clock source rate table
     *
     * NOTE: Fix the table for new SOC support
     */
    uint32_t clksrc_rate_tab[] = {
        __HXT,      // HXT
        0,          // PLLFOUT/2
        0,          // HCLK
        __HIRC      // HIRC
    };
    clksrc_rate_tab[1] = CLK_GetPLLClockFreq() / 2;
    clksrc_rate_tab[2] = CLK_GetHCLKFreq();

    /* SOC in support list
     *
     * NOTE: * NOTE: Fix the conditional for new SOC support
     */
    if (!IS_ENABLED(CONFIG_SOC_SERIES_M46X)) {
        LOG_ERR("Not support SOC: " CONFIG_SOC_SERIES);
        rc = -ENOTSUP;
        goto cleanup;
    }

    /* Module clock source rate */
    clksrc_rate_idx = CLK_GetModuleClockSource(config->clk_modidx);
    if (clksrc_rate_idx >= ARRAY_SIZE(clksrc_rate_tab)) {
        LOG_ERR("Invalid clock source rate table or index");
        rc = -EIO;
        goto cleanup;
    }
    clksrc_rate = clksrc_rate_tab[clksrc_rate_idx];

    /* Module clock divider */
    clkdiv_divider = CLK_GetModuleClockDivider(config->clk_modidx) + 1;

    LOG_DBG("Clock source/divider: %d/%d", clksrc_rate, clkdiv_divider);
    *rate = clksrc_rate / clkdiv_divider;

cleanup:

    return rc;
}

static int can_numaker_init(const struct device *dev)
{
    const struct can_mcan_config *mcan_config = dev->config;
    const struct can_numaker_config *config = mcan_config->custom;
    struct can_mcan_data *mcan_data = dev->data;
    struct can_numaker_data *data = mcan_data->custom;
    int rc = 0;

    /* Clean mutable context */
    memset(data, 0x00, sizeof(*data));

    SYS_UnlockReg();

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
    struct numaker_scc_subsys scc_subsys;

    memset(&scc_subsys, 0x00, sizeof(scc_subsys));
    scc_subsys.subsys_id        = NUMAKER_SCC_SUBSYS_ID_PCC;
    scc_subsys.pcc.clk_modidx   = config->clk_modidx;
    scc_subsys.pcc.clk_src      = config->clk_src;
    scc_subsys.pcc.clk_div      = config->clk_div;

    /* Equivalent to CLK_EnableModuleClock() */
    rc = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t) &scc_subsys);
    if (rc < 0) {
        goto cleanup;
    }
    /* Equivalent to CLK_SetModuleClock() */
    rc = clock_control_configure(config->clkctrl_dev, (clock_control_subsys_t) &scc_subsys, NULL);
    if (rc < 0) {
        goto cleanup;
    }
#else
    /* Enable module clock */
    CLK_EnableModuleClock(config->clk_modidx);

    /* Select module clock source/divider */
    CLK_SetModuleClock(config->clk_modidx, config->clk_src, config->clk_div);
#endif

    /* Configure pinmux (NuMaker's SYS MFP) */
#ifdef CONFIG_PINCTRL
    rc = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
    if (rc < 0) {
        goto cleanup;
    }
#else
#error  "No separate pinmux function implementation. Enable CONFIG_PINCTRL instead."
#endif 

    SYS_ResetModule(config->id_rst);

    config->irq_config_func(dev);

    rc = can_mcan_init(dev);
    if (rc < 0) {
        LOG_ERR("Failed to initialize mcan: %d", rc);
        goto cleanup;
    }

    uint32_t rate;
    rc = can_numaker_get_core_clock(dev, &rate);
    if (rc < 0) {
        goto cleanup;
    }
    LOG_INF("CAN core clock: %d", rate);

cleanup:

    SYS_LockReg();
    return rc;
}

static const struct can_driver_api can_numaker_driver_api = {
    .set_mode = can_mcan_set_mode,
    .set_timing = can_mcan_set_timing,
    .send = can_mcan_send,
    .add_rx_filter = can_mcan_add_rx_filter,
    .remove_rx_filter = can_mcan_remove_rx_filter,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
    .recover = can_mcan_recover,
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
    .get_state = can_mcan_get_state,
    .set_state_change_callback = can_mcan_set_state_change_callback,
    .get_core_clock = can_numaker_get_core_clock,
    .get_max_filters = can_mcan_get_max_filters,
    .get_max_bitrate = can_mcan_get_max_bitrate,
    /* Nominal bit timing limits
     *
     * NUMAKER MCAN timing limits are specified in the "Nominal bit timing and
	 * prescaler register (NBTP)" table in the SoC reference manual.
     *
     * NOTE: The values here are the "physical" timing limits, whereas the register
     *       field limits are physical values minus 1 (which is handled by the
     *       register assignments in the common MCAN driver code).
     */
    .timing_min = {
        .sjw = 1,
        .prop_seg = 0,
        .phase_seg1 = 2,
        .phase_seg2 = 2,
        .prescaler = 1
    },
    .timing_max = {
        .sjw = 128,
        .prop_seg = 0,
        .phase_seg1 = 256,
        .phase_seg2 = 128,
        .prescaler = 512,
    },
#ifdef CONFIG_CAN_FD_MODE
    .set_timing_data = can_mcan_set_timing_data,
    /* Data bit timing limits
     *
     * NUMAKER MCAN timing limits are specified in the "Data bit timing and
	 * prescaler register (DBTP)" table in the SoC reference manual.
     *
     * NOTE: Same "minus one" concern as above.
     */
    .timing_data_min = {
        .sjw = 1,
        .prop_seg = 0,
        .phase_seg1 = 1,
        .phase_seg2 = 1,
        .prescaler = 1,
    },
    .timing_data_max = {
        .sjw = 16,
        .prop_seg = 0,
        .phase_seg1 = 32,
        .phase_seg2 = 16,
        .prescaler = 32,
    }
#endif /* CONFIG_CAN_FD_MODE */
};

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
#define NUMAKER_CLKCTRL_DEV_INIT(inst)          \
    .clkctrl_dev    = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),
#else
#define NUMAKER_CLKCTRL_DEV_INIT(inst)
#endif

#ifdef CONFIG_PINCTRL
#define NUMAKER_PINCTRL_DEFINE(inst)            \
    PINCTRL_DT_INST_DEFINE(inst);
#define NUMAKER_PINCTRL_INIT(inst)              \
    .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),
#else
#define NUMAKER_PINCTRL_DEFINE(inst)
#define NUMAKER_PINCTRL_INIT(inst)
#endif

#define CAN_NUMAKER_INIT(inst)                  \
    NUMAKER_PINCTRL_DEFINE(inst);               \
                                                \
    static void can_numaker_irq_config_func_ ## inst(const struct device *dev);         \
                                                \
    static const struct can_numaker_config can_numaker_config_ ## inst = {              \
        .canfd_base = (CANFD_T *) DT_INST_REG_ADDR_BY_NAME(inst, m_can),                \
        .id_rst     = DT_INST_PROP(inst, reset),                        \
        .clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),    \
        .clk_src    = DT_INST_CLOCKS_CELL(inst, clock_source),          \
        .clk_div    = DT_INST_CLOCKS_CELL(inst, clock_divider),         \
        NUMAKER_CLKCTRL_DEV_INIT(inst)                                  \
        .irq_config_func    = can_numaker_irq_config_func_ ## inst,     \
        NUMAKER_PINCTRL_INIT(inst)                                      \
    };                                          \
                                                \
    static const struct can_mcan_config can_mcan_config_ ## inst =      \
        CAN_MCAN_DT_CONFIG_INST_GET(inst,       \
            &can_numaker_config_ ## inst);      \
                                                \
    static struct can_numaker_data can_numaker_data_ ## inst;           \
                                    \
    static struct can_mcan_data can_mcan_data_ ## inst =                \
        CAN_MCAN_DATA_INITIALIZER((struct can_mcan_msg_sram *) DT_INST_REG_ADDR_BY_NAME(inst, message_ram), \
                      &can_numaker_data_ ## inst);                      \
                                    \
    DEVICE_DT_INST_DEFINE(inst,     \
        &can_numaker_init,          \
        NULL,                       \
        &can_mcan_data_ ## inst,    \
        &can_mcan_config_ ## inst,  \
        POST_KERNEL,                \
        CONFIG_CAN_INIT_PRIORITY,   \
        &can_numaker_driver_api);   \
                                    \
    static void can_numaker_irq_config_func_ ## inst(const struct device *dev)  \
    {                                                       \
        IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 0, irq),       \
                DT_INST_IRQ_BY_IDX(inst, 0, priority),      \
                can_mcan_line_0_isr,                        \
                DEVICE_DT_INST_GET(inst),                   \
                0);                                         \
        irq_enable(DT_INST_IRQ_BY_IDX(inst, 0, irq));       \
                                                            \
        IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 1, irq),       \
                DT_INST_IRQ_BY_IDX(inst, 1, priority),      \
                can_mcan_line_1_isr,                        \
                DEVICE_DT_INST_GET(inst),                   \
                0);                                         \
        irq_enable(DT_INST_IRQ_BY_IDX(inst, 1, irq));       \
    }

DT_INST_FOREACH_STATUS_OKAY(CAN_NUMAKER_INIT);
