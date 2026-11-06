UART FIFO read contract
#######################

The test queues two loopback bytes with RX interrupts disabled, then reads
them in the UART callback. A read larger than the available data must return
both bytes. Reads limited to one byte must preserve the remaining byte, and
zero-capacity reads must preserve both the destination and pending data.
Every case also verifies that an empty read leaves its destination untouched.

The native_sim scenario uses a two-byte emulated RX FIFO and latch. It checks
the test's API expectations without exercising SAM0 registers. The same54_xpro
scenario exercises the SAM0 driver with PA16 (TX) wired to PA17 (RX). The UART
under test is separate from the console. Do not select a physical UART with
less than two bytes of receive buffering for this test.

Run the emulated scenario with::

   west twister -T tests/drivers/uart/uart_fifo_read -p native_sim

Run the hardware scenario using a device map with the ``uart_loopback`` fixture.
