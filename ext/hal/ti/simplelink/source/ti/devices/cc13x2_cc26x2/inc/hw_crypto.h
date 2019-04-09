/******************************************************************************
*  Filename:       hw_crypto_h
*  Revised:        2018-05-14 12:24:52 +0200 (Mon, 14 May 2018)
*  Revision:       51990
*
* Copyright (c) 2015 - 2017, Texas Instruments Incorporated
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1) Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2) Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3) Neither the name of the ORGANIZATION nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef __HW_CRYPTO_H__
#define __HW_CRYPTO_H__

//*****************************************************************************
//
// This section defines the register offsets of
// CRYPTO component
//
//*****************************************************************************
// Channel 0 Control
#define CRYPTO_O_DMACH0CTL                                          0x00000000

// Channel 0 External Address
#define CRYPTO_O_DMACH0EXTADDR                                      0x00000004

// Channel 0 DMA Length
#define CRYPTO_O_DMACH0LEN                                          0x0000000C

// DMAC Status
#define CRYPTO_O_DMASTAT                                            0x00000018

// DMAC Software Reset
#define CRYPTO_O_DMASWRESET                                         0x0000001C

// Channel 1 Control
#define CRYPTO_O_DMACH1CTL                                          0x00000020

// Channel 1 External Address
#define CRYPTO_O_DMACH1EXTADDR                                      0x00000024

// Channel 1 DMA Length
#define CRYPTO_O_DMACH1LEN                                          0x0000002C

// DMAC Master Run-time Parameters
#define CRYPTO_O_DMABUSCFG                                          0x00000078

// DMAC Port Error Raw Status
#define CRYPTO_O_DMAPORTERR                                         0x0000007C

// DMAC Version
#define CRYPTO_O_DMAHWVER                                           0x000000FC

// Key Store Write Area
#define CRYPTO_O_KEYWRITEAREA                                       0x00000400

// Key Store Written Area
#define CRYPTO_O_KEYWRITTENAREA                                     0x00000404

// Key Store Size
#define CRYPTO_O_KEYSIZE                                            0x00000408

// Key Store Read Area
#define CRYPTO_O_KEYREADAREA                                        0x0000040C

// AES_KEY2_0 / AES_GHASH_H_IN_0
#define CRYPTO_O_AESKEY20                                           0x00000500

// AES_KEY2_0 / AES_GHASH_H_IN_0
#define CRYPTO_O_AESKEY21                                           0x00000504

// AES_KEY2_0 / AES_GHASH_H_IN_0
#define CRYPTO_O_AESKEY22                                           0x00000508

// AES_KEY2_0 / AES_GHASH_H_IN_0
#define CRYPTO_O_AESKEY23                                           0x0000050C

// AES_KEY3_0 / AES_KEY2_4
#define CRYPTO_O_AESKEY30                                           0x00000510

// AES_KEY3_0 / AES_KEY2_4
#define CRYPTO_O_AESKEY31                                           0x00000514

// AES_KEY3_0 / AES_KEY2_4
#define CRYPTO_O_AESKEY32                                           0x00000518

// AES_KEY3_0 / AES_KEY2_4
#define CRYPTO_O_AESKEY33                                           0x0000051C

// AES initialization vector registers
#define CRYPTO_O_AESIV0                                             0x00000540

// AES initialization vector registers
#define CRYPTO_O_AESIV1                                             0x00000544

// AES initialization vector registers
#define CRYPTO_O_AESIV2                                             0x00000548

// AES initialization vector registers
#define CRYPTO_O_AESIV3                                             0x0000054C

// AES Control
#define CRYPTO_O_AESCTL                                             0x00000550

// AES Crypto Length 0 (LSW)
#define CRYPTO_O_AESDATALEN0                                        0x00000554

// AES Crypto Length 1 (MSW)
#define CRYPTO_O_AESDATALEN1                                        0x00000558

// AES Authentication Length
#define CRYPTO_O_AESAUTHLEN                                         0x0000055C

// Data Input/Output
#define CRYPTO_O_AESDATAOUT0                                        0x00000560

// AES Data Input_Output 0
#define CRYPTO_O_AESDATAIN0                                         0x00000560

// Data Input/Output
#define CRYPTO_O_AESDATAOUT1                                        0x00000564

// AES Data Input_Output 0
#define CRYPTO_O_AESDATAIN1                                         0x00000564

// Data Input/Output
#define CRYPTO_O_AESDATAOUT2                                        0x00000568

// AES Data Input_Output 2
#define CRYPTO_O_AESDATAIN2                                         0x00000568

// Data Input/Output
#define CRYPTO_O_AESDATAOUT3                                        0x0000056C

// AES Data Input_Output 3
#define CRYPTO_O_AESDATAIN3                                         0x0000056C

// AES Tag Out 0
#define CRYPTO_O_AESTAGOUT0                                         0x00000570

// AES Tag Out 0
#define CRYPTO_O_AESTAGOUT1                                         0x00000574

// AES Tag Out 0
#define CRYPTO_O_AESTAGOUT2                                         0x00000578

// AES Tag Out 0
#define CRYPTO_O_AESTAGOUT3                                         0x0000057C

// HASH Data Input 1
#define CRYPTO_O_HASHDATAIN1                                        0x00000604

// HASH Data Input 2
#define CRYPTO_O_HASHDATAIN2                                        0x00000608

// HASH Data Input 3
#define CRYPTO_O_HASHDATAIN3                                        0x0000060C

// HASH Data Input 4
#define CRYPTO_O_HASHDATAIN4                                        0x00000610

// HASH Data Input 5
#define CRYPTO_O_HASHDATAIN5                                        0x00000614

// HASH Data Input 6
#define CRYPTO_O_HASHDATAIN6                                        0x00000618

// HASH Data Input 7
#define CRYPTO_O_HASHDATAIN7                                        0x0000061C

// HASH Data Input 8
#define CRYPTO_O_HASHDATAIN8                                        0x00000620

// HASH Data Input 9
#define CRYPTO_O_HASHDATAIN9                                        0x00000624

// HASH Data Input 10
#define CRYPTO_O_HASHDATAIN10                                       0x00000628

// HASH Data Input 11
#define CRYPTO_O_HASHDATAIN11                                       0x0000062C

// HASH Data Input 12
#define CRYPTO_O_HASHDATAIN12                                       0x00000630

// HASH Data Input 13
#define CRYPTO_O_HASHDATAIN13                                       0x00000634

// HASH Data Input 14
#define CRYPTO_O_HASHDATAIN14                                       0x00000638

// HASH Data Input 15
#define CRYPTO_O_HASHDATAIN15                                       0x0000063C

// HASH Data Input 16
#define CRYPTO_O_HASHDATAIN16                                       0x00000640

// HASH Data Input 17
#define CRYPTO_O_HASHDATAIN17                                       0x00000644

// HASH Data Input 18
#define CRYPTO_O_HASHDATAIN18                                       0x00000648

// HASH Data Input 19
#define CRYPTO_O_HASHDATAIN19                                       0x0000064C

// HASH Data Input 20
#define CRYPTO_O_HASHDATAIN20                                       0x00000650

// HASH Data Input 21
#define CRYPTO_O_HASHDATAIN21                                       0x00000654

// HASH Data Input 22
#define CRYPTO_O_HASHDATAIN22                                       0x00000658

// HASH Data Input 23
#define CRYPTO_O_HASHDATAIN23                                       0x0000065C

// HASH Data Input 24
#define CRYPTO_O_HASHDATAIN24                                       0x00000660

// HASH Data Input 25
#define CRYPTO_O_HASHDATAIN25                                       0x00000664

// HASH Data Input 26
#define CRYPTO_O_HASHDATAIN26                                       0x00000668

// HASH Data Input 27
#define CRYPTO_O_HASHDATAIN27                                       0x0000066C

// HASH Data Input 28
#define CRYPTO_O_HASHDATAIN28                                       0x00000670

// HASH Data Input 29
#define CRYPTO_O_HASHDATAIN29                                       0x00000674

// HASH Data Input 30
#define CRYPTO_O_HASHDATAIN30                                       0x00000678

// HASH Data Input 31
#define CRYPTO_O_HASHDATAIN31                                       0x0000067C

// HASH Input_Output Buffer Control
#define CRYPTO_O_HASHIOBUFCTRL                                      0x00000680

// HASH Mode
#define CRYPTO_O_HASHMODE                                           0x00000684

// HASH Input Length LSB
#define CRYPTO_O_HASHINLENL                                         0x00000688

// HASH Input Length MSB
#define CRYPTO_O_HASHINLENH                                         0x0000068C

// HASH Digest A
#define CRYPTO_O_HASHDIGESTA                                        0x000006C0

// HASH Digest B
#define CRYPTO_O_HASHDIGESTB                                        0x000006C4

// HASH Digest C
#define CRYPTO_O_HASHDIGESTC                                        0x000006C8

// HASH Digest D
#define CRYPTO_O_HASHDIGESTD                                        0x000006CC

// HASH Digest E
#define CRYPTO_O_HASHDIGESTE                                        0x000006D0

// HASH Digest F
#define CRYPTO_O_HASHDIGESTF                                        0x000006D4

// HASH Digest G
#define CRYPTO_O_HASHDIGESTG                                        0x000006D8

// HASH Digest H
#define CRYPTO_O_HASHDIGESTH                                        0x000006DC

// HASH Digest I
#define CRYPTO_O_HASHDIGESTI                                        0x000006E0

// HASH Digest J
#define CRYPTO_O_HASHDIGESTJ                                        0x000006E4

// HASH Digest K
#define CRYPTO_O_HASHDIGESTK                                        0x000006E8

// HASH Digest L
#define CRYPTO_O_HASHDIGESTL                                        0x000006EC

// HASH Digest M
#define CRYPTO_O_HASHDIGESTM                                        0x000006F0

// HASH Digest N
#define CRYPTO_O_HASHDIGESTN                                        0x000006F4

// HASH Digest 0
#define CRYPTO_O_HASHDIGESTO                                        0x000006F8

// HASH Digest P
#define CRYPTO_O_HASHDIGESTP                                        0x000006FC

// Algorithm Select
#define CRYPTO_O_ALGSEL                                             0x00000700

// DMA Protection Control
#define CRYPTO_O_DMAPROTCTL                                         0x00000704

// Software Reset
#define CRYPTO_O_SWRESET                                            0x00000740

// Control Interrupt Configuration
#define CRYPTO_O_IRQTYPE                                            0x00000780

// Control Interrupt Enable
#define CRYPTO_O_IRQEN                                              0x00000784

// Control Interrupt Clear
#define CRYPTO_O_IRQCLR                                             0x00000788

// Control Interrupt Set
#define CRYPTO_O_IRQSET                                             0x0000078C

// Control Interrupt Status
#define CRYPTO_O_IRQSTAT                                            0x00000790

// Hardware Version
#define CRYPTO_O_HWVER                                              0x000007FC

//*****************************************************************************
//
// Register: CRYPTO_O_DMACH0CTL
//
//*****************************************************************************
// Field:     [1] PRIO
//
// Channel priority
// 0: Low
// 1: High
// If both channels have the same priority, access of the channels to the
// external port is arbitrated using the round robin scheme. If one channel has
// a high priority and another one low, the channel with the high priority is
// served first, in case of simultaneous access requests.
#define CRYPTO_DMACH0CTL_PRIO                                       0x00000002
#define CRYPTO_DMACH0CTL_PRIO_BITN                                           1
#define CRYPTO_DMACH0CTL_PRIO_M                                     0x00000002
#define CRYPTO_DMACH0CTL_PRIO_S                                              1

// Field:     [0] EN
//
// Channel enable
// 0: Disabled
// 1: Enable
// Note: Disabling an active channel interrupts the DMA operation. The ongoing
// block transfer completes, but no new transfers are requested.
#define CRYPTO_DMACH0CTL_EN                                         0x00000001
#define CRYPTO_DMACH0CTL_EN_BITN                                             0
#define CRYPTO_DMACH0CTL_EN_M                                       0x00000001
#define CRYPTO_DMACH0CTL_EN_S                                                0

//*****************************************************************************
//
// Register: CRYPTO_O_DMACH0EXTADDR
//
//*****************************************************************************
// Field:  [31:0] ADDR
//
// Channel external address value
// When read during operation, it holds the last updated external address after
// being sent to the master interface.  Note: The crypto DMA copies out upto 3
// bytes until it hits a word boundary, thus the address need not be word
// aligned.
#define CRYPTO_DMACH0EXTADDR_ADDR_W                                         32
#define CRYPTO_DMACH0EXTADDR_ADDR_M                                 0xFFFFFFFF
#define CRYPTO_DMACH0EXTADDR_ADDR_S                                          0

//*****************************************************************************
//
// Register: CRYPTO_O_DMACH0LEN
//
//*****************************************************************************
// Field:  [15:0] DMALEN
//
// Channel DMA length in bytes
// During configuration, this register contains the DMA transfer length in
// bytes. During operation, it contains the last updated value of the DMA
// transfer length after being sent to the master interface.
// Note: Setting this register to a nonzero value starts the transfer if the
// channel is enabled. Therefore, this register must be written last when
// setting up a DMA channel.
#define CRYPTO_DMACH0LEN_DMALEN_W                                           16
#define CRYPTO_DMACH0LEN_DMALEN_M                                   0x0000FFFF
#define CRYPTO_DMACH0LEN_DMALEN_S                                            0

//*****************************************************************************
//
// Register: CRYPTO_O_DMASTAT
//
//*****************************************************************************
// Field:    [17] PORT_ERR
//
// Reflects possible transfer errors on the AHB port.
#define CRYPTO_DMASTAT_PORT_ERR                                     0x00020000
#define CRYPTO_DMASTAT_PORT_ERR_BITN                                        17
#define CRYPTO_DMASTAT_PORT_ERR_M                                   0x00020000
#define CRYPTO_DMASTAT_PORT_ERR_S                                           17

// Field:     [1] CH1_ACT
//
// A value of 1 indicates that channel 1 is active (DMA transfer on-going).
#define CRYPTO_DMASTAT_CH1_ACT                                      0x00000002
#define CRYPTO_DMASTAT_CH1_ACT_BITN                                          1
#define CRYPTO_DMASTAT_CH1_ACT_M                                    0x00000002
#define CRYPTO_DMASTAT_CH1_ACT_S                                             1

// Field:     [0] CH0_ACT
//
// A value of 1 indicates that channel 0 is active (DMA transfer on-going).
#define CRYPTO_DMASTAT_CH0_ACT                                      0x00000001
#define CRYPTO_DMASTAT_CH0_ACT_BITN                                          0
#define CRYPTO_DMASTAT_CH0_ACT_M                                    0x00000001
#define CRYPTO_DMASTAT_CH0_ACT_S                                             0

//*****************************************************************************
//
// Register: CRYPTO_O_DMASWRESET
//
//*****************************************************************************
// Field:     [0] SWRES
//
// Software reset enable
// 0 : Disabled
// 1 :  Enabled (self-cleared to 0)
// Completion of the software reset must be checked through the DMASTAT
#define CRYPTO_DMASWRESET_SWRES                                     0x00000001
#define CRYPTO_DMASWRESET_SWRES_BITN                                         0
#define CRYPTO_DMASWRESET_SWRES_M                                   0x00000001
#define CRYPTO_DMASWRESET_SWRES_S                                            0

//*****************************************************************************
//
// Register: CRYPTO_O_DMACH1CTL
//
//*****************************************************************************
// Field:     [1] PRIO
//
// Channel priority
// 0: Low
// 1: High
// If both channels have the same priority, access of the channels to the
// external port is arbitrated using the round robin scheme. If one channel has
// a high priority and another one low, the channel with the high priority is
// served first, in case of simultaneous access requests.
#define CRYPTO_DMACH1CTL_PRIO                                       0x00000002
#define CRYPTO_DMACH1CTL_PRIO_BITN                                           1
#define CRYPTO_DMACH1CTL_PRIO_M                                     0x00000002
#define CRYPTO_DMACH1CTL_PRIO_S                                              1

// Field:     [0] EN
//
// Channel enable
// 0: Disabled
// 1: Enable
// Note: Disabling an active channel interrupts the DMA operation. The ongoing
// block transfer completes, but no new transfers are requested.
#define CRYPTO_DMACH1CTL_EN                                         0x00000001
#define CRYPTO_DMACH1CTL_EN_BITN                                             0
#define CRYPTO_DMACH1CTL_EN_M                                       0x00000001
#define CRYPTO_DMACH1CTL_EN_S                                                0

//*****************************************************************************
//
// Register: CRYPTO_O_DMACH1EXTADDR
//
//*****************************************************************************
// Field:  [31:0] ADDR
//
// Channel external address value.
// When read during operation, it holds the last updated external address after
// being sent to the master interface.   Note: The crypto DMA copies out upto 3
// bytes until it hits a word boundary, thus the address need not be word
// aligned.
#define CRYPTO_DMACH1EXTADDR_ADDR_W                                         32
#define CRYPTO_DMACH1EXTADDR_ADDR_M                                 0xFFFFFFFF
#define CRYPTO_DMACH1EXTADDR_ADDR_S                                          0

//*****************************************************************************
//
// Register: CRYPTO_O_DMACH1LEN
//
//*****************************************************************************
// Field:  [15:0] DMALEN
//
// Channel DMA length in bytes.
// During configuration, this register contains the DMA transfer length in
// bytes. During operation, it contains the last updated value of the DMA
// transfer length after being sent to the master interface.
// Note: Setting this register to a nonzero value starts the transfer if the
// channel is enabled. Therefore, this register must be written last when
// setting up a DMA channel.
#define CRYPTO_DMACH1LEN_DMALEN_W                                           16
#define CRYPTO_DMACH1LEN_DMALEN_M                                   0x0000FFFF
#define CRYPTO_DMACH1LEN_DMALEN_S                                            0

//*****************************************************************************
//
// Register: CRYPTO_O_DMABUSCFG
//
//*****************************************************************************
// Field: [15:12] AHB_MST1_BURST_SIZE
//
// Maximum burst size that can be performed on the AHB bus
// ENUMs:
// 64_BYTE                  64 bytes
// 32_BYTE                  32 bytes
// 16_BYTE                  16 bytes
// 8_BYTE                   8 bytes
// 4_BYTE                   4 bytes
#define CRYPTO_DMABUSCFG_AHB_MST1_BURST_SIZE_W                               4
#define CRYPTO_DMABUSCFG_AHB_MST1_BURST_SIZE_M                      0x0000F000
#define CRYPTO_DMABUSCFG_AHB_MST1_BURST_SIZE_S                              12
#define CRYPTO_DMABUSCFG_AHB_MST1_BURST_SIZE_64_BYTE                0x00006000
#define CRYPTO_DMABUSCFG_AHB_MST1_BURST_SIZE_32_BYTE                0x00005000
#define CRYPTO_DMABUSCFG_AHB_MST1_BURST_SIZE_16_BYTE                0x00004000
#define CRYPTO_DMABUSCFG_AHB_MST1_BURST_SIZE_8_BYTE                 0x00003000
#define CRYPTO_DMABUSCFG_AHB_MST1_BURST_SIZE_4_BYTE                 0x00002000

// Field:    [11] AHB_MST1_IDLE_EN
//
// Idle insertion between consecutive burst transfers on AHB
// ENUMs:
// IDLE                     Idle transfer insertion enabled
// NO_IDLE                  Do not insert idle transfers.
#define CRYPTO_DMABUSCFG_AHB_MST1_IDLE_EN                           0x00000800
#define CRYPTO_DMABUSCFG_AHB_MST1_IDLE_EN_BITN                              11
#define CRYPTO_DMABUSCFG_AHB_MST1_IDLE_EN_M                         0x00000800
#define CRYPTO_DMABUSCFG_AHB_MST1_IDLE_EN_S                                 11
#define CRYPTO_DMABUSCFG_AHB_MST1_IDLE_EN_IDLE                      0x00000800
#define CRYPTO_DMABUSCFG_AHB_MST1_IDLE_EN_NO_IDLE                   0x00000000

// Field:    [10] AHB_MST1_INCR_EN
//
// Burst length type of AHB transfer
// ENUMs:
// SPECIFIED                Fixed length bursts or single transfers
// UNSPECIFIED              Unspecified length burst transfers
#define CRYPTO_DMABUSCFG_AHB_MST1_INCR_EN                           0x00000400
#define CRYPTO_DMABUSCFG_AHB_MST1_INCR_EN_BITN                              10
#define CRYPTO_DMABUSCFG_AHB_MST1_INCR_EN_M                         0x00000400
#define CRYPTO_DMABUSCFG_AHB_MST1_INCR_EN_S                                 10
#define CRYPTO_DMABUSCFG_AHB_MST1_INCR_EN_SPECIFIED                 0x00000400
#define CRYPTO_DMABUSCFG_AHB_MST1_INCR_EN_UNSPECIFIED               0x00000000

// Field:     [9] AHB_MST1_LOCK_EN
//
// Locked transform on AHB
// ENUMs:
// LOCKED                   Transfers are locked
// NOT_LOCKED               Transfers are not locked
#define CRYPTO_DMABUSCFG_AHB_MST1_LOCK_EN                           0x00000200
#define CRYPTO_DMABUSCFG_AHB_MST1_LOCK_EN_BITN                               9
#define CRYPTO_DMABUSCFG_AHB_MST1_LOCK_EN_M                         0x00000200
#define CRYPTO_DMABUSCFG_AHB_MST1_LOCK_EN_S                                  9
#define CRYPTO_DMABUSCFG_AHB_MST1_LOCK_EN_LOCKED                    0x00000200
#define CRYPTO_DMABUSCFG_AHB_MST1_LOCK_EN_NOT_LOCKED                0x00000000

// Field:     [8] AHB_MST1_BIGEND
//
// Endianess for the AHB master
// ENUMs:
// BIG_ENDIAN               Big Endian
// LITTLE_ENDIAN            Little Endian
#define CRYPTO_DMABUSCFG_AHB_MST1_BIGEND                            0x00000100
#define CRYPTO_DMABUSCFG_AHB_MST1_BIGEND_BITN                                8
#define CRYPTO_DMABUSCFG_AHB_MST1_BIGEND_M                          0x00000100
#define CRYPTO_DMABUSCFG_AHB_MST1_BIGEND_S                                   8
#define CRYPTO_DMABUSCFG_AHB_MST1_BIGEND_BIG_ENDIAN                 0x00000100
#define CRYPTO_DMABUSCFG_AHB_MST1_BIGEND_LITTLE_ENDIAN              0x00000000

//*****************************************************************************
//
// Register: CRYPTO_O_DMAPORTERR
//
//*****************************************************************************
// Field:    [12] PORT1_AHB_ERROR
//
// A value of 1 indicates that the EIP-101 has detected an AHB bus error
#define CRYPTO_DMAPORTERR_PORT1_AHB_ERROR                           0x00001000
#define CRYPTO_DMAPORTERR_PORT1_AHB_ERROR_BITN                              12
#define CRYPTO_DMAPORTERR_PORT1_AHB_ERROR_M                         0x00001000
#define CRYPTO_DMAPORTERR_PORT1_AHB_ERROR_S                                 12

// Field:     [9] PORT1_CHANNEL
//
// Indicates which channel has serviced last (channel 0 or channel 1) by AHB
// master port.
#define CRYPTO_DMAPORTERR_PORT1_CHANNEL                             0x00000200
#define CRYPTO_DMAPORTERR_PORT1_CHANNEL_BITN                                 9
#define CRYPTO_DMAPORTERR_PORT1_CHANNEL_M                           0x00000200
#define CRYPTO_DMAPORTERR_PORT1_CHANNEL_S                                    9

//*****************************************************************************
//
// Register: CRYPTO_O_DMAHWVER
//
//*****************************************************************************
// Field: [27:24] HW_MAJOR_VERSION
//
// Major version number
#define CRYPTO_DMAHWVER_HW_MAJOR_VERSION_W                                   4
#define CRYPTO_DMAHWVER_HW_MAJOR_VERSION_M                          0x0F000000
#define CRYPTO_DMAHWVER_HW_MAJOR_VERSION_S                                  24

// Field: [23:20] HW_MINOR_VERSION
//
// Minor version number
#define CRYPTO_DMAHWVER_HW_MINOR_VERSION_W                                   4
#define CRYPTO_DMAHWVER_HW_MINOR_VERSION_M                          0x00F00000
#define CRYPTO_DMAHWVER_HW_MINOR_VERSION_S                                  20

// Field: [19:16] HW_PATCH_LEVEL
//
// Patch level
// Starts at 0 at first delivery of this version
#define CRYPTO_DMAHWVER_HW_PATCH_LEVEL_W                                     4
#define CRYPTO_DMAHWVER_HW_PATCH_LEVEL_M                            0x000F0000
#define CRYPTO_DMAHWVER_HW_PATCH_LEVEL_S                                    16

// Field:  [15:8] EIP_NUMBER_COMPL
//
// Bit-by-bit complement of the EIP_NUMBER field bits.
#define CRYPTO_DMAHWVER_EIP_NUMBER_COMPL_W                                   8
#define CRYPTO_DMAHWVER_EIP_NUMBER_COMPL_M                          0x0000FF00
#define CRYPTO_DMAHWVER_EIP_NUMBER_COMPL_S                                   8

// Field:   [7:0] EIP_NUMBER
//
// Binary encoding of the EIP-number of this DMA controller (209)
#define CRYPTO_DMAHWVER_EIP_NUMBER_W                                         8
#define CRYPTO_DMAHWVER_EIP_NUMBER_M                                0x000000FF
#define CRYPTO_DMAHWVER_EIP_NUMBER_S                                         0

//*****************************************************************************
//
// Register: CRYPTO_O_KEYWRITEAREA
//
//*****************************************************************************
// Field:     [7] RAM_AREA7
//
// Each RAM_AREAx represents an area of 128 bits.
// Select the key store RAM area(s) where the key(s) needs to be written
// 0: RAM_AREA7 is not selected to be written.
// 1: RAM_AREA7 is selected to be written.
// Writing to multiple RAM locations is possible only when the selected RAM
// areas are sequential.
// Keys that require more than one RAM locations (key size is 192 or 256 bits),
// must start at one of the following areas: RAM_AREA0, RAM_AREA2, RAM_AREA4,
// or RAM_AREA6.
// ENUMs:
// SEL                      This RAM area is selected to be written
// NOT_SEL                  This RAM area is not selected to be written
#define CRYPTO_KEYWRITEAREA_RAM_AREA7                               0x00000080
#define CRYPTO_KEYWRITEAREA_RAM_AREA7_BITN                                   7
#define CRYPTO_KEYWRITEAREA_RAM_AREA7_M                             0x00000080
#define CRYPTO_KEYWRITEAREA_RAM_AREA7_S                                      7
#define CRYPTO_KEYWRITEAREA_RAM_AREA7_SEL                           0x00000080
#define CRYPTO_KEYWRITEAREA_RAM_AREA7_NOT_SEL                       0x00000000

// Field:     [6] RAM_AREA6
//
// Each RAM_AREAx represents an area of 128 bits.
// Select the key store RAM area(s) where the key(s) needs to be written
// 0: RAM_AREA6 is not selected to be written.
// 1: RAM_AREA6 is selected to be written.
// Writing to multiple RAM locations is possible only when the selected RAM
// areas are sequential.
// Keys that require more than one RAM locations (key size is 192 or 256 bits),
// must start at one of the following areas: RAM_AREA0, RAM_AREA2, RAM_AREA4,
// or RAM_AREA6.
// ENUMs:
// SEL                      This RAM area is selected to be written
// NOT_SEL                  This RAM area is not selected to be written
#define CRYPTO_KEYWRITEAREA_RAM_AREA6                               0x00000040
#define CRYPTO_KEYWRITEAREA_RAM_AREA6_BITN                                   6
#define CRYPTO_KEYWRITEAREA_RAM_AREA6_M                             0x00000040
#define CRYPTO_KEYWRITEAREA_RAM_AREA6_S                                      6
#define CRYPTO_KEYWRITEAREA_RAM_AREA6_SEL                           0x00000040
#define CRYPTO_KEYWRITEAREA_RAM_AREA6_NOT_SEL                       0x00000000

// Field:     [5] RAM_AREA5
//
// Each RAM_AREAx represents an area of 128 bits.
// Select the key store RAM area(s) where the key(s) needs to be written
// 0: RAM_AREA5 is not selected to be written.
// 1: RAM_AREA5 is selected to be written.
// Writing to multiple RAM locations is possible only when the selected RAM
// areas are sequential.
// Keys that require more than one RAM locations (key size is 192 or 256 bits),
// must start at one of the following areas: RAM_AREA0, RAM_AREA2, RAM_AREA4,
// or RAM_AREA6.
// ENUMs:
// SEL                      This RAM area is selected to be written
// NOT_SEL                  This RAM area is not selected to be written
#define CRYPTO_KEYWRITEAREA_RAM_AREA5                               0x00000020
#define CRYPTO_KEYWRITEAREA_RAM_AREA5_BITN                                   5
#define CRYPTO_KEYWRITEAREA_RAM_AREA5_M                             0x00000020
#define CRYPTO_KEYWRITEAREA_RAM_AREA5_S                                      5
#define CRYPTO_KEYWRITEAREA_RAM_AREA5_SEL                           0x00000020
#define CRYPTO_KEYWRITEAREA_RAM_AREA5_NOT_SEL                       0x00000000

// Field:     [4] RAM_AREA4
//
// Each RAM_AREAx represents an area of 128 bits.
// Select the key store RAM area(s) where the key(s) needs to be written
// 0: RAM_AREA4 is not selected to be written.
// 1: RAM_AREA4 is selected to be written.
// Writing to multiple RAM locations is possible only when the selected RAM
// areas are sequential.
// Keys that require more than one RAM locations (key size is 192 or 256 bits),
// must start at one of the following areas: RAM_AREA0, RAM_AREA2, RAM_AREA4,
// or RAM_AREA6.
// ENUMs:
// SEL                      This RAM area is selected to be written
// NOT_SEL                  This RAM area is not selected to be written
#define CRYPTO_KEYWRITEAREA_RAM_AREA4                               0x00000010
#define CRYPTO_KEYWRITEAREA_RAM_AREA4_BITN                                   4
#define CRYPTO_KEYWRITEAREA_RAM_AREA4_M                             0x00000010
#define CRYPTO_KEYWRITEAREA_RAM_AREA4_S                                      4
#define CRYPTO_KEYWRITEAREA_RAM_AREA4_SEL                           0x00000010
#define CRYPTO_KEYWRITEAREA_RAM_AREA4_NOT_SEL                       0x00000000

// Field:     [3] RAM_AREA3
//
// Each RAM_AREAx represents an area of 128 bits.
// Select the key store RAM area(s) where the key(s) needs to be written
// 0: RAM_AREA3 is not selected to be written.
// 1: RAM_AREA3 is selected to be written.
// Writing to multiple RAM locations is possible only when the selected RAM
// areas are sequential.
// Keys that require more than one RAM locations (key size is 192 or 256 bits),
// must start at one of the following areas: RAM_AREA0, RAM_AREA2, RAM_AREA4,
// or RAM_AREA6.
// ENUMs:
// SEL                      This RAM area is selected to be written
// NOT_SEL                  This RAM area is not selected to be written
#define CRYPTO_KEYWRITEAREA_RAM_AREA3                               0x00000008
#define CRYPTO_KEYWRITEAREA_RAM_AREA3_BITN                                   3
#define CRYPTO_KEYWRITEAREA_RAM_AREA3_M                             0x00000008
#define CRYPTO_KEYWRITEAREA_RAM_AREA3_S                                      3
#define CRYPTO_KEYWRITEAREA_RAM_AREA3_SEL                           0x00000008
#define CRYPTO_KEYWRITEAREA_RAM_AREA3_NOT_SEL                       0x00000000

// Field:     [2] RAM_AREA2
//
// Each RAM_AREAx represents an area of 128 bits.
// Select the key store RAM area(s) where the key(s) needs to be written
// 0: RAM_AREA2 is not selected to be written.
// 1: RAM_AREA2 is selected to be written.
// Writing to multiple RAM locations is possible only when the selected RAM
// areas are sequential.
// Keys that require more than one RAM locations (key size is 192 or 256 bits),
// must start at one of the following areas: RAM_AREA0, RAM_AREA2, RAM_AREA4,
// or RAM_AREA6.
// ENUMs:
// SEL                      This RAM area is selected to be written
// NOT_SEL                  This RAM area is not selected to be written
#define CRYPTO_KEYWRITEAREA_RAM_AREA2                               0x00000004
#define CRYPTO_KEYWRITEAREA_RAM_AREA2_BITN                                   2
#define CRYPTO_KEYWRITEAREA_RAM_AREA2_M                             0x00000004
#define CRYPTO_KEYWRITEAREA_RAM_AREA2_S                                      2
#define CRYPTO_KEYWRITEAREA_RAM_AREA2_SEL                           0x00000004
#define CRYPTO_KEYWRITEAREA_RAM_AREA2_NOT_SEL                       0x00000000

// Field:     [1] RAM_AREA1
//
// Each RAM_AREAx represents an area of 128 bits.
// Select the key store RAM area(s) where the key(s) needs to be written
// 0: RAM_AREA1 is not selected to be written.
// 1: RAM_AREA1 is selected to be written.
// Writing to multiple RAM locations is possible only when the selected RAM
// areas are sequential.
// Keys that require more than one RAM locations (key size is 192 or 256 bits),
// must start at one of the following areas: RAM_AREA0, RAM_AREA2, RAM_AREA4,
// or RAM_AREA6.
// ENUMs:
// SEL                      This RAM area is selected to be written
// NOT_SEL                  This RAM area is not selected to be written
#define CRYPTO_KEYWRITEAREA_RAM_AREA1                               0x00000002
#define CRYPTO_KEYWRITEAREA_RAM_AREA1_BITN                                   1
#define CRYPTO_KEYWRITEAREA_RAM_AREA1_M                             0x00000002
#define CRYPTO_KEYWRITEAREA_RAM_AREA1_S                                      1
#define CRYPTO_KEYWRITEAREA_RAM_AREA1_SEL                           0x00000002
#define CRYPTO_KEYWRITEAREA_RAM_AREA1_NOT_SEL                       0x00000000

// Field:     [0] RAM_AREA0
//
// Each RAM_AREAx represents an area of 128 bits.
// Select the key store RAM area(s) where the key(s) needs to be written
// 0: RAM_AREA0 is not selected to be written.
// 1: RAM_AREA0 is selected to be written.
// Writing to multiple RAM locations is possible only when the selected RAM
// areas are sequential.
// Keys that require more than one RAM locations (key size is 192 or 256 bits),
// must start at one of the following areas: RAM_AREA0, RAM_AREA2, RAM_AREA4,
// or RAM_AREA6.
// ENUMs:
// SEL                      This RAM area is selected to be written
// NOT_SEL                  This RAM area is not selected to be written
#define CRYPTO_KEYWRITEAREA_RAM_AREA0                               0x00000001
#define CRYPTO_KEYWRITEAREA_RAM_AREA0_BITN                                   0
#define CRYPTO_KEYWRITEAREA_RAM_AREA0_M                             0x00000001
#define CRYPTO_KEYWRITEAREA_RAM_AREA0_S                                      0
#define CRYPTO_KEYWRITEAREA_RAM_AREA0_SEL                           0x00000001
#define CRYPTO_KEYWRITEAREA_RAM_AREA0_NOT_SEL                       0x00000000

//*****************************************************************************
//
// Register: CRYPTO_O_KEYWRITTENAREA
//
//*****************************************************************************
// Field:     [7] RAM_AREA_WRITTEN7
//
// On read this bit returns the key area written status.
//
// This bit can be reset by writing a 1.
//
// Note: This register will be reset on a soft reset initiated by writing to
// DMASWRESET.SWRES. After a soft reset, all keys must be rewritten to the key
// store memory.
// ENUMs:
// WRITTEN                  This RAM area is written with valid key
//                          information
// NOT_WRITTEN              This RAM area is not written with valid key
//                          information
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN7                     0x00000080
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN7_BITN                         7
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN7_M                   0x00000080
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN7_S                            7
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN7_WRITTEN             0x00000080
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN7_NOT_WRITTEN         0x00000000

// Field:     [6] RAM_AREA_WRITTEN6
//
// On read this bit returns the key area written status.
//
// This bit can be reset by writing a 1.
//
// Note: This register will be reset on a soft reset initiated by writing to
// DMASWRESET.SWRES. After a soft reset, all keys must be rewritten to the key
// store memory.
// ENUMs:
// WRITTEN                  This RAM area is written with valid key
//                          information
// NOT_WRITTEN              This RAM area is not written with valid key
//                          information
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN6                     0x00000040
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN6_BITN                         6
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN6_M                   0x00000040
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN6_S                            6
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN6_WRITTEN             0x00000040
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN6_NOT_WRITTEN         0x00000000

// Field:     [5] RAM_AREA_WRITTEN5
//
// On read this bit returns the key area written status.
//
// This bit can be reset by writing a 1.
//
// Note: This register will be reset on a soft reset initiated by writing to
// DMASWRESET.SWRES. After a soft reset, all keys must be rewritten to the key
// store memory.
// ENUMs:
// WRITTEN                  This RAM area is written with valid key
//                          information
// NOT_WRITTEN              This RAM area is not written with valid key
//                          information
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN5                     0x00000020
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN5_BITN                         5
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN5_M                   0x00000020
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN5_S                            5
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN5_WRITTEN             0x00000020
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN5_NOT_WRITTEN         0x00000000

// Field:     [4] RAM_AREA_WRITTEN4
//
// On read this bit returns the key area written status.
//
// This bit can be reset by writing a 1.
//
// Note: This register will be reset on a soft reset initiated by writing to
// DMASWRESET.SWRES. After a soft reset, all keys must be rewritten to the key
// store memory.
// ENUMs:
// WRITTEN                  This RAM area is written with valid key
//                          information
// NOT_WRITTEN              This RAM area is not written with valid key
//                          information
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN4                     0x00000010
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN4_BITN                         4
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN4_M                   0x00000010
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN4_S                            4
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN4_WRITTEN             0x00000010
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN4_NOT_WRITTEN         0x00000000

// Field:     [3] RAM_AREA_WRITTEN3
//
// On read this bit returns the key area written status.
//
// This bit can be reset by writing a 1.
//
// Note: This register will be reset on a soft reset initiated by writing to
// DMASWRESET.SWRES. After a soft reset, all keys must be rewritten to the key
// store memory.
// ENUMs:
// WRITTEN                  This RAM area is written with valid key
//                          information
// NOT_WRITTEN              This RAM area is not written with valid key
//                          information
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN3                     0x00000008
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN3_BITN                         3
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN3_M                   0x00000008
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN3_S                            3
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN3_WRITTEN             0x00000008
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN3_NOT_WRITTEN         0x00000000

// Field:     [2] RAM_AREA_WRITTEN2
//
// On read this bit returns the key area written status.
//
// This bit can be reset by writing a 1.
//
// Note: This register will be reset on a soft reset initiated by writing to
// DMASWRESET.SWRES. After a soft reset, all keys must be rewritten to the key
// store memory.
// ENUMs:
// WRITTEN                  This RAM area is written with valid key
//                          information
// NOT_WRITTEN              This RAM area is not written with valid key
//                          information
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN2                     0x00000004
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN2_BITN                         2
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN2_M                   0x00000004
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN2_S                            2
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN2_WRITTEN             0x00000004
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN2_NOT_WRITTEN         0x00000000

// Field:     [1] RAM_AREA_WRITTEN1
//
// On read this bit returns the key area written status.
//
// This bit can be reset by writing a 1.
//
// Note: This register will be reset on a soft reset initiated by writing to
// DMASWRESET.SWRES. After a soft reset, all keys must be rewritten to the key
// store memory.
// ENUMs:
// WRITTEN                  This RAM area is written with valid key
//                          information
// NOT_WRITTEN              This RAM area is not written with valid key
//                          information
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN1                     0x00000002
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN1_BITN                         1
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN1_M                   0x00000002
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN1_S                            1
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN1_WRITTEN             0x00000002
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN1_NOT_WRITTEN         0x00000000

// Field:     [0] RAM_AREA_WRITTEN0
//
// On read this bit returns the key area written status.
//
// This bit can be reset by writing a 1.
//
// Note: This register will be reset on a soft reset initiated by writing to
// DMASWRESET.SWRES. After a soft reset, all keys must be rewritten to the key
// store memory.
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN0                     0x00000001
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN0_BITN                         0
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN0_M                   0x00000001
#define CRYPTO_KEYWRITTENAREA_RAM_AREA_WRITTEN0_S                            0

//*****************************************************************************
//
// Register: CRYPTO_O_KEYSIZE
//
//*****************************************************************************
// Field:   [1:0] SIZE
//
// Key size:
// 00: Reserved
// When writing this to this register, the KEY_STORE_WRITTEN_AREA register is
// reset.
// ENUMs:
// 256_BIT                  256 bits
// 192_BIT                  192 bits
// 128_BIT                  128 bits
#define CRYPTO_KEYSIZE_SIZE_W                                                2
#define CRYPTO_KEYSIZE_SIZE_M                                       0x00000003
#define CRYPTO_KEYSIZE_SIZE_S                                                0
#define CRYPTO_KEYSIZE_SIZE_256_BIT                                 0x00000003
#define CRYPTO_KEYSIZE_SIZE_192_BIT                                 0x00000002
#define CRYPTO_KEYSIZE_SIZE_128_BIT                                 0x00000001

//*****************************************************************************
//
// Register: CRYPTO_O_KEYREADAREA
//
//*****************************************************************************
// Field:    [31] BUSY
//
// Key store operation busy status flag (read only):
// 0: Operation is complete.
// 1: Operation is not completed and the key store is busy.
#define CRYPTO_KEYREADAREA_BUSY                                     0x80000000
#define CRYPTO_KEYREADAREA_BUSY_BITN                                        31
#define CRYPTO_KEYREADAREA_BUSY_M                                   0x80000000
#define CRYPTO_KEYREADAREA_BUSY_S                                           31

// Field:   [3:0] RAM_AREA
//
// Selects the area of the key store RAM from where the key needs to be read
// that will be writen to the AES engine
// RAM_AREA:
//
// RAM areas RAM_AREA0, RAM_AREA2, RAM_AREA4 and RAM_AREA6 are the only valid
// read areas for 192 and 256 bits key sizes.
// Only RAM areas that contain valid written keys can be selected.
// ENUMs:
// NO_RAM                   No RAM
// RAM_AREA7                RAM Area 7
// RAM_AREA6                RAM Area 6
// RAM_AREA5                RAM Area 5
// RAM_AREA4                RAM Area 4
// RAM_AREA3                RAM Area 3
// RAM_AREA2                RAM Area 2
// RAM_AREA1                RAM Area 1
// RAM_AREA0                RAM Area 0
#define CRYPTO_KEYREADAREA_RAM_AREA_W                                        4
#define CRYPTO_KEYREADAREA_RAM_AREA_M                               0x0000000F
#define CRYPTO_KEYREADAREA_RAM_AREA_S                                        0
#define CRYPTO_KEYREADAREA_RAM_AREA_NO_RAM                          0x00000008
#define CRYPTO_KEYREADAREA_RAM_AREA_RAM_AREA7                       0x00000007
#define CRYPTO_KEYREADAREA_RAM_AREA_RAM_AREA6                       0x00000006
#define CRYPTO_KEYREADAREA_RAM_AREA_RAM_AREA5                       0x00000005
#define CRYPTO_KEYREADAREA_RAM_AREA_RAM_AREA4                       0x00000004
#define CRYPTO_KEYREADAREA_RAM_AREA_RAM_AREA3                       0x00000003
#define CRYPTO_KEYREADAREA_RAM_AREA_RAM_AREA2                       0x00000002
#define CRYPTO_KEYREADAREA_RAM_AREA_RAM_AREA1                       0x00000001
#define CRYPTO_KEYREADAREA_RAM_AREA_RAM_AREA0                       0x00000000

//*****************************************************************************
//
// Register: CRYPTO_O_AESKEY20
//
//*****************************************************************************
// Field:  [31:0] AES_KEY2
//
// AES_KEY2/AES_GHASH_H[31:0]
//
// For GCM:
// -[127:0] - GHASH_H - The internally calculated GHASH key is stored in these
// registers. Only used for modes that use the GHASH function (GCM).
// -[255:128] - This register is used to store intermediate values and is
// initialized with 0s when loading a new key.
//
// For CCM:
// -[255:0] - This register is used to store intermediate values.
//
// For CBC-MAC:
// -[255:0] - ZEROES - This register must remain 0.
#define CRYPTO_AESKEY20_AES_KEY2_W                                          32
#define CRYPTO_AESKEY20_AES_KEY2_M                                  0xFFFFFFFF
#define CRYPTO_AESKEY20_AES_KEY2_S                                           0

//*****************************************************************************
//
// Register: CRYPTO_O_AESKEY21
//
//*****************************************************************************
// Field:  [31:0] AES_KEY2
//
// AES_KEY2/AES_GHASH_H[31:0]
//
// For GCM:
// -[127:0] - GHASH_H - The internally calculated GHASH key is stored in these
// registers. Only used for modes that use the GHASH function (GCM).
// -[255:128] - This register is used to store intermediate values and is
// initialized with 0s when loading a new key.
//
// For CCM:
// -[255:0] - This register is used to store intermediate values.
//
// For CBC-MAC:
// -[255:0] - ZEROES - This register must remain 0.
#define CRYPTO_AESKEY21_AES_KEY2_W                                          32
#define CRYPTO_AESKEY21_AES_KEY2_M                                  0xFFFFFFFF
#define CRYPTO_AESKEY21_AES_KEY2_S                                           0

//*****************************************************************************
//
// Register: CRYPTO_O_AESKEY22
//
//*****************************************************************************
// Field:  [31:0] AES_KEY2
//
// AES_KEY2/AES_GHASH_H[31:0]
//
// For GCM:
// -[127:0] - GHASH_H - The internally calculated GHASH key is stored in these
// registers. Only used for modes that use the GHASH function (GCM).
// -[255:128] - This register is used to store intermediate values and is
// initialized with 0s when loading a new key.
//
// For CCM:
// -[255:0] - This register is used to store intermediate values.
//
// For CBC-MAC:
// -[255:0] - ZEROES - This register must remain 0.
#define CRYPTO_AESKEY22_AES_KEY2_W                                          32
#define CRYPTO_AESKEY22_AES_KEY2_M                                  0xFFFFFFFF
#define CRYPTO_AESKEY22_AES_KEY2_S                                           0

//*****************************************************************************
//
// Register: CRYPTO_O_AESKEY23
//
//*****************************************************************************
// Field:  [31:0] AES_KEY2
//
// AES_KEY2/AES_GHASH_H[31:0]
//
// For GCM:
// -[127:0] - GHASH_H - The internally calculated GHASH key is stored in these
// registers. Only used for modes that use the GHASH function (GCM).
// -[255:128] - This register is used to store intermediate values and is
// initialized with 0s when loading a new key.
//
// For CCM:
// -[255:0] - This register is used to store intermediate values.
//
// For CBC-MAC:
// -[255:0] - ZEROES - This register must remain 0.
#define CRYPTO_AESKEY23_AES_KEY2_W                                          32
#define CRYPTO_AESKEY23_AES_KEY2_M                                  0xFFFFFFFF
#define CRYPTO_AESKEY23_AES_KEY2_S                                           0

//*****************************************************************************
//
// Register: CRYPTO_O_AESKEY30
//
//*****************************************************************************
// Field:  [31:0] AES_KEY3
//
// AES_KEY3[31:0]/AES_KEY2[159:128]
//
// For GCM:
// -[127:0] - GHASH_H - The internally calculated GHASH key is stored in these
// registers. Only used for modes that use the GHASH function (GCM).
// -[255:128] - This register is used to store intermediate values and is
// initialized with 0s when loading a new key.
//
// For CCM:
// -[255:0] - This register is used to store intermediate values.
//
// For CBC-MAC:
// -[255:0] - ZEROES - This register must remain 0.
#define CRYPTO_AESKEY30_AES_KEY3_W                                          32
#define CRYPTO_AESKEY30_AES_KEY3_M                                  0xFFFFFFFF
#define CRYPTO_AESKEY30_AES_KEY3_S                                           0

//*****************************************************************************
//
// Register: CRYPTO_O_AESKEY31
//
//*****************************************************************************
// Field:  [31:0] AES_KEY3
//
// AES_KEY3[31:0]/AES_KEY2[159:128]
//
// For GCM:
// -[127:0] - GHASH_H - The internally calculated GHASH key is stored in these
// registers. Only used for modes that use the GHASH function (GCM).
// -[255:128] - This register is used to store intermediate values and is
// initialized with 0s when loading a new key.
//
// For CCM:
// -[255:0] - This register is used to store intermediate values.
//
// For CBC-MAC:
// -[255:0] - ZEROES - This register must remain 0.
#define CRYPTO_AESKEY31_AES_KEY3_W                                          32
#define CRYPTO_AESKEY31_AES_KEY3_M                                  0xFFFFFFFF
#define CRYPTO_AESKEY31_AES_KEY3_S                                           0

//*****************************************************************************
//
// Register: CRYPTO_O_AESKEY32
//
//*****************************************************************************
// Field:  [31:0] AES_KEY3
//
// AES_KEY3[31:0]/AES_KEY2[159:128]
//
// For GCM:
// -[127:0] - GHASH_H - The internally calculated GHASH key is stored in these
// registers. Only used for modes that use the GHASH function (GCM).
// -[255:128] - This register is used to store intermediate values and is
// initialized with 0s when loading a new key.
//
// For CCM:
// -[255:0] - This register is used to store intermediate values.
//
// For CBC-MAC:
// -[255:0] - ZEROES - This register must remain 0.
#define CRYPTO_AESKEY32_AES_KEY3_W                                          32
#define CRYPTO_AESKEY32_AES_KEY3_M                                  0xFFFFFFFF
#define CRYPTO_AESKEY32_AES_KEY3_S                                           0

//*****************************************************************************
//
// Register: CRYPTO_O_AESKEY33
//
//*****************************************************************************
// Field:  [31:0] AES_KEY3
//
// AES_KEY3[31:0]/AES_KEY2[159:128]
//
// For GCM:
// -[127:0] - GHASH_H - The internally calculated GHASH key is stored in these
// registers. Only used for modes that use the GHASH function (GCM).
// -[255:128] - This register is used to store intermediate values and is
// initialized with 0s when loading a new key.
//
// For CCM:
// -[255:0] - This register is used to store intermediate values.
//
// For CBC-MAC:
// -[255:0] - ZEROES - This register must remain 0.
#define CRYPTO_AESKEY33_AES_KEY3_W                                          32
#define CRYPTO_AESKEY33_AES_KEY3_M                                  0xFFFFFFFF
#define CRYPTO_AESKEY33_AES_KEY3_S                                           0

//*****************************************************************************
//
// Register: CRYPTO_O_AESIV0
//
//*****************************************************************************
// Field:  [31:0] AES_IV
//
// AES_IV[31:0]
//
// Initialization vector
// Used for regular non-ECB modes (CBC/CTR):
// -[127:0] - AES_IV - For regular AES operations (CBC and CTR) these registers
// must be written with a new 128-bit IV. After an operation, these registers
// contain the latest 128-bit result IV, generated by the EIP-120t. If CTR mode
// is selected, this value is incremented with 0x1: After first use - When a
// new data block is submitted to the engine
//
// For GCM:
// -[127:0] - AES_IV - For GCM operations, these registers must be written with
// a new 128-bit IV.
// After an operation, these registers contain the updated 128-bit result IV,
// generated by the EIP-120t. Note that bits [127:96] of the IV represent the
// initial counter value (which is 1 for GCM) and must therefore be initialized
// to 0x01000000. This value is incremented with 0x1: After first use - When a
// new data block is submitted to the engine.
//
// For CCM:
// -[127:0] - A0: For CCM this field must be written with value A0, this value
// is the concatenation of: A0-flags (5-bits of 0 and 3-bits 'L'), Nonce and
// counter value. 'L' must be a copy from the 'L' value of the AES_CTRL
// register. This 'L' indicates the width of the Nonce and counter. The loaded
// counter must be initialized to 0. The total width of A0 is 128-bit.
//
// For CBC-MAC:
// -[127:0] - Zeroes - For CBC-MAC this register must be written with 0s at the
// start of each operation. After an operation, these registers contain the
// 128-bit TAG output, generated by the EIP-120t.
#define CRYPTO_AESIV0_AES_IV_W                                              32
#define CRYPTO_AESIV0_AES_IV_M                                      0xFFFFFFFF
#define CRYPTO_AESIV0_AES_IV_S                                               0

//*****************************************************************************
//
// Register: CRYPTO_O_AESIV1
//
//*****************************************************************************
// Field:  [31:0] AES_IV
//
// AES_IV[31:0]
//
// Initialization vector
// Used for regular non-ECB modes (CBC/CTR):
// -[127:0] - AES_IV - For regular AES operations (CBC and CTR) these registers
// must be written with a new 128-bit IV. After an operation, these registers
// contain the latest 128-bit result IV, generated by the EIP-120t. If CTR mode
// is selected, this value is incremented with 0x1: After first use - When a
// new data block is submitted to the engine
//
// For GCM:
// -[127:0] - AES_IV - For GCM operations, these registers must be written with
// a new 128-bit IV.
// After an operation, these registers contain the updated 128-bit result IV,
// generated by the EIP-120t. Note that bits [127:96] of the IV represent the
// initial counter value (which is 1 for GCM) and must therefore be initialized
// to 0x01000000. This value is incremented with 0x1: After first use - When a
// new data block is submitted to the engine.
//
// For CCM:
// -[127:0] - A0: For CCM this field must be written with value A0, this value
// is the concatenation of: A0-flags (5-bits of 0 and 3-bits 'L'), Nonce and
// counter value. 'L' must be a copy from the 'L' value of the AES_CTRL
// register. This 'L' indicates the width of the Nonce and counter. The loaded
// counter must be initialized to 0. The total width of A0 is 128-bit.
//
// For CBC-MAC:
// -[127:0] - Zeroes - For CBC-MAC this register must be written with 0s at the
// start of each operation. After an operation, these registers contain the
// 128-bit TAG output, generated by the EIP-120t.
#define CRYPTO_AESIV1_AES_IV_W                                              32
#define CRYPTO_AESIV1_AES_IV_M                                      0xFFFFFFFF
#define CRYPTO_AESIV1_AES_IV_S                                               0

//*****************************************************************************
//
// Register: CRYPTO_O_AESIV2
//
//*****************************************************************************
// Field:  [31:0] AES_IV
//
// AES_IV[31:0]
//
// Initialization vector
// Used for regular non-ECB modes (CBC/CTR):
// -[127:0] - AES_IV - For regular AES operations (CBC and CTR) these registers
// must be written with a new 128-bit IV. After an operation, these registers
// contain the latest 128-bit result IV, generated by the EIP-120t. If CTR mode
// is selected, this value is incremented with 0x1: After first use - When a
// new data block is submitted to the engine
//
// For GCM:
// -[127:0] - AES_IV - For GCM operations, these registers must be written with
// a new 128-bit IV.
// After an operation, these registers contain the updated 128-bit result IV,
// generated by the EIP-120t. Note that bits [127:96] of the IV represent the
// initial counter value (which is 1 for GCM) and must therefore be initialized
// to 0x01000000. This value is incremented with 0x1: After first use - When a
// new data block is submitted to the engine.
//
// For CCM:
// -[127:0] - A0: For CCM this field must be written with value A0, this value
// is the concatenation of: A0-flags (5-bits of 0 and 3-bits 'L'), Nonce and
// counter value. 'L' must be a copy from the 'L' value of the AES_CTRL
// register. This 'L' indicates the width of the Nonce and counter. The loaded
// counter must be initialized to 0. The total width of A0 is 128-bit.
//
// For CBC-MAC:
// -[127:0] - Zeroes - For CBC-MAC this register must be written with 0s at the
// start of each operation. After an operation, these registers contain the
// 128-bit TAG output, generated by the EIP-120t.
#define CRYPTO_AESIV2_AES_IV_W                                              32
#define CRYPTO_AESIV2_AES_IV_M                                      0xFFFFFFFF
#define CRYPTO_AESIV2_AES_IV_S                                               0

//*****************************************************************************
//
// Register: CRYPTO_O_AESIV3
//
//*****************************************************************************
// Field:  [31:0] AES_IV
//
// AES_IV[31:0]
//
// Initialization vector
// Used for regular non-ECB modes (CBC/CTR):
// -[127:0] - AES_IV - For regular AES operations (CBC and CTR) these registers
// must be written with a new 128-bit IV. After an operation, these registers
// contain the latest 128-bit result IV, generated by the EIP-120t. If CTR mode
// is selected, this value is incremented with 0x1: After first use - When a
// new data block is submitted to the engine
//
// For GCM:
// -[127:0] - AES_IV - For GCM operations, these registers must be written with
// a new 128-bit IV.
// After an operation, these registers contain the updated 128-bit result IV,
// generated by the EIP-120t. Note that bits [127:96] of the IV represent the
// initial counter value (which is 1 for GCM) and must therefore be initialized
// to 0x01000000. This value is incremented with 0x1: After first use - When a
// new data block is submitted to the engine.
//
// For CCM:
// -[127:0] - A0: For CCM this field must be written with value A0, this value
// is the concatenation of: A0-flags (5-bits of 0 and 3-bits 'L'), Nonce and
// counter value. 'L' must be a copy from the 'L' value of the AES_CTRL
// register. This 'L' indicates the width of the Nonce and counter. The loaded
// counter must be initialized to 0. The total width of A0 is 128-bit.
//
// For CBC-MAC:
// -[127:0] - Zeroes - For CBC-MAC this register must be written with 0s at the
// start of each operation. After an operation, these registers contain the
// 128-bit TAG output, generated by the EIP-120t.
#define CRYPTO_AESIV3_AES_IV_W                                              32
#define CRYPTO_AESIV3_AES_IV_M                                      0xFFFFFFFF
#define CRYPTO_AESIV3_AES_IV_S                                               0

//*****************************************************************************
//
// Register: CRYPTO_O_AESCTL
//
//*****************************************************************************
// Field:    [31] CONTEXT_READY
//
// If 1, this read-only status bit indicates that the context data registers
// can be overwritten and the host is permitted to write the next context.
#define CRYPTO_AESCTL_CONTEXT_READY                                 0x80000000
#define CRYPTO_AESCTL_CONTEXT_READY_BITN                                    31
#define CRYPTO_AESCTL_CONTEXT_READY_M                               0x80000000
#define CRYPTO_AESCTL_CONTEXT_READY_S                                       31

// Field:    [30] SAVED_CONTEXT_RDY
//
// If 1, this status bit indicates that an AES authentication TAG and/or IV
// block(s) is/are available for the host to retrieve. This bit is only
// asserted if the save_context bit is set to 1. The bit is mutual exclusive
// with the context_ready bit.
// Writing one clears the bit to 0, indicating the AES core can start its next
// operation. This bit is also cleared when the 4th word of the output TAG
// and/or IV is read.
// Note: All other mode bit writes are ignored when this mode bit is written
// with 1.
// Note: This bit is controlled automatically by the EIP-120t for TAG read DMA
// operations.
#define CRYPTO_AESCTL_SAVED_CONTEXT_RDY                             0x40000000
#define CRYPTO_AESCTL_SAVED_CONTEXT_RDY_BITN                                30
#define CRYPTO_AESCTL_SAVED_CONTEXT_RDY_M                           0x40000000
#define CRYPTO_AESCTL_SAVED_CONTEXT_RDY_S                                   30

// Field:    [29] SAVE_CONTEXT
//
// This bit indicates that an authentication TAG or result IV needs to be
// stored as a result context.
// Typically this bit must be set for authentication modes returning a TAG
// (CBC-MAC, GCM and CCM), or for basic encryption modes that require future
// continuation with the current result IV.
// If this bit is set, the engine retains its full context until the TAG and/or
// IV registers are read.
// The TAG or IV must be read before the AES engine can start a new operation.
#define CRYPTO_AESCTL_SAVE_CONTEXT                                  0x20000000
#define CRYPTO_AESCTL_SAVE_CONTEXT_BITN                                     29
#define CRYPTO_AESCTL_SAVE_CONTEXT_M                                0x20000000
#define CRYPTO_AESCTL_SAVE_CONTEXT_S                                        29

// Field: [24:22] CCM_M
//
// Defines M, which indicates the length of the authentication field for CCM
// operations; the authentication field length equals two times (the value of
// CCM-M plus one).
// Note: The EIP-120t always returns a 128-bit authentication field, of which
// the M least significant bytes are valid. All values are supported.
#define CRYPTO_AESCTL_CCM_M_W                                                3
#define CRYPTO_AESCTL_CCM_M_M                                       0x01C00000
#define CRYPTO_AESCTL_CCM_M_S                                               22

// Field: [21:19] CCM_L
//
// Defines L, which indicates the width of the length field for CCM operations;
// the length field in bytes equals the value of CMM-L plus one. All values are
// supported.
#define CRYPTO_AESCTL_CCM_L_W                                                3
#define CRYPTO_AESCTL_CCM_L_M                                       0x00380000
#define CRYPTO_AESCTL_CCM_L_S                                               19

// Field:    [18] CCM
//
// If set to 1, AES-CCM is selected
// AES-CCM is a combined mode, using AES for authentication and encryption.
// Note: Selecting AES-CCM mode requires writing of the AAD length register
// after all other registers.
// Note: The CTR mode bit in this register must also be set to 1 to enable
// AES-CTR; selecting other AES modes than CTR mode is invalid.
#define CRYPTO_AESCTL_CCM                                           0x00040000
#define CRYPTO_AESCTL_CCM_BITN                                              18
#define CRYPTO_AESCTL_CCM_M                                         0x00040000
#define CRYPTO_AESCTL_CCM_S                                                 18

// Field: [17:16] GCM
//
// Set these bits to 11 to select AES-GCM mode.
// AES-GCM is a combined mode, using the Galois field multiplier GF(2 to the
// power of 128) for authentication and AES-CTR mode for encryption.
// Note: The CTR mode bit in this register must also be set to 1 to enable
// AES-CTR
// Bit combination description:
// 00 = No GCM mode
// 01 = Reserved, do not select
// 10 = Reserved, do not select
// 11 = Autonomous GHASH (both H- and Y0-encrypted calculated internally)
// Note: The EIP-120t-1 configuration only supports mode 11 (autonomous GHASH),
// other GCM modes are not allowed.
#define CRYPTO_AESCTL_GCM_W                                                  2
#define CRYPTO_AESCTL_GCM_M                                         0x00030000
#define CRYPTO_AESCTL_GCM_S                                                 16

// Field:    [15] CBC_MAC
//
// Set to 1 to select AES-CBC MAC mode.
// The direction bit must be set to 1 for this mode.
// Selecting this mode requires writing the length register after all other
// registers.
#define CRYPTO_AESCTL_CBC_MAC                                       0x00008000
#define CRYPTO_AESCTL_CBC_MAC_BITN                                          15
#define CRYPTO_AESCTL_CBC_MAC_M                                     0x00008000
#define CRYPTO_AESCTL_CBC_MAC_S                                             15

// Field:   [8:7] CTR_WIDTH
//
// Specifies the counter width for AES-CTR mode
// 00 = 32-bit counter
// 01 = 64-bit counter
// 10 = 96-bit counter
// 11 = 128-bit counter
// ENUMs:
// 128_BIT                  128 bits
// 96_BIT                   96 bits
// 64_BIT                   64 bits
// 32_BIT                   32 bits
#define CRYPTO_AESCTL_CTR_WIDTH_W                                            2
#define CRYPTO_AESCTL_CTR_WIDTH_M                                   0x00000180
#define CRYPTO_AESCTL_CTR_WIDTH_S                                            7
#define CRYPTO_AESCTL_CTR_WIDTH_128_BIT                             0x00000180
#define CRYPTO_AESCTL_CTR_WIDTH_96_BIT                              0x00000100
#define CRYPTO_AESCTL_CTR_WIDTH_64_BIT                              0x00000080
#define CRYPTO_AESCTL_CTR_WIDTH_32_BIT                              0x00000000

// Field:     [6] CTR
//
// If set to 1, AES counter mode (CTR) is selected.
// Note: This bit must also be set for GCM and CCM, when encryption/decryption
// is required.
#define CRYPTO_AESCTL_CTR                                           0x00000040
#define CRYPTO_AESCTL_CTR_BITN                                               6
#define CRYPTO_AESCTL_CTR_M                                         0x00000040
#define CRYPTO_AESCTL_CTR_S                                                  6

// Field:     [5] CBC
//
// If set to 1, cipher-block-chaining (CBC) mode is selected.
#define CRYPTO_AESCTL_CBC                                           0x00000020
#define CRYPTO_AESCTL_CBC_BITN                                               5
#define CRYPTO_AESCTL_CBC_M                                         0x00000020
#define CRYPTO_AESCTL_CBC_S                                                  5

// Field:   [4:3] KEY_SIZE
//
// This read-only field specifies the key size.
// The key size is automatically configured when a new key is loaded through
// the key store module.
// 00 = N/A - Reserved
// 01 = 128-bit
// 10 = 192-bit
// 11 = 256-bit
#define CRYPTO_AESCTL_KEY_SIZE_W                                             2
#define CRYPTO_AESCTL_KEY_SIZE_M                                    0x00000018
#define CRYPTO_AESCTL_KEY_SIZE_S                                             3

// Field:     [2] DIR
//
// If set to 1 an encrypt operation is performed.
// If set to 0 a decrypt operation is performed.
// This bit must be written with a 1 when CBC-MAC is selected.
#define CRYPTO_AESCTL_DIR                                           0x00000004
#define CRYPTO_AESCTL_DIR_BITN                                               2
#define CRYPTO_AESCTL_DIR_M                                         0x00000004
#define CRYPTO_AESCTL_DIR_S                                                  2

// Field:     [1] INPUT_READY
//
// If 1, this status bit indicates that the 16-byte AES input buffer is empty.
// The host is permitted to write the next block of data.
// Writing 0 clears the bit to 0 and indicates that the AES core can use the
// provided input data block.
// Writing 1 to this bit is ignored.
// Note: For DMA operations, this bit is automatically controlled by the
// EIP-120t.
// After reset, this bit is 0. After writing a context, this bit becomes 1.
#define CRYPTO_AESCTL_INPUT_READY                                   0x00000002
#define CRYPTO_AESCTL_INPUT_READY_BITN                                       1
#define CRYPTO_AESCTL_INPUT_READY_M                                 0x00000002
#define CRYPTO_AESCTL_INPUT_READY_S                                          1

// Field:     [0] OUTPUT_READY
//
// If 1, this status bit indicates that an AES output block is available to be
// retrieved by the host.
// Writing 0 clears the bit to 0 and indicates that output data is read by the
// host. The AES core can provide a next output data block.
// Writing 1 to this bit is ignored.
// Note: For DMA operations, this bit is automatically controlled by the
// EIP-120t.
#define CRYPTO_AESCTL_OUTPUT_READY                                  0x00000001
#define CRYPTO_AESCTL_OUTPUT_READY_BITN                                      0
#define CRYPTO_AESCTL_OUTPUT_READY_M                                0x00000001
#define CRYPTO_AESCTL_OUTPUT_READY_S                                         0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATALEN0
//
//*****************************************************************************
// Field:  [31:0] C_LENGTH
//
// C_LENGTH[31:0]
// Bits [60:0] of the crypto length registers (LSW and MSW) store the
// cryptographic data length in bytes for all modes. Once processing with this
// context is started, this length decrements to 0. Data lengths up to (261: 1)
// bytes are allowed.
// For GCM, any value up to 236 - 32 bytes can be used. This is because a
// 32-bit counter mode is used; the maximum number of 128-bit blocks is 232 -
// 2, resulting in a maximum number of bytes of 236 - 32.
// A write to this register triggers the engine to start using this context.
// This is valid for all modes except GCM and CCM.
// Note: For the combined modes (GCM and CCM), this length does not include the
// authentication only data; the authentication length is specified in the
// AESAUTHLEN register
// All modes must have a length greater than 0. For the combined modes, it is
// allowed to have one of the lengths equal to 0.
// For the basic encryption modes (ECB, CBC, and CTR) it is allowed to program
// zero to the length field; in that case the length is assumed infinite.
// All data must be byte (8-bit) aligned for stream cipher modes; bit aligned
// data streams are not supported by the EIP-120t. For block cipher modes, the
// data length must be programmed in multiples of the block cipher size, 16
// bytes.
// For a host read operation, these registers return all-0s.
#define CRYPTO_AESDATALEN0_C_LENGTH_W                                       32
#define CRYPTO_AESDATALEN0_C_LENGTH_M                               0xFFFFFFFF
#define CRYPTO_AESDATALEN0_C_LENGTH_S                                        0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATALEN1
//
//*****************************************************************************
// Field:  [28:0] C_LENGTH
//
// C_LENGTH[60:32]
// Bits [60:0] of the crypto length registers (LSW and MSW) store the
// cryptographic data length in bytes for all modes. Once processing with this
// context is started, this length decrements to 0. Data lengths up to (261: 1)
// bytes are allowed.
// For GCM, any value up to 236 - 32 bytes can be used. This is because a
// 32-bit counter mode is used; the maximum number of 128-bit blocks is 232 -
// 2, resulting in a maximum number of bytes of 236 - 32.
// A write to this register triggers the engine to start using this context.
// This is valid for all modes except GCM and CCM.
// Note: For the combined modes (GCM and CCM), this length does not include the
// authentication only data; the authentication length is specified in the
// AESAUTHLEN register
// All modes must have a length greater than 0. For the combined modes, it is
// allowed to have one of the lengths equal to 0.
// For the basic encryption modes (ECB, CBC, and CTR) it is allowed to program
// zero to the length field; in that case the length is assumed infinite.
// All data must be byte (8-bit) aligned for stream cipher modes; bit aligned
// data streams are not supported by the EIP-120t. For block cipher modes, the
// data length must be programmed in multiples of the block cipher size, 16
// bytes.
// For a host read operation, these registers return all-0s.
#define CRYPTO_AESDATALEN1_C_LENGTH_W                                       29
#define CRYPTO_AESDATALEN1_C_LENGTH_M                               0x1FFFFFFF
#define CRYPTO_AESDATALEN1_C_LENGTH_S                                        0

//*****************************************************************************
//
// Register: CRYPTO_O_AESAUTHLEN
//
//*****************************************************************************
// Field:  [31:0] AUTH_LENGTH
//
// Bits [31:0] of the authentication length register store the authentication
// data length in bytes for combined modes only (GCM or CCM).
// Supported AAD-lengths for CCM are from 0 to (2^16 - 2^8) bytes. For GCM any
// value up to (2^32 - 1) bytes can be used. Once processing with this context
// is started, this length decrements to 0.
// A write to this register triggers the engine to start using this context for
// GCM and CCM.
// For a host read operation, these registers return all-0s.
#define CRYPTO_AESAUTHLEN_AUTH_LENGTH_W                                     32
#define CRYPTO_AESAUTHLEN_AUTH_LENGTH_M                             0xFFFFFFFF
#define CRYPTO_AESAUTHLEN_AUTH_LENGTH_S                                      0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATAOUT0
//
//*****************************************************************************
// Field:  [31:0] DATA
//
// Data register 0 for output block data from the Crypto peripheral.
// These bits = AES Output Data[31:0] of {127:0]
//
// For normal operations, this register is not used, since data input and
// output is transferred from and to the AES engine via DMA.
//
// For a Host read operation, these registers contain the 128-bit output block
// from the latest AES operation. Reading from a word-aligned offset within
// this address range will read one word (4 bytes) of data out the 4-word deep
// (16 bytes = 128-bits AES block) data output buffer. The words (4 words, one
// full block) should be read before the core will move the next block to the
// data output buffer. To empty the data output buffer, AESCTL.OUTPUT_READY
// must be written.
// For the modes with authentication (CBC-MAC, GCM and CCM), the invalid
// (message) bytes/words can be written with any data.
//
// Note: The AAD / authentication only data is not copied to the output buffer
// but only used for authentication.
#define CRYPTO_AESDATAOUT0_DATA_W                                           32
#define CRYPTO_AESDATAOUT0_DATA_M                                   0xFFFFFFFF
#define CRYPTO_AESDATAOUT0_DATA_S                                            0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATAIN0
//
//*****************************************************************************
// Field:  [31:0] AES_DATA_IN_OUT
//
// AES input data[31:0] / AES output data[31:0]
// Data registers for input/output block data to/from the EIP-120t.
// For normal operations, this register is not used, since data input and
// output is transferred from and to the AES core via DMA. For a host write
// operation, these registers must be written with the 128-bit input block for
// the next AES operation. Writing at a word-aligned offset within this address
// range stores the word (4 bytes) of data into the corresponding position of
// 4-word deep (16 bytes = 128-bit AES block) data input buffer. This buffer is
// used for the next AES operation. If the last data block is not completely
// filled with valid data (see notes below), it is allowed to write only the
// words with valid data. Next AES operation is triggered by writing to the
// input_ready flag of the AES_CTRL register.
// For a host read operation, these registers contain the 128-bit output block
// from the latest AES operation. Reading from a word-aligned offset within
// this address range reads one word (4 bytes) of data out the 4-word deep (16
// bytes = 128-bits AES block) data output buffer. The words (4 words, one full
// block) should be read before the core will move the next block to the data
// output buffer. To empty the data output buffer, the output_ready flag of the
// AES_CTRL register must be written.
// For the modes with authentication (CBC-MAC, GCM and CCM), the invalid
// (message) bytes/words can be written with any data.
// Note: AES typically operates on 128 bits block multiple input data. The CTR,
// GCM and CCM modes form an exception. The last block of a CTR-mode message
// may contain less than 128 bits (refer to [NIST 800-38A]). For GCM/CCM, the
// last block of both AAD and message data may contain less than 128 bits
// (refer to [NIST 800-38D]). The EIP-120t automatically pads or masks
// misaligned ending data blocks with 0s for GCM, CCM and CBC-MAC. For CTR
// mode, the remaining data in an unaligned data block is ignored.
// Note: The AAD / authentication only data is not copied to the output buffer
// but only used for authentication.
#define CRYPTO_AESDATAIN0_AES_DATA_IN_OUT_W                                 32
#define CRYPTO_AESDATAIN0_AES_DATA_IN_OUT_M                         0xFFFFFFFF
#define CRYPTO_AESDATAIN0_AES_DATA_IN_OUT_S                                  0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATAOUT1
//
//*****************************************************************************
// Field:  [31:0] DATA
//
// Data register 0 for output block data from the Crypto peripheral.
// These bits = AES Output Data[31:0] of {127:0]
//
// For normal operations, this register is not used, since data input and
// output is transferred from and to the AES engine via DMA.
//
// For a Host read operation, these registers contain the 128-bit output block
// from the latest AES operation. Reading from a word-aligned offset within
// this address range will read one word (4 bytes) of data out the 4-word deep
// (16 bytes = 128-bits AES block) data output buffer. The words (4 words, one
// full block) should be read before the core will move the next block to the
// data output buffer. To empty the data output buffer, AESCTL.OUTPUT_READY
// must be written.
// For the modes with authentication (CBC-MAC, GCM and CCM), the invalid
// (message) bytes/words can be written with any data.
//
// Note: The AAD / authentication only data is not copied to the output buffer
// but only used for authentication.
#define CRYPTO_AESDATAOUT1_DATA_W                                           32
#define CRYPTO_AESDATAOUT1_DATA_M                                   0xFFFFFFFF
#define CRYPTO_AESDATAOUT1_DATA_S                                            0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATAIN1
//
//*****************************************************************************
// Field:  [31:0] AES_DATA_IN_OUT
//
// AES input data[31:0] / AES output data[63:32]
// Data registers for input/output block data to/from the EIP-120t.
// For normal operations, this register is not used, since data input and
// output is transferred from and to the AES core via DMA. For a host write
// operation, these registers must be written with the 128-bit input block for
// the next AES operation. Writing at a word-aligned offset within this address
// range stores the word (4 bytes) of data into the corresponding position of
// 4-word deep (16 bytes = 128-bit AES block) data input buffer. This buffer is
// used for the next AES operation. If the last data block is not completely
// filled with valid data (see notes below), it is allowed to write only the
// words with valid data. Next AES operation is triggered by writing to the
// input_ready flag of the AES_CTRL register.
// For a host read operation, these registers contain the 128-bit output block
// from the latest AES operation. Reading from a word-aligned offset within
// this address range reads one word (4 bytes) of data out the 4-word deep (16
// bytes = 128-bits AES block) data output buffer. The words (4 words, one full
// block) should be read before the core will move the next block to the data
// output buffer. To empty the data output buffer, the output_ready flag of the
// AES_CTRL register must be written.
// For the modes with authentication (CBC-MAC, GCM and CCM), the invalid
// (message) bytes/words can be written with any data.
// Note: AES typically operates on 128 bits block multiple input data. The CTR,
// GCM and CCM modes form an exception. The last block of a CTR-mode message
// may contain less than 128 bits (refer to [NIST 800-38A]). For GCM/CCM, the
// last block of both AAD and message data may contain less than 128 bits
// (refer to [NIST 800-38D]). The EIP-120t automatically pads or masks
// misaligned ending data blocks with 0s for GCM, CCM and CBC-MAC. For CTR
// mode, the remaining data in an unaligned data block is ignored.
// Note: The AAD / authentication only data is not copied to the output buffer
// but only used for authentication.
#define CRYPTO_AESDATAIN1_AES_DATA_IN_OUT_W                                 32
#define CRYPTO_AESDATAIN1_AES_DATA_IN_OUT_M                         0xFFFFFFFF
#define CRYPTO_AESDATAIN1_AES_DATA_IN_OUT_S                                  0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATAOUT2
//
//*****************************************************************************
// Field:  [31:0] DATA
//
// Data register 0 for output block data from the Crypto peripheral.
// These bits = AES Output Data[31:0] of {127:0]
//
// For normal operations, this register is not used, since data input and
// output is transferred from and to the AES engine via DMA.
//
// For a Host read operation, these registers contain the 128-bit output block
// from the latest AES operation. Reading from a word-aligned offset within
// this address range will read one word (4 bytes) of data out the 4-word deep
// (16 bytes = 128-bits AES block) data output buffer. The words (4 words, one
// full block) should be read before the core will move the next block to the
// data output buffer. To empty the data output buffer, AESCTL.OUTPUT_READY
// must be written.
// For the modes with authentication (CBC-MAC, GCM and CCM), the invalid
// (message) bytes/words can be written with any data.
//
// Note: The AAD / authentication only data is not copied to the output buffer
// but only used for authentication.
#define CRYPTO_AESDATAOUT2_DATA_W                                           32
#define CRYPTO_AESDATAOUT2_DATA_M                                   0xFFFFFFFF
#define CRYPTO_AESDATAOUT2_DATA_S                                            0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATAIN2
//
//*****************************************************************************
// Field:  [31:0] AES_DATA_IN_OUT
//
// AES input data[95:64] / AES output data[95:64]
// Data registers for input/output block data to/from the EIP-120t.
// For normal operations, this register is not used, since data input and
// output is transferred from and to the AES core via DMA. For a host write
// operation, these registers must be written with the 128-bit input block for
// the next AES operation. Writing at a word-aligned offset within this address
// range stores the word (4 bytes) of data into the corresponding position of
// 4-word deep (16 bytes = 128-bit AES block) data input buffer. This buffer is
// used for the next AES operation. If the last data block is not completely
// filled with valid data (see notes below), it is allowed to write only the
// words with valid data. Next AES operation is triggered by writing to the
// input_ready flag of the AES_CTRL register.
// For a host read operation, these registers contain the 128-bit output block
// from the latest AES operation. Reading from a word-aligned offset within
// this address range reads one word (4 bytes) of data out the 4-word deep (16
// bytes = 128-bits AES block) data output buffer. The words (4 words, one full
// block) should be read before the core will move the next block to the data
// output buffer. To empty the data output buffer, the output_ready flag of the
// AES_CTRL register must be written.
// For the modes with authentication (CBC-MAC, GCM and CCM), the invalid
// (message) bytes/words can be written with any data.
// Note: AES typically operates on 128 bits block multiple input data. The CTR,
// GCM and CCM modes form an exception. The last block of a CTR-mode message
// may contain less than 128 bits (refer to [NIST 800-38A]). For GCM/CCM, the
// last block of both AAD and message data may contain less than 128 bits
// (refer to [NIST 800-38D]). The EIP-120t automatically pads or masks
// misaligned ending data blocks with 0s for GCM, CCM and CBC-MAC. For CTR
// mode, the remaining data in an unaligned data block is ignored.
// Note: The AAD / authentication only data is not copied to the output buffer
// but only used for authentication.
#define CRYPTO_AESDATAIN2_AES_DATA_IN_OUT_W                                 32
#define CRYPTO_AESDATAIN2_AES_DATA_IN_OUT_M                         0xFFFFFFFF
#define CRYPTO_AESDATAIN2_AES_DATA_IN_OUT_S                                  0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATAOUT3
//
//*****************************************************************************
// Field:  [31:0] DATA
//
// Data register 0 for output block data from the Crypto peripheral.
// These bits = AES Output Data[31:0] of {127:0]
//
// For normal operations, this register is not used, since data input and
// output is transferred from and to the AES engine via DMA.
//
// For a Host read operation, these registers contain the 128-bit output block
// from the latest AES operation. Reading from a word-aligned offset within
// this address range will read one word (4 bytes) of data out the 4-word deep
// (16 bytes = 128-bits AES block) data output buffer. The words (4 words, one
// full block) should be read before the core will move the next block to the
// data output buffer. To empty the data output buffer, AESCTL.OUTPUT_READY
// must be written.
// For the modes with authentication (CBC-MAC, GCM and CCM), the invalid
// (message) bytes/words can be written with any data.
//
// Note: The AAD / authentication only data is not copied to the output buffer
// but only used for authentication.
#define CRYPTO_AESDATAOUT3_DATA_W                                           32
#define CRYPTO_AESDATAOUT3_DATA_M                                   0xFFFFFFFF
#define CRYPTO_AESDATAOUT3_DATA_S                                            0

//*****************************************************************************
//
// Register: CRYPTO_O_AESDATAIN3
//
//*****************************************************************************
// Field:  [31:0] AES_DATA_IN_OUT
//
// AES input data[127:96] / AES output data[127:96]
// Data registers for input/output block data to/from the EIP-120t.
// For normal operations, this register is not used, since data input and
// output is transferred from and to the AES core via DMA. For a host write
// operation, these registers must be written with the 128-bit input block for
// the next AES operation. Writing at a word-aligned offset within this address
// range stores the word (4 bytes) of data into the corresponding position of
// 4-word deep (16 bytes = 128-bit AES block) data input buffer. This buffer is
// used for the next AES operation. If the last data block is not completely
// filled with valid data (see notes below), it is allowed to write only the
// words with valid data. Next AES operation is triggered by writing to the
// input_ready flag of the AES_CTRL register.
// For a host read operation, these registers contain the 128-bit output block
// from the latest AES operation. Reading from a word-aligned offset within
// this address range reads one word (4 bytes) of data out the 4-word deep (16
// bytes = 128-bits AES block) data output buffer. The words (4 words, one full
// block) should be read before the core will move the next block to the data
// output buffer. To empty the data output buffer, the output_ready flag of the
// AES_CTRL register must be written.
// For the modes with authentication (CBC-MAC, GCM and CCM), the invalid
// (message) bytes/words can be written with any data.
// Note: AES typically operates on 128 bits block multiple input data. The CTR,
// GCM and CCM modes form an exception. The last block of a CTR-mode message
// may contain less than 128 bits (refer to [NIST 800-38A]). For GCM/CCM, the
// last block of both AAD and message data may contain less than 128 bits
// (refer to [NIST 800-38D]). The EIP-120t automatically pads or masks
// misaligned ending data blocks with 0s for GCM, CCM and CBC-MAC. For CTR
// mode, the remaining data in an unaligned data block is ignored.
// Note: The AAD / authentication only data is not copied to the output buffer
// but only used for authentication.
#define CRYPTO_AESDATAIN3_AES_DATA_IN_OUT_W                                 32
#define CRYPTO_AESDATAIN3_AES_DATA_IN_OUT_M                         0xFFFFFFFF
#define CRYPTO_AESDATAIN3_AES_DATA_IN_OUT_S                                  0

//*****************************************************************************
//
// Register: CRYPTO_O_AESTAGOUT0
//
//*****************************************************************************
// Field:  [31:0] AES_TAG
//
// AES_TAG[31:0]
// Bits [31:0] of this register stores the authentication value for the
// combined and authentication only modes.
// For a host read operation, these registers contain the last 128-bit TAG
// output of the EIP-120t; the TAG is available until the next context is
// written.
// This register will only contain valid data if the TAG is available and when
// the AESCTL.SAVED_CONTEXT_RDY register is set. During processing or for
// operations/modes that do not return a TAG, reads from this register return
// data from the IV register.
#define CRYPTO_AESTAGOUT0_AES_TAG_W                                         32
#define CRYPTO_AESTAGOUT0_AES_TAG_M                                 0xFFFFFFFF
#define CRYPTO_AESTAGOUT0_AES_TAG_S                                          0

//*****************************************************************************
//
// Register: CRYPTO_O_AESTAGOUT1
//
//*****************************************************************************
// Field:  [31:0] AES_TAG
//
// AES_TAG[31:0]
// Bits [31:0] of this register stores the authentication value for the
// combined and authentication only modes.
// For a host read operation, these registers contain the last 128-bit TAG
// output of the EIP-120t; the TAG is available until the next context is
// written.
// This register will only contain valid data if the TAG is available and when
// the AESCTL.SAVED_CONTEXT_RDY register is set. During processing or for
// operations/modes that do not return a TAG, reads from this register return
// data from the IV register.
#define CRYPTO_AESTAGOUT1_AES_TAG_W                                         32
#define CRYPTO_AESTAGOUT1_AES_TAG_M                                 0xFFFFFFFF
#define CRYPTO_AESTAGOUT1_AES_TAG_S                                          0

//*****************************************************************************
//
// Register: CRYPTO_O_AESTAGOUT2
//
//*****************************************************************************
// Field:  [31:0] AES_TAG
//
// AES_TAG[31:0]
// Bits [31:0] of this register stores the authentication value for the
// combined and authentication only modes.
// For a host read operation, these registers contain the last 128-bit TAG
// output of the EIP-120t; the TAG is available until the next context is
// written.
// This register will only contain valid data if the TAG is available and when
// the AESCTL.SAVED_CONTEXT_RDY register is set. During processing or for
// operations/modes that do not return a TAG, reads from this register return
// data from the IV register.
#define CRYPTO_AESTAGOUT2_AES_TAG_W                                         32
#define CRYPTO_AESTAGOUT2_AES_TAG_M                                 0xFFFFFFFF
#define CRYPTO_AESTAGOUT2_AES_TAG_S                                          0

//*****************************************************************************
//
// Register: CRYPTO_O_AESTAGOUT3
//
//*****************************************************************************
// Field:  [31:0] AES_TAG
//
// AES_TAG[31:0]
// Bits [31:0] of this register stores the authentication value for the
// combined and authentication only modes.
// For a host read operation, these registers contain the last 128-bit TAG
// output of the EIP-120t; the TAG is available until the next context is
// written.
// This register will only contain valid data if the TAG is available and when
// the AESCTL.SAVED_CONTEXT_RDY register is set. During processing or for
// operations/modes that do not return a TAG, reads from this register return
// data from the IV register.
#define CRYPTO_AESTAGOUT3_AES_TAG_W                                         32
#define CRYPTO_AESTAGOUT3_AES_TAG_M                                 0xFFFFFFFF
#define CRYPTO_AESTAGOUT3_AES_TAG_S                                          0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN1
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[63:32]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN is 1. If the HASHIOBUFCTRL.RFD_IN is 0, the engine is
// busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN1_HASH_DATA_IN_W                                   32
#define CRYPTO_HASHDATAIN1_HASH_DATA_IN_M                           0xFFFFFFFF
#define CRYPTO_HASHDATAIN1_HASH_DATA_IN_S                                    0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN2
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[95:64]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN is 1. If the HASHIOBUFCTRL.RFD_IN is 0, the engine is
// busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN2_HASH_DATA_IN_W                                   32
#define CRYPTO_HASHDATAIN2_HASH_DATA_IN_M                           0xFFFFFFFF
#define CRYPTO_HASHDATAIN2_HASH_DATA_IN_S                                    0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN3
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[127:96]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when the rfd_in bit of
// the HASH_IO_BUF_CTRL register is high. If the rfd_in bit is 0, the engine is
// busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASH_IO_BUF_CTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN3_HASH_DATA_IN_W                                   32
#define CRYPTO_HASHDATAIN3_HASH_DATA_IN_M                           0xFFFFFFFF
#define CRYPTO_HASHDATAIN3_HASH_DATA_IN_S                                    0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN4
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[159:128]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is '1'. If the HASHIOBUFCTRL.RFD_IN  is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN4_HASH_DATA_IN_W                                   32
#define CRYPTO_HASHDATAIN4_HASH_DATA_IN_M                           0xFFFFFFFF
#define CRYPTO_HASHDATAIN4_HASH_DATA_IN_S                                    0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN5
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[191:160]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN5_HASH_DATA_IN_W                                   32
#define CRYPTO_HASHDATAIN5_HASH_DATA_IN_M                           0xFFFFFFFF
#define CRYPTO_HASHDATAIN5_HASH_DATA_IN_S                                    0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN6
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[223:192]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN6_HASH_DATA_IN_W                                   32
#define CRYPTO_HASHDATAIN6_HASH_DATA_IN_M                           0xFFFFFFFF
#define CRYPTO_HASHDATAIN6_HASH_DATA_IN_S                                    0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN7
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[255:224]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN7_HASH_DATA_IN_W                                   32
#define CRYPTO_HASHDATAIN7_HASH_DATA_IN_M                           0xFFFFFFFF
#define CRYPTO_HASHDATAIN7_HASH_DATA_IN_S                                    0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN8
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[287:256]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN8_HASH_DATA_IN_W                                   32
#define CRYPTO_HASHDATAIN8_HASH_DATA_IN_M                           0xFFFFFFFF
#define CRYPTO_HASHDATAIN8_HASH_DATA_IN_S                                    0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN9
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[319:288]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN9_HASH_DATA_IN_W                                   32
#define CRYPTO_HASHDATAIN9_HASH_DATA_IN_M                           0xFFFFFFFF
#define CRYPTO_HASHDATAIN9_HASH_DATA_IN_S                                    0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN10
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[351:320]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN10_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN10_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN10_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN11
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[383:352]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN11_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN11_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN11_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN12
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[415:384]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN12_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN12_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN12_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN13
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[447:416]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN13_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN13_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN13_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN14
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[479:448]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN14_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN14_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN14_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN15
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[511:480]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN15_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN15_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN15_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN16
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[543:512]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN16_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN16_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN16_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN17
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[575:544]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN17_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN17_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN17_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN18
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[607:576]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN18_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN18_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN18_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN19
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[639:608]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN19_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN19_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN19_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN20
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[671:640]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN20_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN20_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN20_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN21
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[703:672]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN21_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN21_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN21_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN22
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[735:704]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN22_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN22_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN22_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN23
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[767:736]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN23_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN23_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN23_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN24
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[799:768]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN24_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN24_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN24_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN25
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[831:800]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN25_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN25_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN25_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN26
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[863:832]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN26_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN26_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN26_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN27
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[895:864]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN27_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN27_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN27_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN28
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[923:896]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN28_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN28_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN28_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN29
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[959:924]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN29_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN29_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN29_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN30
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[991:960]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN30_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN30_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN30_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDATAIN31
//
//*****************************************************************************
// Field:  [31:0] HASH_DATA_IN
//
// HASH_DATA_IN[1023:992]
// These registers must be written with the 512-bit input data. The data lines
// are connected directly to the data input of the hash module and hence into
// the engine's internal data buffer. Writing to each of the registers triggers
// a corresponding 32-bit write enable to the internal buffer.
// Note: The host may only write the input data buffer when
// HASHIOBUFCTRL.RFD_IN  is 1. If the HASHIOBUFCTRL.RFD_IN   is 0, the engine
// is busy with processing. During processing, it is not allowed to write new
// input data.
// For message lengths larger than 64 bytes, multiple blocks of data are
// written to this input buffer using a handshake through flags of the
// HASHIOBUFCTRL register. All blocks except the last are required to be 512
// bits in size. If the last block is not 512 bits long, only the least
// significant bits of data must be written, but they must be padded with 0s to
// the next 32-bit boundary.
// Host read operations from these register addresses return 0s.
#define CRYPTO_HASHDATAIN31_HASH_DATA_IN_W                                  32
#define CRYPTO_HASHDATAIN31_HASH_DATA_IN_M                          0xFFFFFFFF
#define CRYPTO_HASHDATAIN31_HASH_DATA_IN_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHIOBUFCTRL
//
//*****************************************************************************
// Field:     [7] PAD_DMA_MESSAGE
//
// Note: This bit must only be used when data is supplied through the DMA. It
// should not be used when data is supplied through the slave interface.
// This bit indicates whether the hash engine has to pad the message, received
// through the DMA and finalize the hash.
// When set to 1, the hash engine pads the last block using the programmed
// length. After padding, the final hash result is calculated.
// When set to 0, the hash engine treats the last written block as block-size
// aligned and calculates the intermediate digest.
// This bit is automatically cleared when the last DMA data block is arrived in
// the hash engine.
#define CRYPTO_HASHIOBUFCTRL_PAD_DMA_MESSAGE                        0x00000080
#define CRYPTO_HASHIOBUFCTRL_PAD_DMA_MESSAGE_BITN                            7
#define CRYPTO_HASHIOBUFCTRL_PAD_DMA_MESSAGE_M                      0x00000080
#define CRYPTO_HASHIOBUFCTRL_PAD_DMA_MESSAGE_S                               7

// Field:     [6] GET_DIGEST
//
// Note: The bit description below is only applicable when data is sent through
// the slave interface. This bit must be set to 0 when data is received through
// the DMA.
// This bit indicates whether the hash engine should provide the hash digest.
// When provided simultaneously with data_in_av, the hash digest is provided
// after processing the data that is currently in the HASHDATAINn register.
// When provided without data_in_av, the current internal digest buffer value
// is copied to the HASHDIGESTn registers.
// The host must write a 1 to this bit to make the intermediate hash digest
// available.
// Writing 0 to this bit has no effect.
// This bit is automatically cleared (that is, reads 0) when the hash engine
// has processed the contents of the HASHDATAINn register. In the period
// between this bit is set by the host and the actual HASHDATAINn processing,
// this bit reads 1.
#define CRYPTO_HASHIOBUFCTRL_GET_DIGEST                             0x00000040
#define CRYPTO_HASHIOBUFCTRL_GET_DIGEST_BITN                                 6
#define CRYPTO_HASHIOBUFCTRL_GET_DIGEST_M                           0x00000040
#define CRYPTO_HASHIOBUFCTRL_GET_DIGEST_S                                    6

// Field:     [5] PAD_MESSAGE
//
// Note: The bit description below is only applicable when data is sent through
// the slave interface. This bit must be set to 0 when data is received through
// the DMA.
// This bit indicates that the HASHDATAINn registers hold the last data of the
// message and hash padding must be applied.
// The host must write this bit to 1 in order to indicate to the hash engine
// that the HASHDATAINn register currently holds the last data of the message.
// When pad_message is set to 1, the hash engine will add padding bits to the
// data currently in the HASHDATAINn register.
// When the last message block is smaller than 512 bits, the pad_message bit
// must be set to 1 together with the data_in_av bit.
// When the last message block is equal to 512 bits, pad_message may be set
// together with data_in_av. In this case the pad_message bit may also be set
// after the last data block has been written to the hash engine (so when the
// rfd_in bit has become 1 again after writing the last data block).
// Writing 0 to this bit has no effect.
// This bit is automatically cleared (i.e. reads 0) by the hash engine. This
// bit reads 1 between the time it was set by the host and the hash engine
// interpreted its value.
#define CRYPTO_HASHIOBUFCTRL_PAD_MESSAGE                            0x00000020
#define CRYPTO_HASHIOBUFCTRL_PAD_MESSAGE_BITN                                5
#define CRYPTO_HASHIOBUFCTRL_PAD_MESSAGE_M                          0x00000020
#define CRYPTO_HASHIOBUFCTRL_PAD_MESSAGE_S                                   5

// Field:     [2] RFD_IN
//
// Note: The bit description below is only applicable when data is sent through
// the slave interface. This bit can be ignored when data is received through
// the DMA.
// Read-only status of the input buffer of the hash engine.
// When 1, the input buffer of the hash engine can accept new data; the
// HASHDATAINn registers can safely be populated with new data.
// When 0, the input buffer of the hash engine is processing the data that is
// currently in HASHDATAINn; writing new data to these registers is not
// allowed.
#define CRYPTO_HASHIOBUFCTRL_RFD_IN                                 0x00000004
#define CRYPTO_HASHIOBUFCTRL_RFD_IN_BITN                                     2
#define CRYPTO_HASHIOBUFCTRL_RFD_IN_M                               0x00000004
#define CRYPTO_HASHIOBUFCTRL_RFD_IN_S                                        2

// Field:     [1] DATA_IN_AV
//
// Note: The bit description below is only applicable when data is sent through
// the slave interface. This bit must be set to 0 when data is received through
// the DMA.
// This bit indicates that the HASHDATAINn registers contain new input data for
// processing.
// The host must write a 1 to this bit to start processing the data in
// HASHDATAINn; the hash engine will process the new data as soon as it is
// ready for it (rfd_in bit is 1).
// Writing 0 to this bit has no effect.
// This bit is automatically cleared (i.e. reads as 0) when the hash engine
// starts processing the HASHDATAINn contents. This bit reads 1 between the
// time it was set by the host and the hash engine actually starts processing
// the input data block.
#define CRYPTO_HASHIOBUFCTRL_DATA_IN_AV                             0x00000002
#define CRYPTO_HASHIOBUFCTRL_DATA_IN_AV_BITN                                 1
#define CRYPTO_HASHIOBUFCTRL_DATA_IN_AV_M                           0x00000002
#define CRYPTO_HASHIOBUFCTRL_DATA_IN_AV_S                                    1

// Field:     [0] OUTPUT_FULL
//
// Indicates that the output buffer registers (HASHDIGESTn) are available for
// reading by the host.
// When this bit reads 0, the output buffer registers are released; the hash
// engine is allowed to write new data to it. In this case, the registers
// should not be read by the host.
// When this bit reads 1, the hash engine has stored the result of the latest
// hash operation in the output buffer registers. As long as this bit reads 1,
// the host may read output buffer registers and the hash engine is prevented
// from writing new data to the output buffer.
// After retrieving the hash result data from the output buffer, the host must
// write a 1 to this bit to clear it. This makes the digest output buffer
// available for the hash engine to store new hash results.
// Writing 0 to this bit has no effect.
// Note: If this bit is asserted (1) no new operation should be started before
// the digest is retrieved from the hash engine and this bit is cleared (0).
#define CRYPTO_HASHIOBUFCTRL_OUTPUT_FULL                            0x00000001
#define CRYPTO_HASHIOBUFCTRL_OUTPUT_FULL_BITN                                0
#define CRYPTO_HASHIOBUFCTRL_OUTPUT_FULL_M                          0x00000001
#define CRYPTO_HASHIOBUFCTRL_OUTPUT_FULL_S                                   0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHMODE
//
//*****************************************************************************
// Field:     [6] SHA384_MODE
//
// The host must write this bit with 1  prior to  processing a SHA 384 session.
#define CRYPTO_HASHMODE_SHA384_MODE                                 0x00000040
#define CRYPTO_HASHMODE_SHA384_MODE_BITN                                     6
#define CRYPTO_HASHMODE_SHA384_MODE_M                               0x00000040
#define CRYPTO_HASHMODE_SHA384_MODE_S                                        6

// Field:     [5] SHA512_MODE
//
// The host must write this bit with 1  prior to  processing a SHA 512 session.
#define CRYPTO_HASHMODE_SHA512_MODE                                 0x00000020
#define CRYPTO_HASHMODE_SHA512_MODE_BITN                                     5
#define CRYPTO_HASHMODE_SHA512_MODE_M                               0x00000020
#define CRYPTO_HASHMODE_SHA512_MODE_S                                        5

// Field:     [4] SHA224_MODE
//
// The host must write this bit with 1  prior to  processing a SHA 224 session.
#define CRYPTO_HASHMODE_SHA224_MODE                                 0x00000010
#define CRYPTO_HASHMODE_SHA224_MODE_BITN                                     4
#define CRYPTO_HASHMODE_SHA224_MODE_M                               0x00000010
#define CRYPTO_HASHMODE_SHA224_MODE_S                                        4

// Field:     [3] SHA256_MODE
//
// The host must write this bit with 1  prior to  processing a SHA 256 session.
#define CRYPTO_HASHMODE_SHA256_MODE                                 0x00000008
#define CRYPTO_HASHMODE_SHA256_MODE_BITN                                     3
#define CRYPTO_HASHMODE_SHA256_MODE_M                               0x00000008
#define CRYPTO_HASHMODE_SHA256_MODE_S                                        3

// Field:     [0] NEW_HASH
//
// When set to 1, it indicates that the hash engine must start processing a new
// hash session. The [HASHDIGESTn.* ] registers will automatically be loaded
// with the initial hash algorithm constants of the selected hash algorithm.
// When this bit is 0 while the hash processing is started, the initial hash
// algorithm constants are not loaded in the HASHDIGESTn registers. The hash
// engine will start processing with the digest that is currently in its
// internal HASHDIGESTn registers.
// This bit is automatically cleared when hash processing is started.
#define CRYPTO_HASHMODE_NEW_HASH                                    0x00000001
#define CRYPTO_HASHMODE_NEW_HASH_BITN                                        0
#define CRYPTO_HASHMODE_NEW_HASH_M                                  0x00000001
#define CRYPTO_HASHMODE_NEW_HASH_S                                           0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHINLENL
//
//*****************************************************************************
// Field:  [31:0] LENGTH_IN
//
// LENGTH_IN[31:0]
// Message length registers. The content of these registers is used by the hash
// engine during the message padding phase of the hash session. The data lines
// of this registers are directly connected to the interface of the hash
// engine.
// For a write operation by the host, these registers should be written with
// the message length in bits.
//
// Final hash operations:
// The total input data length must be programmed for new hash operations that
// require finalization (padding). The input data must be provided through the
// slave or DMA interface.
//
// Continued hash operations (finalized):
// For continued hash operations that require finalization, the total message
// length must be programmed, including the length of previously hashed data
// that corresponds to the written input digest.
//
// Non-final hash operations:
// For hash operations that do not require finalization (input data length is
// multiple of 512-bits which is SHA-256 data block size), the length field
// does not need to be programmed since not used by the operation.
//
// If the message length in bits is below (2^32-1), then only this register
// needs to be written. The hardware automatically sets HASH_LENGTH_IN_H to 0s
// in this case.
// The host may write the length register at any time during the hash session
// when the HASHIOBUFCTRL.RFD_IN is high. The length register must be written
// before the last data of the active hash session is written into the hash
// engine.
// host read operations from these register locations will return 0s.
// Note: When getting data from DMA, this register must be programmed before
// DMA is programmed to start.
#define CRYPTO_HASHINLENL_LENGTH_IN_W                                       32
#define CRYPTO_HASHINLENL_LENGTH_IN_M                               0xFFFFFFFF
#define CRYPTO_HASHINLENL_LENGTH_IN_S                                        0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHINLENH
//
//*****************************************************************************
// Field:  [31:0] LENGTH_IN
//
// LENGTH_IN[63:32]
// Message length registers. The content of these registers is used by the hash
// engine during the message padding phase of the hash session. The data lines
// of this registers are directly connected to the interface of the hash
// engine.
// For a write operation by the host, these registers should be written with
// the message length in bits.
//
// Final hash operations:
// The total input data length must be programmed for new hash operations that
// require finalization (padding). The input data must be provided through the
// slave or DMA interface.
//
// Continued hash operations (finalized):
// For continued hash operations that require finalization, the total message
// length must be programmed, including the length of previously hashed data
// that corresponds to the written input digest.
//
// Non-final hash operations:
// For hash operations that do not require finalization (input data length is
// multiple of 512-bits which is SHA-256 data block size), the length field
// does not need to be programmed since not used by the operation.
//
// If the message length in bits is below (2^32-1), then only HASHINLENL needs
// to be written. The hardware automatically sets HASH_LENGTH_IN_H to 0s in
// this case.
// The host may write the length register at any time during the hash session
// when the HASHIOBUFCTRL.RFD_IN is high. The length register must be written
// before the last data of the active hash session is written into the hash
// engine.
// host read operations from these register locations will return 0s.
// Note: When getting data from DMA, this register must be programmed before
// DMA is programmed to start.
#define CRYPTO_HASHINLENH_LENGTH_IN_W                                       32
#define CRYPTO_HASHINLENH_LENGTH_IN_M                               0xFFFFFFFF
#define CRYPTO_HASHINLENH_LENGTH_IN_S                                        0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTA
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[31:0]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTA_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTA_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTA_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTB
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[63:32]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTB_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTB_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTB_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTC
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[95:64]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTC_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTC_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTC_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTD
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[127:96]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTD_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTD_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTD_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTE
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[159:128]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTE_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTE_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTE_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTF
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[191:160]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTF_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTF_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTF_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTG
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[223:192]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTG_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTG_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTG_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTH
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[255:224]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTH_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTH_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTH_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTI
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[287:256]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTI_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTI_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTI_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTJ
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[319:288]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTJ_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTJ_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTJ_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTK
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[351:320]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTK_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTK_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTK_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTL
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[383:352]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTL_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTL_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTL_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTM
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[415:384]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTM_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTM_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTM_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTN
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[447:416]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTN_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTN_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTN_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTO
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[479:448]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTO_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTO_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTO_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_HASHDIGESTP
//
//*****************************************************************************
// Field:  [31:0] HASH_DIGEST
//
// HASH_DIGEST[511:480]
// Hash digest registers
// Write operation:
//
// Continued hash:
// These registers should be written with the context data, before the start of
// a resumed hash session (the HASHMODE.NEW_HASH bit is 0 when starting a hash
// session).
//
// New hash:
// When initiating a new hash session (theHASHMODE.NEW_HASH bit is 1), the
// internal digest registers are automatically set to the SHA-256 algorithm
// constant and these register should not be written.
//
// Reading from these registers provides the intermediate hash result
// (non-final hash operation) or the final hash result (final hash operation)
// after data processing.
#define CRYPTO_HASHDIGESTP_HASH_DIGEST_W                                    32
#define CRYPTO_HASHDIGESTP_HASH_DIGEST_M                            0xFFFFFFFF
#define CRYPTO_HASHDIGESTP_HASH_DIGEST_S                                     0

//*****************************************************************************
//
// Register: CRYPTO_O_ALGSEL
//
//*****************************************************************************
// Field:    [32] HASH_SHA_512
//
// If set to one, selects the hash engine in 512B mode as destination for the
// DMA
// The maximum transfer size to DMA engine is set to 64 bytes for reading and
// 32 bytes for writing (the latter is only applicable if the hash result is
// written out through the DMA).
#define CRYPTO_ALGSEL_HASH_SHA_512                                  0x100000000
#define CRYPTO_ALGSEL_HASH_SHA_512_BITN                                     32
#define CRYPTO_ALGSEL_HASH_SHA_512_M                                0x100000000
#define CRYPTO_ALGSEL_HASH_SHA_512_S                                        32

// Field:    [31] TAG
//
// If this bit is cleared to 0, the DMA operation involves only data.
// If this bit is set, the DMA operation includes a TAG (Authentication Result
// / Digest).
// For SHA-256 operation, a DMA must be set up for both input data and TAG. For
// any other selected module, setting this bit only allows a DMA that reads the
// TAG. No data allowed to be transferred to or from the selected module via
// the DMA.
#define CRYPTO_ALGSEL_TAG                                           0x80000000
#define CRYPTO_ALGSEL_TAG_BITN                                              31
#define CRYPTO_ALGSEL_TAG_M                                         0x80000000
#define CRYPTO_ALGSEL_TAG_S                                                 31

// Field:     [2] HASH_SHA_256
//
// If set to one, selects the hash engine in 256B mode as destination for the
// DMA
// The maximum transfer size to DMA engine is set to 64 bytes for reading and
// 32 bytes for writing (the latter is only applicable if the hash result is
// written out through the DMA).
#define CRYPTO_ALGSEL_HASH_SHA_256                                  0x00000004
#define CRYPTO_ALGSEL_HASH_SHA_256_BITN                                      2
#define CRYPTO_ALGSEL_HASH_SHA_256_M                                0x00000004
#define CRYPTO_ALGSEL_HASH_SHA_256_S                                         2

// Field:     [1] AES
//
// If set to one, selects the AES engine as source/destination for the DMA
// The read and write maximum transfer size to the DMA engine is set to 16
// bytes.
#define CRYPTO_ALGSEL_AES                                           0x00000002
#define CRYPTO_ALGSEL_AES_BITN                                               1
#define CRYPTO_ALGSEL_AES_M                                         0x00000002
#define CRYPTO_ALGSEL_AES_S                                                  1

// Field:     [0] KEY_STORE
//
// If set to one, selects the Key Store as destination for the DMA
// The maximum transfer size to DMA engine is set to 32 bytes (however
// transfers of 16, 24 and 32 bytes are allowed)
#define CRYPTO_ALGSEL_KEY_STORE                                     0x00000001
#define CRYPTO_ALGSEL_KEY_STORE_BITN                                         0
#define CRYPTO_ALGSEL_KEY_STORE_M                                   0x00000001
#define CRYPTO_ALGSEL_KEY_STORE_S                                            0

//*****************************************************************************
//
// Register: CRYPTO_O_DMAPROTCTL
//
//*****************************************************************************
// Field:     [0] PROT_EN
//
// Select AHB transfer protection control for DMA transfers using the key store
// area as destination.
// 0 : transfers use 'USER' type access.
// 1 : transfers use 'PRIVILEGED' type access.
#define CRYPTO_DMAPROTCTL_PROT_EN                                   0x00000001
#define CRYPTO_DMAPROTCTL_PROT_EN_BITN                                       0
#define CRYPTO_DMAPROTCTL_PROT_EN_M                                 0x00000001
#define CRYPTO_DMAPROTCTL_PROT_EN_S                                          0

//*****************************************************************************
//
// Register: CRYPTO_O_SWRESET
//
//*****************************************************************************
// Field:     [0] SW_RESET
//
// If this bit is set to 1, the following modules are reset:
// - Master control internal state is reset. That includes interrupt, error
// status register, and result available interrupt generation FSM.
// - Key store module state is reset. That includes clearing the written area
// flags; therefore, the keys must be reloaded to the key store module.
// Writing 0 has no effect.
// The bit is self cleared after executing the reset.
#define CRYPTO_SWRESET_SW_RESET                                     0x00000001
#define CRYPTO_SWRESET_SW_RESET_BITN                                         0
#define CRYPTO_SWRESET_SW_RESET_M                                   0x00000001
#define CRYPTO_SWRESET_SW_RESET_S                                            0

//*****************************************************************************
//
// Register: CRYPTO_O_IRQTYPE
//
//*****************************************************************************
// Field:     [0] LEVEL
//
// If this bit is 0, the interrupt output is a pulse.
// If this bit is set to 1, the interrupt is a level interrupt that must be
// cleared by writing the interrupt clear register.
// This bit is applicable for both interrupt output signals.
#define CRYPTO_IRQTYPE_LEVEL                                        0x00000001
#define CRYPTO_IRQTYPE_LEVEL_BITN                                            0
#define CRYPTO_IRQTYPE_LEVEL_M                                      0x00000001
#define CRYPTO_IRQTYPE_LEVEL_S                                               0

//*****************************************************************************
//
// Register: CRYPTO_O_IRQEN
//
//*****************************************************************************
// Field:     [1] DMA_IN_DONE
//
// If this bit is set to 0, the DMA input done (irq_dma_in_done) interrupt
// output is disabled and remains 0.
// If this bit is set to 1, the DMA input done interrupt output is enabled.
#define CRYPTO_IRQEN_DMA_IN_DONE                                    0x00000002
#define CRYPTO_IRQEN_DMA_IN_DONE_BITN                                        1
#define CRYPTO_IRQEN_DMA_IN_DONE_M                                  0x00000002
#define CRYPTO_IRQEN_DMA_IN_DONE_S                                           1

// Field:     [0] RESULT_AVAIL
//
// If this bit is set to 0, the result available (irq_result_av) interrupt
// output is disabled and remains 0.
// If this bit is set to 1, the result available interrupt output is enabled.
#define CRYPTO_IRQEN_RESULT_AVAIL                                   0x00000001
#define CRYPTO_IRQEN_RESULT_AVAIL_BITN                                       0
#define CRYPTO_IRQEN_RESULT_AVAIL_M                                 0x00000001
#define CRYPTO_IRQEN_RESULT_AVAIL_S                                          0

//*****************************************************************************
//
// Register: CRYPTO_O_IRQCLR
//
//*****************************************************************************
// Field:    [31] DMA_BUS_ERR
//
// If 1 is written to this bit, the DMA bus error status is cleared.
// Writing 0 has no effect.
#define CRYPTO_IRQCLR_DMA_BUS_ERR                                   0x80000000
#define CRYPTO_IRQCLR_DMA_BUS_ERR_BITN                                      31
#define CRYPTO_IRQCLR_DMA_BUS_ERR_M                                 0x80000000
#define CRYPTO_IRQCLR_DMA_BUS_ERR_S                                         31

// Field:    [30] KEY_ST_WR_ERR
//
// If 1 is written to this bit, the key store write error status is cleared.
// Writing 0 has no effect.
#define CRYPTO_IRQCLR_KEY_ST_WR_ERR                                 0x40000000
#define CRYPTO_IRQCLR_KEY_ST_WR_ERR_BITN                                    30
#define CRYPTO_IRQCLR_KEY_ST_WR_ERR_M                               0x40000000
#define CRYPTO_IRQCLR_KEY_ST_WR_ERR_S                                       30

// Field:    [29] KEY_ST_RD_ERR
//
// If 1 is written to this bit, the key store read error status is cleared.
// Writing 0 has no effect.
#define CRYPTO_IRQCLR_KEY_ST_RD_ERR                                 0x20000000
#define CRYPTO_IRQCLR_KEY_ST_RD_ERR_BITN                                    29
#define CRYPTO_IRQCLR_KEY_ST_RD_ERR_M                               0x20000000
#define CRYPTO_IRQCLR_KEY_ST_RD_ERR_S                                       29

// Field:     [1] DMA_IN_DONE
//
// If 1 is written to this bit, the DMA in done (irq_dma_in_done) interrupt
// output is cleared.
// Writing 0 has no effect.
// Note that clearing an interrupt makes sense only if the interrupt output is
// programmed as level (refer to IRQTYPE).
#define CRYPTO_IRQCLR_DMA_IN_DONE                                   0x00000002
#define CRYPTO_IRQCLR_DMA_IN_DONE_BITN                                       1
#define CRYPTO_IRQCLR_DMA_IN_DONE_M                                 0x00000002
#define CRYPTO_IRQCLR_DMA_IN_DONE_S                                          1

// Field:     [0] RESULT_AVAIL
//
// If 1 is written to this bit, the result available (irq_result_av) interrupt
// output is cleared.
// Writing 0 has no effect.
// Note that clearing an interrupt makes sense only if the interrupt output is
// programmed as level (refer to IRQTYPE).
#define CRYPTO_IRQCLR_RESULT_AVAIL                                  0x00000001
#define CRYPTO_IRQCLR_RESULT_AVAIL_BITN                                      0
#define CRYPTO_IRQCLR_RESULT_AVAIL_M                                0x00000001
#define CRYPTO_IRQCLR_RESULT_AVAIL_S                                         0

//*****************************************************************************
//
// Register: CRYPTO_O_IRQSET
//
//*****************************************************************************
// Field:     [1] DMA_IN_DONE
//
// If 1 is written to this bit, the DMA data in done (irq_dma_in_done)
// interrupt output is set to one.
// Writing 0 has no effect.
// If the interrupt configuration register is programmed to pulse, clearing the
// DMA data in done (irq_dma_in_done) interrupt is not needed. If it is
// programmed to level, clearing the interrupt output should be done by writing
// the interrupt clear register (IRQCLR.DMA_IN_DONE).
#define CRYPTO_IRQSET_DMA_IN_DONE                                   0x00000002
#define CRYPTO_IRQSET_DMA_IN_DONE_BITN                                       1
#define CRYPTO_IRQSET_DMA_IN_DONE_M                                 0x00000002
#define CRYPTO_IRQSET_DMA_IN_DONE_S                                          1

// Field:     [0] RESULT_AVAIL
//
// If 1 is written to this bit, the result available (irq_result_av) interrupt
// output is set to one.
// Writing 0 has no effect.
// If the interrupt configuration register is programmed to pulse, clearing the
// result available (irq_result_av) interrupt is not needed. If it is
// programmed to level, clearing the interrupt output should be done by writing
// the interrupt clear register (IRQCLR.RESULT_AVAIL).
#define CRYPTO_IRQSET_RESULT_AVAIL                                  0x00000001
#define CRYPTO_IRQSET_RESULT_AVAIL_BITN                                      0
#define CRYPTO_IRQSET_RESULT_AVAIL_M                                0x00000001
#define CRYPTO_IRQSET_RESULT_AVAIL_S                                         0

//*****************************************************************************
//
// Register: CRYPTO_O_IRQSTAT
//
//*****************************************************************************
// Field:    [31] DMA_BUS_ERR
//
// This bit is set when a DMA bus error is detected during a DMA operation. The
// value of this register is held until it is cleared through the
// IRQCLR.DMA_BUS_ERR
// Note: This error is asserted if an error is detected on the AHB master
// interface during a DMA operation.
#define CRYPTO_IRQSTAT_DMA_BUS_ERR                                  0x80000000
#define CRYPTO_IRQSTAT_DMA_BUS_ERR_BITN                                     31
#define CRYPTO_IRQSTAT_DMA_BUS_ERR_M                                0x80000000
#define CRYPTO_IRQSTAT_DMA_BUS_ERR_S                                        31

// Field:    [30] KEY_ST_WR_ERR
//
// This bit is set when a write error is detected during the DMA write
// operation to the key store memory. The value of this register is held until
// it is cleared through the IRQCLR.KEY_ST_WR_ERR register.
// Note: This error is asserted if a DMA operation does not cover a full key
// area or more areas are written than expected.
#define CRYPTO_IRQSTAT_KEY_ST_WR_ERR                                0x40000000
#define CRYPTO_IRQSTAT_KEY_ST_WR_ERR_BITN                                   30
#define CRYPTO_IRQSTAT_KEY_ST_WR_ERR_M                              0x40000000
#define CRYPTO_IRQSTAT_KEY_ST_WR_ERR_S                                      30

// Field:    [29] KEY_ST_RD_ERR
//
// This bit is set when a read error is detected during the read of a key from
// the key store, while copying it to the AES core. The value of this register
// is held until it is cleared through the IRQCLR.KEY_ST_RD_ERR register.
// Note: This error is asserted if a key location is selected in the key store
// that is not available.
#define CRYPTO_IRQSTAT_KEY_ST_RD_ERR                                0x20000000
#define CRYPTO_IRQSTAT_KEY_ST_RD_ERR_BITN                                   29
#define CRYPTO_IRQSTAT_KEY_ST_RD_ERR_M                              0x20000000
#define CRYPTO_IRQSTAT_KEY_ST_RD_ERR_S                                      29

// Field:     [1] DMA_IN_DONE
//
// This read only bit returns the actual DMA data in done (irq_data_in_done)
// interrupt status of the DMA data in done interrupt output pin
// (irq_data_in_done).
#define CRYPTO_IRQSTAT_DMA_IN_DONE                                  0x00000002
#define CRYPTO_IRQSTAT_DMA_IN_DONE_BITN                                      1
#define CRYPTO_IRQSTAT_DMA_IN_DONE_M                                0x00000002
#define CRYPTO_IRQSTAT_DMA_IN_DONE_S                                         1

// Field:     [0] RESULT_AVAIL
//
// This read only bit returns the actual result available (irq_result_av)
// interrupt status of the result available interrupt output pin
// (irq_result_av).
#define CRYPTO_IRQSTAT_RESULT_AVAIL                                 0x00000001
#define CRYPTO_IRQSTAT_RESULT_AVAIL_BITN                                     0
#define CRYPTO_IRQSTAT_RESULT_AVAIL_M                               0x00000001
#define CRYPTO_IRQSTAT_RESULT_AVAIL_S                                        0

//*****************************************************************************
//
// Register: CRYPTO_O_HWVER
//
//*****************************************************************************
// Field: [27:24] HW_MAJOR_VER
//
// Major version number
#define CRYPTO_HWVER_HW_MAJOR_VER_W                                          4
#define CRYPTO_HWVER_HW_MAJOR_VER_M                                 0x0F000000
#define CRYPTO_HWVER_HW_MAJOR_VER_S                                         24

// Field: [23:20] HW_MINOR_VER
//
// Minor version number
#define CRYPTO_HWVER_HW_MINOR_VER_W                                          4
#define CRYPTO_HWVER_HW_MINOR_VER_M                                 0x00F00000
#define CRYPTO_HWVER_HW_MINOR_VER_S                                         20

// Field: [19:16] HW_PATCH_LVL
//
// Patch level
// Starts at 0 at first delivery of this version
#define CRYPTO_HWVER_HW_PATCH_LVL_W                                          4
#define CRYPTO_HWVER_HW_PATCH_LVL_M                                 0x000F0000
#define CRYPTO_HWVER_HW_PATCH_LVL_S                                         16

// Field:  [15:8] VER_NUM_COMPL
//
// These bits simply contain the complement of bits [7:0] (0x87), used by a
// driver to ascertain that the EIP-120t register is indeed read.
#define CRYPTO_HWVER_VER_NUM_COMPL_W                                         8
#define CRYPTO_HWVER_VER_NUM_COMPL_M                                0x0000FF00
#define CRYPTO_HWVER_VER_NUM_COMPL_S                                         8

// Field:   [7:0] VER_NUM
//
// These bits encode the EIP number for the EIP-120t, this field contains the
// value 120 (decimal) or 0x78.
#define CRYPTO_HWVER_VER_NUM_W                                               8
#define CRYPTO_HWVER_VER_NUM_M                                      0x000000FF
#define CRYPTO_HWVER_VER_NUM_S                                               0


#endif // __CRYPTO__
