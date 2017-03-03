/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_rnga.h"

#if defined(FSL_FEATURE_SOC_RNG_COUNT) && FSL_FEATURE_SOC_RNG_COUNT

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*******************************************************************************
 * RNG_CR - RNGA Control Register
 ******************************************************************************/
/*!
 * @brief RNG_CR - RNGA Control Register (RW)
 *
 * Reset value: 0x00000000U
 *
 * Controls the operation of RNGA.
 */
/*!
 * @name Constants and macros for entire RNG_CR register
 */
/*@{*/
#define RNG_CR_REG(base) ((base)->CR)
#define RNG_RD_CR(base) (RNG_CR_REG(base))
#define RNG_WR_CR(base, value) (RNG_CR_REG(base) = (value))
#define RNG_RMW_CR(base, mask, value) (RNG_WR_CR(base, (RNG_RD_CR(base) & ~(mask)) | (value)))
/*@}*/

/*!
 * @name Register RNG_CR, field GO[0] (RW)
 *
 * Specifies whether random-data generation and loading (into OR[RANDOUT]) is
 * enabled.This field is sticky. You must reset RNGA to stop RNGA from loading
 * OR[RANDOUT] with data.
 *
 * Values:
 * - 0b0 - Disabled
 * - 0b1 - Enabled
 */
/*@{*/
/*! @brief Read current value of the RNG_CR_GO field. */
#define RNG_RD_CR_GO(base) ((RNG_CR_REG(base) & RNG_CR_GO_MASK) >> RNG_CR_GO_SHIFT)

/*! @brief Set the GO field to a new value. */
#define RNG_WR_CR_GO(base, value) (RNG_RMW_CR(base, RNG_CR_GO_MASK, RNG_CR_GO(value)))
/*@}*/

/*!
 * @name Register RNG_CR, field SLP[4] (RW)
 *
 * Specifies whether RNGA is in Sleep or Normal mode. You can also enter Sleep
 * mode by asserting the DOZE signal.
 *
 * Values:
 * - 0b0 - Normal mode
 * - 0b1 - Sleep (low-power) mode
 */
/*@{*/
/*! @brief Read current value of the RNG_CR_SLP field. */
#define RNG_RD_CR_SLP(base) ((RNG_CR_REG(base) & RNG_CR_SLP_MASK) >> RNG_CR_SLP_SHIFT)

/*! @brief Set the SLP field to a new value. */
#define RNG_WR_CR_SLP(base, value) (RNG_RMW_CR(base, RNG_CR_SLP_MASK, RNG_CR_SLP(value)))
/*@}*/

/*******************************************************************************
 * RNG_SR - RNGA Status Register
 ******************************************************************************/
#define RNG_SR_REG(base) ((base)->SR)

/*!
 * @name Register RNG_SR, field OREG_LVL[15:8] (RO)
 *
 * Indicates the number of random-data words that are in OR[RANDOUT], which
 * indicates whether OR[RANDOUT] is valid.If you read OR[RANDOUT] when SR[OREG_LVL]
 * is not 0, then the contents of a random number contained in OR[RANDOUT] are
 * returned, and RNGA writes 0 to both OR[RANDOUT] and SR[OREG_LVL].
 *
 * Values:
 * - 0b00000000 - No words (empty)
 * - 0b00000001 - One word (valid)
 */
/*@{*/
/*! @brief Read current value of the RNG_SR_OREG_LVL field. */
#define RNG_RD_SR_OREG_LVL(base) ((RNG_SR_REG(base) & RNG_SR_OREG_LVL_MASK) >> RNG_SR_OREG_LVL_SHIFT)
/*@}*/

/*!
 * @name Register RNG_SR, field SLP[4] (RO)
 *
 * Specifies whether RNGA is in Sleep or Normal mode. You can also enter Sleep
 * mode by asserting the DOZE signal.
 *
 * Values:
 * - 0b0 - Normal mode
 * - 0b1 - Sleep (low-power) mode
 */
/*@{*/
/*! @brief Read current value of the RNG_SR_SLP field. */
#define RNG_RD_SR_SLP(base) ((RNG_SR_REG(base) & RNG_SR_SLP_MASK) >> RNG_SR_SLP_SHIFT)
/*@}*/

