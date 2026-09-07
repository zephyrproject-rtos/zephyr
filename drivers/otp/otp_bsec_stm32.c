/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/toolchain.h>

#define DT_DRV_COMPAT st_stm32_bsec

LOG_MODULE_REGISTER(otp_bsec_stm32, CONFIG_OTP_LOG_LEVEL);

#define BSEC_WORD_SIZE	4

static K_MUTEX_DEFINE(lock);

struct bsec_stm32_config {
	BSEC_TypeDef *base;
	unsigned int upper_fuse_limit;
};

static inline void otp_bsec_stm32_lock(void)
{
	if (!k_is_pre_kernel()) {
		(void)k_mutex_lock(&lock, K_FOREVER);
	}
}

static inline void otp_bsec_stm32_unlock(void)
{
	if (!k_is_pre_kernel()) {
		(void)k_mutex_unlock(&lock);
	}
}

#if defined(CONFIG_SOC_SERIES_STM32MP13X)
/*
 * The STM32MP13 HAL exposes a different BSEC API than the other series: the
 * SAFMEM fuse array has to be powered up before fuses can be read from it or
 * programmed, and reading a fuse loads it into its shadow register on the way.
 * The upper fuses can be accessed from the secure world only, which is where
 * Zephyr runs as the first stage boot loader, whatever the life cycle state,
 * so the upper fuse limit is not checked here. HAL_BSEC_GetSecurityStatus()
 * could not report a closed state anyway: it masks the BSEC_OTP_STATUS bits
 * down to SECURE/FULLDBG/INVALID before testing the CLOSED bit.
 */
#define BSEC_HANDLE_INIT(cfg)                                                                      \
	{                                                                                          \
		.Instance = (cfg)->base, .State = BSEC_STATE_READY                                 \
	}

static const struct device *const bsec_clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
static const struct stm32_pclken bsec_pclken = STM32_DT_INST_CLOCK_INFO(0);

static int otp_bsec_stm32_safmem_on(BSEC_HandleTypeDef *handle)
{
	BSEC_SafMemClkRangeTypeDef clk_range;
	uint32_t rate;
	int ret;

	/* SAFMEM is clocked by the BSEC bus clock, whose range has to be told to the fuse array */
	ret = clock_control_get_rate(bsec_clock, (clock_control_subsys_t)&bsec_pclken, &rate);
	if (ret != 0) {
		LOG_ERR("Could not get the BSEC clock rate: %d", ret);
		return ret;
	}

	if (!IN_RANGE(rate, MHZ(10), MHZ(67))) {
		LOG_ERR("BSEC clock rate %u Hz outside of the SAFMEM range", rate);
		return -ENOTSUP;
	}

	if (rate <= MHZ(20)) {
		clk_range = BSEC_SAFMEM_CLK_RANGE_10MHZ_20MHZ;
	} else if (rate <= MHZ(30)) {
		clk_range = BSEC_SAFMEM_CLK_RANGE_20MHZ_30MHZ;
	} else if (rate <= MHZ(45)) {
		clk_range = BSEC_SAFMEM_CLK_RANGE_30MHZ_45MHZ;
	} else {
		clk_range = BSEC_SAFMEM_CLK_RANGE_45MHZ_67MHZ;
	}

	if (HAL_BSEC_SafMemPwrUp(handle, clk_range) != HAL_OK) {
		return -EIO;
	}

	return 0;
}

static void otp_bsec_stm32_safmem_off(BSEC_HandleTypeDef *handle)
{
	HAL_BSEC_SafMemPwrDown(handle);
}

static HAL_StatusTypeDef hal_bsec_otp_read(BSEC_HandleTypeDef *handle, uint32_t fuse_idx,
					   uint32_t *fuse_data)
{
	return HAL_BSEC_OtpRead(handle, fuse_idx, fuse_data);
}

