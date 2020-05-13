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

.. code-block:: console

   # Linux and macOS
   export MY_VARIABLE=foo

   # Windows
   set MY_VARIABLE=foo

.. warning::

  This is best for experimentation. If you close your terminal window, use
  another terminal window or tab, restart your computer, etc., this setting
  will be lost forever.

  Using options 2 or 3 is recommended if you want to keep using the setting.

Option 2: In all Terminals
--------------------------

**macOS and Linux**:

Add the ``export MY_VARIABLE=foo`` line to your shell's startup script in your
home directory. For Bash, this is usually :file:`~/.bashrc` on Linux or
:file:`~/.bash_profile` on macOS.  Changes in these startup scripts don't
affect shell instances already started; try opening a new terminal window to get
the new settings.

**Windows**:

You can use the ``setx`` program in ``cmd.exe`` or the third-party
RapidEE program.

To use ``setx``, type this command, then close the terminal window. Any new
``cmd.exe`` windows will have :envvar:`MY_VARIABLE` set to ``foo``.

.. code-block:: console

   setx MY_VARIABLE foo

To install RapidEE, a freeware graphical environment variable
editor, `using Chocolatey`_ in an Administrator command prompt:

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

**macOS and Linux**:

Create a file named :file:`~/.zephyrrc` if it doesn't exist, then add this line
to it:

.. code-block:: console

   export MY_VARIABLE=foo

To get this value back into your current terminal environment, **you must run**
``source zephyr-env.sh`` from the main ``zephyr`` repository. Among other
things, this script sources :file:`~/.zephyrrc`.

The value will be lost if you close the window, etc.; run ``source
zephyr-env.sh`` again to get it back.

**Windows**:

Add the line ``set MY_VARIABLE=foo`` to the file
:file:`%userprofile%\\zephyrrc.cmd` using a text editor such as Notepad to save
the value.

To get this value back into your current terminal environment, **you must run**
``zephyr-env.cmd`` in a ``cmd.exe`` window after changing directory to the main
``zephyr`` repository.  Among other things, this script runs
:file:`%userprofile%\\zephyrrc.cmd`.

The value will be lost if you close the window, etc.; run ``zephyr-env.cmd``
again to get it back.

Option 4: Using Zephyr Build Configuration CMake package
--------------------------------------------------------

Choose this option if you want to make those variable settings shared among all
users of your project.

Using a :ref:`cmake_build_config_package` allows you to commit the shared
settings into the repository, so that all users can share them.

It also removes the need for running ``source zephyr-env.sh`` or
``zephyr-env.cmd`` when opening a new terminal.

.. _zephyr-env:

Zephyr Environment Scripts
**************************

You can use the zephyr repository scripts ``zephyr-env.sh`` (for macOS and
Linux) and ``zephyr-env.cmd`` (for Windows) to load Zephyr-specific settings
into your current terminal's environment. To do so, run this command from the
zephyr repository::

  # macOS and Linux
  source zephyr-env.sh

  # Windows
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

Here are some important environment variables and what they contain. This is
not a comprehensive index to the environment variables which affect Zephyr's
behavior.

- :envvar:`BOARD`: allows set the board when building an application; see
  :ref:`important-build-vars`.
- :envvar:`CONF_FILE`: allows adding Kconfig fragments to an application build;
  see :ref:`important-build-vars`.
- :envvar:`DTC_OVERLAY_FILE`: allows adding devicetree overlays to an
  application build; see :ref:`important-build-vars`.
- :envvar:`ZEPHYR_BASE`: the absolute path to the main ``zephyr`` repository.
  This is set whenever you run the ``zephyr-env.sh`` or ``zephyr-env.cmd``
  scripts mentioned above.
- :envvar:`ZEPHYR_TOOLCHAIN_VARIANT`: the current :ref:`toolchain
  <gs_toolchain>` used to build Zephyr applications.

.. _using Chocolatey: https://chocolatey.org/packages/RapidEE
