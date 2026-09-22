.. _workbench_for_zephyr:

Workbench for Zephyr
#########################################

Workbench for Zephyr is a Visual Studio Code (VS Code) extension that adds Zephyr
development support, including **SDK management**, **project creation wizard**, **build/flash**,
and **debugging**.

Check the `Getting started tutorial`_ for a walkthrough.

Key features
************

- Install native Host Tools (Python, CMake, ...)
- Install and assign toolchains (Zephyr SDK, IAR, ...)
- Import West workspaces
- Create and import Zephyr applications
- Build and flash applications
- Debug applications
- Install runners automatically
- Perform memory and static analysis

Compatibility
*************

- Windows 10-11
- Linux (x86_64)

  - Ubuntu
  - Debian
  - Fedora
  - Other distributions may also work (not tested)

- macOS

Getting started
***************

#. Install the extension

   Install `Workbench for Zephyr`_ from the VS Code Marketplace.

#. Open the Workbench for Zephyr extension

   In the Activity Bar, click the :guilabel:`Workbench for Zephyr` icon.

#. Install Host Tools

   Click :guilabel:`Install Host Tools` to download and install the native tools
   (typically into ``${HOME}/.zinstaller``).

   .. figure:: img/workbench_for_zephyr_install_host_tools.webp
      :align: center
      :alt: Installing Host Tools in Workbench for Zephyr

   .. note::

      Some host tools may require administrator privileges.
      On Windows, this is required when installing 7z.
      On Linux, this is required when installing tools using the package manager,
      for example when running :command:`apt install`.

#. Import a toolchain

   Click :guilabel:`Import Toolchain`, choose a toolchain, and select the destination folder.

   A toolchain provides the compiler and debugger required to build and debug Zephyr
   applications.
   The Zephyr SDK is the recommended option and can be installed as either the full
   package or a minimal target-specific version.
   Workbench also supports other toolchains, such as IAR.

#. Initialize / Import a West workspace

   Click :guilabel:`Initialize workspace` and fill the workspace information.

   .. figure:: img/workbench_for_zephyr_west_workspace.webp
      :align: center
      :width: 600px
      :alt: Initializing a West workspace in Workbench for Zephyr

   Workbench will create the workspace and parse the west manifest to configure projects.

#. Create a new application

   On Workbench for Zephyr, new projects are based on samples from Zephyr sources.

   - Click :guilabel:`Create New Application`.
   - Select the :guilabel:`West Workspace` to attach to.
   - Select the :guilabel:`Zephyr SDK` to use.
   - Select the target :guilabel:`Board` (for example, ``ST STM32F4 Discovery``).
   - Select the :guilabel:`Sample` project to base on (for example, ``hello_world``).
   - Enter the project name.
   - Enter the project location.

   .. figure:: img/workbench_for_zephyr_application.webp
      :align: center
      :width: 600px
      :alt: Creating a new application from a sample in Workbench for Zephyr

#. Build the application

   Click :guilabel:`Build` in the status bar, or select the application folder to build.
   Build output is shown in the integrated terminal.

#. Configure and run a debug session

   Use :guilabel:`Debug Manager` to generate/update a debug configuration
   (:file:`.vscode/launch.json`):

   - Generated ELF (Program Path)
   - SVD file (optional)
   - GDB/port/address (if needed)
   - Debug server/runner (OpenOCD, J-Link, LinkServer, pyOCD, ...)

   .. figure:: img/workbench_for_zephyr_debug_manager.webp
      :align: center
      :width: 600px
      :alt: Debug Manager in Workbench for Zephyr

   Then start debugging normally via :guilabel:`Run and Debug`

Install Runners
***************

Workbench for Zephyr can provide installers for some runners (for example, OpenOCD and
STM32CubeProgrammer). Use :guilabel:`Install Runners` to view supported tools and install them
(or open the vendor website).

Useful links
************

- You can explore the `Extension repository`_
- For more details, check out the `Full documentation`_

.. _Extension repository: https://github.com/Ac6Embedded/vscode-zephyr-workbench
.. _Full documentation: https://z-workbench.com/
.. _Workbench for Zephyr: https://marketplace.visualstudio.com/items?itemName=Ac6.zephyr-workbench
.. _Getting started tutorial: https://youtu.be/1RB0GI6rJk0?si=_D2AA3KurzCwLtRv
