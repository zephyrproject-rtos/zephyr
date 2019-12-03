Title: Send Encoded Tracing Data To The Host With Supported Backends

Description:

This application can be used to demonstrate the tracing feature. The tracing
data will be encoded with the existing CTF data format and sent to the host
with the currently supported tracing backend (UART and USB).

--------------------------------------------------------------------------------

Usage for UART Tracing Backend:

Build a UART-tracing image with:

    cmake -DBOARD=sam_e70_xplained -DCONF_FILE=prj_uart.conf ..

After the application has run for a while, run the
trace_capture_uart.py script on your host:

    sudo python3 trace_capture_uart.py -d /dev/ttyACM1 -b 115200 -o log_1

The sam_e70_xplained's serial port is /dev/ttyACM1
and is used as the tracing backend. The script's output file
log_1 and metadata under zephyr/subsys/debug/tracing/ctf/tsdl
can be used to serve Tracecompass (https://www.eclipse.org/tracecompass/).

--------------------------------------------------------------------------------

Usage for USB Tracing Backend

Build a USB-tracing image with:

    cmake -DBOARD=sam_e70_xplained -DCONF_FILE=prj_usb.conf ..

After the serial console has stable output like this:

    threadA: Hello World!
    threadB: Hello World!
    threadA: Hello World!
    threadB: Hello World!

connect the board's USB port to the host device and
run the trace_capture_usb.py script on the host:

    sudo python3 trace_capture_usb.py -v 0x2FE9 -p 0x100 -o log_2

The script's output file log_2 and metadata under
zephyr/subsys/debug/tracing/ctf/tsdl can be used to
serve Tracecompass (https://www.eclipse.org/tracecompass/).
