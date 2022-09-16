.. _sm-pics:

SM ICS
******

PTS version: 8.0.3

M - mandatory

O - optional


Role
====

============== ======== =================================
Parameter Name Selected Description
============== ======== =================================
TSPC_SM_1_1    True     Central Role (Initiator) (C.1)
TSPC_SM_1_2    True     Peripheral Role (Responder) (C.2)
============== ======== =================================

Security Properties
===================

============== ======== ========================================
Parameter Name Selected Description
============== ======== ========================================
TSPC_SM_2_1    True     Authenticated MITM protection (O)
TSPC_SM_2_2    True     Unauthenticated no MITM protection (C.1)
TSPC_SM_2_3    True     No security requirements (M)
TSPC_SM_2_4    True     OOB supported (O)
TSPC_SM_2_5    True     LE Secure Connections (O)
============== ======== ========================================

Encryption Key Size
===================

============== ======== =======================
Parameter Name Selected Description
============== ======== =======================
TSPC_SM_3_1    True     Encryption Key Size (M)
============== ======== =======================

Pairing Method
==============

============== ======== ===================
Parameter Name Selected Description
============== ======== ===================
TSPC_SM_4_1    True     Just Works (O)
TSPC_SM_4_2    True     Passkey Entry (C.1)
TSPC_SM_4_3    True     Out of Band (C.1)
============== ======== ===================

Security Initiation
===================

============== ======== ======================================================
Parameter Name Selected Description
============== ======== ======================================================
TSPC_SM_5_1    True     Encryption Setup using STK (C.3)
TSPC_SM_5_2    True     Encryption Setup using LTK (O)
TSPC_SM_5_3    True     Peripheral Initiated Security (C.1)
TSPC_SM_5_4    True     Peripheral Initiated Security – Central response (C.2)
TSPC_SM_5_5    False    Link Key Conversion Function h7 (C.4)
TSPC_SM_5_6    False    Link Key Conversion Function h6 (C.5)
============== ======== ======================================================

Signing Algorithm
=================

============== ======== ==================================
Parameter Name Selected Description
============== ======== ==================================
TSPC_SM_6_1    True     Signing Algorithm - Generation (O)
TSPC_SM_6_2    True     Signing Algorithm - Resolving (O)
============== ======== ==================================

Key Distribution
================

============== ======== ====================
Parameter Name Selected Description
============== ======== ====================
TSPC_SM_7_1    True     Encryption Key (C.1)
TSPC_SM_7_2    True     Identity Key (C.2)
TSPC_SM_7_3    True     Signing Key (C.3)
============== ======== ====================

Cross-Transport Key Derivation
==============================

============== ======== ===============================================
Parameter Name Selected Description
============== ======== ===============================================
TSPC_SM_8_1    False    Cross Transport Key Derivation Supported (C.1)
TSPC_SM_8_2    False    Derivation of LE LTK from BR/EDR Link Key (C.2)
TSPC_SM_8_3    False    Derivation of BR/EDR Link Key from LE LTK (C.2)
============== ======== ===============================================
