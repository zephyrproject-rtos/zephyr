.. _mac-setup-alts:

macOS alternative setup instructions
####################################

.. _mac-gatekeeper:

Important note about Gatekeeper
*******************************

Starting with macOS 10.15 Catalina, applications launched from the macOS
Terminal application (or any other terminal emulator) are subject to the same
system security policies that are applied to applications launched from the
Dock. This means that if you download executable binaries using a web browser,
macOS will not let you execute those from the Terminal by default. In order to
get around this issue you can take two different approaches:

* Run ``xattr -r -d com.apple.quarantine /path/to/folder`` where
  ``path/to/folder`` is the path to the enclosing folder where the executables
  you want to run are located.

* Open "System Preferences" -> "Security and Privacy" -> "Privacy" and then
  scroll down to "Developer Tools". Then unlock the lock to be able to make
  changes and check the checkbox corresponding to your terminal emulator of
  choice. This will apply to any executable being launched from such terminal
  program.

Note that this section does **not** apply to executables installed with
Homebrew, since those are automatically un-quarantined by ``brew`` itself. This
is however relevant for most :ref:`toolchains`.

.. _macOS Gatekeeper: https://en.wikipedia.org/wiki/Gatekeeper_(macOS)

Additional notes for MacPorts users
***********************************

While MacPorts is not officially supported in this guide, it is possible to use
MacPorts instead of Homebrew to get all the required dependencies on macOS.
Note also that you may need to install ``rust`` and ``cargo`` for the Python
dependencies to install correctly.
