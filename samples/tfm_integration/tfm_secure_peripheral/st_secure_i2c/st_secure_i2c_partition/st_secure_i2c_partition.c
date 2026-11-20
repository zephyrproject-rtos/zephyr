/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <psa/service.h>
#include <psa_manifest/tfm_st_secure_i2c_partition.h>
#include <stm32u5xx_hal.h>
#include <tfm_log.h>
#include <stdint.h>

#include "st_secure_i2c_common.h"

#if TFM_ISOLATION_LEVEL != 1
#error "Partition implementation requires GTZC access, hence require TFM_ISOLATION_LEVEL 1"
#endif

/**
 * b_u585i_iot02a board revisions up to C02 wires GPIO pin F11 to STSAFE Reset line (active low).
 * The signal F11 needs to be driven to 1 to enable the STSAFE.
 * From revision D01, GPIO pin F11 is connected to the STSAFE Power-Enable (active low), which
 *  will naturally drive the STSAFE Reset line to 1. So the signal F11 needs to be driven to 0
 * to enable the STSAFE.
 * Set B_U585I_IOT02A_REV value according to your hardware.
 */
#define REV_D01            1
#define REV_C02            2
#define B_U585I_IOT02A_REV REV_D01

#define STSAFE_EN_PORT GPIOF
#define STSAFE_EN_PIN  GPIO_PIN_11
#define I2C2_SCL_PIN   GPIO_PIN_4
#define I2C2_SDA_PIN   GPIO_PIN_5
#define STSAFE_ADDR    0x20U
#define I2C2_TIMING    0x30909DECU /* 100kHz */

/* STSAFE echo command frame: 0x00 | data | CRC16.
 * The answer is 0x00 | length (2 bytes) | data | CRC16.
 */
#define STSAFE_CMD_ECHO       0x00U
#define STSAFE_CMD_HEADER_LEN 1U
#define STSAFE_RSP_HEADER_LEN 3U
#define STSAFE_CRC_LEN        2U

#define TIM6_TICK_HZ      10000U
#define TIM6_TICKS_PER_MS (TIM6_TICK_HZ / 1000U)

#define STSAFE_ECHO_BUF_SIZE 32U

static I2C_HandleTypeDef hi2c2;
static size_t echo_rx_len;

static void timer_init(void)
{
	RCC_ClkInitTypeDef clk = {0};
	uint32_t flash_latency;
	uint32_t timclk;

	__HAL_RCC_TIM6_CLK_ENABLE();

	SystemCoreClockUpdate();
	HAL_RCC_GetClockConfig(&clk, &flash_latency);
	timclk = HAL_RCC_GetPCLK1Freq();
	if (clk.APB1CLKDivider != RCC_HCLK_DIV1) {
		timclk *= 2U;
	}

	TIM6->CR1 = 0;
	TIM6->PSC = (uint16_t)((timclk / TIM6_TICK_HZ) - 1U);
	TIM6->ARR = 0xFFFFU;
	TIM6->EGR = TIM_EGR_UG;
	TIM6->SR = 0;
	TIM6->CR1 = TIM_CR1_CEN;
}

static uint16_t timer_now(void)
{
	return (uint16_t)TIM6->CNT;
}

static int timer_elapsed(uint16_t start, uint32_t ms)
{
	return (uint16_t)(timer_now() - start) >= ms * TIM6_TICKS_PER_MS;
}

static void delay_ms(uint32_t ms)
{
	uint16_t start = timer_now();

	while (!timer_elapsed(start, ms)) {
	}
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
	INFO("STSAFE powered (EN low)\n");
#elif B_U585I_IOT02A_REV == REV_C02
	HAL_GPIO_WritePin(STSAFE_EN_PORT, STSAFE_EN_PIN, GPIO_PIN_SET);
	INFO("STSAFE reset released (EN high)\n");
#else
#error "Unsupported B_U585I_IOT02A_REV value"
#endif

	delay_ms(50);
}

static psa_status_t i2c2_init(void)
{
	RCC_PeriphCLKInitTypeDef pclk = {0};
	GPIO_InitTypeDef gpio = {0};

	pclk.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
	pclk.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	__HAL_RCC_I2C2_CLK_ENABLE();

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
	if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	INFO("I2C2 initialized\n");
	return PSA_SUCCESS;
}

static HAL_StatusTypeDef i2c2_wait_flag(uint32_t flag, uint16_t start, uint32_t timeout_ms)
{
	while ((I2C2->ISR & flag) == 0U) {
		if ((I2C2->ISR & I2C_ISR_NACKF) != 0U) {
			return HAL_ERROR;
		}
		if (timer_elapsed(start, timeout_ms)) {
			return HAL_TIMEOUT;
		}
	}

	return HAL_OK;
}

/* Register-level transfer with automatic STOP, bounded by TIM6.
 * A zero-length write only checks that the device acknowledges its address.
 */
