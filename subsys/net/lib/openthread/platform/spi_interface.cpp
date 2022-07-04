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

#include <ncp/ncp_spi.hpp>

#include "spi_interface.hpp"

using ot::Spinel::SpinelInterface;

SpiInterface::SpiInterface(SpinelInterface::ReceiveFrameCallback aCallback,
			   void *                                aCallbackContext,
			   SpinelInterface::RxFrameBuffer &      aFrameBuffer)
	: mReceiveFrameCallback(aCallback)
	, mReceiveFrameContext(aCallbackContext)
	, mRxFrameBuffer(aFrameBuffer)
	, mSlaveResetCount(0)
	, mSpiFrameCount(0)
	, mSpiValidFrameCount(0)
	, mSpiGarbageFrameCount(0)
	, mSpiDuplexFrameCount(0)
	, mSpiUnresponsiveFrameCount(0)
	, mSpiRxFrameCount(0)
	, mSpiRxFrameByteCount(0)
	, mSpiTxFrameCount(0)
	, mSpiTxFrameByteCount(0)
	, mSpiTxIsReady(false)
	, mSpiTxRefusedCount(0)
	, mSpiTxPayloadSize(0)
	, mSpiSlaveDataLen(0)
{
    // empty
}

otError SpiInterface::Init(void)
{
	mSpiSmallPacketSize = CONFIG_OPENTHREAD_HOSTPROCESSOR_SPI_SMALL_PACKET_SIZE;
	mSpiAlignAllowance = CONFIG_OPENTHREAD_HOSTPROCESSOR_SPI_ALIGN_ALLOWANCE;

	platformSpiHostInit();

	return OT_ERROR_NONE;
}

SpiInterface::~SpiInterface(void)
{
	Deinit();
}

void SpiInterface::Deinit(void)
{
	// empty
}

uint8_t *SpiInterface::GetRealRxFrameStart(uint8_t *aSpiRxFrameBuffer, uint8_t aAlignAllowance, uint16_t &aSkipLength)
{
	uint8_t *start = aSpiRxFrameBuffer;
	const uint8_t *end = aSpiRxFrameBuffer + aAlignAllowance;

	for (; start != end && start[0] == 0xff; start++)
		;

	aSkipLength = static_cast < uint16_t > (start - aSpiRxFrameBuffer);

	return start;
}

otError SpiInterface::DoSpiTransfer(uint8_t *aSpiRxFrameBuffer, uint32_t aTransferLength)
{
	#define LENGTH_ALIGN 4
	int ret = 0;

	size_t remainder = aTransferLength % LENGTH_ALIGN;
	if (remainder) {
		aTransferLength += LENGTH_ALIGN - remainder;
	}

	ret = platformSpiHostTransfer(mSpiTxFrameBuffer, aSpiRxFrameBuffer, aTransferLength);

	if (ret == 0) {
		mSpiFrameCount++;
	}

	return (ret < 0) ? OT_ERROR_FAILED : OT_ERROR_NONE;
}

