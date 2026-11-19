/*
 * Copyright (c) 2026 Nainaiu Rakhaine
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MAXIM_MAX30003_H_
#define ZEPHYR_DRIVERS_SENSOR_MAXIM_MAX30003_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/max30003.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Register map (24-bit registers). */
#define MAX30003_REG_NO_OP          0x00
#define MAX30003_REG_STATUS         0x01
#define MAX30003_REG_EN_INT         0x02
#define MAX30003_REG_EN_INT2        0x03
#define MAX30003_REG_MNGR_INT       0x04
#define MAX30003_REG_MNGR_DYN       0x05
#define MAX30003_REG_SW_RST         0x08
#define MAX30003_REG_SYNCH          0x09
#define MAX30003_REG_FIFO_RST       0x0A
#define MAX30003_REG_INFO           0x0F
#define MAX30003_REG_CNFG_GEN       0x10
#define MAX30003_REG_CNFG_CAL       0x12
#define MAX30003_REG_CNFG_EMUX      0x14
#define MAX30003_REG_CNFG_ECG       0x15
#define MAX30003_REG_CNFG_RTOR1     0x1D
#define MAX30003_REG_CNFG_RTOR2     0x1E
#define MAX30003_REG_ECG_FIFO_BURST 0x20
#define MAX30003_REG_ECG_FIFO       0x21
#define MAX30003_REG_RTOR           0x25

/* STATUS and EN_INT share bit positions (public MAX30003_STATUS_*). */
#define MAX30003_INT_EINT      MAX30003_STATUS_EINT
#define MAX30003_INT_EOVF      MAX30003_STATUS_EOVF
#define MAX30003_INT_FSTINT    MAX30003_STATUS_FSTINT
#define MAX30003_INT_DCLOFFINT MAX30003_STATUS_DCLOFFINT
#define MAX30003_INT_LONINT    MAX30003_STATUS_LONINT
#define MAX30003_INT_RRINT     MAX30003_STATUS_RRINT
#define MAX30003_INT_SAMP      MAX30003_STATUS_SAMP
#define MAX30003_INT_PLLINT    MAX30003_STATUS_PLLINT
#define MAX30003_STATUS_LDOFF  GENMASK(3, 0)

/* Conditions that stay asserted while they last: masked after they fire. */
#define MAX30003_INT_LEVEL                                                                         \
	(MAX30003_INT_FSTINT | MAX30003_INT_DCLOFFINT | MAX30003_INT_LONINT | MAX30003_INT_PLLINT)

#define MAX30003_EN_INT_INTB_TYPE GENMASK(1, 0)
#define MAX30003_INTB_OD_PULLUP 3 /* open drain with internal 125 kOhm pull-up */

/* MNGR_INT */
#define MAX30003_MNGR_INT_EFIT      GENMASK(23, 19)
#define MAX30003_MNGR_INT_CLR_FAST  BIT(6)
#define MAX30003_MNGR_INT_CLR_RRINT GENMASK(5, 4)
#define MAX30003_MNGR_INT_CLR_SAMP  BIT(2)
#define MAX30003_MNGR_INT_SAMP_IT   GENMASK(1, 0)

/* MNGR_DYN */
#define MAX30003_MNGR_DYN_FAST     GENMASK(23, 22)
#define MAX30003_MNGR_DYN_FAST_TH  GENMASK(21, 16)

/* CNFG_GEN */
#define MAX30003_GEN_EN_ULP_LON    GENMASK(23, 22)
#define MAX30003_GEN_FMSTR         GENMASK(21, 20)
#define MAX30003_GEN_EN_ECG        BIT(19)
#define MAX30003_GEN_EN_DCLOFF     GENMASK(13, 12)
#define MAX30003_GEN_DCLOFF_IPOL   BIT(11)
#define MAX30003_GEN_DCLOFF_IMAG   GENMASK(10, 8)
#define MAX30003_GEN_DCLOFF_VTH    GENMASK(7, 6)
#define MAX30003_GEN_EN_RBIAS      GENMASK(5, 4)
#define MAX30003_GEN_RBIASV        GENMASK(3, 2)
#define MAX30003_GEN_RBIASP        BIT(1)
#define MAX30003_GEN_RBIASN        BIT(0)

