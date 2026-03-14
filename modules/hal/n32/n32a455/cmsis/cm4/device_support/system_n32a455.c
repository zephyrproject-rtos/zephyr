/*
 * N32A455 System Initialization
 * Minimal version - don't wait for clock
 */

#include <n32a455.h>

/* System clock frequency - using HSI 8MHz */
uint32_t SystemCoreClock = 8000000;

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};

/**
 * @brief Initialize the system - minimal version
 */
void SystemInit(void)
{
    /* FPU settings */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
#endif

    /* Just set SystemCoreClock - assume HSI is already running */
    SystemCoreClock = 8000000;
}

/**
 * @brief Update SystemCoreClock
 */
void SystemCoreClockUpdate(void)
{
    SystemCoreClock = 8000000;
}
