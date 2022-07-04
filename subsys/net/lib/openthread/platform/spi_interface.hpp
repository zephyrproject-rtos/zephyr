/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
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
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction
 *   for radio communication using Spinel over SPI.
 *
 */

#include <openthread-system.h>

#include <lib/spinel/spinel_interface.hpp>

#include "spi_host.h"

struct RadioSpinelContext {
	otInstance *mInstance;
};

class SpiInterface
{
public:
	// This constructor initializes the object.

	// @param[in] aCallback         Callback on frame received
	// @param[in] aCallbackContext  Callback context
	// @param[in] aFrameBuffer      A reference to a `RxFrameBuffer` object.
	SpiInterface(ot::Spinel::SpinelInterface::ReceiveFrameCallback aCallback,
		     void *                                            aCallbackContext,
		     ot::Spinel::SpinelInterface::RxFrameBuffer &      aFrameBuffer);

	// The object destructor
	~SpiInterface(void);

	// This method encodes and sends a spinel frame to Radio Co-processor (RCP) over the socket.

	// This is blocking call, i.e., if the socket is not writable,
	// this method waits for it to become writable for up to `kMaxWaitTime` interval.

	// @param[in] aFrame     A pointer to buffer containing the spinel frame to send.
	// @param[in] aLength    The length (number of bytes) in the frame.

	// @retval OT_ERROR_NONE     Successfully encoded and sent the spinel frame.
	// @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to encode the frame.
	// @retval OT_ERROR_FAILED   Failed to send due to socket not becoming writable within `kMaxWaitTime`.
	otError SendFrame(const uint8_t *aFrame, uint16_t aLength);

	// This method waits for receiving part or all of spinel frame within specified interval.

	// @param[in]  aTimeout  The timeout value in microseconds.

	// @retval OT_ERROR_NONE             Part or all of spinel frame is received.
	// @retval OT_ERROR_RESPONSE_TIMEOUT No spinel frame is received within @p aTimeout.
	otError WaitForFrame(uint64_t& aTimeoutUs);

	// This method performs radio driver processing.

	// @param[in]   aContext        The context containing.
	void Process(const RadioSpinelContext &aContext);

	// This method deinitializes the interface to the RCP.
	otError Init(void);

	// This method deinitializes the interface to the RCP.
	void Deinit(void);

	// This method is called when RCP failure detected and resets internal states of the interface.
	void OnRcpReset(void);

	// This method is called when RCP is reset to recreate the connection with it.
	// Intentionally empty.
	otError ResetConnection(void);

private:
	uint8_t *GetRealRxFrameStart(uint8_t *aSpiRxFrameBuffer, uint8_t aAlignAllowance, uint16_t &aSkipLength);
	otError  DoSpiTransfer(uint8_t *aSpiRxFrameBuffer, uint32_t aTransferLength);
	otError  PushPullSpi(void);

	enum {
		kSpiAlignAllowanceMax = 16,
		kSpiFrameHeaderSize   = 5,
		kMsecPerSec           = 1000,
		kUsecPerMsec          = 1000,
		kSpiPollPeriodUs      = kMsecPerSec * kUsecPerMsec / 30,
		kSecPerDay            = 60 * 60 * 24,
		kResetHoldOnUsec      = 10 * kUsecPerMsec,
		kMaxFrameSize         = ot::Spinel::SpinelInterface::kMaxFrameSize,
	};

	ot::Spinel::SpinelInterface::ReceiveFrameCallback mReceiveFrameCallback;
	void *                                            mReceiveFrameContext;
	ot::Spinel::SpinelInterface::RxFrameBuffer &      mRxFrameBuffer;

	uint8_t mSpiAlignAllowance;
	uint16_t mSpiSmallPacketSize;

	uint64_t mSlaveResetCount;
	uint64_t mSpiFrameCount;
	uint64_t mSpiValidFrameCount;
	uint64_t mSpiGarbageFrameCount;
	uint64_t mSpiDuplexFrameCount;
	uint64_t mSpiUnresponsiveFrameCount;
	uint64_t mSpiRxFrameCount;
	uint64_t mSpiRxFrameByteCount;
	uint64_t mSpiTxFrameCount;
	uint64_t mSpiTxFrameByteCount;

	bool mSpiTxIsReady;
	uint16_t mSpiTxRefusedCount;
	uint16_t mSpiTxPayloadSize;
	uint8_t mSpiTxFrameBuffer[kMaxFrameSize + kSpiAlignAllowanceMax];

	uint16_t mSpiSlaveDataLen;

	// Non-copyable
	SpiInterface(const SpiInterface &) = delete;
	SpiInterface &operator = (const SpiInterface &) = delete;
};
