UART receiver error handling test
=================================

Overview
--------

This test checks that a UART receiver (dut) detects line errors and keeps
working after corrupted traffic. A second UART (dut_aux) sends valid frames
and, in dedicated steps, injects parity or stop-bit errors.

Two ztest suites share helpers from uart_errors_common.c:

  - uart_errors_parity   (test_parity.c)
  - uart_errors_stop_bit (test_stop_bit.c, requires fake_tx in devicetree)

Twister runs the same cases in two driver modes:

  - drivers.uart.uart_errors.int_driven  (interrupt-driven API)
  - drivers.uart.uart_errors.async       (async API)

Hardware
--------

Minimum wiring:

  dut RX  <--  dut_aux TX

For hardware flow control tests, also connect RTS/CTS between the instances.

Board overlays may define optional GPIO nodes used by stop-bit injection:

  fake_tx  - GPIO bit-banging on the dut_aux TX line
  fake_cts - GPIO input on dut_aux CTS (wired to dut RTS) for *_hwfc cases

Without fake_tx, stop-bit cases are not compiled in and the stop-bit suite
registers with no runnable tests on that platform.

Test flow (both suites)
-----------------------

Each case follows the same three-phase pattern:

  1. Start the dut receiver with a known UART configuration.
  2. Send a good buffer from dut_aux and verify dut received it.
  3. Send a buffer with an error injected at byte 0 or byte 5.
  4. Assert the expected error counter increased (parity or framing).
  5. Send another good buffer and verify the receiver recovered.

Parity suite (uart_errors_parity)
---------------------------------

Receiver configuration: parity chosen by the test case, always 1 stop bit.

Error injection: dut_aux is temporarily reconfigured to a wrong parity while
transmitting one byte. dut keeps its receiver settings unchanged.

Cases cover:

  - even / odd / none parity mismatch on the transmitter
  - error in the first byte or in the middle of a 10-byte buffer
  - with and without RTS/CTS (hwfc suffix)

Expected result: rx_parity_err_cnt increases; subsequent good traffic is
received correctly.

Stop-bit suite (uart_errors_stop_bit)
-------------------------------------

Receiver configuration: no parity, 1 or 2 stop bits depending on the case.

Error injection: dut_aux TX is released to GPIO. One frame is bit-banged with
a bad stop-bit level, then normal UART TX continues for the rest of the buffer.
dut_aux is suspended/resumed through PM around GPIO takeover.

Case names encode the pattern:

  1_0   - one stop bit, stop bit driven low (framing error)
  2_00  - two stop bits, both driven low
  2_01  - two stop bits, first low second high
  2_10  - two stop bits, first high second low

Each pattern is tested at byte 0 and byte 5, with and without hwfc.

Skip rules:

  - no fake_tx node or GPIO not ready  -> skipsubsequent
  - hwfc case without fake_cts         -> skip
  - two stop bits not supported by dut -> skip

Expected result: rx_framing_err_cnt increases; subsequent good traffic is
received correctly.


Driver mode differences
-----------------------

int_driven:
  dut uses IRQ callbacks; dut_aux TX uses fifo_fill or a dedicated TX callback.

async:
  dut uses uart_rx_enable() with small RX chunks; errors come from
  UART_RX_STOPPED. RX is re-enabled on UART_RX_DISABLED while the test runs.
