/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_HC32F4A0_INT_EVT_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_HC32F4A0_INT_EVT_H_


/**
 * @brief Interrupt Vector Number
 */
#define NonMaskableInt_IRQn             -14  /* Non Maskable                             */
#define HardFault_IRQn                  -13  /* Hard Fault                               */
#define MemoryManagement_IRQn           -12  /* MemManage Fault                          */
#define BusFault_IRQn                   -11  /* Bus Fault                                */
#define UsageFault_IRQn                 -10  /* Usage Fault                              */
#define SVCall_IRQn                     -5   /* SVCall                                   */
#define DebugMonitor_IRQn               -4   /* DebugMonitor                             */
#define PendSV_IRQn                     -2   /* Pend SV                                  */
#define SysTick_IRQn                    -1   /* System Tick                              */
#define INT000_IRQn                     0
#define INT001_IRQn                     1
#define INT002_IRQn                     2
#define INT003_IRQn                     3
#define INT004_IRQn                     4
#define INT005_IRQn                     5
#define INT006_IRQn                     6
#define INT007_IRQn                     7
#define INT008_IRQn                     8
#define INT009_IRQn                     9
#define INT010_IRQn                     10
#define INT011_IRQn                     11
#define INT012_IRQn                     12
#define INT013_IRQn                     13
#define INT014_IRQn                     14
#define INT015_IRQn                     15
#define INT016_IRQn                     16
#define INT017_IRQn                     17
#define INT018_IRQn                     18
#define INT019_IRQn                     19
#define INT020_IRQn                     20
#define INT021_IRQn                     21
#define INT022_IRQn                     22
#define INT023_IRQn                     23
#define INT024_IRQn                     24
#define INT025_IRQn                     25
#define INT026_IRQn                     26
#define INT027_IRQn                     27
#define INT028_IRQn                     28
#define INT029_IRQn                     29
#define INT030_IRQn                     30
#define INT031_IRQn                     31
#define INT032_IRQn                     32
#define INT033_IRQn                     33
#define INT034_IRQn                     34
#define INT035_IRQn                     35
#define INT036_IRQn                     36
#define INT037_IRQn                     37
#define INT038_IRQn                     38
#define INT039_IRQn                     39
#define INT040_IRQn                     40
#define INT041_IRQn                     41
#define INT042_IRQn                     42
#define INT043_IRQn                     43
#define INT044_IRQn                     44
#define INT045_IRQn                     45
#define INT046_IRQn                     46
#define INT047_IRQn                     47
#define INT048_IRQn                     48
#define INT049_IRQn                     49
#define INT050_IRQn                     50
#define INT051_IRQn                     51
#define INT052_IRQn                     52
#define INT053_IRQn                     53
#define INT054_IRQn                     54
#define INT055_IRQn                     55
#define INT056_IRQn                     56
#define INT057_IRQn                     57
#define INT058_IRQn                     58
#define INT059_IRQn                     59
#define INT060_IRQn                     60
#define INT061_IRQn                     61
#define INT062_IRQn                     62
#define INT063_IRQn                     63
#define INT064_IRQn                     64
#define INT065_IRQn                     65
#define INT066_IRQn                     66
#define INT067_IRQn                     67
#define INT068_IRQn                     68
#define INT069_IRQn                     69
#define INT070_IRQn                     70
#define INT071_IRQn                     71
#define INT072_IRQn                     72
#define INT073_IRQn                     73
#define INT074_IRQn                     74
#define INT075_IRQn                     75
#define INT076_IRQn                     76
#define INT077_IRQn                     77
#define INT078_IRQn                     78
#define INT079_IRQn                     79
#define INT080_IRQn                     80
#define INT081_IRQn                     81
#define INT082_IRQn                     82
#define INT083_IRQn                     83
#define INT084_IRQn                     84
#define INT085_IRQn                     85
#define INT086_IRQn                     86
#define INT087_IRQn                     87
#define INT088_IRQn                     88
#define INT089_IRQn                     89
#define INT090_IRQn                     90
#define INT091_IRQn                     91
#define INT092_IRQn                     92
#define INT093_IRQn                     93
#define INT094_IRQn                     94
#define INT095_IRQn                     95
#define INT096_IRQn                     96
#define INT097_IRQn                     97
#define INT098_IRQn                     98
#define INT099_IRQn                     99
#define INT100_IRQn                     100
#define INT101_IRQn                     101
#define INT102_IRQn                     102
#define INT103_IRQn                     103
#define INT104_IRQn                     104
#define INT105_IRQn                     105
#define INT106_IRQn                     106
#define INT107_IRQn                     107
#define INT108_IRQn                     108
#define INT109_IRQn                     109
#define INT110_IRQn                     110
#define INT111_IRQn                     111
#define INT112_IRQn                     112
#define INT113_IRQn                     113
#define INT114_IRQn                     114
#define INT115_IRQn                     115
#define INT116_IRQn                     116
#define INT117_IRQn                     117
#define INT118_IRQn                     118
#define INT119_IRQn                     119
#define INT120_IRQn                     120
#define INT121_IRQn                     121
#define INT122_IRQn                     122
#define INT123_IRQn                     123
#define INT124_IRQn                     124
#define INT125_IRQn                     125
#define INT126_IRQn                     126
#define INT127_IRQn                     127
#define INT128_IRQn                     128
#define INT129_IRQn                     129
#define INT130_IRQn                     130
#define INT131_IRQn                     131
#define INT132_IRQn                     132
#define INT133_IRQn                     133
#define INT134_IRQn                     134
#define INT135_IRQn                     135
#define INT136_IRQn                     136
#define INT137_IRQn                     137
#define INT138_IRQn                     138
#define INT139_IRQn                     139
#define INT140_IRQn                     140
#define INT141_IRQn                     141
#define INT142_IRQn                     142
#define INT143_IRQn                     143


/**
 * @brief Interrupt Priority Level
 */
#define DDL_IRQ_PRIO_00                 (0U)
#define DDL_IRQ_PRIO_01                 (1U)
#define DDL_IRQ_PRIO_02                 (2U)
#define DDL_IRQ_PRIO_03                 (3U)
#define DDL_IRQ_PRIO_04                 (4U)
#define DDL_IRQ_PRIO_05                 (5U)
#define DDL_IRQ_PRIO_06                 (6U)
#define DDL_IRQ_PRIO_07                 (7U)
#define DDL_IRQ_PRIO_08                 (8U)
#define DDL_IRQ_PRIO_09                 (9U)
#define DDL_IRQ_PRIO_10                 (10U)
#define DDL_IRQ_PRIO_11                 (11U)
#define DDL_IRQ_PRIO_12                 (12U)
#define DDL_IRQ_PRIO_13                 (13U)
#define DDL_IRQ_PRIO_14                 (14U)
// #define DDL_IRQ_PRIO_15                 (15U)   /* kernal used */

#define DDL_IRQ_PRIO_DEFAULT            (DDL_IRQ_PRIO_14)


/**
 * @brief Interrupt Source Number
 */
#define INT_SRC_SWI_IRQ0             0U       /* SWI_IRQ0  */
#define INT_SRC_SWI_IRQ1             1U       /* SWI_IRQ1  */
#define INT_SRC_SWI_IRQ2             2U       /* SWI_IRQ2  */
#define INT_SRC_SWI_IRQ3             3U       /* SWI_IRQ3  */
#define INT_SRC_SWI_IRQ4             4U       /* SWI_IRQ4  */
#define INT_SRC_SWI_IRQ5             5U       /* SWI_IRQ5  */
#define INT_SRC_SWI_IRQ6             6U       /* SWI_IRQ6  */
#define INT_SRC_SWI_IRQ7             7U       /* SWI_IRQ7  */
#define INT_SRC_SWI_IRQ8             8U       /* SWI_IRQ8  */
#define INT_SRC_SWI_IRQ9             9U       /* SWI_IRQ9  */
#define INT_SRC_SWI_IRQ10            10U      /* SWI_IRQ10 */
#define INT_SRC_SWI_IRQ11            11U      /* SWI_IRQ11 */
#define INT_SRC_SWI_IRQ12            12U      /* SWI_IRQ12 */
#define INT_SRC_SWI_IRQ13            13U      /* SWI_IRQ13 */
#define INT_SRC_SWI_IRQ14            14U      /* SWI_IRQ14 */
#define INT_SRC_SWI_IRQ15            15U      /* SWI_IRQ15 */
#define INT_SRC_SWI_IRQ16            16U      /* SWI_IRQ16 */
#define INT_SRC_SWI_IRQ17            17U      /* SWI_IRQ17 */
#define INT_SRC_SWI_IRQ18            18U      /* SWI_IRQ18 */
#define INT_SRC_SWI_IRQ19            19U      /* SWI_IRQ19 */
#define INT_SRC_SWI_IRQ20            20U      /* SWI_IRQ20 */
#define INT_SRC_SWI_IRQ21            21U      /* SWI_IRQ21 */
#define INT_SRC_SWI_IRQ22            22U      /* SWI_IRQ22 */
#define INT_SRC_SWI_IRQ23            23U      /* SWI_IRQ23 */
#define INT_SRC_SWI_IRQ24            24U      /* SWI_IRQ24 */
#define INT_SRC_SWI_IRQ25            25U      /* SWI_IRQ25 */
#define INT_SRC_SWI_IRQ26            26U      /* SWI_IRQ26 */
#define INT_SRC_SWI_IRQ27            27U      /* SWI_IRQ27 */
#define INT_SRC_SWI_IRQ28            28U      /* SWI_IRQ28 */
#define INT_SRC_SWI_IRQ29            29U      /* SWI_IRQ29 */
#define INT_SRC_SWI_IRQ30            30U      /* SWI_IRQ30 */
#define INT_SRC_SWI_IRQ31            31U      /* SWI_IRQ31 */

/* External Interrupt. */
#define INT_SRC_PORT_EIRQ0           0U       /* PORT_EIRQ0  */
#define INT_SRC_PORT_EIRQ1           1U       /* PORT_EIRQ1  */
#define INT_SRC_PORT_EIRQ2           2U       /* PORT_EIRQ2  */
#define INT_SRC_PORT_EIRQ3           3U       /* PORT_EIRQ3  */
#define INT_SRC_PORT_EIRQ4           4U       /* PORT_EIRQ4  */
#define INT_SRC_PORT_EIRQ5           5U       /* PORT_EIRQ5  */
#define INT_SRC_PORT_EIRQ6           6U       /* PORT_EIRQ6  */
#define INT_SRC_PORT_EIRQ7           7U       /* PORT_EIRQ7  */
#define INT_SRC_PORT_EIRQ8           8U       /* PORT_EIRQ8  */
#define INT_SRC_PORT_EIRQ9           9U       /* PORT_EIRQ9  */
#define INT_SRC_PORT_EIRQ10          10U      /* PORT_EIRQ10 */
#define INT_SRC_PORT_EIRQ11          11U      /* PORT_EIRQ11 */
#define INT_SRC_PORT_EIRQ12          12U      /* PORT_EIRQ12 */
#define INT_SRC_PORT_EIRQ13          13U      /* PORT_EIRQ13 */
#define INT_SRC_PORT_EIRQ14          14U      /* PORT_EIRQ14 */
#define INT_SRC_PORT_EIRQ15          15U      /* PORT_EIRQ15 */

