.. _dis-pics:

DIS ICS
*******

PTS version: 8.0.3

M - mandatory

O - optional


Service Version
===============

============== ======== ===================================
Parameter Name Selected Description
============== ======== ===================================
TSPC_DIS_0_1   True     Device Information Service v1.1 (M)
============== ======== ===================================

Transport Requirements
======================

============== ======== ===================================
Parameter Name Selected Description
============== ======== ===================================
TSPC_DIS_1_1   False    Service supported over BR/EDR (C.1)
TSPC_DIS_1_2   True     Service supported over LE (C.1)
TSPC_DIS_1_3   False    Service supported over HS (C.1)
============== ======== ===================================

Service Requirements
====================

============== ======== ======================================================================
Parameter Name Selected Description
============== ======== ======================================================================
TSPC_DIS_2_1   True     Device Information Service (M)
TSPC_DIS_2_2   True     Manufacturer Name String Characteristic (O)
TSPC_DIS_2_3   True     Model Number String Characteristic (O)
TSPC_DIS_2_4   True     Serial Number String Characteristic (O)
TSPC_DIS_2_5   True     Hardware Revision String Characteristic (O)
TSPC_DIS_2_6   True     Firmware Revision String Characteristic (O)
TSPC_DIS_2_7   True     Software Revision String Characteristic (O)
TSPC_DIS_2_8   False    System ID Characteristic (O)
TSPC_DIS_2_9   False    IEEE 11073-20601 Regulatory Certification Data List Characteristic (O)
TSPC_DIS_2_10  False    SDP Interoperability (C.1)
TSPC_DIS_2_11  True     PnP ID (O)
============== ======== ======================================================================
