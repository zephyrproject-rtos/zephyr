.. _gap-pics:

GAP ICS
*******

PTS version: 7.4.1

\* - different than PTS defaults

\^ - field not available on PTS

M - mandatory

O - optional


Device Configuration
====================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_0_1	False (*)	BR/EDR (C.1)
TSPC_GAP_0_2	True		LE (C.2)
TSPC_GAP_0_3	False (*)	BR/EDR/LE (C.3)
==============	==============	=======================================


Version Configuration
=====================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_0A_4	False (*)	Core Spec version 4.2 (Core v4.2) (C.4)
TSPC_GAP_0A_5	False (*)	Core Spec version 5.0 (Core v5.0) (C.5)
TSPC_GAP_0A_6	True		Core Spec version 5.1 (Core v5.1) (C.6)
==============	==============	=======================================


Modes
=====

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_1_1	False (*)	Non-discoverable mode (C.1)
TSPC_GAP_1_2	False (*)	Limited-discoverable Mode (O)
TSPC_GAP_1_3	False (*)	General-discoverable mode (O)
TSPC_GAP_1_4	False (*)	Non-connectable mode (O)
TSPC_GAP_1_5	False (*)	Connectable mode (M)
TSPC_GAP_1_6	False (*)	Non-bondable mode (O)
TSPC_GAP_1_7	False (*)	Bondable mode (C.2)
TSPC_GAP_1_8	False (*)	Non-Synchronizable Mode (C.3)
TSPC_GAP_1_9	False (*)	Synchronizable Mode (C.4)
==============	==============	=======================================


Security Aspects
================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_2_1	False (*)	Authentication procedure (C.1)
TSPC_GAP_2_2	False (*)	Support of LMP-Authentication (M)
TSPC_GAP_2_3	False (*)	Initiate LMP-Authentication (C.5)
TSPC_GAP_2_4	False (*)	Security mode 1 (C.2)
TSPC_GAP_2_5	False (*)	Security mode 2 (O)
TSPC_GAP_2_6	False (*)	Security mode 3 (C.7)
TSPC_GAP_2_7	False (*)	Security mode 4 (C.4)
TSPC_GAP_2_7a	False (*)	Security mode 4, level 4 (C.9)
TSPC_GAP_2_8	False (*)	Support of Authenticated link key (C.6)
TSPC_GAP_2_9	False (*)	Support of Unauthenticated link key (C.6)
TSPC_GAP_2_10	False (*)	No security (C.6)
TSPC_GAP_2_11	False (*)	Secure Connections Only Mode (C.8)
==============	==============	=======================================


Idle Mode Procedures
====================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_3_1	False (*)	Initiation of general inquiry (C.1)
TSPC_GAP_3_2	False (*)	Initiation of limited inquiry (C.1)
TSPC_GAP_3_3	False (*)	Initiation of name discover (O)
TSPC_GAP_3_4	False (*)	Initiation of device discovery (O)
TSPC_GAP_3_5	False (*)	Initiation of general bonding (O)
TSPC_GAP_3_6	False (*)	Initiation of dedicated bonding (O)
==============	==============	=======================================


Establishment Procedures
========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_4_1	False (*)	Support link establishment as initiator (M)
TSPC_GAP_4_2	False (*)	Support link establishment as acceptor (M)
TSPC_GAP_4_3	False (*)	Support channel establishment as initiator (O)
TSPC_GAP_4_4	False (*)	Support channel establishment as acceptor (M)
TSPC_GAP_4_5	False (*)	Support connection establishment as
				initiator (O)
TSPC_GAP_4_6	False (*)	Support connection establishment as
				acceptor (O)
TSPC_GAP_4_7	False (*)	Support synchronization establishment
				as receiver (C.1)
==============	==============	=======================================


LE Roles
========

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_5_1	True		Broadcaster (C.1)
TSPC_GAP_5_2	True		Observer (C.1)
TSPC_GAP_5_3	True		Peripheral (C.1)
TSPC_GAP_5_4	True		Central (C.1)
==============	==============	=======================================


