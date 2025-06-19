/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc27xx_rng

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/irq.h>
#include <string.h>

#include <zephyr/sys/sys_io.h>
#include <driverlib/hapi.h>

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "Entropy Driver for CC27xx requires at least one instance"
#endif

#define ENTROPY_LENGTH_BYTES_MIN         64
#define SHA256_DIGEST_LENGTH_BYTES       32
#define ENTROPY_HEALTH_RCT_THRESHOLD_MIN 24
#define ENTROPY_HEALTH_APT_THRESHOLD     461
#define ENTROPY_HEALTH_BIMODAL_THRESHOLD 369
/* Window size for Adaptive Proportion Test */
#define ENTROPY_HEALTH_APT_WINDOW_SIZE   512

#if (CONFIG_ENTROPY_CC27XX_ENTROPY_BYTE_LENGTH < ENTROPY_LENGTH_BYTES_MIN) ||                      \
	(CONFIG_ENTROPY_CC27XX_ENTROPY_BYTE_LENGTH % SHA256_DIGEST_LENGTH_BYTES != 0)
#error "ENTROPY_CC27XX_ENTROPY_BYTE_LENGTH must be a multiple of 32 and at least 64 bytes"
#endif

#if (CONFIG_ENTROPY_CC27XX_RCT_THRESHOLD < ENTROPY_HEALTH_RCT_THRESHOLD_MIN) ||                    \
	(CONFIG_ENTROPY_CC27XX_RCT_THRESHOLD % 3 != 0)
#error "ENTROPY_CC27XX_RCT_THRESHOLD must be a multiple of 3 and at least 24"
#endif

/* Use the function provided by RCL to read noise input */
extern int_fast16_t RCL_AdcNoise_get_samples_blocking(uint32_t *buffer, uint32_t numWords);

/* Entropy buffer needs to be 32-bit aligned for the SHA2 operation. */
static uint8_t __aligned(4) entropy[CONFIG_ENTROPY_CC27XX_ENTROPY_BYTE_LENGTH];
static uint16_t entropy_pool_level;

static int_fast16_t get_rcl_noise(uint32_t *local_noise_input, uint16_t noise_len)
{
	int_fast16_t rcl_status;

	/* Check noise input pointer is valid. */
	if (local_noise_input == NULL) {
		return -EINVAL;
	}

	/* Use the function provided by RCL to read noise input */
	rcl_status = RCL_AdcNoise_get_samples_blocking(local_noise_input, noise_len);

	return rcl_status;
}

/*
 *  ======== add_codes_to_apt_densities ========
 */
static void add_codes_to_apt_densities(uint16_t codes, volatile uint16_t *densities)
{
	/* Counts a set of three codes in the densities array. */
	for (uint_fast8_t i = 0U; i < 3U; i++) {
		uint8_t code = codes & 0x0F;

		densities[code]++;
		codes >>= 5;
	}
}

/*!
 * @brief Runs Repetitive Count Tests (RCT).
 *
 * Each word is assumed to contain 3 IA Codes (IAC) and 3 QA Codes (QAC).
 * Each group of codes (IAC and QAC) are treated independently for the
 * purpose of this test.
 *
 * For efficiency, each group of 3 codes is compared to the last group of
 * three codes (the 3 new IACs are compared to the last 3 IACs in one
 * comparison operation, same for the QACs).
 *
 * @param last_word           is the previous word from noise data
 * @param new_word            is the current word from noise data.
 * @param count_repeated_iac  the current count of repeated IAC codes.
 * @param count_repeated_qac  the current count of repeated QAC codes.
 *
 * @retval -EIO                RCT Failed
 * @retval 0                   RCT Passed
 *
 */
/*
 *  ======== execute_rct ========
 */
