.. _vanes:

Test Setups and Vanes
#####################

A driver test often needs more than a driver: two pins wired to each other, a
loopback on a bus, a known voltage on an ADC input. Which pins, which bus and
which input is a fact about the board, not about the test. A *test setup*
names such a requirement once, and a *vane* is the board-owned metadata that
adapts it to a particular piece of hardware.

Test setups
***********

A test setup is defined in :file:`tests/vanes/<name>.yml`. Its name is the
file name, ``<area>.<what>`` by convention:

.. code-block:: yaml

   # tests/vanes/counter.capture.yml
   description: |
     Counter capture inputs a test can trigger, listed in a test-counter-capture
     node, each fed by a signal the board generates in the same order: a PWM
     output of the zephyr,user node routed to the input inside the SoC, or a
     GPIO of its capture-tester-gpios wired to the input on the board.
   compatible: test-counter-capture

``description``
  What a board has to provide, for whoever writes a vane.

``compatible``
  The devicetree node a vane has to enable. The build fails with a message
  naming the setup and the board when a requested vane does not enable one.
  The binding of the node belongs to the test that reads it, under its
  :file:`dts/bindings/` directory.

``filter``
  A twister filter expression (see :ref:`twister_dt_filter_expressions`) the
  build has to satisfy, for a setup with no node of its own to check. A
  board whose own devicetree describes the setup, ADC channels for instance,
  then needs no vane: twister keeps such platforms and the filter decides,
  and a SoC's vane may describe channels of a controller a board leaves
  disabled without enrolling that board.

A test scenario states the setups it needs in its ``requires`` list (see
:ref:`twister_requires`) and reads the devicetree the vane provides; it
names no board. The fixture a run on hardware needs, as listed in the
hardware map (see :ref:`twister_fixtures`), stays with the scenario, where
it tells a tester that the setup is wired on the board.

Vanes
*****

A board realizes a setup in :file:`vanes/<setup>/` under its own directory,
with devicetree overlays and Kconfig fragments named like the :file:`boards/`
files of an application (see :ref:`application-configuration-directory`):
:file:`<board>_<qualifiers>.overlay`, :file:`<board>.overlay` when the board
has one SoC, their revision and ``.conf`` counterparts.

.. code-block:: none

   boards/nxp/frdm_mcxa153/vanes/counter.capture/frdm_mcxa153.overlay
   boards/nxp/frdm_mcxw71/vanes/counter.capture/frdm_mcxw71.overlay
   boards/nxp/frdm_mcxw71/vanes/counter.capture/vane.yml

What holds for every board with a SoC goes in the same directory under the
SoC, named like the :file:`socs/` files of an application:
:file:`<soc>.overlay`, :file:`<soc>_<cpucluster>.overlay` and their ``.conf``
counterparts. The SoC's files are applied first, then the board's.

.. code-block:: none

   soc/<vendor>/vanes/<setup>/<soc>_<cpucluster>.overlay

The optional :file:`vane.yml` next to the files says what only the board
knows:

``fixture``
  What this hardware needs instead of the fixture the test scenario names.
  A fixture name adds a fixture the scenario does not name: the frdm_mcxw71
  above wires a GPIO to its capture input where the other boards route a
  signal inside the SoC, so its vane names the ``counter_capture_loopback``
  fixture and the scenario names none. A mapping replaces fixtures the
  scenario names, ``none`` waiving one, on a simulated board or when the
  wire is inside the SoC for instance:

  .. code-block:: yaml

     fixture:
       gpio_loopback: i2s_loopback

  .. code-block:: yaml

     fixture:
       dac_adc_loopback: none

  A vane owns only the fixtures it names, so the vanes of a scenario
  requiring several setups compose. A board's ``vane.yml`` also applies to
  a realization whose files come from the SoC.

Requesting vanes
****************

Twister requests the vanes of the setups a scenario requires, and filters
the platforms whose board and SoC provide none, with the reason ``Setup
<name> not provided by the board``, unless the setup states a filter. To
build with them by hand, name the setups in the ``VANES`` variable:

.. code-block:: console

   west build -b frdm_mcxw71/mcxw716c tests/drivers/counter/counter_capture -- \
     -DVANES=counter.capture

``west build -T <scenario>`` passes the setups the scenario requires, as
twister does. Under sysbuild, ``VANES`` applies to the main application
image.

:zephyr_file:`scripts/list_vanes.py` lists the setups, the boards realizing
each, and the setups a board realizes.
