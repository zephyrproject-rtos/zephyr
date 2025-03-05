/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_HC32F460_INT_EVT_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_HC32F460_INT_EVT_H_


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

/* DMAC */
#define INT_SRC_DMA1_TC0             32U      /* DMA1_TC0  */
#define INT_SRC_DMA1_TC1             33U      /* DMA1_TC1  */
#define INT_SRC_DMA1_TC2             34U      /* DMA1_TC2  */
#define INT_SRC_DMA1_TC3             35U      /* DMA1_TC3  */
#define INT_SRC_DMA2_TC0             36U      /* DMA2_TC0  */
#define INT_SRC_DMA2_TC1             37U      /* DMA2_TC1  */
#define INT_SRC_DMA2_TC2             38U      /* DMA2_TC2  */
#define INT_SRC_DMA2_TC3             39U      /* DMA2_TC3  */
#define INT_SRC_DMA1_BTC0            40U      /* DMA1_BTC0 */
#define INT_SRC_DMA1_BTC1            41U      /* DMA1_BTC1 */
#define INT_SRC_DMA1_BTC2            42U      /* DMA1_BTC2 */
#define INT_SRC_DMA1_BTC3            43U      /* DMA1_BTC3 */
#define INT_SRC_DMA2_BTC0            44U      /* DMA2_BTC0 */
#define INT_SRC_DMA2_BTC1            45U      /* DMA2_BTC1 */
#define INT_SRC_DMA2_BTC2            46U      /* DMA2_BTC2 */
#define INT_SRC_DMA2_BTC3            47U      /* DMA2_BTC3 */
#define INT_SRC_DMA1_ERR             48U      /* DMA1_ERR */
#define INT_SRC_DMA2_ERR             49U      /* DMA2_ERR */

/* EFM */
#define INT_SRC_EFM_PEERR            50U      /* EFM_PEERR */
#define INT_SRC_EFM_COLERR           51U      /* EFM_COLERR */
#define INT_SRC_EFM_OPTEND           52U      /* EFM_OPTEND */

/* QSPI */
#define INT_SRC_QSPI_INTR            54U      /* QSPI_INTR */

/* DCU */
#define INT_SRC_DCU1                 55U      /* DCU1 */
#define INT_SRC_DCU2                 56U      /* DCU2 */
#define INT_SRC_DCU3                 57U      /* DCU3 */
#define INT_SRC_DCU4                 58U      /* DCU4 */

/* TIMER 0 */
#define INT_SRC_TMR0_1_CMP_A         64U      /* TMR01_GCMA */
#define INT_SRC_TMR0_1_CMP_B         65U      /* TMR01_GCMB */
#define INT_SRC_TMR0_2_CMP_A         66U      /* TMR02_GCMA */
#define INT_SRC_TMR0_2_CMP_B         67U      /* TMR02_GCMB */

/* RTC */
#define INT_SRC_RTC_ALM              81U      /* RTC_ALM */
#define INT_SRC_RTC_PRD              82U      /* RTC_PRD */

/* XTAL32 stop */
#define INT_SRC_XTAL32_STOP          84U      /* XTAL32_STOP */

/* XTAL stop */
#define INT_SRC_XTAL_STOP            85U      /* XTAL_STOP */

/* wake-up timer */
#define INT_SRC_WKTM_PRD             86U      /* WKTM_PRD */

/* SWDT */
#define INT_SRC_SWDT_REFUDF          87U      /* SWDT_REFUDF */

/* TIMER 6 */
#define INT_SRC_TMR6_1_GCMP_A        96U      /* TMR61_GCMA */
#define INT_SRC_TMR6_1_GCMP_B        97U      /* TMR61_GCMB */
#define INT_SRC_TMR6_1_GCMP_C        98U      /* TMR61_GCMC */
#define INT_SRC_TMR6_1_GCMP_D        99U      /* TMR61_GCMD */
#define INT_SRC_TMR6_1_GCMP_E        100U     /* TMR61_GCME */
#define INT_SRC_TMR6_1_GCMP_F        101U     /* TMR61_GCMF */
#define INT_SRC_TMR6_1_OVF           102U     /* TMR61_GOVF */
#define INT_SRC_TMR6_1_UDF           103U     /* TMR61_GUDF */
#define INT_SRC_TMR6_1_DTE           104U     /* TMR61_GDTE */
#define INT_SRC_TMR6_1_SCMP_A        107U     /* TMR61_SCMA */
#define INT_SRC_TMR6_1_SCMP_B        108U     /* TMR61_SCMB */
#define INT_SRC_TMR6_2_GCMP_A        112U     /* TMR62_GCMA */
#define INT_SRC_TMR6_2_GCMP_B        113U     /* TMR62_GCMB */
#define INT_SRC_TMR6_2_GCMP_C        114U     /* TMR62_GCMC */
#define INT_SRC_TMR6_2_GCMP_D        115U     /* TMR62_GCMD */
#define INT_SRC_TMR6_2_GCMP_E        116U     /* TMR62_GCME */
#define INT_SRC_TMR6_2_GCMP_F        117U     /* TMR62_GCMF */
#define INT_SRC_TMR6_2_OVF           118U     /* TMR62_GOVF */
#define INT_SRC_TMR6_2_UDF           119U     /* TMR62_GUDF */
#define INT_SRC_TMR6_2_DTE           120U     /* TMR62_GDTE */
#define INT_SRC_TMR6_2_SCMP_A        123U     /* TMR62_SCMA */
#define INT_SRC_TMR6_2_SCMP_B        124U     /* TMR62_SCMB */
#define INT_SRC_TMR6_3_GCMP_A        128U     /* TMR63_GCMA */
#define INT_SRC_TMR6_3_GCMP_B        129U     /* TMR63_GCMB */
#define INT_SRC_TMR6_3_GCMP_C        130U     /* TMR63_GCMC */
#define INT_SRC_TMR6_3_GCMP_D        131U     /* TMR63_GCMD */
#define INT_SRC_TMR6_3_GCMP_E        132U     /* TMR63_GCME */
#define INT_SRC_TMR6_3_GCMP_F        133U     /* TMR63_GCMF */
#define INT_SRC_TMR6_3_OVF           134U     /* TMR63_GOVF */
#define INT_SRC_TMR6_3_UDF           135U     /* TMR63_GUDF */
#define INT_SRC_TMR6_3_DTE           136U     /* TMR63_GDTE */
#define INT_SRC_TMR6_3_SCMP_A        139U     /* TMR63_SCMA */
#define INT_SRC_TMR6_3_SCMP_B        140U     /* TMR63_SCMB */

