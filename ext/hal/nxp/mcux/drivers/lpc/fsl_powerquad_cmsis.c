/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_powerquad.h"
#include "fsl_powerquad_data.h"
#include "arm_math.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.powerquad_cmsis"
#endif

#define PQ_SET_FIX32_CONFIG                                                                 \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_Float; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_Float; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_Float; \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_FIX16_CONFIG                                                                 \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float; \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_Q31_CONFIG                                                                     \
    POWERQUAD->OUTFORMAT = ((uint32_t)(-31) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_Float; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_Float;   \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_Float;   \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float;   \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_Q15_CONFIG                                                                     \
    POWERQUAD->OUTFORMAT = ((uint32_t)(-15) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float;   \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float;   \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float;   \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_F32_CONFIG                                                                   \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_FFT_Q31_CONFIG                                                               \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_32Bit; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_32Bit; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_32Bit; \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_32Bit; \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_FFT_Q15_CONFIG                                                               \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_32Bit; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_32Bit; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_32Bit; \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_32Bit; \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_MAT_FIX32_WORKAROUND_SCALE_CONFIG                                            \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_Float; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_Float; \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_MAT_FIX32_WORKAROUND_MULT_CONFIG                                             \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_32Bit << 4U) | kPQ_Float; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_MAT_FIX16_WORKAROUND_SCALE_CONFIG                                            \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float; \
    POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->TMPBASE = 0xE0000000

#define PQ_SET_MAT_FIX16_WORKAROUND_MULT_CONFIG                                             \
    POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float; \
    POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float; \
    POWERQUAD->TMPBASE = 0xE0000000

/*******************************************************************************
 * Code
 ******************************************************************************/
static void _arm_fir_increment(void *pSrc, uint16_t srcLen, void *pTap, uint16_t tapLen, void *pDst, uint32_t offset)
{
    POWERQUAD->INABASE = (uint32_t)pSrc;
    POWERQUAD->INBBASE = (uint32_t)pTap;
    POWERQUAD->LENGTH = ((uint32_t)tapLen << 16U) + srcLen;
    POWERQUAD->OUTBASE = (uint32_t)pDst;
    POWERQUAD->MISC = offset;
    POWERQUAD->CONTROL = (CP_FIR << 4) | PQ_FIR_INCREMENTAL;
}

float32_t arm_cos_f32(float32_t x)
{
    float tmp;

    PQ_CosF32(&x, &tmp);
    return tmp;
}

q31_t arm_cos_q31(q31_t x)
{
    /* For PQ: input -1 to 1 means -pi to pi
     * For CMSIS DSP: input 0 to 1 means pi to 2*pi */
    x *= 2;
    return PQ_CosQ31(x);
}

q15_t arm_cos_q15(q15_t x)
{
    /* For PQ: input -1 to 1 means -pi to pi
     * For CMSIS DSP: input 0 to 1 means pi to 2*pi */
    x *= 2;
    return PQ_CosQ15(x);
}

float32_t arm_sin_f32(float32_t x)
{
    float tmp;

    PQ_SinF32(&x, &tmp);
    return tmp;
}

q31_t arm_sin_q31(q31_t x)
{
    /* For PQ: input -1 to 1 means -pi to pi
     * For CMSIS DSP: input 0 to 1 means pi to 2*pi */
    x *= 2;
    return PQ_SinQ31(x);
}

q15_t arm_sin_q15(q15_t x)
{
    /* For PQ: input -1 to 1 means -pi to pi
     * For CMSIS DSP: input 0 to 1 means pi to 2*pi */
    x *= 2;
    return PQ_SinQ15(x);
}

arm_status arm_sqrt_q31(q31_t in, q31_t *pOut)
{
    uint32_t cppre;

    /* If the input is a positive number then compute the signBits. */
    if (in > 0)
    {
        cppre = POWERQUAD->CPPRE;
        POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_IN(-31) | POWERQUAD_CPPRE_CPPRE_OUT(31);
        *pOut = PQ_SqrtFixed(in);
        POWERQUAD->CPPRE = cppre;

        return (ARM_MATH_SUCCESS);
    }
    /* If the number is a negative number then store zero as its square root value */
    else
    {
        *pOut = 0;

        return (ARM_MATH_ARGUMENT_ERROR);
    }
}

arm_status arm_sqrt_q15(q15_t in, q15_t *pOut)
{
    uint32_t cppre;

    /* If the input is a positive number then compute the signBits. */
    if (in > 0)
    {
        cppre = POWERQUAD->CPPRE;
        POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_IN(-15) | POWERQUAD_CPPRE_CPPRE_OUT(15);
        *pOut = PQ_SqrtFixed(in);
        POWERQUAD->CPPRE = cppre;

        return (ARM_MATH_SUCCESS);
    }
    /* If the number is a negative number then store zero as its square root value */
    else
    {
        *pOut = 0;

        return (ARM_MATH_ARGUMENT_ERROR);
    }
}

void arm_cfft_q31(const arm_cfft_instance_q31 *S, q31_t *p1, uint8_t ifftFlag, uint8_t bitReverseFlag)
{
    assert(bitReverseFlag == 1);

    q31_t *pIn = p1;
    q31_t *pOut = p1;
    uint32_t length = S->fftLen;

    PQ_SET_FFT_Q31_CONFIG;

    if (ifftFlag == 1U)
    {
        PQ_TransformIFFT(POWERQUAD, length, pIn, pOut);
    }
    else
    {
        PQ_TransformCFFT(POWERQUAD, length, pIn, pOut);
    }

    PQ_WaitDone(POWERQUAD);
}

void arm_cfft_q15(const arm_cfft_instance_q15 *S, q15_t *p1, uint8_t ifftFlag, uint8_t bitReverseFlag)
{
    assert(bitReverseFlag == 1);

    q15_t *pIn = p1;
    q15_t *pOut = p1;
    uint32_t length = S->fftLen;

    PQ_SET_FFT_Q15_CONFIG;

    if (ifftFlag == 1U)
    {
        PQ_TransformIFFT(POWERQUAD, length, pIn, pOut);
    }
    else
    {
        PQ_TransformCFFT(POWERQUAD, length, pIn, pOut);
    }

    PQ_WaitDone(POWERQUAD);
}

arm_status arm_rfft_init_q31(arm_rfft_instance_q31 *S, uint32_t fftLenReal, uint32_t ifftFlagR, uint32_t bitReverseFlag)
{
    /* Only supprt such mode. */
    assert(ifftFlagR == 0);
    assert(bitReverseFlag == 1);

    /*  Initialise the default arm status */
    arm_status status = ARM_MATH_SUCCESS;

    /*  Initialize the Real FFT length */
    S->fftLenReal = (uint16_t)fftLenReal;

    /*  Initialize the Flag for selection of RFFT or RIFFT */
    S->ifftFlagR = (uint8_t)ifftFlagR;

    /*  Initialize the Flag for calculation Bit reversal or not */
    S->bitReverseFlagR = (uint8_t)bitReverseFlag;

    /* return the status of RFFT Init function */
    return (status);
}

void arm_rfft_q31(const arm_rfft_instance_q31 *S, q31_t *pSrc, q31_t *pDst)
{
    uint32_t length = S->fftLenReal;
    PQ_SET_FFT_Q31_CONFIG;

    /* Calculation of RFFT of input */
    PQ_TransformRFFT(POWERQUAD, length, pSrc, pDst);

    PQ_WaitDone(POWERQUAD);
}

arm_status arm_rfft_init_q15(arm_rfft_instance_q15 *S, uint32_t fftLenReal, uint32_t ifftFlagR, uint32_t bitReverseFlag)
{
    /* Only supprt such mode. */
    assert(ifftFlagR == 0);
    assert(bitReverseFlag == 1);

    /*  Initialise the default arm status */
    arm_status status = ARM_MATH_SUCCESS;

    /*  Initialize the Real FFT length */
    S->fftLenReal = (uint16_t)fftLenReal;

    /*  Initialize the Flag for selection of RFFT or RIFFT */
    S->ifftFlagR = (uint8_t)ifftFlagR;

    /*  Initialize the Flag for calculation Bit reversal or not */
    S->bitReverseFlagR = (uint8_t)bitReverseFlag;

    /* return the status of RFFT Init function */
    return (status);
}

void arm_rfft_q15(const arm_rfft_instance_q15 *S, q15_t *pSrc, q15_t *pDst)
{
    uint32_t length = S->fftLenReal;
    PQ_SET_FFT_Q15_CONFIG;

    /* Calculation of RFFT of input */
    PQ_TransformRFFT(POWERQUAD, length, pSrc, pDst);

    PQ_WaitDone(POWERQUAD);
}

arm_status arm_dct4_init_q31(arm_dct4_instance_q31 *S,
                             arm_rfft_instance_q31 *S_RFFT,
                             arm_cfft_radix4_instance_q31 *S_CFFT,
                             uint16_t N,
                             uint16_t Nby2,
                             q31_t normalize)
{
    /* Initialize the DCT4 length */
    S->N = N;

    /* Initialize Real FFT Instance */
    S->pRfft = S_RFFT;

    switch (N)
    {
        /* Initialize the table modifier values */
        case 512U:
            S->pTwiddle = dct512_twiddle;
            S->pCosFactor = dct512_cosFactor;
            break;

        case 256U:
            S->pTwiddle = dct256_twiddle;
            S->pCosFactor = dct256_cosFactor;
            break;

        case 128U:
            S->pTwiddle = dct128_twiddle;
            S->pCosFactor = dct128_cosFactor;
            break;

        case 64U:
            S->pTwiddle = dct64_twiddle;
            S->pCosFactor = dct64_cosFactor;
            break;

        case 32U:
            S->pTwiddle = dct32_twiddle;
            S->pCosFactor = dct32_cosFactor;
            break;

        case 16U:
            S->pTwiddle = dct16_twiddle;
            S->pCosFactor = dct16_cosFactor;
            break;

        default:
            return ARM_MATH_ARGUMENT_ERROR;
    }

    /* Initialize the RFFT/RIFFT Function */
    arm_rfft_init_q31(S->pRfft, S->N, 0, 1);

    return ARM_MATH_SUCCESS;
}

void arm_dct4_q31(const arm_dct4_instance_q31 *S, q31_t *pState, q31_t *pInlineBuffer)
{
    /* Calculate DCT-II for N-point input */
    uint16_t i;         /* Loop counter */
    q31_t *weights;     /* Pointer to the Weights table */
    q31_t *pOut;        /* Temporary pointers for output buffer */
    q31_t *pS1, *pbuff; /* Temporary pointers for input buffer and pState buffer */
    q31_t in;           /* Temporary variable */
    q31_t *cosFact;
    uint32_t length;
    uint8_t matRow;
    uint8_t matCol;
    uint8_t matLoop;
    uint16_t lenPerMatLoop;

    /*
     * Pre-processing involves multiplying input with cos factor,
     *     r(n) = 2 * u(n) * cos(pi*(2*n+1)/(4*N))
     *              where,
     *                 r(n) -- output of preprocessing
     *                 u(n) -- input to preprocessing(actual Source buffer)
     */

    /*
     * Use matrix production function for preprocessing. Matrix production
     * supports 16x16 at the most, so the matrix row is set to 16.
     */
    matRow = 16;
    lenPerMatLoop = S->N >= 256 ? 256 : S->N;
    matCol = lenPerMatLoop / 16;
    matLoop = ((S->N - 1) >> 8) + 1;
    cosFact = S->pCosFactor;
    pbuff = pInlineBuffer;

    length = POWERQUAD_MAKE_MATRIX_LEN(matRow, matCol, matCol);

    while (matLoop--)
    {
        PQ_SET_MAT_FIX32_WORKAROUND_SCALE_CONFIG;

        /* cos factor is Q31, convert to float */
        PQ_MatrixScale(POWERQUAD, length, 2.0f / 2147483648.0f, cosFact, (void *)0xE0000000);
        cosFact += lenPerMatLoop;

        PQ_WaitDone(POWERQUAD);

        /* Product. */
        PQ_SET_MAT_FIX32_WORKAROUND_MULT_CONFIG;

        PQ_MatrixProduct(POWERQUAD, length, pbuff, (void *)0xE0000000, pbuff);

        pbuff += lenPerMatLoop;

        PQ_WaitDone(POWERQUAD);
    }

    PQ_SET_FFT_Q31_CONFIG;

    PQ_TransformRDCT(POWERQUAD, S->N, pInlineBuffer, pState);

    /* ARM calculation while PQ is running. */
    /*
     * Use matrix production function for twiddle multiplication.
     * Matrix production supports 16x16 at the most. The total elements are 2*N;
     */
    lenPerMatLoop = S->N >= 128 ? 128 : S->N;
    matCol = lenPerMatLoop / 8;
    matLoop = ((S->N - 1) >> 7) + 1;
    weights = S->pTwiddle;
    pOut = pState;

    length = POWERQUAD_MAKE_MATRIX_LEN(matRow, matCol, matCol);

    PQ_WaitDone(POWERQUAD);

    while (matLoop--)
    {
        PQ_SET_MAT_FIX32_WORKAROUND_SCALE_CONFIG;

        /* Downscale by 1024 * 1024 * 16, because the twiddle are multiplied by 1024 * 1024 * 16. */
        PQ_MatrixScale(POWERQUAD, length, 1.0f / 16777216.0f, weights, (void *)0xE0000000);
        weights += lenPerMatLoop * 2;

        PQ_WaitDone(POWERQUAD);

        /* Product. */
        PQ_SET_MAT_FIX32_WORKAROUND_MULT_CONFIG;

        PQ_MatrixProduct(POWERQUAD, length, pOut, (void *)0xE0000000, pOut);

        PQ_WaitDone(POWERQUAD);

        for (i = 0; i < lenPerMatLoop / 4; i++)
        {
            *pOut -= *(pOut + 1);
            pOut += 2;
            *pOut -= *(pOut + 1);
            pOut += 2;
            *pOut -= *(pOut + 1);
            pOut += 2;
            *pOut -= *(pOut + 1);
            pOut += 2;
        }
    }

    /* ----------- Post-processing ---------- */
    /* DCT-IV can be obtained from DCT-II by the equation,
     *       Y4(k) = Y2(k) - Y4(k-1) and Y4(-1) = Y4(0)
     *       Hence, Y4(0) = Y2(0)/2  */
    /* Getting only real part from the output and Converting to DCT-IV */

    /* pbuff initialized to input buffer. */
    pbuff = pInlineBuffer;

    /* pS1 initialized to pState */
    pS1 = pState;

    /* Calculating Y4(0) from Y2(0) using Y4(0) = Y2(0)/2. Considering the DCT II normalize, here divided by sqrt(2).*/
    in = (q31_t)((float)*pS1 / 1.41421356237f);
    *pbuff++ = in;
    pS1 += 2;

    in = *pS1 - in;
    *pbuff++ = in;
    pS1 += 2;

    in = *pS1 - in;
    *pbuff++ = in;
    pS1 += 2;

    in = *pS1 - in;
    *pbuff++ = in;
    pS1 += 2;

    i = S->N / 4 - 1;

    while (i > 0U)
    {
        in = *pS1 - in;
        *pbuff++ = in;
        pS1 += 2;

        in = *pS1 - in;
        *pbuff++ = in;
        pS1 += 2;

        in = *pS1 - in;
        *pbuff++ = in;
        pS1 += 2;

        in = *pS1 - in;
        *pbuff++ = in;
        pS1 += 2;

        i--;
    }
}

arm_status arm_dct4_init_q15(arm_dct4_instance_q15 *S,
                             arm_rfft_instance_q15 *S_RFFT,
                             arm_cfft_radix4_instance_q15 *S_CFFT,
                             uint16_t N,
                             uint16_t Nby2,
                             q15_t normalize)
{
    /* Initialize the DCT4 length */
    S->N = N;

    /* Initialize Real FFT Instance */
    S->pRfft = S_RFFT;

    switch (N)
    {
        /* Initialize the table modifier values */
        case 512U:
            S->pTwiddle = (q15_t *)dct512_twiddle;
            S->pCosFactor = (q15_t *)dct512_cosFactor;
            break;

        case 256U:
            S->pTwiddle = (q15_t *)dct256_twiddle;
            S->pCosFactor = (q15_t *)dct256_cosFactor;
            break;

        case 128U:
            S->pTwiddle = (q15_t *)dct128_twiddle;
            S->pCosFactor = (q15_t *)dct128_cosFactor;
            break;

        case 64U:
            S->pTwiddle = (q15_t *)dct64_twiddle;
            S->pCosFactor = (q15_t *)dct64_cosFactor;
            break;

        case 32U:
            S->pTwiddle = (q15_t *)dct32_twiddle;
            S->pCosFactor = (q15_t *)dct32_cosFactor;
            break;

        case 16U:
            S->pTwiddle = (q15_t *)dct16_twiddle;
            S->pCosFactor = (q15_t *)dct16_cosFactor;
            break;

        default:
            return ARM_MATH_ARGUMENT_ERROR;
    }

    /* Initialize the RFFT/RIFFT Function */
    arm_rfft_init_q15(S->pRfft, S->N, 0, 1);

    /* return the status of DCT4 Init function */
    return ARM_MATH_SUCCESS;
}

void arm_dct4_q15(const arm_dct4_instance_q15 *S, q15_t *pState, q15_t *pInlineBuffer)
{
    /* Calculate DCT-II for N-point input */
    uint16_t i;         /* Loop counter */
    q15_t *weights;     /* Pointer to the Weights table */
    q15_t *pOut;        /* Temporary pointers for output buffer */
    q15_t *pS1, *pbuff; /* Temporary pointers for input buffer and pState buffer */
    q15_t in;           /* Temporary variable */
    q15_t *cosFact;
    uint32_t length;
    uint8_t matRow;
    uint8_t matCol;
    uint8_t matLoop;
    uint16_t lenPerMatLoop;

    /*
     * Pre-processing involves multiplying input with cos factor,
     *     r(n) = 2 * u(n) * cos(pi*(2*n+1)/(4*N))
     *              where,
     *                 r(n) -- output of preprocessing
     *                 u(n) -- input to preprocessing(actual Source buffer)
     */

    /*
     * Use matrix production function for preprocessing. Matrix production
     * supports 16x16 at the most, so the matrix row is set to 16.
     */
    matRow = 16;
    lenPerMatLoop = S->N >= 256 ? 256 : S->N;
    matCol = lenPerMatLoop / 16;
    matLoop = ((S->N - 1) >> 8) + 1;
    cosFact = S->pCosFactor;
    pbuff = pInlineBuffer;

    length = POWERQUAD_MAKE_MATRIX_LEN(matRow, matCol, 0);

    while (matLoop--)
    {
        PQ_SET_MAT_FIX32_WORKAROUND_SCALE_CONFIG;

        /* cos factor is Q31, convert to float */
        PQ_MatrixScale(POWERQUAD, length, 2.0f / 2147483648.0f, cosFact, (void *)0xE0000000);
        cosFact += 2 * lenPerMatLoop;

        PQ_WaitDone(POWERQUAD);

        /* Product. */
        POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float;
        POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float;
        POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float;
        POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float;
        POWERQUAD->TMPBASE = 0xE0000000;

        PQ_MatrixProduct(POWERQUAD, length, pbuff, (void *)0xE0000000, pbuff);

        PQ_WaitDone(POWERQUAD);

        pbuff += lenPerMatLoop;
    }

    PQ_SET_FFT_Q15_CONFIG;

    PQ_TransformRDCT(POWERQUAD, S->N, pInlineBuffer, pState);

    /* ARM calculation while PQ is running. */
    /*
     * Use matrix production function for twiddle multiplication.
     * Matrix production supports 16x16 at the most. The total elements are 2*N;
     */
    lenPerMatLoop = S->N >= 128 ? 128 : S->N;
    matCol = lenPerMatLoop / 8;
    matLoop = ((S->N - 1) >> 7) + 1;
    weights = S->pTwiddle;
    pOut = pState;

    length = POWERQUAD_MAKE_MATRIX_LEN(matRow, matCol, matCol);

    PQ_WaitDone(POWERQUAD);

    while (matLoop--)
    {
        PQ_SET_MAT_FIX32_WORKAROUND_SCALE_CONFIG;

        /* Downscale by 1024 * 1024 * 16, because the twiddle are multiplied by 1024 * 1024 * 16. */
        PQ_MatrixScale(POWERQUAD, length, 1.0f / 16777216.0f, weights, (void *)0xE0000000);
        weights += lenPerMatLoop * 2;

        PQ_WaitDone(POWERQUAD);

        /* Product. */
        POWERQUAD->OUTFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float;
        POWERQUAD->INAFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_16Bit << 4U) | kPQ_Float;
        POWERQUAD->INBFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float;
        POWERQUAD->TMPFORMAT = ((uint32_t)(0) << 8U) | ((uint32_t)kPQ_Float << 4U) | kPQ_Float;
        POWERQUAD->TMPBASE = 0xE0000000;

        PQ_MatrixProduct(POWERQUAD, length, pOut, (void *)0xE0000000, pOut);

        PQ_WaitDone(POWERQUAD);

        for (i = 0; i < lenPerMatLoop / 4; i++)
        {
            *pOut -= *(pOut + 1);
            pOut += 2;
            *pOut -= *(pOut + 1);
            pOut += 2;
            *pOut -= *(pOut + 1);
            pOut += 2;
            *pOut -= *(pOut + 1);
            pOut += 2;
        }
    }

    /* ----------- Post-processing ---------- */
    /* DCT-IV can be obtained from DCT-II by the equation,
     *       Y4(k) = Y2(k) - Y4(k-1) and Y4(-1) = Y4(0)
     *       Hence, Y4(0) = Y2(0)/2  */
    /* Getting only real part from the output and Converting to DCT-IV */

    /* pbuff initialized to input buffer. */
    pbuff = pInlineBuffer;

    /* pS1 initialized to pState */
    pS1 = pState;

    /* Calculating Y4(0) from Y2(0) using Y4(0) = Y2(0)/2. Considering the DCT II normalize, here divided by sqrt(2).*/
    in = (q15_t)((float)*pS1 / 1.41421356237f);
    *pbuff++ = in;
    pS1 += 2;

    in = *pS1 - in;
    *pbuff++ = in;
    pS1 += 2;

    in = *pS1 - in;
    *pbuff++ = in;
    pS1 += 2;

    in = *pS1 - in;
    *pbuff++ = in;
    pS1 += 2;

    i = S->N / 4 - 1;

    while (i > 0U)
    {
        in = *pS1 - in;
        *pbuff++ = in;
        pS1 += 2;

        in = *pS1 - in;
        *pbuff++ = in;
        pS1 += 2;

        in = *pS1 - in;
        *pbuff++ = in;
        pS1 += 2;

        in = *pS1 - in;
        *pbuff++ = in;
        pS1 += 2;

        i--;
    }
}

