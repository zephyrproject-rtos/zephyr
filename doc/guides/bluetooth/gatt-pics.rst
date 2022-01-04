.. _gatt-pics:

GATT ICS
********

PTS version: 8.0.3

M - mandatory

O - optional


Generic Attribute Profile Support
=================================

============== ======== =============================================
Parameter Name Selected Description
============== ======== =============================================
TSPC_GATT_1_1  True     Generic Attribute Profile (GATT) Client (C.1)
TSPC_GATT_1_2  True     Generic Attribute Profile (GATT) Server (C.2)
============== ======== =============================================

GATT role configuration
=======================

============== ======== =============================
Parameter Name Selected Description
============== ======== =============================
TSPC_GATT_1a_1 True     GATT Client over LE (C.1)
TSPC_GATT_1a_2 False    GATT Client over BR/EDR (C.2)
TSPC_GATT_1a_3 True     GATT Server over LE (C.3)
TSPC_GATT_1a_4 False    GATT Server over BR/EDR (C.4)
============== ======== =============================

Attribute Protocol Transport
============================

============== ======== =======================================================================================
Parameter Name Selected Description
============== ======== =======================================================================================
TSPC_GATT_2_1  False    Attribute Protocol Supported over BR/EDR (L2CAP fixed channel support) (C.1)
TSPC_GATT_2_2  True     Attribute Protocol Supported over LE (C.2)
TSPC_GATT_2_3  True     Enhanced ATT bearer Attribute Protocol Supported (L2CAP fixed EATT PSM supported) (C.3)
TSPC_GATT_2_3a True     Enhanced ATT bearer supported over LE (C.4)
TSPC_GATT_2_3b False    Enhanced ATT bearer supported over BR/EDR (C.5)
============== ======== =======================================================================================

Generic Attribute Profile Feature Support, by Client
====================================================

============== ======== =========================================================
Parameter Name Selected Description
============== ======== =========================================================
TSPC_GATT_3_1  True     Exchange MTU (C.11)
TSPC_GATT_3_2  True     Discover All Primary Services (O)
TSPC_GATT_3_3  True     Discover Primary Services by Service UUID (O)
TSPC_GATT_3_4  True     Find Included Services (O)
TSPC_GATT_3_5  True     Discover All characteristics of a Service (O)
TSPC_GATT_3_6  True     Discover Characteristics by UUID (O)
TSPC_GATT_3_7  True     Discover All Characteristic Descriptors (O)
TSPC_GATT_3_8  True     Read Characteristic Value (O)
TSPC_GATT_3_9  True     Read Using Characteristic UUID (O)
TSPC_GATT_3_10 True     Read Long Characteristic Values (O)
TSPC_GATT_3_11 True     Read Multiple Characteristic Values (O)
TSPC_GATT_3_12 True     Write without Response (O)
TSPC_GATT_3_13 True     Signed Write Without Response (C.11)
TSPC_GATT_3_14 True     Write Characteristic Value (O)
TSPC_GATT_3_15 True     Write Long Characteristic Values (O)
TSPC_GATT_3_16 True     Characteristic Value Reliable Writes (O)
TSPC_GATT_3_17 True     Notifications (C.7)
TSPC_GATT_3_18 True     Indications (M)
TSPC_GATT_3_19 True     Read Characteristic Descriptors (O)
TSPC_GATT_3_20 True     Read Long Characteristic Descriptors (O)
TSPC_GATT_3_21 True     Write Characteristic Descriptors (O)
TSPC_GATT_3_22 True     Write Long Characteristic Descriptors (O)
TSPC_GATT_3_23 True     Service Changed Characteristic (M)
TSPC_GATT_3_24 False    Configured Broadcast (C.2)
TSPC_GATT_3_25 True     Client Supported Features Characteristic (C.4)
TSPC_GATT_3_26 True     Database Hash Characteristic (C.4)
TSPC_GATT_3_27 False    Read and Interpret Characteristic Presentation Format (O)
TSPC_GATT_3_28 False    Read and Interpret Characteristic Aggregate Format (C.6)
TSPC_GATT_3_29 False    Read Multiple Variable Length Characteristic Values (C.9)
TSPC_GATT_3_30 False    Multiple Variable Length Notifications (C.10)
============== ======== =========================================================

Generic Attribute Profile Feature Support, by Server
====================================================

