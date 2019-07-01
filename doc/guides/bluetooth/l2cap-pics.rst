.. _l2cap-pics:

L2CAP ICS
*********

PTS version: 7.4.1

* - different than PTS defaults

L2CAP Transport Configuration
=============================

================	===========	=======================================
Parameter Name		Selected	Description
================	===========	=======================================
TSPC_L2CAP_0_1		False (*)	BR/EDR
TSPC_L2CAP_0_2		True		Bluetooth low energy only
TSPC_L2CAP_0_3		False (*)	BR/EDR/Bluetooth low energy
================	===========	=======================================


Roles
=====

================	===========	=======================================
Parameter Name		Selected	Description
================	===========	=======================================
TSPC_L2CAP_1_1		False (*)	Data Channel Initiator
TSPC_L2CAP_1_2		False (*)	Data Channel Acceptor
TSPC_L2CAP_1_3		True		LE Master
TSPC_L2CAP_1_4		True		LE Slave
TSPC_L2CAP_1_5		True		LE Data Channel Initiator
TSPC_L2CAP_1_6		True		LE Data Channel Acceptor
================	===========	=======================================


General Operation
=================

================	===========	=======================================
Parameter Name		Selected	Description
================	===========	=======================================
TSPC_L2CAP_2_1		False (*)	Support of L2CAP signaling channel
TSPC_L2CAP_2_2		False (*)	Support of configuration process
TSPC_L2CAP_2_3		False (*)	Support of connection oriented data
					channel
TSPC_L2CAP_2_4		False (*)	Support of command echo request
TSPC_L2CAP_2_5		False (*)	Support of command echo response
TSPC_L2CAP_2_6		False (*)	Support of command information request
TSPC_L2CAP_2_7		False (*)	Support of command information response
TSPC_L2CAP_2_8		False (*)	Support of a channel group
TSPC_L2CAP_2_9		False (*)	Support of packet for connectionless
					channel
TSPC_L2CAP_2_10		False (*)	Support retransmission mode
TSPC_L2CAP_2_11		False (*)	Support flow control mode
TSPC_L2CAP_2_12		False (*)	Enhanced Retransmission Mode
TSPC_L2CAP_2_13		False (*)	Streaming Mode
TSPC_L2CAP_2_14		False (*)	FCS Option
TSPC_L2CAP_2_15		False (*)	Generate Local Busy Condition
TSPC_L2CAP_2_16		False (*)	Send Reject
TSPC_L2CAP_2_17		False (*)	Send Selective Reject
TSPC_L2CAP_2_18		False (*)	Mandatory use of ERTM
TSPC_L2CAP_2_19		False (*)	Mandatory use of Streaming Mode
TSPC_L2CAP_2_20		False (*)	Optional use of ERTM
TSPC_L2CAP_2_21		False (*)	Optional use of Streaming Mode
TSPC_L2CAP_2_22		False (*)	Send data using SAR in ERTM
TSPC_L2CAP_2_23		False (*)	Send data using SAR in Streaming Mode
TSPC_L2CAP_2_24		False (*)	Actively request Basic Mode for a PSM
					that supports the use of ERTM or
					Streaming Mode
TSPC_L2CAP_2_25		False (*)	Supports performing L2CAP channel mode
					configuration fallback from SM to ERTM
TSPC_L2CAP_2_26		False (*)	Supports sending more than one
					unacknowledged I-Frame when operating in
					ERTM
TSPC_L2CAP_2_27		False (*)	Supports sending more than three
					unacknowledged I-Frame when operating in
					ERTM
TSPC_L2CAP_2_28		False (*)	Supports configuring the peer TxWindow
					greater than 1
TSPC_L2CAP_2_29		False (*)	AMP Support
TSPC_L2CAP_2_30		False (*)	Fixed Channel Support
TSPC_L2CAP_2_31		False (*)	AMP Manager Support
TSPC_L2CAP_2_32		False (*)	ERTM over AMP
TSPC_L2CAP_2_33		False (*)	Streaming Mode Source over AMP Support
TSPC_L2CAP_2_34		False (*)	Streaming Mode Sink over AMP Support
TSPC_L2CAP_2_35		False (*)	Unicast Connectionless Data, Reception
TSPC_L2CAP_2_36		False (*)	Ability to transmit an unencrypted
					packet over a Unicast connectionless
					L2CAP channel
TSPC_L2CAP_2_37		False (*)	Ability to transmit an encrypted packet
					over a Unicast connectionless L2CAP
					channel
TSPC_L2CAP_2_38		False (*)	Extended Flow Specification for BR/EDR
TSPC_L2CAP_2_39		False (*)	Extended Window Size
TSPC_L2CAP_2_40		True		Support of Low Energy signaling channel
TSPC_L2CAP_2_41		True		Support of command reject
TSPC_L2CAP_2_42		True		Send Connection Parameter Update Request
TSPC_L2CAP_2_43		True		Send Connection Parameter Update
					Response
TSPC_L2CAP_2_44		False (*)	Extended Flow Specification for AMP
TSPC_L2CAP_2_45		False (*)	Send disconnect request command
TSPC_L2CAP_2_45a	True		Send disconnect request command - LE
TSCP_L2CAP_2_46		True		Support LE Credit Based Flow Control
					Mode
TSCP_L2CAP_2_47		True		Support for LE Data Channel
================	===========	=======================================


Configurable Parameters
=======================

================	===========	=======================================
Parameter Name		Selected	Description
================	===========	=======================================
TSPC_L2CAP_3_1		True		Support of RTX timer
TSPC_L2CAP_3_2		False (*)	Support of ERTX timer
TSPC_L2CAP_3_3		False (*)	Support minimum MTU size 48 octets
TSPC_L2CAP_3_4		False (*)	Support MTU size larger than 48 octets
TSPC_L2CAP_3_5		False (*)	Support of flush timeout value for
					reliable channel
TSPC_L2CAP_3_6		False (*)	Support of flush timeout value for
					unreliable channel
TSPC_L2CAP_3_7		False (*)	Support of bi-directional quality of
					service (QoS) option field
TSPC_L2CAP_3_8		False (*)	Negotiate QoS service type
TSPC_L2CAP_3_9		False (*)	Negotiate and support service type
					'No traffic'
TSPC_L2CAP_3_10		False (*)	Negotiate and support service type
					'Best effort'
TSPC_L2CAP_3_11		False (*)	Negotiate and support service type
					'Guaranteed'
TSPC_L2CAP_3_12		True		Support minimum MTU size 23 octets
TSPC_L2CAP_3_13		False (*)	Negotiate and support service type
					'No traffic' for Extended Flow
					Specification
TSPC_L2CAP_3_14		False (*)	Negotiate and support service type
					'Best Effort' for Extended Flow
					Specification
TSPC_L2CAP_3_15		False (*)	Negotiate and support service type
					'Guaranteed' for Extended Flow
					Specification
TSPC_L2CAP_3_16		True		Support Multiple Simultaneous LE Data
					Channels
================	===========	=======================================

