The purpose of this test is to validate that uart_poll_out call is resilient to
being interrupted by another uart API call. That includes uart_poll_out called
from higher priority context, uart_fifo_fill called from UART interrupt context
and uart_tx called from higher priority context. Preemptions shall not lead to
any bytes being dropped.

This test is establishing 3 context from which uart_poll_out is called:
- main thread
- higher priority thread
- k_timer timeout context

From each context stream of data is being sent. Bytes in streams are encoded as
following: 4 MSB bits contains stream ID, 4 LSB bits are incremented.

Test requires that two pairs of pins are shortened: TX with RX and CTS with RTS.
UART receives loopback data and validates if for each stream (identified by ID)
data is consistent.
