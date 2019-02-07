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
#include "system_nrf9160.h"

/*lint ++flb "Enter library region" */


#define __SYSTEM_CLOCK      (64000000UL)     /*!< nRF9160 Application core uses a fixed System Clock Frequency of 64MHz */

#define TRACE_PIN_CNF_VALUE (   (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) | \
                                (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | \
                                (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) | \
                                (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | \
                                (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos) | \
                                (GPIO_PIN_CNF_MCUSEL_TND << GPIO_PIN_CNF_MCUSEL_Pos))
#define TRACE_TRACECLK_PIN   (21)
#define TRACE_TRACEDATA0_PIN (22)
#define TRACE_TRACEDATA1_PIN (23)
#define TRACE_TRACEDATA2_PIN (24)
#define TRACE_TRACEDATA3_PIN (25)

#if defined ( __CC_ARM )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK;  
#elif defined ( __ICCARM__ )
    __root uint32_t SystemCoreClock = __SYSTEM_CLOCK;
#elif defined ( __GNUC__ )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK;
#endif

/* Errata are only handled in secure mode since they usually need access to FICR. */
#if !defined(NRF_TRUSTZONE_NONSECURE)
    static bool uicr_HFXOSRC_erased(void);
    static bool uicr_HFXOCNT_erased(void);
    static bool errata_6(void);
    static bool errata_14(void);
    static bool errata_15(void);
    static bool errata_20(void);
#endif

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = __SYSTEM_CLOCK;
}

void SystemInit(void)
{
    #if !defined(NRF_TRUSTZONE_NONSECURE)
        /* Perform Secure-mode initialization routines. */

        /* Set all ARM SAU regions to NonSecure if TrustZone extensions are enabled.
        * Nordic SPU should handle Secure Attribution tasks */
        #if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
          SAU->CTRL |= (1 << SAU_CTRL_ALLNS_Pos);
        #endif

        /* Trimming of the device. Copy all the trimming values from FICR into the target addresses. Trim 
         until one ADDR is not initialized. */
        uint32_t index = 0;
        for (index = 0; index < 256ul && NRF_FICR_S->TRIMCNF[index].ADDR != 0xFFFFFFFFul; index++){
          #if defined ( __ICCARM__ )
              #pragma diag_suppress=Pa082
          #endif
          *(volatile uint32_t *)NRF_FICR_S->TRIMCNF[index].ADDR = NRF_FICR_S->TRIMCNF[index].DATA;
          #if defined ( __ICCARM__ )
              #pragma diag_default=Pa082
          #endif
        }
          
        /* Set UICR->HFXOSRC and UICR->HFXOCNT to working defaults if UICR was erased */
        if (uicr_HFXOSRC_erased() || uicr_HFXOCNT_erased()) {
          /* Wait for pending NVMC operations to finish */
          while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);
          
          /* Enable write mode in NVMC */
          NRF_NVMC_S->CONFIG = NVMC_CONFIG_WEN_Wen;
          while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);
          
          if (uicr_HFXOSRC_erased()){
            /* Write default value to UICR->HFXOSRC */
            NRF_UICR_S->HFXOSRC = (NRF_UICR_S->HFXOSRC & ~UICR_HFXOSRC_HFXOSRC_Msk) | UICR_HFXOSRC_HFXOSRC_TCXO;
            while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);
          }
          
          if (uicr_HFXOCNT_erased()){
            /* Write default value to UICR->HFXOCNT */
            NRF_UICR_S->HFXOCNT = (NRF_UICR_S->HFXOCNT & ~UICR_HFXOCNT_HFXOCNT_Msk) | 0x20;
            while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);
          }
                
          /* Enable read mode in NVMC */
          NRF_NVMC_S->CONFIG = NVMC_CONFIG_WEN_Ren;
          while (NRF_NVMC_S->READY != NVMC_READY_READY_Ready);
          
          /* Reset to apply clock select update */
          NVIC_SystemReset();
        }
        
        /* Workaround for Errata 6 "POWER: SLEEPENTER and SLEEPEXIT events asserted after pin reset" found at the Errata document
            for your device located at https://www.nordicsemi.com/DocLib  */
        if (errata_6()){
            NRF_POWER_S->EVENTS_SLEEPENTER = (POWER_EVENTS_SLEEPENTER_EVENTS_SLEEPENTER_NotGenerated << POWER_EVENTS_SLEEPENTER_EVENTS_SLEEPENTER_Pos);
            NRF_POWER_S->EVENTS_SLEEPEXIT = (POWER_EVENTS_SLEEPEXIT_EVENTS_SLEEPEXIT_NotGenerated << POWER_EVENTS_SLEEPEXIT_EVENTS_SLEEPEXIT_Pos);
        }

        /* Workaround for Errata 14 "REGULATORS: LDO mode at startup" found at the Errata document
            for your device located at https://www.nordicsemi.com/DocLib  */
        if (errata_14()){
            *((volatile uint32_t *)0x50004A38) = 0x01ul;
            NRF_REGULATORS_S->DCDCEN = REGULATORS_DCDCEN_DCDCEN_Enabled << REGULATORS_DCDCEN_DCDCEN_Pos;
        }

        /* Workaround for Errata 15 "REGULATORS: LDO mode at startup" found at the Errata document
            for your device located at https://www.nordicsemi.com/DocLib  */
        if (errata_15()){
            *((volatile uint32_t *)0x50004A38) = 0x00ul;
            NRF_REGULATORS_S->DCDCEN = REGULATORS_DCDCEN_DCDCEN_Enabled << REGULATORS_DCDCEN_DCDCEN_Pos;
        }

        /* Workaround for Errata 20 "RAM content cannot be trusted upon waking up from System ON Idle or System OFF mode" found at the Errata document
            for your device located at https://www.nordicsemi.com/DocLib  */
        if (errata_20()){
            *((volatile uint32_t *)0x5003AEE4) = 0xC;
        }

        /* Enable SWO trace functionality. If ENABLE_SWO is not defined, SWO pin will be used as GPIO (see Product
           Specification to see which one). */
        #if defined (ENABLE_SWO)
            NRF_TAD_S->ENABLE = TAD_ENABLE_ENABLE_Msk;
            NRF_TAD_S->CLOCKSTART = TAD_CLOCKSTART_START_Msk;
            NRF_TAD_S->PSEL.TRACEDATA0 = TRACE_TRACEDATA0_PIN;
            NRF_TAD_S->TRACEPORTSPEED = TAD_TRACEPORTSPEED_TRACEPORTSPEED_32MHz;

            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA0_PIN] = TRACE_PIN_CNF_VALUE;
        #endif

        /* Enable Trace functionality. If ENABLE_TRACE is not defined, TRACE pins will be used as GPIOs (see Product
           Specification to see which ones). */
        #if defined (ENABLE_TRACE)
            NRF_TAD_S->ENABLE = TAD_ENABLE_ENABLE_Msk;
            NRF_TAD_S->CLOCKSTART = TAD_CLOCKSTART_START_Msk;
            NRF_TAD_S->PSEL.TRACECLK   = TRACE_TRACECLK_PIN;
            NRF_TAD_S->PSEL.TRACEDATA0 = TRACE_TRACEDATA0_PIN;
            NRF_TAD_S->PSEL.TRACEDATA1 = TRACE_TRACEDATA1_PIN;
            NRF_TAD_S->PSEL.TRACEDATA2 = TRACE_TRACEDATA2_PIN;
            NRF_TAD_S->PSEL.TRACEDATA3 = TRACE_TRACEDATA3_PIN;
            NRF_TAD_S->TRACEPORTSPEED = TAD_TRACEPORTSPEED_TRACEPORTSPEED_32MHz;

            NRF_P0_S->PIN_CNF[TRACE_TRACECLK_PIN] =   TRACE_PIN_CNF_VALUE;
            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA0_PIN] = TRACE_PIN_CNF_VALUE;
            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA1_PIN] = TRACE_PIN_CNF_VALUE;
            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA2_PIN] = TRACE_PIN_CNF_VALUE;
            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA3_PIN] = TRACE_PIN_CNF_VALUE;

        #endif

        /* Allow Non-Secure code to run FPU instructions. 
         * If only the secure code should control FPU power state these registers should be configured accordingly in the secure application code. */
        SCB->NSACR |= (3UL << 10);
    #endif
    
    /* Enable the FPU if the compiler used floating point unit instructions. __FPU_USED is a MACRO defined by the
    * compiler. Since the FPU consumes energy, remember to disable FPU use in the compiler if floating point unit
    * operations are not used in your code. */
    #if (__FPU_USED == 1)
      SCB->CPACR |= (3UL << 20) | (3UL << 22);
      __DSB();
      __ISB();
    #endif
    
    SystemCoreClockUpdate();
}


