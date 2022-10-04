.. _env_vars:

Environment Variables
=====================

Various pages in this documentation refer to setting Zephyr-specific
environment variables. This page describes how.

Setting Variables
*****************

Option 1: Just Once
-------------------

To set the environment variable :envvar:`MY_VARIABLE` to ``foo`` for the
lifetime of your current terminal window:

.. tabs::

   .. group-tab:: Linux/macOS

      .. code-block:: console

         export MY_VARIABLE=foo

   .. group-tab:: Windows

      .. code-block:: console

         set MY_VARIABLE=foo

.. warning::

  This is best for experimentation. If you close your terminal window, use
  another terminal window or tab, restart your computer, etc., this setting
  will be lost forever.

  Using options 2 or 3 is recommended if you want to keep using the setting.

Option 2: In all Terminals
--------------------------

.. tabs::

   .. group-tab:: Linux/macOS

      Add the ``export MY_VARIABLE=foo`` line to your shell's startup script in
      your home directory. For Bash, this is usually :file:`~/.bashrc` on Linux
      or :file:`~/.bash_profile` on macOS.  Changes in these startup scripts
      don't affect shell instances already started; try opening a new terminal
      window to get the new settings.

   .. group-tab:: Windows

      You can use the ``setx`` program in ``cmd.exe`` or the third-party RapidEE
      program.

      To use ``setx``, type this command, then close the terminal window. Any
      new ``cmd.exe`` windows will have :envvar:`MY_VARIABLE` set to ``foo``.

      .. code-block:: console

         setx MY_VARIABLE foo

      To install RapidEE, a freeware graphical environment variable editor,
      `using Chocolatey`_ in an Administrator command prompt:

      .. code-block:: console

         choco install rapidee

      You can then run ``rapidee`` from your terminal to launch the program and set
      environment variables. Make sure to use the "User" environment variables area
      -- otherwise, you have to run RapidEE as administrator. Also make sure to save
      your changes by clicking the Save button at top left before exiting.Settings
      you make in RapidEE will be available whenever you open a new terminal window.

.. _env_vars_zephyrrc:

Option 3: Using ``zephyrrc`` files
----------------------------------

Choose this option if you don't want to make the variable's setting available
to all of your terminals, but still want to save the value for loading into
your environment when you are using Zephyr.

.. tabs::

   .. group-tab:: Linux/macOS

      Create a file named :file:`~/.zephyrrc` if it doesn't exist, then add this
      line to it:

      .. code-block:: console

         export MY_VARIABLE=foo

      To get this value back into your current terminal environment, **you must
      run** ``source zephyr-env.sh`` from the main ``zephyr`` repository. Among
      other things, this script sources :file:`~/.zephyrrc`.

      The value will be lost if you close the window, etc.; run ``source
      zephyr-env.sh`` again to get it back.

   .. group-tab:: Windows

      Add the line ``set MY_VARIABLE=foo`` to the file
      :file:`%userprofile%\\zephyrrc.cmd` using a text editor such as Notepad to
      save the value.

      To get this value back into your current terminal environment, **you must
      run** ``zephyr-env.cmd`` in a ``cmd.exe`` window after changing directory
      to the main ``zephyr`` repository.  Among other things, this script runs
      :file:`%userprofile%\\zephyrrc.cmd`.

      The value will be lost if you close the window, etc.; run
      ``zephyr-env.cmd`` again to get it back.

      These scripts:

      - set :envvar:`ZEPHYR_BASE` (see below) to the location of the zephyr
        repository
      - adds some Zephyr-specific locations (such as zephyr's :file:`scripts`
        directory) to your :envvar:`PATH` environment variable
      - loads any settings from the ``zephyrrc`` files described above in
        :ref:`env_vars_zephyrrc`.

      You can thus use them any time you need any of these settings.

.. _zephyr-env:

Zephyr Environment Scripts
**************************

You can use the zephyr repository scripts ``zephyr-env.sh`` (for macOS and
Linux) and ``zephyr-env.cmd`` (for Windows) to load Zephyr-specific settings
into your current terminal's environment. To do so, run this command from the
zephyr repository:

.. tabs::

   .. group-tab:: Linux/macOS

      .. code-block:: console

         source zephyr-env.sh

   .. group-tab:: Windows

      .. code-block:: console

         zephyr-env.cmd

These scripts:

- set :envvar:`ZEPHYR_BASE` (see below) to the location of the zephyr
  repository
- adds some Zephyr-specific locations (such as zephyr's :file:`scripts`
  directory) to your :envvar:`PATH` environment variable
- loads any settings from the ``zephyrrc`` files described above in
  :ref:`env_vars_zephyrrc`.

You can thus use them any time you need any of these settings.

.. _env_vars_important:

Important Environment Variables
*******************************

Some :ref:`important-build-vars` can also be set in the environment. Here
is a description of some of these important environment variables. This is not
a comprehensive list.

- :envvar:`BOARD`
- :envvar:`CONF_FILE`
- :envvar:`SHIELD`
- :envvar:`ZEPHYR_BASE`
- :envvar:`ZEPHYR_EXTRA_MODULES`
- :envvar:`ZEPHYR_MODULES`

The following additional environment variables are significant when configuring
the :ref:`toolchain <gs_toolchain>` used to build Zephyr applications.

- :envvar:`ZEPHYR_TOOLCHAIN_VARIANT`: the name of the toolchain to use
- :envvar:`<TOOLCHAIN>_TOOLCHAIN_PATH`: path to the toolchain specified by
  :envvar:`ZEPHYR_TOOLCHAIN_VARIANT`. For example, if
  ``ZEPHYR_TOOLCHAIN_VARIANT=llvm``, use :envvar:`LLVM_TOOLCHAIN_PATH`. (Note
  the capitalization when forming the environment variable name.)

You might need to update some of these variables when you
:ref:`update the Zephyr SDK toolchain <gs_toolchain_update>`.

Emulators and boards may also depend on additional programs. The build system
will try to locate those programs automatically, but may rely on additional
CMake or environment variables to do so. Please consult your emulator's or
board's documentation for more information.

.. _using Chocolatey: https://chocolatey.org/packages/RapidEE
