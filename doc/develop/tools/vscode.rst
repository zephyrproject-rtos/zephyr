.. _vscode_ide:

Visual Studio Code
##################

`Visual Studio Code`_ (VS Code for short) is a popular cross-platform IDE that supports C projects
and has a rich set of extensions.

This guide describes the process of setting up VS Code for Zephyr's
:zephyr:code-sample:`blinky` sample in VS Code.

The instructions have been tested on Linux, but the steps should be the same for macOS and
Windows, just make sure to adjust the paths if needed.

Get VS Code
***********

`Download VS Code`_ and install it.

Install the required extensions through the `Extensions` marketplace in the left panel.
Search for the `C/C++ Extension Pack`_ and install it.

Initialize a new workspace
**************************

This guide gives details on how to configure the :zephyr:code-sample:`blinky`
sample application, but the instructions would be similar for any Zephyr project and :ref:`workspace
layout <west-workspaces>`.

Before you start, make sure you have a working Zephyr development environment, as per the
instructions in the :ref:`getting_started`.

Open the project in VS Code
***************************

#. In VS Code, select :menuselection:`File --> Open Folder` from the main menu.

#. Navigate to your Zephyr workspace and select it (i.e. the :file:`zephyrproject` folder in your
   HOME directory if you have followed the Getting Started instructions).

#. If prompted, enable workspace trust.

Generate compile commands
*************************

In order to support code navigation and linting capabilities, you must compile your project once to
generate the :file:`compile_commands.json` file that will provide the C/C++ extension with the
required information (ex. include paths):

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/blinky
      :board: native_sim/native/64
      :goals: build
      :compact:

Configure the C/C++ extension
*****************************

You'll now need to point to the generated :file:`compile_commands.json` file to enable linting and
code navigation in VS Code.

#. Go to the :menuselection:`File --> Preferences --> Settings` in the VS Code top menu.

#. Search for the parameter :guilabel:`C_Cpp > Default: Compile Commands` and set its value to:
   ``zephyr/build/compile_commands.json``.

   Linting errors in the code should now be resolved, and you should be able to navigate through the
   code.

Additional resources
********************

There are many other extensions that can be useful when working with Zephyr and VS Code. While this
guide does not cover them yet, you may refer to their documentation to set them up:

Contribution tooling
====================

- `Checkpatch Extension`_
- `EditorConfig Extension`_

Documentation languages extensions
==================================

- `reStructuredText Extension Pack`_

IDE extensions
==============

- `CMake Extension documentation`_
- `nRF Kconfig Extension`_
- `nRF DeviceTree Extension`_
- `GNU Linker Map files Extension`_

Additional guides
=================

- `How to Develop Zephyr Apps with a Modern, Visual IDE`_

.. note::

   Please be aware that these extensions might not all have the same level of quality and maintenance.

.. _Visual Studio Code: https://code.visualstudio.com/
.. _Download VS Code: https://code.visualstudio.com/Download
.. _VS Code documentation: https://code.visualstudio.com/docs
.. _C/C++ Extension Pack: https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack
.. _C/C++ Extension documentation: https://code.visualstudio.com/docs/languages/cpp
.. _CMake Extension documentation: https://code.visualstudio.com/docs/cpp/cmake-linux

.. _Checkpatch Extension: https://marketplace.visualstudio.com/items?itemName=idanp.checkpatch
.. _EditorConfig Extension: https://marketplace.visualstudio.com/items?itemName=EditorConfig.EditorConfig

.. _reStructuredText Extension Pack: https://marketplace.visualstudio.com/items?itemName=lextudio.restructuredtext-pack

.. _nRF Kconfig Extension: https://marketplace.visualstudio.com/items?itemName=nordic-semiconductor.nrf-kconfig
.. _nRF DeviceTree Extension: https://marketplace.visualstudio.com/items?itemName=nordic-semiconductor.nrf-devicetree
.. _GNU Linker Map files Extension: https://marketplace.visualstudio.com/items?itemName=trond-snekvik.gnu-mapfiles

.. _How to Develop Zephyr Apps with a Modern, Visual IDE: https://github.com/beriberikix/zephyr-vscode-example
