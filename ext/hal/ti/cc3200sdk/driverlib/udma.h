//*****************************************************************************
//
//  udma.h
//
//  Prototypes and macros for the uDMA controller.
//
//  Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#ifndef __UDMA_H__
#define __UDMA_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! \addtogroup uDMA_Micro_Direct_Memory_Access_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// A structure that defines an entry in the channel control table.  These
// fields are used by the uDMA controller and normally it is not necessary for
// software to directly read or write fields in the table.
//
//*****************************************************************************
typedef struct
{
    //
    // The ending source address of the data transfer.
    //
    volatile void *pvSrcEndAddr;

    //
    // The ending destination address of the data transfer.
    //
    volatile void *pvDstEndAddr;

    //
    // The channel control mode.
    //
    volatile unsigned long ulControl;

    //
    // An unused location.
    //
    volatile unsigned long ulSpare;
}
tDMAControlTable;

//*****************************************************************************
//
//! A helper macro for building scatter-gather task table entries.
//!
//! \param ulTransferCount is the count of items to transfer for this task.
//! \param ulItemSize is the bit size of the items to transfer for this task.
//! \param ulSrcIncrement is the bit size increment for source data.
//! \param pvSrcAddr is the starting address of the data to transfer.
//! \param ulDstIncrement is the bit size increment for destination data.
//! \param pvDstAddr is the starting address of the destination data.
//! \param ulArbSize is the arbitration size to use for the transfer task.
//! \param ulMode is the transfer mode for this task.
//!
//! This macro is intended to be used to help populate a table of uDMA tasks
//! for a scatter-gather transfer.  This macro will calculate the values for
//! the fields of a task structure entry based on the input parameters.
//!
//! There are specific requirements for the values of each parameter.  No
//! checking is done so it is up to the caller to ensure that correct values
//! are used for the parameters.
//!
//! The \e ulTransferCount parameter is the number of items that will be
//! transferred by this task.  It must be in the range 1-1024.
//!
//! The \e ulItemSize parameter is the bit size of the transfer data.  It must
//! be one of \b UDMA_SIZE_8, \b UDMA_SIZE_16, or \b UDMA_SIZE_32.
//!
//! The \e ulSrcIncrement parameter is the increment size for the source data.
//! It must be one of \b UDMA_SRC_INC_8, \b UDMA_SRC_INC_16,
//! \b UDMA_SRC_INC_32, or \b UDMA_SRC_INC_NONE.
//!
//! The \e pvSrcAddr parameter is a void pointer to the beginning of the source
//! data.
//!
//! The \e ulDstIncrement parameter is the increment size for the destination
//! data.  It must be one of \b UDMA_DST_INC_8, \b UDMA_DST_INC_16,
//! \b UDMA_DST_INC_32, or \b UDMA_DST_INC_NONE.
//!
//! The \e pvDstAddr parameter is a void pointer to the beginning of the
//! location where the data will be transferred.
//!
//! The \e ulArbSize parameter is the arbitration size for the transfer, and
//! must be one of \b UDMA_ARB_1, \b UDMA_ARB_2, \b UDMA_ARB_4, and so on
//! up to \b UDMA_ARB_1024.  This is used to select the arbitration size in
//! powers of 2, from 1 to 1024.
//!
//! The \e ulMode parameter is the mode to use for this transfer task.  It
//! must be one of \b UDMA_MODE_BASIC, \b UDMA_MODE_AUTO,
//! \b UDMA_MODE_MEM_SCATTER_GATHER, or \b UDMA_MODE_PER_SCATTER_GATHER.  Note
//! that normally all tasks will be one of the scatter-gather modes while the
//! last task is a task list will be AUTO or BASIC.
//!
//! This macro is intended to be used to initialize individual entries of
//! a structure of tDMAControlTable type, like this:
//!
//! \verbatim
//!     tDMAControlTable MyTaskList[] =
//!     {
//!         uDMATaskStructEntry(Task1Count, UDMA_SIZE_8,
//!                             UDMA_SRC_INC_8, MySourceBuf,
//!                             UDMA_DST_INC_8, MyDestBuf,
//!                             UDMA_ARB_8, UDMA_MODE_MEM_SCATTER_GATHER),
//!         uDMATaskStructEntry(Task2Count, ... ),
//!     }
//! \endverbatim
//!
//! \return Nothing; this is not a function.
//
//*****************************************************************************
#define uDMATaskStructEntry(ulTransferCount,                                  \
                            ulItemSize,                                       \
                            ulSrcIncrement,                                   \
                            pvSrcAddr,                                        \
                            ulDstIncrement,                                   \
                            pvDstAddr,                                        \
                            ulArbSize,                                        \
                            ulMode)                                           \
    {                                                                         \
        (((ulSrcIncrement) == UDMA_SRC_INC_NONE) ? (void *)(pvSrcAddr) :      \
            ((void *)(&((unsigned char *)(pvSrcAddr))[((ulTransferCount) <<   \
                                         ((ulSrcIncrement) >> 26)) - 1]))),   \
            (((ulDstIncrement) == UDMA_DST_INC_NONE) ? (void *)(pvDstAddr) :  \
            ((void *)(&((unsigned char *)(pvDstAddr))[((ulTransferCount) <<   \
                                         ((ulDstIncrement) >> 30)) - 1]))),   \
        (ulSrcIncrement) | (ulDstIncrement) | (ulItemSize) | (ulArbSize) |    \
        (((ulTransferCount) - 1) << 4) |                                      \
        ((((ulMode) == UDMA_MODE_MEM_SCATTER_GATHER) ||                       \
          ((ulMode) == UDMA_MODE_PER_SCATTER_GATHER)) ?                       \
                (ulMode) | UDMA_MODE_ALT_SELECT : (ulMode)), 0                \
    }

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Flags that can be passed to uDMAChannelAttributeEnable(),
// uDMAChannelAttributeDisable(), and returned from uDMAChannelAttributeGet().
//
//*****************************************************************************
#define UDMA_ATTR_USEBURST      0x00000001
#define UDMA_ATTR_ALTSELECT     0x00000002
#define UDMA_ATTR_HIGH_PRIORITY 0x00000004
#define UDMA_ATTR_REQMASK       0x00000008
#define UDMA_ATTR_ALL           0x0000000F

