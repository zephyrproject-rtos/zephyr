/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_ESP32_PRIV_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_ESP32_PRIV_H_

#define CPU_RESET_REASON RTC_SW_CPU_RESET

#if defined(CONFIG_SOC_SERIES_ESP32)
#define DT_CPU_COMPAT espressif_xtensa_lx6
#undef CPU_RESET_REASON
#define CPU_RESET_REASON SW_CPU_RESET
#include <zephyr/dt-bindings/clock/esp32_clock.h>
#include <esp32/rom/rtc.h>
#include <soc/dport_reg.h>
#include <soc/i2s_reg.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#define DT_CPU_COMPAT espressif_xtensa_lx7
#include <zephyr/dt-bindings/clock/esp32s2_clock.h>
#include <esp32s2/rom/rtc.h>
#include <soc/dport_reg.h>
#include <soc/i2s_reg.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
#define DT_CPU_COMPAT espressif_xtensa_lx7
#include <zephyr/dt-bindings/clock/esp32s3_clock.h>
#include <esp32s3/rom/rtc.h>
#include <soc/dport_reg.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C2)
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32c2_clock.h>
#include <esp32c2/rom/rtc.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C3)
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32c3_clock.h>
#include <esp32c3/rom/rtc.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C5)
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32c5_clock.h>
#include <soc/lp_clkrst_reg.h>
#include <soc/pmu_reg.h>
#include <soc/regi2c_dig_reg.h>
#include <regi2c_ctrl.h>
#include <esp32c5/rom/rtc.h>
#include <soc/dport_access.h>
#include <hal/clk_tree_ll.h>
#include <esp_private/esp_pmu.h>
#include <modem/modem_syscon_struct.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C6)
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32c6_clock.h>
#include <soc/lp_clkrst_reg.h>
#include <soc/pmu_reg.h>
#include <soc/regi2c_dig_reg.h>
#include <regi2c_ctrl.h>
#include <esp32c6/rom/rtc.h>
#include <soc/dport_access.h>
#include <hal/clk_tree_ll.h>
#include <esp_private/esp_pmu.h>
#include <modem/modem_lpcon_struct.h>
#elif defined(CONFIG_SOC_SERIES_ESP32H2)
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32h2_clock.h>
#include <soc/lp_clkrst_reg.h>
#include <regi2c_ctrl.h>
#include <esp32h2/rom/rtc.h>
#include <soc/dport_access.h>
#include <hal/clk_tree_ll.h>
#include <esp_private/esp_pmu.h>
#elif defined(CONFIG_SOC_SERIES_ESP32P4)
#define DT_CPU_COMPAT espressif_riscv
#include <zephyr/dt-bindings/clock/esp32p4_clock.h>
#include <soc/lp_clkrst_reg.h>
#include <soc/pmu_reg.h>
#include <soc/regi2c_dig_reg.h>
#include <regi2c_ctrl.h>
#include <esp32p4/rom/rtc.h>
#include <soc/dport_access.h>
#include <hal/clk_tree_ll.h>
#include <esp_private/esp_pmu.h>
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>

#include <esp_cpu.h>
#include <esp_private/esp_clk.h>
#include <esp_private/esp_clk_tree_common.h>
#include <esp_private/periph_ctrl.h>
#include <esp_private/rtc_clk.h>
#include <esp_rom_caps.h>
#include <esp_rom_serial_output.h>
#include <esp_rom_sys.h>
#include <hal/clk_gate_ll.h>
#include <hal/clk_tree_hal.h>
#include <hal/regi2c_ctrl_ll.h>
#include <hal/timg_ll.h>
#include <hal/uart_ll.h>
#include <soc/periph_defs.h>
#include <soc/rtc.h>
#include "esp_clk_internal.h"

/*
 * Per-family clock implementations. The RTC_CNTL variant covers the SoCs
 * whose slow/CPU clocks are driven through the RTC_CNTL peripheral
 * (ESP32, ESP32-S2, ESP32-S3, ESP32-C2, ESP32-C3). The PMU variant covers
 * the SoCs that use the PMU/LP_CLKRST peripherals (ESP32-C5, ESP32-C6,
 * ESP32-H2, ESP32-P4). Exactly one implementation file is compiled per
 * build, selected from CMakeLists.txt by SoC series.
 */
int esp32_select_rtc_slow_clk(uint8_t slow_clk);

int esp32_cpu_clock_configure(const struct esp32_cpu_clock_config *cpu_cfg);

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_ESP32_PRIV_H_ */