#if defined(CONFIG_OTP_PROGRAM)
static HAL_StatusTypeDef hal_bsec_otp_program(BSEC_HandleTypeDef *handle, uint32_t fuse_idx,
					      uint32_t fuse_data)
{
	return HAL_BSEC_OtpProgram(handle, fuse_idx, fuse_data);
}
#endif /* CONFIG_OTP_PROGRAM */

static int otp_bsec_stm32_check_accessible(BSEC_HandleTypeDef *handle,
					   const struct bsec_stm32_config *config __unused,
					   off_t offset, unsigned int nb_fuse)
{
	uint32_t fuse_idx = offset / BSEC_WORD_SIZE;
	BSEC_ChipSecurityTypeDef bsec_state;

	if ((nb_fuse == 0) || ((fuse_idx + nb_fuse) > HAL_BSEC_OTP_NB_WORDS)) {
		return -EINVAL;
	}

	if ((HAL_BSEC_GetSecurityStatus(handle, &bsec_state) != HAL_OK) ||
	    (bsec_state == BSEC_INVALID_STATE)) {
		return -EACCES;
	}

	return 0;
}

static int otp_bsec_stm32_init(const struct device *dev __unused)
{
	return clock_control_on(bsec_clock, (clock_control_subsys_t)&bsec_pclken);
}
#else /* CONFIG_SOC_SERIES_STM32MP13X */
#define BSEC_HANDLE_INIT(cfg)                                                                      \
	{                                                                                          \
		.Instance = (cfg)->base                                                            \
	}

static int otp_bsec_stm32_safmem_on(BSEC_HandleTypeDef *handle __unused)
{
	return 0;
}

static void otp_bsec_stm32_safmem_off(BSEC_HandleTypeDef *handle __unused)
{
}

static HAL_StatusTypeDef hal_bsec_otp_read(BSEC_HandleTypeDef *handle, uint32_t fuse_idx,
					   uint32_t *fuse_data)
{
	return HAL_BSEC_OTP_Read(handle, fuse_idx, fuse_data);
}

#if defined(CONFIG_OTP_PROGRAM)
static HAL_StatusTypeDef hal_bsec_otp_program(BSEC_HandleTypeDef *handle, uint32_t fuse_idx,
					      uint32_t fuse_data)
{
	return HAL_BSEC_OTP_Program(handle, fuse_idx, fuse_data, 0);
}
#endif /* CONFIG_OTP_PROGRAM */

static int otp_bsec_stm32_check_accessible(BSEC_HandleTypeDef *handle,
					   const struct bsec_stm32_config *config, off_t offset,
					   unsigned int nb_fuse)
{
	uint32_t fuse_idx = offset / BSEC_WORD_SIZE;
	HAL_StatusTypeDef hal_ret;
	uint32_t bsec_state = 0;

	if (nb_fuse == 0) {
		return -EINVAL;
	}

	hal_ret = HAL_BSEC_GetDeviceLifeCycleState(handle, &bsec_state);
	if (hal_ret != HAL_OK) {
		return -EACCES;
	}

	/* Upper fuses are only accessible when the BSEC is in closed locked state */
	if (((fuse_idx + nb_fuse) > config->upper_fuse_limit) &&
	    (bsec_state != HAL_BSEC_CLOSED_STATE)) {
		return -EACCES;
	}

	return 0;
}

#define otp_bsec_stm32_init NULL
#endif /* CONFIG_SOC_SERIES_STM32MP13X */