void arm_fir_init_f32(
    arm_fir_instance_f32 *S, uint16_t numTaps, float32_t *pCoeffs, float32_t *pState, uint32_t blockSize)
{
    uint32_t i;

    /*
     * CMSIS DSP API filter coefficients stored in time reversed order, but PQ
     * uses the positive order. PQ does not use pState, so pState pState[1:numTaps]
     * is used here to save the coefficients in positive order. At the same time,
     * pState[0] is used to save the offset used for incremetal calculation.
     * Because the length of pState is (numTaps + blockSize -1), to ensure enough space,
     * the blockSize should be larger than 1.
     */
    assert(blockSize > 1);

    S->numTaps = numTaps;
    S->pCoeffs = pCoeffs;
    S->pState = pState;

    for (i = 0; i < numTaps; i++)
    {
        pState[numTaps - i] = pCoeffs[i];
    }

    *(uint32_t *)pState = 0;
}

void arm_fir_init_q31(arm_fir_instance_q31 *S, uint16_t numTaps, q31_t *pCoeffs, q31_t *pState, uint32_t blockSize)
{
    uint32_t i;

    /*
     * CMSIS DSP API filter coefficients stored in time reversed order, but PQ
     * uses the positive order. PQ does not use pState, so pState pState[1:numTaps]
     * is used here to save the coefficients in positive order. At the same time,
     * pState[0] is used to save the offset used for incremetal calculation.
     * Because the length of pState is (numTaps + blockSize -1), to ensure enough space,
     * the blockSize should be larger than 1.
     */
    assert(blockSize > 1);

    S->numTaps = numTaps;
    S->pCoeffs = pCoeffs;
    S->pState = pState;

    for (i = 0; i < numTaps; i++)
    {
        pState[numTaps - i] = pCoeffs[i];
    }

    pState[0] = 0;
}

