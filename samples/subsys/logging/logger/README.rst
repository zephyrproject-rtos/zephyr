.. zephyr:code-sample:: logging
   :name: Logging
   :relevant-api: log_api log_ctrl

   Output log messages to the console using the logging subsystem.

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
        [00:00:00.106,051] <inf> sample_module: log in test_module 11
        [00:00:00.106,054] <inf> sample_module: Inline function.
        Disabling logging in the sample_module module
        Function called again but with logging disabled.
        Instance level logging showcase.
        [00:00:00.106,200] <inf> sample_instance.inst1: Inline call.
        [00:00:00.106,204] <inf> sample_instance.inst1: counter_value: 0
        [00:00:00.106,209] <wrn> sample_instance.inst1: Example of hexdump:
        01 02 03 04             |....
        [00:00:00.106,214] <inf> sample_instance.inst2: Inline call.
        [00:00:00.106,218] <inf> sample_instance.inst2: counter_value: 0
        [00:00:00.106,223] <wrn> sample_instance.inst2: Example of hexdump:
        01 02 03 04             |....
        Changing filter to warning on sample_instance.inst1 instance.
        [00:00:00.106,297] <wrn> sample_instance.inst1: Example of hexdump:
        01 02 03 04             |....
        [00:00:00.106,302] <inf> sample_instance.inst2: Inline call.
        [00:00:00.106,307] <inf> sample_instance.inst2: counter_value: 1
        [00:00:00.106,311] <wrn> sample_instance.inst2: Example of hexdump:
        01 02 03 04             |....
        Disabling logging on both instances.
        Function call on both instances with logging disabled.
        String logging showcase.
        [00:00:01.122,316] <inf> main: Logging transient string:transient_string
        Severity levels showcase.
        [00:00:01.122,348] <err> main: Error message example.
        [00:00:01.122,352] <wrn> main: Warning message example.
        [00:00:01.122,355] <inf> main: Info message example.
        Logging performance showcase.
        [00:00:02.151,602] <inf> main: performance test - log message 0
        Estimated logging capabilities: 42000000 messages/second
        Logs from external logging system showcase.
        [00:00:03.165,977] <err> ext_log_system: critical level log
        [00:00:03.165,991] <err> ext_log_system: error level log, 1 arguments: 1
        [00:00:03.166,006] <wrn> ext_log_system: warning level log, 2 arguments: 12
        [00:00:03.166,025] <inf> ext_log_system: notice level log, 3 arguments: 105
        [00:00:03.166,044] <inf> ext_log_system: info level log, 4 arguments : 1 24

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
