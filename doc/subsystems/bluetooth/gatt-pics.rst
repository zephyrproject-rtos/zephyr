GATT PICS
#########

PTS version: 6.4

\* - different than PTS defaults

\^ - field not available on PTS

M - mandatory

O - optional


Generic Attribute Profile Role
******************************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_GATT_1_1	True		Generic Attribute Profile Client (C.1)
TSPC_GATT_1_2	True		Generic Attribute Profile Server (C.2)
TSPC_GATT_1A_1	False (*)	Complete GATT client (C.3)
TSPC_GATT_1A_2	False (*)	Complete GATT server (C.4)
==============	===========	============================================


ATT Bearer Transport
********************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_GATT_2_1	False (*)	Attribute Protocol Supported over BR/EDR
				(L2CAP fixed channel support) (C.1)
TSPC_GATT_2_2	True		Attribute Protocol Supported over LE (C.2)
==============	===========	============================================


Generic Attribute Profile Support
*********************************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_GATT_3_1	True		Client: Exchange MTU (C.1)
TSPC_GATT_3_2	False (*)	Client: Discover All Primary Services (C.1)
TSPC_GATT_3_3	True		Client: Discover Primary Services Service
				UUID (C.1)
TSPC_GATT_3_4	True		Client: Find Included Services (C.1)
TSPC_GATT_3_5	True		Client: Discover All characteristics of a
				Service (C.1)
TSPC_GATT_3_6	True		Client: Discover Characteristics by UUID (C.1)
TSPC_GATT_3_7	True		Client: Discover All Characteristic Descriptors
				(C.1)
TSPC_GATT_3_8	True		Client: Read Characteristic Value (C.1)
TSPC_GATT_3_9	False (*)	Client: Read using Characteristic UUID (C.1)
TSPC_GATT_3_10	True		Client: Read Long Characteristic Values (C.1)
TSPC_GATT_3_11	True		Client: Read Multiple Characteristic
				Values (C.1)
TSPC_GATT_3_12	True		Client: Write without Response (C.1)
TSPC_GATT_3_13	True		Client: Signed Write Without Response (C.1)
TSPC_GATT_3_14	True		Client: Write Characteristic Value (C.1)
TSPC_GATT_3_15	True		Client: Write Long Characteristic Values (C.1)
TSPC_GATT_3_16	False (*)	Client: Characteristic Value Reliable
				Writes (C.1)
TSPC_GATT_3_17	True		Client: Notifications (C.1)
TSPC_GATT_3_18	True		Client: Indications (M)
TSPC_GATT_3_19	True		Client: Read Characteristic Descriptors (C.1)
TSPC_GATT_3_20	True		Client: Read long Characteristic Descriptors
				(C.1)
TSPC_GATT_3_21	True		Client: Write Characteristic Descriptors (C.1)
TSPC_GATT_3_22	True		Client: Write Long Characteristic Descriptors
				(C.1)
TSPC_GATT_3_23	True		Client: Service Changed Characteristic (M)
==============	===========	============================================


Profile Attribute Types and Formats, by client
**********************************************

===============	===========	============================================
Parameter Name	Selected	Description
===============	===========	============================================
TSPC_GATT_3B_1	False (*)	Client: Primary Service Declaration (M)
TSPC_GATT_3B_2	False (*)	Client: Secondary Service Declaration (M)
TSPC_GATT_3B_3	False (*)	Client: Include Declaration (M)
TSPC_GATT_3B_4	False (*)	Client: Characteristic Declaration (M)
TSPC_GATT_3B_5	False (*)	Client: Characteristic Value Declaration (M)
TSPC_GATT_3B_6	False (*)	Client: Characteristic Extended Properties (M)
TSPC_GATT_3B_7	False (*)	Client: Characteristic User Description
				Descriptor (M)
TSPC_GATT_3B_8	False (*)	Client: Client Characteristic Configuration
				Descriptor (M)
TSPC_GATT_3B_9	False (*)	Client: Server Characteristic Configuration
				Descriptor (M)
TSPC_GATT_3B_10	False (*)	Client: Characteristic Format Descriptor (M)
TSPC_GATT_3B_11	False (*)	Client: Characteristic Aggregate Format
				Descriptor (M)