arm_status arm_fir_init_q15(
    arm_fir_instance_q15 *S, uint16_t numTaps, q15_t *pCoeffs, q15_t *pState, uint32_t blockSize)
{
    uint32_t i;

    /*
     * CMSIS DSP API filter coefficients stored in time reversed order, but PQ
     * uses the positive order. PQ does not use pState, so pState pState[2:numTaps]
     * is used here to save the coefficients in positive order. At the same time,
     * pState[0:1] is used to save the offset used for incremetal calculation.
     * Because the length of pState is (numTaps + blockSize -1), to ensure enough space,
     * the blockSize should be larger than 2.
     */
    assert(blockSize > 2);

    S->numTaps = numTaps;
    S->pCoeffs = pCoeffs;
    S->pState = pState;

    for (i = 0; i < numTaps; i++)
    {
        pState[numTaps + 1 - i] = pCoeffs[i];
    }

    *(uint32_t *)pState = 0;

    return ARM_MATH_SUCCESS;
}

void arm_fir_f32(const arm_fir_instance_f32 *S, float32_t *pSrc, float32_t *pDst, uint32_t blockSize)
{
    assert(S);
    assert(pSrc);
    assert(pDst);

    uint32_t curOffset;
    PQ_SET_F32_CONFIG;

    curOffset = *(uint32_t *)(S->pState);

    if (curOffset == 0)
    {
        PQ_FIR(POWERQUAD, pSrc, blockSize, &(S->pState[1]), S->numTaps, pDst, PQ_FIR_FIR);
    }
    else
    {
        _arm_fir_increment(pSrc - curOffset, blockSize, &S->pState[1], S->numTaps, pDst - curOffset, curOffset);
    }

    *(uint32_t *)(S->pState) = curOffset + blockSize;

    PQ_WaitDone(POWERQUAD);
}

