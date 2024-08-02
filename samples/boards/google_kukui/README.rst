.. _google_kukui:

Kukui general features
######################

Overview
********

This provides access to :ref:`Kukui <google_kukui_board>` through a serial shell
so you can try out the supported features, including I2C, GPIO and flash access.

Building
********

The sample can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/google_kukui
   :board: google_kukui
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Welcome to Google Kukui

    uart:~$    (press tab)
      clear    flash    gpio     help     history  i2c      resize   shell
    uart:~$ i2c read I2C_2 36
    00000000: 82 00 00 ff 80 7f 00 ff  00 00 c1 0a c8 5b 0c 62 |........ .....[.b|
