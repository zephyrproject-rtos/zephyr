.. _stm32cube_ide:

STM32CubeIDE
############

STM32CubeIDE_ is designed for the STM32 series of chips from STMicroelectronics.

This guide describes the process of setting up, building, and debugging Zephyr
applications using the IDE.  Using the STM32CubeIDE allows you to build and
debug Zephyr applications as if they were created in the IDE using ST's
toolset.

A project must have already been created with Zephyr and west.

These directions have been validated to work with 1.16.0 of the IDE.

These directions have been adapted from a `GitHub discussion <https://github.com/zephyrproject-rtos/zephyr/discussions/69812#discussioncomment-8770986>`_.

Project Setup
*************

#. Setup a Zephyr workspace such that west is available.  This can be followed
   in the `Getting Started Guide <https://docs.zephyrproject.org/latest/getting_started/index.html>`_.
   You will need to install the SDK and make sure it is available.

#. Activate your environment.

#. Run STM32CubeIDE from your Zephyr environment.  Example:

   .. code-block::

      $ /opt/st/stm32cubeide_1.16.0/stm32cubeide

#. Open up your already existing project by going to
   :menuselection:`File->New->STM32 CMake Project`:

   .. image:: img/stm32cube_new_cmake.png
      :align: center

#. Select :menuselection:`Project with existing CMake sources`

#. Select :menuselection:`Next` and browse to where your sources exist.  The
   folder that you will open should have the ``CMakeLists.txt`` and ``prj.conf``
   in it.

#. Select :menuselection:`Next` and select the MCU that your project is using.
   Press :menuselection:`Finish` and your project should now be available,
   however more setup must still be done in order to configure it properly.

#. Right-click on the newly created project in the workspace, and select
   :menuselection:`Properties`

#. Go to the :menuselection:`C/C++ Properties` page and set the Generator
   to be ``Ninja``.  In the ``Other Options`` field, set your BOARD and
   BOARD_ROOT variables as you would when using west:

   .. image:: img/stm32cube_project_properties.png
      :align: center

   These options may or may not be needed depending on if you have an
   out of tree project or not.

#. Go to the :menuselection:`C/C++ General->Preprocessor Include` page.
   Add an include file that points at the ``autoconf.h`` that Zephyr uses.
   This will ensure that STM32CubeIDE picks up Zephyr ``#define`` s.

   .. image:: img/stm32cube_add_include.png
      :align: center

   .. image:: img/stm32cube_autoconf_h.png
      :align: center

#. Click :menuselection:`Apply and Close`

#. You may now build the project using the ``Build`` button on the toolbar.
   The project can be run using the ``Run`` button, as well as debugged
   using the ``Debug`` button.

Troubleshooting
***************

If when configuring you see an error that looks in part like:

.. code-block::

  Error message: Traceback (most recent call last):

    File "/path/to/zephyr/scripts/list_boards.py", line 11, in <module>
      import pykwalify.core

  ModuleNotFoundError: No module named 'pykwalify'


This means that you did not start the IDE in a Zephyr environment.  You must
delete the ``config_default`` build directory and start STM32CubeIDE again,
making sure that you can run ``west`` in the shell that you start STM32CubeIDE
in.

.. _STM32CubeIDE: https://www.st.com/en/development-tools/stm32cubeide.html