void arm_fir_q31(const arm_fir_instance_q31 *S, q31_t *pSrc, q31_t *pDst, uint32_t blockSize)
{
    assert(S);
    assert(pSrc);
    assert(pDst);

    uint32_t curOffset;
    PQ_SET_Q31_CONFIG;

    curOffset = *(uint32_t *)(S->pState);

    if (curOffset == 0)
    {
        PQ_FIR(POWERQUAD, pSrc, blockSize, &(S->pState[1]), S->numTaps, pDst, PQ_FIR_FIR);
    }
    else
    {
        _arm_fir_increment(pSrc - curOffset, blockSize, &S->pState[1], S->numTaps, pDst - curOffset, curOffset);
    }

    *(uint32_t *)(S->pState) = curOffset + blockSize;

    PQ_WaitDone(POWERQUAD);
}

void arm_fir_q15(const arm_fir_instance_q15 *S, q15_t *pSrc, q15_t *pDst, uint32_t blockSize)
{
    assert(S);
    assert(pSrc);
    assert(pDst);

    uint32_t curOffset;

    PQ_SET_Q15_CONFIG;

    curOffset = *(uint32_t *)(S->pState);

    if (curOffset == 0)
    {
        PQ_FIR(POWERQUAD, pSrc, blockSize, &(S->pState[2]), S->numTaps, pDst, PQ_FIR_FIR);
    }
    else
    {
        _arm_fir_increment(pSrc - curOffset, blockSize, &S->pState[2], S->numTaps, pDst - curOffset, curOffset);
    }

    *(uint32_t *)(S->pState) = curOffset + blockSize;

    PQ_WaitDone(POWERQUAD);
}