//*****************************************************************************
//
// DMA control modes that can be passed to uDMAModeSet() and returned
// uDMAModeGet().
//
//*****************************************************************************
#define UDMA_MODE_STOP          0x00000000
#define UDMA_MODE_BASIC         0x00000001
#define UDMA_MODE_AUTO          0x00000002
#define UDMA_MODE_PINGPONG      0x00000003
#define UDMA_MODE_MEM_SCATTER_GATHER                                          \
                                0x00000004
#define UDMA_MODE_PER_SCATTER_GATHER                                          \
                                0x00000006
#define UDMA_MODE_ALT_SELECT    0x00000001

//*****************************************************************************
//
// Flags to be OR'd with the channel ID to indicate if the primary or alternate
// control structure should be used.
//
//*****************************************************************************
#define UDMA_PRI_SELECT         0x00000000
#define UDMA_ALT_SELECT         0x00000020

//*****************************************************************************
//
// uDMA interrupt sources, to be passed to uDMAIntRegister() and
// uDMAIntUnregister().
//
//*****************************************************************************
#define UDMA_INT_SW             INT_UDMA
#define UDMA_INT_ERR            INT_UDMAERR

//*****************************************************************************

//*****************************************************************************
//
// Channel configuration values that can be passed to uDMAControlSet().
//
//*****************************************************************************
#define UDMA_DST_INC_8          0x00000000
#define UDMA_DST_INC_16         0x40000000
#define UDMA_DST_INC_32         0x80000000
#define UDMA_DST_INC_NONE       0xc0000000
#define UDMA_SRC_INC_8          0x00000000
#define UDMA_SRC_INC_16         0x04000000
#define UDMA_SRC_INC_32         0x08000000
#define UDMA_SRC_INC_NONE       0x0c000000
#define UDMA_SIZE_8             0x00000000
#define UDMA_SIZE_16            0x11000000
#define UDMA_SIZE_32            0x22000000
#define UDMA_ARB_1              0x00000000
#define UDMA_ARB_2              0x00004000
#define UDMA_ARB_4              0x00008000
#define UDMA_ARB_8              0x0000c000
#define UDMA_ARB_16             0x00010000
#define UDMA_ARB_32             0x00014000
#define UDMA_ARB_64             0x00018000
#define UDMA_ARB_128            0x0001c000
#define UDMA_ARB_256            0x00020000
#define UDMA_ARB_512            0x00024000
#define UDMA_ARB_1024           0x00028000
#define UDMA_NEXT_USEBURST      0x00000008

