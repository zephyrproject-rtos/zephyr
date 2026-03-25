.. zephyr:code-sample:: logging-multidomain
	:name: Logging in multidomain environment
	:relevant-api: log_api ipc

	This code sample shows how to use Zephyr logging across application and
	network domains with IPC service.

Overview
********

This sample demonstrates multidomain logging on dual-core targets.

The application core (host) and network core (remote) communicate through the
IPC service RPMsg backend. Logging in the remote domain is forwarded over IPC
to the host domain using the logging multidomain/link features. The sample also
contains a basic IPC ping-pong thread to show endpoint setup and message flow.
In addition, the remote domain runs a continuous logging loop, so ``loop: N``
messages continue after the IPC ping-pong demo ends.

Requirements
************

- One of the following supported boards:

  - nrf5340dk/nrf5340/cpuapp
  - nrf5340bsim/nrf5340/cpuapp

- ``nrf5340bsim`` is a simulated board target (BabbleSim), not physical
	hardware.

- Zephyr workspace initialized and west installed.
- Sysbuild support enabled (used to build host and remote images together).

Building and Running
********************

Build the sample for nrf5340dk/nrf5340/cpuapp:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/logging/multidomain
	:board: nrf5340dk/nrf5340/cpuapp
	:goals: debug
	:west-args: --sysbuild

Build the sample for nrf5340bsim/nrf5340/cpuapp:

.. zephyr-app-commands::
	:zephyr-app: samples/subsys/logging/multidomain
	:board: nrf5340bsim/nrf5340/cpuapp
	:goals: debug
	:west-args: --sysbuild

Viewing Logs on nrf5340dk (real hardware)
==========================================

Flash the sysbuild output to the board, then open a serial terminal connected
to the application core UART.

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

The host logs are printed on the application core UART. Remote-domain logs are
forwarded through IPC and appear there as well.

Viewing Logs on nrf5340bsim (simulated)
=======================================

Run the simulated executable for ``nrf5340bsim``:

.. code-block:: console

	../build/multidomain/zephyr/zephyr.exe

In simulation, output from both cores appears in a single console stream with
prefixes such as ``(CPU:0)`` and ``(CPU:1)``.

After flashing/running, open the console for the application core. You should
observe host logs together with logs forwarded from the remote domain.

Sample Output
*************

Sample Output on nrf5340dk (real hardware)
==========================================

.. code-block:: console

	*** Booting Zephyr OS build v4.x.x ***
	Hello World! nrf5340dk/nrf5340/cpuapp
	[00:00:00.000,000] <inf> app: IPC-service HOST [INST 1] demo started
	[00:00:01.000,244] <inf> app: ipc open 0
	[00:00:01.000,244] <inf> app: wait for bound
	[00:00:01.000,244] <inf> app: bounded
	[00:00:01.000,244] <inf> app: REMOTE [1]: 0
	[00:00:01.000,244] <inf> app: HOST [1]: 1
	...
	[00:00:01.500,000] <inf> app: loop: 3

Sample Output on nrf5340bsim (simulated)
========================================

.. code-block:: console

	d_00: @00:00:00.000000  (CPU:1): IPC-service REMOTE [INST 1] demo started
	d_00: @00:00:00.000000  (CPU:1): [00:00:00.000,000] <inf> app: loop: 0
	d_00: @00:00:00.037842  (CPU:0): Hello World! nrf5340bsim
	d_00: @00:00:00.037842  (CPU:0): [00:00:00.000,000] <inf> app: IPC-service HOST [INST 1] demo started
	d_00: @00:00:01.000245  (CPU:1): [00:00:01.000,244] <inf> app: wait for bound
	d_00: @00:00:01.000245  (CPU:1): [00:00:01.000,244] <inf> app: REMOTE [1]: 0
	d_00: @00:00:01.000245  (CPU:1): [00:00:01.000,244] <inf> app: REMOTE [1]: 2
	d_00: @00:00:01.000245  (CPU:1): [00:00:01.000,244] <inf> app: REMOTE [1]: 4
	...
	d_00: @00:00:01.000245  (CPU:1): IPC-service REMOTE [INST 1] demo ended.
	d_00: @00:00:01.038086  (CPU:0): [00:00:01.000,244] <inf> app: REMOTE [1]: 16
	...
	d_00: @00:00:02.000000  (CPU:1): [00:00:01.500,000] <inf> app: loop: 3

References
**********

- :ref:`ipc_service_api`
- :ref:`logging_api`
- :ref:`sysbuild`
