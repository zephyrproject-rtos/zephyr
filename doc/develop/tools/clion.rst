.. _clion_ide:

CLion
#####

CLion_ is a cross-platform C/C++ IDE that supports multi-threaded RTOS debugging.

This guide describes the process of setting up, building, and debugging Zephyr's
:zephyr:code-sample:`multi-thread-blinky` sample in CLion.

The instructions have been tested on Windows. In terms of the CLion workflow, the steps would be the
same for macOS and Linux, but make sure to select the correct environment file and to adjust the
paths.

Get CLion
*********

`Download CLion`_ and install it.

Initialize a new workspace
**************************

This guide gives details on how to build and debug the :zephyr:code-sample:`multi-thread-blinky`
sample application, but the instructions would be similar for any Zephyr project and :ref:`workspace
layout <west-workspaces>`.

Before you start, make sure you have a working Zephyr development environment, as per the
instructions in the :ref:`getting_started`.

Open the project in CLion
**************************

#. In CLion, click :guilabel:`Open` on the Welcome screen or select :menuselection:`File --> Open`
   from the main menu.

#. Navigate to your Zephyr workspace (i.e.the :file:`zephyrproject` folder in your HOME directory if
   you have followed the Getting Started instructions), then select
   :file:`zephyr/samples/basic/threads` or another sample project folder.

   Click :guilabel:`OK`.

#. If prompted, click :guilabel:`Trust Project`.

   See the `Project security`_ section in CLion web help for more information on project security.

Configure the toolchain and CMake profile
*****************************************

CLion will open the :guilabel:`Open Project Wizard` with the CMake profile settings. If that does
not happen, go to :menuselection:`Settings --> Build, Execution, Deployment --> CMake`.

#. Click :guilabel:`Manage Toolchains` next to the :guilabel:`Toolchain` field. This will open the
   :guilabel:`Toolchain` settings dialog.

#. We recommend that you use the :guilabel:`Bundled MinGW` toolchain with default settings on
   Windows, or the :guilabel:`System` (default) toolchain on Unix machines.

#. Click :menuselection:`Add environment --> From file` and select
   ``..\.venv\Scripts\activate.bat``.

   .. figure:: img/clion_toolchain_mingw.webp
      :width: 600px
      :align: center
      :alt: MinGW toolchain with environment script

   Click :guilabel:`Apply` to save the changes.

#. Back in the CMake profile settings dialog, specify your board in the :guilabel:`CMake options`
   field. For example:

   .. code-block::

      -DBOARD=nrf52840dk/nrf52840

   .. figure:: img/clion_cmakeprofile.webp
      :width: 600px
      :align: center
      :alt: CMake profile

#. Click :guilabel:`Apply` to save the changes.

   CMake load should finish successfully.

Configure Zephyr parameters for debug
*************************************

#. In the configuration switcher on the top right, select :guilabel:`guiconfig` and click the hammer
   icon.

#. Use the GUI application to set the following flags:

   .. code-block::

      DEBUG_THREAD_INFO
      THREAD_RUNTIME_STATS
      DEBUG_OPTIMIZATIONS

Build the project
*****************

In the configuration switcher, select **zephyr_final** and click the hammer icon.

Note that other CMake targets like ``puncover`` or ``hardenconfig`` can also be called at this
point.


Enable RTOS integration
***********************

#. Go to :menuselection:`Settings --> Build, Execution, Deployment --> Embedded Development --> RTOS
   Integration`.

#. Set the :guilabel:`Enable RTOS Integration` checkbox.

   This option enables Zephyr tasks view during debugging. See `Multi-threaded RTOS debug`_ in CLion
   web help for more information.

   You can leave the option set to :guilabel:`Auto`. CLion will detect Zephyr automatically.

Create an Embedded GDB Server configuration
*******************************************

In order to debug a Zephyr application in CLion, you need to create a run/debug configuration out of
the Embedded GDB Server template.

Instructions below show the case of a Nordic Semiconductor board and a Segger J-Link debug probe. If
your setup is different, make sure to adjust the configuration settings accordingly.

#. Select :menuselection:`Run --> New Embedded Configuration` from the main menu.

#. Configure the settings:

    .. list-table::
        :header-rows: 1

        * - Option
          - Value

        * - :guilabel:`Name` (optional)
          - Zephyr-threads

        * - :guilabel:`GDB Server Type`
          - Segger JLink

        * - :guilabel:`Location`
          - The path to ``JLinkGDBServerCL.exe`` on Windows or the ``JLinkGDBServer`` binary on
            macOS/Linux.

        * - :guilabel:`Debugger`
          - Bundled GDB

            .. note:: For non-ARM and non-x86 architectures, use a GDB executable
               from Zephyr SDK. Make sure to pick a version with Python support
               (for example, **riscv64-zephyr-elf-gdb-py**) and check that Python
               is present in the system ``PATH``.

        * - :guilabel:`Target`
          - zephyr-final

        * - :guilabel:`Executable binary`
          - zephyr-final

        * - :guilabel:`Download binary`
          - Always

        * - :guilabel:`TCP/IP port`
          - Auto

    .. figure:: img/clion_gdbserverconfig.webp
       :width: 500px
       :align: center
       :alt: Embedded GDB server configuration

#. Click :guilabel:`Next` to set the Segger J-Link parameters.

    .. figure:: img/clion_segger_settings.webp
       :width: 500px
       :align: center
       :alt: Segger J-Link parameters

#. Click :guilabel:`Create` when ready.

Start debugging
***************

#. Place breakpoints by clicking in the left gutter next to the code lines.

#. Make sure that **Zephyr-threads** is selected in the configuration switcher and click the bug
   icon or press :kbd:`Ctrl+D`.

#. When a breakpoint is hit, CLion opens the Debug tool window.

   Zephyr tasks are listed in the :guilabel:`Threads & Variables` pane. You can switch between them
   and inspect the variables for each task.

    .. figure:: img/clion_debug_threads.webp
       :width: 800px
       :align: center
       :alt: Viewing Zephyr tasks during a debug session

   Refer to `CLion web help`_ for detailed description of the IDE debug capabilities.

.. _CLion: https://www.jetbrains.com/clion/
.. _Download CLion: https://www.jetbrains.com/clion/download
.. _Project security: https://www.jetbrains.com/help/clion/project-security.html#projects_security
.. _Multi-threaded RTOS debug: https://www.jetbrains.com/help/clion/rtos-debug.html
.. _CLion web help: https://www.jetbrains.com/help/clion/debugging-code.html