Broadcaster Physical Layer
==========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_6_1	True		Transmitter (M)
TSPC_GAP_6_2	True		Receiver (O)
==============	==============	=======================================


Broadcaster Link Layer States
=============================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_7_1	True		Standby (M)
TSPC_GAP_7_2	True		Advertising (M)
==============	==============	=======================================


Broadcaster Link Layer Advertising Event Types
==============================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_8_1	True		Non-Connectable Undirected Event (M)
TSPC_GAP_8_2	True		Scannable Undirected Event (O)
TSPC_GAP_8_3	False (*)	Non-Connectable and Non-Scannable
				Directed Event (C.1)
TSPC_GAP_8_4	False (*)	Scannable Directed Event (C.1)
==============	==============	=======================================


Broadcaster Link Layer Advertising Data Types
=============================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_8A_1	True		AD Type-Service UUID (O)
TSPC_GAP_8A_2	True		AD Type-Local Name (O)
TSPC_GAP_8A_3	True		AD Type-Flags (O)
TSPC_GAP_8A_4	True		AD Type-Manufacturer Specific Data (O)
TSPC_GAP_8A_5	True		AD Type-TX Power Level (O)
TSPC_GAP_8A_6	False (*)	AD Type-Security Manager Out of Band
				(OOB) (C.1)
TSPC_GAP_8A_7	True		AD Type-Security manager TK Value (O)
TSPC_GAP_8A_8	True		AD Type-Slave Connection Interval Range (O)
TSPC_GAP_8A_9	True		AD Type-Service Solicitation (O)
TSPC_GAP_8A_10	True		AD Type-Service Data (O)
TSPC_GAP_8A_11	True		AD Type-Appearance (O)
TSPC_GAP_8A_12	True		AD Type-Public Target Address (O)
TSPC_GAP_8A_13	True		AD Type-Random Target Address (O)
TSPC_GAP_8A_14	True		AD Type-Advertising Interval (O)
TSPC_GAP_8A_15	True		AD Type-LE Bluetooth Device Address (O)
TSPC_GAP_8A_16	True		AD Type-LE Role (O)
TSPC_GAP_8A_17	True		AD Type-URI (C.3)
==============	==============	=======================================


Broadcaster Connection Modes and Procedures
===========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_9_1	True		Non-Connectable Mode (M)
==============	==============	=======================================


Broadcaster Broadcasting and Observing Features
===============================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_10_1	True		Broadcast Mode (M)
==============	==============	=======================================


Broadcaster Privacy Feature
===========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_11_1	True		Privacy Feature v1.0 (O)
TSPC_GAP_11_2	True		Resolvable Private Address Generation
				Procedure (C.1)
TSPC_GAP_11_3	True		Non-Resolvable Private Address Generation
				Procedure (C.2)
==============	==============	=======================================


Periodic Advertising Modes and Procedures
=========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_11A_1	False (*)	Periodic Advertising Synchronizability
				mode (C.1)
TSPC_GAP_11A_2	False (*)	Periodic Advertising mode (C.2)
==============	==============	=======================================


Observer Physical Layer
=======================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_12_1	True		Receiver (M)
TSPC_GAP_12_2	True		Transmitter (O)
==============	==============	=======================================


Observer Link Layer States
==========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_13_1	True		Standby (M)
TSPC_GAP_13_2	True		Scanning (M)
==============	==============	=======================================


Observer Link Layer Scanning Types
==================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_14_1	True		Passive Scanning (M)
TSPC_GAP_14_2	True		Active Scanning (O)
==============	==============	=======================================


Observer Connection Modes and Procedures
========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_15_1	True		Non-Connectable Mode (M)
==============	==============	=======================================


Observer Broadcasting and Observing Features
============================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_16_1	True		Observation Procedure (M)
==============	==============	=======================================


Observer Privacy Feature
========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_17_1	True		Privacy Feature v1.0 (O)
TSPC_GAP_17_2	True		Non-Resolvable Private Address Generation
				Procedure (C.1)
TSPC_GAP_17_3	True		Resolvable Private Address Resolution
				Procedure (C.2)
TSPC_GAP_17_4	True		Resolvable Private Address Generation
				Procedure (C.3)
