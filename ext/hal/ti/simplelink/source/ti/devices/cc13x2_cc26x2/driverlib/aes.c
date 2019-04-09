
/******************************************************************************
*  Filename:       crypto.c
*  Revised:        2019-01-25 13:11:50 +0100 (Fri, 25 Jan 2019)
*  Revision:       54285
*
*  Description:    Driver for the aes functions of the crypto module
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

#include "aes.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  AESStartDMAOperation
    #define AESStartDMAOperation            NOROM_AESStartDMAOperation
    #undef  AESSetInitializationVector
    #define AESSetInitializationVector      NOROM_AESSetInitializationVector
    #undef  AESWriteCCMInitializationVector
    #define AESWriteCCMInitializationVector NOROM_AESWriteCCMInitializationVector
    #undef  AESReadTag
    #define AESReadTag                      NOROM_AESReadTag
    #undef  AESVerifyTag
    #define AESVerifyTag                    NOROM_AESVerifyTag
    #undef  AESWriteToKeyStore
    #define AESWriteToKeyStore              NOROM_AESWriteToKeyStore
    #undef  AESReadFromKeyStore
    #define AESReadFromKeyStore             NOROM_AESReadFromKeyStore
    #undef  AESWaitForIRQFlags
    #define AESWaitForIRQFlags              NOROM_AESWaitForIRQFlags
    #undef  AESConfigureCCMCtrl
    #define AESConfigureCCMCtrl             NOROM_AESConfigureCCMCtrl
#endif



//*****************************************************************************
//
// Load the initialization vector.
//
//*****************************************************************************
void AESSetInitializationVector(const uint32_t *initializationVector)
{
    // Write initialization vector to the aes registers
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV0) = initializationVector[0];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV1) = initializationVector[1];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV2) = initializationVector[2];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV3) = initializationVector[3];
}

//*****************************************************************************
//
// Start a crypto DMA operation.
//
//*****************************************************************************
void AESStartDMAOperation(const uint8_t *channel0Addr, uint32_t channel0Length,  uint8_t *channel1Addr, uint32_t channel1Length)
{
    if (channel0Length && channel0Addr) {
        // We actually want to perform an operation. Clear any outstanding events.
        HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = CRYPTO_IRQCLR_RESULT_AVAIL_M | CRYPTO_IRQEN_DMA_IN_DONE_M; // This might need AES_IRQEN_DMA_IN_DONE as well

        while(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & (CRYPTO_IRQSTAT_DMA_IN_DONE_M | CRYPTO_IRQSTAT_RESULT_AVAIL_M));

        // Configure the DMA controller - enable both DMA channels.
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

        // Base address of the payload data in ext. memory.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uint32_t)channel0Addr;

        // Payload data length in bytes, equal to the cipher text length.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = channel0Length;
    }

    if (channel1Length && channel1Addr) {
        // Enable DMA channel 1.
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH1CTL, CRYPTO_DMACH1CTL_EN_BITN) = 1;

        // Base address of the output data buffer.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1EXTADDR) = (uint32_t)channel1Addr;

        // Output data length in bytes, equal to the cipher text length.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1LEN) = channel1Length;
    }
}

//*****************************************************************************
//
// Poll the IRQ status register and return.
//
//*****************************************************************************
uint32_t AESWaitForIRQFlags(uint32_t irqFlags)
{
    uint32_t irqTrigger = 0;
    // Wait for the DMA operation to complete. Add a delay to make sure we are
    // not flooding the bus with requests too much.
    do {
        CPUdelay(1);
    }
    while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & irqFlags & (CRYPTO_IRQSTAT_DMA_IN_DONE_M |
                                                                CRYPTO_IRQSTAT_RESULT_AVAIL_M |
                                                                CRYPTO_IRQSTAT_DMA_BUS_ERR_M |
                                                                CRYPTO_IRQSTAT_KEY_ST_WR_ERR_M)));

    // Save the IRQ trigger source
    irqTrigger = HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & irqFlags;

    // Clear IRQ flags
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = irqTrigger;

    return irqTrigger;
}

//*****************************************************************************
//
// Transfer a key from CM3 memory to a key store location.
//
//*****************************************************************************
uint32_t AESWriteToKeyStore(const uint8_t *aesKey, uint32_t aesKeyLength, uint32_t keyStoreArea)
{
    // Check the arguments.
    ASSERT((keyStoreArea == AES_KEY_AREA_0) ||
           (keyStoreArea == AES_KEY_AREA_1) ||
           (keyStoreArea == AES_KEY_AREA_2) ||
           (keyStoreArea == AES_KEY_AREA_3) ||
           (keyStoreArea == AES_KEY_AREA_4) ||
           (keyStoreArea == AES_KEY_AREA_5) ||
           (keyStoreArea == AES_KEY_AREA_6) ||
           (keyStoreArea == AES_KEY_AREA_7));

    ASSERT((aesKeyLength == AES_128_KEY_LENGTH_BYTES) ||
           (aesKeyLength == AES_192_KEY_LENGTH_BYTES) ||
           (aesKeyLength == AES_256_KEY_LENGTH_BYTES));

    uint32_t keySize = 0;

    switch (aesKeyLength) {
        case AES_128_KEY_LENGTH_BYTES:
            keySize = CRYPTO_KEYSIZE_SIZE_128_BIT;
            break;
        case AES_192_KEY_LENGTH_BYTES:
            keySize = CRYPTO_KEYSIZE_SIZE_192_BIT;
            break;
        case AES_256_KEY_LENGTH_BYTES:
            keySize = CRYPTO_KEYSIZE_SIZE_256_BIT;
            break;
    }

    // Clear any previously written key at the keyLocation
    AESInvalidateKey(keyStoreArea);

    // Disable the external interrupt to stop the interrupt form propagating
    // from the module to the System CPU.
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);

    // Enable internal interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL_M;
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_DMA_IN_DONE_M | CRYPTO_IRQEN_RESULT_AVAIL_M;

    // Configure master control module.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = CRYPTO_ALGSEL_KEY_STORE;

    // Clear any outstanding events.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = (CRYPTO_IRQCLR_DMA_IN_DONE | CRYPTO_IRQCLR_RESULT_AVAIL);

    // Configure the size of keys contained within the key store
    // Do not write to the register if the correct key size is already set.
    // Writing to this register causes all current keys to be invalidated.
    uint32_t keyStoreKeySize = HWREG(CRYPTO_BASE + CRYPTO_O_KEYSIZE);
    if (keySize != keyStoreKeySize) {
        HWREG(CRYPTO_BASE + CRYPTO_O_KEYSIZE) = keySize;
    }

    // Enable key to write (e.g. Key 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITEAREA) = 1 << keyStoreArea;

    // Total key length in bytes (16 for 1 x 128-bit key and 32 for 1 x 256-bit key).
    AESStartDMAOperation(aesKey, aesKeyLength, 0, 0);

    // Wait for the DMA operation to complete.
    uint32_t irqTrigger = AESWaitForIRQFlags(CRYPTO_IRQCLR_RESULT_AVAIL | CRYPTO_IRQCLR_DMA_IN_DONE | CRYPTO_IRQSTAT_DMA_BUS_ERR | CRYPTO_IRQSTAT_KEY_ST_WR_ERR);

    // Re-enable interrupts globally.
    IntPendClear(INT_CRYPTO_RESULT_AVAIL_IRQ);
    IntEnable(INT_CRYPTO_RESULT_AVAIL_IRQ);

    // If we had a bus error or the key is not in the CRYPTO_O_KEYWRITTENAREA, return an error.
    if ((irqTrigger & (CRYPTO_IRQSTAT_DMA_BUS_ERR_M | CRYPTO_IRQSTAT_KEY_ST_WR_ERR_M)) || !(HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITTENAREA) & (1 << keyStoreArea))) {
        // There was an error in writing to the keyStore.
        return AES_KEYSTORE_ERROR;
    }
    else {
        return AES_SUCCESS;
    }
}

//*****************************************************************************
//
// Transfer a key from the keyStoreArea to the internal buffer of the module.
//
//*****************************************************************************
uint32_t AESReadFromKeyStore(uint32_t keyStoreArea)
{
    // Check the arguments.
    ASSERT((keyStoreArea == AES_KEY_AREA_0) ||
           (keyStoreArea == AES_KEY_AREA_1) ||
           (keyStoreArea == AES_KEY_AREA_2) ||
           (keyStoreArea == AES_KEY_AREA_3) ||
           (keyStoreArea == AES_KEY_AREA_4) ||
           (keyStoreArea == AES_KEY_AREA_5) ||
           (keyStoreArea == AES_KEY_AREA_6) ||
           (keyStoreArea == AES_KEY_AREA_7));

    // Check if there is a valid key in the specified keyStoreArea
    if (!(HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITTENAREA) & (1 << keyStoreArea))) {
        return AES_KEYSTORE_AREA_INVALID;
    }

    // Enable keys to read (e.g. Key 0).
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) = keyStoreArea;

    // Wait until key is loaded to the AES module.
    // We cannot simply poll the IRQ status as only an error is communicated through
    // the IRQ status and not the completion of the transfer.
    do {
        CPUdelay(1);
    }
    while((HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) & CRYPTO_KEYREADAREA_BUSY_M));

    // Check for keyStore read error.
    if((HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQSTAT_KEY_ST_RD_ERR_M)) {
        return AES_KEYSTORE_ERROR;
    }
    else {
        return AES_SUCCESS;
    }
}

//*****************************************************************************
//
// Read the tag after a completed CCM, GCM, or CBC-MAC operation.
//
//*****************************************************************************
uint32_t AESReadTag(uint8_t *tag, uint32_t tagLength)
{
    // The intermediate array is used instead of a caller-provided one
    // to enable a simple API with no unintuitive alignment or size requirements.
    // This is a trade-off of stack-depth vs ease-of-use that came out on the
    // ease-of-use side.
    uint32_t computedTag[AES_BLOCK_SIZE / sizeof(uint32_t)];

    // Wait until the computed tag is ready.
    while (!(HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) & CRYPTO_AESCTL_SAVED_CONTEXT_RDY_M));

    // Read computed tag out from the HW registers
    // Need to read out all 128 bits in four reads to correctly clear CRYPTO_AESCTL_SAVED_CONTEXT_RDY flag
    computedTag[0] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT0);
    computedTag[1] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT1);
    computedTag[2] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT2);
    computedTag[3] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT3);

    memcpy(tag, computedTag, tagLength);

    return AES_SUCCESS;
}

//*****************************************************************************
//
// Verify the provided tag against the computed tag after a completed CCM or
// GCM operation.
//
//*****************************************************************************
uint32_t AESVerifyTag(const uint8_t *tag, uint32_t tagLength)
{
    uint32_t resultStatus;
    // The intermediate array is allocated on the stack to ensure users do not
    // point the tag they provide and the one computed at the same location.
    // That would cause memcmp to compare an array against itself. We could add
    // a check that verifies that the arrays are not the same. If we did that and
    // modified AESReadTag to just copy all 128 bits into a provided array,
    // we could save 16 bytes of stack space while making the API much more
    // complicated.
    uint8_t computedTag[AES_BLOCK_SIZE];

    resultStatus = AESReadTag(computedTag, tagLength);

    if (resultStatus != AES_SUCCESS) {
        return resultStatus;
    }

    resultStatus = memcmp(computedTag, tag, tagLength);

    if (resultStatus != 0) {
        return AES_TAG_VERIFICATION_FAILED;
    }

    return AES_SUCCESS;
}

//*****************************************************************************
//
// Configure the AES module for CCM mode
//
//*****************************************************************************
void AESConfigureCCMCtrl(uint32_t nonceLength, uint32_t macLength, bool encrypt)
{
    uint32_t ctrlVal = 0;

    ctrlVal = ((15 - nonceLength - 1) << CRYPTO_AESCTL_CCM_L_S);
    if ( macLength >= 2 ) {
        ctrlVal |= ((( macLength - 2 ) >> 1 ) << CRYPTO_AESCTL_CCM_M_S );
    }
    ctrlVal |= CRYPTO_AESCTL_CCM |
               CRYPTO_AESCTL_CTR |
               CRYPTO_AESCTL_SAVE_CONTEXT |
               CRYPTO_AESCTL_CTR_WIDTH_128_BIT;
    ctrlVal |= encrypt ? CRYPTO_AESCTL_DIR : 0;

    AESSetCtrl(ctrlVal);
}

//*****************************************************************************
//
// Configure an IV for CCM mode of operation
//
//*****************************************************************************
void AESWriteCCMInitializationVector(const uint8_t *nonce, uint32_t nonceLength)
{
    union {
        uint32_t word[4];
        uint8_t  byte[16];
    } initializationVector = {{0}};

    initializationVector.byte[0] = 15 - nonceLength - 1;

    memcpy(&(initializationVector.byte[1]), nonce, nonceLength);

    AESSetInitializationVector(initializationVector.word);
}
