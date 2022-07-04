/*
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction
 *   for radio communication using Spinel.
 *
 */
#include <openthread-system.h>
#include <openthread/platform/radio.h>
#include <zephyr/fatal.h>
#include <zephyr/sys/util_macro.h>

#include <lib/spinel/radio_spinel.hpp>

#ifdef CONFIG_OPENTHREAD_HOSTPROCESSOR_SPI
#include "spi_interface.hpp"

typedef ot::Spinel::RadioSpinel<SpiInterface, RadioSpinelContext> PlatformSpiRadioSpinel;
#else
#error "Unsupported OpenThread Host transport type"
#endif

OT_DEFINE_ALIGNED_VAR(gRadioSpinalInstanceRaw, sizeof(PlatformSpiRadioSpinel), uint64_t);

static PlatformSpiRadioSpinel& RadioSpinelInstance(void)
{
	void *instance = &gRadioSpinalInstanceRaw;

	return *static_cast <PlatformSpiRadioSpinel *> (instance);
}

static void InitPlatformRadioSpinelInstance(void)
{
    new(&gRadioSpinalInstanceRaw) PlatformSpiRadioSpinel();
}

extern "C" void platformRadioInit(void)
{
	InitPlatformRadioSpinelInstance();
	RadioSpinelInstance().GetSpinelInterface().Init();
	RadioSpinelInstance().Init(IS_ENABLED(CONFIG_OPENTHREAD_HOSTPROCESSOR_RESET_RADIO), false, false);
}

extern "C" void platformRadioDeinit(void)
{
	RadioSpinelInstance().Deinit();
}

extern "C" void platformRadioProcess(otInstance *aInstance)
{
	RadioSpinelContext ctx = {
		.mInstance = aInstance,
	};

	RadioSpinelInstance().Process(ctx);
}

static inline otExtAddress otExtAddressLittleEndian(const otExtAddress *aAddress)
{
	otExtAddress addr;

	for (size_t i = 0; i < sizeof(addr); i++) {
		addr.m8[i] = aAddress->m8[sizeof(addr) - 1 - i];
	}

	return addr;
}

void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
	OT_UNUSED_VARIABLE(aInstance);

	SuccessOrDie(RadioSpinelInstance().GetIeeeEui64(aIeeeEui64));
}

void otPlatRadioSetPanId(otInstance *aInstance, uint16_t panid)
{
	OT_UNUSED_VARIABLE(aInstance);

	SuccessOrDie(RadioSpinelInstance().SetPanId(panid));
}

void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
	OT_UNUSED_VARIABLE(aInstance);

	SuccessOrDie(RadioSpinelInstance().SetExtendedAddress(otExtAddressLittleEndian(aAddress)));
}

void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
	OT_UNUSED_VARIABLE(aInstance);

	SuccessOrDie(RadioSpinelInstance().SetShortAddress(aAddress));
}

void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
	OT_UNUSED_VARIABLE(aInstance);

	SuccessOrDie(RadioSpinelInstance().SetPromiscuous(aEnable));
}

bool otPlatRadioIsEnabled(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().IsEnabled();
}

otError otPlatRadioEnable(otInstance *aInstance)
{
	return RadioSpinelInstance().Enable(aInstance);
}

otError otPlatRadioDisable(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().Disable();
}

otError otPlatRadioSleep(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().Sleep();
}

otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().Receive(aChannel);
}

otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().Transmit(*aFrame);
}

otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return &RadioSpinelInstance().GetTransmitFrame();
}

int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().GetRssi();
}

otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().GetRadioCaps();
}

const char *otPlatRadioGetVersionString(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().GetVersion();
}

bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().IsPromiscuous();
}

void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
	OT_UNUSED_VARIABLE(aInstance);

	SuccessOrDie(RadioSpinelInstance().EnableSrcMatch(aEnable));
}

otError otPlatRadioAddSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().AddSrcMatchShortEntry(aShortAddress);
}

otError otPlatRadioAddSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().AddSrcMatchExtEntry(otExtAddressLittleEndian(aExtAddress));
}

otError otPlatRadioClearSrcMatchShortEntry(otInstance *aInstance, uint16_t aShortAddress)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().ClearSrcMatchShortEntry(aShortAddress);
}

