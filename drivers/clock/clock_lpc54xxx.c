/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

#include <stdint.h>

/* SYSCON registers for clock control */
#define SYSCON_BASE             0x40000000

/* Clock source selectors */
#define SYSCON_MAINCLKSEL       (*(volatile uint32_t *)(SYSCON_BASE + 0x280))
#define SYSCON_MAINCLKUEN       (*(volatile uint32_t *)(SYSCON_BASE + 0x284))
#define SYSCON_SYSPLLCLKSEL     (*(volatile uint32_t *)(SYSCON_BASE + 0x290))
#define SYSCON_SYSPLLCLKUEN     (*(volatile uint32_t *)(SYSCON_BASE + 0x294))

/* PLL control registers */
#define SYSCON_SYSPLLCTRL       (*(volatile uint32_t *)(SYSCON_BASE + 0x400))
#define SYSCON_SYSPLLSTAT       (*(volatile uint32_t *)(SYSCON_BASE + 0x404))
#define SYSCON_SYSPLLNDEC       (*(volatile uint32_t *)(SYSCON_BASE + 0x408))
#define SYSCON_SYSPLLPDEC       (*(volatile uint32_t *)(SYSCON_BASE + 0x40C))
#define SYSCON_SYSPLLMDEC       (*(volatile uint32_t *)(SYSCON_BASE + 0x410))

/* Clock dividers */
#define SYSCON_AHBCLKDIV        (*(volatile uint32_t *)(SYSCON_BASE + 0x380))

/* Power control */
#define PMU_BASE                0x40020000
#define PMU_PDRUNCFG0           (*(volatile uint32_t *)(PMU_BASE + 0x100))

/* Power down bits */
#define FRO_PD                  (1 << 13)
#define SYSPLL_PD               (1 << 26)

/* Clock source definitions */
#define MAINCLK_FRO_12MHZ       0
#define MAINCLK_CLKIN           1
#define MAINCLK_FRO_HF          3
#define MAINCLK_PLL             6

/* PLL source definitions */
#define SYSPLL_SRC_FRO_12MHZ    0
#define SYSPLL_SRC_CLKIN        1
#define SYSPLL_SRC_FRO_1MHZ     2

/* PLL configuration for 180MHz from 12MHz FRO
 * Based on SDK example:
 * - MDEC = 8191
 * - NDEC = 770 
 * - PDEC = 98
 * - SELP = 16
 * - SELI = 32
 * - SELR = 0
 */
#define PLL_MDEC_VAL    8191
#define PLL_NDEC_VAL    770
#define PLL_PDEC_VAL    98
#define PLL_SELP_VAL    16
#define PLL_SELI_VAL    32
#define PLL_SELR_VAL    0

/* System Core Clock variable */
extern uint32_t SystemCoreClock;

/**
 * @brief Configure System PLL for 180MHz operation
 */
static void configure_pll_180mhz(void)
{
    /* Power on System PLL */
    PMU_PDRUNCFG0 &= ~SYSPLL_PD;
    
    /* Select FRO 12MHz as PLL source */
    SYSCON_SYSPLLCLKSEL = SYSPLL_SRC_FRO_12MHZ;
    SYSCON_SYSPLLCLKUEN = 0;
    SYSCON_SYSPLLCLKUEN = 1;
    
    /* Configure PLL for 180MHz output */
    SYSCON_SYSPLLCTRL = (PLL_SELI_VAL << 0) |  /* SELI bits 0-5 */
                        (PLL_SELP_VAL << 8) |  /* SELP bits 8-12 */
                        (PLL_SELR_VAL << 14);  /* SELR bits 14-17 */
    
    /* Set MDEC, NDEC, PDEC values */
    SYSCON_SYSPLLMDEC = PLL_MDEC_VAL;
    SYSCON_SYSPLLNDEC = PLL_NDEC_VAL;
    SYSCON_SYSPLLPDEC = PLL_PDEC_VAL;
    
    /* Wait for PLL to lock */
    while (!(SYSCON_SYSPLLSTAT & 1)) {
        /* Wait for lock bit */
    }
}

/**
 * @brief Initialize clocks for 180MHz operation
 * 
 * Called from SystemInit to set up proper clock configuration
 */
void clock_init_180mhz(void)
{
    /* Ensure FRO is on */
    PMU_PDRUNCFG0 &= ~FRO_PD;
    
    /* Set AHB clock divider to 1 (no division) */
    SYSCON_AHBCLKDIV = 0;
    
    /* Configure and enable PLL for 180MHz */
    configure_pll_180mhz();
    
    /* Switch main clock to PLL */
    SYSCON_MAINCLKSEL = MAINCLK_PLL;
    SYSCON_MAINCLKUEN = 0;
    SYSCON_MAINCLKUEN = 1;
    
    /* Update system core clock variable */
    SystemCoreClock = 180000000U;
}