void arm_conv_f32(float32_t *pSrcA, uint32_t srcALen, float32_t *pSrcB, uint32_t srcBLen, float32_t *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    PQ_SET_F32_CONFIG;

    PQ_FIR(POWERQUAD, pSrcA, srcALen, pSrcB, srcBLen, pDst, PQ_FIR_CONVOLUTION);
    PQ_WaitDone(POWERQUAD);
}

void arm_conv_q31(q31_t *pSrcA, uint32_t srcALen, q31_t *pSrcB, uint32_t srcBLen, q31_t *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    PQ_SET_Q31_CONFIG;

    PQ_FIR(POWERQUAD, pSrcA, srcALen, pSrcB, srcBLen, pDst, PQ_FIR_CONVOLUTION);
    PQ_WaitDone(POWERQUAD);
}

void arm_conv_q15(q15_t *pSrcA, uint32_t srcALen, q15_t *pSrcB, uint32_t srcBLen, q15_t *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    PQ_SET_Q15_CONFIG;

    PQ_FIR(POWERQUAD, pSrcA, srcALen, pSrcB, srcBLen, pDst, PQ_FIR_CONVOLUTION);
    PQ_WaitDone(POWERQUAD);
}

void arm_correlate_f32(float32_t *pSrcA, uint32_t srcALen, float32_t *pSrcB, uint32_t srcBLen, float32_t *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    PQ_SET_F32_CONFIG;

    PQ_FIR(POWERQUAD, pSrcA, srcALen, pSrcB, srcBLen, pDst, PQ_FIR_CORRELATION);
    PQ_WaitDone(POWERQUAD);
}

