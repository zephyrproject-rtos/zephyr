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
A keyword ``pytest_root`` placed under ``harness_config`` section can be used to point to other
files, directories or subtests.

Pytest scans the given locations looking for tests, following its default
`discovery rules <https://docs.pytest.org/en/7.1.x/explanation/goodpractices.html#conventions-for-python-test-discovery>`_
One can also pass some extra arguments to the pytest from yaml file using ``pytest_args`` keyword
under ``harness_config``, e.g.: ``pytest_args: [‘-k=test_method’, ‘--log-level=DEBUG’]``.

Helpers & fixtures
==================

dut
---

Give access to a DeviceAdapter type object, that represents Device Under Test.
This fixture is the core of pytest harness plugin. It is required to launch
DUT (initialize logging, flash device, connect serial etc).
This fixture yields a device prepared according to the requested type
(native posix, qemu, hardware, etc.). All types of devices share the same API.
This allows for writing tests which are device-type-agnostic.

.. code-block:: python

   from twister_harness import DeviceAdapter

   def test_sample(dut: DeviceAdapter):
      dut.readlines_until('Hello world')

shell
-----

Provide an object with methods used to interact with shell application.
It calls `wait_for_promt` method, to not start scenario until DUT is ready.
Note that it uses `dut` fixture, so `dut` can be skipped when `shell` is used.

.. code-block:: python

   from twister_harness import Shell

   def test_shell(shell: Shell):
      shell.exec_command('help')

mcumgr
------

Sample fixture to wrap ``mcumgr`` command-line tool used to manage remote devices.
More information about MCUmgr can be found here :ref:`mcu_mgr`.

.. note::
   This fixture requires the ``mcumgr`` available in the system PATH

Only selected functionality of MCUmgr is wrapped by this fixture.
For example, here is a test with a fixture ``mcumgr``

.. code-block:: python

   from twister_harness import DeviceAdapter, Shell, McuMgr

   def test_upgrade(dut: DeviceAdapter, shell: Shell, mcumgr: McuMgr):
      # free the serial port for mcumgr
      dut.disconnect()
      # upload the signed image
      mcumgr.image_upload('path/to/zephyr.signed.bin')
      # obtain the hash of uploaded image from the device
      second_hash = mcumgr.get_hash_to_test()
      # test a new upgrade image
      mcumgr.image_test(second_hash)
      # reset the device remotely
      mcumgr.reset_device()
      # continue test scenario, check version etc.

Limitations
***********

* Not every platform type is supported in the plugin (yet).
