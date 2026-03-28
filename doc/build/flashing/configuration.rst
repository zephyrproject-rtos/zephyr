.. _flashing-soc-board-config:

Flashing configuration
######################

Zephyr supports setting up configuration for flash runners (invoked from
:ref:`west flash<west-flashing>`) which allows for customising how commands are used when
programming boards. This configuration is used for :ref:`sysbuild` projects and allows for
configuring when commands are ran for groups of board targets. As an example: a multi-core SoC
might want to only allow the ``--erase`` argument to be used once for all of the cores in the SoC
which would prevent multiple erase tasks running in a single ``west flash`` invocation, which
could wrongly clear the memory which is used by the other images being programmed.

Priority
********

Flashing configuration is singular, it will only be read from a single location, this
configuration can reside in the following files starting with the highest priority:

 * ``soc.yml`` (in soc folder)
 * ``board.yml`` (in board folder)

Configuration
*************

Configuration is applied in the yml file by using a ``runners`` map with a single ``run_once``
child, this then contains a map of commands as they would be provided to the flash runner e.g.
``--reset`` followed by a list which specifies the settings for each of these commands (these
are grouped by flash runner, and by qualifiers/boards). Commands have associated runners that
they apply to using a ``runners`` list value, this can contain ``all`` if it applies to all
runners, otherwise must contain each runner that it applies to using the runner-specific name.
Groups of board targets can be specified using the ``groups`` key which has a list of board
target sets. Board targets are regular expression matches, for ``soc.yml`` files each set of
board targets must be in a ``qualifiers`` key (only regular expression matches for board
qualifiers are allowed, board names must be omitted from these entries). For ``board.yml``
files each set of board targets must be in a ``boards`` key, these are lists containing the
matches which form a singular group. A final parameter ``run`` can be set to ``first`` which
means that the command will be ran once with the first image flashing process per set of board
targets, or to ``last`` which will be ran once for the final image flash per set of board targets.

An example flashing configuration for a ``soc.yml`` is shown below in which the ``--recover``
command will only be used once for any board targets which used the nRF5340 SoC application or
network CPU cores, and will only reset the network or application core after all images for the
respective core have been flashed.

.. code-block:: yaml

  runners:
    run_once:
      '--recover':
        - run: first
          runners:
            - nrfjprog
          groups:
            - qualifiers:
                - nrf5340/cpunet
                - nrf5340/cpuapp
                - nrf5340/cpuapp/ns
      '--reset':
        - run: last
          runners:
            - nrfjprog
            - jlink
          groups:
            - qualifiers:
                - nrf5340/cpunet
            - qualifiers:
                - nrf5340/cpuapp
                - nrf5340/cpuapp/ns
        # Made up non-real world example to show how to specify different options for different
        # flash runners
        - run: first
          runners:
            - some_other_runner
          groups:
            - qualifiers:
                - nrf5340/cpunet
            - qualifiers:
                - nrf5340/cpuapp
                - nrf5340/cpuapp/ns

Usage
*****

Commands that are supported by flash runners can be used as normal when flashing non-sysbuild
applications, the run once configuration will not be used. When flashing a sysbuild project with
multiple images, the flash runner run once configuration will be applied.

For example, building the :zephyr:code-sample:`smp-svr` sample for the nrf5340dk which will
include MCUboot as a secondary image:

.. code-block:: console

   cmake -GNinja -Sshare/sysbuild/ -Bbuild -DBOARD=nrf5340dk/nrf5340/cpuapp -DAPP_DIR=samples/subsys/mgmt/mcumgr/smp_svr
   cmake --build build

Once built with an nrf5340dk connected, the following command can be used to flash the board with
both applications and will only perform a single device recovery operation when programming the
first image:

.. code-block:: console

   west flash --recover

If the above was ran without the flashing configuration, the recovery process would be ran twice
and the device would be unbootable.