#if !defined(NRF_TRUSTZONE_NONSECURE)

    bool uicr_HFXOCNT_erased()
    {
        if (NRF_UICR_S->HFXOCNT == 0xFFFFFFFFul) {
            return true;
        }
        return false;
    }
    
    
    bool uicr_HFXOSRC_erased()
    {
        if ((NRF_UICR_S->HFXOSRC & UICR_HFXOSRC_HFXOSRC_Msk) != UICR_HFXOSRC_HFXOSRC_TCXO) {
            return true;
        }
        return false;
    }
    

    bool errata_6()
    {
        if (*(uint32_t *)0x00FF0130 == 0x9ul){
            if (*(uint32_t *)0x00FF0134 == 0x01ul){
                return true;
            }
            if (*(uint32_t *)0x00FF0134 == 0x02ul){
                return true;
            }
        }
        
        return false;
    }

    
    bool errata_14()
    {
        if (*(uint32_t *)0x00FF0130 == 0x9ul){
            if (*(uint32_t *)0x00FF0134 == 0x01ul){
                return true;
            }
        }

        return false;
    }


    bool errata_15()
    {
        if (*(uint32_t *)0x00FF0130 == 0x9ul){
            if (*(uint32_t *)0x00FF0134 == 0x02ul){
                return true;
            }
        }

        return false;
    }


    bool errata_20()
    {
        if (*(uint32_t *)0x00FF0130 == 0x9ul){
            if (*(uint32_t *)0x00FF0134 == 0x02ul){
                return true;
            }
        }

        return false;
    }
#endif

/*lint --flb "Leave library region" */
