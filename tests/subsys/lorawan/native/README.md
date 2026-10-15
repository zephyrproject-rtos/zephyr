<!--
SPDX-FileCopyrightText: Copyright (c) 2026 Embeint Inc
SPDX-License-Identifier: Apache-2.0
-->

# Native LoRaWAN Tests

## Overview

This test suite exercises the native LoRaWAN MAC implementation on
`native_sim`.  It uses the fake LoRa radio and a small, protocol-aware network
server fixture.  The fixture validates radio configuration and transmitted
frames, including encryption and MICs, and supplies join accepts and
downlinks in the configured Class A receive windows.

The currently defined scenario targets the EU868 regional parameters.

## Tested features

- **Stack lifecycle and API state checks**: starting and stopping the stack,
  setting the region, and errors returned when APIs are used before the stack
  is started or before a network has been joined.
- **OTAA joins**: join accepts in RX1 and RX2, join receive-delay handling,
  no-response timeouts, CFList channel addition and use, and rejection of
  malformed or unauthenticated join accepts.  A LoRaWAN 1.0.x test vector
  also verifies join-request construction and session-key derivation.
- **Class A radio operation**: transmit and receive radio settings, default
  EU868 channel selection, configured uplink data rates, and RX1/RX2 timing.
- **Uplinks**: unconfirmed and confirmed application uplinks, frame-counter
  progression, payload encryption and MIC generation, invalid message-type
  handling, and confirmed-uplink acknowledgement, retry, and timeout paths.
- **Application downlinks**: delivery through a registered port callback in
  RX1 and RX2, RSSI/SNR and the data-pending flag propagation, acknowledgement
  of confirmed downlinks on the next uplink, and rejection of malformed,
  incorrectly addressed, or unauthenticated data frames.

## Future work

The following coverage should be added as the native implementation grows:

- Regional test configurations beyond EU868, including their channel plans,
  data rates, RX parameters, and multi-word channel masks.
- ABP activation and rejoin/reset behaviour, including persistence and
  monotonic DevNonce handling where non-volatile state is supported.
- MAC-command processing and its API-visible effects, particularly LinkADR,
  RX parameter, duty-cycle, link-check, device-time, and channel-management
  commands.
- ADR behaviour and callbacks across server-requested data-rate/channel
  changes, plus payload-size limits at each data rate.
- Downlink edge cases such as port zero MAC commands, other callback flags,
  duplicate or out-of-order frame counters, and callback registration on
  multiple and wildcard ports.
- Input-validation and recovery cases: invalid join/send configuration,
  maximum payloads, radio errors, and interruption or cancellation while
  waiting for receive windows.
- LoRaWAN interoperability vectors and coverage for supported protocol
  versions beyond the existing LoRaWAN 1.0.x vector.