static inline int_fast16_t execute_rct(uint32_t last_word, uint32_t new_word,
				       size_t *count_repeated_iac, size_t *count_repeated_qac)
{
	/* CONFIG_ENTROPY_CC27XX_RCT_THRESHOLD has been already verified to be
	 * a multiple of 3. Simple division here is sufficient.
	 */
	uint16_t entropy_rct_threshold = CONFIG_ENTROPY_CC27XX_RCT_THRESHOLD / 3;

	if (((new_word ^ last_word) & 0x0000FFFF) != 0U) {
		*count_repeated_qac = 0U;
	} else {
		(*count_repeated_qac)++;

		if (*count_repeated_qac >= entropy_rct_threshold) {
			return -EIO;
		}
	}

	if (((new_word ^ last_word) & 0xFFFF0000) != 0U) {
		*count_repeated_iac = 0U;
	} else {
		(*count_repeated_iac)++;

		if (*count_repeated_iac >= entropy_rct_threshold) {
			return -EIO;
		}
	}
	return 0;
}

/*!
 * @brief Runs Adaptive Proportion Tests (APT).
 *
 * These tests are a modified version of the test described in NIST SP 800-90B.
 * This implementation is more conservative than the implementation described in
 * the NIST document. The NIST specification only considers a single code value
 * within a window of 512 values. This test considers all code values
 * within that window (or slightly more since codes are loaded in
 * 6 code sequences). Thus, this implementation is more likely
 * to detect a failure of the entropy source than the NIST specified
 * test.
 *
 * In addition, this test also considers a bimodal threshold to
 * detect instances where the entropy source is cycling between
 * two distinct code values such that the two values are much
 * more common in the sequence that expected.
 *
 * @param densities is a collection of 4-bit code counts over a 512 (+ 5) code window.
 *
 * @retval -EIO                APT Test failed
 * @retval 0                   Both APT and APT Bimodal passed
 */
/*
 *  ======== execute_apt ========
 */
static inline int_fast16_t execute_apt(volatile uint16_t *densities)
{
	uint8_t populations_above_bimodal_limit = 0U;

	for (uint_fast8_t j = 0U; j < 16U; j++) {
		if (densities[j] > CONFIG_ENTROPY_CC27XX_APT_THRESHOLD) {
			return -EIO;
		}

		if (densities[j] > CONFIG_ENTROPY_CC27XX_APT_BIMODAL_THRESHOLD) {
			if (populations_above_bimodal_limit > 0U) {
				return -EIO;
			}

			populations_above_bimodal_limit++;
		}

		densities[j] = 0;
	}

	return 0;
}

/*!
 * @brief Performs Health Checks on the noise buffer from RCL before conditioning.
 *
 * This function performs 2 Health Checks - Repetitive Count Test (RCT) and
 * Adaptive Proportion Test (APT). These tests are a modified version of the tests
 * described in NIST SP 800-90B. RCT is performed if CONFIG_ENTROPY_CC27XX_RCT_ENABLED
 * is true and apt is performed if CONFIG_ENTROPY_CC27XX_APT_ENABLED is true.
 *
 * The noise_data should not be used if this function returns an error code.
 *
 * @param  noise_data A pointer to the buffer containing noise input from RCL
 *                      A word of input noise data follows the following format
 *                      Bit 31: 0
 *                      Bit 30..26: 5-bit I Arithmetic Code Reading X+2
 *                      Bit 25..21: 5-bit I Arithmetic Code Reading X+1
 *                      Bit 20..16: 5-bit I Arithmetic Code Reading X
 *                      Bit 15: 0
 *                      Bit 14..10: 5-bit Q Arithmetic Code X+2
 *                      Bit  9..5 :  5-bit Q Arithmetic Code X+1
 *                      Bit  4..0 :  5-bit Q Arithmetic Code X
 *
 * @retval -EIO         Health Check Failed, making the noise input invalid
 * @retval 0            All Health Checks passed, making the noise input valid
 */
