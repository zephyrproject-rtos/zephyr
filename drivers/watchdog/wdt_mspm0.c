/*
 * Copyright (c) 2024 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_watchdog

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

// #define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
// #include <zephyr/logging/log.h>
// LOG_MODULE_REGISTER(wdt_mspm0)

/* Driverlib includes */
#include <ti/driverlib/dl_wwdt.h>

#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_25_MAX_MSEC                  (8192000)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_25_INTERVAL                  (1024000)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_21_MAX_MSEC                   (512000)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_21_INTERVAL                    (64000)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_18_MAX_MSEC                    (64000)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_18_INTERVAL                     (8000)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_15_MAX_MSEC                     (8000)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_15_INTERVAL                     (1000)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_12_MAX_MSEC                     (1000)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_12_INTERVAL                      (125)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_10_MAX_MSEC                      (256)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_10_INTERVAL                       (32)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_8_MAX_MSEC                        (64)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_8_INTERVAL                         (8)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_6_MAX_MSEC                        (16)
#define WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_6_INTERVAL                         (2)


struct wwdt_mspm0_config {
    uint32_t base;
    uint32_t reset_action;
};

enum wwdt_mspm0_state {
    WWDT_MSPM0_INITIALIZED,
    WWDT_MSPM0_INSTALLED,
    WWDT_MSPM0_RUNNING,
};

struct wwdt_mspm0_data {
    volatile enum wwdt_mspm0_state state;
    uint32_t periodCount;
    uint32_t clockDivider;
    uint32_t windowCount0;
    uint32_t windowCount1;
};

static int wwdt_mspm0_calculate_timeout_periods(const struct device *dev, const struct wdt_timeout_cfg *cfg){
    struct wwdt_mspm0_data * data = dev->data;

    /* WWDT is always sourced by LFCLK (32768 Hz), but is restricted to counts
     * of T = (CLKDIV + 1) * PER_count / 32768 Hz , where CLKdiv is all
     * integers between 1-8, and PER_count is period count values that are
     * 2^6, 2^8, 2^10, 2^12, 2^15, 2^18, 2^21, 2^25. These periods that can be
     * calculated are from 1.95 ms to 136.53 seconds
     *
     * The closest value above a given value in milliseconds can always be found
     * by performing a search from lowest clock div value to the highest. This
     * is done by performing a search path from lowest to highest. Since there
     * are only 8 comparisons, it's faster to search on MSPM0 than functionally
     * trying to compute the exact value or logarithm (or MSB)
     */

    /* First, a search is performed assuming that the clock divider is 7, and
     * the search is only performed over the period_count values
     */
    uint32_t max_ms = cfg->window.max;
    uint32_t min_ms = cfg->window.min;
    uint32_t periodCountInterval;
    uint32_t proposedInterval;

    if(max_ms > WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_25_MAX_MSEC ||
        min_ms >= max_ms){
        /* millisecond value is too large. Cannot be supported */
        return -EINVAL;
    } else if (max_ms > WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_21_MAX_MSEC){
        data->periodCount = DL_WWDT_TIMER_PERIOD_25_BITS;
        periodCountInterval = WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_25_INTERVAL;
    } else if (max_ms > WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_18_MAX_MSEC){
        periodCountInterval = WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_21_INTERVAL;
        data->periodCount = DL_WWDT_TIMER_PERIOD_21_BITS;
    } else if (max_ms > WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_15_MAX_MSEC){
        periodCountInterval = WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_18_INTERVAL;
        data->periodCount = DL_WWDT_TIMER_PERIOD_18_BITS;
    } else if (max_ms > WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_12_MAX_MSEC){
        periodCountInterval = WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_15_INTERVAL;
        data->periodCount = DL_WWDT_TIMER_PERIOD_15_BITS;
    } else if (max_ms > WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_10_MAX_MSEC){
        periodCountInterval = WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_12_INTERVAL;
        data->periodCount = DL_WWDT_TIMER_PERIOD_12_BITS;
    } else if (max_ms > WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_8_MAX_MSEC){
        periodCountInterval = WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_10_INTERVAL;
        data->periodCount = DL_WWDT_TIMER_PERIOD_10_BITS;
    } else if (max_ms > WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_6_MAX_MSEC){
        periodCountInterval = WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_8_INTERVAL;
        data->periodCount = DL_WWDT_TIMER_PERIOD_8_BITS;
    } else {
        /* ms value is sufficiently small to yield smallest count value */
        periodCountInterval = WWDT_MSPM0_PERIOD_COUNT_LUT_POWER_6_INTERVAL;
        data->periodCount = DL_WWDT_TIMER_PERIOD_6_BITS;
    }

    /* Determine clock divider based on the period count. Since rounding up
     * is the defined behavior, walking up and checking for when the value is
     * equal or underneath will yield the rounded up value
     */
    proposedInterval = periodCountInterval;
    for(int i = 0; i < 8; i++){
        if(max_ms <= proposedInterval){
            data->clockDivider = i;
            break;
        }
        /* addition to proposed interval to mimic clock divider
         * getting larger (bigger period) (from /1 to /2 to /3, etc.) */
        proposedInterval += periodCountInterval;
    }


    /* Determine clock window as a percentage of the total clock width,
     * again rounding up.
     * clock windows are not evenly spaced, and the optons are:
     * 0%, 12.5%, 18.75%, 25%, 50%, 75%, 81.25%, 87.5%,
     * or in sixteenths, the options are:
     * 0/16, 2/16, 3/16, 4/16, 8/16, 12/16, 13/16, 14/16
     */
    const uint32_t window_sixteenth[] = {0, 2, 3, 4, 8, 12, 13, 14};

    for(int i = 0; i < 8; i++){
        if(min_ms <= (proposedInterval * window_sixteenth[i]/16)) {
            data->windowCount0 = (i << WWDT_WWDTCTL0_WINDOW0_OFS);
            data->windowCount1 = (i << WWDT_WWDTCTL0_WINDOW0_OFS);
            break;
        }
        if(i == 7){
            /* No valid window percentage was found. Thus we round down in
             * this specific case ONLY */
            data->windowCount0 = (i << WWDT_WWDTCTL0_WINDOW0_OFS);
            data->windowCount1 = (i << WWDT_WWDTCTL0_WINDOW0_OFS);
        }
    }

    return 0;
}