TSPC_GATT_3B_12	False (*)	Client: Characteristic Format: Boolean (M)
TSPC_GATT_3B_13	False (*)	Client: Characteristic Format: 2Bit (M)
TSPC_GATT_3B_14	False (*)	Client: Characteristic Format: nibble (M)
TSPC_GATT_3B_15	False (*)	Client: Characteristic Format: Uint8 (M)
TSPC_GATT_3B_16	False (*)	Client: Characteristic Format: Uint12 (M)
TSPC_GATT_3B_17	False (*)	Client: Characteristic Format: Uint16 (M)
TSPC_GATT_3B_18	False (*)	Client: Characteristic Format: Uint24 (M)
TSPC_GATT_3B_19	False (*)	Client: Characteristic Format: Uint32 (M)
TSPC_GATT_3B_20	False (*)	Client: Characteristic Format: Uint48 (M)
TSPC_GATT_3B_21	False (*)	Client: Characteristic Format: Uint64 (M)
TSPC_GATT_3B_22	False (*)	Client: Characteristic Format: Uint128 (M)
TSPC_GATT_3B_23	False (*)	Client: Characteristic Format: Sint8 (M)
TSPC_GATT_3B_24	False (*)	Client: Characteristic Format: Sint12 (M)
TSPC_GATT_3B_25	False (*)	Client: Characteristic Format: Sint16 (M)
TSPC_GATT_3B_26	False (*)	Client: Characteristic Format: Sint24 (M)
TSPC_GATT_3B_27	False (*)	Client: Characteristic Format: Sint32 (M)
TSPC_GATT_3B_28	False (*)	Client: Characteristic Format: Sint48 (M)
TSPC_GATT_3B_29	False (*)	Client: Characteristic Format: Sint64 (M)
TSPC_GATT_3B_30	False (*)	Client: Characteristic Format: Sint128 (M)
TSPC_GATT_3B_31	False (*)	Client: Characteristic Format: Float32 (M)
TSPC_GATT_3B_32	False (*)	Client: Characteristic Format: Float64 (M)
TSPC_GATT_3B_33	False (*)	Client: Characteristic Format: SFLOAT (M)
TSPC_GATT_3B_34	False (*)	Client: Characteristic Format: FLOAT (M)
TSPC_GATT_3B_35	False (*)	Client: Characteristic Format: Duint16 (M)
TSPC_GATT_3B_36	False (*)	Client: Characteristic Format: utf8s (M)
TSPC_GATT_3B_37	False (*)	Client: Characteristic Format: utf16s (M)
TSPC_GATT_3B_38	False (*)	Client: Characteristic Format: struct (M)
===============	===========	============================================


Attribute Profile Support, by Server
************************************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_GATT_4_1	True		Server: Exchange MTU (C.4)
TSPC_GATT_4_2	True		Server: Discover All Primary Services (M)
TSPC_GATT_4_3	True		Server: Discover Primary Services Service
				UUID (M)
TSPC_GATT_4_4	True		Server: Find Included Services (M)
TSPC_GATT_4_5	True		Server: Discover All characteristics of
				a Service (M)
TSPC_GATT_4_6	True		Server: Discover Characteristics by UUID (M)
TSPC_GATT_4_7	True		Server: Discover All Characteristic
				Descriptors (M)
TSPC_GATT_4_8	True		Server: Read Characteristic Value (M)
TSPC_GATT_4_9	True		Server: Read using Characteristic UUID (M)
TSPC_GATT_4_10	True		Server: Read Long Characteristic Values (C.4)
TSPC_GATT_4_11	True		Server: Read Multiple Characteristic
				Values (C.4)
TSPC_GATT_4_12	True		Server: Write without Response (C.2)
TSPC_GATT_4_13	True		Server: Signed Write Without Response (C.4)
TSPC_GATT_4_14	True		Server: Write Characteristic Value (C.3)
TSPC_GATT_4_15	True		Server: Write Long Characteristic Values (C.4)
TSPC_GATT_4_16	True		Server: Characteristic Value Reliable
				Writes (C.4)