/* TIMER A */
#define INT_SRC_TMRA_1_OVF           256U     /* TMRA1_OVF */
#define INT_SRC_TMRA_1_UDF           257U     /* TMRA1_UDF */
#define INT_SRC_TMRA_1_CMP           258U     /* TMRA1_CMP */
#define INT_SRC_TMRA_2_OVF           259U     /* TMRA2_OVF */
#define INT_SRC_TMRA_2_UDF           260U     /* TMRA2_UDF */
#define INT_SRC_TMRA_2_CMP           261U     /* TMRA2_CMP */
#define INT_SRC_TMRA_3_OVF           262U     /* TMRA3_OVF */
#define INT_SRC_TMRA_3_UDF           263U     /* TMRA3_UDF */
#define INT_SRC_TMRA_3_CMP           264U     /* TMRA3_CMP */
#define INT_SRC_TMRA_4_OVF           265U     /* TMRA4_OVF */
#define INT_SRC_TMRA_4_UDF           266U     /* TMRA4_UDF */
#define INT_SRC_TMRA_4_CMP           267U     /* TMRA4_CMP */
#define INT_SRC_TMRA_5_OVF           268U     /* TMRA5_OVF */
#define INT_SRC_TMRA_5_UDF           269U     /* TMRA5_UDF */
#define INT_SRC_TMRA_5_CMP           270U     /* TMRA5_CMP */
#define INT_SRC_TMRA_6_OVF           272U     /* TMRA6_OVF */
#define INT_SRC_TMRA_6_UDF           273U     /* TMRA6_UDF */
#define INT_SRC_TMRA_6_CMP           274U     /* TMRA6_CMP */

/* USB FS */
#define INT_SRC_USBFS_GLB            275U     /* USBFS_GLB */

/* USRAT */
#define INT_SRC_USART1_EI            278U     /* USART1_EI  */
#define INT_SRC_USART1_RI            279U     /* USART1_RI  */
#define INT_SRC_USART1_TI            280U     /* USART1_TI  */
#define INT_SRC_USART1_TCI           281U     /* USART1_TCI */
#define INT_SRC_USART1_RTO           282U     /* USART1_RTO */
#define INT_SRC_USART1_WUPI          432U     /* USART1_WUPI */
#define INT_SRC_USART2_EI            283U     /* USART2_EI  */
#define INT_SRC_USART2_RI            284U     /* USART2_RI  */
#define INT_SRC_USART2_TI            285U     /* USART2_TI  */
#define INT_SRC_USART2_TCI           286U     /* USART2_TCI */
#define INT_SRC_USART2_RTO           287U     /* USART2_RTO */
#define INT_SRC_USART3_EI            288U     /* USART3_EI  */
#define INT_SRC_USART3_RI            289U     /* USART3_RI  */
#define INT_SRC_USART3_TI            290U     /* USART3_TI  */
#define INT_SRC_USART3_TCI           291U     /* USART3_TCI */
#define INT_SRC_USART3_RTO           292U     /* USART3_RTO */
#define INT_SRC_USART4_EI            293U     /* USART4_EI  */
#define INT_SRC_USART4_RI            294U     /* USART4_RI  */
#define INT_SRC_USART4_TI            295U     /* USART4_TI  */
#define INT_SRC_USART4_TCI           296U     /* USART4_TCI */
#define INT_SRC_USART4_RTO           297U     /* USART4_RTO */

