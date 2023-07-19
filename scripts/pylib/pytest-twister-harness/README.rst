==============
Pytest Twister harness
==============

Installation
------------

If you plan to use this plugin with Twister, then you don't need to install it
separately by pip. When Twister uses this plugin for pytest tests, it updates
`PYTHONPATH` variable, and then extends pytest command by
`-p twister_harness.plugin` argument.


Usage
-----

Run exemplary test shell application by Twister:

.. code-block:: sh

  cd ${ZEPHYR_BASE}

  # native_posix & QEMU
  ./scripts/twister -p native_posix -p qemu_x86 -T samples/subsys/testsuite/pytest/shell

  # hardware
  ./scripts/twister -p nrf52840dk_nrf52840 --device-testing --device-serial /dev/ttyACM0 -T samples/subsys/testsuite/pytest/shell

or build shell application by west and call pytest directly:

.. code-block:: sh

  export PYTHONPATH=${ZEPHYR_BASE}/scripts/pylib/pytest-twister-harness/src:${PYTHONPATH}

  cd ${ZEPHYR_BASE}/samples/subsys/testsuite/pytest/shell

  # native_posix
  west build -p -b native_posix -- -DCONFIG_NATIVE_UART_0_ON_STDINOUT=y
  pytest --twister-harness --device-type=native --build-dir=build -p twister_harness.plugin

  # QEMU
  west build -p -b qemu_x86 -- -DQEMU_PIPE=qemu-fifo
  pytest --twister-harness --device-type=qemu --build-dir=build -p twister_harness.plugin

  # hardware
  west build -p -b nrf52840dk_nrf52840
  pytest --twister-harness --device-type=hardware --device-serial=/dev/ttyACM0 --build-dir=build -p twister_harness.plugin