static HAL_StatusTypeDef i2c2_xfer(int read, uint8_t *buf, size_t len, uint32_t timeout_ms,
				   uint32_t *isr)
{
	uint16_t start = timer_now();
	HAL_StatusTypeDef status = HAL_OK;

	while ((I2C2->ISR & I2C_ISR_BUSY) != 0U) {
		if (timer_elapsed(start, timeout_ms)) {
			status = HAL_TIMEOUT;
			break;
		}
	}

	if (status == HAL_OK) {
		I2C2->CR2 = (STSAFE_ADDR << 1) | (len << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND |
			    (read ? I2C_CR2_RD_WRN : 0U) | I2C_CR2_START;
	}

	for (size_t i = 0; i < len && status == HAL_OK; i++) {
		if (read) {
			status = i2c2_wait_flag(I2C_ISR_RXNE, start, timeout_ms);
			if (status == HAL_OK) {
				buf[i] = (uint8_t)I2C2->RXDR;
			}
		} else {
			status = i2c2_wait_flag(I2C_ISR_TXIS, start, timeout_ms);
			if (status == HAL_OK) {
				I2C2->TXDR = buf[i];
			}
		}
	}

	if (status == HAL_OK) {
		status = i2c2_wait_flag(I2C_ISR_STOPF, start, timeout_ms);
	}
	if (status == HAL_OK && (I2C2->ISR & I2C_ISR_NACKF) != 0U) {
		status = HAL_ERROR;
	}

	if (isr != NULL) {
		*isr = I2C2->ISR;
	}

	if (status != HAL_OK) {
		/* Software reset: release the bus and flush the peripheral state. */
		__HAL_I2C_DISABLE(&hi2c2);
		(void)I2C2->CR1;
		__HAL_I2C_ENABLE(&hi2c2);
	}
	I2C2->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;

	return status;
}

static psa_status_t handle_i2c_probe(psa_msg_t *msg)
{
	struct i2c_probe_result res = {0};
	uint32_t isr = 0U;

	if (msg->out_size[0] < sizeof(res)) {
		INFO("Output buffer too small\n");
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	timer_init();

	for (int trial = 0; trial < 2; trial++) {
		res.hal_status = (uint8_t)i2c2_xfer(0, NULL, 0, 100, &isr);
		if (res.hal_status == HAL_OK) {
			break;
		}
	}

	res.isr_lo = (uint8_t)(isr & 0xFFU);
	res.isr_hi = (uint8_t)((isr >> 8) & 0xFFU);

	psa_write(msg->handle, 0, &res, sizeof(res));
	return PSA_SUCCESS;
}

static psa_status_t handle_i2c_echo_send(psa_msg_t *msg)
{
	uint8_t tx_buf[STSAFE_ECHO_BUF_SIZE];
	size_t tx_len = msg->in_size[0];
	size_t rx_len = tx_len - STSAFE_CMD_HEADER_LEN + STSAFE_RSP_HEADER_LEN;

	if (tx_len < STSAFE_CMD_HEADER_LEN + STSAFE_CRC_LEN || tx_len > sizeof(tx_buf) ||
	    rx_len > STSAFE_ECHO_BUF_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_read(msg->handle, 0, tx_buf, tx_len);

	if (tx_buf[0] != STSAFE_CMD_ECHO) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	timer_init();

	if (i2c2_xfer(0, tx_buf, tx_len, 20, NULL) != HAL_OK) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	echo_rx_len = rx_len;
	return PSA_SUCCESS;
}

static psa_status_t handle_i2c_echo_recv(psa_msg_t *msg)
{
	uint8_t rx_buf[STSAFE_ECHO_BUF_SIZE];
	size_t rx_len = msg->out_size[0];

	if (echo_rx_len == 0U || rx_len != echo_rx_len) {
		return PSA_ERROR_BAD_STATE;
	}
	echo_rx_len = 0U;

	timer_init();

	if (i2c2_xfer(1, rx_buf, rx_len, 50, NULL) != HAL_OK) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	psa_write(msg->handle, 0, rx_buf, rx_len);
	return PSA_SUCCESS;
}

psa_status_t tfm_st_secure_i2c_req_mngr_init(void)
{
	psa_msg_t msg;

	timer_init();
	stsafe_power_init();
	if (i2c2_init() != PSA_SUCCESS) {
		ERROR("I2C2 initialization failed\n");
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	while (1) {
		psa_signal_t signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);

		if (signals & TFM_ST_SECURE_I2C_PROBE_SIGNAL) {
			psa_get(TFM_ST_SECURE_I2C_PROBE_SIGNAL, &msg);
			switch (msg.type) {
			case PSA_IPC_CONNECT:
			case PSA_IPC_DISCONNECT:
				psa_reply(msg.handle, PSA_SUCCESS);
				break;
			case PSA_IPC_CALL:
				psa_reply(msg.handle, handle_i2c_probe(&msg));
				break;
			default:
				psa_reply(msg.handle, PSA_ERROR_NOT_SUPPORTED);
				break;
			}
		} else if (signals & TFM_ST_SECURE_I2C_ECHO_SIGNAL) {
			static psa_handle_t echo_owner = PSA_NULL_HANDLE;

			psa_get(TFM_ST_SECURE_I2C_ECHO_SIGNAL, &msg);
			switch (msg.type) {
			case PSA_IPC_CONNECT:
				if (echo_owner != PSA_NULL_HANDLE) {
					psa_reply(msg.handle, PSA_ERROR_CONNECTION_BUSY);
					break;
				}
				echo_owner = msg.handle;
				echo_rx_len = 0U;
				psa_reply(msg.handle, PSA_SUCCESS);
				break;
			case PSA_IPC_DISCONNECT:
				echo_owner = PSA_NULL_HANDLE;
				echo_rx_len = 0U;
				psa_reply(msg.handle, PSA_SUCCESS);
				break;
			case ST_SECURE_I2C_ECHO_SEND:
				psa_reply(msg.handle, handle_i2c_echo_send(&msg));
				break;
			case ST_SECURE_I2C_ECHO_RECV:
				psa_reply(msg.handle, handle_i2c_echo_recv(&msg));
				break;
			default:
				psa_reply(msg.handle, PSA_ERROR_NOT_SUPPORTED);
				break;
			}
		}

		/* Ignore unknown messages. */
	}

	return PSA_ERROR_SERVICE_FAILURE;
}
