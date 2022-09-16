Title: Send Tracing Formatted Packet To The Host With Supported Backends

Description:

This application can be used to demonstrate the tracing feature. The tracing
formatted packet will be sent to the host with the currently supported tracing
backend under tracing generic infrastructure.

--------------------------------------------------------------------------------

Usage for UART Tracing Backend:

Build a UART-tracing image with:

    cmake -DBOARD=mps2_an521 -DCONF_FILE=prj_uart.conf ..

or:

    cmake -DBOARD=mps2_an521 -DCONF_FILE=prj_uart_ctf.conf ..

After the application has run for a while, check the trace output file.

--------------------------------------------------------------------------------

Usage for USB Tracing Backend

Build a USB-tracing image with:

    cmake -DBOARD=sam_e70_xplained -DCONF_FILE=prj_usb.conf ..

or:

    cmake -DBOARD=sam_e70_xplained -DCONF_FILE=prj_usb_ctf.conf ..

After the serial console has stable output like this:

    threadA: Hello World!
    threadB: Hello World!
    threadA: Hello World!
    threadB: Hello World!

connect the board's USB port to the host device and
run the trace_capture_usb.py script on the host:

    sudo python3 trace_capture_usb.py -v 0x2FE9 -p 0x100 -o channel0_0

The VID and PID of USB device can be configured, just adjusting it accordingly.

--------------------------------------------------------------------------------

Usage for POSIX Tracing Backend

Build a POSIX-tracing image with:

    cmake -DBOARD=native_posix -DCONF_FILE=prj_native_posix.conf ..

or:

    cmake -DBOARD=native_posix -DCONF_FILE=prj_native_posix_ctf.conf ..

After the application has run for a while, check the trace output file.