//*****************************************************************************
//
// Values that can be passed to uDMAChannelAssign() to select peripheral
// mapping for each channel.  The channels named RESERVED may be assigned
// to a peripheral in future parts.
//
//*****************************************************************************
//
// Channel 0
//
#define UDMA_CH0_TIMERA0_A      0x00000000
#define UDMA_CH0_SHAMD5_CIN     0x00010000
#define UDMA_CH0_SW             0x00030000

//
// Channel 1
//
#define UDMA_CH1_TIMERA0_B      0x00000001
#define UDMA_CH1_SHAMD5_DIN     0x00010001
#define UDMA_CH1_SW             0x00030001

//
// Channel 2
//
#define UDMA_CH2_TIMERA1_A      0x00000002
#define UDMA_CH2_SHAMD5_COUT    0x00010002
#define UDMA_CH2_SW             0x00030002

//
// Channel 3
//
#define UDMA_CH3_TIMERA1_B      0x00000003
#define UDMA_CH3_DES_CIN        0x00010003
#define UDMA_CH3_SW             0x00030003

//
// Channel 4
//
#define UDMA_CH4_TIMERA2_A      0x00000004
#define UDMA_CH4_DES_DIN        0x00010004
#define UDMA_CH4_I2S_RX         0x00020004
#define UDMA_CH4_SW             0x00030004

//
// Channel 5
//
#define UDMA_CH5_TIMERA2_B      0x00000005
#define UDMA_CH5_DES_DOUT       0x00010005
#define UDMA_CH5_I2S_TX         0x00020005
#define UDMA_CH5_SW             0x00030005

//
// Channel 6
//
#define UDMA_CH6_TIMERA3_A      0x00000006
#define UDMA_CH6_GSPI_RX        0x00010006
#define UDMA_CH6_GPIOA2         0x00020006
#define UDMA_CH6_SW             0x00030006

//
// Channel 7
//
#define UDMA_CH7_TIMERA3_B      0x00000007
#define UDMA_CH7_GSPI_TX    0x00010007
#define UDMA_CH7_GPIOA3         0x00020007
#define UDMA_CH7_SW             0x00030007


//
// Channel 8
//
#define UDMA_CH8_UARTA0_RX      0x00000008
#define UDMA_CH8_TIMERA0_A      0x00010008
#define UDMA_CH8_TIMERA2_A      0x00020008
#define UDMA_CH8_SW             0x00030008


//
// Channel 9
//
#define UDMA_CH9_UARTA0_TX      0x00000009
#define UDMA_CH9_TIMERA0_B      0x00010009
#define UDMA_CH9_TIMERA2_B      0x00020009
#define UDMA_CH9_SW             0x00030009


//
// Channel 10
//
#define UDMA_CH10_UARTA1_RX     0x0000000A
#define UDMA_CH10_TIMERA1_A     0x0001000A
#define UDMA_CH10_TIMERA3_A     0x0002000A
#define UDMA_CH10_SW            0x0003000A

