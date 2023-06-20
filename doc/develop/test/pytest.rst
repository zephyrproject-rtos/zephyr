.. integration-with-pytest:

Integration with pytest test framework
######################################

*Please mind that integration of twister with pytest is still work in progress. Not every platform
type is supported in pytest (yet). If you find any issue with the integration or have an idea for
an improvement, please, let us know about it and open a GitHub issue/enhancement.*

Introduction
************

Pytest is a python framework that *“makes it easy to write small, readable tests, and can scale to
support complex functional testing for applications and libraries”* (`<https://docs.pytest.org/en/7.3.x/>`_).
Python is known for its free libraries and ease of using it for scripting. In addition, pytest
utilizes the concept of plugins and fixtures, increasing its expendability and reusability.
A pytest plugin `pytest-twister-harness` was introduced to provide an integration between pytest
and twister, allowing Zephyr’s community to utilize pytest functionality with keeping twister as
the main framework.

Integration with twister
************************

By default, there is nothing to be done to enable pytest support in twister. The plugin is
developed as a part of Zephyr’s tree. To enable install-less operation, twister first extends
``PYTHONPATH`` with path to this plugin, and then during pytest call, it appends the command with
``-p twister_harness.plugin`` argument. If one prefers to use the installed version of the plugin,
they must add ``--allow-installed-plugin`` flag to twister’s call.

Pytest-based test suites are discovered the same way as other twister tests, i.e., by a presence
of testcase/sample.yaml. Inside, a keyword ``harness`` tells twister how to handle a given test.
In the case of ``harness: pytest``, most of twister workflow (test suites discovery,
parallelization, building and reporting) remains the same as for other harnesses. The change
happens during the execution step. The below picture presents a simplified overview of the
integration.

.. figure:: twister_and_pytest.svg
   :figclass: align-center


If ``harness: pytest`` is used, twister delegates the test execution to pytest, by calling it as
a subprocess. Required parameters (such as build directory, device to be used, etc.) are passed
through a CLI command. When pytest is done, twister looks for a pytest report (results.xml) and
sets the test result accordingly.

How to create a pytest test
***************************

An example of a pytest test is given at :zephyr_file:`samples/subsys/testsuite/pytest/shell/pytest/test_shell.py`.
Twister calls pytest for each configuration from the .yaml file which uses ``harness: pytest``.
By default, it points to ``pytest`` directory, located next to a directory with binary sources.
A keyword ``pytest_root`` placed under ``harness_config`` section can be used to point to another
location.

Pytest scans the given folder looking for tests, following its default
`discovery rules <https://docs.pytest.org/en/7.1.x/explanation/goodpractices.html#conventions-for-python-test-discovery>`_
One can also pass some extra arguments to the pytest from yaml file using ``pytest_args`` keyword
under ``harness_config``, e.g.: ``pytest_args: [‘-k=test_method’, ‘--log-level=DEBUG’]``.

Following import is required to include in .py sources:

.. code-block:: python

   from twister_harness import Device

It is important for type checking and enabling IDE hints for ``dut``s (objects representing
Devices Under Test). The ``dut`` fixture is the core of pytest harness plugin. When used as an
argument of a test function it gives access to a DeviceAbstract type object. The fixture yields a
device prepared according to the requested type (native posix, qemu, hardware, etc.). All types of
devices share the same API. This allows for writing tests which are device-type-agnostic.


Limitations
***********

* The whole pytest call is reported as one test in the final twister report (xml or json).
* Device adapters in pytest plugin provide `iter_stdout` method to read from devices. In some
  cases, it is not the most convenient way, and it will be considered how to improve this
  (for example replace it with a simple read function with a given byte size and timeout arguments).
* Not every platform type is supported in the plugin (yet).
