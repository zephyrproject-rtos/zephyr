.. integration-with-robot:

Integration with robot framework
################################

*Please mind that integration of twister with robot is still work in progress. Not every platform
type is supported in robot (yet). If you find any issue with the integration or have an idea for
an improvement, please, let us know about it and open a GitHub issue/enhancement.*

.. note::

   This section of the documentation describes the integration of generic Python Robot Framework
   library -- currently called `robotframework` -- in twister; in contrast to the specific renode
   harness which is called just `robot`.

Introduction
************

*“Robot Framework is a generic open source automation framework. It can be used for test automation
and robotic process automation (RPA).“* (`<https://robotframework.org/>`_).
The focus of Robot Framework is especially on acceptance tests.

The basic behaviour and implementation is similar to the pytest implementation.

Integration with twister
************************

By default, there is nothing to be done to enable robot support in twister. The plugin is
developed as a part of Zephyr’s tree. To enable install-less operation, twister first extends
``PYTHONPATH`` with path to this plugin. If one prefers to use the installed version of the plugin,
they must add ``--allow-installed-plugin`` flag to twister’s call.

Robot Framework test are discovered the same way as other twister tests, i.e., by a presence
of testcase.yaml or sample.yaml. Inside, a keyword ``harness`` tells twister how to handle a given test.
In the case of ``harness: robotframework``, most of twister’s workflow (test suites discovery,
parallelization, building and reporting) remains the same as for other harnesses. The change
happens during the execution step.

If ``harness: robotframework`` is used, twister calls a robot_wrapper entrypoint in `pytest-twister-harness` that
handles proper argument parsing from twister (e.g., baudrate, serial device) and calls the
Python3 Robot Framework library afterwards.
This is needed to not replicate all argument parsing from pytest and robot, but having all configuration
available in the robot execution later.
After test execution, twister searches for a XUnit report file (robot_report.xml) containing test results, and
set its own results accordingly.


How to create a robot test
**************************

An example of a robot test is given at :zephyr_file:`samples/subsys/testsuite/robotframework/shell/robot/shell_module.robot`.
Twister calls robot for each configuration from the .yaml file which uses ``harness: robotframework``.
By default, it points to ``robot`` directory, located next to a directory with binary sources.
A keyword ``robot_test_path`` placed under ``harness_config`` section can be used to point to other
files, directories or subtests.

One can also pass some extra arguments to robot from yaml file using ``robot_args`` keyword
under ``harness_config``, e.g.: ``robot_args: [‘-t=test_method’, ‘-t=test_method’]``.
There is also an option to pass ``--robot-args`` through Twister command line parameters.
This can be particularly useful when one wants to select a specific testcase from a test suite.
For instance, one can use a command:

.. code-block:: console

   $ ./scripts/twister -p qemu_cortex_m3 --device-testing \
   -T samples/subsys/testsuite/robotframework/shell/ \
   --robot-args="--SuiteStatLevel 2 --Metadata Version:3"


Note that ``--robot-args`` can be passed multiple times to pass several arguments to the robot framework.

Robot keywords
==============

Besides some common keywords provided by Robot Framework themselves, there is a
'ZephyrLibrary' that can be loaded and used in the Robot test files.

.. code-block:: robotframework
   *** Settings ***
   Library          twister_harness.robot_framework.ZephyrLibrary

This library provides additional, Zephyr specific keywords and phrases that
can be used to handle interaction with the device.


Get a Device
------------

Give access to a ``DeviceAdapter`` type object, that represents Device Under Test.
This is the core of the following interaction.
It is required to launch and interact with other keywords.
It yields a device prepared according to the requested type
(``native``, ``qemu``, ``hardware``, etc.). All types of devices share the same API.
This allows for writing tests which are device-type-agnostic.


Run Device
----------

This keyword flashes and launches a device that was previously acquired by 'Get a Device'.
Otherwise, the Device will keep its previous state.


Run Command    <command>
------------------------

This connects to the running device or emulator via a serial console and executes
the given command ``<command>`` via shell. Note that the target must be
compiled with ``CONFIG_SHELL=y``.
It waits for a proper response of the ``wait_for_prompt`` method, so it does not
start the scenario until the DUT is ready.


Close Device
------------

This closes the serial connection to the device.


Limitations
***********

* Not every platform type is supported in the plugin (yet).
