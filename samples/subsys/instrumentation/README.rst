.. zephyr:code-sample:: instrumentation
   :name: Instrumentation

   Demonstrate the instrumentation subsystem tracing and profiling features.

Overview
********

This sample shows the instrumentation subsystem tracing and profiling
features. It basically consists of two threads in a ping-pong mode, taking
turns to execute loops that spend some CPU cycles.

Requirements
************

A Linux host and a UART console is required to run this sample.

Building and Running
********************

Build and flash the sample as follows, changing ``mps2/an385`` for your
board.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/instrumentation
   :host-os: unix
   :board: mps2/an385
   :goals: build flash
   :compact:

Alternatively you can run this using QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/instrumentation
   :host-os: unix
   :board: mps2/an385
   :goals: run
   :gen-args: '-DQEMU_SOCKET=y'
   :compact:

After the sample is flashed to the target (or QEMU is running), it must be possible to
collect and visualize traces and profiling info using the instrumentation CLI
tool, :zephyr_file:`scripts/instrumentation/zaru.py`.

.. note::
   Please note, that this subsystem uses the ``retained_mem`` driver, hence it's necessary
   to add the proper devicetree overlay for the target board. See
   :zephyr_file:`./samples/subsys/instrumentation/boards/mps2_an385.overlay` for an example.

Connect the board's UART port to the host device and
run the :zephyr_file:`scripts/instrumentation/zaru.py` script on the host.

Source the :zephyr_file:`zephyr-env.sh` file to set the ``ZEPHYR_BASE`` variable and get
:zephyr_file:`scripts/instrumentation/zaru.py` in your PATH:

.. code-block:: console

   . zephyr-env.sh

Check instrumentation status:

.. code-block:: console

   zaru.py status

Set the tracing/profiling trigger; in this sample the function
``get_sem_and_exec_function`` is the one interesting to allow the observation
of context switches:

.. code-block:: console

   zaru.py trace -v -c get_sem_and_exec_function

Reboot target so tracing/profiling at the location is effective:

.. code-block:: console

   zaru.py reboot

Wait ~2 seconds so the sample finishes 2 rounds of ping-pong between ``main``
and ``thread_A``, and get the traces:

.. code-block:: console

   zaru.py trace -v

Get the profile:

.. code-block:: console

   zaru.py profile -v -n 10

Or alternatively, export the traces to Perfetto (it's necessary
to reboot because ``zaru.py trace`` dumped the buffer and it's now empty):

.. code-block:: console

   zaru.py reboot
   zaru.py trace -v --perfetto --output perfetto_zephyr.json

Then, go to http://perfetto.dev, Trace Viewer, and load ``perfetto_zephyr.json``.