void arm_correlate_q31(q31_t *pSrcA, uint32_t srcALen, q31_t *pSrcB, uint32_t srcBLen, q31_t *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    PQ_SET_Q31_CONFIG;

    PQ_FIR(POWERQUAD, pSrcA, srcALen, pSrcB, srcBLen, pDst, PQ_FIR_CORRELATION);
    PQ_WaitDone(POWERQUAD);
}

void arm_correlate_q15(q15_t *pSrcA, uint32_t srcALen, q15_t *pSrcB, uint32_t srcBLen, q15_t *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    PQ_SET_Q15_CONFIG;

    PQ_FIR(POWERQUAD, pSrcA, srcALen, pSrcB, srcBLen, pDst, PQ_FIR_CORRELATION);
    PQ_WaitDone(POWERQUAD);
}

void arm_mat_init_f32(arm_matrix_instance_f32 *S, uint16_t nRows, uint16_t nColumns, float32_t *pData)
{
    /* Assign Number of Rows */
    S->numRows = nRows;

    /* Assign Number of Columns */
    S->numCols = nColumns;

    /* Assign Data pointer */
    S->pData = pData;
}

void arm_mat_init_q31(arm_matrix_instance_q31 *S, uint16_t nRows, uint16_t nColumns, q31_t *pData)
{
    /* Assign Number of Rows */
    S->numRows = nRows;

    /* Assign Number of Columns */
    S->numCols = nColumns;

    /* Assign Data pointer */
    S->pData = pData;
}

void arm_mat_init_q15(arm_matrix_instance_q15 *S, uint16_t nRows, uint16_t nColumns, q15_t *pData)
{
    /* Assign Number of Rows */
    S->numRows = nRows;

    /* Assign Number of Columns */
    S->numCols = nColumns;

    /* Assign Data pointer */
    S->pData = pData;
}

