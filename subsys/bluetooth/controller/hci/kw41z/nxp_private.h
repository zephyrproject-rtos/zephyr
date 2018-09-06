/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2018 NXP Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NXP_PRIVATE_H_
#define _NXP_PRIVATE_H_

/*
 * -----------------------------------------------------------------------------
 *
 * Exported symbols/typdefs from the NXP BLE Link Layer library
 *
 * -----------------------------------------------------------------------------
 */


/* -----------------------------------------------------------------------------
 *                              Constants
 **------------------------------------------------------------------------------
 */

/* NXP KW41Z BLE result type - the return value of BLE API functions */
typedef enum {
	/*
	 *
	 * Generic result values, base = 0x0000
	 *
	 */

	gBleStatusBase_c = 0x0000,                                                              /*!< General status base. */
	gBleSuccess_c                                               = gBleStatusBase_c | 0x00,  /*!< Function executed successfully. */

	gBleInvalidParameter_c                                      = gBleStatusBase_c | 0x01,  /*!< Parameter has an invalid value or is outside the accepted range. */
	gBleOverflow_c                                              = gBleStatusBase_c | 0x02,  /*!< An internal limit is reached. */
	gBleUnavailable_c                                           = gBleStatusBase_c | 0x03,  /*!< A requested parameter is not available. */
	gBleFeatureNotSupported_c                                   = gBleStatusBase_c | 0x04,  /*!< The requested feature is not supported by this stack version. */
	gBleOutOfMemory_c                                           = gBleStatusBase_c | 0x05,  /*!< An internal memory allocation failed. */
	gBleAlreadyInitialized_c                                    = gBleStatusBase_c | 0x06,  /*!< Ble_HostInitialize function is incorrectly called a second time. */
	gBleOsError_c                                               = gBleStatusBase_c | 0x07,  /*!< An error occurred at the OS level. */
	gBleUnexpectedError_c                                       = gBleStatusBase_c | 0x08,  /*!< A "should never get here"-type error occurred. */
	gBleInvalidState_c                                          = gBleStatusBase_c | 0x09,  /*!< The requested API cannot be called in the current state. */

	/*
	 *
	 * HCI result values
	 *
	 */

	gHciStatusBase_c = 0x0100,
	gHciSuccess_c                                               = gBleSuccess_c,
	/* HCI standard status codes */
	gHciUnknownHciCommand_c                                     = gHciStatusBase_c | 0x01,
	gHciUnknownConnectionIdentifier_c                           = gHciStatusBase_c | 0x02,
	gHciHardwareFailure_c                                       = gHciStatusBase_c | 0x03,
	gHciPageTimeout_c                                           = gHciStatusBase_c | 0x04,
	gHciAuthenticationFailure_c                                 = gHciStatusBase_c | 0x05,
	gHciPinOrKeyMissing_c                                       = gHciStatusBase_c | 0x06,
	gHciMemoryCapacityExceeded_c                                = gHciStatusBase_c | 0x07,
	gHciConnectionTimeout_c                                     = gHciStatusBase_c | 0x08,
	gHciConnectionLimitExceeded_c                               = gHciStatusBase_c | 0x09,
	gHciSynchronousConnectionLimitToADeviceExceeded_c           = gHciStatusBase_c | 0x0A,
	gHciAclConnectionAlreadyExists_c                            = gHciStatusBase_c | 0x0B,
	gHciCommandDisallowed_c                                     = gHciStatusBase_c | 0x0C,
	gHciConnectionRejectedDueToLimitedResources_c               = gHciStatusBase_c | 0x0D,
	gHciConnectionRejectedDueToSecurityReasons_c                = gHciStatusBase_c | 0x0E,
	gHciConnectionRejectedDueToUnacceptableBdAddr_c             = gHciStatusBase_c | 0x0F,
	gHciConnectionAcceptTimeoutExceeded_c                       = gHciStatusBase_c | 0x10,
	gHciUnsupportedFeatureOrParameterValue_c                    = gHciStatusBase_c | 0x11,
	gHciInvalidHciCommandParameters_c                           = gHciStatusBase_c | 0x12,
	gHciRemoteUserTerminatedConnection_c                        = gHciStatusBase_c | 0x13,
	gHciRemoteDeviceTerminatedConnectionLowResources_c          = gHciStatusBase_c | 0x14,
	gHciRemoteDeviceTerminatedConnectionPowerOff_c              = gHciStatusBase_c | 0x15,
	gHciConnectionTerminatedByLocalHost_c                       = gHciStatusBase_c | 0x16,
	gHciRepeatedAttempts_c                                      = gHciStatusBase_c | 0x17,
	gHciPairingNotAllowed_c                                     = gHciStatusBase_c | 0x18,
	gHciUnknownLpmPdu_c                                         = gHciStatusBase_c | 0x19,
	gHciUnsupportedRemoteFeature_c                              = gHciStatusBase_c | 0x1A,
	gHciScoOffsetRejected_c                                     = gHciStatusBase_c | 0x1B,
	gHciScoIntervalRejected_c                                   = gHciStatusBase_c | 0x1C,
	gHciScoAirModeRejected_c                                    = gHciStatusBase_c | 0x1D,
	gHciInvalidLpmParameters_c                                  = gHciStatusBase_c | 0x1E,
	gHciUnspecifiedError_c                                      = gHciStatusBase_c | 0x1F,
	gHciUnsupportedLpmParameterValue_c                          = gHciStatusBase_c | 0x20,
	gHciRoleChangeNotAllowed_c                                  = gHciStatusBase_c | 0x21,
	gHciLLResponseTimeout_c                                     = gHciStatusBase_c | 0x22,
	gHciLmpErrorTransactionCollision_c                          = gHciStatusBase_c | 0x23,
	gHciLmpPduNotAllowed_c                                      = gHciStatusBase_c | 0x24,
	gHciEncryptionModeNotAcceptable_c                           = gHciStatusBase_c | 0x25,
	gHciLinkKeyCannotBeChanged_c                                = gHciStatusBase_c | 0x26,
	gHciRequestedQosNotSupported_c                              = gHciStatusBase_c | 0x27,
	gHciInstantPassed_c                                         = gHciStatusBase_c | 0x28,
	gHciPairingWithUnitKeyNotSupported_c                        = gHciStatusBase_c | 0x29,
	gHciDifferentTransactionCollision_c                         = gHciStatusBase_c | 0x2A,
	gHciReserved_0x2B_c                                         = gHciStatusBase_c | 0x2B,
	gHciQosNotAcceptableParameter_c                             = gHciStatusBase_c | 0x2C,
	gHciQosRejected_c                                           = gHciStatusBase_c | 0x2D,
	gHciChannelClassificationNotSupported_c                     = gHciStatusBase_c | 0x2E,
	gHciInsufficientSecurity_c                                  = gHciStatusBase_c | 0x2F,
	gHciParameterOutOfMandatoryRange_c                          = gHciStatusBase_c | 0x30,
	gHciReserved_0x31_c                                         = gHciStatusBase_c | 0x31,
	gHciRoleSwitchPending_c                                     = gHciStatusBase_c | 0x32,
	gHciReserved_0x33_c                                         = gHciStatusBase_c | 0x33,
	gHciReservedSlotViolation_c                                 = gHciStatusBase_c | 0x34,
	gHciRoleSwitchFailed_c                                      = gHciStatusBase_c | 0x35,
	gHciExtendedInquiryResponseTooLarge_c                       = gHciStatusBase_c | 0x36,
	gHciSecureSimplePairingNotSupportedByHost_c                 = gHciStatusBase_c | 0x37,
	gHciHostBusyPairing_c                                       = gHciStatusBase_c | 0x38,
	gHciConnectionRejectedDueToNoSuitableChannelFound_c         = gHciStatusBase_c | 0x39,
	gHciControllerBusy_c                                        = gHciStatusBase_c | 0x3A,
	gHciUnacceptableConnectionParameters_c                      = gHciStatusBase_c | 0x3B,
	gHciDirectedAdvertisingTimeout_c                            = gHciStatusBase_c | 0x3C,
	gHciConnectionTerminatedDueToMicFailure_c                   = gHciStatusBase_c | 0x3D,
	gHciConnectionFailedToBeEstablished_c                       = gHciStatusBase_c | 0x3E,
	gHciMacConnectionFailed_c                                   = gHciStatusBase_c | 0x3F,
	gHciCoarseClockAdjustmentRejected_c                         = gHciStatusBase_c | 0x40,

	/* HCI internal status codes */
	gHciAlreadyInit_c                                           = gHciStatusBase_c | 0xA0,
	gHciInvalidParameter_c                                      = gHciStatusBase_c | 0xA1,
	gHciCallbackNotInstalled_c                                  = gHciStatusBase_c | 0xA2,
	gHciCallbackAlreadyInstalled_c                              = gHciStatusBase_c | 0xA3,
	gHciCommandNotSupported_c                                   = gHciStatusBase_c | 0xA4,
	gHciEventNotSupported_c                                     = gHciStatusBase_c | 0xA5,
	gHciTransportError_c                                        = gHciStatusBase_c | 0xA6,

	/*
	 *
	 * Controller result values
	 *
	 */
	gCtrlStatusBase_c = 0x0200,
	gCtrlSuccess_c                                              = gBleSuccess_c,
} bleResult_t;


