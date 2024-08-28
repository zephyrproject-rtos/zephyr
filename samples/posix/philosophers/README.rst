.. zephyr:code-sample:: posix-philosophers
   :name: POSIX Philosophers

   Implement a solution to the Dining Philosophers problem using the POSIX API.

Overview
********

This sample implements Zephyr's :ref:`Dining Philosophers Sample <dining-philosophers-sample>` using the
:ref:`POSIX API <posix_support>`. The source code for this sample can be found under
:file:`samples/posix/philosophers`.

Building and Running
********************

This project outputs to the console. It can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/posix/philosophers
   :host-os: unix
   :board: qemu_riscv64
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

   Philosopher 0 [P: 3]  HOLDING ONE FORK
   Philosopher 1 [P: 2]  HOLDING ONE FORK
   Philosopher 2 [P: 1]  EATING  [ 1900 ms ]
   Philosopher 3 [P: 0]  THINKING [ 2500 ms ]
   Philosopher 4 [C:-1]  THINKING [ 2200 ms ]
   Philosopher 5 [C:-2]  THINKING [ 1700 ms ]

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
*********

Like the original philosophers sample, the POSIX variant also enables
:kconfig:option:`CONFIG_DEBUG_THREAD_INFO` by default.

.. zephyr-app-commands::
   :zephyr-app: samples/philosophers
   :host-os: unix
   :board: <board_name>
   :goals: debug
   :compact:

Additional Information
**********************

For additional information, please refer to the
:ref:`Dining Philosophers Sample <dining-philosophers-sample>`.