/* CNFG_CAL */
#define MAX30003_CAL_EN_VCAL       BIT(22)
#define MAX30003_CAL_VMODE         BIT(21)
#define MAX30003_CAL_VMAG          BIT(20)
#define MAX30003_CAL_FCAL          GENMASK(14, 12)
#define MAX30003_CAL_FIFTY         BIT(11)
#define MAX30003_CAL_THIGH         GENMASK(10, 0)

/* CNFG_EMUX */
#define MAX30003_EMUX_POL          BIT(23)
#define MAX30003_EMUX_OPENP        BIT(21)
#define MAX30003_EMUX_OPENN        BIT(20)
#define MAX30003_EMUX_CALP_SEL     GENMASK(19, 18)
#define MAX30003_EMUX_CALN_SEL     GENMASK(17, 16)
#define MAX30003_EMUX_CAL_SOURCES  (MAX30003_EMUX_OPENP | MAX30003_EMUX_OPENN |       \
				    FIELD_PREP(MAX30003_EMUX_CALP_SEL, 2) |            \
				    FIELD_PREP(MAX30003_EMUX_CALN_SEL, 3))

/* CNFG_ECG */
#define MAX30003_ECG_RATE          GENMASK(23, 22)
#define MAX30003_ECG_GAIN          GENMASK(17, 16)
#define MAX30003_ECG_DHPF          BIT(14)
#define MAX30003_ECG_DLPF          GENMASK(13, 12)

/* CNFG_RTOR1 */
#define MAX30003_RTOR1_WNDW        GENMASK(23, 20)
#define MAX30003_RTOR1_GAIN        GENMASK(19, 16)
#define MAX30003_RTOR1_EN          BIT(15)
#define MAX30003_RTOR1_PAVG        GENMASK(13, 12)
#define MAX30003_RTOR1_PTSF        GENMASK(11, 8)

/* CNFG_RTOR2 */
#define MAX30003_RTOR2_HOFF        GENMASK(21, 16)
#define MAX30003_RTOR2_RAVG        GENMASK(13, 12)
#define MAX30003_RTOR2_RHSF        GENMASK(10, 8)

#define MAX30003_INFO_ID_MASK      GENMASK(23, 20)
#define MAX30003_INFO_ID           0x5

/* Register values derived from a validated settings set. */
struct max30003_regs {
	uint32_t gen;
	uint32_t ecg;
	uint32_t emux;
	uint32_t rtor1;
	uint32_t rtor2;
	uint32_t mngr_int;
	uint32_t mngr_dyn;
	uint8_t rate_idx;
};

struct max30003_config {
	struct spi_dt_spec spi;
#ifdef CONFIG_MAX30003_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
	struct pwm_dt_spec fclk_pwm;
	bool has_fclk_pwm;
	uint32_t fclk_hz;
	struct max30003_settings defaults;
};

struct max30003_data {
	struct k_mutex lock;
	struct max30003_settings cur;
	uint8_t rate_idx;
	/* Calibration source state (cal valid while calibrating). */
	bool calibrating;
	struct max30003_calibration cal;
	bool standby;
	/* Newest sample and R-to-R result. */
	int32_t raw;
	bool have_sample;
	uint32_t rr_us;
	bool have_rr;
	uint8_t lead_off;
	/* STATUS flags consumed by the interrupt handler, not yet read by the app. */
	uint32_t status_cache;
#ifdef CONFIG_MAX30003_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work_delayable work;
	uint32_t trig_enable;  /* STATUS bits with a trigger handler */
	uint32_t level_masked; /* level bits masked until the condition clears */
	sensor_trigger_handler_t handler[7];
	const struct sensor_trigger *trigger[7];
#endif
};

int max30003_reg_read(const struct device *dev, uint8_t reg, uint32_t *val);
int max30003_reg_write(const struct device *dev, uint8_t reg, uint32_t val);
int max30003_handle_overflow(const struct device *dev);
/* Read RTOR and update the R-to-R result. Caller holds the lock. */
int max30003_update_rr_locked(const struct device *dev);

#ifdef CONFIG_MAX30003_TRIGGER
int max30003_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
int max30003_init_interrupt(const struct device *dev);
/* Rewrite EN_INT from the trigger state. Caller holds the lock. */
int max30003_write_int_enable_locked(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_MAXIM_MAX30003_H_ */
