.. zephyr:code-sample:: logging-flash-backend
   :name: Logging to a flash partition
   :relevant-api: log_backend_flash

   Keep a log in flash across reboots and read it back from the shell.

Overview
********

This is the :zephyr:code-sample:`logging` sample with a flash log backend under
it, so that the messages it produces outlive the run that produced them, and the
``log_flash`` shell command to read them back.

The sample builds the logging sample's sources rather than its own, so what is
logged is the same; what differs is where it ends up.

Requirements
************

A flash partition to log into, pointed at by the ``zephyr,log-partition``
chosen node, on memory that does not need an explicit erase before a write,
such as RRAM or MRAM. ``boards/native_sim.overlay`` carves one out of the
simulated flash and puts the simulator in the right mode.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/logging/flash_backend
   :board: native_sim
   :goals: build run
   :compact:

The sample logs its messages as it starts. They are in the partition from then
on, so after resetting the board the shell still has them::

   uart:~$ log_flash info
   records:   138
   dropped:   0
   used:      4400 of 32704 bytes
   build id:  v4.4.0-17475-gf2223977a5b0
   uart:~$ log_flash read 0 4
   [         0] *** Booting Zephyr OS build v4.4.0-17475-gf2223977a5b0 ***
   [    130000] ---=< RUNNING LOGGER DEMO FROM KERNEL THREAD >=---
   [    130000] Module logging showcase.
   [    130000] <inf> sample_module: log in test_module 11

A record carries the timestamp the message was logged at, in whatever unit
:c:type:`log_timestamp_t` counts. The ones that came from :c:func:`printk` are
printed without a source.

``log_flash read`` formats the records against the running firmware, which only
works for a log that firmware wrote; ``log_flash dump`` prints them as hex for
:file:`scripts/logging/dictionary/log_parser.py` to decode against the ``.elf``
that did. ``log_flash erase`` discards the log.
