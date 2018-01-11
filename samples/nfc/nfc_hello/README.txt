NFC Sample App
--------------

This is a simple application to test an elementary signal-through
with NFC module connected to the second UART.


Build and run
-------------

To test the serial line routines, open a terminal window and type:

	nc -l 8888

Open another terminal window and type:

For QEMU x86:
	mkdir build; cd build
	cmake -DBOARD=qemu_x86 ..
	make run

For QEMU ARM:
	mkdir build; cd build
	cmake -DBOARD=qemu_cortex_m3 ..
	make run


Sample output
-------------

Write some random text on the nc terminal window. The terminal
window running qemu must display:

For QEMU x86:

[QEMU] CPU: qemu32
Sample app running on: x86
uart1_init() done
uart1_isr: 61 73 64 73 61 64 73 0a (8 bytes)

For QEMU ARM:

[QEMU] CPU: cortex-m3
Sample app running on: arm
uart1_init() done
uart1_isr: 61 (1 bytes)
uart1_isr: 73 (1 bytes)
uart1_isr: 64 (1 bytes)
uart1_isr: 73 (1 bytes)
uart1_isr: 61 (1 bytes)
uart1_isr: 0a (1 bytes)
uart1_isr: 73 (1 bytes)
uart1_isr: 73 (1 bytes)

