.. _m5stack-cores3-bus-out-en:

M5Stack CoreS3: Enable BUS 5V output
####################################

.. code-block:: console

   west build -S cores3-bus-out-en [...]

This snippet enables 5V output on the M-Bus/Grove connectors of M5Stack
:zephyr:board:`m5stack_cores3` board by asserting the ``BUS_OUT_EN`` control
signal.
