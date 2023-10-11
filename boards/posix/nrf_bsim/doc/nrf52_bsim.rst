.. _nrf52_bsim:

NRF52 simulated board (BabbleSim)
#################################

.. contents::
   :depth: 1
   :backlinks: entry
   :local:


Overview
********

To allow simulating a nRF52833 SOC a Zephyr target boards is provided: the
nrf52_bsim.

This uses `BabbleSim`_ to simulate the radio activity, and the
:ref:`POSIX architecture<Posix arch>` and the `native simulator`_ to
run applications natively on the development system. This has the benefit of
providing native code execution performance and easy debugging using
native tools, but inherits :ref:`its limitations <posix_arch_limitations>`.

This board includes models of some of the nRF52 SOC peripherals:

* Radio
* Timers
* AAR (Accelerated Address Resolver)
* AES CCM & AES ECB encryption HW
* CLOCK (Clock control)
* EGU (Event Generator Unit)
* FICR (Factory Information Configuration Registers)
* GPIO & GPIOTE
* NVMC (Non-Volatile Memory Controller / Flash)
* PPI (Programmable Peripheral Interconnect)
* RNG (Random Number Generator)
* RTC (Real Time Counter)
* TEMP (Temperature sensor)
* UICR (User Information Configuration Registers)

and will use the same drivers as the nrf52 dk targets for these.
For more information on what is modelled to which level of detail,
check the `HW models implementation status`_.

Note that unlike a real nrf52 device, the nrf52_bsim has unlimited RAM and flash for code.

.. _BabbleSim:
   https://BabbleSim.github.io

.. _native simulator:
   https://github.com/BabbleSim/native_simulator/blob/main/docs/README.md

.. _HW models implementation status:
   https://github.com/BabbleSim/ext_nRF_hw_models/blob/main/docs/README_impl_status.md

.. _nrf52bsim_build_and_run:

Building and running
********************

This board requires the host 32 bit C library. See
:ref:`POSIX Arch dependencies<posix_arch_deps>`.

To target this board you also need to have `BabbleSim`_ compiled in your system.
If you do not have it yet, the easiest way to get it, is to enable the babblesim group
in your local west configuration, running west update, and building the simulator:

.. code-block:: console

   west config manifest.group-filter -- +babblesim
   west update
   cd ${ZEPHYR_BASE}/../tools/bsim
   make everything -j 8

.. note::

   If you need more BabbleSim components, or more up to date versions,
   you can check the `BabbleSim web page <https://BabbleSim.github.io>`_
   for instructions on how to
   `fetch <https://babblesim.github.io/fetching.html>`_ and
   `build <https://babblesim.github.io/building.html>`_ it.

You will now need to define two environment variables to point to your BabbleSim
installation, ``BSIM_OUT_PATH`` and ``BSIM_COMPONENTS_PATH``.
If you followed the previous steps, you can just do:

.. code-block:: console

   export BSIM_OUT_PATH=${ZEPHYR_BASE}/../tools/bsim
   export BSIM_COMPONENTS_PATH=${BSIM_OUT_PATH}/components/

.. note::

   You can add these two lines to your ``~/.zephyrrc`` file, or to your shell
   initialization script (``~/.bashrc``), so you won't need to rerun them
   manually for each new shell.

You're now ready to build applications targeting this board, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: nrf52_bsim
   :goals: build
   :compact:

Then you can execute your application using:

.. code-block:: console

   $ ./build/zephyr/zephyr.exe -nosim
   # Press Ctrl+C to exit

Note that the executable is a BabbleSim executable. The ``-nosim`` command line
option indicates you want to run it detached from a BabbleSim simulation. This
is possible only while there is no radio activity. But is perfectly fine for
most Zephyr samples and tests.

When you want to run a simulation with radio activity you need to run also the
BableSim 2G4 (2.4GHz) physical layer simulation (phy).

For example, if you would like to run a simple case with 1 BLE ``central_hr``
sample application connecting to a BLE ``peripheral`` sample application:
Build the ``central_hr`` application targeting this board and copy the resulting
executable to the simulator bin folder with a sensible name:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/central_hr
   :host-os: unix
   :board: nrf52_bsim
   :goals: build
   :compact:

.. code-block:: console

   $ cp build/zephyr/zephyr.exe \
     ${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_samples_bluetooth_central_hr

Do the same for the ``peripheral`` sample app:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral
   :host-os: unix
   :board: nrf52_bsim
   :goals: build
   :compact:

.. code-block:: console

   $ cp build/zephyr/zephyr.exe \
     ${BSIM_OUT_PATH}/bin/bs_nrf52_bsim_samples_bluetooth_peripheral

And then run them together with BabbleSim's 2G4 physical layer simulation:

.. code-block:: console

   cd ${BSIM_OUT_PATH}/bin/
   ./bs_nrf52_bsim_samples_bluetooth_peripheral -s=trial_sim -d=0 &
   ./bs_nrf52_bsim_samples_bluetooth_central_hr -s=trial_sim -d=1 &
   ./bs_2G4_phy_v1 -s=trial_sim -D=2 -sim_length=10e6 &

Where the ``-s`` command line option provides a string which uniquely identifies
this simulation; the ``-D`` option tells the Phy how many devices will be run
in this simulation; the ``-d`` option tells each device which is its device
number in the simulation; and the ``-sim_length`` option specifies the length
of the simulation in microseconds.
BabbleSim devices and Phy support many command line switches.
Run them with ``-help`` for more information.

You can find more information about how to run BabbleSim simulations in
`this BabbleSim example <https://babblesim.github.io/example_2g4.html>`_.


C library choice
****************

These nRF bsim boards use the `native simulator`_ at their core, so you can chose with which
C library you want to build your embedded code.
Check the :ref:`native simulator C library choice section<native_sim_Clib_choice>` for more info.


Debugging, coverage and address sanitizer
*****************************************

Just like with :ref:`native_posix<native_posix_debug>`, the resulting
executables are Linux native applications.
Therefore they can be debugged or instrumented with the same tools as any other
native application, like for example ``gdb`` or ``valgrind``.

The same
:ref:`code coverage analysis means from the POSIX arch<coverage_posix>`
are inherited in this board.
Similarly, the
:ref:`address and undefined behavior sanitizers can be used as in native_posix<native_posix_asan>`.


Note that BabbleSim will run fine if one or several of its components are
being run in a debugger or instrumented. For example, pausing a device in a
breakpoint will pause the whole simulation.

BabbleSim is fully deterministic by design and the results are not affected by
the host computing speed. All randomness is controlled by random seeds which can
be provided as command line options.


About time in BabbleSim
************************

Note that time in BabbleSim is simulated and decoupled from real time. Normally
simulated time will pass several orders of magnitude faster than real time,
only limited by your workstation compute power.
If for some reason you want to limit the speed of the simulation to real
time or a ratio of it, you can do so by connecting the `handbrake device`_
to the BabbleSim Phy.

.. _handbrake device:
   https://github.com/BabbleSim/base/tree/master/device_handbrake
