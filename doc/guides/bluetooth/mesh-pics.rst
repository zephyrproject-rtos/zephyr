.. _mesh-pics:

MESH ICS
********

PTS version: 7.4.1

\* - different than PTS defaults

\^ - field not available on PTS

M - mandatory

O - optional


Major Profile Version (X.Y)
===========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_0_1	True		Mesh v1.0 (M)
==============	==============	=======================================


Minor Profile Version (X.Y.Z)
=============================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_0A_1	True		Erratum 10395 (C.1)
TSPC_MESH_0A_2	True		Mesh v1.0.1 (C.2)
==============	==============	=======================================


Bluetooth Core Specification v4.0 and Later
===========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_1_1	True		Core Specification 4.0 or later version (M)
==============	==============	=======================================


Roles
=====

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_2_1	True		Node (C.1)
TSPC_MESH_2_2	False (*)	Provisioner (C.1)
==============	==============	=======================================


Node Capabilities - Bearers
===========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_3_1	True		Advertising Bearer (C.1)
TSPC_MESH_3_2	True		GATT Bearer (C.1)
==============	==============	=======================================


Provisioning
============

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_4_1	True		PB-ADV (C.1)
TSPC_MESH_4_2	True		PB-GATT (C.2)
TSPC_MESH_4_3	True		Device UUID (C.3)
TSPC_MESH_4_4	True		Sending Unprovisioned Device Beacon (C.4)
TSPC_MESH_4_5	True		Generic Provisioning Layer (C.3)
TSPC_MESH_4_6	True		Provisioning Protocol (C.3)
TSPC_MESH_4_7	False (*)	Provisioning: Public Key OOB (C.5)
TSPC_MESH_4_8	True		Provisioning: Public Key No OOB (C.5)
TSPC_MESH_4_9	True		Provisioning: Authentication Output OOB (C.6)
TSPC_MESH_4_10	False (*)	Provisioning: Authentication Input OOB (C.6)
TSPC_MESH_4_11	False (*)	Provisioning: Authentication Static OOB (C.6)
TSPC_MESH_4_12	True		Provisioning: Authentication No OOB (C.3)
==============	==============	=======================================


Node Capabilities - Network Layer
=================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_5_1	True		Transmitting and Receiving Secured Network
				Layer Messages (M)
TSPC_MESH_5_2	True		Relay Feature (C.1)
TSPC_MESH_5_3	True		Network Message Cache (C.2)
==============	==============	=======================================


Node Capabilities - Lower Transport Layer
=========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_6_1	True		Transmitting and Receiving a Lower Transport
				PDU (M)
TSPC_MESH_6_2	True		Segmentation and Reassembly Behavior (M)
TSPC_MESH_6_3	True		Friend Cache (C.1)
==============	==============	=======================================


Node Capabilities - Upper Transport Layer
=========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_7_1	True		Transmitting a Secured Access Payload (M)
TSPC_MESH_7_2	True		Receiving a Secured Upper Transport PDU (M)
TSPC_MESH_7_3	True		Friend Feature (C.1)
TSPC_MESH_7_4	True		Low Power Feature (C.1)
TSPC_MESH_7_5	True		Heartbeat (M)
==============	==============	=======================================


Node Capabilities - Access Layer
================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_8_1	True		Transmitting and Receiving an Access Layer
				Message (M)
==============	==============	=======================================


Node Capabilities - Security
============================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_9_1	True		Message Replay Protection (M)
==============	==============	=======================================


Node Capabilities - Mesh Management
===================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_10_1	True		Secure Network Beacon (M)
TSPC_MESH_10_2	True		Key Refresh Procedure (M)
TSPC_MESH_10_3	True		IV Update Procedure (M)
TSPC_MESH_10_4	True		IV Index Recovery Procedure (M)
==============	==============	=======================================


Node Capabilities - Foundation Mesh Models
==========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_11_1	True		Configuration Server Model (M)
TSPC_MESH_11_2	True		Health Server Model (M)
TSPC_MESH_11_3	False (*)	Configuration Client Model (O)
TSPC_MESH_11_4	False (*)	Health Client Model (O)
==============	==============	=======================================


