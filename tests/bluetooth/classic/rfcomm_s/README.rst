# RFCOMM Server Role Test Specification

## Overview

This document defines the test cases for validating RFCOMM server role implementation in Bluetooth devices. The test cases cover various aspects of RFCOMM functionality including session management, DLC establishment, information transfer with different flow control mechanisms, and command handling.

## Table of Contents

- [Case.1: RFCOMM Server - Reject RFCOMM Session](#rfcomms1)
- [Case.2: RFCOMM Server - Reject DLC Establishment Due to Missing PN Command](#rfcomms2)
- [Case.3: RFCOMM Server - Respond to Control Commands After DLC Establishment](#rfcomms3)
- [Case.4: RFCOMM Server - Information Transfer with No Flow Control(not implemented)](#rfcomms4)
- [Case.5: RFCOMM Server - Information Transfer with Aggregate Flow Control(not implemented)](#rfcomms5)
- [Case.6: RFCOMM Server - Information Transfer with Credit Based Flow Control and Active Disconnection](#rfcomms6)
- [Case.7: RFCOMM Server - Information Transfer with Credit Based Flow Control and Passive Disconnection](#rfcomms7)

---

<a id='rfcomms1'></a>
## Case.1: RFCOMM Server - Reject RFCOMM Session

**Case ID:** RFCOMM-S-1

### Purpose

Verify that the RFCOMM server can properly reject an incoming RFCOMM session

### Covered Features

- Initialize RFCOMM Session
- Respond to Initialization of an RFCOMM Session

### Test Conditions

- DUT is configured as an RFCOMM server
- Tester has connectivity with DUT

### Test Procedure

1. Initialize DUT in RFCOMM server mode
2. Create RFCOMM server on DUT
3. Disconnect BR connection
4. Tester initiates an RFCOMM session connection to DUT
5. DUT rejects the incoming RFCOMM session

### Expected Outcomes

1. DUT successfully creates RFCOMM server
2. DUT properly rejects the incoming RFCOMM session when BR connection is disconnected
3. DUT returns to idle state

---

<a id='rfcomms2'></a>
## Case.2: RFCOMM Server - Reject DLC Establishment Due to Missing PN Command

**Case ID:** RFCOMM-S-2

### Purpose

Verify that the RFCOMM server properly rejects DLC establishment when no PN command is received

### Covered Features

- Initialize RFCOMM Session
- Respond to Initialization of an RFCOMM Session
- Respond to Establishment of a DLC

### Test Conditions

- DUT is configured as an RFCOMM server
- Tester has connectivity with DUT

### Test Procedure

1. Initialize DUT in RFCOMM server mode
2. Create RFCOMM server on DUT
3. Tester initiates an RFCOMM session connection to DUT without sending PN command
4. DUT accepts the RFCOMM session
5. Tester fails to send a valid PN command within timeout period
6. DUT rejects the DLC establishment

### Expected Outcomes

1. DUT successfully creates RFCOMM server
2. DUT accepts the RFCOMM session
3. DUT properly rejects DLC establishment due to missing PN command
4. DUT terminates the RFCOMM session

---

<a id='rfcomms3'></a>
## Case.3: RFCOMM Server - Respond to Control Commands After DLC Establishment

**Case ID:** RFCOMM-S-3

### Purpose

Verify that the RFCOMM server can properly respond to control commands (RLS, RPN, Test) and send NSC responses after DLC establishment

### Covered Features

- Initialize RFCOMM Session
- Respond to Initialization of an RFCOMM Session
- Respond to PN Command
- Send PN Command
- Respond to Establishment of a DLC
- Respond to RLS Command
- Respond to RPN Command
- Respond to Test Command
- Send Non-Supported Command (NSC) response
- Respond to Disconnection of a DLC

### Test Conditions

- DUT is configured as an RFCOMM server
- Tester has connectivity with DUT

### Test Procedure

1. Initialize DUT in RFCOMM server mode
2. Create RFCOMM server on DUT
3. Tester initiates an RFCOMM session connection to DUT
4. DUT accepts the RFCOMM session
5. Tester sends PN command to DUT
6. DUT receives PN command
7. DUT sends PN response
8. Tester initiates DLC establishment
9. DUT accepts DLC establishment
10. Tester sends RLS command to DUT
11. DUT responds to RLS command
12. Tester sends RPN command to DUT
13. DUT responds to RPN command
14. Tester sends Test command to DUT
15. DUT responds to Test command
16. Tester sends unsupported command to DUT
17. DUT sends NSC response
18. Tester initiates DLC disconnection
19. DUT responds to DLC disconnection

### Expected Outcomes

1. DUT successfully creates RFCOMM server
2. DUT accepts the RFCOMM session
3. DUT properly processes and responds to PN command
4. DUT accepts DLC establishment
5. DUT properly responds to RLS command
6. DUT properly responds to RPN command
7. DUT properly responds to Test command
8. DUT sends proper NSC response for unsupported command
9. DUT correctly handles DLC disconnection

---

<a id='rfcomms4'></a>
## Case.4: RFCOMM Server - Information Transfer with No Flow Control

**Case ID:** RFCOMM-S-4

### Purpose

Verify that the RFCOMM server can properly handle information transfer with no flow control

### Covered Features

- Initialize RFCOMM Session
- Respond to Initialization of an RFCOMM Session
- Respond to PN Command
- Send PN Command
- Respond to Establishment of a DLC
- Initiate Transfer Information
- Respond to Disconnection of a DLC
- Respond to a Shutdown of an RFCOMM Session

### Test Conditions

- DUT is configured as an RFCOMM server
- Tester has connectivity with DUT
- Flow control is disabled

### Test Procedure

1. Initialize DUT in RFCOMM server mode
2. Create RFCOMM server on DUT
3. Tester initiates an RFCOMM session connection to DUT
4. DUT accepts the RFCOMM session
5. Tester sends PN command to DUT with flow control disabled
6. DUT receives PN command
7. DUT sends PN response
8. Tester initiates DLC establishment
9. DUT accepts DLC establishment
10. Tester transfers data to DUT with no flow control
11. DUT receives data
12. DUT transfers data to tester with no flow control
13. Tester initiates DLC disconnection
14. DUT responds to DLC disconnection
15. Tester initiates RFCOMM session shutdown
16. DUT responds to RFCOMM session shutdown

### Expected Outcomes

1. DUT successfully creates RFCOMM server
2. DUT accepts the RFCOMM session
3. DUT properly processes and responds to PN command
4. DUT accepts DLC establishment
5. DUT successfully receives data with no flow control
6. DUT successfully transfers data with no flow control
7. DUT correctly handles DLC disconnection
8. DUT correctly handles RFCOMM session shutdown

---

<a id='rfcomms5'></a>
## Case.5: RFCOMM Server - Information Transfer with Aggregate Flow Control

**Case ID:** RFCOMM-S-5

### Purpose

Verify that the RFCOMM server can properly handle information transfer with aggregate flow control

### Covered Features

- Initialize RFCOMM Session
- Respond to Initialization of an RFCOMM Session
- Respond to PN Command
- Send PN Command
- Respond to Establishment of a DLC
- Initiate Transfer Information
- React to Aggregate Flow Control
- Respond to and send MSC Command
- Respond to Disconnection of a DLC
- Respond to a Shutdown of an RFCOMM Session

### Test Conditions

- DUT is configured as an RFCOMM server
- Tester has connectivity with DUT
- Aggregate flow control is enabled

### Test Procedure

1. Initialize DUT in RFCOMM server mode
2. Create RFCOMM server on DUT
3. Tester initiates an RFCOMM session connection to DUT
4. DUT accepts the RFCOMM session
5. Tester sends PN command to DUT with aggregate flow control enabled
6. DUT receives PN command
7. DUT sends PN response
8. Tester initiates DLC establishment
9. DUT accepts DLC establishment
10. Tester transfers data to DUT with aggregate flow control
11. DUT receives data
12. DUT sends flow control signals as needed
13. DUT transfers data to tester with aggregate flow control
14. Tester initiates DLC disconnection
15. DUT responds to DLC disconnection
16. Tester initiates RFCOMM session shutdown
17. DUT responds to RFCOMM session shutdown

### Expected Outcomes

1. DUT successfully creates RFCOMM server
2. DUT accepts the RFCOMM session
3. DUT properly processes and responds to PN command
4. DUT accepts DLC establishment
5. DUT successfully receives data with aggregate flow control
6. DUT properly issues flow control signals when buffer is full
7. DUT successfully transfers data with aggregate flow control
8. DUT correctly handles DLC disconnection
9. DUT correctly handles RFCOMM session shutdown

---

<a id='rfcomms6'></a>
## Case.6: RFCOMM Server - Information Transfer with Credit Based Flow Control and Active Disconnection

**Case ID:** RFCOMM-S-6

### Purpose

Verify that the RFCOMM server can properly handle information transfer with credit based flow control and actively disconnect DLC

### Covered Features

- Initialize RFCOMM Session
- Respond to Initialization of an RFCOMM Session
- Respond to PN Command
- Send PN Command
- Respond to Establishment of a DLC
- Initiate Transfer Information
- Support of Credit Based Flow Control
- Disconnect DLC
- Shutdown RFCOMM Session

### Test Conditions

- DUT is configured as an RFCOMM server
- Tester has connectivity with DUT
- Credit based flow control is enabled

### Test Procedure

1. Initialize DUT in RFCOMM server mode
2. Create RFCOMM server on DUT
3. Tester initiates an RFCOMM session connection to DUT
4. DUT accepts the RFCOMM session
5. Tester sends PN command to DUT with credit based flow control enabled
6. DUT receives PN command
7. DUT sends PN response
8. Tester initiates DLC establishment
9. DUT accepts DLC establishment
10. Tester transfers data to DUT with credit based flow control
11. DUT receives data and updates credits
12. DUT transfers data to tester with credit based flow control
13. DUT initiates DLC disconnection
14. Tester responds to DLC disconnection
15. DUT initiates RFCOMM session shutdown
16. Tester responds to RFCOMM session shutdown

### Expected Outcomes

1. DUT successfully creates RFCOMM server
2. DUT accepts the RFCOMM session
3. DUT properly processes and responds to PN command
4. DUT accepts DLC establishment
5. DUT successfully receives data with credit based flow control
6. DUT properly manages and updates credits during data exchange
7. DUT successfully transfers data with credit based flow control
8. DUT correctly initiates and completes DLC disconnection
9. DUT correctly initiates and completes RFCOMM session shutdown

---

<a id='rfcomms7'></a>
## Case.7: RFCOMM Server - Information Transfer with Credit Based Flow Control and Passive Disconnection

**Case ID:** RFCOMM-S-7

### Purpose

Verify that the RFCOMM server can properly handle information transfer with credit based flow control and respond to DLC disconnection

### Covered Features

- Initialize RFCOMM Session
- Respond to Initialization of an RFCOMM Session
- Respond to PN Command
- Send PN Command
- Respond to Establishment of a DLC
- Initiate Transfer Information
- Support of Credit Based Flow Control
- Respond to Disconnection of a DLC
- Respond to a Shutdown of an RFCOMM Session

### Test Conditions

- DUT is configured as an RFCOMM server
- Tester has connectivity with DUT
- Credit based flow control is enabled

### Test Procedure

1. Initialize DUT in RFCOMM server mode
2. Create RFCOMM server on DUT
3. Tester initiates an RFCOMM session connection to DUT
4. DUT accepts the RFCOMM session
5. Tester sends PN command to DUT with credit based flow control enabled
6. DUT receives PN command
7. DUT sends PN response
8. Tester initiates DLC establishment
9. DUT accepts DLC establishment
10. Tester transfers data to DUT with credit based flow control
11. DUT receives data and updates credits
12. DUT transfers data to tester with credit based flow control
13. Tester initiates DLC disconnection
14. DUT responds to DLC disconnection
15. Tester initiates RFCOMM session shutdown
16. DUT responds to RFCOMM session shutdown

### Expected Outcomes

1. DUT successfully creates RFCOMM server
2. DUT accepts the RFCOMM session
3. DUT properly processes and responds to PN command
4. DUT accepts DLC establishment
5. DUT successfully receives data with credit based flow control
6. DUT properly manages and updates credits during data exchange
7. DUT successfully transfers data with credit based flow control
8. DUT correctly responds to DLC disconnection
9. DUT correctly responds to RFCOMM session shutdown

---