arm_status arm_mat_add_f32(const arm_matrix_instance_f32 *pSrcA,
                           const arm_matrix_instance_f32 *pSrcB,
                           arm_matrix_instance_f32 *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrcA->numRows != pSrcB->numRows) || (pSrcA->numCols != pSrcB->numCols) || (pSrcA->numRows != pDst->numRows) ||
        (pSrcA->numCols != pDst->numCols))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_F32_CONFIG;

        length = (pSrcB->numCols << 16) | (pSrcA->numCols << 8) | (pSrcA->numRows << 0);

        PQ_MatrixAddition(POWERQUAD, length, pSrcA->pData, pSrcB->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_add_q31(const arm_matrix_instance_q31 *pSrcA,
                           const arm_matrix_instance_q31 *pSrcB,
                           arm_matrix_instance_q31 *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrcA->numRows != pSrcB->numRows) || (pSrcA->numCols != pSrcB->numCols) || (pSrcA->numRows != pDst->numRows) ||
        (pSrcA->numCols != pDst->numCols))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_FIX32_CONFIG;

        length = (pSrcB->numCols << 16) | (pSrcA->numCols << 8) | (pSrcA->numRows << 0);

        PQ_MatrixAddition(POWERQUAD, length, pSrcA->pData, pSrcB->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_add_q15(const arm_matrix_instance_q15 *pSrcA,
                           const arm_matrix_instance_q15 *pSrcB,
                           arm_matrix_instance_q15 *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrcA->numRows != pSrcB->numRows) || (pSrcA->numCols != pSrcB->numCols) || (pSrcA->numRows != pDst->numRows) ||
        (pSrcA->numCols != pDst->numCols))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_FIX16_CONFIG;

        length = (pSrcB->numCols << 16) | (pSrcA->numCols << 8) | (pSrcA->numRows << 0);

        PQ_MatrixAddition(POWERQUAD, length, pSrcA->pData, pSrcB->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_sub_f32(const arm_matrix_instance_f32 *pSrcA,
                           const arm_matrix_instance_f32 *pSrcB,
                           arm_matrix_instance_f32 *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrcA->numRows != pSrcB->numRows) || (pSrcA->numCols != pSrcB->numCols) || (pSrcA->numRows != pDst->numRows) ||
        (pSrcA->numCols != pDst->numCols))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_F32_CONFIG;

        length = (pSrcB->numCols << 16) | (pSrcA->numCols << 8) | (pSrcA->numRows << 0);

        PQ_MatrixSubtraction(POWERQUAD, length, pSrcA->pData, pSrcB->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_sub_q31(const arm_matrix_instance_q31 *pSrcA,
                           const arm_matrix_instance_q31 *pSrcB,
                           arm_matrix_instance_q31 *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrcA->numRows != pSrcB->numRows) || (pSrcA->numCols != pSrcB->numCols) || (pSrcA->numRows != pDst->numRows) ||
        (pSrcA->numCols != pDst->numCols))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_FIX32_CONFIG;

        length = (pSrcB->numCols << 16) | (pSrcA->numCols << 8) | (pSrcA->numRows << 0);

        PQ_MatrixSubtraction(POWERQUAD, length, pSrcA->pData, pSrcB->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_sub_q15(const arm_matrix_instance_q15 *pSrcA,
                           const arm_matrix_instance_q15 *pSrcB,
                           arm_matrix_instance_q15 *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrcA->numRows != pSrcB->numRows) || (pSrcA->numCols != pSrcB->numCols) || (pSrcA->numRows != pDst->numRows) ||
        (pSrcA->numCols != pDst->numCols))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_FIX16_CONFIG;

        length = (pSrcB->numCols << 16) | (pSrcA->numCols << 8) | (pSrcA->numRows << 0);

        PQ_MatrixSubtraction(POWERQUAD, length, pSrcA->pData, pSrcB->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_mult_f32(const arm_matrix_instance_f32 *pSrcA,
                            const arm_matrix_instance_f32 *pSrcB,
                            arm_matrix_instance_f32 *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrcA->numRows != pSrcB->numRows) || (pSrcA->numCols != pSrcB->numCols) || (pSrcA->numRows != pDst->numRows) ||
        (pSrcA->numCols != pDst->numCols))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_F32_CONFIG;

        length = (pSrcB->numCols << 16) | (pSrcA->numCols << 8) | (pSrcA->numRows << 0);

        PQ_MatrixMultiplication(POWERQUAD, length, pSrcA->pData, pSrcB->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_mult_q31(const arm_matrix_instance_q31 *pSrcA,
                            const arm_matrix_instance_q31 *pSrcB,
                            arm_matrix_instance_q31 *pDst)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    arm_status status;
    q31_t length;

/*
 * The output prescale does not supprt negative value due to hardware issue,
 * workaround:
 * 1. Downscale the matrix B and save the float output value to private memory.
 * 2. Multiply the float matrix B in private memory with matrix A, output as Q31.
 * Note: Put matrix B in private memory is faster.
 */

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrcA->numRows != pSrcB->numRows) || (pSrcA->numCols != pSrcB->numCols) || (pSrcA->numRows != pDst->numRows) ||
        (pSrcA->numCols != pDst->numCols))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        length = (pSrcB->numCols << 8) | (pSrcB->numRows << 0);

        PQ_SET_MAT_FIX32_WORKAROUND_SCALE_CONFIG;

        /* Downscale. */
        PQ_MatrixScale(POWERQUAD, length, 1.0f / 2147483648.0f, pSrcB->pData, (void *)0xE0000000);

        length = (pSrcB->numCols << 16) | (pSrcA->numCols << 8) | (pSrcA->numRows << 0);

        PQ_WaitDone(POWERQUAD);

        PQ_SET_MAT_FIX32_WORKAROUND_MULT_CONFIG;

        PQ_MatrixMultiplication(POWERQUAD, length, pSrcA->pData, (void *)0xE0000000, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_mult_q15(const arm_matrix_instance_q15 *pSrcA,
                            const arm_matrix_instance_q15 *pSrcB,
                            arm_matrix_instance_q15 *pDst,
                            q15_t *pState)
{
    assert(pSrcA);
    assert(pSrcB);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrcA->numRows != pSrcB->numRows) || (pSrcA->numCols != pSrcB->numCols) || (pSrcA->numRows != pDst->numRows) ||
        (pSrcA->numCols != pDst->numCols))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        length = (pSrcB->numCols << 8) | (pSrcB->numRows << 0);

        PQ_SET_MAT_FIX16_WORKAROUND_SCALE_CONFIG;

        /* Downscale. */
        PQ_MatrixScale(POWERQUAD, length, 1.0f / 32768.0f, pSrcB->pData, (void *)0xE0000000);

        length = (pSrcB->numCols << 16) | (pSrcA->numCols << 8) | (pSrcA->numRows << 0);

        PQ_WaitDone(POWERQUAD);

        PQ_SET_MAT_FIX16_WORKAROUND_MULT_CONFIG;

        PQ_MatrixMultiplication(POWERQUAD, length, pSrcA->pData, (void *)0xE0000000, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_inverse_f32(const arm_matrix_instance_f32 *pSrc, arm_matrix_instance_f32 *pDst)
{
    assert(pSrc);
    assert(pDst);

    arm_status status;
    q31_t length;
    float tmp[1024];

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrc->numRows != pSrc->numCols) || (pDst->numRows != pDst->numCols) || (pSrc->numRows != pDst->numRows))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_F32_CONFIG;

        length = (pSrc->numRows << 16) | (pSrc->numRows << 8) | (pSrc->numRows << 0);

        PQ_MatrixInversion(POWERQUAD, length, pSrc->pData, tmp, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_trans_f32(const arm_matrix_instance_f32 *pSrc, arm_matrix_instance_f32 *pDst)
{
    assert(pSrc);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrc->numRows != pSrc->numCols) || (pDst->numRows != pDst->numCols) || (pSrc->numRows != pDst->numRows))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        length = (pSrc->numCols << 8) | (pSrc->numRows << 0);

        PQ_SetFormat(POWERQUAD, kPQ_CP_MTX, kPQ_Float);

        PQ_MatrixTranspose(POWERQUAD, length, pSrc->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);
        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_trans_q31(const arm_matrix_instance_q31 *pSrc, arm_matrix_instance_q31 *pDst)
{
    assert(pSrc);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrc->numRows != pSrc->numCols) || (pDst->numRows != pDst->numCols) || (pSrc->numRows != pDst->numRows))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_FIX32_CONFIG;

        length = (pSrc->numCols << 8) | (pSrc->numRows << 0);

        PQ_MatrixTranspose(POWERQUAD, length, pSrc->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_trans_q15(const arm_matrix_instance_q15 *pSrc, arm_matrix_instance_q15 *pDst)
{
    assert(pSrc);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrc->numRows != pSrc->numCols) || (pDst->numRows != pDst->numCols) || (pSrc->numRows != pDst->numRows))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_FIX16_CONFIG;

        length = (pSrc->numCols << 8) | (pSrc->numRows << 0);

        PQ_MatrixTranspose(POWERQUAD, length, pSrc->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_scale_f32(const arm_matrix_instance_f32 *pSrc, float32_t scale, arm_matrix_instance_f32 *pDst)
{
    assert(pSrc);
    assert(pDst);

    arm_status status;
    q31_t length;

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrc->numRows != pSrc->numCols) || (pDst->numRows != pDst->numCols) || (pSrc->numRows != pDst->numRows))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        PQ_SET_F32_CONFIG;

        length = (pSrc->numCols << 8) | (pSrc->numRows << 0);

        PQ_MatrixScale(POWERQUAD, length, scale, pSrc->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_scale_q31(const arm_matrix_instance_q31 *pSrc,
                             q31_t scale,
                             int32_t shift,
                             arm_matrix_instance_q31 *pDst)
{
    assert(pSrc);
    assert(pDst);

    arm_status status;
    q31_t length;
    float scaleFloat;

    pq_config_t config = {
        kPQ_32Bit,          /* inputAFormat   */
        0,                  /* inputAPrescale */
        kPQ_32Bit,          /* inputBFormat   */
        0,                  /* inputBPrescale */
        kPQ_32Bit,          /* outputFormat   */
        shift,              /* outputPrescale */
        kPQ_Float,          /* tmpFormat      */
        0,                  /* tmpPrescale    */
        kPQ_Float,          /* machineFormat  */
        (void *)0xe0000000, /* tmpBase        */
    };

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrc->numRows != pSrc->numCols) || (pDst->numRows != pDst->numCols) || (pSrc->numRows != pDst->numRows))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        length = (pSrc->numCols << 8) | (pSrc->numRows << 0);

        scaleFloat = PQ_Q31_2_FLOAT(scale);

        PQ_SetConfig(POWERQUAD, &config);

        PQ_MatrixScale(POWERQUAD, length, scaleFloat, pSrc->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}