otError SpiInterface::PushPullSpi(void)
{
	otError error = OT_ERROR_FAILED;
	uint16_t spiTransferBytes = 0;
	uint8_t successfulExchanges = 0;
	bool discardRxFrame = true;
	uint8_t *spiRxFrameBuffer;
	uint8_t *spiRxFrame;
	uint8_t slaveHeader;
	uint16_t slaveAcceptLen;
	ot::Ncp::SpiFrame txFrame(mSpiTxFrameBuffer);
	uint16_t skipAlignAllowanceLength;

	// Set the reset flag to indicate to our slave that we are coming up from scratch.
	txFrame.SetHeaderFlagByte(mSpiValidFrameCount == 0);

	// Zero out our rx_accept and our data_len for now.
	txFrame.SetHeaderAcceptLen(0);
	txFrame.SetHeaderDataLen(0);

	// Sanity check.
	if (mSpiSlaveDataLen > kMaxFrameSize) {
		mSpiSlaveDataLen = 0;
	}

	if (mSpiTxIsReady) {
		// Go ahead and try to immediately send a frame if we have it queued up.
		txFrame.SetHeaderDataLen(mSpiTxPayloadSize);

		if (mSpiTxPayloadSize > spiTransferBytes) {
			spiTransferBytes = mSpiTxPayloadSize;
		}
	}

	if (mSpiSlaveDataLen != 0) {
		// In a previous transaction the slave indicated it had something to send us. Make sure our transaction
		// is large enough to handle it.
		if (mSpiSlaveDataLen > spiTransferBytes) {
			spiTransferBytes = mSpiSlaveDataLen;
		}
	} else {
		// Set up a minimum transfer size to allow small frames the slave wants to send us to be handled in a
		// single transaction.
		if (spiTransferBytes < mSpiSmallPacketSize) {
			spiTransferBytes = mSpiSmallPacketSize;
		}
	}

	txFrame.SetHeaderAcceptLen(spiTransferBytes);

	// Set skip length to make MultiFrameBuffer to reserve a space in front of the frame buffer.
	SuccessOrExit(error = mRxFrameBuffer.SetSkipLength(kSpiFrameHeaderSize));

	// Check whether the remaining frame buffer has enough space to store the data to be received.
	VerifyOrExit(mRxFrameBuffer.GetFrameMaxLength() >= spiTransferBytes + mSpiAlignAllowance);

	// Point to the start of the reserved buffer.
	spiRxFrameBuffer = mRxFrameBuffer.GetFrame() - kSpiFrameHeaderSize;

	// Set the total number of bytes to be transmitted.
	spiTransferBytes += kSpiFrameHeaderSize + mSpiAlignAllowance;

	// Perform the SPI transaction.
	error = DoSpiTransfer(spiRxFrameBuffer, spiTransferBytes);

	if (error != OT_ERROR_NONE) {
		otLogWarnPlat("PushPullSpi:DoSpiTransfer: error=%d", error);
		ExitNow();
	}
	// Account for misalignment (0xFF bytes at the start)
	spiRxFrame = GetRealRxFrameStart(spiRxFrameBuffer, mSpiAlignAllowance, skipAlignAllowanceLength);

	{
		ot::Ncp::SpiFrame rxFrame(spiRxFrame);

		otLogDebgPlat("spi_transfer TX: H:%02X ACCEPT:%d DATA:%d", txFrame.GetHeaderFlagByte(),
		txFrame.GetHeaderAcceptLen(), txFrame.GetHeaderDataLen());
		otLogDebgPlat("spi_transfer RX: H:%02X ACCEPT:%d DATA:%d", rxFrame.GetHeaderFlagByte(),
		rxFrame.GetHeaderAcceptLen(), rxFrame.GetHeaderDataLen());

		slaveHeader = rxFrame.GetHeaderFlagByte();
		if ((slaveHeader == 0xFF) || (slaveHeader == 0x00)) {
		if ((slaveHeader == spiRxFrame[1]) && (slaveHeader == spiRxFrame[2]) && (slaveHeader == spiRxFrame[3]) &&
			(slaveHeader == spiRxFrame[4])) {
			// Device is off or in a bad state. In some cases may be induced by flow control.
			if (mSpiSlaveDataLen == 0) {
				otLogDebgPlat("Slave did not respond to frame. (Header was all 0x%02X)", slaveHeader);
			} else {
				otLogWarnPlat("Slave did not respond to frame. (Header was all 0x%02X)", slaveHeader);
			}

			mSpiUnresponsiveFrameCount++;
		} else {
			// Header is full of garbage
			mSpiGarbageFrameCount++;

			otLogWarnPlat("Garbage in header : %02X %02X %02X %02X %02X", spiRxFrame[0], spiRxFrame[1],
			spiRxFrame[2], spiRxFrame[3], spiRxFrame[4]);
		}

		mSpiTxRefusedCount++;
		ExitNow();
		}

		slaveAcceptLen = rxFrame.GetHeaderAcceptLen();
		mSpiSlaveDataLen = rxFrame.GetHeaderDataLen();

		if (!rxFrame.IsValid() || (slaveAcceptLen > kMaxFrameSize) || (mSpiSlaveDataLen > kMaxFrameSize)) {
			mSpiGarbageFrameCount++;
			mSpiTxRefusedCount++;
			mSpiSlaveDataLen = 0;

			otLogWarnPlat("Garbage in header : %02X %02X %02X %02X %02X",
				spiRxFrame[0], spiRxFrame[1], spiRxFrame[2], spiRxFrame[3], spiRxFrame[4]);

			ExitNow();
		}

		mSpiValidFrameCount++;

		if (rxFrame.IsResetFlagSet()) {
			mSlaveResetCount++;
		}

		// Handle received packet, if any.
		if ((mSpiSlaveDataLen != 0) && (mSpiSlaveDataLen <= txFrame.GetHeaderAcceptLen())) {
			mSpiRxFrameByteCount += mSpiSlaveDataLen;
			mSpiSlaveDataLen = 0;
			mSpiRxFrameCount++;
			successfulExchanges++;

			// Set the skip length to skip align bytes and SPI frame header.
			SuccessOrExit(mRxFrameBuffer.SetSkipLength(skipAlignAllowanceLength + kSpiFrameHeaderSize));
			// Set the received frame length.
			SuccessOrExit(mRxFrameBuffer.SetLength(rxFrame.GetHeaderDataLen()));

			// Upper layer will free the frame buffer.
			discardRxFrame = false;

			mReceiveFrameCallback(mReceiveFrameContext);
		}
	}
	// Handle transmitted packet, if any.
	if (mSpiTxIsReady && (mSpiTxPayloadSize == txFrame.GetHeaderDataLen())) {
		if (txFrame.GetHeaderDataLen() <= slaveAcceptLen) {
			// Our outbound packet has been successfully transmitted. Clear mSpiTxPayloadSize and mSpiTxIsReady so
			// that uplayer can pull another packet for us to send.
			successfulExchanges++;

			mSpiTxFrameCount++;
			mSpiTxFrameByteCount += mSpiTxPayloadSize;

			mSpiTxIsReady = false;
			mSpiTxPayloadSize = 0;
			mSpiTxRefusedCount = 0;
		} else {
			// The slave wasn't ready for what we had to send them. Incrementing this counter will turn on rate
			// limiting so that we don't waste a ton of CPU bombarding them with useless SPI transfers.
			mSpiTxRefusedCount++;
		}
	}

	if (!mSpiTxIsReady) {
		mSpiTxRefusedCount = 0;
	}

	if (successfulExchanges == 2) {
		mSpiDuplexFrameCount++;
	}

exit:
	if (discardRxFrame) {
		mRxFrameBuffer.DiscardFrame();
	}

	return error;
}

