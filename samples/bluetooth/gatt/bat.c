/*
Copyright (c) 2012, Vinayak Kariappa Chettimada
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "nrf.h"

#include "bat.h"

#ifdef NRF51

#define BAT_VBG         (1200)
#define BAT_PRESCALER   (3)
#define BAT_DIODE_DROP  (270)

const uint16_t c_bat_levels_cr2032[BAT_LEVELS_CR2023] = {2800, 2700, 2600, 2500};
static volatile uint32_t gs_result;

void bat_acquire(void)
{
  NRF_ADC->CONFIG = (ADC_CONFIG_RES_8bit                        << ADC_CONFIG_RES_Pos)
                    | (ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos)
                    | (ADC_CONFIG_REFSEL_VBG                      << ADC_CONFIG_REFSEL_Pos)
                    | (ADC_CONFIG_PSEL_Disabled                   << ADC_CONFIG_PSEL_Pos)
                    | (ADC_CONFIG_EXTREFSEL_None                  << ADC_CONFIG_EXTREFSEL_Pos);

  NRF_ADC->ENABLE = 1;

  NRF_ADC->INTENSET = ADC_INTENSET_END_Msk;

  NVIC_SetPriority(ADC_IRQn, 3);
  NVIC_EnableIRQ(ADC_IRQn);

  NRF_ADC->EVENTS_END = 0;
  NRF_ADC->TASKS_START = 1;
}

uint32_t bat_get(void)
{
  /** @todo may be use NRF_ADC->BUSY? */

  return (((gs_result * BAT_VBG * BAT_PRESCALER) >> 8) + BAT_DIODE_DROP);
}

uint8_t bat_level_get(uint8_t levels, uint16_t const * const p_levels)
{
  uint8_t index;
  uint16_t mv;

  mv = bat_get();

  index = levels;
  while (index)
  {
    index--;
    if (mv < p_levels[index])
    {
      index++;
      break;
    }
  }

  return(100 - (index * (100 / (levels + 1))));
}

void C_ADC_IRQHandler(void)
{
  if (NRF_ADC->EVENTS_END != 0)
  {
    NRF_ADC->EVENTS_END = 0;

    gs_result = NRF_ADC->RESULT;

    NRF_ADC->TASKS_STOP = 1;
    NRF_ADC->ENABLE = 0;

    //NVIC_DisableIRQ(ADC_IRQn);
    //NRF_ADC->INTENCLR = ADC_INTENSET_END_Msk;
  }
}

#endif