/*
 * -----------------------------------------------------------------------------
 *                              Macros
 * -----------------------------------------------------------------------------
 */


/*
 * -----------------------------------------------------------------------------
 *                        Structures/Typedefs
 * -----------------------------------------------------------------------------
 */
typedef uint8_t bool_t;

typedef enum {
	gHciCommandPacket_c          = 0x01,    /* HCI Command       */
	gHciDataPacket_c             = 0x02,    /* L2CAP Data Packet */
	gHciSynchronousDataPacket_c  = 0x03,    /* Not used in BLE   */
	gHciEventPacket_c            = 0x04,    /* HCI Event         */
} hciPacketType_t;

typedef enum txChannelType_tag {
	gAdvTxChannel_c,
	gConnTxChannel_c
} txChannelType_t;

typedef bleResult_t (*gHostHciRecvCallback_t)
(
	hciPacketType_t packetType,
	void *pHciPacket,
	uint16_t hciPacketLength
);


/*
 * -----------------------------------------------------------------------------
 *                         GLOBAL VARIABLE DECLARATIONS
 * -----------------------------------------------------------------------------
 */

extern bool_t gEnableSingleAdvertisement;
extern bool_t gMCUSleepDuringBleEvents;

/*
 *  ----------------------------------------------------------------------------
 *                           Function Prototypes
 * -----------------------------------------------------------------------------
 */

