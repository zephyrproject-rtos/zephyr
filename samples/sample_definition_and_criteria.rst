.. _definition_and_criteria:

Sample Definition and Criteria
##############################

Definition
==========

A sample is a concise Zephyr application that provides an accessible overview of one or
more features, subsystems, or modules. This includes kernel services, drivers, protocols, etc.
Samples should be limited in scope and should focus only on demonstrating non-trivial or
essential aspects of the subsystem(s) or module(s) being highlighted.

Sample Criteria
===============

1. Samples must not be used as a testcase.
++++++++++++++++++++++++++++++++++++++++++
  * The primary purpose of a sample is to provide a reference to the user.
  * Samples must not use Zephyr's testing framework.

    * Must not use :kconfig:option:`CONFIG_ZTEST` or :kconfig:option:`CONFIG_ZTEST_NEW_API`.
    * Must not use zasserts in samples.

  * If a sample can provide output that can be verified, then output should be evaluated against
    expected value to have some level of testing for the sample itself.
    Refer to :ref:`twister_script` for more details.
  * Samples are optional.

2. Twister should be able to build every sample.
++++++++++++++++++++++++++++++++++++++++++++++++
  * Every sample must have a YAML file. Reference: :ref:`twister_script`.

    **For example:**

    .. code-block:: yaml

      tests:
        sample.kernel.cond_var:
          integration_platforms:
            - native_posix
          tags: kernel condition_variables
          harness: console
          harness_config:
            type: one_line
            regex:
              - ".*Waited and joined with 3 threads"

  **Guidelines for Samples:**
    * Minimize the use of ``platform_allow`` and architecture filters.
    * Do not mark the test as build only unless necessary.
    * Any test case-specific configuration options are added using ``extra_args`` or
      ``extra_configs`` options in the YAML file. The `prj.conf` file should have the in-common
      configuration options.
    * The sample itself can be evaluated using multiple configurations in the sample's YAML file.
      For example, the sample is used to run with different logging modes using multiple scenarios
      in ``samples/subsys/logging/syst``.
    * Sample output can be validated leveraging the ``harness_config`` regex option,
      wherever applicable.

3. Samples should target multiple platforms and architectures.
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  * Minimize the use of ``platform_allow`` and architecture filters as it limits the scope
    of testing to the mentioned platforms and architectures.
    Reference: :ref:`twister_script`
  * Make use of ``integration_platforms`` to limit the scope when there are timing or
    resource constraints.
  * Make the sample as generic as possible. Avoid making a sample platform specific unless it is
    for particular hardware.

4. A sample should provide sufficient coverage of a subsystem, feature, or module.
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  **DO's:**
    * Cover the most common and important use cases of the functionality.
    * Keep the code simple and easy to read. Example: :zephyr_file:`samples/philosophers`.

  **DONT's:**
    * Samples must not test the negative or edge case behaviors.
    * Must not be unit tests.

5. Samples must be documented.
++++++++++++++++++++++++++++++
  * Samples must have a ``README.rst`` file in the samples folder.
    Example: ``samples/subsys/foo/README.rst``. clearly explaining the purpose of the sample, its
    HW requirements, and the expected sample output, if applicable.
  * Ensure that the ``README.rst`` file is accessible in the sample hierarchy starting at
    ``samples/index.rst``.

  **README Template:**
    * Overview, if applicable.
    * SW/HW requirements
    * Building & Running instructions
    * Sample output, if applicable.


As a starting point, this sample is a good example to refer to
:zephyr_file:`samples/kernel/condition_variables/condvar`.