/*  DMA_1  */
#define    INT_SRC_DMA1_TC0          32U     /* DMA_1_TC0 */
#define    INT_SRC_DMA1_TC1          33U     /* DMA_1_TC1 */
#define    INT_SRC_DMA1_TC2          34U     /* DMA_1_TC2 */
#define    INT_SRC_DMA1_TC3          35U     /* DMA_1_TC3 */
#define    INT_SRC_DMA1_TC4          36U     /* DMA_1_TC4 */
#define    INT_SRC_DMA1_TC5          37U     /* DMA_1_TC5 */
#define    INT_SRC_DMA1_TC6          38U     /* DMA_1_TC6 */
#define    INT_SRC_DMA1_TC7          39U     /* DMA_1_TC7 */
#define    INT_SRC_DMA1_BTC0         40U     /* DMA_1_BTC0 */
#define    INT_SRC_DMA1_BTC1         41U     /* DMA_1_BTC1 */
#define    INT_SRC_DMA1_BTC2         42U     /* DMA_1_BTC2 */
#define    INT_SRC_DMA1_BTC3         43U     /* DMA_1_BTC3 */
#define    INT_SRC_DMA1_BTC4         44U     /* DMA_1_BTC4 */
#define    INT_SRC_DMA1_BTC5         45U     /* DMA_1_BTC5 */
#define    INT_SRC_DMA1_BTC6         46U     /* DMA_1_BTC6 */
#define    INT_SRC_DMA1_BTC7         47U     /* DMA_1_BTC7 */
#define    INT_SRC_DMA1_ERR          48U     /* DMA_1_ERR */
/*  EFM  */
#define    INT_SRC_EFM_PEERR         49U     /* EFM_PEERR */
#define    INT_SRC_EFM_RDCOL         50U     /* EFM_RDCOL */
#define    INT_SRC_EFM_OPTEND        51U     /* EFM_OPTEND */
/*  QSPI  */
#define    INT_SRC_QSPI_INTR         54U     /* QSPI_INTR */
/*  DCU  */
#define    INT_SRC_DCU1              55U     /* DCU1 */
#define    INT_SRC_DCU2              56U     /* DCU2 */
#define    INT_SRC_DCU3              57U     /* DCU3 */
#define    INT_SRC_DCU4              58U     /* DCU4 */
#define    INT_SRC_DCU5              59U     /* DCU5 */
#define    INT_SRC_DCU6              60U     /* DCU6 */
#define    INT_SRC_DCU7              61U     /* DCU7 */
#define    INT_SRC_DCU8              62U     /* DCU8 */
/*  DMA2  */
#define    INT_SRC_DMA2_TC0          64U     /* DMA_2_TC0 */
#define    INT_SRC_DMA2_TC1          65U     /* DMA_2_TC1 */
#define    INT_SRC_DMA2_TC2          66U     /* DMA_2_TC2 */
#define    INT_SRC_DMA2_TC3          67U     /* DMA_2_TC3 */
#define    INT_SRC_DMA2_TC4          68U     /* DMA_2_TC4 */
#define    INT_SRC_DMA2_TC5          69U     /* DMA_2_TC5 */
#define    INT_SRC_DMA2_TC6          70U     /* DMA_2_TC6 */
#define    INT_SRC_DMA2_TC7          71U     /* DMA_2_TC7 */
#define    INT_SRC_DMA2_BTC0         72U     /* DMA_2_BTC0 */
#define    INT_SRC_DMA2_BTC1         73U     /* DMA_2_BTC1 */
#define    INT_SRC_DMA2_BTC2         74U     /* DMA_2_BTC2 */
#define    INT_SRC_DMA2_BTC3         75U     /* DMA_2_BTC3 */
#define    INT_SRC_DMA2_BTC4         76U     /* DMA_2_BTC4 */
#define    INT_SRC_DMA2_BTC5         77U     /* DMA_2_BTC5 */
#define    INT_SRC_DMA2_BTC6         78U     /* DMA_2_BTC6 */
#define    INT_SRC_DMA2_BTC7         79U     /* DMA_2_BTC7 */
#define    INT_SRC_DMA2_ERR          80U     /* DMA_2_ERR */
/*  MAU  */
#define    INT_SRC_MAU_SQRT          83U     /* MAU_SQRT */
/*  DVP  */
#define    INT_SRC_DVP_FRAMSTA       84U     /* DVP_FRAMSTA */
#define    INT_SRC_DVP_LINESTA       85U     /* DVP_LINESTA */
#define    INT_SRC_DVP_LINEEND       86U     /* DVP_LINEEND */
#define    INT_SRC_DVP_FRAMEND       87U     /* DVP_FRAMEND */
#define    INT_SRC_DVP_SQUERR        88U     /* DVP_SQUERR */
#define    INT_SRC_DVP_FIFOERR       89U     /* DVP_FIFOERR */
/*  FMAC  */
#define    INT_SRC_FMAC_1            91U     /* FMAC_1_FIR */
#define    INT_SRC_FMAC_2            92U     /* FMAC_2_FIR */
#define    INT_SRC_FMAC_3            93U     /* FMAC_3_FIR */
#define    INT_SRC_FMAC_4            94U     /* FMAC_4_FIR */
/*  TIMER0  */
#define    INT_SRC_TMR0_1_CMP_A      96U     /* TMR0_1_CMPA */
#define    INT_SRC_TMR0_1_CMP_B      97U     /* TMR0_1_CMPB */
#define    INT_SRC_TMR0_2_CMP_A      98U     /* TMR0_2_CMPA */
#define    INT_SRC_TMR0_2_CMP_B      99U     /* TMR0_2_CMPB */
/*  TIMER2  */
#define    INT_SRC_TMR2_1_CMP_A      100U    /* TMR2_1_CMPA  */
#define    INT_SRC_TMR2_1_CMP_B      101U    /* TMR2_1_CMPB  */
#define    INT_SRC_TMR2_1_OVF_A      102U    /* TMR2_1_OVFA  */
#define    INT_SRC_TMR2_1_OVF_B      103U    /* TMR2_1_OVFB  */
#define    INT_SRC_TMR2_2_CMP_A      104U    /* TMR2_2_CMPA  */
#define    INT_SRC_TMR2_2_CMP_B      105U    /* TMR2_2_CMPB  */
#define    INT_SRC_TMR2_2_OVF_A      106U    /* TMR2_2_OVFA  */
#define    INT_SRC_TMR2_2_OVF_B      107U    /* TMR2_2_OVFB  */
#define    INT_SRC_TMR2_3_CMP_A      108U    /* TMR2_3_CMPA  */
#define    INT_SRC_TMR2_3_CMP_B      109U    /* TMR2_3_CMPB  */
#define    INT_SRC_TMR2_3_OVF_A      110U    /* TMR2_3_OVFA  */
#define    INT_SRC_TMR2_3_OVF_B      111U    /* TMR2_3_OVFB  */
#define    INT_SRC_TMR2_4_CMP_A      112U    /* TMR2_4_CMPA  */
#define    INT_SRC_TMR2_4_CMP_B      113U    /* TMR2_4_CMPB  */
#define    INT_SRC_TMR2_4_OVF_A      114U    /* TMR2_4_OVFA  */
#define    INT_SRC_TMR2_4_OVF_B      115U    /* TMR2_4_OVFB  */
/*  RTC  */
#define    INT_SRC_RTC_TP            120U     /* RTC_TP */
#define    INT_SRC_RTC_ALM           121U     /* RTC_ALM */
#define    INT_SRC_RTC_PRD           122U     /* RTC_PRD */
/*  XTAL  */
#define    INT_SRC_XTAL_STOP         125U     /* XTAL_STOP */
/*  WKTM  */
#define    INT_SRC_WKTM_PRD          126U     /* WKTM_PRD */
/*  SWDT  */
#define    INT_SRC_SWDT_REFUDF       127U     /* SWDT_REFUDF */
/*  TIMER6_1  */
#define    INT_SRC_TMR6_1_GCMP_A     128U     /* TMR6_1_GCMA */
#define    INT_SRC_TMR6_1_GCMP_B     129U     /* TMR6_1_GCMB */
#define    INT_SRC_TMR6_1_GCMP_C     130U     /* TMR6_1_GCMC */
#define    INT_SRC_TMR6_1_GCMP_D     131U     /* TMR6_1_GCMD */
#define    INT_SRC_TMR6_1_GCMP_E     132U     /* TMR6_1_GCME */
#define    INT_SRC_TMR6_1_GCMP_F     133U     /* TMR6_1_GCMF */
#define    INT_SRC_TMR6_1_OVF        134U     /* TMR6_1_GOVF */
#define    INT_SRC_TMR6_1_UDF        135U     /* TMR6_1_GUDF */
/*  TIMER4_1  */
#define    INT_SRC_TMR4_1_GCMP_UH    136U     /* TMR4_1_GCMUH */
#define    INT_SRC_TMR4_1_GCMP_UL    137U     /* TMR4_1_GCMUL */
#define    INT_SRC_TMR4_1_GCMP_VH    138U     /* TMR4_1_GCMVH */
#define    INT_SRC_TMR4_1_GCMP_VL    139U     /* TMR4_1_GCMVL */
#define    INT_SRC_TMR4_1_GCMP_WH    140U     /* TMR4_1_GCMWH */
#define    INT_SRC_TMR4_1_GCMP_WL    141U     /* TMR4_1_GCMWL */
#define    INT_SRC_TMR4_1_OVF        142U     /* TMR4_1_GOVF */
#define    INT_SRC_TMR4_1_UDF        143U     /* TMR4_1_GUDF */
/*  TIMER6_2  */
#define    INT_SRC_TMR6_2_GCMP_A     144U     /* TMR6_2_GCMA */
#define    INT_SRC_TMR6_2_GCMP_B     145U     /* TMR6_2_GCMB */
#define    INT_SRC_TMR6_2_GCMP_C     146U     /* TMR6_2_GCMC */
#define    INT_SRC_TMR6_2_GCMP_D     147U     /* TMR6_2_GCMD */
#define    INT_SRC_TMR6_2_GCMP_E     148U     /* TMR6_2_GCME */
#define    INT_SRC_TMR6_2_GCMP_F     149U     /* TMR6_2_GCMF */
#define    INT_SRC_TMR6_2_OVF        150U     /* TMR6_2_GOVF */
#define    INT_SRC_TMR6_2_UDF        151U     /* TMR6_2_GUDF */
/*  TIMER4_2  */
#define    INT_SRC_TMR4_2_GCMP_UH    152U     /* TMR4_2_GCMUH */
#define    INT_SRC_TMR4_2_GCMP_UL    153U     /* TMR4_2_GCMUL */
#define    INT_SRC_TMR4_2_GCMP_VH    154U     /* TMR4_2_GCMVH */
#define    INT_SRC_TMR4_2_GCMP_VL    155U     /* TMR4_2_GCMVL */
#define    INT_SRC_TMR4_2_GCMP_WH    156U     /* TMR4_2_GCMWH */
#define    INT_SRC_TMR4_2_GCMP_WL    157U     /* TMR4_2_GCMWL */
#define    INT_SRC_TMR4_2_OVF        158U     /* TMR4_2_GOVF */
#define    INT_SRC_TMR4_2_UDF        159U     /* TMR4_2_GUDF */
/*  TIMER6_3  */
#define    INT_SRC_TMR6_3_GCMP_A     160U     /* TMR6_3_GCMA */
#define    INT_SRC_TMR6_3_GCMP_B     161U     /* TMR6_3_GCMB */
#define    INT_SRC_TMR6_3_GCMP_C     162U     /* TMR6_3_GCMC */
#define    INT_SRC_TMR6_3_GCMP_D     163U     /* TMR6_3_GCMD */
#define    INT_SRC_TMR6_3_GCMP_E     164U     /* TMR6_3_GCME */
#define    INT_SRC_TMR6_3_GCMP_F     165U     /* TMR6_3_GCMF */
#define    INT_SRC_TMR6_3_OVF        166U     /* TMR6_3_GOVF */
#define    INT_SRC_TMR6_3_UDF        167U     /* TMR6_3_GUDF */
/*  TIMER4_3  */
#define    INT_SRC_TMR4_3_GCMP_UH    168U     /* TMR4_3_GCMUH */
#define    INT_SRC_TMR4_3_GCMP_UL    169U     /* TMR4_3_GCMUL */
#define    INT_SRC_TMR4_3_GCMP_VH    170U     /* TMR4_3_GCMVH */
#define    INT_SRC_TMR4_3_GCMP_VL    171U     /* TMR4_3_GCMVL */
#define    INT_SRC_TMR4_3_GCMP_WH    172U     /* TMR4_3_GCMWH */
#define    INT_SRC_TMR4_3_GCMP_WL    173U     /* TMR4_3_GCMWL */
#define    INT_SRC_TMR4_3_OVF        174U     /* TMR4_3_GOVF */
#define    INT_SRC_TMR4_3_UDF        175U     /* TMR4_3_GUDF */
/*  TIMER6_1  */
#define    INT_SRC_TMR6_1_DTE        176U     /* TMR6_1_GDTE */
#define    INT_SRC_TMR6_1_SCMP_A     179U     /* TMR6_1_SCMA */
#define    INT_SRC_TMR6_1_SCMP_B     180U     /* TMR6_1_SCMB */
/*  TIMER4_1  */
#define    INT_SRC_TMR4_1_RELOAD_U   181U     /* TMR4_1_RLOU */
#define    INT_SRC_TMR4_1_RELOAD_V   182U     /* TMR4_1_RLOV */
#define    INT_SRC_TMR4_1_RELOAD_W   183U     /* TMR4_1_RLOW */
/*  TIMER6_2  */
#define    INT_SRC_TMR6_2_DTE        184U     /* TMR6_2_GDTE */
#define    INT_SRC_TMR6_2_SCMP_A     187U     /* TMR6_2_SCMA */
#define    INT_SRC_TMR6_2_SCMP_B     188U     /* TMR6_2_SCMB */
/*  TIMER4_2  */
#define    INT_SRC_TMR4_2_RELOAD_U   189U     /* TMR4_2_RLOU */
#define    INT_SRC_TMR4_2_RELOAD_V   190U     /* TMR4_2_RLOV */
#define    INT_SRC_TMR4_2_RELOAD_W   191U     /* TMR4_2_RLOW */
/*  TIMER6_3  */
#define    INT_SRC_TMR6_3_DTE        192U     /* TMR6_3_GDTE */
#define    INT_SRC_TMR6_3_SCMP_A     195U     /* TMR6_3_SCMA */
#define    INT_SRC_TMR6_3_SCMP_B     196U     /* TMR6_3_SCMB */
/*  TIMER4_3  */
#define    INT_SRC_TMR4_3_RELOAD_U   197U     /* TMR4_3_RLOU */
#define    INT_SRC_TMR4_3_RELOAD_V   198U     /* TMR4_3_RLOV */
#define    INT_SRC_TMR4_3_RELOAD_W   199U     /* TMR4_3_RLOW */
/*  TIMER6_4 TIMER6_5  */
#define    INT_SRC_TMR6_4_GCMP_A     208U     /* TMR6_4_GCMA */
#define    INT_SRC_TMR6_4_GCMP_B     209U     /* TMR6_4_GCMB */
#define    INT_SRC_TMR6_4_GCMP_C     210U     /* TMR6_4_GCMC */
#define    INT_SRC_TMR6_4_GCMP_D     211U     /* TMR6_4_GCMD */
#define    INT_SRC_TMR6_4_GCMP_E     212U     /* TMR6_4_GCME */
#define    INT_SRC_TMR6_4_GCMP_F     213U     /* TMR6_4_GCMF */
#define    INT_SRC_TMR6_4_OVF        214U     /* TMR6_4_GOVF */
#define    INT_SRC_TMR6_4_UDF        215U     /* TMR6_4_GUDF */
#define    INT_SRC_TMR6_4_DTE        216U     /* TMR6_4_GDTE */
#define    INT_SRC_TMR6_4_SCMP_A     219U     /* TMR6_4_SCMA */
#define    INT_SRC_TMR6_4_SCMP_B     220U     /* TMR6_4_SCMB */
#define    INT_SRC_TMR6_5_GCMP_A     224U     /* TMR6_5_GCMA */
#define    INT_SRC_TMR6_5_GCMP_B     225U     /* TMR6_5_GCMB */
#define    INT_SRC_TMR6_5_GCMP_C     226U     /* TMR6_5_GCMC */
#define    INT_SRC_TMR6_5_GCMP_D     227U     /* TMR6_5_GCMD */
#define    INT_SRC_TMR6_5_GCMP_E     228U     /* TMR6_5_GCME */
#define    INT_SRC_TMR6_5_GCMP_F     229U     /* TMR6_5_GCMF */
#define    INT_SRC_TMR6_5_OVF        230U     /* TMR6_5_GOVF */
#define    INT_SRC_TMR6_5_UDF        231U     /* TMR6_5_GUDF */
#define    INT_SRC_TMR6_5_DTE        232U     /* TMR6_5_GDTE */
#define    INT_SRC_TMR6_5_SCMP_A     235U     /* TMR6_5_SCMA */
#define    INT_SRC_TMR6_5_SCMP_B     236U     /* TMR6_5_SCMB */
/*  TIMERA_1  */
#define    INT_SRC_TMRA_1_OVF        237U     /* TMRA_1_OVF */
#define    INT_SRC_TMRA_1_UDF        238U     /* TMRA_1_UDF */
#define    INT_SRC_TMRA_1_CMP        239U     /* TMRA_1_CMP */
/*  TIMER6_6  */
#define    INT_SRC_TMR6_6_GCMP_A     240U     /* TMR6_6_GCMA */
#define    INT_SRC_TMR6_6_GCMP_B     241U     /* TMR6_6_GCMB */
#define    INT_SRC_TMR6_6_GCMP_C     242U     /* TMR6_6_GCMC */
#define    INT_SRC_TMR6_6_GCMP_D     243U     /* TMR6_6_GCMD */
#define    INT_SRC_TMR6_6_GCMP_E     244U     /* TMR6_6_GCME */
#define    INT_SRC_TMR6_6_GCMP_F     245U     /* TMR6_6_GCMF */
#define    INT_SRC_TMR6_6_OVF        246U     /* TMR6_6_GOVF */
#define    INT_SRC_TMR6_6_UDF        247U     /* TMR6_6_GUDF */
#define    INT_SRC_TMR6_6_DTE        248U     /* TMR6_6_GDTE */
#define    INT_SRC_TMR6_6_SCMP_A     251U     /* TMR6_6_SCMA */
#define    INT_SRC_TMR6_6_SCMP_B     252U     /* TMR6_6_SCMB */
/*  TIMERA_2  */
#define    INT_SRC_TMRA_2_OVF        253U     /* TMRA_2_OVF */
#define    INT_SRC_TMRA_2_UDF        254U     /* TMRA_2_UDF */
#define    INT_SRC_TMRA_2_CMP        255U     /* TMRA_2_CMP */
/*  TIMER6_7  */
#define    INT_SRC_TMR6_7_GCMP_A     256U     /* TMR6_7_GCMA */
#define    INT_SRC_TMR6_7_GCMP_B     257U     /* TMR6_7_GCMB */
#define    INT_SRC_TMR6_7_GCMP_C     258U     /* TMR6_7_GCMC */
#define    INT_SRC_TMR6_7_GCMP_D     259U     /* TMR6_7_GCMD */
#define    INT_SRC_TMR6_7_GCMP_E     260U     /* TMR6_7_GCME */
#define    INT_SRC_TMR6_7_GCMP_F     261U     /* TMR6_7_GCMF */
#define    INT_SRC_TMR6_7_OVF        262U     /* TMR6_7_GOVF */
#define    INT_SRC_TMR6_7_UDF        263U     /* TMR6_7_GUDF */
#define    INT_SRC_TMR6_7_DTE        264U     /* TMR6_7_GDTE */
#define    INT_SRC_TMR6_7_SCMP_A     267U     /* TMR6_7_SCMA */
#define    INT_SRC_TMR6_7_SCMP_B     268U     /* TMR6_7_SCMB */
/*  TIMERA_3  */
#define    INT_SRC_TMRA_3_OVF        269U     /* TMRA_3_OVF */
#define    INT_SRC_TMRA_3_UDF        270U     /* TMRA_3_UDF */
#define    INT_SRC_TMRA_3_CMP        271U     /* TMRA_3_CMP */
/*  TIMER6_8  */
#define    INT_SRC_TMR6_8_GCMP_A     272U     /* TMR6_8_GCMA */
#define    INT_SRC_TMR6_8_GCMP_B     273U     /* TMR6_8_GCMB */
#define    INT_SRC_TMR6_8_GCMP_C     274U     /* TMR6_8_GCMC */
#define    INT_SRC_TMR6_8_GCMP_D     275U     /* TMR6_8_GCMD */
#define    INT_SRC_TMR6_8_GCMP_E     276U     /* TMR6_8_GCME */
#define    INT_SRC_TMR6_8_GCMP_F     277U     /* TMR6_8_GCMF */
#define    INT_SRC_TMR6_8_OVF        278U     /* TMR6_8_GOVF */
#define    INT_SRC_TMR6_8_UDF        279U     /* TMR6_8_GUDF */
#define    INT_SRC_TMR6_8_DTE        280U     /* TMR6_8_GDTE */
#define    INT_SRC_TMR6_8_SCMP_A     283U     /* TMR6_8_SCMA */
#define    INT_SRC_TMR6_8_SCMP_B     284U     /* TMR6_8_SCMB */
/*  TIMERA_4  */
#define    INT_SRC_TMRA_4_OVF        285U     /* TMRA_4_OVF */
#define    INT_SRC_TMRA_4_UDF        286U     /* TMRA_4_UDF */
#define    INT_SRC_TMRA_4_CMP        287U     /* TMRA_4_CMP */
/*  EMB  */
#define    INT_SRC_EMB_GR0           288U     /* EMB_GR0 */
#define    INT_SRC_EMB_GR1           289U     /* EMB_GR1 */
#define    INT_SRC_EMB_GR2           290U     /* EMB_GR2 */
#define    INT_SRC_EMB_GR3           291U     /* EMB_GR3 */
#define    INT_SRC_EMB_GR4           292U     /* EMB_GR4 */
#define    INT_SRC_EMB_GR5           293U     /* EMB_GR5 */
#define    INT_SRC_EMB_GR6           294U     /* EMB_GR6 */
/*  USBHS  */
#define    INT_SRC_USBHS_EP1_OUT     295U     /* USBHS_EP1_OUT */
#define    INT_SRC_USBHS_EP1_IN      296U     /* USBHS_EP1_IN */
#define    INT_SRC_USBHS_GLB         297U     /* USBHS_GLB */
#define    INT_SRC_USBHS_WKUP        298U     /* USBHS_WKUP */
/*  USART1 USART2  */
#define    INT_SRC_USART1_EI         300U     /* USART_1_EI */
#define    INT_SRC_USART1_RI         301U     /* USART_1_RI */
#define    INT_SRC_USART1_TI         302U     /* USART_1_TI */
#define    INT_SRC_USART1_TCI        303U     /* USART_1_TCI */
#define    INT_SRC_USART1_RTO        304U     /* USART_1_RTO */
#define    INT_SRC_USART2_EI         305U     /* USART_2_EI */
#define    INT_SRC_USART2_RI         306U     /* USART_2_RI */
#define    INT_SRC_USART2_TI         307U     /* USART_2_TI */
#define    INT_SRC_USART2_TCI        308U     /* USART_2_TCI */
#define    INT_SRC_USART2_RTO        309U     /* USART_2_RTO */
/*  SPI1 SPI2  */
#define    INT_SRC_SPI1_SPRI         310U     /* SPI_1_SPRI */
#define    INT_SRC_SPI1_SPTI         311U     /* SPI_1_SPTI */
#define    INT_SRC_SPI1_SPII         312U     /* SPI_1_SPII */
#define    INT_SRC_SPI1_SPEI         313U     /* SPI_1_SPEI */
#define    INT_SRC_SPI2_SPRI         315U     /* SPI_2_SPRI */
#define    INT_SRC_SPI2_SPTI         316U     /* SPI_2_SPTI */
#define    INT_SRC_SPI2_SPII         317U     /* SPI_2_SPII */
#define    INT_SRC_SPI2_SPEI         318U     /* SPI_2_SPEI */
/*  TIMERA_5 TIMERA_6 TIMERA_7 TIMERA*/
#define    INT_SRC_TMRA_5_OVF        320U     /* TMRA_5_OVF */
#define    INT_SRC_TMRA_5_UDF        321U     /* TMRA_5_UDF */
#define    INT_SRC_TMRA_5_CMP        322U     /* TMRA_5_CMP */
#define    INT_SRC_TMRA_6_OVF        323U     /* TMRA_6_OVF */
#define    INT_SRC_TMRA_6_UDF        324U     /* TMRA_6_UDF */
#define    INT_SRC_TMRA_6_CMP        325U     /* TMRA_6_CMP */
#define    INT_SRC_TMRA_7_OVF        326U     /* TMRA_7_OVF */
#define    INT_SRC_TMRA_7_UDF        327U     /* TMRA_7_UDF */
#define    INT_SRC_TMRA_7_CMP        328U     /* TMRA_7_CMP */
#define    INT_SRC_TMRA_8_OVF        329U     /* TMRA_8_OVF */
#define    INT_SRC_TMRA_8_UDF        330U     /* TMRA_8_UDF */
#define    INT_SRC_TMRA_8_CMP        331U     /* TMRA_8_CMP */
/*  USART3 USART4  */
#define    INT_SRC_USART3_EI         332U     /* USART_3_EI */
#define    INT_SRC_USART3_RI         333U     /* USART_3_RI */
#define    INT_SRC_USART3_TI         334U     /* USART_3_TI */
#define    INT_SRC_USART3_TCI        335U     /* USART_3_TCI */
#define    INT_SRC_USART4_EI         336U     /* USART_4_EI */
#define    INT_SRC_USART4_RI         337U     /* USART_4_RI */
#define    INT_SRC_USART4_TI         338U     /* USART_4_TI */
#define    INT_SRC_USART4_TCI        339U     /* USART_4_TCI */
/*  CAN1 CAN2  */
#define    INT_SRC_CAN1_HOST         340U     /* CAN_1_HOST */
#define    INT_SRC_CAN2_HOST         341U     /* CAN_2_HOST */
/*  SPI3 SPI4  */
#define    INT_SRC_SPI3_SPRI         342U     /* SPI_3_SPRI */
#define    INT_SRC_SPI3_SPTI         343U     /* SPI_3_SPTI */
#define    INT_SRC_SPI3_SPII         344U     /* SPI_3_SPII */
#define    INT_SRC_SPI3_SPEI         345U     /* SPI_3_SPEI */
#define    INT_SRC_SPI4_SPRI         347U     /* SPI_4_SPRI */
#define    INT_SRC_SPI4_SPTI         348U     /* SPI_4_SPTI */
#define    INT_SRC_SPI4_SPII         349U     /* SPI_4_SPII */
#define    INT_SRC_SPI4_SPEI         350U     /* SPI_4_SPEI */
/*  TIMERA_9 TIMERA_10 TIMER_11 TIMER */
#define    INT_SRC_TMRA_9_OVF        352U     /* TMRA_9_OVF */
#define    INT_SRC_TMRA_9_UDF        353U     /* TMRA_9_UDF */
#define    INT_SRC_TMRA_9_CMP        354U     /* TMRA_9_CMP */
#define    INT_SRC_TMRA_10_OVF       355U     /* TMRA_10_OVF */
#define    INT_SRC_TMRA_10_UDF       356U     /* TMRA_10_UDF */
#define    INT_SRC_TMRA_10_CMP       357U     /* TMRA_10_CMP */
#define    INT_SRC_TMRA_11_OVF       358U     /* TMRA_11_OVF */
#define    INT_SRC_TMRA_11_UDF       359U     /* TMRA_11_UDF */
#define    INT_SRC_TMRA_11_CMP       360U     /* TMRA_11_CMP */
#define    INT_SRC_TMRA_12_OVF       361U     /* TMRA_12_OVF */
#define    INT_SRC_TMRA_12_UDF       362U     /* TMRA_12_UDF */
#define    INT_SRC_TMRA_12_CMP       363U     /* TMRA_12_CMP */
/*  USART5 USART6  */
#define    INT_SRC_USART5_BRKWKPI    364U     /* USART_5_BRKWKPI */
#define    INT_SRC_USART5_EI         365U     /* USART_5_EI */
#define    INT_SRC_USART5_RI         366U     /* USART_5_RI */
#define    INT_SRC_USART5_TI         367U     /* USART_5_TI */
#define    INT_SRC_USART5_TCI        368U     /* USART_5_TCI */
#define    INT_SRC_USART6_EI         369U     /* USART_6_EI */
#define    INT_SRC_USART6_RI         370U     /* USART_6_RI */
#define    INT_SRC_USART6_TI         371U     /* USART_6_TI */
#define    INT_SRC_USART6_TCI        372U     /* USART_6_TCI */
#define    INT_SRC_USART6_RTO        373U     /* USART_6_RTO */
/*  SPI5 SPI6  */
#define    INT_SRC_SPI5_SPRI         374U     /* SPI_5_SPRI */
#define    INT_SRC_SPI5_SPTI         375U     /* SPI_5_SPTI */
#define    INT_SRC_SPI5_SPII         376U     /* SPI_5_SPII */
#define    INT_SRC_SPI5_SPEI         377U     /* SPI_5_SPEI */
#define    INT_SRC_SPI6_SPRI         379U     /* SPI_6_SPRI */
#define    INT_SRC_SPI6_SPTI         380U     /* SPI_6_SPTI */
#define    INT_SRC_SPI6_SPII         381U     /* SPI_6_SPII */
#define    INT_SRC_SPI6_SPEI         382U     /* SPI_6_SPEI */
/*  I2S1 I2S2  */
#define    INT_SRC_I2S1_TXIRQOUT     384U     /* I2S_1_TXIRQOUT */
#define    INT_SRC_I2S1_RXIRQOUT     385U     /* I2S_1_RXIRQOUT */
#define    INT_SRC_I2S1_ERRIRQOUT    386U     /* I2S_1_ERRIRQOUT */
#define    INT_SRC_I2S2_TXIRQOUT     387U     /* I2S_2_TXIRQOUT */
#define    INT_SRC_I2S2_RXIRQOUT     388U     /* I2S_2_RXIRQOUT */
#define    INT_SRC_I2S2_ERRIRQOUT    389U     /* I2S_2_ERRIRQOUT */
/*  USART7 USART8  */
#define    INT_SRC_USART7_EI         390U     /* USART_7_EI */
#define    INT_SRC_USART7_RI         391U     /* USART_7_RI */
#define    INT_SRC_USART7_TI         392U     /* USART_7_TI */
#define    INT_SRC_USART7_TCI        393U     /* USART_7_TCI */
#define    INT_SRC_USART7_RTO        394U     /* USART_7_RTO */
#define    INT_SRC_USART8_EI         395U     /* USART_8_EI */
#define    INT_SRC_USART8_RI         396U     /* USART_8_RI */
#define    INT_SRC_USART8_TI         397U     /* USART_8_TI */
#define    INT_SRC_USART8_TCI        398U     /* USART_8_TCI */
/*  USBFS  */
#define    INT_SRC_USBFS_GLB         399U     /* USBFS_GLB */
#define    INT_SRC_USBFS_WKUP        400U     /* USBFS_WKUP */
/*  HASH  */
#define    INT_SRC_HASH              401U     /* HASH_INT */
/*  SDIOC  */
#define    INT_SRC_SDIOC1_SD         404U     /* SDIOC_1_SD */
#define    INT_SRC_SDIOC2_SD         407U     /* SDIOC_2_SD */
/*  EVENT PORT  */
#define    INT_SRC_EVENT_PORT1       408U     /* EVENT_PORT1 */
#define    INT_SRC_EVENT_PORT2       409U     /* EVENT_PORT2 */
#define    INT_SRC_EVENT_PORT3       410U     /* EVENT_PORT3 */
#define    INT_SRC_EVENT_PORT4       411U     /* EVENT_PORT4 */
/*  ETHER  */
#define    INT_SRC_ETH_GLB_INT       412U     /* ETH_GLB_INT */
#define    INT_SRC_ETH_WKP_INT       413U     /* ETH_WKP_INT */
/*  I2S3 I2S4  */
#define    INT_SRC_I2S3_TXIRQOUT     416U     /* I2S_3_TXIRQOUT */
#define    INT_SRC_I2S3_RXIRQOUT     417U     /* I2S_3_RXIRQOUT */
#define    INT_SRC_I2S3_ERRIRQOUT    418U     /* I2S_3_ERRIRQOUT */
#define    INT_SRC_I2S4_TXIRQOUT     419U     /* I2S_4_TXIRQOUT */
#define    INT_SRC_I2S4_RXIRQOUT     420U     /* I2S_4_RXIRQOUT */
#define    INT_SRC_I2S4_ERRIRQOUT    421U     /* I2S_4_ERRIRQOUT */
/*  USART9 USART10  */
#define    INT_SRC_USART9_EI         422U     /* USART_9_EI */
#define    INT_SRC_USART9_RI         423U     /* USART_9_RI */
#define    INT_SRC_USART9_TI         424U     /* USART_9_TI */
#define    INT_SRC_USART9_TCI        425U     /* USART_9_TCI */
#define    INT_SRC_USART10_BRKWKPI   426U     /* USART_10_BRKWKPI */
#define    INT_SRC_USART10_EI        427U     /* USART_10_EI */
#define    INT_SRC_USART10_RI        428U     /* USART_10_RI */
#define    INT_SRC_USART10_TI        429U     /* USART_10_TI */
#define    INT_SRC_USART10_TCI       430U     /* USART_10_TCI */
/*  I2C1 I2C2 I2C3  */
#define    INT_SRC_I2C1_RXI          432U     /* I2C_1_RXI */
#define    INT_SRC_I2C1_TXI          433U     /* I2C_1_TXI */
#define    INT_SRC_I2C1_TEI          434U     /* I2C_1_TEI */
#define    INT_SRC_I2C1_EEI          435U     /* I2C_1_EEI */
#define    INT_SRC_I2C2_RXI          436U     /* I2C_2_RXI */
#define    INT_SRC_I2C2_TXI          437U     /* I2C_2_TXI */
#define    INT_SRC_I2C2_TEI          438U     /* I2C_2_TEI */
#define    INT_SRC_I2C2_EEI          439U     /* I2C_2_EEI */
#define    INT_SRC_I2C3_RXI          440U     /* I2C_3_RXI */
#define    INT_SRC_I2C3_TXI          441U     /* I2C_3_TXI */
#define    INT_SRC_I2C3_TEI          442U     /* I2C_3_TEI */
#define    INT_SRC_I2C3_EEI          443U     /* I2C_3_EEI */
/*  ACMP  */
#define    INT_SRC_CMP1              444U     /* CMP1 */
#define    INT_SRC_CMP2              445U     /* CMP2 */
#define    INT_SRC_CMP3              446U     /* CMP3 */
#define    INT_SRC_CMP4              447U     /* CMP4 */
/*  I2C4 I2C5 I2C6  */
#define    INT_SRC_I2C4_RXI          448U     /* I2C_4_RXI */
#define    INT_SRC_I2C4_TXI          449U     /* I2C_4_TXI */
#define    INT_SRC_I2C4_TEI          450U     /* I2C_4_TEI */
#define    INT_SRC_I2C4_EEI          451U     /* I2C_4_EEI */
#define    INT_SRC_I2C5_RXI          452U     /* I2C_5_RXI */
#define    INT_SRC_I2C5_TXI          453U     /* I2C_5_TXI */
#define    INT_SRC_I2C5_TEI          454U     /* I2C_5_TEI */
#define    INT_SRC_I2C5_EEI          455U     /* I2C_5_EEI */
#define    INT_SRC_I2C6_RXI          456U     /* I2C_6_RXI */
#define    INT_SRC_I2C6_TXI          457U     /* I2C_6_TXI */
#define    INT_SRC_I2C6_TEI          458U     /* I2C_6_TEI */
#define    INT_SRC_I2C6_EEI          459U     /* I2C_6_EEI */
/*  USART1  */
#define    INT_SRC_USART1_WUPI       460U     /* USART_1_WUPI */
/*  LVD  */
#define    INT_SRC_LVD1              461U     /* LVD1 */
#define    INT_SRC_LVD2              462U     /* LVD2 */
/*  OTS  */
#define    INT_SRC_OTS               463U     /* OTS */
/*  FCM  */
#define    INT_SRC_FCMFERRI          464U     /* FCMFERRI */
#define    INT_SRC_FCMMENDI          465U     /* FCMMENDI */
#define    INT_SRC_FCMCOVFI          466U     /* FCMCOVFI */
/*  WDT  */
#define    INT_SRC_WDT_REFUDF        467U     /* WDT_REFUDF */
/*  CTC  */
#define    INT_SRC_CTC_ERR           468U     /* CTC_ERR */
/*  ADC  */
#define    INT_SRC_ADC1_EOCA         480U     /* ADC_1_EOCA */
#define    INT_SRC_ADC1_EOCB         481U     /* ADC_1_EOCB */
#define    INT_SRC_ADC1_CMP0         482U     /* ADC_1_CMP0 */
#define    INT_SRC_ADC1_CMP1         483U     /* ADC_1_CMP1 */
#define    INT_SRC_ADC2_EOCA         484U     /* ADC_2_EOCA */
#define    INT_SRC_ADC2_EOCB         485U     /* ADC_2_EOCB */
#define    INT_SRC_ADC2_CMP0         486U     /* ADC_2_CMP0 */
#define    INT_SRC_ADC2_CMP1         487U     /* ADC_2_CMP1 */
#define    INT_SRC_ADC3_EOCA         488U     /* ADC_3_EOCA */
#define    INT_SRC_ADC3_EOCB         489U     /* ADC_3_EOCB */
#define    INT_SRC_ADC3_CMP0         490U     /* ADC_3_CMP0 */
#define    INT_SRC_ADC3_CMP1         491U     /* ADC_3_CMP1 */
/*  TRNG  */
#define    INT_SRC_TRNG_END          492U     /* TRNG_END */
/*  NFC  */
#define    INT_SRC_NFC_INT           496U     /* NFC_INT */
#define    INT_SRC_MAX               511U