/* SPI */
#define INT_SRC_SPI1_SPRI            299U     /* SPI1_SPRI   */
#define INT_SRC_SPI1_SPTI            300U     /* SPI1_SPTI   */
#define INT_SRC_SPI1_SPII            301U     /* SPI1_SPII   */
#define INT_SRC_SPI1_SPEI            302U     /* SPI1_SPEI   */
#define INT_SRC_SPI2_SPRI            304U     /* SPI2_SPRI   */
#define INT_SRC_SPI2_SPTI            305U     /* SPI2_SPTI   */
#define INT_SRC_SPI2_SPII            306U     /* SPI2_SPII   */
#define INT_SRC_SPI2_SPEI            307U     /* SPI2_SPEI   */
#define INT_SRC_SPI3_SPRI            309U     /* SPI3_SPRI   */
#define INT_SRC_SPI3_SPTI            310U     /* SPI3_SPTI   */
#define INT_SRC_SPI3_SPII            311U     /* SPI3_SPII   */
#define INT_SRC_SPI3_SPEI            312U     /* SPI3_SPEI   */
#define INT_SRC_SPI4_SPRI            314U     /* SPI4_SPRI   */
#define INT_SRC_SPI4_SPTI            315U     /* SPI4_SPTI   */
#define INT_SRC_SPI4_SPII            316U     /* SPI4_SPII   */
#define INT_SRC_SPI4_SPEI            317U     /* SPI4_SPEI   */

/* TIMER 4 */
#define INT_SRC_TMR4_1_GCMP_UH       320U     /* TMR41_GCMUH */
#define INT_SRC_TMR4_1_GCMP_UL       321U     /* TMR41_GCMUL */
#define INT_SRC_TMR4_1_GCMP_VH       322U     /* TMR41_GCMVH */
#define INT_SRC_TMR4_1_GCMP_VL       323U     /* TMR41_GCMVL */
#define INT_SRC_TMR4_1_GCMP_WH       324U     /* TMR41_GCMWH */
#define INT_SRC_TMR4_1_GCMP_WL       325U     /* TMR41_GCMWL */
#define INT_SRC_TMR4_1_OVF           326U     /* TMR41_GOVF  */
#define INT_SRC_TMR4_1_UDF           327U     /* TMR41_GUDF  */
#define INT_SRC_TMR4_1_RELOAD_U      328U     /* TMR41_RLOU  */
#define INT_SRC_TMR4_1_RELOAD_V      329U     /* TMR41_RLOV  */
#define INT_SRC_TMR4_1_RELOAD_W      330U     /* TMR41_RLOW  */
#define INT_SRC_TMR4_2_GCMP_UH       336U     /* TMR42_GCMUH */
#define INT_SRC_TMR4_2_GCMP_UL       337U     /* TMR42_GCMUL */
#define INT_SRC_TMR4_2_GCMP_VH       338U     /* TMR42_GCMVH */
#define INT_SRC_TMR4_2_GCMP_VL       339U     /* TMR42_GCMVL */
#define INT_SRC_TMR4_2_GCMP_WH       340U     /* TMR42_GCMWH */
#define INT_SRC_TMR4_2_GCMP_WL       341U     /* TMR42_GCMWL */
#define INT_SRC_TMR4_2_OVF           342U     /* TMR42_GOVF  */
#define INT_SRC_TMR4_2_UDF           343U     /* TMR42_GUDF  */
#define INT_SRC_TMR4_2_RELOAD_U      344U     /* TMR42_RLOU  */
#define INT_SRC_TMR4_2_RELOAD_V      345U     /* TMR42_RLOV  */
#define INT_SRC_TMR4_2_RELOAD_W      346U     /* TMR42_RLOW  */
#define INT_SRC_TMR4_3_GCMP_UH       352U     /* TMR43_GCMUH */
#define INT_SRC_TMR4_3_GCMP_UL       353U     /* TMR43_GCMUL */
#define INT_SRC_TMR4_3_GCMP_VH       354U     /* TMR43_GCMVH */
#define INT_SRC_TMR4_3_GCMP_VL       355U     /* TMR43_GCMVL */
#define INT_SRC_TMR4_3_GCMP_WH       356U     /* TMR43_GCMWH */
#define INT_SRC_TMR4_3_GCMP_WL       357U     /* TMR43_GCMWL */
#define INT_SRC_TMR4_3_OVF           358U     /* TMR43_GOVF  */
#define INT_SRC_TMR4_3_UDF           359U     /* TMR43_GUDF  */
#define INT_SRC_TMR4_3_RELOAD_U      360U     /* TMR43_RLOU  */
#define INT_SRC_TMR4_3_RELOAD_V      361U     /* TMR43_RLOV  */
#define INT_SRC_TMR4_3_RELOAD_W      362U     /* TMR43_RLOW  */

/* EMB */
#define INT_SRC_EMB_GR0              390U     /* EMB_GR0  */
#define INT_SRC_EMB_GR1              391U     /* EMB_GR1  */
#define INT_SRC_EMB_GR2              392U     /* EMB_GR2  */
#define INT_SRC_EMB_GR3              393U     /* EMB_GR3  */

/* EVENT PORT */
#define INT_SRC_EVENT_PORT1          394U     /* EVENT_PORT1  */
#define INT_SRC_EVENT_PORT2          395U     /* EVENT_PORT2  */
#define INT_SRC_EVENT_PORT3          396U     /* EVENT_PORT3  */
#define INT_SRC_EVENT_PORT4          397U     /* EVENT_PORT4  */

