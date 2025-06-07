#include "openthread/instance.h"
#include "openthread/ip6.h"
#include "openthread/trel.h"

void otPlatTrelEnable(otInstance *aInstance, uint16_t *aUdpPort)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aUdpPort);
}

void otPlatTrelDisable(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
}

void otPlatTrelSend(otInstance *aInstance, const uint8_t *aUdpPayload, uint16_t aUdpPayloadLen,
		    const otSockAddr *aDestSockAddr)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aUdpPayload);
	OT_UNUSED_VARIABLE(aUdpPayloadLen);
	OT_UNUSED_VARIABLE(aDestSockAddr);
}

void otPlatTrelNotifyPeerSocketAddressDifference(otInstance *aInstance,
						 const otSockAddr *aPeerSockAddr,
						 const otSockAddr *aRxSockAddr)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aPeerSockAddr);
	OT_UNUSED_VARIABLE(aRxSockAddr);
}

void otPlatTrelRegisterService(otInstance *aInstance, uint16_t aPort, const uint8_t *aTxtData,
			       uint8_t aTxtLength)
{
	OT_UNUSED_VARIABLE(aInstance);
	OT_UNUSED_VARIABLE(aPort);
	OT_UNUSED_VARIABLE(aTxtData);
	OT_UNUSED_VARIABLE(aTxtLength);
}

const otPlatTrelCounters *otPlatTrelGetCounters(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);

	return NULL;
}

void otPlatTrelResetCounters(otInstance *aInstance)
{
	OT_UNUSED_VARIABLE(aInstance);
}