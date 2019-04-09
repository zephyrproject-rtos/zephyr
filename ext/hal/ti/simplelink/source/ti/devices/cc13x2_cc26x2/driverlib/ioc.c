/******************************************************************************
*  Filename:       ioc.c
*  Revised:        2017-06-05 12:13:49 +0200 (Mon, 05 Jun 2017)
*  Revision:       49096
*
*  Description:    Driver for the IOC.
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#include "ioc.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  IOCPortConfigureSet
    #define IOCPortConfigureSet             NOROM_IOCPortConfigureSet
    #undef  IOCPortConfigureGet
    #define IOCPortConfigureGet             NOROM_IOCPortConfigureGet
    #undef  IOCIOShutdownSet
    #define IOCIOShutdownSet                NOROM_IOCIOShutdownSet
    #undef  IOCIOModeSet
    #define IOCIOModeSet                    NOROM_IOCIOModeSet
    #undef  IOCIOIntSet
    #define IOCIOIntSet                     NOROM_IOCIOIntSet
    #undef  IOCIOEvtSet
    #define IOCIOEvtSet                     NOROM_IOCIOEvtSet
    #undef  IOCIOPortPullSet
    #define IOCIOPortPullSet                NOROM_IOCIOPortPullSet
    #undef  IOCIOHystSet
    #define IOCIOHystSet                    NOROM_IOCIOHystSet
    #undef  IOCIOInputSet
    #define IOCIOInputSet                   NOROM_IOCIOInputSet
    #undef  IOCIOSlewCtrlSet
    #define IOCIOSlewCtrlSet                NOROM_IOCIOSlewCtrlSet
    #undef  IOCIODrvStrengthSet
    #define IOCIODrvStrengthSet             NOROM_IOCIODrvStrengthSet
    #undef  IOCIOPortIdSet
    #define IOCIOPortIdSet                  NOROM_IOCIOPortIdSet
    #undef  IOCIntEnable
    #define IOCIntEnable                    NOROM_IOCIntEnable
    #undef  IOCIntDisable
    #define IOCIntDisable                   NOROM_IOCIntDisable
    #undef  IOCPinTypeGpioInput
    #define IOCPinTypeGpioInput             NOROM_IOCPinTypeGpioInput
    #undef  IOCPinTypeGpioOutput
    #define IOCPinTypeGpioOutput            NOROM_IOCPinTypeGpioOutput
    #undef  IOCPinTypeUart
    #define IOCPinTypeUart                  NOROM_IOCPinTypeUart
    #undef  IOCPinTypeSsiMaster
    #define IOCPinTypeSsiMaster             NOROM_IOCPinTypeSsiMaster
    #undef  IOCPinTypeSsiSlave
    #define IOCPinTypeSsiSlave              NOROM_IOCPinTypeSsiSlave
    #undef  IOCPinTypeI2c
    #define IOCPinTypeI2c                   NOROM_IOCPinTypeI2c
    #undef  IOCPinTypeAux
    #define IOCPinTypeAux                   NOROM_IOCPinTypeAux
#endif

//*****************************************************************************
//
// Set the configuration of an IO port
//
//*****************************************************************************
void
IOCPortConfigureSet(uint32_t ui32IOId, uint32_t ui32PortId,
                    uint32_t ui32IOConfig)
{
    uint32_t ui32Reg;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT(ui32PortId <= IOC_PORT_RFC_GPI1);

    // Get the register address.
    ui32Reg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the port.
    HWREG(ui32Reg) = ui32IOConfig | ui32PortId;
}

//*****************************************************************************
//
// Get the configuration of an IO port
//
//*****************************************************************************
uint32_t
IOCPortConfigureGet(uint32_t ui32IOId)
{
    uint32_t ui32Reg;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);

    // Get the register address.
    ui32Reg = IOC_BASE + ( ui32IOId << 2 );

    // Return the IO configuration.
    return HWREG(ui32Reg);
}

//*****************************************************************************
//
// Set wake-up on an IO port
//
//*****************************************************************************
void
IOCIOShutdownSet(uint32_t ui32IOId, uint32_t ui32IOShutdown)
{
    uint32_t ui32Reg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT((ui32IOShutdown == IOC_NO_WAKE_UP) ||
           (ui32IOShutdown == IOC_WAKE_ON_LOW) ||
           (ui32IOShutdown == IOC_WAKE_ON_HIGH));

    // Get the register address.
    ui32Reg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the IO.
    ui32Config = HWREG(ui32Reg);
    ui32Config &= ~IOC_IOCFG0_WU_CFG_M;
    HWREG(ui32Reg) = ui32Config | ui32IOShutdown;
}


//*****************************************************************************
//
// Set the IO Mode of an IO Port
//
//*****************************************************************************
void
IOCIOModeSet(uint32_t ui32IOId, uint32_t ui32IOMode)
{
    uint32_t ui32Reg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT((ui32IOMode == IOC_IOMODE_NORMAL) ||
           (ui32IOMode == IOC_IOMODE_INV) ||
           (ui32IOMode == IOC_IOMODE_OPEN_DRAIN_NORMAL) ||
           (ui32IOMode == IOC_IOMODE_OPEN_DRAIN_INV) ||
           (ui32IOMode == IOC_IOMODE_OPEN_SRC_NORMAL) ||
           (ui32IOMode == IOC_IOMODE_OPEN_SRC_INV));

    // Get the register address.
    ui32Reg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the IO.
    ui32Config = HWREG(ui32Reg);
    ui32Config &= ~IOC_IOCFG0_IOMODE_M;
    HWREG(ui32Reg) = ui32Config | ui32IOMode;
}

//*****************************************************************************
//
// Setup interrupt detection on an IO Port
//
//*****************************************************************************
void
IOCIOIntSet(uint32_t ui32IOId, uint32_t ui32Int, uint32_t ui32EdgeDet)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT((ui32Int == IOC_INT_ENABLE) ||
           (ui32Int == IOC_INT_DISABLE));
    ASSERT((ui32EdgeDet == IOC_NO_EDGE) ||
           (ui32EdgeDet == IOC_FALLING_EDGE) ||
           (ui32EdgeDet == IOC_RISING_EDGE) ||
           (ui32EdgeDet == IOC_BOTH_EDGES));

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the IO.
    ui32Config = HWREG(ui32IOReg);
    ui32Config &=  ~(IOC_IOCFG0_EDGE_IRQ_EN | IOC_IOCFG0_EDGE_DET_M);
    HWREG(ui32IOReg) = ui32Config | ((ui32Int ? IOC_IOCFG0_EDGE_IRQ_EN : 0) | ui32EdgeDet);
}

//*****************************************************************************
//
// Setup event generation on IO edge detection
//
//*****************************************************************************
void
IOCIOEvtSet(uint32_t ui32IOId, uint32_t ui32Evt)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT( (ui32Evt & ~(IOC_IOCFG0_IOEV_AON_PROG2_EN_M |
                         IOC_IOCFG0_IOEV_AON_PROG1_EN_M |
                         IOC_IOCFG0_IOEV_AON_PROG0_EN_M |
                         IOC_IOCFG0_IOEV_RTC_EN_M |
                         IOC_IOCFG0_IOEV_MCU_WU_EN_M) ) == 0x00000000);

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Read current configuration.
    ui32Config = HWREG(ui32IOReg);

    // Disable generation of all events.
    ui32Config &=  ~(IOC_IOCFG0_IOEV_AON_PROG2_EN_M |
                     IOC_IOCFG0_IOEV_AON_PROG1_EN_M |
                     IOC_IOCFG0_IOEV_AON_PROG0_EN_M |
                     IOC_IOCFG0_IOEV_RTC_EN_M |
                     IOC_IOCFG0_IOEV_MCU_WU_EN_M);

    // Enable the required events.
    HWREG(ui32IOReg) = ui32Config | ui32Evt;
}

//*****************************************************************************
//
// Set the pull on an IO port
//
//*****************************************************************************
void
IOCIOPortPullSet(uint32_t ui32IOId, uint32_t ui32Pull)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the argument.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT((ui32Pull == IOC_NO_IOPULL) ||
           (ui32Pull == IOC_IOPULL_UP) ||
           (ui32Pull == IOC_IOPULL_DOWN));

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the IO.
    ui32Config = HWREG(ui32IOReg);
    ui32Config &= ~IOC_IOCFG0_PULL_CTL_M;
    HWREG(ui32IOReg) = ui32Config | ui32Pull;
}

//*****************************************************************************
//
// Configure hysteresis on and IO port
//
//*****************************************************************************
void
IOCIOHystSet(uint32_t ui32IOId, uint32_t ui32Hysteresis)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT((ui32Hysteresis == IOC_HYST_ENABLE) ||
           (ui32Hysteresis == IOC_HYST_DISABLE));

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the IO.
    ui32Config = HWREG(ui32IOReg);
    ui32Config &= ~IOC_IOCFG0_HYST_EN;
    HWREG(ui32IOReg) = ui32Config | ui32Hysteresis;
}

//*****************************************************************************
//
// Enable/disable IO port as input
//
//*****************************************************************************
void
IOCIOInputSet(uint32_t ui32IOId, uint32_t ui32Input)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT((ui32Input == IOC_INPUT_ENABLE) ||
           (ui32Input == IOC_INPUT_DISABLE));

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the IO.
    ui32Config = HWREG(ui32IOReg);
    ui32Config &= ~IOC_IOCFG0_IE;
    HWREG(ui32IOReg) = ui32Config | ui32Input;
}

//*****************************************************************************
//
// Enable/disable the slew control on an IO port
//
//*****************************************************************************
void
IOCIOSlewCtrlSet(uint32_t ui32IOId, uint32_t ui32SlewEnable)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT((ui32SlewEnable == IOC_SLEW_ENABLE) ||
           (ui32SlewEnable == IOC_SLEW_DISABLE));

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the IO.
    ui32Config = HWREG(ui32IOReg);
    ui32Config &= ~IOC_IOCFG0_SLEW_RED;
    HWREG(ui32IOReg) = ui32Config | ui32SlewEnable;
}

//*****************************************************************************
//
// Configure the drive strength and maximum current of an IO port
//
//*****************************************************************************
void
IOCIODrvStrengthSet(uint32_t ui32IOId, uint32_t ui32IOCurrent,
                    uint32_t ui32DrvStrength)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT((ui32IOCurrent == IOC_CURRENT_2MA) ||
           (ui32IOCurrent == IOC_CURRENT_4MA) ||
           (ui32IOCurrent == IOC_CURRENT_8MA));
    ASSERT((ui32DrvStrength == IOC_STRENGTH_MIN) ||
           (ui32DrvStrength == IOC_STRENGTH_MAX) ||
           (ui32DrvStrength == IOC_STRENGTH_MED) ||
           (ui32DrvStrength == IOC_STRENGTH_AUTO));

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the IO.
    ui32Config = HWREG(ui32IOReg);
    ui32Config &= ~(IOC_IOCFG0_IOCURR_M | IOC_IOCFG0_IOSTR_M);
    HWREG(ui32IOReg) = ui32Config | (ui32IOCurrent | ui32DrvStrength);
}

//*****************************************************************************
//
// Setup the Port ID for this IO
//
//*****************************************************************************
void
IOCIOPortIdSet(uint32_t ui32IOId, uint32_t ui32PortId)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);
    ASSERT(ui32PortId <= IOC_PORT_RFC_GPI1);

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Configure the IO.
    ui32Config = HWREG(ui32IOReg);
    ui32Config &= ~IOC_IOCFG0_PORT_ID_M;
    HWREG(ui32IOReg) = ui32Config | ui32PortId;
}

//*****************************************************************************
//
// Enables individual IO edge detect interrupt
//
//*****************************************************************************
void
IOCIntEnable(uint32_t ui32IOId)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Enable the specified interrupt.
    ui32Config = HWREG(ui32IOReg);
    ui32Config |= IOC_IOCFG0_EDGE_IRQ_EN;
    HWREG(ui32IOReg) = ui32Config;
}

//*****************************************************************************
//
// Disables individual IO edge interrupt sources
//
//*****************************************************************************
void
IOCIntDisable(uint32_t ui32IOId)
{
    uint32_t ui32IOReg;
    uint32_t ui32Config;

    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);

    // Get the register address.
    ui32IOReg = IOC_BASE + ( ui32IOId << 2 );

    // Disable the specified interrupt.
    ui32Config = HWREG(ui32IOReg);
    ui32Config &= ~IOC_IOCFG0_EDGE_IRQ_EN;
    HWREG(ui32IOReg) = ui32Config;
}

//*****************************************************************************
//
// Setup an IO for standard GPIO input
//
//*****************************************************************************
void
IOCPinTypeGpioInput(uint32_t ui32IOId)
{
    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);

    // Setup the IO for standard input.
    IOCPortConfigureSet(ui32IOId, IOC_PORT_GPIO, IOC_STD_INPUT);

    // Enable input mode in the GPIO module.
    GPIO_setOutputEnableDio(ui32IOId, GPIO_OUTPUT_DISABLE);
}

//*****************************************************************************
//
// Setup an IO for standard GPIO output
//
//*****************************************************************************
void
IOCPinTypeGpioOutput(uint32_t ui32IOId)
{
    // Check the arguments.
    ASSERT(ui32IOId <= IOID_31);

    // Setup the IO for standard input.
    IOCPortConfigureSet(ui32IOId, IOC_PORT_GPIO, IOC_STD_OUTPUT);

    // Enable output mode in the GPIO module.
    GPIO_setOutputEnableDio(ui32IOId, GPIO_OUTPUT_ENABLE);
}

//*****************************************************************************
//
// Configure a set of IOs for standard UART peripheral control
//
//*****************************************************************************
void
IOCPinTypeUart(uint32_t ui32Base, uint32_t ui32Rx, uint32_t ui32Tx,
               uint32_t ui32Cts, uint32_t ui32Rts)
{
    // Check the arguments.
    ASSERT(ui32Base == UART0_BASE);
    ASSERT((ui32Rx <= IOID_31) || (ui32Rx == IOID_UNUSED));
    ASSERT((ui32Tx <= IOID_31) || (ui32Tx == IOID_UNUSED));
    ASSERT((ui32Cts <= IOID_31) || (ui32Cts == IOID_UNUSED));
    ASSERT((ui32Rts <= IOID_31) || (ui32Rts == IOID_UNUSED));

    // Setup the IOs in the desired configuration.
    if(ui32Rx != IOID_UNUSED)
    {
        IOCPortConfigureSet(ui32Rx, IOC_PORT_MCU_UART0_RX, IOC_STD_INPUT);
    }
    if(ui32Tx != IOID_UNUSED)
    {
        IOCPortConfigureSet(ui32Tx, IOC_PORT_MCU_UART0_TX, IOC_STD_OUTPUT);
    }
    if(ui32Cts != IOID_UNUSED)
    {
        IOCPortConfigureSet(ui32Cts, IOC_PORT_MCU_UART0_CTS, IOC_STD_INPUT);
    }
    if(ui32Rts != IOID_UNUSED)
    {
        IOCPortConfigureSet(ui32Rts, IOC_PORT_MCU_UART0_RTS, IOC_STD_OUTPUT);
    }
}

//*****************************************************************************
//
// Configure a set of IOs for standard SSI peripheral master control
//
//*****************************************************************************
void
IOCPinTypeSsiMaster(uint32_t ui32Base, uint32_t ui32Rx,
                    uint32_t ui32Tx, uint32_t ui32Fss,
                    uint32_t ui32Clk)
{
    // Check the arguments.
    ASSERT((ui32Base == SSI0_BASE) || (ui32Base == SSI1_BASE));
    ASSERT((ui32Rx <= IOID_31) || (ui32Rx == IOID_UNUSED));
    ASSERT((ui32Tx <= IOID_31) || (ui32Tx == IOID_UNUSED));
    ASSERT((ui32Fss <= IOID_31) || (ui32Fss == IOID_UNUSED));
    ASSERT((ui32Clk <= IOID_31) || (ui32Clk == IOID_UNUSED));

    // Setup the IOs in the desired configuration.
    if(ui32Base == SSI0_BASE)
    {
        if(ui32Rx != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Rx, IOC_PORT_MCU_SSI0_RX, IOC_STD_INPUT);
        }
        if(ui32Tx != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Tx, IOC_PORT_MCU_SSI0_TX, IOC_STD_OUTPUT);
        }
        if(ui32Fss != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Fss, IOC_PORT_MCU_SSI0_FSS, IOC_STD_OUTPUT);
        }
        if(ui32Clk != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Clk, IOC_PORT_MCU_SSI0_CLK, IOC_STD_OUTPUT);
        }
    }
    else
    {
        if(ui32Rx != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Rx, IOC_PORT_MCU_SSI1_RX, IOC_STD_INPUT);
        }
        if(ui32Tx != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Tx, IOC_PORT_MCU_SSI1_TX, IOC_STD_OUTPUT);
        }
        if(ui32Fss != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Fss, IOC_PORT_MCU_SSI1_FSS, IOC_STD_OUTPUT);
        }
        if(ui32Clk != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Clk, IOC_PORT_MCU_SSI1_CLK, IOC_STD_OUTPUT);
        }
    }
}

//*****************************************************************************
//
// Configure a set of IOs for standard SSI peripheral slave control
//
//*****************************************************************************
void
IOCPinTypeSsiSlave(uint32_t ui32Base, uint32_t ui32Rx,
                   uint32_t ui32Tx, uint32_t ui32Fss,
                   uint32_t ui32Clk)
{
    // Check the arguments.
    ASSERT((ui32Base == SSI0_BASE) || (ui32Base == SSI1_BASE));
    ASSERT((ui32Rx <= IOID_31) || (ui32Rx == IOID_UNUSED));
    ASSERT((ui32Tx <= IOID_31) || (ui32Tx == IOID_UNUSED));
    ASSERT((ui32Fss <= IOID_31) || (ui32Fss == IOID_UNUSED));
    ASSERT((ui32Clk <= IOID_31) || (ui32Clk == IOID_UNUSED));

    // Setup the IOs in the desired configuration.
    if(ui32Base == SSI0_BASE)
    {
        if(ui32Rx != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Rx, IOC_PORT_MCU_SSI0_RX,  IOC_STD_INPUT);
        }
        if(ui32Tx != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Tx, IOC_PORT_MCU_SSI0_TX, IOC_STD_OUTPUT);
        }
        if(ui32Fss != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Fss, IOC_PORT_MCU_SSI0_FSS, IOC_STD_INPUT);
        }
        if(ui32Clk != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Clk, IOC_PORT_MCU_SSI0_CLK, IOC_STD_INPUT);
        }
    }
    else
    {
        if(ui32Rx != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Rx, IOC_PORT_MCU_SSI1_RX, IOC_STD_INPUT);
        }
        if(ui32Tx != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Tx, IOC_PORT_MCU_SSI1_TX, IOC_STD_OUTPUT);
        }
        if(ui32Fss != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Fss, IOC_PORT_MCU_SSI1_FSS, IOC_STD_INPUT);
        }
        if(ui32Clk != IOID_UNUSED)
        {
            IOCPortConfigureSet(ui32Clk, IOC_PORT_MCU_SSI1_CLK, IOC_STD_INPUT);
        }
    }
}

//*****************************************************************************
//
// Configure a set of IOs for standard I2C peripheral control
//
//*****************************************************************************
void
IOCPinTypeI2c(uint32_t ui32Base, uint32_t ui32Data, uint32_t ui32Clk)
{
    uint32_t ui32IOConfig;

    // Check the arguments.
    ASSERT((ui32Data <= IOID_31) || (ui32Data == IOID_UNUSED));
    ASSERT((ui32Clk <= IOID_31) || (ui32Clk == IOID_UNUSED));

    // Define the IO configuration parameters.
    ui32IOConfig = IOC_CURRENT_2MA | IOC_STRENGTH_AUTO | IOC_IOPULL_UP |
                   IOC_SLEW_DISABLE | IOC_HYST_DISABLE | IOC_NO_EDGE |
                   IOC_INT_DISABLE | IOC_IOMODE_OPEN_DRAIN_NORMAL |
                   IOC_NO_WAKE_UP | IOC_INPUT_ENABLE;

    // Setup the IOs in the desired configuration.
    IOCPortConfigureSet(ui32Data, IOC_PORT_MCU_I2C_MSSDA, ui32IOConfig);
    IOCPortConfigureSet(ui32Clk, IOC_PORT_MCU_I2C_MSSCL, ui32IOConfig);
}


//*****************************************************************************
//
// Configure an IO for AUX control
//
//*****************************************************************************
void
IOCPinTypeAux(uint32_t ui32IOId)
{
    // Check the arguments.
    ASSERT((ui32IOId <= IOID_31) || (ui32IOId == IOID_UNUSED));

    // Setup the IO.
    IOCPortConfigureSet(ui32IOId, IOC_PORT_AUX_IO, IOC_STD_INPUT);
}