otError otPlatRadioClearSrcMatchExtEntry(otInstance *aInstance, const otExtAddress *aExtAddress)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().ClearSrcMatchExtEntry(otExtAddressLittleEndian(aExtAddress));
}

void otPlatRadioClearSrcMatchShortEntries(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	SuccessOrDie(RadioSpinelInstance().ClearSrcMatchShortEntries());
}

void otPlatRadioClearSrcMatchExtEntries(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	SuccessOrDie(RadioSpinelInstance().ClearSrcMatchExtEntries());
}

otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().EnergyScan(aScanChannel, aScanDuration);
}

otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
	OT_UNUSED_VARIABLE(aInstance);

	if (aPower == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	return RadioSpinelInstance().GetTransmitPower(*aPower);
}

otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().SetTransmitPower(aPower);
}

otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);

	if (aThreshold == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	return RadioSpinelInstance().GetCcaEnergyDetectThreshold(*aThreshold);
}

otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().SetCcaEnergyDetectThreshold(aThreshold);
}

int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().GetReceiveSensitivity();
}

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
otError otPlatRadioSetCoexEnabled(otInstance *aInstance, bool aEnabled)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().SetCoexEnabled(aEnabled);
}

bool otPlatRadioIsCoexEnabled(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().IsCoexEnabled();
}

otError otPlatRadioGetCoexMetrics(otInstance *aInstance, otRadioCoexMetrics *aCoexMetrics)
{
	OT_UNUSED_VARIABLE(aInstance);

	if (aCoexMetrics == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	return RadioSpinelInstance().GetCoexMetrics(*aCoexMetrics);
}
#endif

#if OPENTHREAD_CONFIG_DIAG_ENABLE
otError otPlatDiagProcess(otInstance *aInstance, int argc, char *argv[], char *aOutput, size_t aOutputMaxLen)
{
	// deliver the platform specific diags commands to radio only ncp.
	OT_UNUSED_VARIABLE(aInstance);

	char cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE] = { '\0' };
	char *cur = cmd;
	char *end = cmd + sizeof(cmd);

	for (int index = 0; index < argc; index++) {
		cur += snprintf(cur, static_cast < size_t > (end - cur), "%s ", argv[index]);
	}

	return RadioSpinelInstance().PlatDiagProcess(cmd, aOutput, aOutputMaxLen);
}

void otPlatDiagModeSet(bool aMode)
{
	if (RadioSpinelInstance().PlatDiagProcess(aMode ? "start" : "stop", NULL, 0) == OT_ERROR_NONE) {
		RadioSpinelInstance().SetDiagEnabled(aMode);
	}
}

bool otPlatDiagModeGet(void)
{
	return RadioSpinelInstance().IsDiagEnabled();
}

void otPlatDiagTxPowerSet(int8_t aTxPower)
{
	char cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

	snprintf(cmd, sizeof(cmd), "power %d", aTxPower);

	RadioSpinelInstance().PlatDiagProcess(cmd, NULL, 0);
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
	char cmd[OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE];

	snprintf(cmd, sizeof(cmd), "channel %d", aChannel);
	RadioSpinelInstance().PlatDiagProcess(cmd, NULL, 0);
}

void otPlatDiagRadioReceived(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aFrame);
	OT_UNUSED_VARIABLE(aError);
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
}
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE

uint32_t otPlatRadioGetSupportedChannelMask(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

        return RadioSpinelInstance().GetRadioChannelMask(false);
}

uint32_t otPlatRadioGetPreferredChannelMask(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().GetRadioChannelMask(true);
}

otRadioState otPlatRadioGetState(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return RadioSpinelInstance().GetState();
}

void otPlatRadioSetMacKey(otInstance *            aInstance,
			  uint8_t                 aKeyIdMode,
			  uint8_t                 aKeyId,
			  const otMacKeyMaterial *aPrevKey,
			  const otMacKeyMaterial *aCurrKey,
			  const otMacKeyMaterial *aNextKey,
			  otRadioKeyType aKeyType)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aKeyType);

	RadioSpinelInstance().SetMacKey(aKeyIdMode, aKeyId, aPrevKey, aCurrKey, aNextKey);
}

void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
	OT_UNUSED_VARIABLE(aInstance);

	RadioSpinelInstance().SetMacFrameCounter(aMacFrameCounter);
}
