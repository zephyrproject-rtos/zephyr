.. _infineon-pse84-cm55-ram-load:

Infineon PSE84 CM55 RAM-load snippet (infineon-pse84-cm55-ram-load)
#####################################################################

Overview
********

By default, every core on PSE84 boards -- CM33 secure, CM33 non-secure,
and CM55 -- executes in place (XIP) from external SMIF0 flash, each
through its own :file:`flash0`/:file:`flash0_s`/:file:`flash0_sahb` bus
alias onto that same physical chip. This snippet moves the CM55 half of
that off SMIF0:

- :kconfig:option:`CONFIG_SOC_PSE84_M55_RAM_LOAD`, set on the CM33
  ``enable_cm55`` companion application: copies the CM55 image into
  internal SoCMEM RAM before releasing the CM55 core. Releasing CM55 is
  ``enable_cm55``'s only job, so CM33 goes back to sleep immediately
  afterwards and is no longer fetching from SMIF0 either.
- :kconfig:option:`CONFIG_SOC_PSE84_M55_DISABLE_SMIF_XIP`, set on the CM55
  main image: once CM55's own code and data are fully resident in RAM
  (copied there by the option above), leaves SMIF0's XIP mode for good.

Every ``Cy_SMIF_Mem*()`` call checks SMIF0's XIP-mode bit and, while it is
set, wraps itself in a global interrupt disable. Leaving XIP mode for
good, once neither core still needs it, is what keeps a later
:c:func:`flash_area_erase` from re-entering that critical section and
freezing CM55's interrupts for the duration of the erase.

This snippet applies to both sysbuild images produced when building for an
M55-capable PSE84 board target: the main application, built for the
``.../m55`` board qualifier, and the ``enable_cm55`` companion application
sysbuild adds automatically for the ``.../m33`` board qualifier.

Requirements
************

This snippet currently only supports the ``kit_pse84_ai`` board
(``kit_pse84_ai/pse846gps2dbzc4a/m55``).

Usage
*****

Build :zephyr:code-sample:`hello_world` for the CM55 core with this
snippet applied to the whole sysbuild:

.. code-block:: console

   west build -b kit_pse84_ai/pse846gps2dbzc4a/m55 --sysbuild \
     -S infineon-pse84-cm55-ram-load samples/hello_world

PSE84 boots to the CM33 core, so something has to enable and release CM55.
The ``--sysbuild`` flag is what builds this snippet's ``enable_cm55``
companion application, one of two ways to do that -- a TF-M flow where
CM33 instead enables CM55 itself exists too, but is not what this snippet
uses. See the :zephyr:board:`kit_pse84_ai` board documentation.

Flashing
********

A plain ``west flash`` does not work with this snippet and must not be
used: by default it flashes every domain in :file:`build/domains.yaml`
(the CM55 main image, then ``enable_cm55``) in one pass, and CM55's
``zephyr.hex`` now encodes its SoCMEM execution address rather than its
flash storage address, so flashing it as a hex would write to the wrong
place. Flash each domain separately instead, **CM55 first**:

.. code-block:: console

   west flash --domain hello_world -- --use-bin --flash-address=0x60580000 --verify
   west flash --domain enable_cm55

CM55's image must already be in ``m55_xip`` before ``enable_cm55`` is
flashed: each ``west flash`` invocation ends by resetting and running the
whole chip, and ``enable_cm55`` (CM33) copies whatever is currently in
``m55_xip`` into SoCMEM and releases CM55 as soon as it boots. Flashing
``enable_cm55`` second means that copy always picks up the new CM55 image;
flashing it first would still end up correct once CM55 is flashed too
(CM55's own reset re-triggers the whole sequence), but would needlessly
release CM55 into whatever stale image was in ``m55_xip`` beforehand in
the meantime.

CM55 needs ``--use-bin`` (write the address-independent :file:`zephyr.bin`
instead of the hex) plus an explicit ``--flash-address`` pointing at
``m55_xip``'s flash storage location -- ``0x60580000`` is
:file:`flash0_sahb`'s base (``0x60000000``) plus :file:`m55_xip`'s offset
(``0x580000``); confirm both against the generated
:file:`build/hello_world/zephyr/zephyr.dts` if building something other
than :zephyr:code-sample:`hello_world`. ``--verify`` is required: while
halted, SMIF0 may not be in memory-mapped/XIP mode, so a raw memory read
back is not reliable confirmation that the write landed. ``enable_cm55``
needs neither option: it flashes normally, as a hex, to its own flash
partition.

Replace ``hello_world`` above with your own application's domain name
(shown as the first entry in :file:`build/domains.yaml`) if it differs.

Hardcoding ``--flash-address`` like this means it has to be updated by
hand if ``m55_xip``'s address ever changes. A future improvement could
instead patch the generated hex's load address from the devicetree at
build time, so that a plain ``west flash`` picks the right artifact and
address on its own.
