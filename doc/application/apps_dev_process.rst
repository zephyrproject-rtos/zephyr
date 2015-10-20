.. _apps_dev_process:

Application Development Workflow
################################

The application development workflow identifies procedures needed to create, build, and
run a Zephyr microkernel or nanokernel application.

Before you build
----------------

* Check that your Linux host meets the minimum requirements specified in the
  :ref:`quick_start`.

* Check that environment variables have been configured correctly as outlined
  in :ref:`apps_common_procedures`.

Workflow
--------

1. Create a directory structure for your Zephyr application.

   a) :ref:`create_directory_structure`

2. Add a Makefile

   b) :ref:`create_src_makefile`

3. Define the application's default kernel configuration using
   :ref:`Defining the Application's Default Kernel Configuration`.

4. Define kernel configuration override options for the application
   using :ref:`Overriding the Application's Default Kernel
   Configuration`.

5. For a microkernel application, define objects as you develop code
   using :ref:`Creating and Configuring an MDEF File for a Microkernel
   Application`.

6. For all applications, define nanokernel objects as you need them in
   code.

7. Develop source code and add source code files to the src directory.

   * :ref:`Understanding Naming Conventions`
   * :ref:`Understanding src Directory Makefile Requirements`
   * :ref:`Adding Source Files and Makefiles to src Directories`

8. Build an application image.

   * :ref:`Building an Application`
   * :ref:`Rebuilding an Application`

9. To test the application image's functionality on simulated hardware
   with QEMU, see :ref:`Running an Application`.

10. To load an application image on a target hardware, see using
    :ref:`Platform` documentation.
