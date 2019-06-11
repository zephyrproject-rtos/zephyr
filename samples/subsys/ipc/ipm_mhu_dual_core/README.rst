.. _ipm_mhu_dual_core:

MHU Dual Core
#############

Overview
********
An MHU (Message Handling Unit) enables software to raise interrupts to
the processor cores. This sample is a simple dual-core example for a
Musca A1 board that has two MHU units. This sample only test MHU0, the
steps are:

1. CPU 0 will wake up CPU 1 after initialization
2. CPU 1 will send to CPU 0 an interrupt over MHU0
3. CPU 0 return the same to CPU 1 when received MHU0 interrupt
4. Test done when CPU 1 received MHU0 interrupt

Building and Running
********************

On Musca A1
-----------

This project outputs 'IPM MHU sample on musca_a' to the console.
It can be built and executed on Musca A1 CPU 0 as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipm_mhu_dual_core
   :board: v2m_musca
   :goals: run
   :compact:

This project outputs 'IPM MHU sample on v2m_musca_nonsecure' to the console.
It can be built and executed on Musca A1 CPU 1 as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipm_mhu_dual_core
   :board: v2m_musca_nonsecure
   :goals: run
   :compact:

On Musca B1
-----------

This project outputs 'IPM MHU sample on musca_b1' to the console.
It can be built and executed on Musca B1 CPU 0 as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipm_mhu_dual_core
   :board: v2m_musca_b1
   :goals: run
   :compact:

This project outputs 'IPM MHU sample on v2m_musca_b1_nonsecure' to the console.
It can be built and executed on Musca B1 CPU 1 as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipm_mhu_dual_core
   :board: v2m_musca_b1_nonsecure
   :goals: run
   :compact:

Combine images for Musca
========================

A third-party tool (srecord) is used to generate the Intel formatted hex image.
For more information refer to the `Srecord Manual`_.

.. code-block:: bash

   srec_cat zephyr.bin -Binary -offset $IMAGE_OFFSET zephyr_nonsecure.bin -Binary -offset $IMAGE_NS_OFFSET -o dual_core_zephyr.hex -Intel

   # This command is an example for Musca B1
   srec_cat zephyr.bin -Binary -offset 0xA000000 zephyr_nonsecure.bin -Binary -offset 0xA060400 -o dual_core_zephyr.hex -Intel

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port.

Sample Output
=============

.. code-block:: console

   ***** Booting Zephyr OS zephyr-v1.13.0-3378-g3625524 *****
   IPM MHU sample on musca_a
   CPU 0, get MHU0 success!
   ***** Booting Zephyr OS zephyr-v1.13.0-3378-g3625524 *****
   IPM MHU sample on musca_a_nonsecure
   CPU 1, get MHU0 success!
   MHU ISR on CPU 0
   MHU ISR on CPU 1
   MHU Test Done.


.. _Srecord Manual:
   http://srecord.sourceforge.net/man/man1/srec_cat.html
