/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lorawan_se.h"
#include <secure-element.h>

/**
 * Active Secure Element used by the LoRaWAN stack
 * Must be set before any operation is done by the LoRaWAN stack
 */
static const struct lorawan_se *active_se;

void lorawan_se_register(const struct lorawan_se *se)
{
	__ASSERT(!active_se,
		 "Only one Secure Element can be active at all time");
	__ASSERT_NO_MSG(se->get_joineui);
	__ASSERT_NO_MSG(se->set_joineui);
	__ASSERT_NO_MSG(se->get_pin);
	__ASSERT_NO_MSG(se->set_pin);
	__ASSERT_NO_MSG(se->set_deveui);
	__ASSERT_NO_MSG(se->get_deveui);
	__ASSERT_NO_MSG(se->init);
	__ASSERT_NO_MSG(se->set_key);
	__ASSERT_NO_MSG(se->compute_cmac);
	__ASSERT_NO_MSG(se->verify_cmac);
	__ASSERT_NO_MSG(se->encrypt);
	__ASSERT_NO_MSG(se->derive);
	__ASSERT_NO_MSG(se->process_join_accept);

	active_se = se;
}

int lorawan_se_update(SecureElementNvmData_t *data)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->update(data);
}

SecureElementStatus_t SecureElementInit(SecureElementNvmData_t *nvm)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->init(nvm);
}

SecureElementStatus_t SecureElementSetKey(KeyIdentifier_t keyID, uint8_t *key)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->set_key(keyID, key);
}

SecureElementStatus_t SecureElementComputeAesCmac(uint8_t *micBxBuffer,
						  uint8_t *buffer,
						  uint16_t size,
						  KeyIdentifier_t keyID,
						  uint32_t *cmac)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->compute_cmac(micBxBuffer,
				       buffer,
				       size,
				       keyID,
				       cmac);
}

SecureElementStatus_t SecureElementVerifyAesCmac(uint8_t *buffer,
						 uint16_t size,
						 uint32_t expectedCmac,
						 KeyIdentifier_t keyID)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->verify_cmac(buffer, size, expectedCmac, keyID);
}

SecureElementStatus_t SecureElementAesEncrypt(uint8_t *buffer,
					      uint16_t size,
					      KeyIdentifier_t keyID,
					      uint8_t *encBuffer)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->encrypt(buffer, size, keyID, encBuffer);
}

SecureElementStatus_t
SecureElementDeriveAndStoreKey(uint8_t *input,
			       KeyIdentifier_t rootKeyID,
			       KeyIdentifier_t targetKeyID)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->derive(input, rootKeyID, targetKeyID);
}

SecureElementStatus_t
SecureElementProcessJoinAccept(JoinReqIdentifier_t joinReqType,
			       uint8_t *joinEui,
			       uint16_t devNonce,
			       uint8_t *encJoinAccept,
			       uint8_t encJoinAcceptSize,
			       uint8_t *decJoinAccept,
			       uint8_t *versionMinor)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->process_join_accept(joinReqType,
					      joinEui,
					      devNonce,
					      encJoinAccept,
					      encJoinAcceptSize,
					      decJoinAccept,
					      versionMinor);
}

SecureElementStatus_t SecureElementSetDevEui(uint8_t *devEui)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->set_deveui(devEui);
}

uint8_t *SecureElementGetDevEui(void)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->get_deveui();
}

SecureElementStatus_t SecureElementSetJoinEui(uint8_t *joinEui)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->set_joineui(joinEui);
}

uint8_t *SecureElementGetJoinEui(void)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->get_joineui();
}

SecureElementStatus_t SecureElementSetPin(uint8_t *pin)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->set_pin(pin);
}

uint8_t *SecureElementGetPin(void)
{
	__ASSERT(active_se, "No Secure Element is registered");

	return active_se->get_pin();
}
