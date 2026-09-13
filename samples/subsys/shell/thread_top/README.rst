.. zephyr:code-sample:: shell-thread-top
   :name: kernel thread top shell
   :relevant-api: shell_api thread_apis

   Watch synthetic CPU load threads with the ``kernel thread top`` command.

Overview
********

This sample starts three named load threads with known CPU consumption so
that the ``kernel thread top`` shell command has something interesting to
show:

* ``load_50_percent`` (priority 10) burns 50% of the CPU in 100 ms periods.
* ``load_25_percent`` (priority 11) burns 25% of the CPU in 100 ms periods.
* ``load_remainder`` (lowest application priority) never sleeps and soaks
  up whatever CPU is left, driving the idle percentage close to zero.

The duty-cycle threads measure their busy window with their own runtime
statistics rather than wall-clock time, so the displayed percentages match
the thread names even while higher priority threads preempt them.

The shell thread priority is raised above the load threads so the shell
stays responsive while ``load_remainder`` spins.

Building and Running
********************

Build for a QEMU target and run:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/thread_top
   :host-os: unix
   :board: qemu_cortex_m3
   :goals: run
   :compact:

Then start the live view in the shell (refresh interval in seconds is
optional, default 3):

.. code-block:: console

   uart:~$ kernel thread top -d 1

Sample Output
=============

.. code-block:: console

   top - uptime 4.110 s, refresh 2 s, press 'q' to quit
   Threads: 6 total, CPU: 100.0% busy, 0.0% idle

    %CPU  PRIO  STATE       STACK%  NAME
    46.3    10  queued         23%  load_50_percent
    30.7    14  queued          9%  load_remainder
    22.4    11  sleeping       23%  load_25_percent
     0.4    -1  queued         59%  sysworkq
     0.0     5  pending        32%  shell_uart
     0.0    15                 18%  idle

The duty-cycle percentages read slightly below their nominal values because
shell rendering time slightly stretches each 100 ms load period.

Press :kbd:`q` to quit back to the shell prompt.