==============	==============	=======================================


Periodic Advertising Modes and Procedures
=========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_17A_1	False (*)	Periodic Advertising Synchronization
				Establishment procedure without listening
				for periodic advertising (C.1)
TSPC_GAP_17A_2	False (*)	Periodic Advertising Synchronization
				Establishment procedure with listening for
				periodic advertising (C.1)
==============	==============	=======================================


Peripheral Physical Layer
=========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_18_1	True		Transmitter (M)
TSPC_GAP_18_2	True		Receiver (M)
==============	==============	=======================================


Peripheral Link Layer States
============================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_19_1	True		Standby (M)
TSPC_GAP_19_2	True		Advertising (M)
TSPC_GAP_19_3	True		Connection, Slave Role (C.1)
==============	==============	=======================================


Peripheral Link Layer Advertising Event Types
=============================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_20_1	True		Connectable Undirected Event (C.1)
TSPC_GAP_20_2	True		Connectable Directed Event (C.2)
TSPC_GAP_20_3	True		Non-Connectable Undirected Event (O)
TSPC_GAP_20_4	True		Scannable Undirected Event (O)
TSPC_GAP_20_5	False (*)	Connectable Undirected Event (C.3)
TSPC_GAP_20_6	False (*)	Non-Connectable and Non-Scannable Directed Event (C.3)
TSPC_GAP_20_7	False (*)	Scannable Directed Event (C.3)
==============	==============	=======================================


Peripheral Link Layer Advertising Data Types
============================================

===============	=============	=======================================
Parameter Name   Selected	Description
===============	=============	=======================================
TSPC_GAP_20A_1	True		AD Type-Service UUID (C.1)
TSPC_GAP_20A_2	True		AD Type-Local Name (C.1)
TSPC_GAP_20A_3	True		AD Type-Flags (C.2)
TSPC_GAP_20A_4	True		AD Type-Manufacturer Specific Data (C.1)
TSPC_GAP_20A_5	True		AD Type-TX Power Level (C.1)
TSPC_GAP_20A_6	False (*)	AD Type-Security Manager Out of Band (OOB)
				(C.3)
TSPC_GAP_20A_7	True		AD Type-Security manager TK Value (C.1)
TSPC_GAP_20A_8	True		AD Type-Slave Connection Interval Range (C.1)
TSPC_GAP_20A_9	True		AD Type-Service Solicitation (C.1)
TSPC_GAP_20A_10	True		AD Type-Service Data (C.1)
TSPC_GAP_20A_11	True		AD Type-Appearance (C.1)
TSPC_GAP_20A_12	True		AD Type-Public Target Address (C.1)
TSPC_GAP_20A_13	True		AD Type-Random Target Address (C.1)
TSPC_GAP_20A_14	True		AD Type-Advertising Interval (C.1)
TSPC_GAP_20A_15	True		AD Type-LE Bluetooth Device Address (C.1)
TSPC_GAP_20A_16	True		AD Type-LE Role (C.1)
TSPC_GAP_20A_17	True		AD Type-URI (C.4)
===============	=============	=======================================


Peripheral Link Layer Control Procedures
========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_21_1	True		Connection Update Procedure (C.1)
TSPC_GAP_21_2	True		Channel Map Update Procedure (C.1)
TSPC_GAP_21_3	True		Encryption Procedure (C.2)
TSPC_GAP_21_4	True		Feature Exchange Procedure (C.1)
TSPC_GAP_21_5	True		Version Exchange Procedure (C.1)
TSPC_GAP_21_6	True		Termination Procedure (C.1)
TSPC_GAP_21_7	False (*)	LE Ping Procedure (C.3)
TSPC_GAP_21_8	True		Slave Initiated Feature Exchange Procedure
				(C.4)
TSPC_GAP_21_9	True		Connection Parameter Request Procedure (C.5)
TSPC_GAP_21_10	True		Data Length Update Procedure (C.6)
TSPC_GAP_21_11	True		PHY Update Procedure (C.7)
TSPC_GAP_21_12	False (*)	Minimum Number Of Used Channels Procedure (C.7)
==============	==============	=======================================


