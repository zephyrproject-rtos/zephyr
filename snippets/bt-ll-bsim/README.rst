.. _snippet-bt-ll-bsim:

BabbleSim LLL of the Zephyr Bluetooth LE Controller (bt-ll-bsim)
################################################################

You can build with this snippet by following the instructions in :ref:`the snippets usage page<using-snippets>`.
When building with ``west``, you can do:

.. code-block:: console

   west build -S bt-ll-bsim [...]

Overview
********

This runs the Zephyr Bluetooth LE Controller on the generic packet level
BabbleSim 2.4GHz radio model instead of a vendor radio model, so that Link
Layer features can be tested in simulation independently of any vendor LLL.

Requirements
************

A BabbleSim board that the snippet supports:

- :ref:`nrf52_bsim <nrf52_bsim>`
