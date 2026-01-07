NXP virtual board (generated)
=============================

This sample provides a **virtual Zephyr board** that is **generated on the fly** from
SoC metadata plus a small JSON mapping file.

Goal
----

- Provide ``<vendor>_virtual/<soc>[/<cpucluster>]`` board targets
- Use it for **build-only SoC coverage**, including SoCs that do not have a real board
- Cover vendor families and **all CPU clusters**

Generate
--------

From the Zephyr repo root:

.. code-block:: bash

   python3 boards/virtual_board/tools/gen_nxp_virtual_board.py \
     --configuration boards/virtual_board/nxp/defs/nxp_virtual.json

This writes generated board files into:

.. code-block:: text

   boards/virtual_board/generated/boards/nxp/nxp_virtual

Build
-----

Point Zephyr at the generated boards:

Then build any target:

.. code-block:: bash

   # NOTE: do not set BOARD_ROOT to the legacy samples/boards/nxp/virtual_board/generated path.
   # The generator now writes to boards/virtual_board/generated.

   unset BOARD_ROOT

   west build -b nxp_virtual/mcxc242 samples/hello_world
   west build -b nxp_virtual/mcxn947/cpu0 samples/hello_world
   west build -b nxp_virtual/rw612 samples/hello_world

Twister (build-only)
--------------------

.. code-block:: bash

   # Use the generated board root (same as the -A path shown above)
   export BOARD_ROOT=$PWD/boards/virtual_board/generated

   # Example: build only for all nxp_virtual targets that twister discovers
   # (You can also pass -p <board> to select specific targets)
   ./scripts/twister -T samples/hello_world --build-only -v

Using this for other vendors
----------------------------

The virtual-board concept here is not NXP-specific: it is a small Python generator plus
a vendor mapping file, with shared plumbing in ``boards/virtual_board/tools/virtual_board_gen_common.py``.

To add a similar *build-only* virtual board for another vendor, the simplest workflow is:

1. Create a vendor config JSON.

   - Add a new folder: ``boards/virtual_board/<vendor>/defs/``
   - Create ``<vendor>_virtual.json`` next to the NXP example
     (see ``boards/virtual_board/nxp/defs/nxp_virtual.json``)

   A minimal starting point looks like:

   .. code-block:: json

      {
        "board": {
          "name": "<vendor>_virtual",
          "full_name": "<VENDOR> Virtual Board (SoC template)",
          "vendor": "<vendor>"
        },
        "soc_ymls": [
          "soc/<vendor>/<family>/soc.yml"
        ],
        "include_all_cpuclusters": true,
        "output": {
          "root": "boards/virtual_board/generated",
          "board_relpath": "boards/<vendor>/<vendor>_virtual"
        },
        "master_platforms": {
          "<soc_series_from_soc_yml>": "<an_existing_real_board_identifier>"
        }
      }

   Notes on the key fields:

   - ``soc_ymls``: points at the vendor ``soc.yml`` files you want to enumerate.
   - ``master_platforms``: maps each SoC *series* to a real board "identifier" used as a DTS template.
   - ``overrides.dtsi`` (optional): use when the SoC/cluster-to-DTSI mapping cannot be derived.
   - ``overrides.patch_generated_dts`` (optional): per-SoC/per-target list of DTS nodes
     (e.g. ``"&lpi2c1"``) to remove from the copied master DTS, plus optional
     feature strings to drop from the emitted YAML ``supported:`` list.
   - ``dts_labels`` (optional): only needed if overlays require specific devicetree node labels.
   - ``reuse_<vendor>_platforms``-style behavior is vendor-specific; see NXP's
     ``reuse_nxp_platforms`` as an example of reusing metadata from existing boards.

2. Add a small generator entry point.

   - Copy ``boards/virtual_board/tools/gen_nxp_virtual_board.py`` to
     ``boards/virtual_board/tools/gen_<vendor>_virtual_board.py``
   - Keep it as a thin wrapper around the shared code in
     ``boards/virtual_board/tools/virtual_board_gen_common.py``
   - Point it at your ``boards/virtual_board/<vendor>/defs/<vendor>_virtual.json``

3. Generate and build.

   .. code-block:: bash

      python3 boards/virtual_board/tools/gen_<vendor>_virtual_board.py \
        --configuration boards/virtual_board/<vendor>/defs/<vendor>_virtual.json

      west build -b <vendor>_virtual/<soc> samples/hello_world

Notes / troubleshooting
-----------------------

- The generated DTS files intentionally do **not** set ``chosen { zephyr,sram ... }`` etc.
  Many SoCs differ in naming and memory layout; for build-only coverage, letting SoC
  defaults stand is usually enough. If a specific SoC needs a chosen override, add a
  custom DTSI include mapping or extend the generator.

- For i.MX / i.MX RT / DSP clusters, DTSI mapping is not always derivable from soc name.
  Use ``defs/nxp_virtual.json`` ``overrides.dtsi`` entries.

- SoC enumeration comes from Zephyr ``soc.yml`` files (configured via ``soc_ymls`` in
  ``defs/nxp_virtual.json``). The legacy ``families`` key is still accepted.

- The generator will, by default, scan existing boards under ``boards/nxp/`` and reuse:

  - ``supported:`` (capabilities list) for matching SoCs/variants
  - a suitable SoC DTSI include (derived from the real board ``.dts`` include list)

  If a SoC/variant has no matching board, the generator will pick a “master” board
  per SoC series (from ``soc.yml``) and use that as a fallback.

  You can disable this behavior by setting ``reuse_nxp_platforms: false`` in the JSON config.

- Master platform mapping (for DTS templating):

  The generated DTS files are based on a “master” real board DTS for each SoC *series*.
  This mapping is configured in ``defs/nxp_virtual.json`` under ``master_platforms``.

  - Keys are SoC series names from ``soc.yml`` (e.g. ``imxrt10xx``, ``imx9``, ``mcxc``)
  - Values are existing board ``identifier`` strings from ``boards/nxp/**/*.yaml``
    (for example ``mimxrt1024_evk`` or ``mimxrt1170_evk@A/mimxrt1176/cm7``).
  - You can override the master platform per SoC/target via ``overrides.master_platform``
    using keys ``soc`` or ``soc/cluster``.

- Some SoC bindings require board DTS overlays which reference devicetree **node labels**
  (e.g. MCXC uses ``&sim``, ``&osc``, ``&cpu0``). If your SoC DTSI uses different labels,
  configure them in ``defs/nxp_virtual.json``:

  - ``dts_labels``: global defaults
  - ``overrides.dts_labels``: per-SoC or per-target overrides using the same key format
    as ``overrides.dtsi`` (e.g. ``"mcxc141"`` or ``"mimx9352/a55"``).