/* I2S */
#define INT_SRC_I2S1_TXIRQOUT        400U     /* I2S1_TXIRQOUT */
#define INT_SRC_I2S1_RXIRQOUT        401U     /* I2S1_RXIRQOUT */
#define INT_SRC_I2S1_ERRIRQOUT       402U     /* I2S1_ERRIRQOUT */
#define INT_SRC_I2S2_TXIRQOUT        403U     /* I2S2_TXIRQOUT */
#define INT_SRC_I2S2_RXIRQOUT        404U     /* I2S2_RXIRQOUT */
#define INT_SRC_I2S2_ERRIRQOUT       405U     /* I2S2_ERRIRQOUT */
#define INT_SRC_I2S3_TXIRQOUT        406U     /* I2S3_TXIRQOUT */
#define INT_SRC_I2S3_RXIRQOUT        407U     /* I2S3_RXIRQOUT */
#define INT_SRC_I2S3_ERRIRQOUT       408U     /* I2S3_ERRIRQOUT */
#define INT_SRC_I2S4_TXIRQOUT        409U     /* I2S4_TXIRQOUT */
#define INT_SRC_I2S4_RXIRQOUT        410U     /* I2S4_RXIRQOUT */
#define INT_SRC_I2S4_ERRIRQOUT       411U     /* I2S4_ERRIRQOUT */

/* COMPARATOR */
#define INT_SRC_CMP1                 416U     /* ACMP1 */
#define INT_SRC_CMP2                 417U     /* ACMP2 */
#define INT_SRC_CMP3                 418U     /* ACMP3 */

/* I2C */
#define INT_SRC_I2C1_RXI             420U     /* I2C1_RXI */
#define INT_SRC_I2C1_TXI             421U     /* I2C1_TXI */
#define INT_SRC_I2C1_TEI             422U     /* I2C1_TEI */
#define INT_SRC_I2C1_EEI             423U     /* I2C1_EEI */
#define INT_SRC_I2C2_RXI             424U     /* I2C2_RXI */
#define INT_SRC_I2C2_TXI             425U     /* I2C2_TXI */
#define INT_SRC_I2C2_TEI             426U     /* I2C2_TEI */
#define INT_SRC_I2C2_EEI             427U     /* I2C2_EEI */
#define INT_SRC_I2C3_RXI             428U     /* I2C3_RXI */
#define INT_SRC_I2C3_TXI             429U     /* I2C3_TXI */
#define INT_SRC_I2C3_TEI             430U     /* I2C3_TEI */
#define INT_SRC_I2C3_EEI             431U     /* I2C3_EEI */

/* LVD */
#define INT_SRC_LVD1                 433U     /* LVD1 */
#define INT_SRC_LVD2                 434U     /* LVD2 */

/* Temp. sensor */
#define INT_SRC_OTS                  435U     /* OTS */

/* FCM */
#define INT_SRC_FCMFERRI             436U     /* FCMFERRI */
#define INT_SRC_FCMMENDI             437U     /* FCMMENDI */
#define INT_SRC_FCMCOVFI             438U     /* FCMCOVFI */

/* WDT */
#define INT_SRC_WDT_REFUDF           439U     /* WDT_REFUDF */

/* ADC */
#define INT_SRC_ADC1_EOCA            448U     /* ADC1_EOCA   */
#define INT_SRC_ADC1_EOCB            449U     /* ADC1_EOCB   */
#define INT_SRC_ADC1_CHCMP           450U     /* ADC1_CHCMP  */
#define INT_SRC_ADC1_SEQCMP          451U     /* ADC1_SEQCMP */
#define INT_SRC_ADC2_EOCA            452U     /* ADC2_EOCA   */
#define INT_SRC_ADC2_EOCB            453U     /* ADC2_EOCB   */
#define INT_SRC_ADC2_CHCMP           454U     /* ADC2_CHCMP  */
#define INT_SRC_ADC2_SEQCMP          455U     /* ADC2_SEQCMP */

/* TRNG */
#define INT_SRC_TRNG_END             456U     /* TRNG_END */

/* SDIOC */
#define INT_SRC_SDIOC1_SD            482U     /* SDIOC1_SD */
#define INT_SRC_SDIOC2_SD            485U     /* SDIOC2_SD */

/* CAN */
#define INT_SRC_CAN_INT              486U     /* CAN_INT */

#define INT_SRC_MAX                  511U


/**
 * @brief Event Source Number
 */
