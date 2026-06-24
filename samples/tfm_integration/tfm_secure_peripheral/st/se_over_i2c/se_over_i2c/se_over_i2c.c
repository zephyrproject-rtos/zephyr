/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <psa/service.h>
#include <psa_manifest/tfm_se_over_i2c.h>
#include <stm32u5xx_hal.h>
#include <tfm_log.h>
#include <stdint.h>

#if TFM_ISOLATION_LEVEL != 1
#error "Partition implementation requires GTZC access, hence require TFM_ISOLATION_LEVEL 1"
#endif

/**
 * b_u585i_iot02a board revisions up to C02 wires GPIO pin F11 to STSAFE Reset line (active low).
 * The signal F11 needs to be driven to 1 to enable the STSAFE.
 * From revision D01, GPIO pin F11 is connected to the STSAFE Power-Enable (active low), which
 *  will naturally drive the STSAFE Reset line to 1. So the signal F11 needs to be driven to 0
 * to enable the STSAFE.
 * Set B_U585I_IOT02A_REV value arccording to your hardware.
 */
#define REV_D01 1
#define REV_C02 2
#define B_U585I_IOT02A_REV REV_D01

#define STSAFE_EN_PORT GPIOF
#define STSAFE_EN_PIN  GPIO_PIN_11
#define I2C2_SCL_PIN   GPIO_PIN_4
#define I2C2_SDA_PIN   GPIO_PIN_5
#define STSAFE_ADDR    0x20U
#define I2C2_TIMING    0x30909DECU /* 100kHz */

#define TIM6_TICK_HZ 10000U

static I2C_HandleTypeDef hi2c2;

struct i2c_probe_result {
	uint8_t hal_status;
	uint8_t isr_lo;
	uint8_t isr_hi;
};

static void timer_delay_init(void)
{
	RCC_ClkInitTypeDef clk = {0};
	uint32_t flash_latency;
	uint32_t timclk;

	__HAL_RCC_TIM6_CLK_ENABLE();
	HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_TIM6,
					     GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_NPRIV);

	HAL_RCC_GetClockConfig(&clk, &flash_latency);
	timclk = HAL_RCC_GetPCLK1Freq();
	if (clk.APB1CLKDivider != RCC_HCLK_DIV1) {
		timclk *= 2U;
	}

	TIM6->CR1 = 0;
	TIM6->PSC = (uint16_t)((timclk / TIM6_TICK_HZ) - 1U);
	TIM6->EGR = TIM_EGR_UG;
	TIM6->SR = 0;
}

static void delay_ms(uint32_t ms)
{
	uint32_t ticks = ms * (TIM6_TICK_HZ / 1000U);

	if (ticks == 0U) {
		return;
	}
	if (ticks > 0xFFFFU) {
		ticks = 0xFFFFU;
	}

	TIM6->CR1 = 0;
	TIM6->CNT = 0;
	TIM6->ARR = (uint16_t)ticks;

	TIM6->EGR = TIM_EGR_UG;
	__DSB();
	TIM6->SR = 0;
	__DSB();

	while (TIM6->SR & TIM_SR_UIF) {
		TIM6->SR = 0;
	}

	TIM6->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;

	while (!(TIM6->SR & TIM_SR_UIF)) {
		__NOP();
	}
	TIM6->SR = 0;
}

static void stsafe_power_init(void)
{
	GPIO_InitTypeDef gpio = {0};

	__HAL_RCC_GPIOF_CLK_ENABLE();
	HAL_GPIO_ConfigPinAttributes(STSAFE_EN_PORT, STSAFE_EN_PIN, GPIO_PIN_SEC);

	gpio.Pin = STSAFE_EN_PIN;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(STSAFE_EN_PORT, &gpio);

#if B_U585I_IOT02A_REV == REV_D01
	HAL_GPIO_WritePin(STSAFE_EN_PORT, STSAFE_EN_PIN, GPIO_PIN_RESET);
#elif B_U585I_IOT02A_REV == REV_C02
	HAL_GPIO_WritePin(STSAFE_EN_PORT, STSAFE_EN_PIN, GPIO_PIN_SET);
#endif
	INFO("STSAFE powered (EN low)\n");

	delay_ms(50);
}

