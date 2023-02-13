.. _rfcomm-pics:

RFCOMM PICS
***********

PTS version: 6.4

* - different than PTS defaults

Protocol Version
================

===============		============	======================================
Parameter Name		Selected	Description
===============		============	======================================
TSPC_RFCOMM_0_1		False		RFCOMM 1.1 with TS 07.10
TSPC_RFCOMM_0_2		True (*)	RFCOMM 1.2 with TS 07.10
===============		============	======================================


Supported Procedures
====================

================	===========	======================================
Parameter Name		Selected	Description
================	===========	======================================
TSPC_RFCOMM_1_1		True (*)	Initialize RFCOMM Session
TSPC_RFCOMM_1_2		True (*)	Respond to Initialization of an RFCOMM
					Session
TSPC_RFCOMM_1_3		True		Shutdown RFCOMM Session
TSPC_RFCOMM_1_4		True		Respond to a Shutdown of an RFCOMM
					Session
TSPC_RFCOMM_1_5		True (*)	Establish DLC
TSPC_RFCOMM_1_6		True (*)	Respond to Establishment of a DLC
TSPC_RFCOMM_1_7		True		Disconnect DLC
TSPC_RFCOMM_1_8		True		Respond to Disconnection of a DLC
TSPC_RFCOMM_1_9		True		Respond to and send MSC Command
TSPC_RFCOMM_1_10	True		Initiate Transfer Information
TSPC_RFCOMM_1_11	True		Respond to Test Command
TSPC_RFCOMM_1_12	False		Send Test Command
TSPC_RFCOMM_1_13	True		React to Aggregate Flow Control
TSPC_RFCOMM_1_14	True		Respond to RLS Command
TSPC_RFCOMM_1_15	False		Send RLS Command
TSPC_RFCOMM_1_16	True		Respond to PN Command
TSPC_RFCOMM_1_17	True (*)	Send PN Command
TSPC_RFCOMM_1_18	True (*)	Send Non-Supported Command (NSC)
					response
TSPC_RFCOMM_1_19	True		Respond to RPN Command
TSPC_RFCOMM_1_20	False		Send RPN Command
TSPC_RFCOMM_1_21	True (*)	Closing Multiplexer by First Sending
					a DISC Command
TSPC_RFCOMM_1_22	True		Support of Credit Based Flow Control
================	===========	======================================
