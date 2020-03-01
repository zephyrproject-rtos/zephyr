.. _menuconfig:

Interactive Kconfig interfaces
##############################

There are two interactive configuration interfaces available for exploring the
available Kconfig options and making temporary changes: ``menuconfig`` and
``guiconfig``. ``menuconfig`` is a curses-based interface that runs in the
terminal, while ``guiconfig`` is a graphical configuration interface.

.. note::

   The configuration can also be changed by editing :file:`zephyr/.config` in
   the application build directory by hand. Using one of the configuration
   interfaces is often handier, as they correctly handle dependencies between
   configuration symbols.

   If you try to enable a symbol with unsatisfied dependencies in
   :file:`zephyr/.config`, the assignment will be ignored and overwritten when
   re-configuring.

To make a setting permanent, you should set it in a :file:`*.conf` file, as
described in :ref:`setting_configuration_values`.

.. tip::

   Saving a minimal configuration file (with e.g. :kbd:`D` in menuconfig) and
   inspecting it can be handy when making settings permanent. The minimal
   configuration file only lists symbols that differ from their default value.

To run one of the configuration interfaces, do this:

#. Build your application as usual using either ``west`` or ``cmake``:

   .. zephyr-app-commands::
      :tool: all
      :cd-into:
      :board: <board>
      :goals: build
      :compact:

#. To run the terminal-based ``menuconfig`` interface, use either of these
   commands:

   .. code-block:: bash

      west build -t menuconfig

   .. code-block:: bash

      ninja menuconfig

   To run the graphical ``guiconfig``, use either of these commands:

   .. code-block:: bash

      west build -t guiconfig

   .. code-block:: bash

      ninja guiconfig

   .. note::

      If you get an import error for ``tkinter`` when trying to run
      ``guiconfig``, you are missing required packages. See
      :ref:`installation_linux`. The package you need is usually called
      something like ``python3-tk``/``python3-tkinter``.

      ``tkinter`` is not included by default in many Python installations,
      despite being part of the standard library.

   The two interfaces are shown below:

   .. figure:: menuconfig.png
      :alt: menuconfig interface

   .. figure:: guiconfig.png
      :alt: guiconfig interface

   ``guiconfig`` always shows the help text and other information related to
   the currently selected item in the bottom window pane. In the terminal
   interface, press :kbd:`?` to view the same information.

   .. note::

      If you prefer to work in the ``guiconfig`` interface, then it's a good
      idea to check any changes to Kconfig files you make in *single-menu
      mode*, which is toggled via a checkbox at the top. Unlike full-tree
      mode, single-menu mode will distinguish between symbols defined with
      ``config`` and symbols defined with ``menuconfig``, showing you what
      things would look like in the ``menuconfig`` interface.

#. Change configuration values in the ``menuconfig`` interface as follows:

   * Navigate the menu with the arrow keys. Common `Vim
     <https://www.vim.org>`__ key bindings are supported as well.

   * Use :kbd:`Space` and :kbd:`Enter` to enter menus and toggle values. Menus
     appear with ``--->`` next to them. Press :kbd:`ESC` to return to the
     parent menu.

     Boolean configuration options are shown with :guilabel:`[ ]` brackets,
     while numeric and string-valued configuration symbols are shown with
     :guilabel:`( )` brackets. Symbol values that can't be changed are shown as
     :guilabel:`- -` or :guilabel:`-*-`.

     .. note::

        You can also press :kbd:`Y` or :kbd:`N` to set a boolean configuration
        symbol to the corresponding value.

   * Press :kbd:`?` to display information about the currently selected symbol,
     including its help text. Press :kbd:`ESC` or :kbd:`Q` to return from the
     information display to the menu.

   In the ``guiconfig`` interface, either click on the image next to the symbol
   to change its value, or double-click on the row with the symbol (this only
   works if the symbol has no children, as double-clicking a symbol with
   children open/closes its menu instead).

   ``guiconfig`` also supports keyboard controls, which are similar to
   ``menuconfig``.

#. Pressing :kbd:`Q` in the ``menuconfig`` interface will bring up the
   save-and-quit dialog (if there are changes to save):

   .. figure:: menuconfig-quit.png
      :alt: Save and Quit Dialog

   Press :kbd:`Y` to save the kernel configuration options to the default
   filename (:file:`zephyr/.config`). You will typically save to the default
   filename unless you are experimenting with different configurations.

   The ``guiconfig`` interface will also prompt for saving the configuration on
   exit if it has been modified.

   .. note::

      The configuration file used during the build is always
      :file:`zephyr/.config`. If you have another saved configuration that you
      want to build with, copy it to :file:`zephyr/.config`. Make sure to back
      up your original configuration file.

      Also note that filenames starting with ``.`` are not listed by ``ls`` by
      default on Linux and macOS. Use the ``-a`` flag to see them.

Finding a symbol in the menu tree and navigating to it can be tedious. To jump
directly to a symbol, press the :kbd:`/` key (this also works in
``guiconfig``). This brings up the following dialog, where you can search for
symbols by name and jump to them. In ``guiconfig``, you can also change symbol
values directly within the dialog.

.. figure:: menuconfig-jump-to.png
   :alt: menuconfig jump-to dialog

.. figure:: guiconfig-jump-to.png
   :alt: guiconfig jump-to dialog

If you jump to a symbol that isn't currently visible (e.g., due to having
unsatisfied dependencies), then *show-all mode* will be enabled. In show-all
mode, all symbols are displayed, including currently invisible symbols. To turn
off show-all mode, press :kbd:`A` in ``menuconfig`` or :kbd:`Ctrl-A` in
``guiconfig``.

.. note::

   Show-all mode can't be turned off if there are no visible items in the
   current menu.

To figure out why a symbol you jumped to isn't visible, inspect its
dependencies, either by pressing :kbd:`?` in ``menuconfig`` or in the
information pane at the bottom in ``guiconfig``. If you discover that the
symbol depends on another symbol that isn't enabled, you can jump to that
symbol in turn to see if it can be enabled.

.. note::

   In ``menuconfig``, you can press :kbd:`Ctrl-F` to view the help of the
   currently selected item in the jump-to dialog without leaving the dialog.

For more information on ``menuconfig`` and ``guiconfig``, see the Python
docstrings at the top of :zephyr_file:`menuconfig.py
<scripts/kconfig/menuconfig.py>` and :zephyr_file:`guiconfig.py
<scripts/kconfig/guiconfig.py>`.
