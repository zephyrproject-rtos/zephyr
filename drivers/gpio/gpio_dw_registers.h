/*gpio_dw_registers.h - Private gpio's registers header*/

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef CONFIG_PLATFORM_QUARK_SE_SS
#define SWPORTA_DR     0x00
#define SWPORTA_DDR    0x01
#define INTEN          0x03
#define INTMASK                0x04
#define INTTYPE_LEVEL  0x05
#define INT_POLARITY   0x06
#define INTSTATUS      0x07
#define PORTA_DEBOUNCE 0x08
#define PORTA_EOI      0x09
#define EXT_PORTA      0x0A
#define INT_CLOCK_SYNC 0x0B

#define CLK_ENA_POS	(31)

#else
#define SWPORTA_DR     0x00
#define SWPORTA_DDR    0x04
#define SWPORTB_DR     0x0c
#define SWPORTB_DDR    0x10
#define SWPORTC_DR     0x18
#define SWPORTC_DDR    0x1c
#define SWPORTD_DR     0x24
#define SWPORTD_DDR    0x28
#define INTEN          0x30
#define INTMASK                0x34
#define INTTYPE_LEVEL  0x38
#define INT_POLARITY   0x3c
#define INTSTATUS      0x40
#define PORTA_DEBOUNCE 0x48
#define PORTA_EOI      0x4c
#define EXT_PORTA      0x50
#define EXT_PORTB      0x54
#define EXT_PORTC      0x58
#define EXT_PORTD      0x5c
#define INT_CLOCK_SYNC 0x60
#define INT_BOTHEDGE   0x68
#endif

#define LS_SYNC_POS	(0)
