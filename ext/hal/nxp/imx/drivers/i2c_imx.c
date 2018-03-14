/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
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

#include "i2c_imx.h"

/*******************************************************************************
 * Constant
 ******************************************************************************/
static const uint32_t i2cClkDivTab[][2] =
{
    {22, 0x20},   {24, 0x21},   {26, 0x22},   {28, 0x23},   {30, 0x00},   {32, 0x24},   {36, 0x25},   {40, 0x26},
    {42, 0x03},   {44, 0x27},   {48, 0x28},   {52, 0x05},   {56, 0x29},   {60, 0x06},   {64, 0x2A},   {72, 0x2B},
    {80, 0x2C},   {88, 0x09},   {96, 0x2D},   {104, 0x0A},  {112, 0x2E},  {128, 0x2F},  {144, 0x0C},  {160, 0x30},
    {192, 0x31},  {224, 0x32},  {240, 0x0F},  {256, 0x33},  {288, 0x10},  {320, 0x34},  {384, 0x35},  {448, 0x36},
    {480, 0x13},  {512, 0x37},  {576, 0x14},  {640, 0x38},  {768, 0x39},  {896, 0x3A},  {960, 0x17},  {1024, 0x3B},
    {1152, 0x18}, {1280, 0x3C}, {1536, 0x3D}, {1792, 0x3E}, {1920, 0x1B}, {2048, 0x3F}, {2304, 0x1C}, {2560, 0x1D},
    {3072, 0x1E}, {3840, 0x1F}
};

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * I2C Initialization and Configuration functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : I2C_Init
 * Description   : Initialize I2C module with given initialize structure.
 *
 *END**************************************************************************/
void I2C_Init(I2C_Type* base, const i2c_init_config_t* initConfig)
{
    assert(initConfig);

    /* Disable I2C Module. */
    I2C_I2CR_REG(base) &= ~I2C_I2CR_IEN_MASK;

    /* Reset I2C register to its default value. */
    I2C_Deinit(base);

    /* Set I2C Module own Slave Address. */
    I2C_SetSlaveAddress(base, initConfig->slaveAddress);

    /* Set I2C BaudRate according to i2c initialize struct. */
    I2C_SetBaudRate(base, initConfig->clockRate, initConfig->baudRate);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : I2C_Deinit
 * Description   : This function reset I2C module register content to
 *                 its default value.
 *
 *END**************************************************************************/
void I2C_Deinit(I2C_Type* base)
{
    /* Disable I2C Module */
    I2C_I2CR_REG(base) &= ~I2C_I2CR_IEN_MASK;

    /* Reset I2C Module Register content to default value */
    I2C_IADR_REG(base) = 0x0;
    I2C_IFDR_REG(base) = 0x0;
    I2C_I2CR_REG(base) = 0x0;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : I2C_SetBaudRate
 * Description   : This function is used to set the baud rate of I2C Module.
 *
 *END**************************************************************************/
void I2C_SetBaudRate(I2C_Type* base, uint32_t clockRate, uint32_t baudRate)
{
    uint32_t clockDiv;
    uint8_t clkDivIndex = 0;

    assert(baudRate <= 400000);

    /* Calculate accurate baudRate divider. */
    clockDiv = clockRate / baudRate;

    if (clockDiv < i2cClkDivTab[0][0])
    {
        /* If clock divider is too small, using smallest legal divider */
        clkDivIndex = 0;
    }
    else if (clockDiv > i2cClkDivTab[sizeof(i2cClkDivTab)/sizeof(i2cClkDivTab[0]) - 1][0])
    {
        /* If clock divider is too large, using largest legal divider */
        clkDivIndex = sizeof(i2cClkDivTab)/sizeof(i2cClkDivTab[0]) - 1;
    }
    else
    {
        while (i2cClkDivTab[clkDivIndex][0] < clockDiv)
            clkDivIndex++;
    }

    I2C_IFDR_REG(base) = i2cClkDivTab[clkDivIndex][1];
}

/*******************************************************************************
 * I2C Bus Control functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : I2C_SetAckBit
 * Description   : This function is used to set the Transmit Acknowledge
 *                 action when receive data from other device.
 *
 *END**************************************************************************/
void I2C_SetAckBit(I2C_Type* base, bool ack)
{
    if (ack)
        I2C_I2CR_REG(base) &= ~I2C_I2CR_TXAK_MASK;
    else
        I2C_I2CR_REG(base) |= I2C_I2CR_TXAK_MASK;
}

/*******************************************************************************
 * Interrupts and flags management functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : I2C_SetIntCmd
 * Description   : Enables or disables I2C interrupt requests.
 *
 *END**************************************************************************/
void I2C_SetIntCmd(I2C_Type* base, bool enable)
{
    if (enable)
        I2C_I2CR_REG(base) |= I2C_I2CR_IIEN_MASK;
    else
        I2C_I2CR_REG(base) &= ~I2C_I2CR_IIEN_MASK;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
