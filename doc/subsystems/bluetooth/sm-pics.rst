SM PICS
#######

PTS version: 7.0.1

\* - different than PTS defaults

\^ - field not available on PTS

M - mandatory

O - optional


Connection Roles
****************

===============	===========	=======================================
Parameter Name	Selected	Description
===============	===========	=======================================
TSPC_SM_1_1	True		Master Role (Initiator) (C.1)
TSPC_SM_1_2	True		Slave Role (Responder) (C.2)
===============	===========	=======================================


Security Properties
*******************

===============	===========	=======================================
Parameter Name	Selected	Description
===============	===========	=======================================
TSPC_SM_2_1	True		Authenticated MITM protection (O)
TSPC_SM_2_2	True		Unauthenticated no MITM protection (C.1)
TSPC_SM_2_3	True		No security requirements (M)
TSPC_SM_2_4	False		OOB supported (O)
TSPC_SM_2_5	True		LE Secure Connections (C.2)
===============	===========	=======================================


Encryption Key Size
*******************

===============	===========	=======================================
Parameter Name	Selected	Description
===============	===========	=======================================
TSPC_SM_3_1	True		Encryption Key Size Negotiation (M)
===============	===========	=======================================


Pairing Method
**************

===============	===========	=======================================
Parameter Name	Selected	Description
===============	===========	=======================================
TSPC_SM_4_1	True		Just Works (O)
TSPC_SM_4_2	True		Passkey Entry (C.1)
TSPC_SM_4_3	False (*)	Out of Band (C.1)
===============	===========	=======================================


Security Initiation
*******************

===============	===========	=======================================
Parameter Name	Selected	Description
===============	===========	=======================================
TSPC_SM_5_1	True		Encryption Setup using STK (C.3)
TSPC_SM_5_2	True		Encryption Setup using LTK (O)
TSPC_SM_5_3	True		Slave Initiated Security (C.1)
TSPC_SM_5_4	True		Slave Initiated Security - Master response(C.2)
===============	===========	=======================================


Signing Algorithm
*****************

===============	===========	=======================================
Parameter Name	Selected	Description
===============	===========	=======================================
TSPC_SM_6_1	True		Signing Algorithm - Generation (O)
TSPC_SM_6_2	True		Signing Algorithm - Resolving (O)
===============	===========	=======================================


Key Distribution
****************

===============	===========	=======================================
Parameter Name	Selected	Description
===============	===========	=======================================
TSPC_SM_7_1	True		Encryption Key (C.1)
TSPC_SM_7_2	True		Identity Key (C.2)
TSPC_SM_7_3	True		Signing Key (C.3)
===============	===========	=======================================
