.. _samples_lcd_hd44780:

LCD HD44780 driver sample
#########################

Overview
********
Display text strings on parallel interfacing HD44780 based
generic LCD controller using GPIO pins to interface with
Arduino Due (SAM3).

Building and Running
********************

This project can be built and executed on as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/lcd_hd44780
   :host-os: unix
   :board: arduino_due
   :goals: flash
   :compact:

Sample Output
=============

.. code-block:: console

    LCD Init
    Page 1: message
    Page 2: message
    Page 3: message

Display output
==============

.. code-block:: console

    ********************
    Arduino Due
    yalpsiD DCL 4x02
    ********************

.. code-block:: console

    -------------------
    Zephyr Rocks!
    My super RTOS
    -------------------

.. code-block:: console

    --------------------
    --------HOME--------
    I am home!
