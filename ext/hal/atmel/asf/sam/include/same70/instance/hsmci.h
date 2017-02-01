/**
 * \file
 *
 * \brief Instance description for HSMCI
 *
 * Copyright (c) 2016 Atmel Corporation, a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
 */

#ifndef _SAME70_HSMCI_INSTANCE_H_
#define _SAME70_HSMCI_INSTANCE_H_

/* ========== Register definition for HSMCI peripheral ========== */
#if (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))

#define REG_HSMCI_CR            (0x40000000) /**< (HSMCI) Control Register */
#define REG_HSMCI_MR            (0x40000004) /**< (HSMCI) Mode Register */
#define REG_HSMCI_DTOR          (0x40000008) /**< (HSMCI) Data Timeout Register */
#define REG_HSMCI_SDCR          (0x4000000C) /**< (HSMCI) SD/SDIO Card Register */
#define REG_HSMCI_ARGR          (0x40000010) /**< (HSMCI) Argument Register */
#define REG_HSMCI_CMDR          (0x40000014) /**< (HSMCI) Command Register */
#define REG_HSMCI_BLKR          (0x40000018) /**< (HSMCI) Block Register */
#define REG_HSMCI_CSTOR         (0x4000001C) /**< (HSMCI) Completion Signal Timeout Register */
#define REG_HSMCI_RSPR          (0x40000020) /**< (HSMCI) Response Register 0 */
#define REG_HSMCI_RSPR0         (0x40000020) /**< (HSMCI) Response Register 0 */
#define REG_HSMCI_RSPR1         (0x40000024) /**< (HSMCI) Response Register 1 */
#define REG_HSMCI_RSPR2         (0x40000028) /**< (HSMCI) Response Register 2 */
#define REG_HSMCI_RSPR3         (0x4000002C) /**< (HSMCI) Response Register 3 */
#define REG_HSMCI_RDR           (0x40000030) /**< (HSMCI) Receive Data Register */
#define REG_HSMCI_TDR           (0x40000034) /**< (HSMCI) Transmit Data Register */
#define REG_HSMCI_SR            (0x40000040) /**< (HSMCI) Status Register */
#define REG_HSMCI_IER           (0x40000044) /**< (HSMCI) Interrupt Enable Register */
#define REG_HSMCI_IDR           (0x40000048) /**< (HSMCI) Interrupt Disable Register */
#define REG_HSMCI_IMR           (0x4000004C) /**< (HSMCI) Interrupt Mask Register */
#define REG_HSMCI_DMA           (0x40000050) /**< (HSMCI) DMA Configuration Register */
#define REG_HSMCI_CFG           (0x40000054) /**< (HSMCI) Configuration Register */
#define REG_HSMCI_WPMR          (0x400000E4) /**< (HSMCI) Write Protection Mode Register */
#define REG_HSMCI_WPSR          (0x400000E8) /**< (HSMCI) Write Protection Status Register */
#define REG_HSMCI_FIFO          (0x40000200) /**< (HSMCI) FIFO Memory Aperture0 0 */
#define REG_HSMCI_FIFO0         (0x40000200) /**< (HSMCI) FIFO Memory Aperture0 0 */
#define REG_HSMCI_FIFO1         (0x40000204) /**< (HSMCI) FIFO Memory Aperture0 1 */
#define REG_HSMCI_FIFO2         (0x40000208) /**< (HSMCI) FIFO Memory Aperture0 2 */
#define REG_HSMCI_FIFO3         (0x4000020C) /**< (HSMCI) FIFO Memory Aperture0 3 */
#define REG_HSMCI_FIFO4         (0x40000210) /**< (HSMCI) FIFO Memory Aperture0 4 */
#define REG_HSMCI_FIFO5         (0x40000214) /**< (HSMCI) FIFO Memory Aperture0 5 */
#define REG_HSMCI_FIFO6         (0x40000218) /**< (HSMCI) FIFO Memory Aperture0 6 */
#define REG_HSMCI_FIFO7         (0x4000021C) /**< (HSMCI) FIFO Memory Aperture0 7 */
#define REG_HSMCI_FIFO8         (0x40000220) /**< (HSMCI) FIFO Memory Aperture0 8 */
#define REG_HSMCI_FIFO9         (0x40000224) /**< (HSMCI) FIFO Memory Aperture0 9 */
#define REG_HSMCI_FIFO10        (0x40000228) /**< (HSMCI) FIFO Memory Aperture0 10 */
#define REG_HSMCI_FIFO11        (0x4000022C) /**< (HSMCI) FIFO Memory Aperture0 11 */
#define REG_HSMCI_FIFO12        (0x40000230) /**< (HSMCI) FIFO Memory Aperture0 12 */
#define REG_HSMCI_FIFO13        (0x40000234) /**< (HSMCI) FIFO Memory Aperture0 13 */
#define REG_HSMCI_FIFO14        (0x40000238) /**< (HSMCI) FIFO Memory Aperture0 14 */
#define REG_HSMCI_FIFO15        (0x4000023C) /**< (HSMCI) FIFO Memory Aperture0 15 */
#define REG_HSMCI_FIFO16        (0x40000240) /**< (HSMCI) FIFO Memory Aperture0 16 */
#define REG_HSMCI_FIFO17        (0x40000244) /**< (HSMCI) FIFO Memory Aperture0 17 */
#define REG_HSMCI_FIFO18        (0x40000248) /**< (HSMCI) FIFO Memory Aperture0 18 */
#define REG_HSMCI_FIFO19        (0x4000024C) /**< (HSMCI) FIFO Memory Aperture0 19 */
#define REG_HSMCI_FIFO20        (0x40000250) /**< (HSMCI) FIFO Memory Aperture0 20 */
#define REG_HSMCI_FIFO21        (0x40000254) /**< (HSMCI) FIFO Memory Aperture0 21 */
#define REG_HSMCI_FIFO22        (0x40000258) /**< (HSMCI) FIFO Memory Aperture0 22 */
#define REG_HSMCI_FIFO23        (0x4000025C) /**< (HSMCI) FIFO Memory Aperture0 23 */
#define REG_HSMCI_FIFO24        (0x40000260) /**< (HSMCI) FIFO Memory Aperture0 24 */
#define REG_HSMCI_FIFO25        (0x40000264) /**< (HSMCI) FIFO Memory Aperture0 25 */
#define REG_HSMCI_FIFO26        (0x40000268) /**< (HSMCI) FIFO Memory Aperture0 26 */
#define REG_HSMCI_FIFO27        (0x4000026C) /**< (HSMCI) FIFO Memory Aperture0 27 */
#define REG_HSMCI_FIFO28        (0x40000270) /**< (HSMCI) FIFO Memory Aperture0 28 */
#define REG_HSMCI_FIFO29        (0x40000274) /**< (HSMCI) FIFO Memory Aperture0 29 */
#define REG_HSMCI_FIFO30        (0x40000278) /**< (HSMCI) FIFO Memory Aperture0 30 */
#define REG_HSMCI_FIFO31        (0x4000027C) /**< (HSMCI) FIFO Memory Aperture0 31 */
#define REG_HSMCI_FIFO32        (0x40000280) /**< (HSMCI) FIFO Memory Aperture0 32 */
#define REG_HSMCI_FIFO33        (0x40000284) /**< (HSMCI) FIFO Memory Aperture0 33 */
#define REG_HSMCI_FIFO34        (0x40000288) /**< (HSMCI) FIFO Memory Aperture0 34 */
#define REG_HSMCI_FIFO35        (0x4000028C) /**< (HSMCI) FIFO Memory Aperture0 35 */
#define REG_HSMCI_FIFO36        (0x40000290) /**< (HSMCI) FIFO Memory Aperture0 36 */
#define REG_HSMCI_FIFO37        (0x40000294) /**< (HSMCI) FIFO Memory Aperture0 37 */
#define REG_HSMCI_FIFO38        (0x40000298) /**< (HSMCI) FIFO Memory Aperture0 38 */
#define REG_HSMCI_FIFO39        (0x4000029C) /**< (HSMCI) FIFO Memory Aperture0 39 */
#define REG_HSMCI_FIFO40        (0x400002A0) /**< (HSMCI) FIFO Memory Aperture0 40 */
#define REG_HSMCI_FIFO41        (0x400002A4) /**< (HSMCI) FIFO Memory Aperture0 41 */
#define REG_HSMCI_FIFO42        (0x400002A8) /**< (HSMCI) FIFO Memory Aperture0 42 */
#define REG_HSMCI_FIFO43        (0x400002AC) /**< (HSMCI) FIFO Memory Aperture0 43 */
#define REG_HSMCI_FIFO44        (0x400002B0) /**< (HSMCI) FIFO Memory Aperture0 44 */
#define REG_HSMCI_FIFO45        (0x400002B4) /**< (HSMCI) FIFO Memory Aperture0 45 */
#define REG_HSMCI_FIFO46        (0x400002B8) /**< (HSMCI) FIFO Memory Aperture0 46 */
#define REG_HSMCI_FIFO47        (0x400002BC) /**< (HSMCI) FIFO Memory Aperture0 47 */
#define REG_HSMCI_FIFO48        (0x400002C0) /**< (HSMCI) FIFO Memory Aperture0 48 */
#define REG_HSMCI_FIFO49        (0x400002C4) /**< (HSMCI) FIFO Memory Aperture0 49 */
#define REG_HSMCI_FIFO50        (0x400002C8) /**< (HSMCI) FIFO Memory Aperture0 50 */
#define REG_HSMCI_FIFO51        (0x400002CC) /**< (HSMCI) FIFO Memory Aperture0 51 */
#define REG_HSMCI_FIFO52        (0x400002D0) /**< (HSMCI) FIFO Memory Aperture0 52 */
#define REG_HSMCI_FIFO53        (0x400002D4) /**< (HSMCI) FIFO Memory Aperture0 53 */
#define REG_HSMCI_FIFO54        (0x400002D8) /**< (HSMCI) FIFO Memory Aperture0 54 */
#define REG_HSMCI_FIFO55        (0x400002DC) /**< (HSMCI) FIFO Memory Aperture0 55 */
#define REG_HSMCI_FIFO56        (0x400002E0) /**< (HSMCI) FIFO Memory Aperture0 56 */
#define REG_HSMCI_FIFO57        (0x400002E4) /**< (HSMCI) FIFO Memory Aperture0 57 */
#define REG_HSMCI_FIFO58        (0x400002E8) /**< (HSMCI) FIFO Memory Aperture0 58 */
#define REG_HSMCI_FIFO59        (0x400002EC) /**< (HSMCI) FIFO Memory Aperture0 59 */
#define REG_HSMCI_FIFO60        (0x400002F0) /**< (HSMCI) FIFO Memory Aperture0 60 */
#define REG_HSMCI_FIFO61        (0x400002F4) /**< (HSMCI) FIFO Memory Aperture0 61 */
#define REG_HSMCI_FIFO62        (0x400002F8) /**< (HSMCI) FIFO Memory Aperture0 62 */
#define REG_HSMCI_FIFO63        (0x400002FC) /**< (HSMCI) FIFO Memory Aperture0 63 */
#define REG_HSMCI_FIFO64        (0x40000300) /**< (HSMCI) FIFO Memory Aperture0 64 */
#define REG_HSMCI_FIFO65        (0x40000304) /**< (HSMCI) FIFO Memory Aperture0 65 */
#define REG_HSMCI_FIFO66        (0x40000308) /**< (HSMCI) FIFO Memory Aperture0 66 */
#define REG_HSMCI_FIFO67        (0x4000030C) /**< (HSMCI) FIFO Memory Aperture0 67 */
#define REG_HSMCI_FIFO68        (0x40000310) /**< (HSMCI) FIFO Memory Aperture0 68 */
#define REG_HSMCI_FIFO69        (0x40000314) /**< (HSMCI) FIFO Memory Aperture0 69 */
#define REG_HSMCI_FIFO70        (0x40000318) /**< (HSMCI) FIFO Memory Aperture0 70 */
#define REG_HSMCI_FIFO71        (0x4000031C) /**< (HSMCI) FIFO Memory Aperture0 71 */
#define REG_HSMCI_FIFO72        (0x40000320) /**< (HSMCI) FIFO Memory Aperture0 72 */
#define REG_HSMCI_FIFO73        (0x40000324) /**< (HSMCI) FIFO Memory Aperture0 73 */
#define REG_HSMCI_FIFO74        (0x40000328) /**< (HSMCI) FIFO Memory Aperture0 74 */
#define REG_HSMCI_FIFO75        (0x4000032C) /**< (HSMCI) FIFO Memory Aperture0 75 */
#define REG_HSMCI_FIFO76        (0x40000330) /**< (HSMCI) FIFO Memory Aperture0 76 */
#define REG_HSMCI_FIFO77        (0x40000334) /**< (HSMCI) FIFO Memory Aperture0 77 */
#define REG_HSMCI_FIFO78        (0x40000338) /**< (HSMCI) FIFO Memory Aperture0 78 */
#define REG_HSMCI_FIFO79        (0x4000033C) /**< (HSMCI) FIFO Memory Aperture0 79 */
#define REG_HSMCI_FIFO80        (0x40000340) /**< (HSMCI) FIFO Memory Aperture0 80 */
#define REG_HSMCI_FIFO81        (0x40000344) /**< (HSMCI) FIFO Memory Aperture0 81 */
#define REG_HSMCI_FIFO82        (0x40000348) /**< (HSMCI) FIFO Memory Aperture0 82 */
#define REG_HSMCI_FIFO83        (0x4000034C) /**< (HSMCI) FIFO Memory Aperture0 83 */
#define REG_HSMCI_FIFO84        (0x40000350) /**< (HSMCI) FIFO Memory Aperture0 84 */
#define REG_HSMCI_FIFO85        (0x40000354) /**< (HSMCI) FIFO Memory Aperture0 85 */
#define REG_HSMCI_FIFO86        (0x40000358) /**< (HSMCI) FIFO Memory Aperture0 86 */
#define REG_HSMCI_FIFO87        (0x4000035C) /**< (HSMCI) FIFO Memory Aperture0 87 */
#define REG_HSMCI_FIFO88        (0x40000360) /**< (HSMCI) FIFO Memory Aperture0 88 */
#define REG_HSMCI_FIFO89        (0x40000364) /**< (HSMCI) FIFO Memory Aperture0 89 */
#define REG_HSMCI_FIFO90        (0x40000368) /**< (HSMCI) FIFO Memory Aperture0 90 */
#define REG_HSMCI_FIFO91        (0x4000036C) /**< (HSMCI) FIFO Memory Aperture0 91 */
#define REG_HSMCI_FIFO92        (0x40000370) /**< (HSMCI) FIFO Memory Aperture0 92 */
#define REG_HSMCI_FIFO93        (0x40000374) /**< (HSMCI) FIFO Memory Aperture0 93 */
#define REG_HSMCI_FIFO94        (0x40000378) /**< (HSMCI) FIFO Memory Aperture0 94 */
#define REG_HSMCI_FIFO95        (0x4000037C) /**< (HSMCI) FIFO Memory Aperture0 95 */
#define REG_HSMCI_FIFO96        (0x40000380) /**< (HSMCI) FIFO Memory Aperture0 96 */
#define REG_HSMCI_FIFO97        (0x40000384) /**< (HSMCI) FIFO Memory Aperture0 97 */
#define REG_HSMCI_FIFO98        (0x40000388) /**< (HSMCI) FIFO Memory Aperture0 98 */
#define REG_HSMCI_FIFO99        (0x4000038C) /**< (HSMCI) FIFO Memory Aperture0 99 */
#define REG_HSMCI_FIFO100       (0x40000390) /**< (HSMCI) FIFO Memory Aperture0 100 */
#define REG_HSMCI_FIFO101       (0x40000394) /**< (HSMCI) FIFO Memory Aperture0 101 */
#define REG_HSMCI_FIFO102       (0x40000398) /**< (HSMCI) FIFO Memory Aperture0 102 */
#define REG_HSMCI_FIFO103       (0x4000039C) /**< (HSMCI) FIFO Memory Aperture0 103 */
#define REG_HSMCI_FIFO104       (0x400003A0) /**< (HSMCI) FIFO Memory Aperture0 104 */
#define REG_HSMCI_FIFO105       (0x400003A4) /**< (HSMCI) FIFO Memory Aperture0 105 */
#define REG_HSMCI_FIFO106       (0x400003A8) /**< (HSMCI) FIFO Memory Aperture0 106 */
#define REG_HSMCI_FIFO107       (0x400003AC) /**< (HSMCI) FIFO Memory Aperture0 107 */
#define REG_HSMCI_FIFO108       (0x400003B0) /**< (HSMCI) FIFO Memory Aperture0 108 */
#define REG_HSMCI_FIFO109       (0x400003B4) /**< (HSMCI) FIFO Memory Aperture0 109 */
#define REG_HSMCI_FIFO110       (0x400003B8) /**< (HSMCI) FIFO Memory Aperture0 110 */
#define REG_HSMCI_FIFO111       (0x400003BC) /**< (HSMCI) FIFO Memory Aperture0 111 */
#define REG_HSMCI_FIFO112       (0x400003C0) /**< (HSMCI) FIFO Memory Aperture0 112 */
#define REG_HSMCI_FIFO113       (0x400003C4) /**< (HSMCI) FIFO Memory Aperture0 113 */
#define REG_HSMCI_FIFO114       (0x400003C8) /**< (HSMCI) FIFO Memory Aperture0 114 */
#define REG_HSMCI_FIFO115       (0x400003CC) /**< (HSMCI) FIFO Memory Aperture0 115 */
#define REG_HSMCI_FIFO116       (0x400003D0) /**< (HSMCI) FIFO Memory Aperture0 116 */
#define REG_HSMCI_FIFO117       (0x400003D4) /**< (HSMCI) FIFO Memory Aperture0 117 */
#define REG_HSMCI_FIFO118       (0x400003D8) /**< (HSMCI) FIFO Memory Aperture0 118 */
#define REG_HSMCI_FIFO119       (0x400003DC) /**< (HSMCI) FIFO Memory Aperture0 119 */
#define REG_HSMCI_FIFO120       (0x400003E0) /**< (HSMCI) FIFO Memory Aperture0 120 */
#define REG_HSMCI_FIFO121       (0x400003E4) /**< (HSMCI) FIFO Memory Aperture0 121 */
#define REG_HSMCI_FIFO122       (0x400003E8) /**< (HSMCI) FIFO Memory Aperture0 122 */
#define REG_HSMCI_FIFO123       (0x400003EC) /**< (HSMCI) FIFO Memory Aperture0 123 */
#define REG_HSMCI_FIFO124       (0x400003F0) /**< (HSMCI) FIFO Memory Aperture0 124 */
#define REG_HSMCI_FIFO125       (0x400003F4) /**< (HSMCI) FIFO Memory Aperture0 125 */
#define REG_HSMCI_FIFO126       (0x400003F8) /**< (HSMCI) FIFO Memory Aperture0 126 */
#define REG_HSMCI_FIFO127       (0x400003FC) /**< (HSMCI) FIFO Memory Aperture0 127 */
#define REG_HSMCI_FIFO128       (0x40000400) /**< (HSMCI) FIFO Memory Aperture0 128 */
#define REG_HSMCI_FIFO129       (0x40000404) /**< (HSMCI) FIFO Memory Aperture0 129 */
#define REG_HSMCI_FIFO130       (0x40000408) /**< (HSMCI) FIFO Memory Aperture0 130 */
#define REG_HSMCI_FIFO131       (0x4000040C) /**< (HSMCI) FIFO Memory Aperture0 131 */
#define REG_HSMCI_FIFO132       (0x40000410) /**< (HSMCI) FIFO Memory Aperture0 132 */
#define REG_HSMCI_FIFO133       (0x40000414) /**< (HSMCI) FIFO Memory Aperture0 133 */
#define REG_HSMCI_FIFO134       (0x40000418) /**< (HSMCI) FIFO Memory Aperture0 134 */
#define REG_HSMCI_FIFO135       (0x4000041C) /**< (HSMCI) FIFO Memory Aperture0 135 */
#define REG_HSMCI_FIFO136       (0x40000420) /**< (HSMCI) FIFO Memory Aperture0 136 */
#define REG_HSMCI_FIFO137       (0x40000424) /**< (HSMCI) FIFO Memory Aperture0 137 */
#define REG_HSMCI_FIFO138       (0x40000428) /**< (HSMCI) FIFO Memory Aperture0 138 */
#define REG_HSMCI_FIFO139       (0x4000042C) /**< (HSMCI) FIFO Memory Aperture0 139 */
#define REG_HSMCI_FIFO140       (0x40000430) /**< (HSMCI) FIFO Memory Aperture0 140 */
#define REG_HSMCI_FIFO141       (0x40000434) /**< (HSMCI) FIFO Memory Aperture0 141 */
#define REG_HSMCI_FIFO142       (0x40000438) /**< (HSMCI) FIFO Memory Aperture0 142 */
#define REG_HSMCI_FIFO143       (0x4000043C) /**< (HSMCI) FIFO Memory Aperture0 143 */
#define REG_HSMCI_FIFO144       (0x40000440) /**< (HSMCI) FIFO Memory Aperture0 144 */
#define REG_HSMCI_FIFO145       (0x40000444) /**< (HSMCI) FIFO Memory Aperture0 145 */
#define REG_HSMCI_FIFO146       (0x40000448) /**< (HSMCI) FIFO Memory Aperture0 146 */
#define REG_HSMCI_FIFO147       (0x4000044C) /**< (HSMCI) FIFO Memory Aperture0 147 */
#define REG_HSMCI_FIFO148       (0x40000450) /**< (HSMCI) FIFO Memory Aperture0 148 */
#define REG_HSMCI_FIFO149       (0x40000454) /**< (HSMCI) FIFO Memory Aperture0 149 */
#define REG_HSMCI_FIFO150       (0x40000458) /**< (HSMCI) FIFO Memory Aperture0 150 */
#define REG_HSMCI_FIFO151       (0x4000045C) /**< (HSMCI) FIFO Memory Aperture0 151 */
#define REG_HSMCI_FIFO152       (0x40000460) /**< (HSMCI) FIFO Memory Aperture0 152 */
#define REG_HSMCI_FIFO153       (0x40000464) /**< (HSMCI) FIFO Memory Aperture0 153 */
#define REG_HSMCI_FIFO154       (0x40000468) /**< (HSMCI) FIFO Memory Aperture0 154 */
#define REG_HSMCI_FIFO155       (0x4000046C) /**< (HSMCI) FIFO Memory Aperture0 155 */
#define REG_HSMCI_FIFO156       (0x40000470) /**< (HSMCI) FIFO Memory Aperture0 156 */
#define REG_HSMCI_FIFO157       (0x40000474) /**< (HSMCI) FIFO Memory Aperture0 157 */
#define REG_HSMCI_FIFO158       (0x40000478) /**< (HSMCI) FIFO Memory Aperture0 158 */
#define REG_HSMCI_FIFO159       (0x4000047C) /**< (HSMCI) FIFO Memory Aperture0 159 */
#define REG_HSMCI_FIFO160       (0x40000480) /**< (HSMCI) FIFO Memory Aperture0 160 */
#define REG_HSMCI_FIFO161       (0x40000484) /**< (HSMCI) FIFO Memory Aperture0 161 */
#define REG_HSMCI_FIFO162       (0x40000488) /**< (HSMCI) FIFO Memory Aperture0 162 */
#define REG_HSMCI_FIFO163       (0x4000048C) /**< (HSMCI) FIFO Memory Aperture0 163 */
#define REG_HSMCI_FIFO164       (0x40000490) /**< (HSMCI) FIFO Memory Aperture0 164 */
#define REG_HSMCI_FIFO165       (0x40000494) /**< (HSMCI) FIFO Memory Aperture0 165 */
#define REG_HSMCI_FIFO166       (0x40000498) /**< (HSMCI) FIFO Memory Aperture0 166 */
#define REG_HSMCI_FIFO167       (0x4000049C) /**< (HSMCI) FIFO Memory Aperture0 167 */
#define REG_HSMCI_FIFO168       (0x400004A0) /**< (HSMCI) FIFO Memory Aperture0 168 */
#define REG_HSMCI_FIFO169       (0x400004A4) /**< (HSMCI) FIFO Memory Aperture0 169 */
#define REG_HSMCI_FIFO170       (0x400004A8) /**< (HSMCI) FIFO Memory Aperture0 170 */
#define REG_HSMCI_FIFO171       (0x400004AC) /**< (HSMCI) FIFO Memory Aperture0 171 */
#define REG_HSMCI_FIFO172       (0x400004B0) /**< (HSMCI) FIFO Memory Aperture0 172 */
#define REG_HSMCI_FIFO173       (0x400004B4) /**< (HSMCI) FIFO Memory Aperture0 173 */
#define REG_HSMCI_FIFO174       (0x400004B8) /**< (HSMCI) FIFO Memory Aperture0 174 */
#define REG_HSMCI_FIFO175       (0x400004BC) /**< (HSMCI) FIFO Memory Aperture0 175 */
#define REG_HSMCI_FIFO176       (0x400004C0) /**< (HSMCI) FIFO Memory Aperture0 176 */
#define REG_HSMCI_FIFO177       (0x400004C4) /**< (HSMCI) FIFO Memory Aperture0 177 */
#define REG_HSMCI_FIFO178       (0x400004C8) /**< (HSMCI) FIFO Memory Aperture0 178 */
#define REG_HSMCI_FIFO179       (0x400004CC) /**< (HSMCI) FIFO Memory Aperture0 179 */
#define REG_HSMCI_FIFO180       (0x400004D0) /**< (HSMCI) FIFO Memory Aperture0 180 */
#define REG_HSMCI_FIFO181       (0x400004D4) /**< (HSMCI) FIFO Memory Aperture0 181 */
#define REG_HSMCI_FIFO182       (0x400004D8) /**< (HSMCI) FIFO Memory Aperture0 182 */
#define REG_HSMCI_FIFO183       (0x400004DC) /**< (HSMCI) FIFO Memory Aperture0 183 */
#define REG_HSMCI_FIFO184       (0x400004E0) /**< (HSMCI) FIFO Memory Aperture0 184 */
#define REG_HSMCI_FIFO185       (0x400004E4) /**< (HSMCI) FIFO Memory Aperture0 185 */
#define REG_HSMCI_FIFO186       (0x400004E8) /**< (HSMCI) FIFO Memory Aperture0 186 */
#define REG_HSMCI_FIFO187       (0x400004EC) /**< (HSMCI) FIFO Memory Aperture0 187 */
#define REG_HSMCI_FIFO188       (0x400004F0) /**< (HSMCI) FIFO Memory Aperture0 188 */
#define REG_HSMCI_FIFO189       (0x400004F4) /**< (HSMCI) FIFO Memory Aperture0 189 */
#define REG_HSMCI_FIFO190       (0x400004F8) /**< (HSMCI) FIFO Memory Aperture0 190 */
#define REG_HSMCI_FIFO191       (0x400004FC) /**< (HSMCI) FIFO Memory Aperture0 191 */
#define REG_HSMCI_FIFO192       (0x40000500) /**< (HSMCI) FIFO Memory Aperture0 192 */
#define REG_HSMCI_FIFO193       (0x40000504) /**< (HSMCI) FIFO Memory Aperture0 193 */
#define REG_HSMCI_FIFO194       (0x40000508) /**< (HSMCI) FIFO Memory Aperture0 194 */
#define REG_HSMCI_FIFO195       (0x4000050C) /**< (HSMCI) FIFO Memory Aperture0 195 */
#define REG_HSMCI_FIFO196       (0x40000510) /**< (HSMCI) FIFO Memory Aperture0 196 */
#define REG_HSMCI_FIFO197       (0x40000514) /**< (HSMCI) FIFO Memory Aperture0 197 */
#define REG_HSMCI_FIFO198       (0x40000518) /**< (HSMCI) FIFO Memory Aperture0 198 */
#define REG_HSMCI_FIFO199       (0x4000051C) /**< (HSMCI) FIFO Memory Aperture0 199 */
#define REG_HSMCI_FIFO200       (0x40000520) /**< (HSMCI) FIFO Memory Aperture0 200 */
#define REG_HSMCI_FIFO201       (0x40000524) /**< (HSMCI) FIFO Memory Aperture0 201 */
#define REG_HSMCI_FIFO202       (0x40000528) /**< (HSMCI) FIFO Memory Aperture0 202 */
#define REG_HSMCI_FIFO203       (0x4000052C) /**< (HSMCI) FIFO Memory Aperture0 203 */
#define REG_HSMCI_FIFO204       (0x40000530) /**< (HSMCI) FIFO Memory Aperture0 204 */
#define REG_HSMCI_FIFO205       (0x40000534) /**< (HSMCI) FIFO Memory Aperture0 205 */
#define REG_HSMCI_FIFO206       (0x40000538) /**< (HSMCI) FIFO Memory Aperture0 206 */
#define REG_HSMCI_FIFO207       (0x4000053C) /**< (HSMCI) FIFO Memory Aperture0 207 */
#define REG_HSMCI_FIFO208       (0x40000540) /**< (HSMCI) FIFO Memory Aperture0 208 */
#define REG_HSMCI_FIFO209       (0x40000544) /**< (HSMCI) FIFO Memory Aperture0 209 */
#define REG_HSMCI_FIFO210       (0x40000548) /**< (HSMCI) FIFO Memory Aperture0 210 */
#define REG_HSMCI_FIFO211       (0x4000054C) /**< (HSMCI) FIFO Memory Aperture0 211 */
#define REG_HSMCI_FIFO212       (0x40000550) /**< (HSMCI) FIFO Memory Aperture0 212 */
#define REG_HSMCI_FIFO213       (0x40000554) /**< (HSMCI) FIFO Memory Aperture0 213 */
#define REG_HSMCI_FIFO214       (0x40000558) /**< (HSMCI) FIFO Memory Aperture0 214 */
#define REG_HSMCI_FIFO215       (0x4000055C) /**< (HSMCI) FIFO Memory Aperture0 215 */
#define REG_HSMCI_FIFO216       (0x40000560) /**< (HSMCI) FIFO Memory Aperture0 216 */
#define REG_HSMCI_FIFO217       (0x40000564) /**< (HSMCI) FIFO Memory Aperture0 217 */
#define REG_HSMCI_FIFO218       (0x40000568) /**< (HSMCI) FIFO Memory Aperture0 218 */
#define REG_HSMCI_FIFO219       (0x4000056C) /**< (HSMCI) FIFO Memory Aperture0 219 */
#define REG_HSMCI_FIFO220       (0x40000570) /**< (HSMCI) FIFO Memory Aperture0 220 */
#define REG_HSMCI_FIFO221       (0x40000574) /**< (HSMCI) FIFO Memory Aperture0 221 */
#define REG_HSMCI_FIFO222       (0x40000578) /**< (HSMCI) FIFO Memory Aperture0 222 */
#define REG_HSMCI_FIFO223       (0x4000057C) /**< (HSMCI) FIFO Memory Aperture0 223 */
#define REG_HSMCI_FIFO224       (0x40000580) /**< (HSMCI) FIFO Memory Aperture0 224 */
#define REG_HSMCI_FIFO225       (0x40000584) /**< (HSMCI) FIFO Memory Aperture0 225 */
#define REG_HSMCI_FIFO226       (0x40000588) /**< (HSMCI) FIFO Memory Aperture0 226 */
#define REG_HSMCI_FIFO227       (0x4000058C) /**< (HSMCI) FIFO Memory Aperture0 227 */
#define REG_HSMCI_FIFO228       (0x40000590) /**< (HSMCI) FIFO Memory Aperture0 228 */
#define REG_HSMCI_FIFO229       (0x40000594) /**< (HSMCI) FIFO Memory Aperture0 229 */
#define REG_HSMCI_FIFO230       (0x40000598) /**< (HSMCI) FIFO Memory Aperture0 230 */
#define REG_HSMCI_FIFO231       (0x4000059C) /**< (HSMCI) FIFO Memory Aperture0 231 */
#define REG_HSMCI_FIFO232       (0x400005A0) /**< (HSMCI) FIFO Memory Aperture0 232 */
#define REG_HSMCI_FIFO233       (0x400005A4) /**< (HSMCI) FIFO Memory Aperture0 233 */
#define REG_HSMCI_FIFO234       (0x400005A8) /**< (HSMCI) FIFO Memory Aperture0 234 */
#define REG_HSMCI_FIFO235       (0x400005AC) /**< (HSMCI) FIFO Memory Aperture0 235 */
#define REG_HSMCI_FIFO236       (0x400005B0) /**< (HSMCI) FIFO Memory Aperture0 236 */
#define REG_HSMCI_FIFO237       (0x400005B4) /**< (HSMCI) FIFO Memory Aperture0 237 */
#define REG_HSMCI_FIFO238       (0x400005B8) /**< (HSMCI) FIFO Memory Aperture0 238 */
#define REG_HSMCI_FIFO239       (0x400005BC) /**< (HSMCI) FIFO Memory Aperture0 239 */
#define REG_HSMCI_FIFO240       (0x400005C0) /**< (HSMCI) FIFO Memory Aperture0 240 */
#define REG_HSMCI_FIFO241       (0x400005C4) /**< (HSMCI) FIFO Memory Aperture0 241 */
#define REG_HSMCI_FIFO242       (0x400005C8) /**< (HSMCI) FIFO Memory Aperture0 242 */
#define REG_HSMCI_FIFO243       (0x400005CC) /**< (HSMCI) FIFO Memory Aperture0 243 */
#define REG_HSMCI_FIFO244       (0x400005D0) /**< (HSMCI) FIFO Memory Aperture0 244 */
#define REG_HSMCI_FIFO245       (0x400005D4) /**< (HSMCI) FIFO Memory Aperture0 245 */
#define REG_HSMCI_FIFO246       (0x400005D8) /**< (HSMCI) FIFO Memory Aperture0 246 */
#define REG_HSMCI_FIFO247       (0x400005DC) /**< (HSMCI) FIFO Memory Aperture0 247 */
#define REG_HSMCI_FIFO248       (0x400005E0) /**< (HSMCI) FIFO Memory Aperture0 248 */
#define REG_HSMCI_FIFO249       (0x400005E4) /**< (HSMCI) FIFO Memory Aperture0 249 */
#define REG_HSMCI_FIFO250       (0x400005E8) /**< (HSMCI) FIFO Memory Aperture0 250 */
#define REG_HSMCI_FIFO251       (0x400005EC) /**< (HSMCI) FIFO Memory Aperture0 251 */
#define REG_HSMCI_FIFO252       (0x400005F0) /**< (HSMCI) FIFO Memory Aperture0 252 */
#define REG_HSMCI_FIFO253       (0x400005F4) /**< (HSMCI) FIFO Memory Aperture0 253 */
#define REG_HSMCI_FIFO254       (0x400005F8) /**< (HSMCI) FIFO Memory Aperture0 254 */
#define REG_HSMCI_FIFO255       (0x400005FC) /**< (HSMCI) FIFO Memory Aperture0 255 */