TSPC_GATT_4_17	True		Server: Notifications (C.4)
TSPC_GATT_4_18	False (*)	Server: Indications (C.1)
TSPC_GATT_4_19	True		Server: Read Characteristic Descriptors (C.4)
TSPC_GATT_4_20	True		Server: Read long Characteristic
				Descriptors (C.4)
TSPC_GATT_4_21	True		Server: Write Characteristic Descriptors (C.4)
TSPC_GATT_4_22	True		Server: Write Long Characteristic
				Descriptors (C.4)
TSPC_GATT_4_23	False (*)	Server: Service Changed Characteristic (C.1)
==============	===========	============================================


Profile Attribute Types and Characteristic Formats
**************************************************

===============	===========	============================================
Parameter Name	Selected	Description
===============	===========	============================================
TSPC_GATT_4B_1	True		Server: Primary Service Declaration (M)
TSPC_GATT_4B_2	True		Server: Secondary Service Declaration (M)
TSPC_GATT_4B_3	True		Server: Include Declaration (M)
TSPC_GATT_4B_4	True		Server: Characteristic Declaration (M)
TSPC_GATT_4B_5	True		Server: Characteristic Value Declaration (M)
TSPC_GATT_4B_6	True		Server: Characteristic Extended Properties (M)
TSPC_GATT_4B_7	True		Server: Characteristic User Description
				Descriptor (M)
TSPC_GATT_4B_8	True		Server: Client Characteristic Configuration
				Descriptor (M)
TSPC_GATT_4B_9	True		Server: Server Characteristic Configuration
				Descriptor (M)
TSPC_GATT_4B_10	True		Server: Characteristic Format Descriptor (M)
TSPC_GATT_4B_11	True		Server: Characteristic Aggregate Format
				Descriptor (M)
TSPC_GATT_4B_12	True		Server: Characteristic Format: Boolean (M)
TSPC_GATT_4B_13	True		Server: Characteristic Format: 2Bit (M)
TSPC_GATT_4B_14	True		Server: Characteristic Format: nibble (M)
TSPC_GATT_4B_15	True		Server: Characteristic Format: Uint8 (M)
TSPC_GATT_4B_16	True		Server: Characteristic Format: Uint12 (M)
TSPC_GATT_4B_17	True		Server: Characteristic Format: Uint16 (M)
TSPC_GATT_4B_18	True		Server: Characteristic Format: Uint24 (M)
TSPC_GATT_4B_19	True		Server: Characteristic Format: Uint32 (M)
TSPC_GATT_4B_20	True		Server: Characteristic Format: Uint48 (M)
TSPC_GATT_4B_21	True		Server: Characteristic Format: Uint64 (M)
TSPC_GATT_4B_22	True		Server: Characteristic Format: Uint128 (M)
TSPC_GATT_4B_23	True		Server: Characteristic Format: Sint8 (M)
TSPC_GATT_4B_24	True		Server: Characteristic Format: Sint12 (M)
TSPC_GATT_4B_25	True		Server: Characteristic Format: Sint16 (M)
TSPC_GATT_4B_26	True		Server: Characteristic Format: Sint24 (M)
TSPC_GATT_4B_27	True		Server: Characteristic Format: Sint32 (M)
TSPC_GATT_4B_28	True		Server: Characteristic Format: Sint48 (M)
TSPC_GATT_4B_29	True		Server: Characteristic Format: Sint64 (M)
TSPC_GATT_4B_30	True		Server: Characteristic Format: Sint128 (M)
TSPC_GATT_4B_31	True		Server: Characteristic Format: Float32 (M)
TSPC_GATT_4B_32	True		Server: Characteristic Format: Float64 (M)
TSPC_GATT_4B_33	True		Server: Characteristic Format: SFLOAT (M)
TSPC_GATT_4B_34	True		Server: Characteristic Format: FLOAT (M)
TSPC_GATT_4B_35	True		Server: Characteristic Format: Duint16 (M)
TSPC_GATT_4B_36	True		Server: Characteristic Format: utf8s (M)
TSPC_GATT_4B_37	True		Server: Characteristic Format: utf16s (M)
TSPC_GATT_4B_38	True		Server: Characteristic Format: struct (M)
===============	===========	============================================