Node Capabilities - Proxy
=========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_12_1	True		Proxy Server (C.1)
TSPC_MESH_12_2	True		GATT Server (C.2)
TSPC_MESH_12_3	True		Advertising with Network ID (C.2)
TSPC_MESH_12_4	True		Advertising with Node Identity (C.2)
TSPC_MESH_12_5	False (*)	Proxy Client (C.3)
TSPC_MESH_12_6	False (*)	GATT Client (C.4)
==============	==============	=======================================


Mesh GATT Services
==================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_13_1	True		Mesh Provisioning Service (C.1)
TSPC_MESH_13_2	True		Mesh Proxy Service (C.2)
==============	==============	=======================================


GATT Server Requirements
========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_14_1	True		Discover All Primary Services (M)
TSPC_MESH_14_2	True		Discover Primary Services by Service UUID (M)
TSPC_MESH_14_3	True		Write without Response (M)
TSPC_MESH_14_4	True		Notification (M)
TSPC_MESH_14_5	True		Write Characteristic Descriptors (M)
==============	==============	=======================================


GATT Client Requirements
========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_15_1	False (*)	Discover All Primary Services (C.1)
TSPC_MESH_15_2	False (*)	Discover Primary Services by Service UUID (C.1)
TSPC_MESH_15_3	False (*)	Write without Response (M)
TSPC_MESH_15_4	False (*)	Notification (M)
TSPC_MESH_15_5	False (*)	Write Characteristic Descriptors (M)
==============	==============	=======================================


GAP Requirements
================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_16_1	True		Broadcaster (C.1)
TSPC_MESH_16_2	True		Observer (C.1)
TSPC_MESH_16_3	True		Peripheral (C.2)
TSPC_MESH_16_4	True		Peripheral - Security Mode 1 (C.2)
TSPC_MESH_16_5	False (*)	Central (C.3)
TSPC_MESH_16_6	False (*)	Central - Security Mode 1 (C.3)
==============	==============	=======================================


Provisioner - Bearers
=====================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_17_1	False (*)	Advertising Bearer (C.1)
TSPC_MESH_17_2	False (*)	GATT Bearer (C.1)
==============	==============	=======================================


Provisioner - Provisioning
==========================

===============	==============	=======================================
Parameter Name	Selected	Description
===============	==============	=======================================
TSPC_MESH_18_1	False (*)	Receiving Unprovisioned Device Beacon (C.1)
TSPC_MESH_18_2	False (*)	PB-ADV (C.1)
TSPC_MESH_18_3	False (*)	Generic Provisioning Layer (M)
TSPC_MESH_18_4	False (*)	Provisioning Protocol (M)
TSPC_MESH_18_5	False (*)	PB-GATT (C.2)
TSPC_MESH_18_6	False (*)	GATT Client (C.2)
TSPC_MESH_18_7	False (*)	Provisioning: Public Key OOB (M)
TSPC_MESH_18_8	False (*)	Provisioning: Public Key No OOB (M)
TSPC_MESH_18_9	False (*)	Provisioning: Authentication Output OOB (M)
TSPC_MESH_18_10	False (*)	Provisioning: Authentication Input OOB (M)
TSPC_MESH_18_11	False (*)	Provisioning: Authentication Static or No OOB (M)
===============	==============	=======================================


Provisioner - Mesh Management
=============================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_19_1	False (*)	Receiving Secure Network Beacon (M)
==============	==============	=======================================


GATT Client Requirements
========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_20_1	False (*)	Discover All Primary Services (C.1)
TSPC_MESH_20_2	False (*)	Discover Primary Services by Service UUID (C.1)
TSPC_MESH_20_3	False (*)	Write without Response (M)
TSPC_MESH_20_4	False (*)	Notification (M)
TSPC_MESH_20_5	False (*)	Write Characteristic Descriptors (M)
==============	==============	=======================================


GAP Requirements
================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_MESH_21_1	False (*)	Broadcaster (C.1)
TSPC_MESH_21_2	False (*)	Observer (C.1)
TSPC_MESH_21_3	False (*)	Central (C.2)
TSPC_MESH_21_4	False (*)	Central - Security Mode 1 (C.2)
==============	==============	=======================================