#else

#define REG_HSMCI_CR            (*(__O  uint32_t*)0x40000000U) /**< (HSMCI) Control Register */
#define REG_HSMCI_MR            (*(__IO uint32_t*)0x40000004U) /**< (HSMCI) Mode Register */
#define REG_HSMCI_DTOR          (*(__IO uint32_t*)0x40000008U) /**< (HSMCI) Data Timeout Register */
#define REG_HSMCI_SDCR          (*(__IO uint32_t*)0x4000000CU) /**< (HSMCI) SD/SDIO Card Register */
#define REG_HSMCI_ARGR          (*(__IO uint32_t*)0x40000010U) /**< (HSMCI) Argument Register */
#define REG_HSMCI_CMDR          (*(__O  uint32_t*)0x40000014U) /**< (HSMCI) Command Register */
#define REG_HSMCI_BLKR          (*(__IO uint32_t*)0x40000018U) /**< (HSMCI) Block Register */
#define REG_HSMCI_CSTOR         (*(__IO uint32_t*)0x4000001CU) /**< (HSMCI) Completion Signal Timeout Register */
#define REG_HSMCI_RSPR          (*(__I  uint32_t*)0x40000020U) /**< (HSMCI) Response Register 0 */
#define REG_HSMCI_RSPR0         (*(__I  uint32_t*)0x40000020U) /**< (HSMCI) Response Register 0 */
#define REG_HSMCI_RSPR1         (*(__I  uint32_t*)0x40000024U) /**< (HSMCI) Response Register 1 */
#define REG_HSMCI_RSPR2         (*(__I  uint32_t*)0x40000028U) /**< (HSMCI) Response Register 2 */
#define REG_HSMCI_RSPR3         (*(__I  uint32_t*)0x4000002CU) /**< (HSMCI) Response Register 3 */
#define REG_HSMCI_RDR           (*(__I  uint32_t*)0x40000030U) /**< (HSMCI) Receive Data Register */
#define REG_HSMCI_TDR           (*(__O  uint32_t*)0x40000034U) /**< (HSMCI) Transmit Data Register */
#define REG_HSMCI_SR            (*(__I  uint32_t*)0x40000040U) /**< (HSMCI) Status Register */
#define REG_HSMCI_IER           (*(__O  uint32_t*)0x40000044U) /**< (HSMCI) Interrupt Enable Register */
#define REG_HSMCI_IDR           (*(__O  uint32_t*)0x40000048U) /**< (HSMCI) Interrupt Disable Register */
#define REG_HSMCI_IMR           (*(__I  uint32_t*)0x4000004CU) /**< (HSMCI) Interrupt Mask Register */
#define REG_HSMCI_DMA           (*(__IO uint32_t*)0x40000050U) /**< (HSMCI) DMA Configuration Register */
#define REG_HSMCI_CFG           (*(__IO uint32_t*)0x40000054U) /**< (HSMCI) Configuration Register */
#define REG_HSMCI_WPMR          (*(__IO uint32_t*)0x400000E4U) /**< (HSMCI) Write Protection Mode Register */
#define REG_HSMCI_WPSR          (*(__I  uint32_t*)0x400000E8U) /**< (HSMCI) Write Protection Status Register */
#define REG_HSMCI_FIFO          (*(__IO uint32_t*)0x40000200U) /**< (HSMCI) FIFO Memory Aperture0 0 */
#define REG_HSMCI_FIFO0         (*(__IO uint32_t*)0x40000200U) /**< (HSMCI) FIFO Memory Aperture0 0 */
#define REG_HSMCI_FIFO1         (*(__IO uint32_t*)0x40000204U) /**< (HSMCI) FIFO Memory Aperture0 1 */
#define REG_HSMCI_FIFO2         (*(__IO uint32_t*)0x40000208U) /**< (HSMCI) FIFO Memory Aperture0 2 */
#define REG_HSMCI_FIFO3         (*(__IO uint32_t*)0x4000020CU) /**< (HSMCI) FIFO Memory Aperture0 3 */
#define REG_HSMCI_FIFO4         (*(__IO uint32_t*)0x40000210U) /**< (HSMCI) FIFO Memory Aperture0 4 */
#define REG_HSMCI_FIFO5         (*(__IO uint32_t*)0x40000214U) /**< (HSMCI) FIFO Memory Aperture0 5 */
#define REG_HSMCI_FIFO6         (*(__IO uint32_t*)0x40000218U) /**< (HSMCI) FIFO Memory Aperture0 6 */
#define REG_HSMCI_FIFO7         (*(__IO uint32_t*)0x4000021CU) /**< (HSMCI) FIFO Memory Aperture0 7 */
#define REG_HSMCI_FIFO8         (*(__IO uint32_t*)0x40000220U) /**< (HSMCI) FIFO Memory Aperture0 8 */
#define REG_HSMCI_FIFO9         (*(__IO uint32_t*)0x40000224U) /**< (HSMCI) FIFO Memory Aperture0 9 */
#define REG_HSMCI_FIFO10        (*(__IO uint32_t*)0x40000228U) /**< (HSMCI) FIFO Memory Aperture0 10 */
#define REG_HSMCI_FIFO11        (*(__IO uint32_t*)0x4000022CU) /**< (HSMCI) FIFO Memory Aperture0 11 */
#define REG_HSMCI_FIFO12        (*(__IO uint32_t*)0x40000230U) /**< (HSMCI) FIFO Memory Aperture0 12 */
#define REG_HSMCI_FIFO13        (*(__IO uint32_t*)0x40000234U) /**< (HSMCI) FIFO Memory Aperture0 13 */
#define REG_HSMCI_FIFO14        (*(__IO uint32_t*)0x40000238U) /**< (HSMCI) FIFO Memory Aperture0 14 */
#define REG_HSMCI_FIFO15        (*(__IO uint32_t*)0x4000023CU) /**< (HSMCI) FIFO Memory Aperture0 15 */
#define REG_HSMCI_FIFO16        (*(__IO uint32_t*)0x40000240U) /**< (HSMCI) FIFO Memory Aperture0 16 */
#define REG_HSMCI_FIFO17        (*(__IO uint32_t*)0x40000244U) /**< (HSMCI) FIFO Memory Aperture0 17 */
#define REG_HSMCI_FIFO18        (*(__IO uint32_t*)0x40000248U) /**< (HSMCI) FIFO Memory Aperture0 18 */
#define REG_HSMCI_FIFO19        (*(__IO uint32_t*)0x4000024CU) /**< (HSMCI) FIFO Memory Aperture0 19 */
#define REG_HSMCI_FIFO20        (*(__IO uint32_t*)0x40000250U) /**< (HSMCI) FIFO Memory Aperture0 20 */
#define REG_HSMCI_FIFO21        (*(__IO uint32_t*)0x40000254U) /**< (HSMCI) FIFO Memory Aperture0 21 */
#define REG_HSMCI_FIFO22        (*(__IO uint32_t*)0x40000258U) /**< (HSMCI) FIFO Memory Aperture0 22 */
#define REG_HSMCI_FIFO23        (*(__IO uint32_t*)0x4000025CU) /**< (HSMCI) FIFO Memory Aperture0 23 */
#define REG_HSMCI_FIFO24        (*(__IO uint32_t*)0x40000260U) /**< (HSMCI) FIFO Memory Aperture0 24 */
#define REG_HSMCI_FIFO25        (*(__IO uint32_t*)0x40000264U) /**< (HSMCI) FIFO Memory Aperture0 25 */
#define REG_HSMCI_FIFO26        (*(__IO uint32_t*)0x40000268U) /**< (HSMCI) FIFO Memory Aperture0 26 */
#define REG_HSMCI_FIFO27        (*(__IO uint32_t*)0x4000026CU) /**< (HSMCI) FIFO Memory Aperture0 27 */
#define REG_HSMCI_FIFO28        (*(__IO uint32_t*)0x40000270U) /**< (HSMCI) FIFO Memory Aperture0 28 */
#define REG_HSMCI_FIFO29        (*(__IO uint32_t*)0x40000274U) /**< (HSMCI) FIFO Memory Aperture0 29 */
#define REG_HSMCI_FIFO30        (*(__IO uint32_t*)0x40000278U) /**< (HSMCI) FIFO Memory Aperture0 30 */
#define REG_HSMCI_FIFO31        (*(__IO uint32_t*)0x4000027CU) /**< (HSMCI) FIFO Memory Aperture0 31 */
#define REG_HSMCI_FIFO32        (*(__IO uint32_t*)0x40000280U) /**< (HSMCI) FIFO Memory Aperture0 32 */
#define REG_HSMCI_FIFO33        (*(__IO uint32_t*)0x40000284U) /**< (HSMCI) FIFO Memory Aperture0 33 */
#define REG_HSMCI_FIFO34        (*(__IO uint32_t*)0x40000288U) /**< (HSMCI) FIFO Memory Aperture0 34 */
#define REG_HSMCI_FIFO35        (*(__IO uint32_t*)0x4000028CU) /**< (HSMCI) FIFO Memory Aperture0 35 */
#define REG_HSMCI_FIFO36        (*(__IO uint32_t*)0x40000290U) /**< (HSMCI) FIFO Memory Aperture0 36 */
#define REG_HSMCI_FIFO37        (*(__IO uint32_t*)0x40000294U) /**< (HSMCI) FIFO Memory Aperture0 37 */
#define REG_HSMCI_FIFO38        (*(__IO uint32_t*)0x40000298U) /**< (HSMCI) FIFO Memory Aperture0 38 */
#define REG_HSMCI_FIFO39        (*(__IO uint32_t*)0x4000029CU) /**< (HSMCI) FIFO Memory Aperture0 39 */
#define REG_HSMCI_FIFO40        (*(__IO uint32_t*)0x400002A0U) /**< (HSMCI) FIFO Memory Aperture0 40 */
#define REG_HSMCI_FIFO41        (*(__IO uint32_t*)0x400002A4U) /**< (HSMCI) FIFO Memory Aperture0 41 */
#define REG_HSMCI_FIFO42        (*(__IO uint32_t*)0x400002A8U) /**< (HSMCI) FIFO Memory Aperture0 42 */
#define REG_HSMCI_FIFO43        (*(__IO uint32_t*)0x400002ACU) /**< (HSMCI) FIFO Memory Aperture0 43 */
#define REG_HSMCI_FIFO44        (*(__IO uint32_t*)0x400002B0U) /**< (HSMCI) FIFO Memory Aperture0 44 */
#define REG_HSMCI_FIFO45        (*(__IO uint32_t*)0x400002B4U) /**< (HSMCI) FIFO Memory Aperture0 45 */
#define REG_HSMCI_FIFO46        (*(__IO uint32_t*)0x400002B8U) /**< (HSMCI) FIFO Memory Aperture0 46 */
#define REG_HSMCI_FIFO47        (*(__IO uint32_t*)0x400002BCU) /**< (HSMCI) FIFO Memory Aperture0 47 */
#define REG_HSMCI_FIFO48        (*(__IO uint32_t*)0x400002C0U) /**< (HSMCI) FIFO Memory Aperture0 48 */
#define REG_HSMCI_FIFO49        (*(__IO uint32_t*)0x400002C4U) /**< (HSMCI) FIFO Memory Aperture0 49 */
#define REG_HSMCI_FIFO50        (*(__IO uint32_t*)0x400002C8U) /**< (HSMCI) FIFO Memory Aperture0 50 */
#define REG_HSMCI_FIFO51        (*(__IO uint32_t*)0x400002CCU) /**< (HSMCI) FIFO Memory Aperture0 51 */
#define REG_HSMCI_FIFO52        (*(__IO uint32_t*)0x400002D0U) /**< (HSMCI) FIFO Memory Aperture0 52 */
#define REG_HSMCI_FIFO53        (*(__IO uint32_t*)0x400002D4U) /**< (HSMCI) FIFO Memory Aperture0 53 */
#define REG_HSMCI_FIFO54        (*(__IO uint32_t*)0x400002D8U) /**< (HSMCI) FIFO Memory Aperture0 54 */
#define REG_HSMCI_FIFO55        (*(__IO uint32_t*)0x400002DCU) /**< (HSMCI) FIFO Memory Aperture0 55 */
#define REG_HSMCI_FIFO56        (*(__IO uint32_t*)0x400002E0U) /**< (HSMCI) FIFO Memory Aperture0 56 */
#define REG_HSMCI_FIFO57        (*(__IO uint32_t*)0x400002E4U) /**< (HSMCI) FIFO Memory Aperture0 57 */
#define REG_HSMCI_FIFO58        (*(__IO uint32_t*)0x400002E8U) /**< (HSMCI) FIFO Memory Aperture0 58 */
#define REG_HSMCI_FIFO59        (*(__IO uint32_t*)0x400002ECU) /**< (HSMCI) FIFO Memory Aperture0 59 */
#define REG_HSMCI_FIFO60        (*(__IO uint32_t*)0x400002F0U) /**< (HSMCI) FIFO Memory Aperture0 60 */
#define REG_HSMCI_FIFO61        (*(__IO uint32_t*)0x400002F4U) /**< (HSMCI) FIFO Memory Aperture0 61 */
#define REG_HSMCI_FIFO62        (*(__IO uint32_t*)0x400002F8U) /**< (HSMCI) FIFO Memory Aperture0 62 */
#define REG_HSMCI_FIFO63        (*(__IO uint32_t*)0x400002FCU) /**< (HSMCI) FIFO Memory Aperture0 63 */
#define REG_HSMCI_FIFO64        (*(__IO uint32_t*)0x40000300U) /**< (HSMCI) FIFO Memory Aperture0 64 */
#define REG_HSMCI_FIFO65        (*(__IO uint32_t*)0x40000304U) /**< (HSMCI) FIFO Memory Aperture0 65 */
#define REG_HSMCI_FIFO66        (*(__IO uint32_t*)0x40000308U) /**< (HSMCI) FIFO Memory Aperture0 66 */
#define REG_HSMCI_FIFO67        (*(__IO uint32_t*)0x4000030CU) /**< (HSMCI) FIFO Memory Aperture0 67 */
#define REG_HSMCI_FIFO68        (*(__IO uint32_t*)0x40000310U) /**< (HSMCI) FIFO Memory Aperture0 68 */
#define REG_HSMCI_FIFO69        (*(__IO uint32_t*)0x40000314U) /**< (HSMCI) FIFO Memory Aperture0 69 */
#define REG_HSMCI_FIFO70        (*(__IO uint32_t*)0x40000318U) /**< (HSMCI) FIFO Memory Aperture0 70 */
#define REG_HSMCI_FIFO71        (*(__IO uint32_t*)0x4000031CU) /**< (HSMCI) FIFO Memory Aperture0 71 */
#define REG_HSMCI_FIFO72        (*(__IO uint32_t*)0x40000320U) /**< (HSMCI) FIFO Memory Aperture0 72 */
#define REG_HSMCI_FIFO73        (*(__IO uint32_t*)0x40000324U) /**< (HSMCI) FIFO Memory Aperture0 73 */
#define REG_HSMCI_FIFO74        (*(__IO uint32_t*)0x40000328U) /**< (HSMCI) FIFO Memory Aperture0 74 */
#define REG_HSMCI_FIFO75        (*(__IO uint32_t*)0x4000032CU) /**< (HSMCI) FIFO Memory Aperture0 75 */
#define REG_HSMCI_FIFO76        (*(__IO uint32_t*)0x40000330U) /**< (HSMCI) FIFO Memory Aperture0 76 */
#define REG_HSMCI_FIFO77        (*(__IO uint32_t*)0x40000334U) /**< (HSMCI) FIFO Memory Aperture0 77 */
#define REG_HSMCI_FIFO78        (*(__IO uint32_t*)0x40000338U) /**< (HSMCI) FIFO Memory Aperture0 78 */
#define REG_HSMCI_FIFO79        (*(__IO uint32_t*)0x4000033CU) /**< (HSMCI) FIFO Memory Aperture0 79 */
#define REG_HSMCI_FIFO80        (*(__IO uint32_t*)0x40000340U) /**< (HSMCI) FIFO Memory Aperture0 80 */
#define REG_HSMCI_FIFO81        (*(__IO uint32_t*)0x40000344U) /**< (HSMCI) FIFO Memory Aperture0 81 */
#define REG_HSMCI_FIFO82        (*(__IO uint32_t*)0x40000348U) /**< (HSMCI) FIFO Memory Aperture0 82 */
#define REG_HSMCI_FIFO83        (*(__IO uint32_t*)0x4000034CU) /**< (HSMCI) FIFO Memory Aperture0 83 */
#define REG_HSMCI_FIFO84        (*(__IO uint32_t*)0x40000350U) /**< (HSMCI) FIFO Memory Aperture0 84 */
#define REG_HSMCI_FIFO85        (*(__IO uint32_t*)0x40000354U) /**< (HSMCI) FIFO Memory Aperture0 85 */
#define REG_HSMCI_FIFO86        (*(__IO uint32_t*)0x40000358U) /**< (HSMCI) FIFO Memory Aperture0 86 */
#define REG_HSMCI_FIFO87        (*(__IO uint32_t*)0x4000035CU) /**< (HSMCI) FIFO Memory Aperture0 87 */
#define REG_HSMCI_FIFO88        (*(__IO uint32_t*)0x40000360U) /**< (HSMCI) FIFO Memory Aperture0 88 */
#define REG_HSMCI_FIFO89        (*(__IO uint32_t*)0x40000364U) /**< (HSMCI) FIFO Memory Aperture0 89 */
#define REG_HSMCI_FIFO90        (*(__IO uint32_t*)0x40000368U) /**< (HSMCI) FIFO Memory Aperture0 90 */
#define REG_HSMCI_FIFO91        (*(__IO uint32_t*)0x4000036CU) /**< (HSMCI) FIFO Memory Aperture0 91 */
#define REG_HSMCI_FIFO92        (*(__IO uint32_t*)0x40000370U) /**< (HSMCI) FIFO Memory Aperture0 92 */
#define REG_HSMCI_FIFO93        (*(__IO uint32_t*)0x40000374U) /**< (HSMCI) FIFO Memory Aperture0 93 */
#define REG_HSMCI_FIFO94        (*(__IO uint32_t*)0x40000378U) /**< (HSMCI) FIFO Memory Aperture0 94 */
#define REG_HSMCI_FIFO95        (*(__IO uint32_t*)0x4000037CU) /**< (HSMCI) FIFO Memory Aperture0 95 */
#define REG_HSMCI_FIFO96        (*(__IO uint32_t*)0x40000380U) /**< (HSMCI) FIFO Memory Aperture0 96 */
#define REG_HSMCI_FIFO97        (*(__IO uint32_t*)0x40000384U) /**< (HSMCI) FIFO Memory Aperture0 97 */
#define REG_HSMCI_FIFO98        (*(__IO uint32_t*)0x40000388U) /**< (HSMCI) FIFO Memory Aperture0 98 */
#define REG_HSMCI_FIFO99        (*(__IO uint32_t*)0x4000038CU) /**< (HSMCI) FIFO Memory Aperture0 99 */
#define REG_HSMCI_FIFO100       (*(__IO uint32_t*)0x40000390U) /**< (HSMCI) FIFO Memory Aperture0 100 */
#define REG_HSMCI_FIFO101       (*(__IO uint32_t*)0x40000394U) /**< (HSMCI) FIFO Memory Aperture0 101 */
#define REG_HSMCI_FIFO102       (*(__IO uint32_t*)0x40000398U) /**< (HSMCI) FIFO Memory Aperture0 102 */
#define REG_HSMCI_FIFO103       (*(__IO uint32_t*)0x4000039CU) /**< (HSMCI) FIFO Memory Aperture0 103 */
#define REG_HSMCI_FIFO104       (*(__IO uint32_t*)0x400003A0U) /**< (HSMCI) FIFO Memory Aperture0 104 */
#define REG_HSMCI_FIFO105       (*(__IO uint32_t*)0x400003A4U) /**< (HSMCI) FIFO Memory Aperture0 105 */
#define REG_HSMCI_FIFO106       (*(__IO uint32_t*)0x400003A8U) /**< (HSMCI) FIFO Memory Aperture0 106 */
#define REG_HSMCI_FIFO107       (*(__IO uint32_t*)0x400003ACU) /**< (HSMCI) FIFO Memory Aperture0 107 */
#define REG_HSMCI_FIFO108       (*(__IO uint32_t*)0x400003B0U) /**< (HSMCI) FIFO Memory Aperture0 108 */
#define REG_HSMCI_FIFO109       (*(__IO uint32_t*)0x400003B4U) /**< (HSMCI) FIFO Memory Aperture0 109 */
#define REG_HSMCI_FIFO110       (*(__IO uint32_t*)0x400003B8U) /**< (HSMCI) FIFO Memory Aperture0 110 */
#define REG_HSMCI_FIFO111       (*(__IO uint32_t*)0x400003BCU) /**< (HSMCI) FIFO Memory Aperture0 111 */
#define REG_HSMCI_FIFO112       (*(__IO uint32_t*)0x400003C0U) /**< (HSMCI) FIFO Memory Aperture0 112 */
#define REG_HSMCI_FIFO113       (*(__IO uint32_t*)0x400003C4U) /**< (HSMCI) FIFO Memory Aperture0 113 */
#define REG_HSMCI_FIFO114       (*(__IO uint32_t*)0x400003C8U) /**< (HSMCI) FIFO Memory Aperture0 114 */
#define REG_HSMCI_FIFO115       (*(__IO uint32_t*)0x400003CCU) /**< (HSMCI) FIFO Memory Aperture0 115 */
#define REG_HSMCI_FIFO116       (*(__IO uint32_t*)0x400003D0U) /**< (HSMCI) FIFO Memory Aperture0 116 */
#define REG_HSMCI_FIFO117       (*(__IO uint32_t*)0x400003D4U) /**< (HSMCI) FIFO Memory Aperture0 117 */
#define REG_HSMCI_FIFO118       (*(__IO uint32_t*)0x400003D8U) /**< (HSMCI) FIFO Memory Aperture0 118 */
#define REG_HSMCI_FIFO119       (*(__IO uint32_t*)0x400003DCU) /**< (HSMCI) FIFO Memory Aperture0 119 */
#define REG_HSMCI_FIFO120       (*(__IO uint32_t*)0x400003E0U) /**< (HSMCI) FIFO Memory Aperture0 120 */
#define REG_HSMCI_FIFO121       (*(__IO uint32_t*)0x400003E4U) /**< (HSMCI) FIFO Memory Aperture0 121 */
#define REG_HSMCI_FIFO122       (*(__IO uint32_t*)0x400003E8U) /**< (HSMCI) FIFO Memory Aperture0 122 */
#define REG_HSMCI_FIFO123       (*(__IO uint32_t*)0x400003ECU) /**< (HSMCI) FIFO Memory Aperture0 123 */
#define REG_HSMCI_FIFO124       (*(__IO uint32_t*)0x400003F0U) /**< (HSMCI) FIFO Memory Aperture0 124 */
#define REG_HSMCI_FIFO125       (*(__IO uint32_t*)0x400003F4U) /**< (HSMCI) FIFO Memory Aperture0 125 */
#define REG_HSMCI_FIFO126       (*(__IO uint32_t*)0x400003F8U) /**< (HSMCI) FIFO Memory Aperture0 126 */
#define REG_HSMCI_FIFO127       (*(__IO uint32_t*)0x400003FCU) /**< (HSMCI) FIFO Memory Aperture0 127 */
#define REG_HSMCI_FIFO128       (*(__IO uint32_t*)0x40000400U) /**< (HSMCI) FIFO Memory Aperture0 128 */
#define REG_HSMCI_FIFO129       (*(__IO uint32_t*)0x40000404U) /**< (HSMCI) FIFO Memory Aperture0 129 */
#define REG_HSMCI_FIFO130       (*(__IO uint32_t*)0x40000408U) /**< (HSMCI) FIFO Memory Aperture0 130 */
#define REG_HSMCI_FIFO131       (*(__IO uint32_t*)0x4000040CU) /**< (HSMCI) FIFO Memory Aperture0 131 */
#define REG_HSMCI_FIFO132       (*(__IO uint32_t*)0x40000410U) /**< (HSMCI) FIFO Memory Aperture0 132 */
#define REG_HSMCI_FIFO133       (*(__IO uint32_t*)0x40000414U) /**< (HSMCI) FIFO Memory Aperture0 133 */
#define REG_HSMCI_FIFO134       (*(__IO uint32_t*)0x40000418U) /**< (HSMCI) FIFO Memory Aperture0 134 */
#define REG_HSMCI_FIFO135       (*(__IO uint32_t*)0x4000041CU) /**< (HSMCI) FIFO Memory Aperture0 135 */
#define REG_HSMCI_FIFO136       (*(__IO uint32_t*)0x40000420U) /**< (HSMCI) FIFO Memory Aperture0 136 */
#define REG_HSMCI_FIFO137       (*(__IO uint32_t*)0x40000424U) /**< (HSMCI) FIFO Memory Aperture0 137 */
#define REG_HSMCI_FIFO138       (*(__IO uint32_t*)0x40000428U) /**< (HSMCI) FIFO Memory Aperture0 138 */
#define REG_HSMCI_FIFO139       (*(__IO uint32_t*)0x4000042CU) /**< (HSMCI) FIFO Memory Aperture0 139 */
#define REG_HSMCI_FIFO140       (*(__IO uint32_t*)0x40000430U) /**< (HSMCI) FIFO Memory Aperture0 140 */
#define REG_HSMCI_FIFO141       (*(__IO uint32_t*)0x40000434U) /**< (HSMCI) FIFO Memory Aperture0 141 */
#define REG_HSMCI_FIFO142       (*(__IO uint32_t*)0x40000438U) /**< (HSMCI) FIFO Memory Aperture0 142 */
#define REG_HSMCI_FIFO143       (*(__IO uint32_t*)0x4000043CU) /**< (HSMCI) FIFO Memory Aperture0 143 */
#define REG_HSMCI_FIFO144       (*(__IO uint32_t*)0x40000440U) /**< (HSMCI) FIFO Memory Aperture0 144 */
#define REG_HSMCI_FIFO145       (*(__IO uint32_t*)0x40000444U) /**< (HSMCI) FIFO Memory Aperture0 145 */
#define REG_HSMCI_FIFO146       (*(__IO uint32_t*)0x40000448U) /**< (HSMCI) FIFO Memory Aperture0 146 */
#define REG_HSMCI_FIFO147       (*(__IO uint32_t*)0x4000044CU) /**< (HSMCI) FIFO Memory Aperture0 147 */
#define REG_HSMCI_FIFO148       (*(__IO uint32_t*)0x40000450U) /**< (HSMCI) FIFO Memory Aperture0 148 */
#define REG_HSMCI_FIFO149       (*(__IO uint32_t*)0x40000454U) /**< (HSMCI) FIFO Memory Aperture0 149 */
#define REG_HSMCI_FIFO150       (*(__IO uint32_t*)0x40000458U) /**< (HSMCI) FIFO Memory Aperture0 150 */
#define REG_HSMCI_FIFO151       (*(__IO uint32_t*)0x4000045CU) /**< (HSMCI) FIFO Memory Aperture0 151 */
#define REG_HSMCI_FIFO152       (*(__IO uint32_t*)0x40000460U) /**< (HSMCI) FIFO Memory Aperture0 152 */
#define REG_HSMCI_FIFO153       (*(__IO uint32_t*)0x40000464U) /**< (HSMCI) FIFO Memory Aperture0 153 */
#define REG_HSMCI_FIFO154       (*(__IO uint32_t*)0x40000468U) /**< (HSMCI) FIFO Memory Aperture0 154 */
#define REG_HSMCI_FIFO155       (*(__IO uint32_t*)0x4000046CU) /**< (HSMCI) FIFO Memory Aperture0 155 */
#define REG_HSMCI_FIFO156       (*(__IO uint32_t*)0x40000470U) /**< (HSMCI) FIFO Memory Aperture0 156 */
#define REG_HSMCI_FIFO157       (*(__IO uint32_t*)0x40000474U) /**< (HSMCI) FIFO Memory Aperture0 157 */
#define REG_HSMCI_FIFO158       (*(__IO uint32_t*)0x40000478U) /**< (HSMCI) FIFO Memory Aperture0 158 */
#define REG_HSMCI_FIFO159       (*(__IO uint32_t*)0x4000047CU) /**< (HSMCI) FIFO Memory Aperture0 159 */
#define REG_HSMCI_FIFO160       (*(__IO uint32_t*)0x40000480U) /**< (HSMCI) FIFO Memory Aperture0 160 */
#define REG_HSMCI_FIFO161       (*(__IO uint32_t*)0x40000484U) /**< (HSMCI) FIFO Memory Aperture0 161 */
#define REG_HSMCI_FIFO162       (*(__IO uint32_t*)0x40000488U) /**< (HSMCI) FIFO Memory Aperture0 162 */
#define REG_HSMCI_FIFO163       (*(__IO uint32_t*)0x4000048CU) /**< (HSMCI) FIFO Memory Aperture0 163 */
#define REG_HSMCI_FIFO164       (*(__IO uint32_t*)0x40000490U) /**< (HSMCI) FIFO Memory Aperture0 164 */
#define REG_HSMCI_FIFO165       (*(__IO uint32_t*)0x40000494U) /**< (HSMCI) FIFO Memory Aperture0 165 */
#define REG_HSMCI_FIFO166       (*(__IO uint32_t*)0x40000498U) /**< (HSMCI) FIFO Memory Aperture0 166 */
#define REG_HSMCI_FIFO167       (*(__IO uint32_t*)0x4000049CU) /**< (HSMCI) FIFO Memory Aperture0 167 */
#define REG_HSMCI_FIFO168       (*(__IO uint32_t*)0x400004A0U) /**< (HSMCI) FIFO Memory Aperture0 168 */
#define REG_HSMCI_FIFO169       (*(__IO uint32_t*)0x400004A4U) /**< (HSMCI) FIFO Memory Aperture0 169 */
#define REG_HSMCI_FIFO170       (*(__IO uint32_t*)0x400004A8U) /**< (HSMCI) FIFO Memory Aperture0 170 */
#define REG_HSMCI_FIFO171       (*(__IO uint32_t*)0x400004ACU) /**< (HSMCI) FIFO Memory Aperture0 171 */
#define REG_HSMCI_FIFO172       (*(__IO uint32_t*)0x400004B0U) /**< (HSMCI) FIFO Memory Aperture0 172 */
#define REG_HSMCI_FIFO173       (*(__IO uint32_t*)0x400004B4U) /**< (HSMCI) FIFO Memory Aperture0 173 */
#define REG_HSMCI_FIFO174       (*(__IO uint32_t*)0x400004B8U) /**< (HSMCI) FIFO Memory Aperture0 174 */
#define REG_HSMCI_FIFO175       (*(__IO uint32_t*)0x400004BCU) /**< (HSMCI) FIFO Memory Aperture0 175 */
#define REG_HSMCI_FIFO176       (*(__IO uint32_t*)0x400004C0U) /**< (HSMCI) FIFO Memory Aperture0 176 */
#define REG_HSMCI_FIFO177       (*(__IO uint32_t*)0x400004C4U) /**< (HSMCI) FIFO Memory Aperture0 177 */
#define REG_HSMCI_FIFO178       (*(__IO uint32_t*)0x400004C8U) /**< (HSMCI) FIFO Memory Aperture0 178 */
#define REG_HSMCI_FIFO179       (*(__IO uint32_t*)0x400004CCU) /**< (HSMCI) FIFO Memory Aperture0 179 */
#define REG_HSMCI_FIFO180       (*(__IO uint32_t*)0x400004D0U) /**< (HSMCI) FIFO Memory Aperture0 180 */
#define REG_HSMCI_FIFO181       (*(__IO uint32_t*)0x400004D4U) /**< (HSMCI) FIFO Memory Aperture0 181 */
#define REG_HSMCI_FIFO182       (*(__IO uint32_t*)0x400004D8U) /**< (HSMCI) FIFO Memory Aperture0 182 */
#define REG_HSMCI_FIFO183       (*(__IO uint32_t*)0x400004DCU) /**< (HSMCI) FIFO Memory Aperture0 183 */
#define REG_HSMCI_FIFO184       (*(__IO uint32_t*)0x400004E0U) /**< (HSMCI) FIFO Memory Aperture0 184 */
#define REG_HSMCI_FIFO185       (*(__IO uint32_t*)0x400004E4U) /**< (HSMCI) FIFO Memory Aperture0 185 */
#define REG_HSMCI_FIFO186       (*(__IO uint32_t*)0x400004E8U) /**< (HSMCI) FIFO Memory Aperture0 186 */
#define REG_HSMCI_FIFO187       (*(__IO uint32_t*)0x400004ECU) /**< (HSMCI) FIFO Memory Aperture0 187 */
#define REG_HSMCI_FIFO188       (*(__IO uint32_t*)0x400004F0U) /**< (HSMCI) FIFO Memory Aperture0 188 */
#define REG_HSMCI_FIFO189       (*(__IO uint32_t*)0x400004F4U) /**< (HSMCI) FIFO Memory Aperture0 189 */
#define REG_HSMCI_FIFO190       (*(__IO uint32_t*)0x400004F8U) /**< (HSMCI) FIFO Memory Aperture0 190 */
#define REG_HSMCI_FIFO191       (*(__IO uint32_t*)0x400004FCU) /**< (HSMCI) FIFO Memory Aperture0 191 */
#define REG_HSMCI_FIFO192       (*(__IO uint32_t*)0x40000500U) /**< (HSMCI) FIFO Memory Aperture0 192 */
#define REG_HSMCI_FIFO193       (*(__IO uint32_t*)0x40000504U) /**< (HSMCI) FIFO Memory Aperture0 193 */
#define REG_HSMCI_FIFO194       (*(__IO uint32_t*)0x40000508U) /**< (HSMCI) FIFO Memory Aperture0 194 */
#define REG_HSMCI_FIFO195       (*(__IO uint32_t*)0x4000050CU) /**< (HSMCI) FIFO Memory Aperture0 195 */
#define REG_HSMCI_FIFO196       (*(__IO uint32_t*)0x40000510U) /**< (HSMCI) FIFO Memory Aperture0 196 */
#define REG_HSMCI_FIFO197       (*(__IO uint32_t*)0x40000514U) /**< (HSMCI) FIFO Memory Aperture0 197 */
#define REG_HSMCI_FIFO198       (*(__IO uint32_t*)0x40000518U) /**< (HSMCI) FIFO Memory Aperture0 198 */
#define REG_HSMCI_FIFO199       (*(__IO uint32_t*)0x4000051CU) /**< (HSMCI) FIFO Memory Aperture0 199 */
#define REG_HSMCI_FIFO200       (*(__IO uint32_t*)0x40000520U) /**< (HSMCI) FIFO Memory Aperture0 200 */
#define REG_HSMCI_FIFO201       (*(__IO uint32_t*)0x40000524U) /**< (HSMCI) FIFO Memory Aperture0 201 */
#define REG_HSMCI_FIFO202       (*(__IO uint32_t*)0x40000528U) /**< (HSMCI) FIFO Memory Aperture0 202 */
#define REG_HSMCI_FIFO203       (*(__IO uint32_t*)0x4000052CU) /**< (HSMCI) FIFO Memory Aperture0 203 */
#define REG_HSMCI_FIFO204       (*(__IO uint32_t*)0x40000530U) /**< (HSMCI) FIFO Memory Aperture0 204 */
#define REG_HSMCI_FIFO205       (*(__IO uint32_t*)0x40000534U) /**< (HSMCI) FIFO Memory Aperture0 205 */
#define REG_HSMCI_FIFO206       (*(__IO uint32_t*)0x40000538U) /**< (HSMCI) FIFO Memory Aperture0 206 */
#define REG_HSMCI_FIFO207       (*(__IO uint32_t*)0x4000053CU) /**< (HSMCI) FIFO Memory Aperture0 207 */
#define REG_HSMCI_FIFO208       (*(__IO uint32_t*)0x40000540U) /**< (HSMCI) FIFO Memory Aperture0 208 */
#define REG_HSMCI_FIFO209       (*(__IO uint32_t*)0x40000544U) /**< (HSMCI) FIFO Memory Aperture0 209 */
#define REG_HSMCI_FIFO210       (*(__IO uint32_t*)0x40000548U) /**< (HSMCI) FIFO Memory Aperture0 210 */
#define REG_HSMCI_FIFO211       (*(__IO uint32_t*)0x4000054CU) /**< (HSMCI) FIFO Memory Aperture0 211 */
#define REG_HSMCI_FIFO212       (*(__IO uint32_t*)0x40000550U) /**< (HSMCI) FIFO Memory Aperture0 212 */
#define REG_HSMCI_FIFO213       (*(__IO uint32_t*)0x40000554U) /**< (HSMCI) FIFO Memory Aperture0 213 */
#define REG_HSMCI_FIFO214       (*(__IO uint32_t*)0x40000558U) /**< (HSMCI) FIFO Memory Aperture0 214 */
#define REG_HSMCI_FIFO215       (*(__IO uint32_t*)0x4000055CU) /**< (HSMCI) FIFO Memory Aperture0 215 */
#define REG_HSMCI_FIFO216       (*(__IO uint32_t*)0x40000560U) /**< (HSMCI) FIFO Memory Aperture0 216 */
#define REG_HSMCI_FIFO217       (*(__IO uint32_t*)0x40000564U) /**< (HSMCI) FIFO Memory Aperture0 217 */
#define REG_HSMCI_FIFO218       (*(__IO uint32_t*)0x40000568U) /**< (HSMCI) FIFO Memory Aperture0 218 */
#define REG_HSMCI_FIFO219       (*(__IO uint32_t*)0x4000056CU) /**< (HSMCI) FIFO Memory Aperture0 219 */
#define REG_HSMCI_FIFO220       (*(__IO uint32_t*)0x40000570U) /**< (HSMCI) FIFO Memory Aperture0 220 */
#define REG_HSMCI_FIFO221       (*(__IO uint32_t*)0x40000574U) /**< (HSMCI) FIFO Memory Aperture0 221 */
#define REG_HSMCI_FIFO222       (*(__IO uint32_t*)0x40000578U) /**< (HSMCI) FIFO Memory Aperture0 222 */
#define REG_HSMCI_FIFO223       (*(__IO uint32_t*)0x4000057CU) /**< (HSMCI) FIFO Memory Aperture0 223 */
#define REG_HSMCI_FIFO224       (*(__IO uint32_t*)0x40000580U) /**< (HSMCI) FIFO Memory Aperture0 224 */
#define REG_HSMCI_FIFO225       (*(__IO uint32_t*)0x40000584U) /**< (HSMCI) FIFO Memory Aperture0 225 */
#define REG_HSMCI_FIFO226       (*(__IO uint32_t*)0x40000588U) /**< (HSMCI) FIFO Memory Aperture0 226 */
#define REG_HSMCI_FIFO227       (*(__IO uint32_t*)0x4000058CU) /**< (HSMCI) FIFO Memory Aperture0 227 */
#define REG_HSMCI_FIFO228       (*(__IO uint32_t*)0x40000590U) /**< (HSMCI) FIFO Memory Aperture0 228 */
#define REG_HSMCI_FIFO229       (*(__IO uint32_t*)0x40000594U) /**< (HSMCI) FIFO Memory Aperture0 229 */
#define REG_HSMCI_FIFO230       (*(__IO uint32_t*)0x40000598U) /**< (HSMCI) FIFO Memory Aperture0 230 */
#define REG_HSMCI_FIFO231       (*(__IO uint32_t*)0x4000059CU) /**< (HSMCI) FIFO Memory Aperture0 231 */
#define REG_HSMCI_FIFO232       (*(__IO uint32_t*)0x400005A0U) /**< (HSMCI) FIFO Memory Aperture0 232 */
#define REG_HSMCI_FIFO233       (*(__IO uint32_t*)0x400005A4U) /**< (HSMCI) FIFO Memory Aperture0 233 */
#define REG_HSMCI_FIFO234       (*(__IO uint32_t*)0x400005A8U) /**< (HSMCI) FIFO Memory Aperture0 234 */
#define REG_HSMCI_FIFO235       (*(__IO uint32_t*)0x400005ACU) /**< (HSMCI) FIFO Memory Aperture0 235 */
#define REG_HSMCI_FIFO236       (*(__IO uint32_t*)0x400005B0U) /**< (HSMCI) FIFO Memory Aperture0 236 */
#define REG_HSMCI_FIFO237       (*(__IO uint32_t*)0x400005B4U) /**< (HSMCI) FIFO Memory Aperture0 237 */
#define REG_HSMCI_FIFO238       (*(__IO uint32_t*)0x400005B8U) /**< (HSMCI) FIFO Memory Aperture0 238 */
#define REG_HSMCI_FIFO239       (*(__IO uint32_t*)0x400005BCU) /**< (HSMCI) FIFO Memory Aperture0 239 */
#define REG_HSMCI_FIFO240       (*(__IO uint32_t*)0x400005C0U) /**< (HSMCI) FIFO Memory Aperture0 240 */
#define REG_HSMCI_FIFO241       (*(__IO uint32_t*)0x400005C4U) /**< (HSMCI) FIFO Memory Aperture0 241 */
#define REG_HSMCI_FIFO242       (*(__IO uint32_t*)0x400005C8U) /**< (HSMCI) FIFO Memory Aperture0 242 */
#define REG_HSMCI_FIFO243       (*(__IO uint32_t*)0x400005CCU) /**< (HSMCI) FIFO Memory Aperture0 243 */
#define REG_HSMCI_FIFO244       (*(__IO uint32_t*)0x400005D0U) /**< (HSMCI) FIFO Memory Aperture0 244 */
#define REG_HSMCI_FIFO245       (*(__IO uint32_t*)0x400005D4U) /**< (HSMCI) FIFO Memory Aperture0 245 */
#define REG_HSMCI_FIFO246       (*(__IO uint32_t*)0x400005D8U) /**< (HSMCI) FIFO Memory Aperture0 246 */
#define REG_HSMCI_FIFO247       (*(__IO uint32_t*)0x400005DCU) /**< (HSMCI) FIFO Memory Aperture0 247 */
#define REG_HSMCI_FIFO248       (*(__IO uint32_t*)0x400005E0U) /**< (HSMCI) FIFO Memory Aperture0 248 */
#define REG_HSMCI_FIFO249       (*(__IO uint32_t*)0x400005E4U) /**< (HSMCI) FIFO Memory Aperture0 249 */
#define REG_HSMCI_FIFO250       (*(__IO uint32_t*)0x400005E8U) /**< (HSMCI) FIFO Memory Aperture0 250 */
#define REG_HSMCI_FIFO251       (*(__IO uint32_t*)0x400005ECU) /**< (HSMCI) FIFO Memory Aperture0 251 */
#define REG_HSMCI_FIFO252       (*(__IO uint32_t*)0x400005F0U) /**< (HSMCI) FIFO Memory Aperture0 252 */
#define REG_HSMCI_FIFO253       (*(__IO uint32_t*)0x400005F4U) /**< (HSMCI) FIFO Memory Aperture0 253 */
#define REG_HSMCI_FIFO254       (*(__IO uint32_t*)0x400005F8U) /**< (HSMCI) FIFO Memory Aperture0 254 */
#define REG_HSMCI_FIFO255       (*(__IO uint32_t*)0x400005FCU) /**< (HSMCI) FIFO Memory Aperture0 255 */

#endif /* (defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
/* ========== Instance Parameter definitions for HSMCI peripheral ========== */
#define HSMCI_INSTANCE_ID                        18        
#define HSMCI_DMAC_ID_TX                         0         
#define HSMCI_DMAC_ID_RX                         0         

#endif /* _SAME70_HSMCI_INSTANCE_ */