============== ======== =============================================================================
Parameter Name Selected Description
============== ======== =============================================================================
TSPC_GATT_4_1  True     Exchange MTU (C.6)
TSPC_GATT_4_2  True     Discover All Primary Services (M)
TSPC_GATT_4_3  True     Discover Primary Services by Service UUID (M)
TSPC_GATT_4_4  True     Find Included Services (M)
TSPC_GATT_4_5  True     Discover All characteristics of a Service (M)
TSPC_GATT_4_6  True     Discover Characteristics by UUID (M)
TSPC_GATT_4_7  True     Discover All Characteristic Descriptors (M)
TSPC_GATT_4_8  True     Read Characteristic Value (M)
TSPC_GATT_4_9  True     Read Using Characteristic UUID (M)
TSPC_GATT_4_10 True     Read Long Characteristic Values (C.12)
TSPC_GATT_4_11 True     Read Multiple Characteristic Values (O)
TSPC_GATT_4_12 True     Write without Response (C.2)
TSPC_GATT_4_13 True     Signed Write Without Response (C.6)
TSPC_GATT_4_14 True     Write Characteristic Value (C.3)
TSPC_GATT_4_15 True     Write Long Characteristic Values (C.12)
TSPC_GATT_4_16 True     Characteristic Value ReliableWrites (O)
TSPC_GATT_4_17 True     Notifications (O)
TSPC_GATT_4_18 True     Indications (C.1)
TSPC_GATT_4_19 True     Read Characteristic Descriptors (C.12)
TSPC_GATT_4_20 True     Read Long Characteristic Descriptors (C.12)
TSPC_GATT_4_21 True     Write Characteristic Descriptors (C.12)
TSPC_GATT_4_22 True     Write Long Characteristic Descriptors (O)
TSPC_GATT_4_23 True     Service Changed Characteristic (C.1)
TSPC_GATT_4_24 False    Configured Broadcast (C.5)
TSPC_GATT_4_25 False    Execute Write Request with empty queue (C.7)
TSPC_GATT_4_26 True     Client Supported Features Characteristic (C.9)
TSPC_GATT_4_27 True     Database Hash Characteristic (C.8)
TSPC_GATT_4_28 False    Report Characteristic Value: Characteristic Presentation Format (O)
TSPC_GATT_4_29 False    Report aggregate Characteristic Value: Characteristic Aggregate Format (C.10)
TSPC_GATT_4_30 False    Read Multiple Variable Length Characteristic Values (C.13)
TSPC_GATT_4_31 False    Multiple Variable Length Notifications (C.13)
============== ======== =============================================================================

SDP Interoperability
====================

============== ======== =============================================================
Parameter Name Selected Description
============== ======== =============================================================
TSPC_GATT_6_2  False    Discover GATT Services using Service Discovery Profile (C.1)
TSPC_GATT_6_3  False    Publish SDP record for GATT services support via BR/EDR (C.2)
============== ======== =============================================================

Attribute Protocol Transport Security
=====================================

============== ======== ===========================================
Parameter Name Selected Description
============== ======== ===========================================
TSPC_GATT_7_1  False    Security Mode 4 (C.1)
TSPC_GATT_7_2  True     LE Security Mode 1 (C.5)
TSPC_GATT_7_3  True     LE Security Mode 2 (C.6)
TSPC_GATT_7_4  True     LE Authentication Procedure (C.4)
TSPC_GATT_7_5  True     LE connection data signing procedure (C.2)
TSPC_GATT_7_6  True     LE Authenticate signed data procedure (C.2)
TSPC_GATT_7_7  True     LE Authorization Procedure (C.3)
============== ======== ===========================================

Multiple Simultaneous ATT Bearers
=================================

============== ======== ==================================================================================================================
Parameter Name Selected Description
============== ======== ==================================================================================================================
TSPC_GATT_8_1  False    Support for multiple simultaneous active ATT bearers from same device – ATT over LE and ATT over BR/EDR (C.1)
TSPC_GATT_8_2  True     Support for multiple simultaneous active ATT bearers from same device – ATT over LE and EATT over LE (C.2)
TSPC_GATT_8_3  False    Support for multiple simultaneous active ATT bearers from same device – ATT over BR/EDR and EATT over BR/EDR (C.3)
TSPC_GATT_8_4  False    Support for multiple simultaneous active ATT bearers from same device – ATT over LE and EATT over BR/EDR (C.4)
TSPC_GATT_8_5  False    Support for multiple simultaneous active ATT bearers from same device – ATT over BR/EDR and EATT over LE (C.5)
TSPC_GATT_8_6  False    Support for multiple simultaneous active EATT bearers from same device – EATT over BR/EDR and EATT over LE (C.6)
TSPC_GATT_8_7  False    Support for multiple simultaneous active EATT bearers from same device – EATT over BR/EDR (C.7)
TSPC_GATT_8_8  True     Support for multiple simultaneous active EATT bearers from same device – EATT over LE (C.7)
============== ======== ==================================================================================================================