//
// Channel 11
//
#define UDMA_CH11_UARTA1_TX     0x0000000B
#define UDMA_CH11_TIMERA1_B     0x0001000B
#define UDMA_CH11_TIMERA3_B     0x0002000B
#define UDMA_CH11_SW            0x0003000B


//
// Channel 12
//
#define UDMA_CH12_LSPI_RX       0x0000000C
#define UDMA_CH12_SW            0x0003000C


//
// Channel 13
//
#define UDMA_CH13_LSPI_TX       0x0000000D
#define UDMA_CH13_SW		0x0003000D


//
// Channel 14
//
#define UDMA_CH14_ADC_CH0       0x0000000E
#define UDMA_CH14_SDHOST_RX     0x0002000E
#define UDMA_CH14_SW            0x0003000E


//
// Channel 15
//
#define UDMA_CH15_ADC_CH1       0x0000000F
#define UDMA_CH15_SDHOST_TX     0x0002000F
#define UDMA_CH15_SW            0x0003000F


//
// Channel 16
//
#define UDMA_CH16_ADC_CH2       0x00000010
#define UDMA_CH16_TIMERA2_A     0x00010010
#define UDMA_CH16_SW            0x00030010


//
// Channel 17
//
#define UDMA_CH17_ADC_CH3       0x00000011
#define UDMA_CH17_TIMERA2_B     0x00010011
#define UDMA_CH17_SW            0x00030011

//
// Channel 18
//
#define UDMA_CH18_GPIOA0        0x00000012
#define UDMA_CH18_AES_CIN       0x00010012
#define UDMA_CH18_I2S_RX      0x00020012
#define UDMA_CH18_SW            0x00030012


//
// Channel 19
//
#define UDMA_CH19_GPOIA1        0x00000013
#define UDMA_CH19_AES_COUT      0x00010013
#define UDMA_CH19_I2S_TX      0x00020013
#define UDMA_CH19_SW            0x00030013


//
// Channel 20
//
#define UDMA_CH20_GPIOA2        0x00000014
#define UDMA_CH20_AES_DIN       0x00010014
#define UDMA_CH20_SW            0x00030014


//
// Channel 21
//
#define UDMA_CH21_GPIOA3        0x00000015
#define UDMA_CH21_AES_DOUT      0x00010015
#define UDMA_CH21_SW            0x00030015


//
// Channel 22
//
#define UDMA_CH22_CAMERA        0x00000016
#define UDMA_CH22_GPIOA4        0x00010016
#define UDMA_CH22_SW            0x00030016


//
// Channel 23
//
#define UDMA_CH23_SDHOST_RX     0x00000017
#define UDMA_CH23_TIMERA3_A     0x00010017
#define UDMA_CH23_TIMERA2_A     0x00020017
#define UDMA_CH23_SW            0x00030017


//
// Channel 24
//
#define UDMA_CH24_SDHOST_TX     0x00000018
#define UDMA_CH24_TIMERA3_B     0x00010018
#define UDMA_CH24_TIMERA2_B     0x00020018
#define UDMA_CH24_SW            0x00030018


//
// Channel 25
//
#define UDMA_CH25_SSPI_RX       0x00000019
#define UDMA_CH25_I2CA0_RX      0x00010019
#define UDMA_CH25_SW            0x00030019


//
// Channel 26
//
#define UDMA_CH26_SSPI_TX       0x0000001A
#define UDMA_CH26_I2CA0_TX      0x0001001A
#define UDMA_CH26_SW            0x0003001A


//
// Channel 27
//
#define UDMA_CH27_GPIOA0        0x0001001B
#define UDMA_CH27_SW            0x0003001B


//
// Channel 28
//
#define UDMA_CH28_GPIOA1        0x0001001C
#define UDMA_CH28_SW            0x0003001C


//
// Channel 29
//
#define UDMA_CH29_GPIOA4        0x0000001D
#define UDMA_CH29_SW            0x0003001D


