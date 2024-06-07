/* DTI Process, Voltage and Thermal Sensor Driver
 *
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_DTI_PVT_H_
#define ZEPHYR_DRIVERS_DTI_PVT_H_

#include <zephyr/sys/util.h>

struct dti_pvt_reg {
  // PVT CTRL
  volatile uint32_t req;           // 0x00
  volatile uint32_t treg;          // 0x04
  volatile uint32_t conf;          // 0x08
  volatile uint32_t test;          // 0x0C
  volatile const uint32_t result;  // 0x10
  volatile const uint32_t stt;     // 0x14
};

// =====================================
// req fields : PVT Request Register
// ====================================
//! are these fields self-cleared when we get a DONE signal?

enum dti_pvt_request {
  DTI_PVT_REQUEST_PROCESS_MONITOR = 0x1,
  DTI_PVT_REQUEST_VOLTAGE_MONITOR = 0x2,
  DTI_PVT_REQUEST_THERMAL_SENSOR_MONITOR = 0x4
};

// ========================================
// treg fields: PVT Timing config register
// ========================================

// Timeout for process monitor
#define DTI_PVT_OFFSET_TREG_TIMEOUT 0
#define DTI_PVT_MASK_TREG_TIMEOUT 0xFF

// Enable time before starting the monitor/sensor, RoundUp(100ns * CLK)
#define DTI_PVT_OFFSET_TREG_EN 8
#define DTI_PVT_MASK_TREG_EN 0x1F00

// Measuring time, RoundUp(30ns * CLK)
#define DTI_PVT_OFFSET_TREG_MSTEP 13
#define DTI_PVT_MASK_TREG_MSTEP 0xE000

// =====================================
// conf fields:  PVT config register
// ====================================

// Resistor value adjustment for post silicon to calibrate to simulation results
#define DTI_PVT_OFFSET_CONF_TRIM 0
#define DTI_PVT_MASK_CONF_TRIM 0xF

// Clock Frquency range
#define DTI_PVT_OFFSET_CONF_FREQRANGE 4
#define DTI_PVT_MASK_CONF_FREQRANGE 0x10

enum dti_pvt_conf_freqrange {
  DTI_PVT_CONF_FREQRANGE_200M_75M = 0,
  DTI_PVT_CONF_FREQRANGE_UNDER_75M = 1
};

// Division factor
#define DTI_PVT_OFFSET_CONF_DIV 5
#define DTI_PVT_MASK_CONF_DIV 0x1E0

// Division factor
// DIV = 1200/Freq_Mhz -1 if freq range = 0
// DIV = 600/Freq_Mhz -1 if freq range = 1
enum dti_pvt_conf_div {
  DTI_PVT_CONF_DIV_FREQ_200M = 5,
  DTI_PVT_CONF_DIV_FREQ_150M = 7,
  DTI_PVT_CONF_DIV_FREQ_133M = 8,
  DTI_PVT_CONF_DIV_FREQ_100M_OR_50M = 11,
  DTI_PVT_CONF_DIV_FREQ_75M = 15
};

#define DTI_PVT_OFFSET_CONF_VMRANGE 9
#define DTI_PVT_MASK_CONF_VMRANGE 0x200

// Voltage calibration offset in signed two's complement
#define DTI_PVT_OFFSET_CONF_VMCAL_OFFSET 10
#define DTI_PVT_MASK_CONF_VMCAL_OFFSET 0x7C00

// Thermal calibration offset in signed two's complement
#define DTI_PVT_OFFSET_CONF_TSCAL_OFFSET 15
#define DTI_PVT_MASK_CONF_TSCAL_OFFSET 0xF8000

// =====================================
// test fields: Register for PVT tests
// ====================================

enum dti_pvt_test {
  DTI_PVT_TEST_BADGAP_REF_ON_TSTOUT = 0x1,
  DTI_PVT_TEST_DAC_VOLTAGE_THERMAL_SENSOR = 0x2,
  DTI_PVT_TEST_DAC_VOLTAGE_OF_VOLTAGE_MONITOR = 0x4,
  DTI_PVT_TEST_REGULATED_VOLTAGE_OF_PROCESS_MON = 0x8
};

// =======================================
// result fields: PVT measurement results
// ========================================

#define DTI_PVT_OFFSET_RESULT_PM_DIFF 0
#define DTI_PVT_MASK_RESULT_PM_DIFF 0x3FF
#define DTI_PVT_RST_RESULT_PM_DIFF 0x0

#define DTI_PVT_OFFSET_RESULT_PM_FAST 10
#define DTI_PVT_MASK_RESULT_PM_FAST 0x400
#define DTI_PVT_RST_RESULT_PM_FAST 0x0

enum dti_pvt_result_pm_fast {
  dti_pvt_result_pm_fast_slower = 0,
  dti_pvt_result_pm_fast_faster = 1
};

#define DTI_PVT_OFFSET_RESULT_PM_DONE 11
#define DTI_PVT_MASK_RESULT_PM_DONE 0x800
#define DTI_PVT_RST_RESULT_PM_DONE 0x0

#define DTI_PVT_OFFSET_RESULT_VM_C 12
#define DTI_PVT_MASK_RESULT_VM_C 0x1FF000
#define DTI_PVT_RST_RESULT_VM_C 0x0

#define DTI_PVT_OFFSET_RESULT_VM_DONE 21
#define DTI_PVT_MASK_RESULT_VM_DONE 0x200000
#define DTI_PVT_RST_RESULT_VM_DONE 0x0

#define DTI_PVT_OFFSET_RESULT_TS_C 22
#define DTI_PVT_MASK_RESULT_TS_C 0x7FC00000
#define DTI_PVT_RST_RESULT_TS_C 0x0

#define DTI_PVT_OFFSET_RESULT_TS_DONE 31
#define DTI_PVT_MASK_RESULT_TS_DONE 0x80000000
#define DTI_PVT_RST_RESULT_TS_DONE 0x0

// =====================================
// stt fields - monitor status
// ====================================
//! are these fields self-cleared when we get a DONE signal?

#define DTI_PVT_OFFSET_STT_PM_ERROR 0
#define DTI_PVT_MASK_STT_PM_ERROR 0x1
#define DTI_PVT_RST_STT_PM_ERROR 0x0

#define DTI_PVT_OFFSET_STT_VM_ERROR 1
#define DTI_PVT_MASK_STT_VM_ERROR 0x2
#define DTI_PVT_RST_STT_VM_ERROR 0x0

#define DTI_PVT_OFFSET_STT_TS_ERROR 2
#define DTI_PVT_MASK_STT_TS_ERROR 0x4
#define DTI_PVT_RST_STT_TS_ERROR 0x0

#define DTI_PVT_OFFSET_STT_REQ_READY 3
#define DTI_PVT_MASK_STT_REQ_READY 0x8
#define DTI_PVT_RST_STT_REQ_READY 0x1

enum dti_pvt_stt {
  DTI_PVT_STT_PROCESS_MONITOR_ERROR = 0x1,
  DTI_PVT_STT_VOLTAGE_MONITOR_ERROR = 0x2,
  DTI_PVT_STT_THERMAL_SENSOR_MONITOR_ERROR = 0x4,
  DTI_PVT_STT_REQ_READY = 0x8
};

//------------------ API Related -------------------//

struct dti_pvt_results {
  enum dti_pvt_result_pm_fast process_status;
  uint16_t process_diff_percentage;
  int16_t voltage;
  int16_t temperature;
  uint16_t error_flags;
};

struct dti_pvt_config {
  uint32_t clk_mhz;
};

struct dti_pvt_data {
  struct dti_pvt_reg *pvt_regs;
  struct dti_pvt_results results;
};

#endif /* ZEPHYR_DRIVERS_DTI_PVT_H_ */