Peripheral Discovery Modes and Procedures
=========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_22_1	True		Non-Discoverable Mode (C.1)
TSPC_GAP_22_2	True		Limited Discoverable Mode (C.2)
TSPC_GAP_22_3	True		General Discoverable Mode (C.3)
TSPC_GAP_22_4	True		Name Discovery Procedure (C.4)
==============	==============	=======================================


Peripheral Connection Modes and Procedures
==========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_23_1	True		Non-Connectable Mode (M)
TSPC_GAP_23_2	False (*)	Directed Connectable Mode (C.1)
TSPC_GAP_23_3	True		Undirected Connectable Mode (C.2)
TSPC_GAP_23_4	True		Connection Parameter Update Procedure (C.1)
TSPC_GAP_23_5	True		Terminate Connection Procedure (C.2)
==============	==============	=======================================


Peripheral Bonding Modes and Procedures
=======================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_24_1	True		Non-Bondable Mode (M)
TSPC_GAP_24_2	True		Bondable Mode (C.1)
TSPC_GAP_24_3	True		Bonding Procedure  (C.1)
TSPC_GAP_24_4	True		Multiple Bonds (C.2)
==============	==============	=======================================


Peripheral Security Aspects Features
====================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_25_1	True		Security Mode (C.2)
TSPC_GAP_25_2	True		Security Mode 2 (C.2)
TSPC_GAP_25_3	True		Authentication Procedure (C.2)
TSPC_GAP_25_4	False (*)	Authorization Procedure (C.2)
TSPC_GAP_25_5	True		Connection Data Signing Procedure (C.2)
TSPC_GAP_25_6	True		Authenticate Signed Data Procedure (C.2)
TSPC_GAP_25_7	True		Authenticated Pairing
				(LE security mode 1 level 3) (C.1)
TSPC_GAP_25_8	True		Unauthenticated Pairing
				(LE security mode 1 level 2) (C.1)
TSPC_GAP_25_9	True		LE Security Mode 1 Level 4 (C.3)
TSPC_GAP_25_10	True		Secure Connections Only Mode  (C.4)
==============	==============	=======================================


Peripheral Privacy Feature
==========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_26_1	True		Privacy Feature (O)
TSPC_GAP_26_2	True		Non-Resolvable Private Address Generation
				Procedure (C.1)
TSPC_GAP_26_3	True		Resolvable Private Address Generation
				Procedure (C.2)
TSPC_GAP_26_4	True		Resolvable Private Address Generation
				Procedure (C.3)
==============	==============	=======================================


Peripheral GAP Characteristics
==============================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_27_1	True		Device Name (M)
TSPC_GAP_27_2	True		Appearance (M)
TSPC_GAP_27_3	False (*)	Peripheral Privacy Flag (C.1)
TSPC_GAP_27_4	False (*)	Reconnection Address (C.2)
TSPC_GAP_27_5	True		Peripheral Preferred Connection Parameters
				(C.3)
TSPC_GAP_27_6	True		Writable Device Name (C.3)
TSPC_GAP_27_7	False (*)	Writable Appearance (C.3)
TSPC_GAP_27_8	False (*)	Writable Peripheral Privacy Flag (C.4)
TSPC_GAP_27_9	True		Central Address Resolution (C.5)
==============	==============	=======================================


Periodic Advertising Modes and Procedures
=========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_27A_1	False (*)	Periodic Advertising Synchronization
				Transfer procedure (C.1)
TSPC_GAP_27A_2	False (*)	Periodic Advertising Synchronization
				Establishment procedure over an LE
				connection without listening for periodic
				advertising (C.2)
TSPC_GAP_27A_3	False (*)	Periodic Advertising Synchronization
				Establishment procedure over an LE
				connection with listening for periodic
				advertising (C.3)
==============	==============	=======================================


Central Physical Layer
======================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_28_1	True		Transmitter (M)
TSPC_GAP_28_2	True		Receiver (M)
==============	==============	=======================================


