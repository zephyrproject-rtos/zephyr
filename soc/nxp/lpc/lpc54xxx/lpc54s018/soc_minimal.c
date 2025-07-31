/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

#include <stdint.h>

/* SYSCON registers */
#define SYSCON_BASE         0x40000000
#define SYSCON_AHBCLKCTRL0  (*(volatile uint32_t *)(SYSCON_BASE + 0x200))
#define SYSCON_AHBCLKCTRL1  (*(volatile uint32_t *)(SYSCON_BASE + 0x204))
#define SYSCON_MAINCLKSEL   (*(volatile uint32_t *)(SYSCON_BASE + 0x280))
#define SYSCON_MAINCLKUEN   (*(volatile uint32_t *)(SYSCON_BASE + 0x284))

/* Clock enable bits */
#define CLK_EN_IOCON    (1 << 13)
#define CLK_EN_GPIO0    (1 << 14)
#define CLK_EN_GPIO1    (1 << 15)
#define CLK_EN_GPIO2    (1 << 16)
#define CLK_EN_GPIO3    (1 << 17)

/* Power control */
#define PMU_BASE        0x40020000
#define PMU_PDRUNCFG0   (*(volatile uint32_t *)(PMU_BASE + 0x100))
#define FRO_PD          (1 << 13)  /* FRO power down bit */

/* Main clock sources */
#define MAINCLK_FRO_12MHZ   0

/* System Core Clock (will be updated to 180MHz) */
uint32_t SystemCoreClock = 12000000U;

/* External clock initialization function */
extern void clock_init_180mhz(void);

/**
 * @brief Perform basic hardware initialization
 * 
 * This is called from the reset handler before main()
 */
void SystemInit(void)
{
    /* Ensure FRO is powered on */
    PMU_PDRUNCFG0 &= ~FRO_PD;
    
    /* First run at 12MHz FRO for safety */
    SYSCON_MAINCLKSEL = MAINCLK_FRO_12MHZ;
    SYSCON_MAINCLKUEN = 0;
    SYSCON_MAINCLKUEN = 1;
    
    /* Enable clocks for IOCON and GPIO */
    SYSCON_AHBCLKCTRL0 |= CLK_EN_IOCON | CLK_EN_GPIO0 | 
                          CLK_EN_GPIO1 | CLK_EN_GPIO2 | CLK_EN_GPIO3;
    
    /* Configure PLL and switch to 180MHz operation */
    clock_init_180mhz();
}

/**
 * @brief Update system core clock variable
 */
void SystemCoreClockUpdate(void)
{
    /* For now, always 12MHz FRO */
    SystemCoreClock = 12000000U;
}

/* External functions */
extern void Reset_Handler(void);