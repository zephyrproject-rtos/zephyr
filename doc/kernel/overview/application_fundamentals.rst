.. _application_fundamentals:

Application Fundamentals
########################

A Zephyr application is defined by creating a directory containing the
following files.

* Application source code files

  An application typically provides one or more application-specific files,
  written in C or assembly language. These files are usually located in a
  sub-directory called :file:`src`.

* Kernel configuration files

  An application typically provides a configuration file (:file:`.conf`)
  that specifies values for one or more kernel configuration options.
  If omitted, the application's existing kernel configuration option values
  are used; if no existing values are provided, the kernel's default
  configuration values are used.

  A microkernel application typically provides an additional microkernel
  definitions (MDEF) file (:file:`.mdef`) that defines all of the system's
  public microkernel objects. This file is not used with a nanokernel-only
  application.

* Makefile

  This file typically contains a handful of lines that tell the build system
  where to find the files mentioned above, as well as the desired target
  platform configuration and kernel type (either microkernel or nanokernel).

Once the application has been defined it can be built using a single command.
The results of the build process, including the final application image,
are located in a sub-directory called :file:`outdir`.

For additional information see:

* :ref:`application`
