/*

Copyright (c) 2009-2018 ARM Limited. All rights reserved.

    SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the License); you may
not use this file except in compliance with the License.
You may obtain a copy of the License at

    www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an AS IS BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

NOTICE: This file has been modified by Nordic Semiconductor ASA.

*/

/* NOTE: Template files (including this one) are application specific and therefore expected to
   be copied into the application project folder prior to its use! */

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "system_nrf52811.h"

/*lint ++flb "Enter library region" */

#define __SYSTEM_CLOCK_64M      (64000000UL)

static bool errata_31(void);
static bool errata_36(void);
static bool errata_66(void);
static bool errata_108(void);
static bool errata_136(void);

/* nRF52840 erratas */
#ifdef DEVELOP_IN_NRF52840
    static bool errata_103(void);
    static bool errata_115(void);
#endif

#if defined ( __CC_ARM )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK_64M;
#elif defined ( __ICCARM__ )
    __root uint32_t SystemCoreClock = __SYSTEM_CLOCK_64M;
#elif defined ( __GNUC__ )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK_64M;
#endif

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = __SYSTEM_CLOCK_64M;
}

void SystemInit(void)
{
    /* Workaround for Errata 31 "CLOCK: Calibration values are not correctly loaded from FICR at reset" found at the Errata document
       for your device located at https://www.nordicsemi.com/DocLib */
    if (errata_31()){
        *(volatile uint32_t *)0x4000053C = ((*(volatile uint32_t *)0x10000244) & 0x0000E000) >> 13;
    }

    /* Workaround for Errata 36 "CLOCK: Some registers are not reset when expected" found at the Errata document
       for your device located at https://www.nordicsemi.com/DocLib  */
    if (errata_36()){
        NRF_CLOCK->EVENTS_DONE = 0;
        NRF_CLOCK->EVENTS_CTTO = 0;
        NRF_CLOCK->CTIV = 0;
    }
    
    /* Workaround for Errata 66 "TEMP: Linearity specification not met with default settings" found at the Errata document
       for your device located at https://www.nordicsemi.com/DocLib  */
    if (errata_66()){
        NRF_TEMP->A0 = NRF_FICR->TEMP.A0;
        NRF_TEMP->A1 = NRF_FICR->TEMP.A1;
        NRF_TEMP->A2 = NRF_FICR->TEMP.A2;
        NRF_TEMP->A3 = NRF_FICR->TEMP.A3;
        NRF_TEMP->A4 = NRF_FICR->TEMP.A4;
        NRF_TEMP->A5 = NRF_FICR->TEMP.A5;
        NRF_TEMP->B0 = NRF_FICR->TEMP.B0;
        NRF_TEMP->B1 = NRF_FICR->TEMP.B1;
        NRF_TEMP->B2 = NRF_FICR->TEMP.B2;
        NRF_TEMP->B3 = NRF_FICR->TEMP.B3;
        NRF_TEMP->B4 = NRF_FICR->TEMP.B4;
        NRF_TEMP->B5 = NRF_FICR->TEMP.B5;
        NRF_TEMP->T0 = NRF_FICR->TEMP.T0;
        NRF_TEMP->T1 = NRF_FICR->TEMP.T1;
        NRF_TEMP->T2 = NRF_FICR->TEMP.T2;
        NRF_TEMP->T3 = NRF_FICR->TEMP.T3;
        NRF_TEMP->T4 = NRF_FICR->TEMP.T4;
    }

    #ifdef DEVELOP_IN_NRF52840

        /* Workaround for Errata 103 "CCM: Wrong reset value of CCM MAXPACKETSIZE" found at the Errata document
           for your device located at https://www.nordicsemi.com/DocLib  */
        if (errata_103()){
            NRF_CCM->MAXPACKETSIZE = 0xFBul;
        }

        /* Workaround for Errata 115 "RAM: RAM content cannot be trusted upon waking up from System ON Idle or System OFF mode" found at the Errata document
           for your device located at https://www.nordicsemi.com/DocLib  */
        if (errata_115()){
            *(volatile uint32_t *)0x40000EE4 = (*(volatile uint32_t *)0x40000EE4 & 0xFFFFFFF0) | (*(uint32_t *)0x10000258 & 0x0000000F);
        }
    #endif

    /* Workaround for Errata 108 "RAM: RAM content cannot be trusted upon waking up from System ON Idle or System OFF mode" found at the Errata document
       for your device located at https://www.nordicsemi.com/DocLib  */
    if (errata_108()){
        *(volatile uint32_t *)0x40000EE4 = *(volatile uint32_t *)0x10000258 & 0x0000004F;
    }
    
    /* Workaround for Errata 136 "System: Bits in RESETREAS are set when they should not be" found at the Errata document
       for your device located at https://www.nordicsemi.com/DocLib  */
    if (errata_136()){
        if (NRF_POWER->RESETREAS & POWER_RESETREAS_RESETPIN_Msk){
            NRF_POWER->RESETREAS =  ~POWER_RESETREAS_RESETPIN_Msk;
        }
    }
    
    /* Configure GPIO pads as pPin Reset pin if Pin Reset capabilities desired. If CONFIG_GPIO_AS_PINRESET is not
      defined, pin reset will not be available. One GPIO (see Product Specification to see which one) will then be
      reserved for PinReset and not available as normal GPIO. */
    #if defined (CONFIG_GPIO_AS_PINRESET)

        #ifdef DEVELOP_IN_NRF52840
            #define RESET_PIN 18
        #else
            #define RESET_PIN 21
        #endif

        if (((NRF_UICR->PSELRESET[0] & UICR_PSELRESET_CONNECT_Msk) != (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos)) ||
            ((NRF_UICR->PSELRESET[1] & UICR_PSELRESET_CONNECT_Msk) != (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos))){
            NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_UICR->PSELRESET[0] = RESET_PIN;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_UICR->PSELRESET[1] = RESET_PIN;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NVIC_SystemReset();
        }
    #endif

    SystemCoreClockUpdate();
}