Central Link Layer States
=========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_29_1	True		Standby (M)
TSPC_GAP_29_2	True		Scanning (M)
TSPC_GAP_29_3	True		Initiating (M)
TSPC_GAP_29_4	True		Connection, Master Role (M)
==============	==============	=======================================


Central Link Layer Scanning Types
=================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_30_1	True		Passive Scanning (O)
TSPC_GAP_30_2	True		Active Scanning (C.1)
==============	==============	=======================================


Central Link Layer Control Procedures
=====================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_31_1	True		Connection Update Procedure (M)
TSPC_GAP_31_2	True		Channel Map Update Procedure (M)
TSPC_GAP_31_3	True		Encryption Procedure (O)
TSPC_GAP_31_4	True		Feature Exchange Procedure (M)
TSPC_GAP_31_5	True		Version Exchange Procedure (M)
TSPC_GAP_31_6	True		Termination Procedure (M)
TSPC_GAP_31_7	False (*)	LE Ping Procedure (C.1)
TSPC_GAP_31_8	True		Slave Initiated Feature Exchange Procedure
				(C.2)
TSPC_GAP_31_9	True		Connection Parameter Request Procedure (C.3)
TSPC_GAP_31_10	True		Data Length Update Procedure (C.4)
TSPC_GAP_31_11	True		PHY Update Procedure (C.5)
TSPC_GAP_31_12	False (*)	Minimum Number Of Used Channels Procedure (C.5)
==============	==============	=======================================


Central Discovery Modes and Procedures
======================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_32_1	True		Limited Discovery Procedure (C.2)
TSPC_GAP_32_2	True		General Discovery Procedure (C.1)
TSPC_GAP_32_3	True		Name Discovery Procedure (C.3)
==============	==============	=======================================


Central Connection Modes and Procedures
=======================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_33_1	True		Auto Connection Establishment Procedure (C.3)
TSPC_GAP_33_2	True		General Connection Establishment Procedure (C.1)
TSPC_GAP_33_3	False (*)	Selective Connection Establishment Procedure
				(C.3)
TSPC_GAP_33_4	True		Direct Connection Establishment Procedure (C.2)
TSPC_GAP_33_5	True		Connection Parameter Update Procedure (C.2)
TSPC_GAP_33_6	True		Terminate Connection Procedure (C.2)
==============	==============	=======================================


Central Bonding Modes and Procedures
====================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_34_1	True		Non-Bondable Mode (C.1)
TSPC_GAP_34_2	True		Bondable Mode (C.2)
TSPC_GAP_34_3	True		Bonding Procedure (C.2)
==============	==============	=======================================


Central Security Features
=========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_35_1	True		Security Mode 1 (O)
TSPC_GAP_35_2	True		Security Mode 2 (O)
TSPC_GAP_35_3	True		Authentication Procedure (O)
TSPC_GAP_35_4	False (*)	Authorization Procedure (O)
TSPC_GAP_35_5	True		Connection Data Signing Procedure (O)
TSPC_GAP_35_6	True		Authenticate Signed Data Procedure (O)
TSPC_GAP_35_7	True		Authenticated Pairing
				(LE security mode 1 level 3) (C.1)
TSPC_GAP_35_8	True		Unauthenticated Pairing
				(LE security mode 1 level 2) (C.1)
TSPC_GAP_35_9	True		LE Security Mode 1 Level 4 (C.2)
TSPC_GAP_35_10	True		Secure Connections Only Mode  (C.3)
==============	==============	=======================================


Central Privacy Feature
=======================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_36_1	True		Privacy Feature v1.0 (O)
TSPC_GAP_36_2	True		Non-Resolvable Private Address Generation
				Procedure (C.1)
TSPC_GAP_36_3	True		Resolvable Private Address Resolution
				Procedure (C.2)
TSPC_GAP_36_4	False (*)	Write to Privacy Characteristic
				(Enable/Disable Privacy) (C.3)
TSPC_GAP_36_5	True		Resolvable Private Address Generation
				Procedure (C.4)
==============	==============	=======================================


Central GAP Characteristics
===========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_37_1	True		Device Name (M)
TSPC_GAP_37_2	True		Appearance (M)
TSPC_GAP_37_3	True		Central Address Resolution  (C.1)
==============	==============	=======================================


