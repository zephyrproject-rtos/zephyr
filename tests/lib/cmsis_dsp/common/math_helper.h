/* ----------------------------------------------------------------------
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * $Date:        29. November 2010
 * $Revision:    V1.0.3
 *
 * Project:      CMSIS DSP Library
 *
 * Title:        math_helper.h
 *
 *
 * Description:	Prototypes of all helper functions required.
 *
 * Target Processor: Cortex-M4/Cortex-M3
 *
 * Version 1.0.3 2010/11/29
 *    Re-organized the CMSIS folders and updated documentation.
 *
 * Version 1.0.2 2010/11/11
 *    Documentation updated.
 *
 * Version 1.0.1 2010/10/05
 *    Production release and review comments incorporated.
 *
 * Version 1.0.0 2010/09/20
 *    Production release and review comments incorporated.
 *
 * Version 0.0.7  2010/06/10
 *    Misra-C changes done
 * -------------------------------------------------------------------- */

/*
 * This file was imported from the ARM CMSIS-DSP library test suite and
 * reworked for use in the Zephyr tests.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MATH_HELPER_H
#define MATH_HELPER_H

#include "arm_math.h"

/**
 * @brief  Calculation of SNR
 * @param  float*   Pointer to the reference buffer
 * @param  float*   Pointer to the test buffer
 * @param  uint32_t    total number of samples
 * @return float    SNR
 * The function calculates signal to noise ratio for the reference output
 * and test output
 */

/* If NaN, force SNR to 0.0 to ensure test will fail */
#define IFNANRETURNZERO(val)		\
	do {				\
		if (isnan((val))) {	\
			return 0.0;	\
		}			\
	} while (0)

#define IFINFINITERETURN(val, def)		\
	do {					\
		if (isinf((val))) {		\
			if ((val) > 0) {	\
				return def;	\
			} else {		\
				return -def;	\
			}			\
		}				\
	} while (0)

static inline double arm_snr_f64(const double *pRef, const double *pTest,
	uint32_t buffSize)
{
	double EnergySignal = 0.0, EnergyError = 0.0;
	uint32_t i;
	double SNR;

	for (i = 0; i < buffSize; i++) {
		/* Checking for a NAN value in pRef array */
		IFNANRETURNZERO(pRef[i]);

		/* Checking for a NAN value in pTest array */
		IFNANRETURNZERO(pTest[i]);

		EnergySignal += pRef[i] * pRef[i];
		EnergyError += (pRef[i] - pTest[i]) * (pRef[i] - pTest[i]);
	}

	/* Checking for a NAN value in EnergyError */
	IFNANRETURNZERO(EnergyError);

	SNR = 10 * log10(EnergySignal / EnergyError);

	/* Checking for a NAN value in SNR */
	IFNANRETURNZERO(SNR);
	IFINFINITERETURN(SNR, 100000.0);

	return SNR;
}

static inline float arm_snr_f32(const float *pRef, const float *pTest,
	uint32_t buffSize)
{
	float EnergySignal = 0.0, EnergyError = 0.0;
	uint32_t i;
	float SNR;

	for (i = 0; i < buffSize; i++) {
		/* Checking for a NAN value in pRef array */
		IFNANRETURNZERO(pRef[i]);

		/* Checking for a NAN value in pTest array */
		IFNANRETURNZERO(pTest[i]);

		EnergySignal += pRef[i] * pRef[i];
		EnergyError += (pRef[i] - pTest[i]) * (pRef[i] - pTest[i]);
	}

	/* Checking for a NAN value in EnergyError */
	IFNANRETURNZERO(EnergyError);


	SNR = 10 * log10f(EnergySignal / EnergyError);

	/* Checking for a NAN value in SNR */
	IFNANRETURNZERO(SNR);
	IFINFINITERETURN(SNR, 100000.0);

	return SNR;
}

static inline float arm_snr_q63(const q63_t *pRef, const q63_t *pTest,
	uint32_t buffSize)
{
	double EnergySignal = 0.0, EnergyError = 0.0;
	uint32_t i;
	float SNR;

	double testVal, refVal;

	for (i = 0; i < buffSize; i++) {
		testVal = ((double)pTest[i]) / 9223372036854775808.0;
		refVal = ((double)pRef[i]) / 9223372036854775808.0;

		EnergySignal += refVal * refVal;
		EnergyError += (refVal - testVal) * (refVal - testVal);
	}


	SNR = 10 * log10(EnergySignal / EnergyError);

	/* Checking for a NAN value in SNR */
	IFNANRETURNZERO(SNR);
	IFINFINITERETURN(SNR, 100000.0);

	return SNR;
}

static inline float arm_snr_q31(const q31_t *pRef, const q31_t *pTest,
	uint32_t buffSize)
{
	float EnergySignal = 0.0, EnergyError = 0.0;
	uint32_t i;
	float SNR;

	float32_t testVal, refVal;

	for (i = 0; i < buffSize; i++) {

		testVal = ((float32_t)pTest[i]) / 2147483648.0f;
		refVal = ((float32_t)pRef[i]) / 2147483648.0f;

		EnergySignal += refVal * refVal;
		EnergyError += (refVal - testVal) * (refVal - testVal);
	}


	SNR = 10 * log10f(EnergySignal / EnergyError);

	/* Checking for a NAN value in SNR */
	IFNANRETURNZERO(SNR);
	IFINFINITERETURN(SNR, 100000.0);

	return SNR;
}

static inline float arm_snr_q15(const q15_t *pRef, const q15_t *pTest,
	uint32_t buffSize)
{
	float EnergySignal = 0.0, EnergyError = 0.0;
	uint32_t i;
	float SNR;

	float32_t testVal, refVal;

	for (i = 0; i < buffSize; i++) {
		testVal = ((float32_t)pTest[i]) / 32768.0f;
		refVal = ((float32_t)pRef[i]) / 32768.0f;

		EnergySignal += refVal * refVal;
		EnergyError += (refVal - testVal) * (refVal - testVal);
	}

	SNR = 10 * log10f(EnergySignal / EnergyError);

	/* Checking for a NAN value in SNR */
	IFNANRETURNZERO(SNR);
	IFINFINITERETURN(SNR, 100000.0);

	return SNR;
}

static inline float arm_snr_q7(const q7_t *pRef, const q7_t *pTest,
	uint32_t buffSize)
{
	float EnergySignal = 0.0, EnergyError = 0.0;
	uint32_t i;
	float SNR;

	float32_t testVal, refVal;

	for (i = 0; i < buffSize; i++) {
		testVal = ((float32_t)pTest[i]) / 128.0f;
		refVal = ((float32_t)pRef[i]) / 128.0f;

		EnergySignal += refVal * refVal;
		EnergyError += (refVal - testVal) * (refVal - testVal);
	}

	SNR = 10 * log10f(EnergySignal / EnergyError);

	IFNANRETURNZERO(SNR);
	IFINFINITERETURN(SNR, 100000.0);

	return SNR;
}

#endif
