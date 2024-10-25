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
   Press :menuselection:`Finish` and your project should now be available.
   However, more actions must still be done in order to configure it properly.

#. Right-click on the newly created project in the workspace, and select
   :menuselection:`Properties`

#. Go to the :menuselection:`C/C++ Properties` page and set the Generator
   to ``Ninja``.  In ``Other Options``, specify the target ``BOARD`` in
   CMake argument format. If an out-of-tree board is targeted, the ``BOARD_ROOT``
   option must also be set. The resulting settings page should look similar to this:

   .. image:: img/stm32cube_project_properties.png
      :align: center

   These options may or may not be needed depending on if you have an
   out of tree project or not.

#. Go to the :menuselection:`C/C++ General->Preprocessor Include` page.
   Select the :menuselection:`GNU C` language, and click on the
   :menuselection:`CDT User Settings Entires` option.

   .. image:: img/stm32cube_preprocessor_include.png
      :align: center

   Press the `Add` button to add an `Include File`
   that points to Zephyr's ``autoconf.h``, which is located in
   ``<build dir>/zephyr/include/generated/autoconf.h``. This will ensure
   that STM32CubeIDE picks up Zephyr ``#define`` s.
   The following dialog will be shown.  Fill it in as follows:

   .. image:: img/stm32cube_add_include.png
      :align: center

   Once the include file has been added, your properties page should look
   similar to the following:

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
