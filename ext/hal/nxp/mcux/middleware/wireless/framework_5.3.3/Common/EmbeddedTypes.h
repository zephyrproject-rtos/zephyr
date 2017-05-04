/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
*
* This file holds type definitions that maps the standard c-types into types
* with guaranteed sizes. The types are target/platform specific and must be edited
* for each new target/platform.
* The header file also provides definitions for TRUE, FALSE and NULL.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _EMBEDDEDTYPES_H_
#define _EMBEDDEDTYPES_H_


/************************************************************************************
*
*       INCLUDES
*
************************************************************************************/

#include <stdint.h>


/************************************************************************************
*
*       TYPE DEFINITIONS
*
************************************************************************************/

/* boolean types */
typedef uint8_t   bool_t;

typedef uint8_t   index_t;

/* TRUE/FALSE definition*/
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* null pointer definition*/
#ifndef NULL
#define NULL (( void * )( 0x0UL ))
#endif

#if defined(__GNUC__)
#define PACKED_STRUCT struct __attribute__ ((__packed__))
#define PACKED_UNION  union __attribute__ ((__packed__))
#elif defined(__IAR_SYSTEMS_ICC__)
#define PACKED_STRUCT __packed struct
#define PACKED_UNION __packed union
#else
#define PACKED_STRUCT struct
#define PACKED_UNION union
#endif

typedef unsigned char uintn8_t;
typedef unsigned long uintn32_t;

typedef unsigned char uchar_t;

#if !defined(MIN)
#define MIN(a,b)                    (((a) < (b))?(a):(b))
#endif

#if !defined(MAX)
#define MAX(a,b)                    (((a) > (b))?(a):(b))
#endif 

/* Compute the number of elements of an array */
#define NumberOfElements(x) (sizeof(x)/sizeof((x)[0]))

/* Compute the size of a string initialized with quotation marks */
#define SizeOfString(string) (sizeof(string) - 1)

#define GetRelAddr(strct, member) ((uint32_t)&(((strct*)(void *)0)->member))
#define GetSizeOfMember(strct, member) sizeof(((strct*)(void *)0)->member)

/* Type definitions for link configuration of instantiable layers  */
#define gInvalidInstanceId_c (instanceId_t)(-1)
typedef uint32_t instanceId_t;

/* Bit shift definitions */
#define BIT0              0x01
#define BIT1              0x02
#define BIT2              0x04
#define BIT3              0x08
#define BIT4              0x10
#define BIT5              0x20
#define BIT6              0x40
#define BIT7              0x80
#define BIT8             0x100
#define BIT9             0x200
#define BIT10            0x400
#define BIT11            0x800
#define BIT12           0x1000
#define BIT13           0x2000
#define BIT14           0x4000
#define BIT15           0x8000
#define BIT16          0x10000
#define BIT17          0x20000
#define BIT18          0x40000
#define BIT19          0x80000
#define BIT20         0x100000
#define BIT21         0x200000
#define BIT22         0x400000
#define BIT23         0x800000
#define BIT24        0x1000000
#define BIT25        0x2000000
#define BIT26        0x4000000
#define BIT27        0x8000000
#define BIT28       0x10000000
#define BIT29       0x20000000
#define BIT30       0x40000000
#define BIT31       0x80000000

/* Shift definitions */
#define SHIFT0      (0)
#define SHIFT1      (1)
#define SHIFT2      (2)
#define SHIFT3      (3)
#define SHIFT4      (4)
#define SHIFT5      (5)
#define SHIFT6      (6)
#define SHIFT7      (7)
#define SHIFT8      (8)
#define SHIFT9      (9)
#define SHIFT10     (10)
#define SHIFT11     (11)
#define SHIFT12     (12)
#define SHIFT13     (13)
#define SHIFT14     (14)
#define SHIFT15     (15)
#define SHIFT16     (16)
#define SHIFT17     (17)
#define SHIFT18     (18)
#define SHIFT19     (19)
#define SHIFT20     (20)
#define SHIFT21     (21)
#define SHIFT22     (22)
#define SHIFT23     (23)
#define SHIFT24     (24)
#define SHIFT25     (25)
#define SHIFT26     (26)
#define SHIFT27     (27)
#define SHIFT28     (28)
#define SHIFT29     (29)
#define SHIFT30     (30)
#define SHIFT31     (31)

#define SHIFT32     (32)
#define SHIFT33     (33)
#define SHIFT34     (34)
#define SHIFT35     (35)
#define SHIFT36     (36)
#define SHIFT37     (37)
#define SHIFT38     (38)
#define SHIFT39     (39)
#define SHIFT40     (40)
#define SHIFT41     (41)
#define SHIFT42     (42)
#define SHIFT43     (43)
#define SHIFT44     (44)
#define SHIFT45     (45)
#define SHIFT46     (46)
#define SHIFT47     (47)
#define SHIFT48     (48)
#define SHIFT49     (49)
#define SHIFT50     (50)
#define SHIFT51     (51)
#define SHIFT52     (52)
#define SHIFT53     (53)
#define SHIFT54     (54)
#define SHIFT55     (55)
#define SHIFT56     (56)
#define SHIFT57     (57)
#define SHIFT58     (58)
#define SHIFT59     (59)
#define SHIFT60     (60)
#define SHIFT61     (61)
#define SHIFT62     (62)
#define SHIFT63     (63)


#endif /* _EMBEDDEDTYPES_H_ */
