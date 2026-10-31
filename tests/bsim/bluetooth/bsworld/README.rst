.. Copyright (c) 2026 Xiaomi Corporation
..
.. SPDX-License-Identifier: Apache-2.0

Bluetooth tests with BSWorld
############################

These tests run ``nrf52_bsim/native`` images on a shared BabbleSim PHY and
simulation clock, using `BSWorld <https://github.com/XiaoMi/bsworld>`_.
Every image contains the Host, Controller and radio. pytest drives the images
through their consoles and checks their output.

.. list-table::
   :header-rows: 1

   * - File
     - Scenarios
   * - ``shell/test_shell.py``
     - Passive and active scanning; an encrypted connection carrying a GATT
       notification
   * - ``samples/test_heart_rate.py``
     - ``central_hr`` subscribing to ``peripheral_hr`` and receiving heart rate notifications

The shell is the default ``tests/bluetooth/shell`` build with its console on
stdio, as in the ``bluetooth.native_shell`` twister scenario. The samples are
built with their own ``prj.conf``.

Running
*******

Fetch BSWorld and the Python packages, then run the CI script with
``BSIM_OUT_PATH`` set:

.. code-block:: console

   west config manifest.group-filter -- +bsworld
   west update bsworld
   pip install -r scripts/requirements-build-test.txt
   tests/bsim/ci.bt.bsworld.sh

The script builds the images with ``west build`` and runs pytest. Results are
written to ``twister-out-bsworld/``: ``bsworld.xml`` and, for every test, the
console logs and an Ellisys trace of the radio activity.