/**
 * @brief Event Source Number
 */
#define    EVT_SRC_SWI_IRQ0          0       /* SWI_IRQ0  */
#define    EVT_SRC_SWI_IRQ1          1       /* SWI_IRQ1  */
#define    EVT_SRC_SWI_IRQ2          2       /* SWI_IRQ2  */
#define    EVT_SRC_SWI_IRQ3          3       /* SWI_IRQ3  */
#define    EVT_SRC_SWI_IRQ4          4       /* SWI_IRQ4  */
#define    EVT_SRC_SWI_IRQ5          5       /* SWI_IRQ5  */
#define    EVT_SRC_SWI_IRQ6          6       /* SWI_IRQ6  */
#define    EVT_SRC_SWI_IRQ7          7       /* SWI_IRQ7  */
#define    EVT_SRC_SWI_IRQ8          8       /* SWI_IRQ8  */
#define    EVT_SRC_SWI_IRQ9          9       /* SWI_IRQ9  */
#define    EVT_SRC_SWI_IRQ10         10      /* SWI_IRQ10 */
#define    EVT_SRC_SWI_IRQ11         11      /* SWI_IRQ11 */
#define    EVT_SRC_SWI_IRQ12         12      /* SWI_IRQ12 */
#define    EVT_SRC_SWI_IRQ13         13      /* SWI_IRQ13 */
#define    EVT_SRC_SWI_IRQ14         14      /* SWI_IRQ14 */
#define    EVT_SRC_SWI_IRQ15         15      /* SWI_IRQ15 */
#define    EVT_SRC_SWI_IRQ16         16      /* SWI_IRQ16 */
#define    EVT_SRC_SWI_IRQ17         17      /* SWI_IRQ17 */
#define    EVT_SRC_SWI_IRQ18         18      /* SWI_IRQ18 */
#define    EVT_SRC_SWI_IRQ19         19      /* SWI_IRQ19 */
#define    EVT_SRC_SWI_IRQ20         20      /* SWI_IRQ20 */
#define    EVT_SRC_SWI_IRQ21         21      /* SWI_IRQ21 */
#define    EVT_SRC_SWI_IRQ22         22      /* SWI_IRQ22 */
#define    EVT_SRC_SWI_IRQ23         23      /* SWI_IRQ23 */
#define    EVT_SRC_SWI_IRQ24         24      /* SWI_IRQ24 */
#define    EVT_SRC_SWI_IRQ25         25      /* SWI_IRQ25 */
#define    EVT_SRC_SWI_IRQ26         26      /* SWI_IRQ26 */
#define    EVT_SRC_SWI_IRQ27         27      /* SWI_IRQ27 */
#define    EVT_SRC_SWI_IRQ28         28      /* SWI_IRQ28 */
#define    EVT_SRC_SWI_IRQ29         29      /* SWI_IRQ29 */
#define    EVT_SRC_SWI_IRQ30         30      /* SWI_IRQ30 */
#define    EVT_SRC_SWI_IRQ31         31      /* SWI_IRQ31 */