//
// Channel 30
//
#define UDMA_CH30_GSPI_RX       0x0000001E
#define UDMA_CH30_SDHOST_RX     0x0001001E
#define UDMA_CH30_I2CA0_RX      0x0002001E
#define UDMA_CH30_SW            0x0003001E


//
// Channel 31
//
#define UDMA_CH31_GSPI_TX       0x0000001F
#define UDMA_CH31_SDHOST_TX     0x0001001F
#define UDMA_CH31_I2CA0_RX      0x0002001F
#define UDMA_CH31_SW            0x0003001F

//*****************************************************************************
//
// The following are defines for the Micro Direct Memory Access (uDMA) offsets.
//
//*****************************************************************************
#define UDMA_O_SRCENDP          0x00000000  // DMA Channel Source Address End
                                            // Pointer
#define UDMA_O_DSTENDP          0x00000004  // DMA Channel Destination Address
                                            // End Pointer
#define UDMA_O_CHCTL            0x00000008  // DMA Channel Control Word

//*****************************************************************************
//
// The following are defines for the bit fields in the UDMA_O_SRCENDP register.
//
//*****************************************************************************
#define UDMA_SRCENDP_ADDR_M     0xFFFFFFFF  // Source Address End Pointer
#define UDMA_SRCENDP_ADDR_S     0

//*****************************************************************************
//
// The following are defines for the bit fields in the UDMA_O_DSTENDP register.
//
//*****************************************************************************
#define UDMA_DSTENDP_ADDR_M     0xFFFFFFFF  // Destination Address End Pointer
#define UDMA_DSTENDP_ADDR_S     0

//*****************************************************************************
//
// The following are defines for the bit fields in the UDMA_O_CHCTL register.
//
//*****************************************************************************
#define UDMA_CHCTL_DSTINC_M     0xC0000000  // Destination Address Increment
#define UDMA_CHCTL_DSTINC_8     0x00000000  // Byte
#define UDMA_CHCTL_DSTINC_16    0x40000000  // Half-word
#define UDMA_CHCTL_DSTINC_32    0x80000000  // Word
#define UDMA_CHCTL_DSTINC_NONE  0xC0000000  // No increment
#define UDMA_CHCTL_DSTSIZE_M    0x30000000  // Destination Data Size
#define UDMA_CHCTL_DSTSIZE_8    0x00000000  // Byte
#define UDMA_CHCTL_DSTSIZE_16   0x10000000  // Half-word
#define UDMA_CHCTL_DSTSIZE_32   0x20000000  // Word
#define UDMA_CHCTL_SRCINC_M     0x0C000000  // Source Address Increment
#define UDMA_CHCTL_SRCINC_8     0x00000000  // Byte
#define UDMA_CHCTL_SRCINC_16    0x04000000  // Half-word
#define UDMA_CHCTL_SRCINC_32    0x08000000  // Word
#define UDMA_CHCTL_SRCINC_NONE  0x0C000000  // No increment
#define UDMA_CHCTL_SRCSIZE_M    0x03000000  // Source Data Size
#define UDMA_CHCTL_SRCSIZE_8    0x00000000  // Byte
#define UDMA_CHCTL_SRCSIZE_16   0x01000000  // Half-word
#define UDMA_CHCTL_SRCSIZE_32   0x02000000  // Word
#define UDMA_CHCTL_ARBSIZE_M    0x0003C000  // Arbitration Size
#define UDMA_CHCTL_ARBSIZE_1    0x00000000  // 1 Transfer
#define UDMA_CHCTL_ARBSIZE_2    0x00004000  // 2 Transfers
#define UDMA_CHCTL_ARBSIZE_4    0x00008000  // 4 Transfers
#define UDMA_CHCTL_ARBSIZE_8    0x0000C000  // 8 Transfers
#define UDMA_CHCTL_ARBSIZE_16   0x00010000  // 16 Transfers
#define UDMA_CHCTL_ARBSIZE_32   0x00014000  // 32 Transfers
#define UDMA_CHCTL_ARBSIZE_64   0x00018000  // 64 Transfers
#define UDMA_CHCTL_ARBSIZE_128  0x0001C000  // 128 Transfers
#define UDMA_CHCTL_ARBSIZE_256  0x00020000  // 256 Transfers
#define UDMA_CHCTL_ARBSIZE_512  0x00024000  // 512 Transfers
#define UDMA_CHCTL_ARBSIZE_1024 0x00028000  // 1024 Transfers
#define UDMA_CHCTL_XFERSIZE_M   0x00003FF0  // Transfer Size (minus 1)
#define UDMA_CHCTL_NXTUSEBURST  0x00000008  // Next Useburst
#define UDMA_CHCTL_XFERMODE_M   0x00000007  // uDMA Transfer Mode
#define UDMA_CHCTL_XFERMODE_STOP \
                                0x00000000  // Stop