#define    EVT_SRC_SWI_IRQ0             0       /* SWI_IRQ0  */
#define    EVT_SRC_SWI_IRQ1             1       /* SWI_IRQ1  */
#define    EVT_SRC_SWI_IRQ2             2       /* SWI_IRQ2  */
#define    EVT_SRC_SWI_IRQ3             3       /* SWI_IRQ3  */
#define    EVT_SRC_SWI_IRQ4             4       /* SWI_IRQ4  */
#define    EVT_SRC_SWI_IRQ5             5       /* SWI_IRQ5  */
#define    EVT_SRC_SWI_IRQ6             6       /* SWI_IRQ6  */
#define    EVT_SRC_SWI_IRQ7             7       /* SWI_IRQ7  */
#define    EVT_SRC_SWI_IRQ8             8       /* SWI_IRQ8  */
#define    EVT_SRC_SWI_IRQ9             9       /* SWI_IRQ9  */
#define    EVT_SRC_SWI_IRQ10            10      /* SWI_IRQ10 */
#define    EVT_SRC_SWI_IRQ11            11      /* SWI_IRQ11 */
#define    EVT_SRC_SWI_IRQ12            12      /* SWI_IRQ12 */
#define    EVT_SRC_SWI_IRQ13            13      /* SWI_IRQ13 */
#define    EVT_SRC_SWI_IRQ14            14      /* SWI_IRQ14 */
#define    EVT_SRC_SWI_IRQ15            15      /* SWI_IRQ15 */
#define    EVT_SRC_SWI_IRQ16            16      /* SWI_IRQ16 */
#define    EVT_SRC_SWI_IRQ17            17      /* SWI_IRQ17 */
#define    EVT_SRC_SWI_IRQ18            18      /* SWI_IRQ18 */
#define    EVT_SRC_SWI_IRQ19            19      /* SWI_IRQ19 */
#define    EVT_SRC_SWI_IRQ20            20      /* SWI_IRQ20 */
#define    EVT_SRC_SWI_IRQ21            21      /* SWI_IRQ21 */
#define    EVT_SRC_SWI_IRQ22            22      /* SWI_IRQ22 */
#define    EVT_SRC_SWI_IRQ23            23      /* SWI_IRQ23 */
#define    EVT_SRC_SWI_IRQ24            24      /* SWI_IRQ24 */
#define    EVT_SRC_SWI_IRQ25            25      /* SWI_IRQ25 */
#define    EVT_SRC_SWI_IRQ26            26      /* SWI_IRQ26 */
#define    EVT_SRC_SWI_IRQ27            27      /* SWI_IRQ27 */
#define    EVT_SRC_SWI_IRQ28            28      /* SWI_IRQ28 */
#define    EVT_SRC_SWI_IRQ29            29      /* SWI_IRQ29 */
#define    EVT_SRC_SWI_IRQ30            30      /* SWI_IRQ30 */
#define    EVT_SRC_SWI_IRQ31            31      /* SWI_IRQ31 */

/* External Interrupt. */
#define    EVT_SRC_PORT_EIRQ0           0       /* PORT_EIRQ0  */
#define    EVT_SRC_PORT_EIRQ1           1       /* PORT_EIRQ1  */
#define    EVT_SRC_PORT_EIRQ2           2       /* PORT_EIRQ2  */
#define    EVT_SRC_PORT_EIRQ3           3       /* PORT_EIRQ3  */
#define    EVT_SRC_PORT_EIRQ4           4       /* PORT_EIRQ4  */
#define    EVT_SRC_PORT_EIRQ5           5       /* PORT_EIRQ5  */
#define    EVT_SRC_PORT_EIRQ6           6       /* PORT_EIRQ6  */
#define    EVT_SRC_PORT_EIRQ7           7       /* PORT_EIRQ7  */
#define    EVT_SRC_PORT_EIRQ8           8       /* PORT_EIRQ8  */
#define    EVT_SRC_PORT_EIRQ9           9       /* PORT_EIRQ9  */
#define    EVT_SRC_PORT_EIRQ10          10      /* PORT_EIRQ10 */
#define    EVT_SRC_PORT_EIRQ11          11      /* PORT_EIRQ11 */
#define    EVT_SRC_PORT_EIRQ12          12      /* PORT_EIRQ12 */
#define    EVT_SRC_PORT_EIRQ13          13      /* PORT_EIRQ13 */
#define    EVT_SRC_PORT_EIRQ14          14      /* PORT_EIRQ14 */
#define    EVT_SRC_PORT_EIRQ15          15      /* PORT_EIRQ15 */

/* DMAC */
#define    EVT_SRC_DMA1_TC0             32      /* DMA1_TC0  */
#define    EVT_SRC_DMA1_TC1             33      /* DMA1_TC1  */
#define    EVT_SRC_DMA1_TC2             34      /* DMA1_TC2  */
#define    EVT_SRC_DMA1_TC3             35      /* DMA1_TC3  */
#define    EVT_SRC_DMA2_TC0             36      /* DMA2_TC0  */
#define    EVT_SRC_DMA2_TC1             37      /* DMA2_TC1  */
#define    EVT_SRC_DMA2_TC2             38      /* DMA2_TC2  */
#define    EVT_SRC_DMA2_TC3             39      /* DMA2_TC3  */
#define    EVT_SRC_DMA1_BTC0            40      /* DMA1_BTC0 */
#define    EVT_SRC_DMA1_BTC1            41      /* DMA1_BTC1 */
#define    EVT_SRC_DMA1_BTC2            42      /* DMA1_BTC2 */
#define    EVT_SRC_DMA1_BTC3            43      /* DMA1_BTC3 */
#define    EVT_SRC_DMA2_BTC0            44      /* DMA2_BTC0 */
#define    EVT_SRC_DMA2_BTC1            45      /* DMA2_BTC1 */
#define    EVT_SRC_DMA2_BTC2            46      /* DMA2_BTC2 */
#define    EVT_SRC_DMA2_BTC3            47      /* DMA2_BTC3 */