/* External Interrupt. */
#define    EVT_SRC_PORT_EIRQ0        0       /* PORT_EIRQ0  */
#define    EVT_SRC_PORT_EIRQ1        1       /* PORT_EIRQ1  */
#define    EVT_SRC_PORT_EIRQ2        2       /* PORT_EIRQ2  */
#define    EVT_SRC_PORT_EIRQ3        3       /* PORT_EIRQ3  */
#define    EVT_SRC_PORT_EIRQ4        4       /* PORT_EIRQ4  */
#define    EVT_SRC_PORT_EIRQ5        5       /* PORT_EIRQ5  */
#define    EVT_SRC_PORT_EIRQ6        6       /* PORT_EIRQ6  */
#define    EVT_SRC_PORT_EIRQ7        7       /* PORT_EIRQ7  */
#define    EVT_SRC_PORT_EIRQ8        8       /* PORT_EIRQ8  */
#define    EVT_SRC_PORT_EIRQ9        9       /* PORT_EIRQ9  */
#define    EVT_SRC_PORT_EIRQ10       10      /* PORT_EIRQ10 */
#define    EVT_SRC_PORT_EIRQ11       11      /* PORT_EIRQ11 */
#define    EVT_SRC_PORT_EIRQ12       12      /* PORT_EIRQ12 */
#define    EVT_SRC_PORT_EIRQ13       13      /* PORT_EIRQ13 */
#define    EVT_SRC_PORT_EIRQ14       14      /* PORT_EIRQ14 */
#define    EVT_SRC_PORT_EIRQ15       15      /* PORT_EIRQ15 */

