.. _apps_dev_process:

Application Development Workflow
################################

The application development workflow identifies procedures needed to create, build, and
run a |codename| microkernel or nanokernel application.

Before you build
----------------

* Check that your Linux host meets the minimum requirements specified in the |codename|
  :ref:`quick_start`.

* Check that environment variables have been configured correctly as outlined
  in :ref:`apps_common_procedures`.

Workflow
--------

1. Create a directory structure for your Zephyr application

   a) :ref:`Creating an Application and Source Code Directory using the
      CLI`

  and add a Makefile

   b) :ref:`Creating an Application Makefile`

2. Define the application's default kernel configuration using
   :ref:`Defining the Application's Default Kernel Configuration`.

3. Define kernel configuration override options for the application
   using :ref:`Overriding the Application's Default Kernel
   Configuration`.

4. For a microkernel application, define objects as you develop code
   using :ref:`Creating and Configuring an MDEF File for a Microkernel
   Application`.

5. For all applications, define nanokernel objects as you need them in
   code.

6. Develop source code and add source code files to the src directory.

   * :ref:`Understanding Naming Conventions`
   * :ref:`Understanding src Directory Makefile Requirements`
   * :ref:`Adding Source Files and Makefiles to src Directories`

7. Build an application image.

   * :ref:`Building an Application`
   * :ref:`Rebuilding an Application`

8. To test the application image's functionality on simulated hardware
   with QEMU, see :ref:`Running an Application`

9. To load an application image on a target hardware, see  using :ref:`Platform`
   documentation.
