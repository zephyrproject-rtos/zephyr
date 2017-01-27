/* Copyright (c) 2014-2017 Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @file
 * @brief ADC HAL implementation
 */

#include "nrf_adc.h"

#ifdef ADC_PRESENT

/**
 * @brief Function for configuring ADC.
 *
 * This function powers on ADC and configures it. ADC is in DISABLE state after configuration,
 * so it should be enabled before using it.
 *
 * @param[in] config  Requested configuration.
 */
void nrf_adc_configure(nrf_adc_config_t * config)
{
    uint32_t config_reg = 0;

    config_reg |= ((uint32_t)config->resolution << ADC_CONFIG_RES_Pos) & ADC_CONFIG_RES_Msk;
    config_reg |= ((uint32_t)config->scaling << ADC_CONFIG_INPSEL_Pos) & ADC_CONFIG_INPSEL_Msk;
    config_reg |= ((uint32_t)config->reference << ADC_CONFIG_REFSEL_Pos) & ADC_CONFIG_REFSEL_Msk;

    if (config->reference & ADC_CONFIG_EXTREFSEL_Msk)
    {
        config_reg |= config->reference & ADC_CONFIG_EXTREFSEL_Msk;
    }

    /* select input */
    nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_DISABLED);

    /* set new configuration keeping selected input */
    NRF_ADC->CONFIG = config_reg | (NRF_ADC->CONFIG & ADC_CONFIG_PSEL_Msk);
}


/**
 * @brief Blocking function for executing single ADC conversion.
 *
 * This function selects the desired input, starts a single conversion,
 * waits for it to finish, and returns the result.
 * ADC is left in STOP state, the given input is selected.
 * This function does not check if ADC is initialized and powered.
 *
 * @param[in] input Requested input to be selected.
 *
 * @return Conversion result
 */
int32_t nrf_adc_convert_single(nrf_adc_config_input_t input)
{
    int32_t val;

    nrf_adc_input_select(input);
    nrf_adc_start();

    while (!nrf_adc_conversion_finished())
    {
    }
    nrf_adc_conversion_event_clean();
    val = nrf_adc_result_get();
    nrf_adc_stop();
    return val;
}
#endif
