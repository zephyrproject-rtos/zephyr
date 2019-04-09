/******************************************************************************
*  Filename:       crypto.c
*  Revised:        2017-12-20 16:40:03 +0100 (Wed, 20 Dec 2017)
*  Revision:       50869
*
*  Description:    Driver for the Crypto module
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

#include "crypto.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  CRYPTOAesLoadKey
    #define CRYPTOAesLoadKey                NOROM_CRYPTOAesLoadKey
    #undef  CRYPTOAesCbc
    #define CRYPTOAesCbc                    NOROM_CRYPTOAesCbc
    #undef  CRYPTOAesCbcStatus
    #define CRYPTOAesCbcStatus              NOROM_CRYPTOAesCbcStatus
    #undef  CRYPTOAesEcb
    #define CRYPTOAesEcb                    NOROM_CRYPTOAesEcb
    #undef  CRYPTOAesEcbStatus
    #define CRYPTOAesEcbStatus              NOROM_CRYPTOAesEcbStatus
    #undef  CRYPTOCcmAuthEncrypt
    #define CRYPTOCcmAuthEncrypt            NOROM_CRYPTOCcmAuthEncrypt
    #undef  CRYPTOCcmAuthEncryptStatus
    #define CRYPTOCcmAuthEncryptStatus      NOROM_CRYPTOCcmAuthEncryptStatus
    #undef  CRYPTOCcmAuthEncryptResultGet
    #define CRYPTOCcmAuthEncryptResultGet   NOROM_CRYPTOCcmAuthEncryptResultGet
    #undef  CRYPTOCcmInvAuthDecrypt
    #define CRYPTOCcmInvAuthDecrypt         NOROM_CRYPTOCcmInvAuthDecrypt
    #undef  CRYPTOCcmInvAuthDecryptStatus
    #define CRYPTOCcmInvAuthDecryptStatus   NOROM_CRYPTOCcmInvAuthDecryptStatus
    #undef  CRYPTOCcmInvAuthDecryptResultGet
    #define CRYPTOCcmInvAuthDecryptResultGet NOROM_CRYPTOCcmInvAuthDecryptResultGet
    #undef  CRYPTODmaEnable
    #define CRYPTODmaEnable                 NOROM_CRYPTODmaEnable
    #undef  CRYPTODmaDisable
    #define CRYPTODmaDisable                NOROM_CRYPTODmaDisable
#endif

//*****************************************************************************
//
// Write the key into the Key Ram.
//
//*****************************************************************************
uint32_t
CRYPTOAesLoadKey(uint32_t *pui32AesKey,
                 uint32_t ui32KeyLocation)
{
    uint32_t returnStatus = AES_KEYSTORE_READ_ERROR;

    // Check the arguments.
    ASSERT((ui32KeyLocation == CRYPTO_KEY_AREA_0) |
           (ui32KeyLocation == CRYPTO_KEY_AREA_1) |
           (ui32KeyLocation == CRYPTO_KEY_AREA_2) |
           (ui32KeyLocation == CRYPTO_KEY_AREA_3) |
           (ui32KeyLocation == CRYPTO_KEY_AREA_4) |
           (ui32KeyLocation == CRYPTO_KEY_AREA_5) |
           (ui32KeyLocation == CRYPTO_KEY_AREA_6) |
           (ui32KeyLocation == CRYPTO_KEY_AREA_7));

    // Disable the external interrupt to stop the interrupt form propagating
    // from the module to the System CPU.
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);

    // Clear any previously written key at the keyLocation
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITTENAREA) = (0x00000001 << ui32KeyLocation);

    // Enable internal interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_DMA_IN_DONE |
                                          CRYPTO_IRQEN_RESULT_AVAIL;

    // Configure master control module.
    HWREGBITW(CRYPTO_BASE + CRYPTO_O_ALGSEL, CRYPTO_ALGSEL_KEY_STORE_BITN) = 1;

    // Clear any outstanding events.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                            CRYPTO_IRQCLR_RESULT_AVAIL);

    // Configure key store module for 128 bit operation.
    // Do not write to the register if the correct key size is already set.
    // Writing to this register causes all current keys to be invalidated.
    if (HWREG(CRYPTO_BASE + CRYPTO_O_KEYSIZE) != KEY_STORE_SIZE_128) {
        HWREG(CRYPTO_BASE + CRYPTO_O_KEYSIZE) = KEY_STORE_SIZE_128;
    }

    // Enable keys to write (e.g. Key 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITEAREA) = (0x00000001 << ui32KeyLocation);

    // Enable Crypto DMA channel 0.
    HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

    // Base address of the key in ext. memory.
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uint32_t)pui32AesKey;

    // Total key length in bytes (e.g. 16 for 1 x 128-bit key).
    // Writing the length of the key enables the DMA operation.
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = KEY_BLENGTH;

    // Wait for the DMA operation to complete.
    do
    {
        CPUdelay(1);
    }
    while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) &
            (CRYPTO_IRQSTAT_DMA_BUS_ERR_M |
             CRYPTO_IRQSTAT_KEY_ST_WR_ERR_M |
             CRYPTO_IRQSTAT_DMA_IN_DONE |
             CRYPTO_IRQSTAT_RESULT_AVAIL_M)));

    // Check for errors in DMA and key store.
    if((HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) &
            (CRYPTO_IRQSTAT_DMA_BUS_ERR |
             CRYPTO_IRQSTAT_KEY_ST_WR_ERR)) == 0)
    {
        // Acknowledge/clear the interrupt and disable the master control.
        HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                                CRYPTO_IRQCLR_RESULT_AVAIL);
        HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = 0x00000000;

        // Check key status, return success if key valid.
        if(HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITTENAREA) & (0x00000001 << ui32KeyLocation))
        {
            returnStatus =  AES_SUCCESS;
        }
    }

    // Return status.
    return returnStatus;
}

//*****************************************************************************
//
// Start an AES-CBC operation (encryption or decryption).
//
//*****************************************************************************
uint32_t
CRYPTOAesCbc(uint32_t *pui32MsgIn, uint32_t *pui32MsgOut, uint32_t ui32MsgLength,
             uint32_t *pui32Nonce, uint32_t ui32KeyLocation,
             bool bEncrypt, bool bIntEnable)
{
    uint32_t ui32CtrlVal;

    // Enable internal interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_RESULT_AVAIL;

    // Clear any outstanding interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                            CRYPTO_IRQCLR_RESULT_AVAIL);

    // Wait for interrupt lines from module to be cleared
    while(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & (CRYPTO_IRQSTAT_DMA_IN_DONE | CRYPTO_IRQSTAT_RESULT_AVAIL));

    // If using interrupts clear any pending interrupts and enable interrupts
    // for the Crypto module.
    if(bIntEnable)
    {
        IntPendClear(INT_CRYPTO_RESULT_AVAIL_IRQ);
        IntEnable(INT_CRYPTO_RESULT_AVAIL_IRQ);
    }

    // Configure Master Control module.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = CRYPTO_ALGSEL_AES;

    // Enable keys to read (e.g. Key 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) = ui32KeyLocation;

    //Wait until key is loaded to the AES module.
    do
    {
        CPUdelay(1);
    }
    while((HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) & CRYPTO_KEYREADAREA_BUSY));

    // Check for Key store Read error.
    if((HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT)& CRYPTO_KEY_ST_RD_ERR))
    {
        return (AES_KEYSTORE_READ_ERROR);
    }

    // Write initialization vector.
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV0) = pui32Nonce[0];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV1) = pui32Nonce[1];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV2) = pui32Nonce[2];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV3) = pui32Nonce[3];

    // Configure AES engine for AES-CBC with 128-bit key size.
    ui32CtrlVal  = (CRYPTO_AESCTL_SAVE_CONTEXT | CRYPTO_AESCTL_CBC);
    if(bEncrypt)
    {
        ui32CtrlVal |= CRYPTO_AES128_ENCRYPT;
    }
    else
    {
        ui32CtrlVal |= CRYPTO_AES128_DECRYPT;
    }
    HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) = ui32CtrlVal;

    // Write the length of the crypto block (plain text).
    // Low and high part (high part is assumed to be always 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN0) = ui32MsgLength;
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN1) = 0;
    HWREG(CRYPTO_BASE + CRYPTO_O_AESAUTHLEN)  = 0;

    // Enable Crypto DMA channel 0.
    HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

    // Base address of the input data in ext. memory.
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uint32_t)pui32MsgIn;

    // Input data length in bytes, equal to the message.
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = ui32MsgLength;

    // Enable Crypto DMA channel 1.
    HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH1CTL, CRYPTO_DMACH1CTL_EN_BITN) = 1;

    // Set up the address and length of the output data.
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1EXTADDR) = (uint32_t)pui32MsgOut;
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1LEN) = ui32MsgLength;

    // Return success
    return AES_SUCCESS;
}

//*****************************************************************************
//
// Check the result of an AES CBC operation
//
//*****************************************************************************
uint32_t
CRYPTOAesCbcStatus(void)
{
    return(CRYPTOAesEcbStatus());
}

//*****************************************************************************
//
// Start an AES-ECB operation (encryption or decryption).
//
//*****************************************************************************
uint32_t
CRYPTOAesEcb(uint32_t *pui32MsgIn, uint32_t *pui32MsgOut,
             uint32_t ui32KeyLocation, bool bEncrypt,
             bool bIntEnable)
{
    // Enable internal interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_RESULT_AVAIL;

    // Clear any outstanding interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                            CRYPTO_IRQCLR_RESULT_AVAIL);

    // Wait for interrupt lines from module to be cleared
    while(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & (CRYPTO_IRQSTAT_DMA_IN_DONE | CRYPTO_IRQSTAT_RESULT_AVAIL));

    // If using interrupts clear any pending interrupts and enable interrupts
    // for the Crypto module.
    if(bIntEnable)
    {
        IntPendClear(INT_CRYPTO_RESULT_AVAIL_IRQ);
        IntEnable(INT_CRYPTO_RESULT_AVAIL_IRQ);
    }

    // Configure Master Control module.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = CRYPTO_ALGSEL_AES;

    // Enable keys to read (e.g. Key 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) = ui32KeyLocation;

    //Wait until key is loaded to the AES module.
    do
    {
        CPUdelay(1);
    }
    while((HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) & CRYPTO_KEYREADAREA_BUSY));

    // Check for Key store Read error.
    if((HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT)& CRYPTO_KEY_ST_RD_ERR))
    {
        return (AES_KEYSTORE_READ_ERROR);
    }

    // Configure AES engine (program AES-ECB-128 encryption and no
    // initialization vector - IV).
    if(bEncrypt)
    {
        HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) = CRYPTO_AES128_ENCRYPT;
    }
    else
    {
        HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) = CRYPTO_AES128_DECRYPT;
    }

    // Write the length of the data.
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN0) = AES_ECB_LENGTH;
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN1) = 0;

    // Enable Crypto DMA channel 0.
    HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

    // Base address of the input data in ext. memory.
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uint32_t)pui32MsgIn;

    // Input data length in bytes, equal to the message.
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = AES_ECB_LENGTH;

    // Enable Crypto DMA channel 1.
    HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH1CTL, CRYPTO_DMACH1CTL_EN_BITN) = 1;

    // Set up the address and length of the output data.
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1EXTADDR) = (uint32_t)pui32MsgOut;
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1LEN) = AES_ECB_LENGTH;

    // Return success
    return AES_SUCCESS;
}

//*****************************************************************************
//
// Check the result of an AES ECB operation
//
//*****************************************************************************
uint32_t
CRYPTOAesEcbStatus(void)
{
    uint32_t ui32Status;

    // Get the current DMA status.
    ui32Status = HWREG(CRYPTO_BASE + CRYPTO_O_DMASTAT);

    // Check if DMA is still busy.
    if(ui32Status & CRYPTO_DMA_BSY)
    {
        return (AES_DMA_BSY);
    }

    // Check the status of the DMA operation - return error if not success.
    if(ui32Status & CRYPTO_DMA_BUS_ERROR)
    {
        return (AES_DMA_BUS_ERROR);
    }

    // Operation successful - disable interrupt and return success.
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);
    return (AES_SUCCESS);
}

//*****************************************************************************
//
// Start CCM operation
//
//*****************************************************************************
uint32_t
CRYPTOCcmAuthEncrypt(bool bEncrypt, uint32_t ui32AuthLength ,
                     uint32_t *pui32Nonce, uint32_t *pui32PlainText,
                     uint32_t ui32PlainTextLength, uint32_t *pui32Header,
                     uint32_t ui32HeaderLength, uint32_t ui32KeyLocation,
                     uint32_t ui32FieldLength, bool bIntEnable)
{
    uint32_t ui32CtrlVal;
    uint32_t i;
    uint32_t *pui32CipherText;
    union {
        uint32_t w[4];
        uint8_t  b[16];
    } ui8InitVec;

    // Input address for the encryption engine is the same as the output.
    pui32CipherText = pui32PlainText;

    // Disable global interrupt, enable local interrupt and clear any pending
    // interrupts.
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                            CRYPTO_IRQCLR_RESULT_AVAIL);

    // Enable internal interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_DMA_IN_DONE |
                                          CRYPTO_IRQEN_RESULT_AVAIL;

    // Configure master control module for AES operation.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = CRYPTO_ALGSEL_AES;

    // Enable keys to read (e.g. Key 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) = ui32KeyLocation;

    // Wait until key is loaded to the AES module.
    do
    {
        CPUdelay(1);
    }
    while((HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) & CRYPTO_KEYREADAREA_BUSY));

    // Check for Key store Read error.
    if((HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT)& CRYPTO_KEY_ST_RD_ERR))
    {
        return (AES_KEYSTORE_READ_ERROR);
    }

    // Prepare the initialization vector (IV),
    // Length of Nonce l(n) = 15 - ui32FieldLength.
    ui8InitVec.b[0] = ui32FieldLength - 1;
    for(i = 0; i < 12; i++)
    {
        ui8InitVec.b[i + 1] = ((uint8_t*)pui32Nonce)[i];
    }
    if(ui32FieldLength == 2)
    {
        ui8InitVec.b[13] = ((uint8_t*)pui32Nonce)[12];
    }
    else
    {
        ui8InitVec.b[13] = 0;
    }
    ui8InitVec.b[14] = 0;
    ui8InitVec.b[15] = 0;

    // Write initialization vector.
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV0) = ui8InitVec.w[0];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV1) = ui8InitVec.w[1];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV2) = ui8InitVec.w[2];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV3) = ui8InitVec.w[3];

    // Configure AES engine.
    ui32CtrlVal = ((ui32FieldLength - 1) << CRYPTO_AESCTL_CCM_L_S);
    if ( ui32AuthLength >= 2 ) {
        ui32CtrlVal |= ((( ui32AuthLength - 2 ) >> 1 ) << CRYPTO_AESCTL_CCM_M_S );
    }
    ui32CtrlVal |= CRYPTO_AESCTL_CCM;
    ui32CtrlVal |= CRYPTO_AESCTL_CTR;
    ui32CtrlVal |= CRYPTO_AESCTL_SAVE_CONTEXT;
    ui32CtrlVal |= (KEY_STORE_SIZE_128 << CRYPTO_AESCTL_KEY_SIZE_S);
    ui32CtrlVal |= (1 << CRYPTO_AESCTL_DIR_S);
    ui32CtrlVal |= (CRYPTO_AES_CTR_128 << CRYPTO_AESCTL_CTR_WIDTH_S);

    // Write the configuration for 128 bit AES-CCM.
    HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) = ui32CtrlVal;

    // Write the length of the crypto block (plain text).
    // Low and high part (high part is assumed to be always 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN0) = ui32PlainTextLength;
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN1)  =  0;

    // Write the length of the header field.
    // Also called AAD - Additional Authentication Data.
    HWREG(CRYPTO_BASE + CRYPTO_O_AESAUTHLEN) = ui32HeaderLength;

    // Check if any header information (AAD).
    // If so configure the DMA controller to fetch the header.
    if(ui32HeaderLength != 0)
    {
        // Enable DMA channel 0.
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

        // Register the base address of the header (AAD).
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uint32_t)pui32Header;

        // Header length in bytes (may be non-block size aligned).
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = ui32HeaderLength;

        // Wait for completion of the header data transfer, DMA_IN_DONE.
        do
        {
            CPUdelay(1);
        }
        while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQSTAT_DMA_IN_DONE));

        // Check for DMA errors.
        if(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_DMA_BUS_ERR)
        {
            return AES_DMA_BUS_ERROR;
        }
    }

    // Clear interrupt status.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                            CRYPTO_IRQCLR_RESULT_AVAIL);

    // Wait for interrupt lines from module to be cleared
    while(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & (CRYPTO_IRQSTAT_DMA_IN_DONE | CRYPTO_IRQSTAT_RESULT_AVAIL));

    // Disable CRYPTO_IRQEN_DMA_IN_DONE interrupt as we only
    // want interrupt to trigger once RESULT_AVAIL occurs.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) &= ~CRYPTO_IRQEN_DMA_IN_DONE;


    // Is using interrupts enable globally.
    if(bIntEnable)
    {
        IntPendClear(INT_CRYPTO_RESULT_AVAIL_IRQ);
        IntEnable(INT_CRYPTO_RESULT_AVAIL_IRQ);
    }

    // Enable interrupts locally.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_RESULT_AVAIL;

    // Perform encryption if requested.
    if(bEncrypt)
    {
        // Enable DMA channel 0
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

        // base address of the payload data in ext. memory.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) =
            (uint32_t)pui32PlainText;

        // Enable DMA channel 1
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH1CTL, CRYPTO_DMACH1CTL_EN_BITN) = 1;

        // Base address of the output data buffer.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1EXTADDR) =
            (uint32_t)pui32CipherText;

        // Payload data length in bytes, equal to the plaintext length.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = ui32PlainTextLength;
        // Output data length in bytes, equal to the plaintext length.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1LEN) = ui32PlainTextLength;
    }

    return AES_SUCCESS;
}

//*****************************************************************************
//
// Check the result of an AES CCM operation.
//
//*****************************************************************************
uint32_t
CRYPTOCcmAuthEncryptStatus(void)
{
    uint32_t ui32Status;

    // Get the current DMA status.
    ui32Status = HWREG(CRYPTO_BASE + CRYPTO_O_DMASTAT);

    // Check if DMA is still busy.
    if(ui32Status & CRYPTO_DMA_BSY)
    {
        return (AES_DMA_BSY);
    }

    // Check the status of the DMA operation - return error if not success.
    if(ui32Status & CRYPTO_DMA_BUS_ERROR)
    {
        return (AES_DMA_BUS_ERROR);
    }

    // Operation successful - disable interrupt and return success.
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);
    return (AES_SUCCESS);
}

//*****************************************************************************
//
// Get the result of an AES-CCM operation
//
//*****************************************************************************
uint32_t
CRYPTOCcmAuthEncryptResultGet(uint32_t ui32TagLength, uint32_t *pui32CcmTag)
{
    uint32_t volatile ui32Tag[4];
    uint32_t ui32Idx;

    // Result has already been copied to the output buffer by DMA
    // Disable master control.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = 0x00000000;

    // Read tag - wait for the context ready bit.
    do
    {
        CPUdelay(1);
    }
    while(!(HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) &
            CRYPTO_AESCTL_SAVED_CONTEXT_RDY));

    // Read the Tag registers.
    ui32Tag[0] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT0);
    ui32Tag[1] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT1);
    ui32Tag[2] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT2);
    ui32Tag[3] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT3);

    for(ui32Idx = 0; ui32Idx < ui32TagLength ; ui32Idx++)
    {
        *((uint8_t*)pui32CcmTag + ui32Idx) = *((uint8_t*)ui32Tag + ui32Idx);
    }

    // Operation successful -  clear interrupt status.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                            CRYPTO_IRQCLR_RESULT_AVAIL);
    return AES_SUCCESS;
}

//*****************************************************************************
//
// Start a CCM Decryption and Inverse Authentication operation.
//
//*****************************************************************************
uint32_t
CRYPTOCcmInvAuthDecrypt(bool bDecrypt, uint32_t ui32AuthLength,
                        uint32_t *pui32Nonce, uint32_t *pui32CipherText,
                        uint32_t ui32CipherTextLength,
                        uint32_t *pui32Header, uint32_t ui32HeaderLength,
                        uint32_t ui32KeyLocation,
                        uint32_t ui32FieldLength, bool bIntEnable)
{
    uint32_t ui32CtrlVal;
    uint32_t i;
    uint32_t *pui32PlainText;
    uint32_t ui32CryptoBlockLength;
    union {
        uint32_t w[4];
        uint8_t  b[16];
    } ui8InitVec;

    // Input address for the encryption engine is the same as the output.
    pui32PlainText = pui32CipherText;

    // Disable global interrupt, enable local interrupt and clear any pending.
    // interrupts.
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                            CRYPTO_IRQCLR_RESULT_AVAIL);
    // Enable internal interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_DMA_IN_DONE |
                                          CRYPTO_IRQEN_RESULT_AVAIL;

    // Configure master control module for AES operation.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = CRYPTO_ALGSEL_AES;

    // Enable keys to read (e.g. Key 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) = ui32KeyLocation;

    // Wait until key is loaded to the AES module.
    do
    {
        CPUdelay(1);
    }
    while((HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) & CRYPTO_KEYREADAREA_BUSY));

    // Check for Key store Read error.
    if((HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT)& CRYPTO_KEY_ST_RD_ERR))
    {
        return (AES_KEYSTORE_READ_ERROR);
    }

    // Prepare the initialization vector (IV),
    // Length of Nonce l(n) = 15 - ui32FieldLength.
    ui8InitVec.b[0] = ui32FieldLength - 1;
    for(i = 0; i < 12; i++)
    {
        ui8InitVec.b[i + 1] = ((uint8_t*)pui32Nonce)[i];
    }
    if(ui32FieldLength == 2)
    {
        ui8InitVec.b[13] = ((uint8_t*)pui32Nonce)[12];
    }
    else
    {
        ui8InitVec.b[13] = 0;
    }
    ui8InitVec.b[14] = 0;
    ui8InitVec.b[15] = 0;

    // Write initialization vector.
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV0) = ui8InitVec.w[0];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV1) = ui8InitVec.w[1];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV2) = ui8InitVec.w[2];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV3) = ui8InitVec.w[3];

    // Configure AES engine
    ui32CryptoBlockLength = ui32CipherTextLength - ui32AuthLength;
    ui32CtrlVal = ((ui32FieldLength - 1) << CRYPTO_AESCTL_CCM_L_S);
    if ( ui32AuthLength >= 2 ) {
        ui32CtrlVal |= ((( ui32AuthLength - 2 ) >> 1 ) << CRYPTO_AESCTL_CCM_M_S );
    }
    ui32CtrlVal |= CRYPTO_AESCTL_CCM;
    ui32CtrlVal |= CRYPTO_AESCTL_CTR;
    ui32CtrlVal |= CRYPTO_AESCTL_SAVE_CONTEXT;
    ui32CtrlVal |= (KEY_STORE_SIZE_128 << CRYPTO_AESCTL_KEY_SIZE_S);
    ui32CtrlVal |= (0 << CRYPTO_AESCTL_DIR_S);
    ui32CtrlVal |= (CRYPTO_AES_CTR_128 << CRYPTO_AESCTL_CTR_WIDTH_S);

    // Write the configuration for 128 bit AES-CCM.
    HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) = ui32CtrlVal;

    // Write the length of the crypto block (plain text).
    // Low and high part (high part is assumed to be always 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN0) = ui32CryptoBlockLength;
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN1)  =  0;

    // Write the length of the header field.
    // Also called AAD - Additional Authentication Data.
    HWREG(CRYPTO_BASE + CRYPTO_O_AESAUTHLEN) = ui32HeaderLength;

    // Check if any header information (AAD).
    // If so configure the DMA controller to fetch the header.
    if(ui32HeaderLength != 0)
    {
        // Enable DMA channel 0.
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

        // Register the base address of the header (AAD).
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uint32_t)pui32Header;

        // Header length in bytes (may be non-block size aligned).
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = ui32HeaderLength;

        // Wait for completion of the header data transfer, DMA_IN_DONE.
        do
        {
            CPUdelay(1);
        }
        while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQSTAT_DMA_IN_DONE));

        // Check for DMA errors.
        if(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_DMA_BUS_ERR)
        {
            return AES_DMA_BUS_ERROR;
        }
    }

    // Clear interrupt status.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                            CRYPTO_IRQCLR_RESULT_AVAIL);

    // Wait for interrupt lines from module to be cleared
    while(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & (CRYPTO_IRQSTAT_DMA_IN_DONE | CRYPTO_IRQSTAT_RESULT_AVAIL));

    // Disable CRYPTO_IRQEN_DMA_IN_DONE interrupt as we only
    // want interrupt to trigger once RESULT_AVAIL occurs.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) &= ~CRYPTO_IRQEN_DMA_IN_DONE;

    // Is using interrupts - clear and enable globally.
    if(bIntEnable)
    {
        IntPendClear(INT_CRYPTO_RESULT_AVAIL_IRQ);
        IntEnable(INT_CRYPTO_RESULT_AVAIL_IRQ);
    }

    // Enable internal interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_RESULT_AVAIL;

    // Perform decryption if requested.
    if(bDecrypt)
    {
        // Configure the DMA controller - enable both DMA channels.
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

        // Base address of the payload data in ext. memory.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) =
            (uint32_t)pui32CipherText;

        // Payload data length in bytes, equal to the cipher text length.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = ui32CryptoBlockLength;

        // Enable DMA channel 1.
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH1CTL, CRYPTO_DMACH1CTL_EN_BITN) = 1;

        // Base address of the output data buffer.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1EXTADDR) =
            (uint32_t)pui32PlainText;

        // Output data length in bytes, equal to the cipher text length.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1LEN) = ui32CryptoBlockLength;
    }

    return AES_SUCCESS;
}

//*****************************************************************************
//
// Checks CCM decrypt and Inverse Authentication result.
//
//*****************************************************************************
uint32_t
CRYPTOCcmInvAuthDecryptStatus(void)
{
    uint32_t ui32Status;

    // Get the current DMA status.
    ui32Status = HWREG(CRYPTO_BASE + CRYPTO_O_DMASTAT);

    // Check if DMA is still busy.
    if(ui32Status & CRYPTO_DMA_BSY)
    {
        return (AES_DMA_BSY);
    }

    // Check the status of the DMA operation - return error if not success.
    if(ui32Status & CRYPTO_DMA_BUS_ERROR)
    {
        return (AES_DMA_BUS_ERROR);
    }

    // Operation successful - disable interrupt and return success
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);
    return (AES_SUCCESS);
}

//*****************************************************************************
//
// Get the result of the CCM operation.
//
//*****************************************************************************
uint32_t
CRYPTOCcmInvAuthDecryptResultGet(uint32_t ui32AuthLength,
                                 uint32_t *pui32CipherText,
                                 uint32_t ui32CipherTextLength,
                                 uint32_t *pui32CcmTag)
{
    uint32_t volatile ui32Tag[4];
    uint32_t ui32TagIndex;
    uint32_t i;
    uint32_t ui32Idx;

    ui32TagIndex = ui32CipherTextLength - ui32AuthLength;

    // Result has already been copied to the output buffer by DMA
    // Disable master control.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = 0x00000000;

    // Read tag - wait for the context ready bit.
    do
    {
        CPUdelay(1);
    }
    while(!(HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) &
            CRYPTO_AESCTL_SAVED_CONTEXT_RDY));

    // Read the Tag registers.
    ui32Tag[0] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT0);
    ui32Tag[1] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT1);
    ui32Tag[2] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT2);
    ui32Tag[3] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT3);

    for(ui32Idx = 0; ui32Idx < ui32AuthLength ; ui32Idx++)
    {
        *((uint8_t*)pui32CcmTag + ui32Idx) = *((uint8_t*)ui32Tag + ui32Idx);
    }

    // Operation successful -  clear interrupt status.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE |
                                            CRYPTO_IRQCLR_RESULT_AVAIL);

    // Verify the Tag.
    for(i = 0; i < ui32AuthLength; i++)
    {
        if(*((uint8_t *)pui32CcmTag + i) !=
            (*((uint8_t *)pui32CipherText + ui32TagIndex + i)))
        {
            return CCM_AUTHENTICATION_FAILED;
        }
    }

    return AES_SUCCESS;
}

//*****************************************************************************
//
// Enable Crypto DMA operation
//
//*****************************************************************************
void
CRYPTODmaEnable(uint32_t ui32Channels)
{
    // Check the arguments.
    ASSERT((ui32Channels & CRYPTO_DMA_CHAN0) |
           (ui32Channels & CRYPTO_DMA_CHAN1));

    // Enable the selected channels,
    if(ui32Channels & CRYPTO_DMA_CHAN0)
    {
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;
    }
    if(ui32Channels & CRYPTO_DMA_CHAN1)
    {
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH1CTL, CRYPTO_DMACH1CTL_EN_BITN) = 1;
    }
}

//*****************************************************************************
//
// Disable Crypto DMA operation
//
//*****************************************************************************
void
CRYPTODmaDisable(uint32_t ui32Channels)
{
    // Check the arguments.
    ASSERT((ui32Channels & CRYPTO_DMA_CHAN0) |
           (ui32Channels & CRYPTO_DMA_CHAN1));

    // Enable the selected channels.
    if(ui32Channels & CRYPTO_DMA_CHAN0)
    {
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 0;
    }
    if(ui32Channels & CRYPTO_DMA_CHAN1)
    {
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH1CTL, CRYPTO_DMACH1CTL_EN_BITN) = 0;
    }
}