Generic Attribute Profile Service - SDP Inteoperability
*******************************************************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_GATT_6_2	False (*)	Discover GATT Services using Service Discovery
				Profile (C.1)
TSPC_GATT_6_3	False (*)	Publish SDP record for GATT services support
				via BR/EDR (C.2)
==============	===========	============================================


Attribute Protocol Transport Security
*************************************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_GATT_7_1	False (*)	Security Mode 4 (C.1)
TSPC_GATT_7_2	True		LE Security Mode 1 (C.2)
TSPC_GATT_7_3	True		LE Security Mode 2 (C.2)
TSPC_GATT_7_4	True		LE Authentication Procedure (C.2)
TSPC_GATT_7_5	False (*)	LE connection data signing procedure (C.2)
TSPC_GATT_7_6	False (*)	LE Authenticate signed data procedure (C.2)
TSPC_GATT_7_7	True		LE Authorization Procedure (C.2)
==============	===========	============================================


Attribute Protocol Client Messages
**********************************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_ATT_3_1	False (*)	Attribute Error Response (M)
TSPC_ATT_3_2	False (*)	Exchange MTU Request (O)
TSPC_ATT_3_4	False (*)	Find Information Request (O)
TSPC_ATT_3_6	False (*)	Find by Type Value Request (O)
TSPC_ATT_3_8	False (*)	Read by Type Request (O)
TSPC_ATT_3_10	False (*)	Read Request (O)
TSPC_ATT_3_12	False (*)	Read Blob Request (O)
TSPC_ATT_3_14	False (*)	Read Multiple Request (O)
TSPC_ATT_3_16	False (*)	Read by Group Type Request (O)
TSPC_ATT_3_17	False (*)	Read by Group Type Response (C.6)
TSPC_ATT_3_18	False (*)	Write Request (O)
TSPC_ATT_3_20	False (*)	Write Command (O)
TSPC_ATT_3_21	False (*)	Signed Write Command (O)
TSPC_ATT_3_22	False (*)	Prepare Write Request (O)
TSPC_ATT_3_24	False (*)	Execute Write Request (C.8)
TSPC_ATT_3_26	False (*)	Handle Value Notification (M)
TSPC_ATT_3_28	False (*)	Handle Value Confirmation (M)
==============	===========	============================================


Attribute Protocol Server Messages
**********************************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_ATT_4_1	True		Attribute Error Response (M)
TSPC_ATT_4_2	True		Exchange MTU Request (M)
TSPC_ATT_4_3	True		Exchange MTU Response (M)
TSPC_ATT_4_5	True		Find Information Response (M)
TSPC_ATT_4_7	True		Find by Type Value Response (M)
TSPC_ATT_4_8	True		Read by Type Request (M)
TSPC_ATT_4_9	False (*)	Read by Type Response (M)
TSPC_ATT_4_11	True		Read Response (M)
TSPC_ATT_4_13	False (*)	Read Blob Response (C.1)
TSPC_ATT_4_15	False (*)	Read Multiple Response (C.2)
TSPC_ATT_4_17	True		Read by Group Type Response (M)
TSPC_ATT_4_19	False (*)	Write Response (C.3)
TSPC_ATT_4_20	False (*)	Write Command (O)
TSPC_ATT_4_21	False (*)	Signed Write Command (O)
TSPC_ATT_4_23	False (*)	Prepare Write Response (C.4)
TSPC_ATT_4_25	False (*)	Execute Write Response (C.4)
TSPC_ATT_4_26	False (*)	Handle Value Notification (O)
TSPC_ATT_4_27	False (*)	Handle Value Indication (O)
==============	===========	============================================


Attribute Protocol Transport
****************************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_ATT_5_2	True		LE Security Mode 1 (C.2)
TSPC_ATT_5_4	True		LE Authentication Procedure (C.2)
TSPC_ATT_5_7	True		LE Authorization Procedure (C.2)
==============	===========	============================================


Device Configuration
********************

==============	===========	============================================
Parameter Name	Selected	Description
==============	===========	============================================
TSPC_GAP_0_2	True		LE (C.2)
==============	===========	============================================