arm_status arm_mat_scale_q15(const arm_matrix_instance_q15 *pSrc,
                             q15_t scale,
                             int32_t shift,
                             arm_matrix_instance_q15 *pDst)
{
    assert(pSrc);
    assert(pDst);

    arm_status status;
    q31_t length;
    float scaleFloat;

    pq_config_t config = {
        kPQ_16Bit,          /* inputAFormat   */
        0,                  /* inputAPrescale */
        kPQ_16Bit,          /* inputBFormat   */
        0,                  /* inputBPrescale */
        kPQ_16Bit,          /* outputFormat   */
        shift,              /* outputPrescale */
        kPQ_Float,          /* tmpFormat      */
        0,                  /* tmpPrescale    */
        kPQ_Float,          /* machineFormat  */
        (void *)0xe0000000, /* tmpBase        */
    };

#ifdef ARM_MATH_MATRIX_CHECK
    /* Check for matrix mismatch condition */
    if ((pSrc->numRows != pSrc->numCols) || (pDst->numRows != pDst->numCols) || (pSrc->numRows != pDst->numRows))
    {
        /* Set status as ARM_MATH_SIZE_MISMATCH */
        status = ARM_MATH_SIZE_MISMATCH;
    }
    else
#endif
    {
        length = (pSrc->numCols << 8) | (pSrc->numRows << 0);

        scaleFloat = PQ_Q15_2_FLOAT(scale);

        PQ_SetConfig(POWERQUAD, &config);

        PQ_MatrixScale(POWERQUAD, length, scaleFloat, pSrc->pData, pDst->pData);

        /* Wait for the completion */
        PQ_WaitDone(POWERQUAD);

        status = ARM_MATH_SUCCESS;
    }

    /* Return to application */
    return status;
}
