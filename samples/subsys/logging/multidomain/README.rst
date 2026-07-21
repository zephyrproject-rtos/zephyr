.. zephyr:code-sample:: logging_multidomain
   :name: Logging in multi-domain environment
   :relevant-api: log_ctrl ipc

   Use the multidomain logging feature to aggregate log messages from multiple
   cores into a single console output via the IPC service.

Overview
********
This sample demonstrates the multidomain logging feature in Zephyr on a dual-core SoC. It uses a HOST/REMOTE architecture where the HOST core aggregates log messages from the REMOTE core via the IPC service, so output from both cores appears in a single terminal.

Requirements
************
This sample requires a board with dual-core support and IPC capability.
The following targets are supported:

* ``nrf5340dk/nrf5340/cpuapp``
* ``nrf5340bsim/nrf5340/cpuapp``

Building and Running
********************
This sample can be built and flashed on the nrf5340dk as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/logging/multidomain
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build flash
   :west-args: --sysbuild
   :compact:

Sample Output
=============

The following lines are expected to appear in the HOST UART output
(the order may vary):

.. code-block:: console

   Hello World! nrf5340dk
   <inf> app: IPC-service HOST [INST 1] demo started
   <inf> app: loop: 0
   <inf> app: ipc open 0
   <inf> app: wait for bound
   <inf> app: ipc open 0
   <inf> app: bounded
   <inf> app: REMOTE [1]: 0
   <inf> app: HOST [1]: 1
   ...
   <inf> app: IPC-service HOST [INST 1] demo ended.