/* EFM */
#define    EVT_SRC_EFM_OPTEND           52      /* EFM_OPTEND */

/* USB SOF */
#define    EVT_SRC_USBFS_SOF            53      /* USBFS_SOF */

/* DCU */
#define    EVT_SRC_DCU1                 55      /* DCU1 */
#define    EVT_SRC_DCU2                 56      /* DCU2 */
#define    EVT_SRC_DCU3                 57      /* DCU3 */
#define    EVT_SRC_DCU4                 58      /* DCU4 */

/* TIMER 0 */
#define    EVT_SRC_TMR0_1_CMP_A         64      /* TMR01_GCMA */
#define    EVT_SRC_TMR0_1_CMP_B         65      /* TMR01_GCMB */
#define    EVT_SRC_TMR0_2_CMP_A         66      /* TMR02_GCMA */
#define    EVT_SRC_TMR0_2_CMP_B         67      /* TMR02_GCMB */

/* RTC */
#define    EVT_SRC_RTC_ALM              81      /* RTC_ALM */
#define    EVT_SRC_RTC_PRD              82      /* RTC_PRD */

/* TIMER 6 */
#define    EVT_SRC_TMR6_1_GCMP_A        96      /* TMR61_GCMA */
#define    EVT_SRC_TMR6_1_GCMP_B        97      /* TMR61_GCMB */
#define    EVT_SRC_TMR6_1_GCMP_C        98      /* TMR61_GCMC */
#define    EVT_SRC_TMR6_1_GCMP_D        99      /* TMR61_GCMD */
#define    EVT_SRC_TMR6_1_GCMP_E        100     /* TMR61_GCME */
#define    EVT_SRC_TMR6_1_GCMP_F        101     /* TMR61_GCMF */
#define    EVT_SRC_TMR6_1_OVF           102     /* TMR61_GOVF */
#define    EVT_SRC_TMR6_1_UDF           103     /* TMR61_GUDF */
#define    EVT_SRC_TMR6_1_SCMP_A        107     /* TMR61_SCMA */
#define    EVT_SRC_TMR6_1_SCMP_B        108     /* TMR61_SCMB */
#define    EVT_SRC_TMR6_2_GCMP_A        112     /* TMR62_GCMA */
#define    EVT_SRC_TMR6_2_GCMP_B        113     /* TMR62_GCMB */
#define    EVT_SRC_TMR6_2_GCMP_C        114     /* TMR62_GCMC */
#define    EVT_SRC_TMR6_2_GCMP_D        115     /* TMR62_GCMD */
#define    EVT_SRC_TMR6_2_GCMP_E        116     /* TMR62_GCME */
#define    EVT_SRC_TMR6_2_GCMP_F        117     /* TMR62_GCMF */
#define    EVT_SRC_TMR6_2_OVF           118     /* TMR62_GOVF */
#define    EVT_SRC_TMR6_2_UDF           119     /* TMR62_GUDF */
#define    EVT_SRC_TMR6_2_SCMP_A        123     /* TMR62_SCMA */
#define    EVT_SRC_TMR6_2_SCMP_B        124     /* TMR62_SCMB */
#define    EVT_SRC_TMR6_3_GCMP_A        128     /* TMR63_GCMA */
#define    EVT_SRC_TMR6_3_GCMP_B        129     /* TMR63_GCMB */
#define    EVT_SRC_TMR6_3_GCMP_C        130     /* TMR63_GCMC */
#define    EVT_SRC_TMR6_3_GCMP_D        131     /* TMR63_GCMD */
#define    EVT_SRC_TMR6_3_GCMP_E        132     /* TMR63_GCME */
#define    EVT_SRC_TMR6_3_GCMP_F        133     /* TMR63_GCMF */
#define    EVT_SRC_TMR6_3_OVF           134     /* TMR63_GOVF */
#define    EVT_SRC_TMR6_3_UDF           135     /* TMR63_GUDF */
#define    EVT_SRC_TMR6_3_SCMP_A        139     /* TMR63_SCMA */
#define    EVT_SRC_TMR6_3_SCMP_B        140     /* TMR63_SCMB */

/* TIMER A */
#define    EVT_SRC_TMRA_1_OVF           256     /* TMRA1_OVF */
#define    EVT_SRC_TMRA_1_UDF           257     /* TMRA1_UDF */
#define    EVT_SRC_TMRA_1_CMP           258     /* TMRA1_CMP */
#define    EVT_SRC_TMRA_2_OVF           259     /* TMRA2_OVF */
#define    EVT_SRC_TMRA_2_UDF           260     /* TMRA2_UDF */
#define    EVT_SRC_TMRA_2_CMP           261     /* TMRA2_CMP */
#define    EVT_SRC_TMRA_3_OVF           262     /* TMRA3_OVF */
#define    EVT_SRC_TMRA_3_UDF           263     /* TMRA3_UDF */
#define    EVT_SRC_TMRA_3_CMP           264     /* TMRA3_CMP */
#define    EVT_SRC_TMRA_4_OVF           265     /* TMRA4_OVF */
#define    EVT_SRC_TMRA_4_UDF           266     /* TMRA4_UDF */
#define    EVT_SRC_TMRA_4_CMP           267     /* TMRA4_CMP */
#define    EVT_SRC_TMRA_5_OVF           268     /* TMRA5_OVF */
#define    EVT_SRC_TMRA_5_UDF           269     /* TMRA5_UDF */
#define    EVT_SRC_TMRA_5_CMP           270     /* TMRA5_CMP */
#define    EVT_SRC_TMRA_6_OVF           272     /* TMRA6_OVF */
#define    EVT_SRC_TMRA_6_UDF           273     /* TMRA6_UDF */
#define    EVT_SRC_TMRA_6_CMP           274     /* TMRA6_CMP */

