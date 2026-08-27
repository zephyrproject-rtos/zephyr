The shell script run-sample-tests.sh runs selected Zephyr samples against
the network test applications Docker container provided by the 'net-tools'
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
 * cd docker/
 * Run 'docker build -t net-tools .'

This creates a Docker image called 'net-tools' which the script will need as
its counterpart when testing various network sample applications.


Using
*****

The scripts/net/run-sample-tests.sh shell script can be used in two ways:

1. From a Zephyr network sample test directory.
   Example:

   cd samples/net/ethernet/gptp
   $ZEPHYR_BASE/scripts/net/run-sample-tests.sh

2. By giving the test directories as parameters to the runner script.
   Example:

   cd $ZEPHYR_BASE
   ./scripts/net/run-sample-tests.sh samples/net/ethernet/gptp \
                                     samples/net/sockets/echo_server

User can see what samples are supported like this:

   $ZEPHYR_BASE/scripts/net/run-sample-tests.sh --scan

The Docker container and a corresponding 'net-tools0' Docker network is started
by the script, as well as Zephyr using native_sim board. IP addresses are
assigned to the Docker network, which is a Linux network bridge interface.
The default IP addresses are:

 * Zephyr uses addresses 192.0.2.1 and 2001:db8::1
 * Docker net-tools image uses addresses 192.0.2.2 and 2001:db8::2
 * The Docker bridge interface uses addresses 192.0.2.254 and 2001:db8::254

The default IP addresses are used by echo_client and mqtt_publisher, but
with the echo_server the IP addresses are switched between Zephyr and Docker
so that the echo_client application always uses addresses ending in .1 and
the echo_server application uses those ending in .2. The script does the IP
address setup for each sample test, be it the default ones or the switched
ones.

When completed, the return value, either from Zephyr or from the Docker
container, is returned to the script on Zephyr or Docker application
termination. The return value is used as a simple verdict whether the sample
passed or failed.


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
