The shell script run-sample-tests.sh runs a few Zephyr samples against the
network test applications container provided by the 'net-tools'
Zephyr project, https://github.com/zephyrproject-rtos/net-tools.


Installation
************

As a prerequisite it is assumed that Docker is installed and that the
'net-tools' Docker container has been created, see the first bullet point
at the net-tools Docker README file
https://github.com/zephyrproject-rtos/net-tools/blob/master/README.docker.
In essence, the following needs to be done:

 * Install Docker
 * Check out the net-tools project from github or update it with west
 * Change working directory to the net-tools repository
 * Run './net-setup.sh --config docker.conf'

This runs a Docker image called 'net-tools' and sets up docker network
'net-tools0' on your local machine.


Using
*****

The scripts/net/run-sample-tests.sh shell script is meant to be run from the
relevant Zephyr network sample test directory. Currently the following two
samples are supported:

 * samples/net/sockets/echo_client
 * samples/net/sockets/echo_server

The applications to run in the net-tools Docker container are selected based
on the name of the sample directory, for echo_client the echo_server
application is started from the Docker container, and with echo_server
echo_client is started in the Docker container. When completed, the return
value, either from Zephyr or from the Docker container, is returned to the
script on Zephyr or Docker application termination. The return value is used
as a simple verdict whether the sample passed or failed.

The Docker container and a corresponding 'net-tools0' Docker network is started
by the script, as well as Zephyr using native_posix. IP addresses are assigned
to the Docker network, which is a Linux network bridge interface. The IP
addresses are set based on the sample being run.

 * echo_client uses addresses 192.0.2.1 and 2001:db8::1
 * echo_server uses addresses 192.0.2.2 and 2001:db8::2
 * the Docker bridge interface uses addresses 192.0.2.254 and 2001:db8::254


Directories
***********

The sample test script tries to automatically figure out the Zephyr base
directory, which is assumed to be set by the ZEPHYR_BASE environment variable.
Should this not be the case, the directory can be set using '-Z' or
'--zephyr-dir' command line arguments. The sample test script also assumes
that the net-tools git repository containing the Docker networking setup file
'docker.conf' exists at the same directory level as the Zephyr base directory.
If the net tools are found elsewhere, the net-tools directory can be set with
the 'N' or '--net-tools-dir' command line argument.

Help is also available using the 'h' or '--help' argument.
