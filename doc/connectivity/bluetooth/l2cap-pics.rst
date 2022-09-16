.. _l2cap-pics:

L2CAP ICS
*********

PTS version: 8.0.3

M - mandatory

O - optional


L2CAP Transport Configuration
=============================

============== ======== ===================================================================================
Parameter Name Selected Description
============== ======== ===================================================================================
TSPC_L2CAP_0_1 False    BR/EDR (includes possible support of GAP LE Broadcaster or LE Observer roles) (C.1)
TSPC_L2CAP_0_2 True     LE (C.2)
TSPC_L2CAP_0_3 False    BR/EDR/LE (C.3)
============== ======== ===================================================================================

Roles
=====

============== ======== ===============================
Parameter Name Selected Description
============== ======== ===============================
TSPC_L2CAP_1_1 False    Data Channel Initiator (C.3)
TSPC_L2CAP_1_2 False    Data Channel Acceptor (C.1)
TSPC_L2CAP_1_3 True     LE Master (C.2)
TSPC_L2CAP_1_4 True     LE Slave (C.2)
TSPC_L2CAP_1_5 True     LE Data Channel Initiator (C.4)
TSPC_L2CAP_1_6 True     LE Data Channel Acceptor (C.5)
============== ======== ===============================

General Operation
=================

================ ======== ===========================================================================================
Parameter Name   Selected Description
================ ======== ===========================================================================================
TSPC_L2CAP_2_1   False    Support of L2CAP signalling channel (C.16)
TSPC_L2CAP_2_2   False    Support of configuration process (C.16)
TSPC_L2CAP_2_3   False    Support of connection oriented data channel (C.16)
TSPC_L2CAP_2_4   False    Support of command echo request (C.17)
TSPC_L2CAP_2_5   False    Support of command echo response (C.16)
TSPC_L2CAP_2_6   False    Support of command information request (C.17)
TSPC_L2CAP_2_7   False    Support of command information response (C.16)
TSPC_L2CAP_2_8   False    Support of a channel group (C.17)
TSPC_L2CAP_2_9   False    Support of packet for connectionless channel (C.17)
TSPC_L2CAP_2_10  False    Support retransmission mode (C.17)
TSPC_L2CAP_2_11  False    Support flow control mode (C.17)
TSPC_L2CAP_2_12  False    Enhanced Retransmission Mode (C.11)
TSPC_L2CAP_2_13  False    Streaming Mode (O)
TSPC_L2CAP_2_14  False    FCS Option (C.1)
TSPC_L2CAP_2_15  False    Generate Local Busy Condition (C.2)
TSPC_L2CAP_2_16  False    Send Reject (C.2)
TSPC_L2CAP_2_17  False    Send Selective Reject (C.2)
TSPC_L2CAP_2_18  False    Mandatory use of ERTM (C.3)
TSPC_L2CAP_2_19  False    Mandatory use of Streaming Mode (C.4)
TSPC_L2CAP_2_20  False    Optional use of ERTM (C.3)
TSPC_L2CAP_2_21  False    Optional use of Streaming Mode (C.4)
TSPC_L2CAP_2_22  False    Send data using SAR in ERTM (C.5)
TSPC_L2CAP_2_23  False    Send data using SAR in Streaming Mode (C.6)
TSPC_L2CAP_2_24  False    Actively request Basic Mode for a PSM that supports the use of ERTM or Streaming Mode (C.7)
TSPC_L2CAP_2_25  False    Supports performing L2CAP channel mode configuration fallback from SM to ERTM (C.8)
TSPC_L2CAP_2_26  False    Supports sending more than one unacknowledged I-Frame when operating in ERTM (C.9)
TSPC_L2CAP_2_27  False    Supports sending more than three unacknowledged I-Frame when operating in ERTM (C.9)
TSPC_L2CAP_2_28  False    Supports configuring the peer TxWindow greater than 1. (C.10)
TSPC_L2CAP_2_29  False    AMP Support (C.11)
TSPC_L2CAP_2_30  False    Fixed Channel Support (C.11)
TSPC_L2CAP_2_31  False    AMP Manager Support (C.11)
TSPC_L2CAP_2_32  False    ERTM over AMP (C.11)
TSPC_L2CAP_2_33  False    Streaming Mode Source over AMP Support (C.12)
TSPC_L2CAP_2_34  False    Streaming Mode Sink over AMP Support (C.12)
TSPC_L2CAP_2_35  False    Unicast Connectionless Data, Reception (O)
TSPC_L2CAP_2_36  False    Ability to transmit an unencrypted packet over a unicast connectionless L2CAP channel (O)
TSPC_L2CAP_2_37  False    Ability to transmit an encrypted packet over a unicast connectionless L2CAP channel. (O)
TSPC_L2CAP_2_38  False    Extended Flow Specification for BR/EDR (C.7)
TSPC_L2CAP_2_39  False    Extended Window Size (C.7)
TSPC_L2CAP_2_40  True     Support of Low Energy signaling channel (C.13)
TSPC_L2CAP_2_41  True     Support of command reject (C.13)
TSPC_L2CAP_2_42  True     Send Connection Parameter Update Request (C.14)
TSPC_L2CAP_2_43  True     Send Connection Parameter Update Response (C.15)
TSPC_L2CAP_2_44  False    Extended Flow Specification for AMP (C.18)
TSPC_L2CAP_2_45  False    Send Disconnect Request Command (C.21)
TSPC_L2CAP_2_45a True     Send Disconnect Request Command – LE (C.22)
TSPC_L2CAP_2_46  True     Support LE Credit Based Flow Control Mode (C.19)
TSPC_L2CAP_2_47  True     Support for LE Data Channel (C.20)
TSPC_L2CAP_2_48  True     Support Enhanced Credit Based Flow Control Mode (C.23)
================ ======== ===========================================================================================

Configurable Parameters
=======================

=============== ======== ======================================================================================
Parameter Name  Selected Description
=============== ======== ======================================================================================
TSPC_L2CAP_3_1  True     Support of RTX timer (M)
TSPC_L2CAP_3_2  False    Support of ERTX timer (C.4)
TSPC_L2CAP_3_3  False    Support minimum MTU size 48 octets (C.4)
TSPC_L2CAP_3_4  False    Support MTU size larger than 48 octets (C.5)
TSPC_L2CAP_3_5  False    Support of flush timeout value for reliable channel (C.4)
TSPC_L2CAP_3_6  False    Support of flush timeout value for unreliable channel (C.5)
TSPC_L2CAP_3_7  False    Support of bi-directional quality of service (QoS) option field (C.1)
TSPC_L2CAP_3_8  False    Negotiate QoS service type (C.5)
TSPC_L2CAP_3_9  False    Negotiate and support service type 'No Traffic' (C.2)
TSPC_L2CAP_3_10 False    Negotiate and support service type 'Best effort' (C.3)
TSPC_L2CAP_3_11 False    Negotiate and support service type 'Guaranteed' (C.2)
TSPC_L2CAP_3_12 True     Support minimum MTU size 23 octets (C.6)
TSPC_L2CAP_3_13 False    Negotiate and support service type ‘No traffic’ for Extended Flow Specification (C.7)
TSPC_L2CAP_3_14 False    Negotiate and support service type ‘Best Effort’ for Extended Flow Specification (C.8)
TSPC_L2CAP_3_15 False    Negotiate and support service type ‘Guaranteed’ for Extended Flow Specification. (C.9)
TSPC_L2CAP_3_16 True     Support Multiple Simultaneous LE Data Channels (C.10)
=============== ======== ======================================================================================