static bool errata_31(void)
{
    if (*(uint32_t *)0x10000130ul == 0xEul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
    }

    /* Apply by default for unknown devices until errata is confirmed fixed. */
    return true;
}

static bool errata_36(void)
{
    if (*(uint32_t *)0x10000130ul == 0xEul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
    }

    #ifdef DEVELOP_IN_NRF52840
        if (*(uint32_t *)0x10000130ul == 0x8ul){
            if (*(uint32_t *)0x10000134ul == 0x0ul){
                return true;
            }
            if (*(uint32_t *)0x10000134ul == 0x1ul){
                return true;
            }
            if (*(uint32_t *)0x10000134ul == 0x2ul){
                return true;
            }
            if (*(uint32_t *)0x10000134ul == 0x3ul){
                return true;
            }
        }
    #endif

    /* Apply by default for unknown devices until errata is confirmed fixed. */
    return true;
}

static bool errata_66(void)
{
    if (*(uint32_t *)0x10000130ul == 0xEul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
    }

    #ifdef DEVELOP_IN_NRF52840
        if (*(uint32_t *)0x10000130ul == 0x8ul){
            if (*(uint32_t *)0x10000134ul == 0x0ul){
                return true;
            }
            if (*(uint32_t *)0x10000134ul == 0x1ul){
                return true;
            }
            if (*(uint32_t *)0x10000134ul == 0x2ul){
                return true;
            }
            if (*(uint32_t *)0x10000134ul == 0x3ul){
                return true;
            }
        }
    #endif

    /* Apply by default for unknown devices until errata is confirmed fixed. */
    return true;
}

static bool errata_108(void)
{
    if (*(uint32_t *)0x10000130ul == 0xEul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
    }

    /* Apply by default for unknown devices until errata is confirmed fixed. */
    return true;
}

static bool errata_136(void)
{
    if (*(uint32_t *)0x10000130ul == 0xEul){
        if (*(uint32_t *)0x10000134ul == 0x0ul){
            return true;
        }
    }

    #ifdef DEVELOP_IN_NRF52840
        if (*(uint32_t *)0x10000130ul == 0x8ul){
            if (*(uint32_t *)0x10000134ul == 0x0ul){
                return true;
            }
            if (*(uint32_t *)0x10000134ul == 0x1ul){
                return true;
            }
            if (*(uint32_t *)0x10000134ul == 0x2ul){
                return true;
            }
            if (*(uint32_t *)0x10000134ul == 0x3ul){
                return true;
            }
        }
    #endif

    /* Apply by default for unknown devices until errata is confirmed fixed. */
    return true;
}


#ifdef DEVELOP_IN_NRF52840
    static bool errata_103(void)
    {
        if (*(uint32_t *)0x10000130ul == 0x8ul){
            if (*(uint32_t *)0x10000134ul == 0x0ul){
                return true;
            }
        }

        return false;
    }


    static bool errata_115(void)
    {
        if (*(uint32_t *)0x10000130ul == 0x8ul){
            if (*(uint32_t *)0x10000134ul == 0x0ul){
                return true;
            }
        }

        return false;
    }
#endif


/*lint --flb "Leave library region" */