#define UDMA_CHCTL_XFERMODE_BASIC \
                                0x00000001  // Basic
#define UDMA_CHCTL_XFERMODE_AUTO \
                                0x00000002  // Auto-Request
#define UDMA_CHCTL_XFERMODE_PINGPONG \
                                0x00000003  // Ping-Pong
#define UDMA_CHCTL_XFERMODE_MEM_SG \
                                0x00000004  // Memory Scatter-Gather
#define UDMA_CHCTL_XFERMODE_MEM_SGA \
                                0x00000005  // Alternate Memory Scatter-Gather
#define UDMA_CHCTL_XFERMODE_PER_SG \
                                0x00000006  // Peripheral Scatter-Gather
#define UDMA_CHCTL_XFERMODE_PER_SGA \
                                0x00000007  // Alternate Peripheral
                                            // Scatter-Gather
#define UDMA_CHCTL_XFERSIZE_S   4



//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void uDMAEnable(void);
extern void uDMADisable(void);
extern unsigned long uDMAErrorStatusGet(void);
extern void uDMAErrorStatusClear(void);
extern void uDMAChannelEnable(unsigned long ulChannelNum);
extern void uDMAChannelDisable(unsigned long ulChannelNum);
extern tBoolean uDMAChannelIsEnabled(unsigned long ulChannelNum);
extern void uDMAControlBaseSet(void *pControlTable);
extern void *uDMAControlBaseGet(void);
extern void *uDMAControlAlternateBaseGet(void);
extern void uDMAChannelRequest(unsigned long ulChannelNum);
extern void uDMAChannelAttributeEnable(unsigned long ulChannelNum,
                                       unsigned long ulAttr);
extern void uDMAChannelAttributeDisable(unsigned long ulChannelNum,
                                        unsigned long ulAttr);
extern unsigned long uDMAChannelAttributeGet(unsigned long ulChannelNum);
extern void uDMAChannelControlSet(unsigned long ulChannelStructIndex,
                                  unsigned long ulControl);
extern void uDMAChannelTransferSet(unsigned long ulChannelStructIndex,
                                   unsigned long ulMode, void *pvSrcAddr,
                                   void *pvDstAddr,
                                   unsigned long ulTransferSize);
extern void uDMAChannelScatterGatherSet(unsigned long ulChannelNum,
                                        unsigned ulTaskCount, void *pvTaskList,
                                        unsigned long ulIsPeriphSG);
extern unsigned long uDMAChannelSizeGet(unsigned long ulChannelStructIndex);
extern unsigned long uDMAChannelModeGet(unsigned long ulChannelStructIndex);
extern void uDMAIntRegister(unsigned long ulIntChannel,
                            void (*pfnHandler)(void));
extern void uDMAIntUnregister(unsigned long ulIntChannel);
extern unsigned long uDMAIntStatus(void);
extern void uDMAIntClear(unsigned long ulChanMask);
extern void uDMAChannelAssign(unsigned long ulMapping);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __UDMA_H__