/* USART */
#define    EVT_SRC_USART1_EI            278     /* USART1_EI  */
#define    EVT_SRC_USART1_RI            279     /* USART1_RI  */
#define    EVT_SRC_USART1_TI            280     /* USART1_TI  */
#define    EVT_SRC_USART1_TCI           281     /* USART1_TCI */
#define    EVT_SRC_USART1_RTO           282     /* USART1_RTO */
#define    EVT_SRC_USART2_EI            283     /* USART2_EI  */
#define    EVT_SRC_USART2_RI            284     /* USART2_RI  */
#define    EVT_SRC_USART2_TI            285     /* USART2_TI  */
#define    EVT_SRC_USART2_TCI           286     /* USART2_TCI */
#define    EVT_SRC_USART2_RTO           287     /* USART2_RTO */
#define    EVT_SRC_USART3_EI            288     /* USART3_EI  */
#define    EVT_SRC_USART3_RI            289     /* USART3_RI  */
#define    EVT_SRC_USART3_TI            290     /* USART3_TI  */
#define    EVT_SRC_USART3_TCI           291     /* USART3_TCI */
#define    EVT_SRC_USART3_RTO           292     /* USART3_RTO */
#define    EVT_SRC_USART4_EI            293     /* USART4_EI  */
#define    EVT_SRC_USART4_RI            294     /* USART4_RI  */
#define    EVT_SRC_USART4_TI            295     /* USART4_TI  */
#define    EVT_SRC_USART4_TCI           296     /* USART4_TCI */
#define    EVT_SRC_USART4_RTO           297     /* USART4_RTO */

/* SPI */
#define    EVT_SRC_SPI1_SPRI            299     /* SPI1_SPRI   */
#define    EVT_SRC_SPI1_SPTI            300     /* SPI1_SPTI   */
#define    EVT_SRC_SPI1_SPII            301     /* SPI1_SPII   */
#define    EVT_SRC_SPI1_SPEI            302     /* SPI1_SPEI   */
#define    EVT_SRC_SPI1_SPTEND          303     /* SPI1_SPTEND */
#define    EVT_SRC_SPI2_SPRI            304     /* SPI2_SPRI   */
#define    EVT_SRC_SPI2_SPTI            305     /* SPI2_SPTI   */
#define    EVT_SRC_SPI2_SPII            306     /* SPI2_SPII   */
#define    EVT_SRC_SPI2_SPEI            307     /* SPI2_SPEI   */
#define    EVT_SRC_SPI2_SPTEND          308     /* SPI2_SPTEND */
#define    EVT_SRC_SPI3_SPRI            309     /* SPI3_SPRI   */
#define    EVT_SRC_SPI3_SPTI            310     /* SPI3_SPTI   */
#define    EVT_SRC_SPI3_SPII            311     /* SPI3_SPII   */
#define    EVT_SRC_SPI3_SPEI            312     /* SPI3_SPEI   */
#define    EVT_SRC_SPI3_SPTEND          313     /* SPI3_SPTEND */
#define    EVT_SRC_SPI4_SPRI            314     /* SPI4_SPRI   */
#define    EVT_SRC_SPI4_SPTI            315     /* SPI4_SPTI   */
#define    EVT_SRC_SPI4_SPII            316     /* SPI4_SPII   */
#define    EVT_SRC_SPI4_SPEI            317     /* SPI4_SPEI   */
#define    EVT_SRC_SPI4_SPTEND          318     /* SPI4_SPTEND */

/* AOS */
#define    EVT_SRC_AOS_STRG             319     /* AOS_STRG */

/* TIMER 4 */
#define    EVT_SRC_TMR4_1_SCMP0         368     /* TMR41_SCM0 */
#define    EVT_SRC_TMR4_1_SCMP1         369     /* TMR41_SCM1 */
#define    EVT_SRC_TMR4_1_SCMP2         370     /* TMR41_SCM2 */
#define    EVT_SRC_TMR4_1_SCMP3         371     /* TMR41_SCM3 */
#define    EVT_SRC_TMR4_1_SCMP4         372     /* TMR41_SCM4 */
#define    EVT_SRC_TMR4_1_SCMP5         373     /* TMR41_SCM5 */
#define    EVT_SRC_TMR4_2_SCMP0         374     /* TMR42_SCM0 */
#define    EVT_SRC_TMR4_2_SCMP1         375     /* TMR42_SCM1 */
#define    EVT_SRC_TMR4_2_SCMP2         376     /* TMR42_SCM2 */
#define    EVT_SRC_TMR4_2_SCMP3         377     /* TMR42_SCM3 */
#define    EVT_SRC_TMR4_2_SCMP4         378     /* TMR42_SCM4 */
#define    EVT_SRC_TMR4_2_SCMP5         379     /* TMR42_SCM5 */
#define    EVT_SRC_TMR4_3_SCMP0         384     /* TMR43_SCM0 */
#define    EVT_SRC_TMR4_3_SCMP1         385     /* TMR43_SCM1 */
#define    EVT_SRC_TMR4_3_SCMP2         386     /* TMR43_SCM2 */
#define    EVT_SRC_TMR4_3_SCMP3         387     /* TMR43_SCM3 */
#define    EVT_SRC_TMR4_3_SCMP4         388     /* TMR43_SCM4 */
#define    EVT_SRC_TMR4_3_SCMP5         389     /* TMR43_SCM5 */