/* Send packet interface */
extern uint32_t Hci_SendPacketToController(hciPacketType_t packetType,
					   void *pPacket,
					   uint16_t packetSize);

/*
 * Interrupt handler for BLE interrupts instantiated within the link
 * layer library.
 */
extern void Controller_InterruptHandler(void);

/*
 * Performs initialization of the Controller.
 *
 * @param[in]  callback HCI Host Receive Callback
 *
 * @return osaStatus_Success or osaStatus_Error
 */
extern osaStatus_t Controller_Init(gHostHciRecvCallback_t callback);

/*
 * Sets the TX Power on the advertising or connection channel.
 *
 * @param[in]  level    Power level (range 0-15) as defined in the table below.
 * @param[in]  channel  Advertising or connection channel.
 *
 * @return gBleSuccess_c or error.
 *
 * @remarks This function executes synchronously.
 *
 * @remarks For MKW40Z BLE controller there are 16 possible power levels
 * 0 <= N <= 15 for which the output power is distributed evenly between
 * minimum and maximum power levels.
 * For further details see the silicon datasheet.
 *
 * @remarks For MKW41Z BLE controller there are 32 possible power levels
 * 0 <= N <= 31 for which the output power is distributed evenly between
 * minimum and maximum power levels.
 * For further details see the silicon datasheet.
 */
extern bleResult_t Controller_SetTxPowerLevel(uint8_t level,
					      txChannelType_t channel);

/*
 * The Controller Task Handler
 *
 * The KW41Z Link Layer controller library expects the link layer to operate in
 * its own thread with sufficient priority to ensure that the BLE HW is properly
 * serviced.
 *
 * The controller task has its own function interface definition that doesn't
 * align with the Zephyr task function so a wrapper function is needed to call
 * the controller library tas function.
 *
 */
extern void Controller_TaskHandler(void *handler);

/*
 * -----------------------------------------------------------------------------
 *                               Debug support
 * -----------------------------------------------------------------------------
 */
#if (LOG_LEVEL == LOG_LEVEL_DBG)
/*
 * Lifted from net_private.h and slightly modified.
 * We should consider a generic logging function to do this for us
 * instead of replicating where needed.
 */
static inline void _hexdump(const u8_t *buffer, size_t length)
{
	char output[sizeof("xxxxyyyy xxxxyyyy")];
	int n = 0, k = 0;
	u8_t byte;

	while (length--) {
		if (n % 16 == 0) {
			printk(" %08X ", n);
		}

		byte = *buffer++;

		printk("%02X ", byte);

		if (byte < 0x20 || byte > 0x7f) {
			output[k++] = '.';
		} else {
			output[k++] = byte;
		}

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				output[k] = '\0';
				printk(" [%s]\n", output);
				k = 0;
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		int i;

		output[k] = '\0';

		for (i = 0; i < (16 - (n % 16)); i++) {
			printk("   ");
		}

		if ((n % 16) < 8) {
			printk(" "); /* one extra delimiter after 8 chars */
		}

		printk(" [%s]\n", output);
	}
}
#else
#define _hexdump(_x1, _x2)
#endif


#endif  /* _NXP_PRIVATE_H_ */