#if defined(CONFIG_OTP_PROGRAM)
static int otp_bsec_stm32_program(const struct device *dev, off_t offset, const void *buf,
				  size_t len)
{
	const struct bsec_stm32_config *config = dev->config;
	BSEC_HandleTypeDef handle = BSEC_HANDLE_INIT(config);
	HAL_StatusTypeDef hal_ret;
	unsigned int nb_fuse;
	unsigned int i;
	int ret;

	/* Allow programming of 4bytes words only */
	if (!IS_ALIGNED(len, BSEC_WORD_SIZE)) {
		LOG_ERR("Invalid length to program OTP: %zu", len);
		return -EINVAL;
	}

	/* Allow programming only at the beginning of a new word */
	if (!IS_ALIGNED(offset, BSEC_WORD_SIZE)) {
		LOG_ERR("Programmed data not aligned on an OTP word");
		return -EINVAL;
	}

	nb_fuse = len / BSEC_WORD_SIZE;

	ret = otp_bsec_stm32_check_accessible(&handle, config, offset, nb_fuse);
	if (ret != 0) {
		return ret;
	}

	otp_bsec_stm32_lock();

	ret = otp_bsec_stm32_safmem_on(&handle);
	if (ret != 0) {
		otp_bsec_stm32_unlock();
		return ret;
	}

	for (i = 0; i < nb_fuse; i++) {
		uint32_t prog_data = 0;

		LOG_DBG("Programming Fuse %lu", (offset / BSEC_WORD_SIZE) + i);

		prog_data = UNALIGNED_GET((uint32_t *)((uint8_t *)buf + i * BSEC_WORD_SIZE));

		hal_ret = hal_bsec_otp_program(&handle, (offset / BSEC_WORD_SIZE) + i, prog_data);
		if (hal_ret != HAL_OK) {
			ret = -EACCES;
			break;
		}
	}

	otp_bsec_stm32_safmem_off(&handle);
	otp_bsec_stm32_unlock();

	return ret;
}
#endif /* CONFIG_OTP_PROGRAM */

static int otp_bsec_stm32_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct bsec_stm32_config *config = dev->config;
	BSEC_HandleTypeDef handle = BSEC_HANDLE_INIT(config);
	uint8_t *dest = (uint8_t *)buf;
	HAL_StatusTypeDef hal_ret;
	size_t bytes_left = len;
	unsigned int nb_fuse;
	unsigned int i;
	int ret;

	/* Allow intra-word and spanned reads but not 0-sized reads */
	nb_fuse = len != 0 ? DIV_ROUND_UP(offset % BSEC_WORD_SIZE + len, BSEC_WORD_SIZE) : 0;

	ret = otp_bsec_stm32_check_accessible(&handle, config, offset, nb_fuse);
	if (ret != 0) {
		return ret;
	}

	otp_bsec_stm32_lock();

	ret = otp_bsec_stm32_safmem_on(&handle);
	if (ret != 0) {
		otp_bsec_stm32_unlock();
		return ret;
	}

	for (i = 0; i < nb_fuse; i++) {
		size_t first_offset  = (i == 0) ? offset % BSEC_WORD_SIZE : 0;
		size_t read_sz = MIN(BSEC_WORD_SIZE - first_offset, bytes_left);
		uint32_t fuse_data = 0;

		LOG_DBG("Reading Fuse %lu", (offset / BSEC_WORD_SIZE) + i);

		hal_ret = hal_bsec_otp_read(&handle, (offset / BSEC_WORD_SIZE) + i, &fuse_data);
		if (hal_ret != HAL_OK) {
			ret = -EACCES;
			break;
		}

		memcpy(dest, ((uint8_t *)&fuse_data) + first_offset, read_sz);
		dest += read_sz;
		bytes_left -= read_sz;
		if (bytes_left == 0) {
			break;
		}
	}

	otp_bsec_stm32_safmem_off(&handle);
	otp_bsec_stm32_unlock();

	return ret;
}

static const struct bsec_stm32_config bsec_config = {
	.base = (void *)DT_INST_REG_ADDR(0),
	.upper_fuse_limit = DT_INST_PROP(0, st_upper_fuse_limit),
};

static DEVICE_API(otp, otp_bsec_stm32_api) = {
#if defined(CONFIG_OTP_PROGRAM)
	.program = otp_bsec_stm32_program,
#endif
	.read = otp_bsec_stm32_read,
};

DEVICE_DT_INST_DEFINE(0, otp_bsec_stm32_init, NULL, NULL, &bsec_config, PRE_KERNEL_1,
		      CONFIG_OTP_INIT_PRIORITY, &otp_bsec_stm32_api);