static int_fast16_t entropy_health_tests(uint32_t *noise_data)
{
	volatile uint16_t densities[32];
	uint32_t new_word;
	uint16_t ia_codes;
	uint16_t qa_codes;
	int_fast16_t return_value = 0;
	size_t num_codes_in_apt_window = 0U;
	size_t count_repeated_iac = 0U;
	size_t count_repeated_qac = 0U;
	uint32_t last_word = 0xFFFFFFFF;

	memset((void *)densities, 0x0, 32 * sizeof(uint16_t));

	for (size_t i = 0U; i < CONFIG_ENTROPY_CC27XX_NOISE_INPUT_WORD_LENGTH; i++) {
		new_word = noise_data[i];

		if (CONFIG_ENTROPY_CC27XX_RCT_ENABLED) {
			return_value = execute_rct(last_word, new_word, &count_repeated_iac,
						   &count_repeated_qac);
			if (return_value != 0) {
				return return_value;
			}
			last_word = new_word;
		}

		if (CONFIG_ENTROPY_CC27XX_APT_ENABLED) {
			qa_codes = new_word & 0xFFFF;
			add_codes_to_apt_densities(qa_codes, densities);
			ia_codes = new_word >> 16;
			add_codes_to_apt_densities(ia_codes, densities);

			num_codes_in_apt_window += 6U;

			if (num_codes_in_apt_window >= ENTROPY_HEALTH_APT_WINDOW_SIZE) {
				return_value = execute_apt(densities);
				if (return_value != 0) {
					return return_value;
				}
				num_codes_in_apt_window = 0U;
			}
		}
	}
	return return_value;
}

static int entropy_cc27xx_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	int ret_status = 0;

	if (len > entropy_pool_level) {
		ret_status = -EPERM;
	} else {
		if ((len > 0) && (entropy_pool_level > 0)) {
			/* Copy entropy generated to the buffer */
			memcpy(buf, &entropy[entropy_pool_level - len], len);
			memset(&entropy[entropy_pool_level - len], 0, len);
			entropy_pool_level -= len;
		} else {
			/* Invalid length */
			ret_status = -EPERM;
		}
	}

	return ret_status;
}

static int entropy_cc27xx_init(const struct device *dev)
{
	SHA256SW_Object sha256_sw_object;
	int_fast16_t rcl_status;
	int_fast16_t sha256_sw_status;
	size_t noise_length = CONFIG_ENTROPY_CC27XX_NOISE_INPUT_WORD_LENGTH * 4;

	/* Each 32-bytes of entropy will be generated from given noise input array */
	uint32_t *rcl_noise;

	rcl_noise = k_malloc(noise_length);
	if (rcl_noise == NULL) {
		return -ENOSR;
	}

	/* Since the value of entropy length has already been verified, performing
	 * a simple division and taking the floor value here suffice.
	 */
	uint16_t entropy_iter =
		CONFIG_ENTROPY_CC27XX_ENTROPY_BYTE_LENGTH / SHA256_DIGEST_LENGTH_BYTES;

	/* The following code is used to generate the entropy from the RCL noise.
	 * The entropy is generated using SHA2 Operation.
	 */
	for (int i = 0; i < entropy_iter; i++) {
		/* Clear noise data array and retrieve RCL noise */
		memset(rcl_noise, 0, noise_length);
		rcl_status =
			get_rcl_noise(rcl_noise, CONFIG_ENTROPY_CC27XX_NOISE_INPUT_WORD_LENGTH);

		if (CONFIG_ENTROPY_CC27XX_RCT_ENABLED || CONFIG_ENTROPY_CC27XX_APT_ENABLED) {
			/* Perform Health Checks on the noise data before generating entropy */
			rcl_status = entropy_health_tests(rcl_noise);
		}

		if (rcl_status == 0) {
			/* RCL noise successfully retrieved. Generate 32 bytes of entropy */
			sha256_sw_status = HapiSha256SwHashData(
				(SHA256SW_Handle)&sha256_sw_object, rcl_noise, noise_length,
				(uint32_t *)(entropy + (i * SHA256_DIGEST_LENGTH_BYTES)));
			if (sha256_sw_status != SHA2SW_STATUS_SUCCESS) {
				k_free(rcl_noise);
				return -EIO;
			}
			entropy_pool_level += SHA256_DIGEST_LENGTH_BYTES;
		} else {
			/* Error retrieving noise from RCL or not suitable noise data */
			k_free(rcl_noise);
			return rcl_status;
		}
	}
	k_free(rcl_noise);
	return 0;
}

static DEVICE_API(entropy, entropy_cc27xx_driver_api) = {
	.get_entropy = entropy_cc27xx_get_entropy,
};

DEVICE_DT_INST_DEFINE(0, entropy_cc27xx_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_cc27xx_driver_api);
