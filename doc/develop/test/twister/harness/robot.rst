.. _twister_robot_harness:

Robot
#####

The ``robot`` harness is used to execute Robot Framework test suites
in simulation target (Qemu, Native Simulator, Renode).

robot_testsuite: <robot file path> (default empty)
    Specify one or more paths to a file containing a Robot Framework test suite to be run.

robot_option: <robot option> (default empty)
    One or more options to be send to robotframework.
