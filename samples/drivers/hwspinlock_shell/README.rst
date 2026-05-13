.. zephyr:code-sample:: hwspinlock_shell
   :name: HWSPINLOCK shell
   :relevant-api: hwspinlock_interface

Overview
********

This sample enables the ``hwspinlock`` shell command (see
:ref:`hwspinlock_api`) for interactive testing.

It is useful when the remote side is not running Zephyr (e.g. another OS or a
firmware monitor). The shell provides commands to list hwspinlock controllers,
select a lock, and lock/unlock it.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/hwspinlock_shell
   :board: imx95_evk/mimx9596/m7
   :goals: build

Running
*******

On the UART shell:

.. code-block:: console
   uart:~$ hwspinlock select hwspinlock@44260000 0x0
   selected hwspinlock@44260000 id=0 tip: use 'hwspinlock status' or 'hwspinlock trylock'
   uart:~$ hwspinlock trylock
   trylock ok
   uart:~$ hwspinlock status
   status: locked (held locally)
   uart:~$ hwspinlock lock
   already holding by local
   uart:~$ hwspinlock unlock
   unlocked
   uart:~$ hwspinlock lock
   locking...
   locked
   uart:~$ hwspinlock lock
   already holding by local
   uart:~$ hwspinlock unlock
   unlocked
   uart:~$ hwspinlock lock
   status: locked by remote
   locking...
   locked
   uart:~$
   uart:~$
   uart:~$ hwspinlock status
   status: unlocked
   uart:~$ hwspinlock status
   status: locked by remote

.. code-block:: console
   Hello from SM (Build 855, Commit 17cc3827, May 15 2026 11:26:55)
   *** SM Debug Monitor ***
   >$ mm.b 0x44260003 9 1
   >$ mm.b 0x44260003 9 1
   >$ mm.b 0x44260003 0 1
   >$ mm.b 0x44260003 9 1