otError SpiInterface::WaitForFrame(uint64_t& aTimeoutUs)
{
	if (!platformSpiHostWaitForFrame(aTimeoutUs)) {
		return OT_ERROR_RESPONSE_TIMEOUT;
	}

	IgnoreError(PushPullSpi());

	return OT_ERROR_NONE;
}

otError SpiInterface::SendFrame(const uint8_t *aFrame, uint16_t aLength)
{
	if (aLength >= (kMaxFrameSize - kSpiFrameHeaderSize)) {
		return OT_ERROR_NO_BUFS;
	}

	if (mSpiTxIsReady) {
		return OT_ERROR_BUSY;
	}

	memcpy(&mSpiTxFrameBuffer[kSpiFrameHeaderSize], aFrame, aLength);

	mSpiTxIsReady = true;
	mSpiTxPayloadSize = aLength;

	IgnoreError(PushPullSpi());

	return OT_ERROR_NONE;
}

void SpiInterface::Process(const RadioSpinelContext& aContext)
{
	platformSpiHostProcess(aContext.mInstance);

	// Service the SPI port if we can receive a packet or we have a packet to be sent.
	if (mSpiTxIsReady || platformSpiHostCheckInterrupt()) {
		// We guard this with the above check because we don't want to overwrite any previously received frames.
		PushPullSpi();
	}
}

void SpiInterface::OnRcpReset(void)
{
	otLogWarnPlat("OnRcpReset");
	platformSpiHostInit();
}

otError SpiInterface::ResetConnection(void)
{
	platformSpiHostInit();

	return OT_ERROR_NONE;
}
