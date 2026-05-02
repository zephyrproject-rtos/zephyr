.. _stm32cube_ide:

STM32CubeIDE
############

STM32CubeIDE_ is an Eclipse-based integrated development environment from STMicroelectronics designed for the STM32 series of MCUs and MPUs.

This guide describes the process of setting up, building, and debugging Zephyr
applications using the IDE.

A project must have already been created with Zephyr and west.

These directions have been validated to work with version 1.16.0 of the IDE
on Linux.

Project Setup
*************

#. Before you start, make sure you have a working Zephyr development environment, as per the
   instructions in the :ref:`getting_started`.

#. Run STM32CubeIDE from your Zephyr environment.  Example:

   .. code-block::

      $ /opt/st/stm32cubeide_1.16.0/stm32cubeide

#. Open your already existing project by going to
   :menuselection:`File --> New --> STM32 CMake Project`:

   .. figure:: img/stm32cube_new_cmake.webp
      :align: center
      :alt: Create new CMake project

#. Select :guilabel:`Project with existing CMake sources`, then click :guilabel:`Next`.

#. Select :menuselection:`Next` and browse to the location of your sources.  The
   folder that is opened should have the ``CMakeLists.txt`` and ``prj.conf`` files.

#. Select :menuselection:`Next` and select the appropriate MCU.
   Press :guilabel:`Finish` and your project should now be available.
   However, more actions must still be done in order to configure it properly.

#. Right-click on the newly created project in the workspace, and select
   :guilabel:`Properties`.

#. Go to the :guilabel:`C/C++ Build` page and set the Generator
   to ``Ninja``.  In :guilabel:`Other Options`, specify the target ``BOARD`` in
   CMake argument format. If an out-of-tree board is targeted, the ``BOARD_ROOT``
   option must also be set. The resulting settings page should look similar to this:

   .. figure:: img/stm32cube_project_properties.webp
      :align: center
      :alt: Properties dialog for project

   These options may or may not be needed depending on if you have an
   out-of-tree project or not.

#. Go to the :menuselection:`C/C++ General --> Preprocessor Include` page.
   Select the :guilabel:`GNU C` language, and click on the
   :menuselection:`CDT User Settings Entries` option.

   .. figure:: img/stm32cube_preprocessor_include.webp
      :align: center
      :alt: Properties dialog for preprocessor options

   Click :guilabel:`Add` to add an :guilabel:`Include File`
   that points to Zephyr's ``autoconf.h``, which is located in
   ``<build dir>/zephyr/include/generated/autoconf.h``. This will ensure
   that STM32CubeIDE picks up Zephyr configuration options.
   The following dialog will be shown.  Fill it in as follows:

   .. figure:: img/stm32cube_add_include.webp
      :align: center
      :alt: Add include file dialog

   Once the include file has been added, your properties page should look
   similar to the following:

   .. figure:: img/stm32cube_autoconf_h.webp
      :align: center
      :alt: Properties page after adding autoconf.h file

#. Click :guilabel:`Apply and Close`

#. You may now build the project using the :guilabel:`Build` button on the toolbar.
   The project can be run using the :guilabel:`Run` button, as well as debugged
   using the :guilabel:`Debug` button.

Debugging only
**************

If you only want to use STM32CubeIDE to debug your project you can proceed as follows:

#. First, make sure to compile your project and have the ``zephyr.elf`` available.

#. Run STM32CubeIDE and import your project by going to :menuselection:`File --> Import...`:

   .. figure:: img/stm32cube_menu_import.webp
      :align: center
      :alt: Import project

#. Select :menuselection:`C/C++ --> STM32 Cortex-M Executable`, then click :guilabel:`Next`:

   .. figure:: img/stm32cube_import_project.webp
      :align: center
      :alt: Import project selection

#. Click on :guilabel:`Browse` to browse to your build folder and select your ``zephyr.elf``.

#. Click on :guilabel:`Select` to select your MCU. If relevant, choose also your CPU and/or core.

#. Click on :guilabel:`Finish`.

#. The project can now be debugged using the :guilabel:`Debug` button.

Troubleshooting
***************

When configuring your project you see an error that looks similar to:

.. code-block::

  Error message: Traceback (most recent call last):

    File "/path/to/zephyr/scripts/list_boards.py", line 11, in <module>
      import pykwalify.core

  ModuleNotFoundError: No module named 'pykwalify'


This means that you did not start the IDE in a Zephyr environment.  You must
delete the ``config_default`` build directory and start STM32CubeIDE again,
making sure that you can run ``west`` in the shell that you start STM32CubeIDE
from.

.. _STM32CubeIDE: https://www.st.com/en/development-tools/stm32cubeide.html
