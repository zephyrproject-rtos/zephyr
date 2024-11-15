.. _definition_and_criteria:

Sample Definition and Criteria
##############################

Definition
==========

A sample is a concise Zephyr application that provides an accessible overview of one or
more features, subsystems, or modules. This includes kernel services, drivers, protocols, etc.
Samples should be limited in scope and should focus only on demonstrating non-trivial or
essential aspects of the subsystem(s) or module(s) being highlighted.

Samples are recommended when submitting new features; however, they are not mandatory.

Sample Criteria
===============

1. Samples must not be used to test features or verify platforms
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  * The primary purpose of a sample is to provide a reference to the user.
  * Samples must not use Zephyr's testing framework.

    * Must not use :kconfig:option:`CONFIG_ZTEST`
    * Must not use zasserts in samples.

  * If a sample can provide output that can be verified, then output should be evaluated against
    expected value to have some level of testing for the sample itself.
    Refer to :ref:`twister_script` for more details.

  * Although being able to run a sample successfully does verify the
    functionality is working as expected, this should not replace dedicated and
    comprehensive tests with full coverage to be submitted into the
    :zephyr_file:`tests/` folder.  In a sample, the only thing you test is
    buildability and, in some cases, if a sample is performing as expected, i.e. you
    are testing the sample, not the subsystem it builds on top.

2. Twister should be able to build every sample.
++++++++++++++++++++++++++++++++++++++++++++++++
  * Every sample must have a YAML file. Reference: :ref:`twister_script`.

    **For example:**

    .. code-block:: yaml

      tests:
        sample.kernel.cond_var:
          integration_platforms:
            - native_sim
          tags:
            - kernel
            - condition_variables
          harness: console
          harness_config:
            type: one_line
            regex:
              - ".*Waited and joined with 3 threads"

  * Do not mark the test as build only. A sample should be able to build *and*
    run on the documented platforms. Use the ``harness`` option to skip the
    execution. Twister only attempts to flash and execute the build binary if
    the harness is set to ``ztest`` or ``console``.
  * The default configuration for the sample must be in the :file:`prj.conf`
    file and should be operational on all supported platforms. Do not rely on the
    ``extra_args`` or ``extra_configs`` options in the YAML file to build the
    sample.
  * The tests should verify the default configuration of the sample and any
    optional features documented in the sample's README file. Sample should
    never be used to test functionality of the subsystem or module the sample
    belongs to.
  * Any documented configuration options related to the sample and its
    operation can be verified using ``extra_args`` or
    ``extra_configs`` options in the YAML file. The :file:`prj.conf` file should have the
    base configuration options.
  * Sample output can be validated by leveraging the ``harness_config`` regex option,
    wherever applicable.
  * Use ``platform_allow`` to limit test to the platforms the sample was actually
    verified on. Those platforms should be listed in the sample's README file.
  * Make use of ``integration_platforms`` to limit the scope when there are timing or
    resource constraints. Ideally, one platform should be enough to verify the
    sample when changes are submitted to the Zephyr tree via a pull-request.
  * Make the sample as generic as possible. Avoid making a sample platform specific unless it is
    for particular hardware.

3. A sample should provide sufficient coverage of a subsystem, feature, or module.
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  **DO's:**
    * Cover the most common and important use cases of the functionality.
    * Keep the code simple and easy to read. Example: :zephyr_file:`samples/philosophers`.

  **DONT's:**
    * Samples must not test the negative or edge case behaviors.
    * Must not be unit tests.

4. Samples must be documented.
++++++++++++++++++++++++++++++
  * Samples must have a ``README.rst`` file in the samples folder.
    Example: ``samples/subsys/foo/README.rst``. clearly explaining the purpose of the sample, its
    hardware requirements, and the expected sample output, if applicable.
  * Ensure that the ``README.rst`` file is accessible in the sample hierarchy starting at
    ``samples/index.rst``.

  **README Template:**
    * Overview, if applicable.
    * Software/Hardware requirements
    * Building & Running instructions
    * Sample output, if applicable.


As a starting point, this sample is a good example to refer to
:zephyr_file:`samples/kernel/condition_variables/condvar`.