/* EVENT PORT */
#define    EVT_SRC_EVENT_PORT1          394     /* EVENT_PORT1 */
#define    EVT_SRC_EVENT_PORT2          395     /* EVENT_PORT2 */
#define    EVT_SRC_EVENT_PORT3          396     /* EVENT_PORT3 */
#define    EVT_SRC_EVENT_PORT4          397     /* EVENT_PORT4 */

/* I2S */
#define    EVT_SRC_I2S1_TXIRQOUT        400     /* I2S1_TXIRQOUT */
#define    EVT_SRC_I2S1_RXIRQOUT        401     /* I2S1_RXIRQOUT */
#define    EVT_SRC_I2S2_TXIRQOUT        403     /* I2S2_TXIRQOUT */
#define    EVT_SRC_I2S2_RXIRQOUT        404     /* I2S2_RXIRQOUT */
#define    EVT_SRC_I2S3_TXIRQOUT        406     /* I2S3_TXIRQOUT */
#define    EVT_SRC_I2S3_RXIRQOUT        407     /* I2S3_RXIRQOUT */
#define    EVT_SRC_I2S4_TXIRQOUT        409     /* I2S4_TXIRQOUT */
#define    EVT_SRC_I2S4_RXIRQOUT        410     /* I2S4_RXIRQOUT */

/* COMPARATOR */
#define    EVT_SRC_CMP1                 416     /* ACMP1 */
#define    EVT_SRC_CMP2                 417     /* ACMP1 */
#define    EVT_SRC_CMP3                 418     /* ACMP1 */

/* I2C */
#define    EVT_SRC_I2C1_RXI             420     /* I2C1_RXI */
#define    EVT_SRC_I2C1_TXI             421     /* I2C1_TXI */
#define    EVT_SRC_I2C1_TEI             422     /* I2C1_TEI */
#define    EVT_SRC_I2C1_EEI             423     /* I2C1_EEI */
#define    EVT_SRC_I2C2_RXI             424     /* I2C2_RXI */
#define    EVT_SRC_I2C2_TXI             425     /* I2C2_TXI */
#define    EVT_SRC_I2C2_TEI             426     /* I2C2_TEI */
#define    EVT_SRC_I2C2_EEI             427     /* I2C2_EEI */
#define    EVT_SRC_I2C3_RXI             428     /* I2C3_RXI */
#define    EVT_SRC_I2C3_TXI             429     /* I2C3_TXI */
#define    EVT_SRC_I2C3_TEI             430     /* I2C3_TEI */
#define    EVT_SRC_I2C3_EEI             431     /* I2C3_EEI */

/* LVD */
#define    EVT_SRC_LVD1                 433     /* LVD1 */
#define    EVT_SRC_LVD2                 434     /* LVD2 */

/* OTS */
#define    EVT_SRC_OTS                  435     /* OTS */

/* WDT */
#define    EVT_SRC_WDT_REFUDF           439     /* WDT_REFUDF */

/* ADC */
#define    EVT_SRC_ADC1_EOCA            448     /* ADC1_EOCA   */
#define    EVT_SRC_ADC1_EOCB            449     /* ADC1_EOCB   */
#define    EVT_SRC_ADC1_CHCMP           450     /* ADC1_CHCMP  */
#define    EVT_SRC_ADC1_SEQCMP          451     /* ADC1_SEQCMP */
#define    EVT_SRC_ADC2_EOCA            452     /* ADC2_EOCA   */
#define    EVT_SRC_ADC2_EOCB            453     /* ADC2_EOCB   */
#define    EVT_SRC_ADC2_CHCMP           454     /* ADC2_CHCMP  */
#define    EVT_SRC_ADC2_SEQCMP          455     /* ADC2_SEQCMP */

/* TRNG */
#define    EVT_SRC_TRNG_END             456     /* TRNG_END */

/* SDIO */
#define    EVT_SRC_SDIOC1_DMAR          480     /* SDIOC1_DMAR */
#define    EVT_SRC_SDIOC1_DMAW          481     /* SDIOC1_DMAW */
#define    EVT_SRC_SDIOC2_DMAR          483     /* SDIOC2_DMAR */
#define    EVT_SRC_SDIOC2_DMAW          484     /* SDIOC2_DMAW */
#define    EVT_SRC_MAX                  511

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_HC32F460_INT_EVT_H_ */