static int wwdt_mspm0_init(const struct device *dev)
{
    const struct wwdt_mspm0_config * config = dev->config;
    struct wwdt_mspm0_data * data = dev->data;
    DL_WWDT_reset((WWDT_Regs *) config->base);

    DL_WWDT_enablePower((WWDT_Regs *) config->base);
    data->state = WWDT_MSPM0_INITIALIZED;

    return 0;
}

static int wwdt_mspm0_setup(const struct device *dev, uint8_t options){
    const struct wwdt_mspm0_config * config = dev->config;
    struct wwdt_mspm0_data * data = dev->data;

    DL_WWDT_SLEEP_MODE sleepMode = DL_WWDT_RUN_IN_SLEEP;

    if((options & WDT_OPT_PAUSE_IN_SLEEP) == WDT_OPT_PAUSE_IN_SLEEP){
        sleepMode = DL_WWDT_STOP_IN_SLEEP;
    }

    if((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != WDT_OPT_PAUSE_HALTED_BY_DBG){
        /* On reset the MSPM0 is set to halt with the core halting */
        WWDT_Regs * WWDT_INST = (WWDT_Regs *) config->base;
        WWDT_INST->PDBGCTL = WWDT_PDBGCTL_FREE_RUN;
    }

    /* This call enables the Watchdog */
    DL_WWDT_initWatchdogMode((WWDT_Regs *) config->base, data->clockDivider,
        data->periodCount, sleepMode, data->windowCount0, data->windowCount1);

    return 0;
}

static int wwdt_mspm0_disable(const struct device *dev){

    /* Disabling a watchdog that is configured is not possible */
    ARG_UNUSED(dev);

    return -EPERM;
}

static int wwdt_mspm0_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg){
    const struct wwdt_mspm0_config * config = dev->config;
    struct wwdt_mspm0_data * data = dev->data;

    if(data->state == WWDT_MSPM0_RUNNING){
        return -EBUSY;
    }
    else if(data->state == WWDT_MSPM0_INSTALLED) {
        return -ENOMEM;
    }

    if(cfg->callback != NULL){
        return -ENOTSUP;
    }
    if(!(cfg->flags & config->reset_action)){
        return -ENOTSUP;
    }

    int status = wwdt_mspm0_calculate_timeout_periods(dev, cfg);
    if(status == 0){
        data->state = WWDT_MSPM0_INSTALLED;
    }

    return status;
}

static int wwdt_mspm0_feed(const struct device *dev, int channel_id){
    const struct wwdt_mspm0_config * config = dev->config;
    DL_WWDT_restart((WWDT_Regs *) config->base);
    return 0;
}

static const struct wdt_driver_api wwdt_mspm0_driver_api = {
    .setup = wwdt_mspm0_setup,
    .disable = wwdt_mspm0_disable,
    .install_timeout = wwdt_mspm0_install_timeout,
    .feed = wwdt_mspm0_feed
};

#define MSP_WDT_INIT_FN(index)                                                 \
                                                                               \
static const struct wwdt_mspm0_config wwdt_mspm0_cfg_##index = {                \
    .base =  DT_INST_REG_ADDR(index),                                          \
    .reset_action = COND_CODE_1(DT_INST_PROP(index, ti_watchdog_reset_action), \
                                WDT_FLAG_RESET_SOC, WDT_FLAG_RESET_CPU_CORE),  \
};                                                                             \
                                                                               \
static struct wwdt_mspm0_data wwdt_mspm0_data_##index;                          \
                                                                               \
DEVICE_DT_INST_DEFINE(index, wwdt_mspm0_init, NULL, &wwdt_mspm0_data_##index,  \
                      &wwdt_mspm0_cfg_##index, POST_KERNEL,                    \
                      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                      \
                      &wwdt_mspm0_driver_api);                                 \

DT_INST_FOREACH_STATUS_OKAY(MSP_WDT_INIT_FN)