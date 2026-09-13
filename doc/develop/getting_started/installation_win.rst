.. _win-setup-alts:

Windows alternative setup instructions
######################################

.. _win-wsl:

Windows 10 WSL (Windows Subsystem for Linux)
********************************************

If you are running a recent version of Windows 10 you can make use of the
built-in functionality to natively run Ubuntu binaries directly on a standard
command-prompt. This allows you to use software such as the :ref:`Zephyr SDK
<toolchain_zephyr_sdk>` without setting up a virtual machine.

.. warning::
      Windows 10 version 1803 has an issue that will cause CMake to not work
      properly and is fixed in version 1809 (and later).
      More information can be found in :github:`Zephyr Issue 10420 <10420>`.

#. `Install the Windows Subsystem for Linux (WSL)`_.

   .. note::
         For the Zephyr SDK to function properly you will need Windows 10
         build 15002 or greater. You can check which Windows 10 build you are
         running in the "About your PC" section of the System Settings.
         If you are running an older Windows 10 build you might need to install
         the Creator's Update.

#. Follow the Ubuntu instructions in the :ref:`installation_linux` document.

.. NOTE FOR DOCS AUTHORS: as a reminder, do *NOT* put dependencies for building
   the documentation itself here.

.. _Install the Windows Subsystem for Linux (WSL): https://msdn.microsoft.com/en-us/commandline/wsl/install_guide

Using USB in WSL2 (Troubleshooting)
===================================

Out of the box, WSL2 can't see physical USB devices plugged into your Windows machine. This means you won't be able to flash your board right away. You can easily fix this by using a tool called ``usbipd-win`` to share the USB port from Windows over to Linux.

1. Install usbipd on Windows:
   Open PowerShell as Administrator on your Windows side and install the tool:

   .. code-block:: console

      winget install --interactive --exact dorssel.usbipd-win

2. Attach your board to WSL:
   Plug in your board and run these commands in Windows PowerShell to find it and share it:

   .. code-block:: console

      usbipd list
      usbipd bind --busid <BUSID>
      usbipd attach --wsl --busid <BUSID>

3. Fixing WSL Permissions:
   Even after attaching the USB, Linux might give you a "Permission Denied" error (like on ``/dev/ttyUSB0``) when you actually try to flash. To fix this, just add your WSL user to the dialout group:

   .. code-block:: console

      sudo usermod -aG dialout $USER

   Make sure to completely close and reopen your WSL terminal after running this so the permissions take effect!
