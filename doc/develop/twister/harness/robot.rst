.. _twister_robot_harness:

Robot
#####
Zephyr supports `Robot Framework <https://robotframework.org/>`_ as one of
solutions for automated testing.

Robot files allow you to express interactive test scenarios in human-readable
text format and execute them in simulation or against hardware.  At this moment
Zephyr integration supports running Robot tests in the `Renode
<https://renode.io/>`_ simulation framework, QEMU and Native Simulator.

To execute a Robot test suite with twister, run the following command:

.. code-block:: console

   $ west twister --platform hifive1 --test samples/subsys/shell/shell_module/sample.shell.shell_module.robot

Writing Robot tests
===================

For the list of keywords provided by the Robot Framework itself, refer to `the
official Robot documentation <https://robotframework.org/robotframework/>`_.

Information on writing and running Robot Framework tests in Renode can be found
in `the testing section
<https://renode.readthedocs.io/en/latest/introduction/testing.html>`_ of Renode
documentation.  It provides a list of the most commonly used keywords together
with links to the source code where those are defined.

It's possible to extend the framework by adding new keywords expressed directly
in Robot test suite files, as an external Python library or, like Renode does
it, dynamically via XML-RPC.  For details see the `extending Robot Framework
<https://robotframework.org/robotframework/latest/RobotFrameworkUserGuide.html#extending-robot-framework>`_
section in the official Robot documentation.

Running a single testsuite
==========================

To run a single testsuite instead of a whole group of test you can run:

.. code-block:: bash

   $ west twister -p qemu_riscv32 -s arch.shared_interrupt

The ``robot`` harness is used to execute Robot Framework test suites
in simulation target (Qemu, Native Simulator, Renode).

robot_testsuite: <robot file path> (default empty)
    Specify one or more paths to a file containing a Robot Framework test suite
    to be run.

robot_option: <robot option> (default empty)
    One or more options to be send to robotframework.
