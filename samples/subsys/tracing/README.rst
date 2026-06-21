.. zephyr:code-sample:: tracing
   :name: Tracing

   Generate kernel and application trace events and stream them to a host with
   any of the supported tracing formats and backends.

Overview
********

This sample demonstrates the :ref:`tracing` subsystem. It drives a
representative cross-section of the kernel so that, whatever tracing format and
backend are selected at build time, a rich and continuous stream of trace
events is produced for capture and visualization on a host:

* Two threads (one static, one dynamic) ping-pong "Hello World" greetings,
  synchronising with a pair of semaphores. This keeps generating thread switch,
  sleep and semaphore give/take events for as long as the sample runs.
* On start-up, a one-shot workload drives a mutex, the system work queue and a
  kernel timer so that those objects' lifecycle events appear in the trace as
  well.
* :c:func:`sys_trace_named_event` is used throughout to emit custom,
  application-defined events (a short name plus two 32-bit arguments). Named
  events are the primary way to annotate a trace from application code.

The optional GPIO scenario (:zephyr_file:`samples/subsys/tracing/src/gpio_main.c`)
additionally exercises the GPIO API so that the GPIO tracing hooks fire.

Formats and backends
********************

Tracing is configured along two independent axes: the *format* describes how
events are encoded, and the *backend* describes how the encoded bytes leave the
device.

The default :zephyr_file:`samples/subsys/tracing/prj.conf` is a complete,
working baseline: it enables tracing with the :ref:`CTF <ctf>` format and the
RAM backend, which needs no transport or devicetree setup and therefore builds
and runs on any board. Building the sample with no extra options already
produces a trace.

To stream the trace over a transport, or to use a different format, layer one of
the ``prj_*.conf`` overlays on top of the baseline with ``EXTRA_CONF_FILE``.
Each overlay contains only the options that differ from the baseline, so they
can be combined with it (and with board configuration) without repetition:

.. list-table::
   :header-rows: 1

   * - Configuration
     - Format
     - Backend
     - Notes
   * - ``prj.conf`` (default)
     - CTF
     - RAM
     - Binary CTF buffered in RAM, dumped with a debugger. Works on any board.
   * - ``prj_uart.conf``
     - test
     - UART
     - Human-readable strings over a dedicated UART.
   * - ``prj_uart_ctf.conf``
     - CTF
     - UART
     - Binary CTF stream over a dedicated UART.
   * - ``prj_usb_ctf.conf``
     - CTF
     - USB
     - Binary CTF stream over a USB CDC ACM channel.
   * - ``prj_native_ctf.conf``
     - CTF
     - POSIX
     - Binary CTF written to a file by ``native_sim``.
   * - ``prj_user.conf``
     - user
     - n/a
     - Application-provided callbacks (see ``src/tracing_user.c``).
   * - ``prj_gpio.conf``
     - user
     - n/a
     - User callbacks focused on the GPIO hooks.
   * - ``prj_percepio.conf``
     - Percepio
     - module
     - Percepio Tracealyzer recorder (requires the ``percepio`` module).
   * - ``rtt-tracing`` snippet
     - SystemView
     - RTT
     - SEGGER SystemView over RTT.

Capturing and decoding the output may also require host-side tooling, noted in
each section below.

Usage for the default (RAM) configuration
*****************************************

Build the baseline for any board, with no extra options:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: qemu_x86
	:goals: build
	:compact:

The CTF stream is buffered in the in-RAM ``ram_tracing`` array. Let the
application run for a while, halt the target in your debugger, and dump the
buffer to a file. For example, from GDB:

.. code-block:: console

	dump binary memory channel0_0 &ram_tracing &ram_tracing+sizeof(ram_tracing)

The size of the buffer is controlled by :kconfig:option:`CONFIG_RAM_TRACING_BUFFER_SIZE`.
The dumped file is a CTF stream; decode it as described in
`Decoding a CTF trace`_.

Usage for UART Tracing Backend
*******************************

Build a UART-tracing image with:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: mps2/an521
	:gen-args: -DEXTRA_CONF_FILE=prj_uart.conf
	:goals: build
	:compact:

or, for a binary CTF stream:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: mps2/an521
	:gen-args: -DEXTRA_CONF_FILE=prj_uart_ctf.conf
	:goals: build
	:compact:

