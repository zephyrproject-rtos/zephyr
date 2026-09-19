.. zephyr:code-sample:: frdm-rw612-iped-flash
   :name: IPED Flash Driver Sample

   Sample demonstrating flash read/write/erase on IPED-encrypted regions
   using the frdm_rw612 board.

Overview
********

This sample verifies that the Zephyr flash driver correctly handles
IPED (Inline Prince Encryption/Decryption) encrypted regions on the
NXP RW612 FlexSPI interface.

IPED transparently encrypts/decrypts data on the AHB bus. When IPED
regions are configured by the BootROM, the FlexSPI memc driver discovers
them at init and routes reads through the AHB memory-mapped path so IPED
decrypts inline. IP-command reads bypass IPED and return ciphertext.

Test Groups
===========

**AHB reads** — Read encrypted data at various offsets, sizes (1B to
64KB), and alignments. Verifies the AHB read path returns decrypted data.

**Read consistency** — Three consecutive reads must match. 100x stability
read. Chunked vs whole read comparison.

**Boundary detection** — Reads crossing IPED boundaries must return
``-EINVAL``. Reads fully inside or outside must succeed.

**Non-IPED reads** — FCB magic check, boot header, flash beyond IPED.
Verifies IP-command read path works normally.

**Storage partition** — Full erase/write/read cycle on ``storage_partition``
(outside IPED). Confirms normal flash operations are unaffected.

**Edge cases** — Zero-length read, first/last byte of IPED region,
block-aligned read.

**Data validation** — Different offsets return different data. IPED data
is not all-0xFF. Vector table looks like valid ARM code. 12-offset
uniqueness check.

**IPED erase/write** — Erase sector in IPED padding area, verify
erased-range tracking returns 0xFF. Write to erased IPED sector
(verifies flash_write succeeds). Full erase-write-erase cycle.

Prerequisites
*************

IPED regions must be provisioned on the board **before** running this
test. See `Provisioning`_ below.

Hardware
========

* FRDM-RW612 board
* USB cable for serial console and ISP mode

Software
========

* ``spsdk`` Python package (provides ``blhost`` and ``nxpimage``)::

    pip install spsdk

* ``pyserial`` (optional, for automated serial capture)::

    pip install pyserial

Provisioning
************

An automated script handles the full cycle:

.. code-block:: console

   cd samples/boards/nxp/frdm_rw612/iped

   # Full cycle: build + provision + capture results
   python3 scripts/provision_and_test.py \
       --port /dev/cu.usbmodem0010633821641 \
       --capture-port /dev/tty.usbmodem0010633821641 \
       --zephyr-base /path/to/zephyr

   # Skip build (use existing build directory)
   python3 scripts/provision_and_test.py --skip-build \
       --build-dir /path/to/build

   # Capture only (already provisioned)
   python3 scripts/provision_and_test.py --skip-build --skip-provision

On macOS, ``blhost`` uses ``/dev/cu.*`` (callout) and serial console
uses ``/dev/tty.*`` (dial-in). On Linux both are typically
``/dev/ttyACM0``.

Disabling IPED
**************

.. warning::

   IPED configuration persists in flash IFR (In-Field Record) across
   power cycles and image reflash. After testing, you **must** disable
   IPED before flashing a normal (non-encrypted) image, otherwise the
   board will not boot.

To disable IPED and restore normal operation:

.. code-block:: console

   # Put the board in ISP mode first, then:
   python3 scripts/provision_and_test.py --disable-iped \
       --port /dev/cu.usbmodem0010633821641

Or manually via ``blhost``:

.. code-block:: console

   PORT="/dev/cu.usbmodem0010633821641,115200"
   blhost -p $PORT -- fill-memory 0x2000F000 4 0xA0000000
   blhost -p $PORT -- fill-memory 0x2000F004 4 0x08000000
   blhost -p $PORT -- configure-memory 9 0x2000f000

After disabling, press RESET and flash any image normally with
``west flash`` or JLink.

Known Limitations
*****************

* **XIP constraint**: The sample runs from XIP flash within the IPED code
  region (0x1000-0x18000). Erase/write tests target the padding area
  at the end of the IPED region (last 4KB sector), not the code area.

* **GCM write restriction**: In GCM mode, IP-command writes store
  plaintext. AHB reads of plaintext trigger GCM authentication failure
  (chip security reset). The erase/write tests therefore do NOT read
  back after writing — they immediately re-erase to restore safe state.

* **Image padding**: The IPED region must be completely filled with
  encrypted data. The provisioning script pads the image with zeros
  before ROM encryption.

* **IPED end alignment**: The IPED end address must be 4KB-aligned.

* **Non-fused OTP keys**: The provisioning script uses the ROM default
  fuse key (deterministic zero-derived key). This provides functional
  IPED encryption for testing but **no cryptographic security**.
  Production deployments must program device-specific OTP key fuses.