/*******************************************************************************
 * RNG_OR - RNGA Output Register
 ******************************************************************************/
/*!
 * @brief RNG_OR - RNGA Output Register (RO)
 *
 * Reset value: 0x00000000U
 *
 * Stores a random-data word generated by RNGA.
 */
/*!
 * @name Constants and macros for entire RNG_OR register
 */
/*@{*/
#define RNG_OR_REG(base) ((base)->OR)
#define RNG_RD_OR(base) (RNG_OR_REG(base))
/*@}*/

/*******************************************************************************
 * RNG_ER - RNGA Entropy Register
 ******************************************************************************/
/*!
 * @brief RNG_ER - RNGA Entropy Register (WORZ)
 *
 * Reset value: 0x00000000U
 *
 * Specifies an entropy value that RNGA uses in addition to its ring oscillators
 * to seed its pseudorandom algorithm. This is a write-only register; reads
 * return all zeros.
 */
/*!
 * @name Constants and macros for entire RNG_ER register
 */
/*@{*/
#define RNG_ER_REG(base) ((base)->ER)
#define RNG_RD_ER(base) (RNG_ER_REG(base))
#define RNG_WR_ER(base, value) (RNG_ER_REG(base) = (value))
/*@}*/

/*******************************************************************************
 * Prototypes
 *******************************************************************************/

static uint32_t rnga_ReadEntropy(RNG_Type *base);

/*******************************************************************************
 * Code
 ******************************************************************************/

void RNGA_Init(RNG_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable the clock gate. */
    CLOCK_EnableClock(kCLOCK_Rnga0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    CLOCK_DisableClock(kCLOCK_Rnga0); /* To solve the release version on twrkm43z75m */
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(kCLOCK_Rnga0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Reset the registers for RNGA module to reset state. */
    RNG_WR_CR(base, 0);
    /* Enables the RNGA random data generation and loading.*/
    RNG_WR_CR_GO(base, 1);
}

void RNGA_Deinit(RNG_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable the clock for RNGA module.*/
    CLOCK_DisableClock(kCLOCK_Rnga0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

/*!
 * @brief Get a random data from RNGA.
 *
 * @param base RNGA base address
 */
static uint32_t rnga_ReadEntropy(RNG_Type *base)
{
    uint32_t data = 0;
    if (RNGA_GetMode(base) == kRNGA_ModeNormal) /* Is in normal mode.*/
    {
        /* Wait for valid random-data.*/
        while (RNG_RD_SR_OREG_LVL(base) == 0)
        {
        }
        data = RNG_RD_OR(base);
    }
    /* Get random-data word generated by RNGA.*/
    return data;
}

status_t RNGA_GetRandomData(RNG_Type *base, void *data, size_t data_size)
{
    status_t result = kStatus_Success;
    uint32_t random_32;
    uint8_t *random_p;
    uint32_t random_size;
    uint8_t *data_p = (uint8_t *)data;
    uint32_t i;

    /* Check input parameters.*/
    if (base && data && data_size)
    {
        do
        {
            /* Read Entropy.*/
            random_32 = rnga_ReadEntropy(base);

            random_p = (uint8_t *)&random_32;

            if (data_size < sizeof(random_32))
            {
                random_size = data_size;
            }
            else
            {
                random_size = sizeof(random_32);
            }

            for (i = 0; i < random_size; i++)
            {
                *data_p++ = *random_p++;
            }

            data_size -= random_size;
        } while (data_size > 0);
    }
    else
    {
        result = kStatus_InvalidArgument;
    }

    return result;
}

void RNGA_SetMode(RNG_Type *base, rnga_mode_t mode)
{
    RNG_WR_CR_SLP(base, (uint32_t)mode);
}

rnga_mode_t RNGA_GetMode(RNG_Type *base)
{
    return (rnga_mode_t)RNG_RD_SR_SLP(base);
}

void RNGA_Seed(RNG_Type *base, uint32_t seed)
{
    /* Write to RNGA Entropy Register.*/
    RNG_WR_ER(base, seed);
}

#endif /* FSL_FEATURE_SOC_RNG_COUNT */