.. note::
   You may need to set the ``zephyr,tracing-uart`` property under the chosen
   node in your devicetree. See
   :zephyr_file:`samples/subsys/tracing/boards/mps2_an521_cpu0.overlay` for an
   example.

After the application has run for a while, check the trace output file (the
bytes received on the tracing UART). For the CTF stream, decode it as described
in `Decoding a CTF trace`_.

Usage for USB Tracing Backend
*****************************

Build a USB-tracing image with:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: reel_board
	:gen-args: -DEXTRA_CONF_FILE=prj_usb_ctf.conf
	:goals: build
	:compact:

After the serial console has stable output like this:

.. code-block:: console

	thread_a: Hello World from reel_board!
	thread_b: Hello World from reel_board!
	thread_a: Hello World from reel_board!
	thread_b: Hello World from reel_board!

Connect the board's USB port to the host device and
run the :zephyr_file:`scripts/tracing/trace_capture_usb.py` script on the host:

.. code-block:: console

	sudo python3 trace_capture_usb.py -v 0x2FE3 -p 0x0001 -o channel0_0

The VID and PID of the USB device can be configured; just adjust them
accordingly. The captured ``channel0_0`` file is a CTF stream; decode it as
described in `Decoding a CTF trace`_.

Usage for POSIX Tracing Backend
*******************************

Build a POSIX-tracing image with:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: native_sim
	:gen-args: -DEXTRA_CONF_FILE=prj_native_ctf.conf
	:goals: build
	:compact:

Run the resulting executable; it writes the CTF stream to a file named
``channel0_0`` in the current working directory:

.. code-block:: console

	./build/zephyr/zephyr.exe

Stop it after a few seconds with :kbd:`Ctrl-C` and decode ``channel0_0`` as
described in `Decoding a CTF trace`_.

Usage for USER Tracing Backend
******************************

The "user" format does not use a backend at all: instead the kernel calls weak
``sys_trace_*_user`` callbacks that the application implements directly. See
:zephyr_file:`samples/subsys/tracing/src/tracing_user.c` for the thread, ISR,
sleep and GPIO hooks implemented by this sample. Build a USER-tracing image
with:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: qemu_x86
	:gen-args: -DEXTRA_CONF_FILE=prj_user.conf
	:goals: build
	:compact:

The callbacks print directly to the console as the events occur, for example:

.. code-block:: console

	sys_trace_thread_switched_in_user: 0x...
	sys_trace_k_thread_sleep_enter_user: thread=0x... timeout=... ticks

Usage for SEGGER SystemView RTT
*******************************

Build a SystemView-tracing image with the :ref:`snippet-rtt-tracing`:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: frdm_k64f
	:snippets: rtt-tracing
	:goals: build
	:compact:

Open the trace in the SEGGER SystemView host application while the target runs.

Usage for Percepio Tracealyzer
******************************

Build a Percepio-tracing image (requires the ``percepio`` module to be present
in your workspace):

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: frdm_k64f
	:gen-args: -DEXTRA_CONF_FILE=prj_percepio.conf
	:goals: build
	:compact:

Open the resulting recording in Percepio Tracealyzer.

Usage for GPIO Tracing
**********************

Build a GPIO-tracing image (the ``user`` format with the GPIO hooks) with:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/tracing
	:board: native_sim
	:gen-args: -DEXTRA_CONF_FILE=prj_gpio.conf -DEXTRA_DTC_OVERLAY_FILE=gpio.overlay
	:goals: build
	:compact:

The GPIO callbacks print to the console as the GPIO API is exercised.

Decoding a CTF trace
********************

The CTF backends (UART, USB, POSIX and RAM) emit a binary stream that follows
the metadata in :zephyr_file:`subsys/tracing/ctf/tsdl/metadata`. To view it,
place the captured stream next to that metadata file and open it with a
CTF-aware tool such as `babeltrace2 <https://babeltrace.org/>`_:

.. code-block:: console

	mkdir ctf
	cp channel0_0 ctf/
	cp $ZEPHYR_BASE/subsys/tracing/ctf/tsdl/metadata ctf/
	babeltrace2 ctf/

The same ``ctf`` directory can also be opened in `Trace Compass
<https://eclipse.dev/tracecompass/>`_ for a graphical timeline.
