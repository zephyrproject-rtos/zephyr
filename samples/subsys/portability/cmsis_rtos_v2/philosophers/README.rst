.. zephyr:code-sample:: cmsis-rtos-v2
   :name: Dining Philosophers (CMSIS RTOS V2 APIs)

   Implement a solution to the Dining Philosophers problem using CMSIS RTOS V2.

Overview
********
This sample implements a solution to the `Dining Philosophers problem
<https://en.wikipedia.org/wiki/Dining_philosophers_problem>`_ (a classic
multi-thread synchronization problem) using CMSIS RTOS V2 APIs.  This particular
implementation demonstrates the usage of multiple preemptible and cooperative
threads of differing priorities, as well as mutex/semaphore and thread sleeping
which uses CMSIS RTOS V2 API implementation in Zephyr.

The philosopher always tries to get the lowest fork first (f1 then f2).  When
done, he will give back the forks in the reverse order (f2 then f1).  If he
gets two forks, he is EATING.  Otherwise, he is THINKING. Transitional states
are shown as well, such as STARVING when the philosopher is hungry but the
forks are not available, and HOLDING ONE FORK when a philosopher is waiting
for the second fork to be available.

Each Philosopher will randomly alternate between the EATING and THINKING state.


Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/philosophers
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

    Philosopher 0 [P: 8]    HOLDING ONE FORK
    Philosopher 1 [P: 9]    HOLDING ONE FORK
    Philosopher 2 [P: 10]   EATING  [  575 ms ]
    Philosopher 3 [P: 11]  THINKING [  225 ms ]
    Philosopher 4 [P: 12]  THINKING [  425 ms ]
    Philosopher 5 [P: 13]  THINKING [  325 ms ]

    Demo Description
    ----------------
    An implementation of a solution to the Dining Philosophers
    problem (a classic multi-thread synchronization problem) using
    CMSIS RTOS V2 APIs. This particular implementation demonstrates the
    usage of multiple preemptible threads of differing
    priorities, as well as semaphores and thread sleeping.

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