Periodic Advertising Modes and Procedures
=========================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_37A_1	False (*)	Periodic Advertising Synchronization
				Transfer procedure (C.1)
TSPC_GAP_37A_2	False (*)	Periodic Advertising Synchronization
				Establishment procedure over an LE
				connection without listening for
				periodic advertising
TSPC_GAP_37A_3	False (*)	Periodic Advertising Synchronization
				Establishment procedure over an LE
				connection with listening for periodic
				advertising
==============	==============	=======================================


BR/EDR/LE Roles
===============

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_38_1	False (*)	Broadcaster (C.1)
TSPC_GAP_38_2	False (*)	Observer (C.1)
TSPC_GAP_38_3	False (*)	Peripheral (C.1)
TSPC_GAP_38_4	False (*)	Central (C.1)
==============	==============	=======================================


Central BR/EDR/LE Modes
=======================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_39_1	False (*)	Non-Discoverable Mode (C.1)
TSPC_GAP_39_2	False (*)	Discoverable Mode (C.2)
TSPC_GAP_39_3	False (*)	Non-Connectable Mode (C.3)
TSPC_GAP_39_4	False (*)	Connectable Mode (M)
TSPC_GAP_39_5	False (*)	Non-Bondable Mode (C.4)
TSPC_GAP_39_6	False (*)	Bondable Mode (C.5)
==============	==============	=======================================


Central BR/EDR/LE Idle Mode Procedures
======================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_40_1	False (*)	General Discovery (C.1)
TSPC_GAP_40_2	False (*)	Limited Discovery (C.2)
TSPC_GAP_40_3	False (*)	Device Type Discovery (C.3)
TSPC_GAP_40_4	False (*)	Name Discovery (C.4)
TSPC_GAP_40_5	False (*)	Link Establishment (C.5)
==============	==============	=======================================


Central BR/EDR/LE Security Aspects
==================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_41_1	False (*)	Security Aspects (M)
TSPC_GAP_41_2A	False (*)	Derivation of BR/EDR Link Key from LE LTK (C.1)
TSPC_GAP_41_2B	False (*)	Derivation of LE LTK from BR/EDR Link Key (C.1)
==============	==============	=======================================


Peripheral BR/EDR/LE Modes
==========================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_42_1	False (*)	Non-Discoverable Mode (C.1)
TSPC_GAP_42_2	False (*)	Discoverable Mode (C.2)
TSPC_GAP_42_3	False (*)	Non-Connectable Mode (C.3)
TSPC_GAP_42_4	False (*)	Connectable Mode (M)
TSPC_GAP_42_5	False (*)	Non-Bondable Mode (C.4)
TSPC_GAP_42_6	False (*)	Bondable Mode (C.5)
==============	==============	=======================================


Peripheral BR/EDR/LE Security Aspects
=====================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_43_1	False (*)	Peripheral BR/EDR/LE: Non-Discoverable Mode
TSPC_GAP_43_2A	False (*)	Derivation of BR/EDR Link Key from LE LTK (C.1)
TSPC_GAP_43_2B	False (*)	Derivation of LE LTK from BR/EDR Link Key (C.1)
==============	==============	=======================================


Central Simultaneous BR/EDR and LE Transports
=============================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_44_1	False (*)	Simultaneous BR/EDR and LE Transports - BR/EDR
				Slave to the same device (C.1)
TSPC_GAP_44_2	False (*)	Simultaneous BR/EDR and LE Transports - BR/EDR
				Master to the same device (C.1)
==============	==============	=======================================


Peripheral Simultaneous BR/EDR and LE Transports
================================================

==============	==============	=======================================
Parameter Name	Selected	Description
==============	==============	=======================================
TSPC_GAP_45_1	False (*)	Simultaneous BR/EDR and LE Transports - BR/EDR
				Slave to the same device (C.1)
TSPC_GAP_45_2	False (*)	Simultaneous BR/EDR and LE Transports - BR/EDR
				Master to the same device (C.1)
==============	==============	=======================================
