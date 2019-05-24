/******************************************************************************
*  Filename:       sha2.c
*  Revised:        2018-04-17 15:57:27 +0200 (Tue, 17 Apr 2018)
*  Revision:       51892
*
*  Description:    Driver for the SHA-2 functions of the crypto module
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

#include "sha2.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  SHA2StartDMAOperation
    #define SHA2StartDMAOperation           NOROM_SHA2StartDMAOperation
    #undef  SHA2WaitForIRQFlags
    #define SHA2WaitForIRQFlags             NOROM_SHA2WaitForIRQFlags
    #undef  SHA2ComputeInitialHash
    #define SHA2ComputeInitialHash          NOROM_SHA2ComputeInitialHash
    #undef  SHA2ComputeIntermediateHash
    #define SHA2ComputeIntermediateHash     NOROM_SHA2ComputeIntermediateHash
    #undef  SHA2ComputeFinalHash
    #define SHA2ComputeFinalHash            NOROM_SHA2ComputeFinalHash
    #undef  SHA2ComputeHash
    #define SHA2ComputeHash                 NOROM_SHA2ComputeHash
#endif


static uint32_t SHA2ExecuteHash(const uint8_t *message, uint8_t *resultDigest, uint32_t *intermediateDigest, uint32_t totalMsgLength, uint32_t messageLength, uint32_t hashAlgorithm, bool initialHash, bool finalHash);


//*****************************************************************************
//
// Start a SHA-2 DMA operation.
//
//*****************************************************************************
void SHA2StartDMAOperation(uint8_t *channel0Addr, uint32_t channel0Length,  uint8_t *channel1Addr, uint32_t channel1Length)
{

    // Clear any outstanding events.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = CRYPTO_IRQCLR_RESULT_AVAIL_M | CRYPTO_IRQEN_DMA_IN_DONE_M;

    while(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & (CRYPTO_IRQSTAT_DMA_IN_DONE_M | CRYPTO_IRQSTAT_RESULT_AVAIL_M));

    if (channel0Addr) {
        // Configure the DMA controller - enable both DMA channels.
        HWREGBITW(CRYPTO_BASE + CRYPTO_O_DMACH0CTL, CRYPTO_DMACH0CTL_EN_BITN) = 1;

        // Base address of the payload data in ext. memory.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uint32_t)channel0Addr;

        // Payload data length in bytes, equal to the cipher text length.
        HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = channel0Length;
    }

    if (channel1Addr) {
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
uint32_t SHA2WaitForIRQFlags(uint32_t irqFlags)
{
    uint32_t irqTrigger = 0;
    // Wait for the DMA operation to complete. Add a delay to make sure we are
    // not flooding the bus with requests too much.
    do {
        CPUdelay(1);
    }
    while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & irqFlags & (CRYPTO_IRQSTAT_DMA_IN_DONE_M | CRYPTO_IRQSTAT_RESULT_AVAIL_M)));

    // Save the IRQ trigger source
    irqTrigger = HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT);

    // Clear IRQ flags
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = irqFlags;

    while(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & irqFlags & (CRYPTO_IRQSTAT_DMA_IN_DONE_M | CRYPTO_IRQSTAT_RESULT_AVAIL_M));

    return irqTrigger;
}

//*****************************************************************************
//
// Start a new SHA-2 hash operation.
//
//*****************************************************************************
uint32_t SHA2ComputeInitialHash(const uint8_t *message, uint32_t *intermediateDigest, uint32_t hashAlgorithm, uint32_t initialMessageLength)
{
    ASSERT(message);
    ASSERT((hashAlgorithm == SHA2_MODE_SELECT_SHA224) ||
           (hashAlgorithm == SHA2_MODE_SELECT_SHA256) ||
           (hashAlgorithm == SHA2_MODE_SELECT_SHA384) ||
           (hashAlgorithm == SHA2_MODE_SELECT_SHA512));
    ASSERT(!(intermediateDigest == NULL) && !((uint32_t)intermediateDigest & 0x03));

    return SHA2ExecuteHash(message, (uint8_t *)intermediateDigest, intermediateDigest, initialMessageLength, initialMessageLength, hashAlgorithm, true, false);
}

//*****************************************************************************
//
// Start an intermediate SHA-2 hash operation.
//
//*****************************************************************************
uint32_t SHA2ComputeIntermediateHash(const uint8_t *message, uint32_t *intermediateDigest, uint32_t hashAlgorithm, uint32_t intermediateMessageLength)
{
    ASSERT(message);
    ASSERT(!(intermediateDigest == NULL) && !((uint32_t)intermediateDigest & 0x03));
    ASSERT((hashAlgorithm == SHA2_MODE_SELECT_SHA224) ||
           (hashAlgorithm == SHA2_MODE_SELECT_SHA256) ||
           (hashAlgorithm == SHA2_MODE_SELECT_SHA384) ||
           (hashAlgorithm == SHA2_MODE_SELECT_SHA512));

    return SHA2ExecuteHash(message, (uint8_t *)intermediateDigest, intermediateDigest, 0, intermediateMessageLength, hashAlgorithm, false, false);
}

//*****************************************************************************
//
// Start an intermediate SHA-2 hash operation and finalize it.
//
//*****************************************************************************
uint32_t SHA2ComputeFinalHash(const uint8_t *message, uint8_t *resultDigest, uint32_t *intermediateDigest, uint32_t totalMsgLength, uint32_t messageLength, uint32_t hashAlgorithm)
{
    ASSERT(message);
    ASSERT(totalMsgLength);
    ASSERT(!(intermediateDigest == NULL) && !((uint32_t)intermediateDigest & 0x03));
    ASSERT(resultDigest);
    ASSERT((hashAlgorithm == SHA2_MODE_SELECT_SHA224) ||
           (hashAlgorithm == SHA2_MODE_SELECT_SHA256) ||
           (hashAlgorithm == SHA2_MODE_SELECT_SHA384) ||
           (hashAlgorithm == SHA2_MODE_SELECT_SHA512));

    return SHA2ExecuteHash(message, resultDigest, intermediateDigest, totalMsgLength, messageLength, hashAlgorithm, false, true);
}

//*****************************************************************************
//
// Start and finalize a new SHA-2 hash operation.
//
//*****************************************************************************
uint32_t SHA2ComputeHash(const uint8_t *message, uint8_t *resultDigest, uint32_t totalMsgLength, uint32_t hashAlgorithm)
{
    ASSERT(message);
    ASSERT(totalMsgLength);
    ASSERT(resultDigest);
    ASSERT((hashAlgorithm == SHA2_MODE_SELECT_SHA224) ||
       (hashAlgorithm == SHA2_MODE_SELECT_SHA256) ||
       (hashAlgorithm == SHA2_MODE_SELECT_SHA384) ||
       (hashAlgorithm == SHA2_MODE_SELECT_SHA512));

    return SHA2ExecuteHash(message, resultDigest, 0, totalMsgLength, totalMsgLength, hashAlgorithm, true, true);
}

//*****************************************************************************
//
// Start any SHA-2 hash operation.
//
//*****************************************************************************
static uint32_t SHA2ExecuteHash(const uint8_t *message, uint8_t *resultDigest, uint32_t *intermediateDigest, uint32_t totalMsgLength, uint32_t messageLength, uint32_t hashAlgorithm, bool initialHash, bool finalHash)
{
    uint8_t digestLength = 0;
    uint32_t dmaAlgorithmSelect = 0;

    SHA2ClearDigestAvailableFlag();

    switch (hashAlgorithm) {
        case SHA2_MODE_SELECT_SHA224:
            digestLength = SHA2_SHA224_DIGEST_LENGTH_BYTES;
            dmaAlgorithmSelect = SHA2_ALGSEL_SHA256;
            break;
        case SHA2_MODE_SELECT_SHA256:
            digestLength = SHA2_SHA256_DIGEST_LENGTH_BYTES;
            dmaAlgorithmSelect = SHA2_ALGSEL_SHA256;
            break;
        case SHA2_MODE_SELECT_SHA384:
            digestLength = SHA2_SHA384_DIGEST_LENGTH_BYTES;
            dmaAlgorithmSelect = SHA2_ALGSEL_SHA512;
            break;
        case SHA2_MODE_SELECT_SHA512:
            digestLength = SHA2_SHA512_DIGEST_LENGTH_BYTES;
            dmaAlgorithmSelect = SHA2_ALGSEL_SHA512;
            break;
        default:
            return SHA2_INVALID_ALGORITHM;
    }

    if (initialHash && finalHash) {
        // The empty string is a perfectly valid message. It obviously has a length of 0. The DMA cannot
        // handle running with a transfer length of 0. This workaround depends on the hash engine adding the
        // trailing 1 bit and 0-padding bits after the DMAtransfer is complete and not in the DMA itself.
        // totalMsgLength is purposefully not altered as it is appended to the end of the message during finalization
        // and determines how many padding-bytes are added.
        // Altering totalMsgLength would alter the final hash digest.
        // Because totalMsgLength specifies that the message is of length 0, the content of the byte loaded
        // through the DMA is irrelevant. It is overwritten internally in the hash engine.
        messageLength = messageLength ? messageLength : 1;
    }

    // Setting the incorrect number of bits here leads to the calculation of the correct result
    // but a failure to read them out.
    // The tag bit is set to read out the digest via DMA rather than through the slave interface.
    SHA2SelectAlgorithm(dmaAlgorithmSelect | (resultDigest ? SHA2_ALGSEL_TAG : 0));
    SHA2IntClear(SHA2_DMA_IN_DONE | SHA2_RESULT_RDY);
    SHA2IntEnable(SHA2_DMA_IN_DONE | SHA2_RESULT_RDY);

    HWREG(CRYPTO_BASE + CRYPTO_O_HASHMODE) = hashAlgorithm | (initialHash ? CRYPTO_HASHMODE_NEW_HASH_M : 0);

    // Only load the intermediate digest if requested.
    if (intermediateDigest && !initialHash) {
        SHA2SetDigest(intermediateDigest, digestLength);
    }

    // If this is the final hash, finalization is required. This means appending a 1 bit, padding the message until this section
    // is 448 bytes long, and adding the 64 bit total length of the message in bits. Thankfully, this is all done in hardware.
    if (finalHash) {
        // This specific length must be specified in bits not bytes.
        SHA2SetMessageLength(totalMsgLength * 8);
        HWREG(CRYPTO_BASE + CRYPTO_O_HASHIOBUFCTRL) = CRYPTO_HASHIOBUFCTRL_PAD_DMA_MESSAGE_M;

    }

    // The cast is fine in this case. SHA2StartDMAOperation channel one serves as input and no one does
    // hash operations in-place.
    SHA2StartDMAOperation((uint8_t *)message, messageLength,  resultDigest, digestLength);

    return SHA2_SUCCESS;
}