/*  DMA_1  */
#define    EVT_SRC_DMA1_TC0          32U     /* DMA_1_TC0 */
#define    EVT_SRC_DMA1_TC1          33U     /* DMA_1_TC1 */
#define    EVT_SRC_DMA1_TC2          34U     /* DMA_1_TC2 */
#define    EVT_SRC_DMA1_TC3          35U     /* DMA_1_TC3 */
#define    EVT_SRC_DMA1_TC4          36U     /* DMA_1_TC4 */
#define    EVT_SRC_DMA1_TC5          37U     /* DMA_1_TC5 */
#define    EVT_SRC_DMA1_TC6          38U     /* DMA_1_TC6 */
#define    EVT_SRC_DMA1_TC7          39U     /* DMA_1_TC7 */
#define    EVT_SRC_DMA1_BTC0         40U     /* DMA_1_BTC0 */
#define    EVT_SRC_DMA1_BTC1         41U     /* DMA_1_BTC1 */
#define    EVT_SRC_DMA1_BTC2         42U     /* DMA_1_BTC2 */
#define    EVT_SRC_DMA1_BTC3         43U     /* DMA_1_BTC3 */
#define    EVT_SRC_DMA1_BTC4         44U     /* DMA_1_BTC4 */
#define    EVT_SRC_DMA1_BTC5         45U     /* DMA_1_BTC5 */
#define    EVT_SRC_DMA1_BTC6         46U     /* DMA_1_BTC6 */
#define    EVT_SRC_DMA1_BTC7         47U     /* DMA_1_BTC7 */
/*  EFM  */
#define    EVT_SRC_EFM_OPTEND        51U     /* EFM_OPTEND */
/*  USBFS  */
#define    EVT_SRC_USBFS_SOF         52U     /* USBFS_SOF */
/*  USBHS  */
#define    EVT_SRC_USBHS_SOF         53U     /* USBHS_SOF */
/*  DCU  */
#define    EVT_SRC_DCU1              55U     /* DCU1 */
#define    EVT_SRC_DCU2              56U     /* DCU2 */
#define    EVT_SRC_DCU3              57U     /* DCU3 */
#define    EVT_SRC_DCU4              58U     /* DCU4 */
#define    EVT_SRC_DCU5              59U     /* DCU5 */
#define    EVT_SRC_DCU6              60U     /* DCU6 */
#define    EVT_SRC_DCU7              61U     /* DCU7 */
#define    EVT_SRC_DCU8              62U     /* DCU8 */
/*  DMA_2  */
#define    EVT_SRC_DMA2_TC0          64U     /* DMA_2_TC0 */
#define    EVT_SRC_DMA2_TC1          65U     /* DMA_2_TC1 */
#define    EVT_SRC_DMA2_TC2          66U     /* DMA_2_TC2 */
#define    EVT_SRC_DMA2_TC3          67U     /* DMA_2_TC3 */
#define    EVT_SRC_DMA2_TC4          68U     /* DMA_2_TC4 */
#define    EVT_SRC_DMA2_TC5          69U     /* DMA_2_TC5 */
#define    EVT_SRC_DMA2_TC6          70U     /* DMA_2_TC6 */
#define    EVT_SRC_DMA2_TC7          71U     /* DMA_2_TC7 */
#define    EVT_SRC_DMA2_BTC0         72U     /* DMA_2_BTC0 */
#define    EVT_SRC_DMA2_BTC1         73U     /* DMA_2_BTC1 */
#define    EVT_SRC_DMA2_BTC2         74U     /* DMA_2_BTC2 */
#define    EVT_SRC_DMA2_BTC3         75U     /* DMA_2_BTC3 */
#define    EVT_SRC_DMA2_BTC4         76U     /* DMA_2_BTC4 */
#define    EVT_SRC_DMA2_BTC5         77U     /* DMA_2_BTC5 */
#define    EVT_SRC_DMA2_BTC6         78U     /* DMA_2_BTC6 */
#define    EVT_SRC_DMA2_BTC7         79U     /* DMA_2_BTC7 */
/*  MAU  */
#define    EVT_SRC_MAU_SQRT          83U     /* MAU_SQRT */
/*  DVP  */
#define    EVT_SRC_DVP_FRAMSTA       84U     /* DVP_FRAMSTA */
#define    EVT_SRC_DVP_LINESTA       85U     /* DVP_LINESTA */
#define    EVT_SRC_DVP_LINEEND       86U     /* DVP_LINEEND */
#define    EVT_SRC_DVP_FRAMEND       87U     /* DVP_FRAMEND */
#define    EVT_SRC_DVP_SQUERR        88U     /* DVP_SQUERR */
#define    EVT_SRC_DVP_FIFOERR       89U     /* DVP_FIFOERR */
#define    EVT_SRC_DVP_DMAREQ        90U     /* DVP_DMAREQ */
/*  FMAC  */
#define    EVT_SRC_FMAC_1            91U     /* FMAC_1_FIR */
#define    EVT_SRC_FMAC_2            92U     /* FMAC_2_FIR */
#define    EVT_SRC_FMAC_3            93U     /* FMAC_3_FIR */
#define    EVT_SRC_FMAC_4            94U     /* FMAC_4_FIR */
/*  TIMER0  */
#define    EVT_SRC_TMR0_1_CMP_A      96U     /* TMR0_1_CMPA */
#define    EVT_SRC_TMR0_1_CMP_B      97U     /* TMR0_1_CMPB */
#define    EVT_SRC_TMR0_2_CMP_A      98U     /* TMR0_2_CMPA */
#define    EVT_SRC_TMR0_2_CMP_B      99U     /* TMR0_2_CMPB */
/*  TIMER2  */
#define    EVT_SRC_TMR2_1_CMP_A      100U    /* TMR2_1_CMPA  */
#define    EVT_SRC_TMR2_1_CMP_B      101U    /* TMR2_1_CMPB  */
#define    EVT_SRC_TMR2_1_OVF_A      102U    /* TMR2_1_OVFA  */
#define    EVT_SRC_TMR2_1_OVF_B      103U    /* TMR2_1_OVFB  */
#define    EVT_SRC_TMR2_2_CMP_A      104U    /* TMR2_2_CMPA  */
#define    EVT_SRC_TMR2_2_CMP_B      105U    /* TMR2_2_CMPB  */
#define    EVT_SRC_TMR2_2_OVF_A      106U    /* TMR2_2_OVFA  */
#define    EVT_SRC_TMR2_2_OVF_B      107U    /* TMR2_2_OVFB  */
#define    EVT_SRC_TMR2_3_CMP_A      108U    /* TMR2_3_CMPA  */
#define    EVT_SRC_TMR2_3_CMP_B      109U    /* TMR2_3_CMPB  */
#define    EVT_SRC_TMR2_3_OVF_A      110U    /* TMR2_3_OVFA  */
#define    EVT_SRC_TMR2_3_OVF_B      111U    /* TMR2_3_OVFB  */
#define    EVT_SRC_TMR2_4_CMP_A      112U    /* TMR2_4_CMPA  */
#define    EVT_SRC_TMR2_4_CMP_B      113U    /* TMR2_4_CMPB  */
#define    EVT_SRC_TMR2_4_OVF_A      114U    /* TMR2_4_OVFA  */
#define    EVT_SRC_TMR2_4_OVF_B      115U    /* TMR2_4_OVFB  */
/*  RTC  */
#define    EVT_SRC_RTC_ALM           121U    /* RTC_ALM */
#define    EVT_SRC_RTC_PRD           122U    /* RTC_PRD */
/*  TIMER6_1  */
#define    EVT_SRC_TMR6_1_GCMP_A     128U    /* TMR6_1_GCMA */
#define    EVT_SRC_TMR6_1_GCMP_B     129U    /* TMR6_1_GCMB */
#define    EVT_SRC_TMR6_1_GCMP_C     130U    /* TMR6_1_GCMC */
#define    EVT_SRC_TMR6_1_GCMP_D     131U    /* TMR6_1_GCMD */
#define    EVT_SRC_TMR6_1_GCMP_E     132U    /* TMR6_1_GCME */
#define    EVT_SRC_TMR6_1_GCMP_F     133U    /* TMR6_1_GCMF */
#define    EVT_SRC_TMR6_1_OVF        134U    /* TMR6_1_GOVF */
#define    EVT_SRC_TMR6_1_UDF        135U    /* TMR6_1_GUDF */
/*  TIMER4_1  */
#define    EVT_SRC_TMR4_1_SCMP0      136U    /* TMR4_1_SCM0 */
#define    EVT_SRC_TMR4_1_SCMP1      137U    /* TMR4_1_SCM1 */
#define    EVT_SRC_TMR4_1_SCMP2      138U    /* TMR4_1_SCM2 */
#define    EVT_SRC_TMR4_1_SCMP3      139U    /* TMR4_1_SCM3 */
#define    EVT_SRC_TMR4_1_SCMP4      140U    /* TMR4_1_SCM4 */
#define    EVT_SRC_TMR4_1_SCMP5      141U    /* TMR4_1_SCM5 */
/*  TIMER6_2  */
#define    EVT_SRC_TMR6_2_GCMP_A     144U    /* TMR6_2_GCMA */
#define    EVT_SRC_TMR6_2_GCMP_B     145U    /* TMR6_2_GCMB */
#define    EVT_SRC_TMR6_2_GCMP_C     146U    /* TMR6_2_GCMC */
#define    EVT_SRC_TMR6_2_GCMP_D     147U    /* TMR6_2_GCMD */
#define    EVT_SRC_TMR6_2_GCMP_E     148U    /* TMR6_2_GCME */
#define    EVT_SRC_TMR6_2_GCMP_F     149U    /* TMR6_2_GCMF */
#define    EVT_SRC_TMR6_2_OVF        150U    /* TMR6_2_GOVF */
#define    EVT_SRC_TMR6_2_UDF        151U    /* TMR6_2_GUDF */
/*  TIMER4_2  */
#define    EVT_SRC_TMR4_2_SCMP0      152U    /* TMR4_2_SCM0 */
#define    EVT_SRC_TMR4_2_SCMP1      153U    /* TMR4_2_SCM1 */
#define    EVT_SRC_TMR4_2_SCMP2      154U    /* TMR4_2_SCM2 */
#define    EVT_SRC_TMR4_2_SCMP3      155U    /* TMR4_2_SCM3 */
#define    EVT_SRC_TMR4_2_SCMP4      156U    /* TMR4_2_SCM4 */
#define    EVT_SRC_TMR4_2_SCMP5      157U    /* TMR4_2_SCM5 */
/*  TIMER6_3  */
#define    EVT_SRC_TMR6_3_GCMP_A     160U    /* TMR6_3_GCMA */
#define    EVT_SRC_TMR6_3_GCMP_B     161U    /* TMR6_3_GCMB */
#define    EVT_SRC_TMR6_3_GCMP_C     162U    /* TMR6_3_GCMC */
#define    EVT_SRC_TMR6_3_GCMP_D     163U    /* TMR6_3_GCMD */
#define    EVT_SRC_TMR6_3_GCMP_E     164U    /* TMR6_3_GCME */
#define    EVT_SRC_TMR6_3_GCMP_F     165U    /* TMR6_3_GCMF */
#define    EVT_SRC_TMR6_3_OVF        166U    /* TMR6_3_GOVF */
#define    EVT_SRC_TMR6_3_UDF        167U    /* TMR6_3_GUDF */
/*  TIMER4_3  */
#define    EVT_SRC_TMR4_3_SCMP0      168U    /* TMR4_3_SCM0 */
#define    EVT_SRC_TMR4_3_SCMP1      169U    /* TMR4_3_SCM1 */
#define    EVT_SRC_TMR4_3_SCMP2      170U    /* TMR4_3_SCM2 */
#define    EVT_SRC_TMR4_3_SCMP3      171U    /* TMR4_3_SCM3 */
#define    EVT_SRC_TMR4_3_SCMP4      172U    /* TMR4_3_SCM4 */
#define    EVT_SRC_TMR4_3_SCMP5      173U    /* TMR4_3_SCM5 */
/*  TIMER6  */
#define    EVT_SRC_TMR6_1_SCMP_A     179U    /* TMR6_1_SCMA */
#define    EVT_SRC_TMR6_1_SCMP_B     180U    /* TMR6_1_SCMB */
#define    EVT_SRC_TMR6_2_SCMP_A     187U    /* TMR6_2_SCMA */
#define    EVT_SRC_TMR6_2_SCMP_B     188U    /* TMR6_2_SCMB */
#define    EVT_SRC_TMR6_3_SCMP_A     195U    /* TMR6_3_SCMA */
#define    EVT_SRC_TMR6_3_SCMP_B     196U    /* TMR6_3_SCMB */
#define    EVT_SRC_TMR6_4_GCMP_A     208U    /* TMR6_4_GCMA */
#define    EVT_SRC_TMR6_4_GCMP_B     209U    /* TMR6_4_GCMB */
#define    EVT_SRC_TMR6_4_GCMP_C     210U    /* TMR6_4_GCMC */
#define    EVT_SRC_TMR6_4_GCMP_D     211U    /* TMR6_4_GCMD */
#define    EVT_SRC_TMR6_4_GCMP_E     212U    /* TMR6_4_GCME */
#define    EVT_SRC_TMR6_4_GCMP_F     213U    /* TMR6_4_GCMF */
#define    EVT_SRC_TMR6_4_OVF        214U    /* TMR6_4_GOVF */
#define    EVT_SRC_TMR6_4_UDF        215U    /* TMR6_4_GUDF */
#define    EVT_SRC_TMR6_4_SCMP_A     219U    /* TMR6_4_SCMA */
#define    EVT_SRC_TMR6_4_SCMP_B     220U    /* TMR6_4_SCMB */
#define    EVT_SRC_TMR6_5_GCMP_A     224U    /* TMR6_5_GCMA */
#define    EVT_SRC_TMR6_5_GCMP_B     225U    /* TMR6_5_GCMB */
#define    EVT_SRC_TMR6_5_GCMP_C     226U    /* TMR6_5_GCMC */
#define    EVT_SRC_TMR6_5_GCMP_D     227U    /* TMR6_5_GCMD */
#define    EVT_SRC_TMR6_5_GCMP_E     228U    /* TMR6_5_GCME */
#define    EVT_SRC_TMR6_5_GCMP_F     229U    /* TMR6_5_GCMF */
#define    EVT_SRC_TMR6_5_OVF        230U    /* TMR6_5_GOVF */
#define    EVT_SRC_TMR6_5_UDF        231U    /* TMR6_5_GUDF */
#define    EVT_SRC_TMR6_5_SCMP_A     235U    /* TMR6_5_SCMA */
#define    EVT_SRC_TMR6_5_SCMP_B     236U    /* TMR6_5_SCMB */
/*  TIMERA_1  */
#define    EVT_SRC_TMRA_1_OVF        237U    /* TMRA_1_OVF */
#define    EVT_SRC_TMRA_1_UDF        238U    /* TMRA_1_UDF */
#define    EVT_SRC_TMRA_1_CMP        239U    /* TMRA_1_CMP */
/*  TIMER6_6  */
#define    EVT_SRC_TMR6_6_GCMP_A     240U    /* TMR6_6_GCMA */
#define    EVT_SRC_TMR6_6_GCMP_B     241U    /* TMR6_6_GCMB */
#define    EVT_SRC_TMR6_6_GCMP_C     242U    /* TMR6_6_GCMC */
#define    EVT_SRC_TMR6_6_GCMP_D     243U    /* TMR6_6_GCMD */
#define    EVT_SRC_TMR6_6_GCMP_E     244U    /* TMR6_6_GCME */
#define    EVT_SRC_TMR6_6_GCMP_F     245U    /* TMR6_6_GCMF */
#define    EVT_SRC_TMR6_6_OVF        246U    /* TMR6_6_GOVF */
#define    EVT_SRC_TMR6_6_UDF        247U    /* TMR6_6_GUDF */
#define    EVT_SRC_TMR6_6_SCMP_A     251U    /* TMR6_6_SCMA */
#define    EVT_SRC_TMR6_6_SCMP_B     252U    /* TMR6_6_SCMB */
/*  TIMERA_2  */
#define    EVT_SRC_TMRA_2_OVF        253U    /* TMRA_2_OVF */
#define    EVT_SRC_TMRA_2_UDF        254U    /* TMRA_2_UDF */
#define    EVT_SRC_TMRA_2_CMP        255U    /* TMRA_2_CMP */
/*  TIMER6_7  */
#define    EVT_SRC_TMR6_7_GCMP_A     256U    /* TMR6_7_GCMA */
#define    EVT_SRC_TMR6_7_GCMP_B     257U    /* TMR6_7_GCMB */
#define    EVT_SRC_TMR6_7_GCMP_C     258U    /* TMR6_7_GCMC */
#define    EVT_SRC_TMR6_7_GCMP_D     259U    /* TMR6_7_GCMD */
#define    EVT_SRC_TMR6_7_GCMP_E     260U    /* TMR6_7_GCME */
#define    EVT_SRC_TMR6_7_GCMP_F     261U    /* TMR6_7_GCMF */
#define    EVT_SRC_TMR6_7_OVF        262U    /* TMR6_7_GOVF */
#define    EVT_SRC_TMR6_7_UDF        263U    /* TMR6_7_GUDF */
#define    EVT_SRC_TMR6_7_SCMP_A     267U    /* TMR6_7_SCMA */
#define    EVT_SRC_TMR6_7_SCMP_B     268U    /* TMR6_7_SCMB */
/*  TIMERA_3  */
#define    EVT_SRC_TMRA_3_OVF        269U    /* TMRA_3_OVF */
#define    EVT_SRC_TMRA_3_UDF        270U    /* TMRA_3_UDF */
#define    EVT_SRC_TMRA_3_CMP        271U    /* TMRA_3_CMP */
/*  TIMER6_8  */
#define    EVT_SRC_TMR6_8_GCMP_A     272U    /* TMR6_8_GCMA */
#define    EVT_SRC_TMR6_8_GCMP_B     273U    /* TMR6_8_GCMB */
#define    EVT_SRC_TMR6_8_GCMP_C     274U    /* TMR6_8_GCMC */
#define    EVT_SRC_TMR6_8_GCMP_D     275U    /* TMR6_8_GCMD */
#define    EVT_SRC_TMR6_8_GCMP_E     276U    /* TMR6_8_GCME */
#define    EVT_SRC_TMR6_8_GCMP_F     277U    /* TMR6_8_GCMF */
#define    EVT_SRC_TMR6_8_OVF        278U    /* TMR6_8_GOVF */
#define    EVT_SRC_TMR6_8_UDF        279U    /* TMR6_8_GUDF */
#define    EVT_SRC_TMR6_8_SCMP_A     283U    /* TMR6_8_SCMA */
#define    EVT_SRC_TMR6_8_SCMP_B     284U    /* TMR6_8_SCMB */
/*  TIMERA_4  */
#define    EVT_SRC_TMRA_4_OVF        285U    /* TMRA_4_OVF */
#define    EVT_SRC_TMRA_4_UDF        286U    /* TMRA_4_UDF */
#define    EVT_SRC_TMRA_4_CMP        287U    /* TMRA_4_CMP */
/*  AOS_STRG  */
#define    EVT_SRC_AOS_STRG          299U    /* AOS_STRG */
/*  USART1 USART2  */
#define    EVT_SRC_USART1_EI         300U    /* USART_1_EI */
#define    EVT_SRC_USART1_RI         301U    /* USART_1_RI */
#define    EVT_SRC_USART1_TI         302U    /* USART_1_TI */
#define    EVT_SRC_USART1_TCI        303U    /* USART_1_TCI */
#define    EVT_SRC_USART1_RTO        304U    /* USART_1_RTO */
#define    EVT_SRC_USART2_EI         305U    /* USART_2_EI */
#define    EVT_SRC_USART2_RI         306U    /* USART_2_RI */
#define    EVT_SRC_USART2_TI         307U    /* USART_2_TI */
#define    EVT_SRC_USART2_TCI        308U    /* USART_2_TCI */
#define    EVT_SRC_USART2_RTO        309U    /* USART_2_RTO */
/*  SPI1 SPI2  */
#define    EVT_SRC_SPI1_SPRI         310U    /* SPI_1_SPRI */
#define    EVT_SRC_SPI1_SPTI         311U    /* SPI_1_SPTI */
#define    EVT_SRC_SPI1_SPII         312U    /* SPI_1_SPII */
#define    EVT_SRC_SPI1_SPEI         313U    /* SPI_1_SPEI */
#define    EVT_SRC_SPI1_SPEND        314U    /* SPI_1_SPEND */
#define    EVT_SRC_SPI2_SPRI         315U    /* SPI_2_SPRI */
#define    EVT_SRC_SPI2_SPTI         316U    /* SPI_2_SPTI */
#define    EVT_SRC_SPI2_SPII         317U    /* SPI_2_SPII */
#define    EVT_SRC_SPI2_SPEI         318U    /* SPI_2_SPEI */
#define    EVT_SRC_SPI2_SPEND        319U    /* SPI_2_SPEND */
/*  TIMERA_5 TIMERA_6 TIMERA_7 TIMERA_8  */
#define    EVT_SRC_TMRA_5_OVF        320U    /* TMRA_5_OVF */
#define    EVT_SRC_TMRA_5_UDF        321U    /* TMRA_5_UDF */
#define    EVT_SRC_TMRA_5_CMP        322U    /* TMRA_5_CMP */
#define    EVT_SRC_TMRA_6_OVF        323U    /* TMRA_6_OVF */
#define    EVT_SRC_TMRA_6_UDF        324U    /* TMRA_6_UDF */
#define    EVT_SRC_TMRA_6_CMP        325U    /* TMRA_6_CMP */
#define    EVT_SRC_TMRA_7_OVF        326U    /* TMRA_7_OVF */
#define    EVT_SRC_TMRA_7_UDF        327U    /* TMRA_7_UDF */
#define    EVT_SRC_TMRA_7_CMP        328U    /* TMRA_7_CMP */
#define    EVT_SRC_TMRA_8_OVF        329U    /* TMRA_8_OVF */
#define    EVT_SRC_TMRA_8_UDF        330U    /* TMRA_8_UDF */
#define    EVT_SRC_TMRA_8_CMP        331U    /* TMRA_8_CMP */
/*  USART3 USART4  */
#define    EVT_SRC_USART3_EI         332U    /* USART_3_EI */
#define    EVT_SRC_USART3_RI         333U    /* USART_3_RI */
#define    EVT_SRC_USART3_TI         334U    /* USART_3_TI */
#define    EVT_SRC_USART3_TCI        335U    /* USART_3_TCI */
#define    EVT_SRC_USART4_EI         336U    /* USART_4_EI */
#define    EVT_SRC_USART4_RI         337U    /* USART_4_RI */
#define    EVT_SRC_USART4_TI         338U    /* USART_4_TI */
#define    EVT_SRC_USART4_TCI        339U    /* USART_4_TCI */
/*  SPI3 SPI4  */
#define    EVT_SRC_SPI3_SPRI         342U    /* SPI_3_SPRI */
#define    EVT_SRC_SPI3_SPTI         343U    /* SPI_3_SPTI */
#define    EVT_SRC_SPI3_SPII         344U    /* SPI_3_SPII */
#define    EVT_SRC_SPI3_SPEI         345U    /* SPI_3_SPEI */
#define    EVT_SRC_SPI3_SPEND        346U    /* SPI_3_SPEND */
#define    EVT_SRC_SPI4_SPRI         347U    /* SPI_4_SPRI */
#define    EVT_SRC_SPI4_SPTI         348U    /* SPI_4_SPTI */
#define    EVT_SRC_SPI4_SPII         349U    /* SPI_4_SPII */
#define    EVT_SRC_SPI4_SPEI         350U    /* SPI_4_SPEI */
#define    EVT_SRC_SPI4_SPEND        351U    /* SPI_4_SPEND */
/*  TIMERA_9 TIMERA_10 TIMERA_11 TIMERA_12  */
#define    EVT_SRC_TMRA_9_OVF        352U    /* TMRA_9_OVF */
#define    EVT_SRC_TMRA_9_UDF        353U    /* TMRA_9_UDF */
#define    EVT_SRC_TMRA_9_CMP        354U    /* TMRA_9_CMP */
#define    EVT_SRC_TMRA_10_OVF       355U    /* TMRA_10_OVF */
#define    EVT_SRC_TMRA_10_UDF       356U    /* TMRA_10_UDF */
#define    EVT_SRC_TMRA_10_CMP       357U    /* TMRA_10_CMP */
#define    EVT_SRC_TMRA_11_OVF       358U    /* TMRA_11_OVF */
#define    EVT_SRC_TMRA_11_UDF       359U    /* TMRA_11_UDF */
#define    EVT_SRC_TMRA_11_CMP       360U    /* TMRA_11_CMP */
#define    EVT_SRC_TMRA_12_OVF       361U    /* TMRA_12_OVF */
#define    EVT_SRC_TMRA_12_UDF       362U    /* TMRA_12_UDF */
#define    EVT_SRC_TMRA_12_CMP       363U    /* TMRA_12_CMP */
/*  USART5 USART6  */
#define    EVT_SRC_USART5_BRKWKPI    364U    /* USART_5_BRKWKPI */
#define    EVT_SRC_USART5_EI         365U    /* USART_5_EI */
#define    EVT_SRC_USART5_RI         366U    /* USART_5_RI */
#define    EVT_SRC_USART5_TI         367U    /* USART_5_TI */
#define    EVT_SRC_USART5_TCI        368U    /* USART_5_TCI */
#define    EVT_SRC_USART6_EI         369U    /* USART_6_EI */
#define    EVT_SRC_USART6_RI         370U    /* USART_6_RI */
#define    EVT_SRC_USART6_TI         371U    /* USART_6_TI */
#define    EVT_SRC_USART6_TCI        372U    /* USART_6_TCI */
#define    EVT_SRC_USART6_RTO        373U    /* USART_6_RTO */
/*  SPI5 SPI6  */
#define    EVT_SRC_SPI5_SPRI         374U    /* SPI_5_SPRI */
#define    EVT_SRC_SPI5_SPTI         375U    /* SPI_5_SPTI */
#define    EVT_SRC_SPI5_SPII         376U    /* SPI_5_SPII */
#define    EVT_SRC_SPI5_SPEI         377U    /* SPI_5_SPEI */
#define    EVT_SRC_SPI5_SPEND        378U    /* SPI_5_SPEND */
#define    EVT_SRC_SPI6_SPRI         379U    /* SPI_6_SPRI */
#define    EVT_SRC_SPI6_SPTI         380U    /* SPI_6_SPTI */
#define    EVT_SRC_SPI6_SPII         381U    /* SPI_6_SPII */
#define    EVT_SRC_SPI6_SPEI         382U    /* SPI_6_SPEI */
#define    EVT_SRC_SPI6_SPEND        383U    /* SPI_6_SPEND */
/*  I2S1 I2S2  */
#define    EVT_SRC_I2S1_TXIRQOUT     384U    /* I2S_1_TXIRQOUT */
#define    EVT_SRC_I2S1_RXIRQOUT     385U    /* I2S_1_RXIRQOUT */
#define    EVT_SRC_I2S2_TXIRQOUT     387U    /* I2S_2_TXIRQOUT */
#define    EVT_SRC_I2S2_RXIRQOUT     388U    /* I2S_2_RXIRQOUT */
/*  USART7 USART8  */
#define    EVT_SRC_USART7_EI         390U    /* USART_7_EI */
#define    EVT_SRC_USART7_RI         391U    /* USART_7_RI */
#define    EVT_SRC_USART7_TI         392U    /* USART_7_TI */
#define    EVT_SRC_USART7_TCI        393U    /* USART_7_TCI */
#define    EVT_SRC_USART7_RTO        394U    /* USART_7_RTO */
#define    EVT_SRC_USART8_EI         395U    /* USART_8_EI */
#define    EVT_SRC_USART8_RI         396U    /* USART_8_RI */
#define    EVT_SRC_USART8_TI         397U    /* USART_8_TI */
#define    EVT_SRC_USART8_TCI        398U    /* USART_8_TCI */
/*  HASH  */
#define    EVT_SRC_HASH              401U    /* HASH_INT */
/*  SDIOC  */
#define    EVT_SRC_SDIOC1_DMAR       402U    /* SDIOC_1_DMAR */
#define    EVT_SRC_SDIOC1_DMAW       403U    /* SDIOC_1_DMAW */
#define    EVT_SRC_SDIOC2_DMAR       405U    /* SDIOC_2_DMAR */
#define    EVT_SRC_SDIOC2_DMAW       406U    /* SDIOC_2_DMAW */
/*  EVENT PORT  */
#define    EVT_SRC_EVENT_PORT1       408U    /* EVENT_PORT1 */
#define    EVT_SRC_EVENT_PORT2       409U    /* EVENT_PORT2 */
#define    EVT_SRC_EVENT_PORT3       410U    /* EVENT_PORT3 */
#define    EVT_SRC_EVENT_PORT4       411U    /* EVENT_PORT4 */
/*  ETHER  */
#define    EVT_SRC_ETH_PPS_OUT_0     414U    /* ETH_PPS_OUT_0 */
#define    EVT_SRC_ETH_PPS_OUT_1     415U    /* ETH_PPS_OUT_1 */
/*  I2S3 I2S4  */
#define    EVT_SRC_I2S3_TXIRQOUT     416U    /* I2S_3_TXIRQOUT */
#define    EVT_SRC_I2S3_RXIRQOUT     417U    /* I2S_3_RXIRQOUT */
#define    EVT_SRC_I2S4_TXIRQOUT     419U    /* I2S_4_TXIRQOUT */
#define    EVT_SRC_I2S4_RXIRQOUT     420U    /* I2S_4_RXIRQOUT */
/*  USART9 USART10  */
#define    EVT_SRC_USART9_EI         422U    /* USART_9_EI */
#define    EVT_SRC_USART9_RI         423U    /* USART_9_RI */
#define    EVT_SRC_USART9_TI         424U    /* USART_9_TI */
#define    EVT_SRC_USART9_TCI        425U    /* USART_9_TCI */
#define    EVT_SRC_USART10_BRKWKPI   426U    /* USART_10_BRKWKPI */
#define    EVT_SRC_USART10_EI        427U    /* USART_10_EI */
#define    EVT_SRC_USART10_RI        428U    /* USART_10_RI */
#define    EVT_SRC_USART10_TI        429U    /* USART_10_TI */
#define    EVT_SRC_USART10_TCI       430U    /* USART_10_TCI */
/*  I2C1 I2C2 I2C3  */
#define    EVT_SRC_I2C1_RXI          432U    /* I2C_1_RXI */
#define    EVT_SRC_I2C1_TXI          433U    /* I2C_1_TXI */
#define    EVT_SRC_I2C1_TEI          434U    /* I2C_1_TEI */
#define    EVT_SRC_I2C1_EEI          435U    /* I2C_1_EEI */
#define    EVT_SRC_I2C2_RXI          436U    /* I2C_2_RXI */
#define    EVT_SRC_I2C2_TXI          437U    /* I2C_2_TXI */
#define    EVT_SRC_I2C2_TEI          438U    /* I2C_2_TEI */
#define    EVT_SRC_I2C2_EEI          439U    /* I2C_2_EEI */
#define    EVT_SRC_I2C3_RXI          440U    /* I2C_3_RXI */
#define    EVT_SRC_I2C3_TXI          441U    /* I2C_3_TXI */
#define    EVT_SRC_I2C3_TEI          442U    /* I2C_3_TEI */
#define    EVT_SRC_I2C3_EEI          443U    /* I2C_3_EEI */
/*  ACMP  */
#define    EVT_SRC_CMP1              444U    /* CMP1 */
#define    EVT_SRC_CMP2              445U    /* CMP2 */
#define    EVT_SRC_CMP3              446U    /* CMP3 */
#define    EVT_SRC_CMP4              447U    /* CMP4 */
/*  I2C4 I2C5 I2C6  */
#define    EVT_SRC_I2C4_RXI          448U    /* I2C_4_RXI */
#define    EVT_SRC_I2C4_TXI          449U    /* I2C_4_TXI */
#define    EVT_SRC_I2C4_TEI          450U    /* I2C_4_TEI */
#define    EVT_SRC_I2C4_EEI          451U    /* I2C_4_EEI */
#define    EVT_SRC_I2C5_RXI          452U    /* I2C_5_RXI */
#define    EVT_SRC_I2C5_TXI          453U    /* I2C_5_TXI */
#define    EVT_SRC_I2C5_TEI          454U    /* I2C_5_TEI */
#define    EVT_SRC_I2C5_EEI          455U    /* I2C_5_EEI */
#define    EVT_SRC_I2C6_RXI          456U    /* I2C_6_RXI */
#define    EVT_SRC_I2C6_TXI          457U    /* I2C_6_TXI */
#define    EVT_SRC_I2C6_TEI          458U    /* I2C_6_TEI */
#define    EVT_SRC_I2C6_EEI          459U    /* I2C_6_EEI */
/*  LVD  */
#define    EVT_SRC_LVD1             461U    /* LVD1 */
#define    EVT_SRC_LVD2             462U    /* LVD2 */
/*  OTS  */
#define    EVT_SRC_OTS               463U    /* OTS */
/*  WDT  */
#define    EVT_SRC_WDT_REFUDF        467U    /* WDT_REFUDF */
/*  ADC  */
#define    EVT_SRC_ADC1_EOCA         480U    /* ADC_1_EOCA */
#define    EVT_SRC_ADC1_EOCB         481U    /* ADC_1_EOCB */
#define    EVT_SRC_ADC1_CMP0         482U    /* ADC_1_CMP0 */
#define    EVT_SRC_ADC1_CMP1         483U    /* ADC_1_CMP1 */
#define    EVT_SRC_ADC2_EOCA         484U    /* ADC_2_EOCA */
#define    EVT_SRC_ADC2_EOCB         485U    /* ADC_2_EOCB */
#define    EVT_SRC_ADC2_CMP0         486U    /* ADC_2_CMP0 */
#define    EVT_SRC_ADC2_CMP1         487U    /* ADC_2_CMP1 */
#define    EVT_SRC_ADC3_EOCA         488U    /* ADC_3_EOCA */
#define    EVT_SRC_ADC3_EOCB         489U    /* ADC_3_EOCB */
#define    EVT_SRC_ADC3_CMP0         490U    /* ADC_3_CMP0 */
#define    EVT_SRC_ADC3_CMP1         491U    /* ADC_3_CMP1 */
/*  TRNG  */
#define    EVT_SRC_TRNG_END          492U    /* TRNG_END */
#define    EVT_SRC_MAX               511U

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_HC32F4A0_INT_EVT_H_ */