static void i2c2_init(void)
{
	RCC_PeriphCLKInitTypeDef pclk = {0};
	GPIO_InitTypeDef gpio = {0};

	pclk.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
	pclk.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
	HAL_RCCEx_PeriphCLKConfig(&pclk);

	__HAL_RCC_I2C2_CLK_ENABLE();
	HAL_GTZC_TZSC_ConfigPeriphAttributes(GTZC_PERIPH_I2C2,
					     GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_NPRIV);

	__HAL_RCC_GPIOH_CLK_ENABLE();
	HAL_GPIO_ConfigPinAttributes(GPIOH, I2C2_SCL_PIN | I2C2_SDA_PIN, GPIO_PIN_SEC);

	gpio.Pin = I2C2_SCL_PIN | I2C2_SDA_PIN;
	gpio.Mode = GPIO_MODE_AF_OD;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	gpio.Alternate = GPIO_AF4_I2C2;
	HAL_GPIO_Init(GPIOH, &gpio);

	hi2c2.Instance = I2C2;
	hi2c2.Init.Timing = I2C2_TIMING;
	hi2c2.Init.OwnAddress1 = 0;
	hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.OwnAddress2 = 0;
	hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	HAL_I2C_Init(&hi2c2);

	INFO("I2C2 initialized\n");
}

static psa_status_t handle_i2c_probe(psa_msg_t *msg)
{
	struct i2c_probe_result res = {0};

	if (msg->out_size[0] < sizeof(res)) {
		INFO("Output buffer too small\n");
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	res.hal_status = (uint8_t)HAL_I2C_IsDeviceReady(&hi2c2, (STSAFE_ADDR << 1), 2, 100);

	res.isr_lo = (uint8_t)(I2C2->ISR & 0xFFU);
	res.isr_hi = (uint8_t)((I2C2->ISR >> 8) & 0xFFU);

	psa_write(msg->handle, 0, &res, sizeof(res));
	return PSA_SUCCESS;
}

static psa_status_t handle_i2c_echo(psa_msg_t *msg)
{
	uint8_t tx_buf[32];
	uint8_t rx_buf[32];
	size_t tx_len = msg->in_size[0];
	size_t rx_len = msg->out_size[0];

	if (tx_len > sizeof(tx_buf) || rx_len > sizeof(rx_buf)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (tx_len > 0) {
		psa_read(msg->handle, 0, tx_buf, tx_len);

		if (HAL_I2C_Master_Transmit(&hi2c2, (STSAFE_ADDR << 1), tx_buf, tx_len, 100) !=
		    HAL_OK) {
			return PSA_ERROR_HARDWARE_FAILURE;
		}
	}

	delay_ms(50);

	if (rx_len > 0) {
		if (HAL_I2C_Master_Receive(&hi2c2, (STSAFE_ADDR << 1), rx_buf, rx_len, 100) !=
		    HAL_OK) {
			return PSA_ERROR_HARDWARE_FAILURE;
		}

		psa_write(msg->handle, 0, rx_buf, rx_len);
	}

	return PSA_SUCCESS;
}

psa_status_t tfm_se_req_mngr_init(void)
{
	psa_msg_t msg;

	timer_delay_init();
	stsafe_power_init();
	i2c2_init();

	while (1) {
		psa_signal_t signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);

		if (signals & TFM_SE_I2C_PROBE_SIGNAL) {
			psa_get(TFM_SE_I2C_PROBE_SIGNAL, &msg);
			switch (msg.type) {
			case PSA_IPC_CONNECT:
			case PSA_IPC_DISCONNECT:
				psa_reply(msg.handle, PSA_SUCCESS);
				break;
			case PSA_IPC_CALL:
				psa_reply(msg.handle, handle_i2c_probe(&msg));
				break;
			default:
				psa_panic();
			}
		} else if (signals & TFM_SE_I2C_ECHO_SIGNAL) {
			psa_get(TFM_SE_I2C_ECHO_SIGNAL, &msg);
			switch (msg.type) {
			case PSA_IPC_CONNECT:
			case PSA_IPC_DISCONNECT:
				psa_reply(msg.handle, PSA_SUCCESS);
				break;
			case PSA_IPC_CALL:
				psa_reply(msg.handle, handle_i2c_echo(&msg));
				break;
			default:
				psa_panic();
			}
		} else {
			psa_panic();
		}
	}

	return PSA_ERROR_SERVICE_FAILURE;
}
