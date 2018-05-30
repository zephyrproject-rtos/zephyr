.. _logger_sample:

Logging
###########

Overview
********
A simple application that demonstrates use of logging subsystem. It demonstrates
main features: severity levels, timestamping, module level filtering and
instance level filtering. It also showcases logging capabilities in terms of
performance.

Building and Running
********************

This project outputs multiple log message to the console.  It can be built and
executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/logging/logger
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

         Module logging showcase.
         [00:00:00.000,122] <info> foo: log in test_module 11
         Disabling logging in the foo module
         Function called again but with logging disabled.
         Instance level logging showcase.
         [00:00:00.000,274] <info> sample_instance.inst1: counter_value: 0
         [00:00:00.000,274] <warn> sample_instance.inst1: Example of hexdump:
         [00:00:00.000,305] <warn> sample_instance.inst1:  01 02 03 04            |....
         [00:00:00.000,305] <info> sample_instance.inst2: counter_value: 0
         [00:00:00.000,305] <warn> sample_instance.inst2: Example of hexdump:
         [00:00:00.000,305] <warn> sample_instance.inst2:  01 02 03 04            |....
         Changing filter to warning on sample_instance.inst1 instance.
         [00:00:00.000,396] <warn> sample_instance.inst1: Example of hexdump:
         [00:00:00.000,427] <warn> sample_instance.inst1:  01 02 03 04            |....
         [00:00:00.000,427] <info> sample_instance.inst2: counter_value: 1
         [00:00:00.000,427] <warn> sample_instance.inst2: Example of hexdump:
         [00:00:00.000,427] <warn> sample_instance.inst2:  01 02 03 04            |....
         Disabling logging on both instances.
         Function call on both instances with logging disabled.
         Severity levels showcase.
         [00:00:00.000,610] <err> main: Error message example.
         [00:00:00.000,610] <warn> main: Warning message example.
         [00:00:00.000,610] <info> main: Info message example.
         Logging performance showcase.
         [00:00:00.134,887] <info> main: performance test - log message 0
         [00:00:00.134,887] <info> main: performance test - log message 1
         [00:00:00.134,887] <info> main: performance test - log message 2
         [00:00:00.134,887] <info> main: performance test - log message 3
         [00:00:00.134,887] <info> main: performance test - log message 4
         [00:00:00.134,887] <info> main: performance test - log message 5
         [00:00:00.134,887] <info> main: performance test - log message 6
         [00:00:00.134,918] <info> main: performance test - log message 7
         [00:00:00.134,918] <info> main: performance test - log message 8
         [00:00:00.134,918] <info> main: performance test - log message 9
         [00:00:00.134,918] <info> main: performance test - log message 10
         [00:00:00.134,918] <info> main: performance test - log message 11
         [00:00:00.134,918] <info> main: performance test - log message 12
         [00:00:00.134,918] <info> main: performance test - log message 13
         [00:00:00.134,948] <info> main: performance test - log message 14
         Estimated logging capabilities: 245760 messages/second
