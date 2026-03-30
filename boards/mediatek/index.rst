.. _boards-mtk_adsp:

Mediatek Audio DSPs
###################

Zephyr can be built and run on the Audio DSPs included in various
members of the Mediatek MT8xxx series of ARM SOCs used in Chromebooks
from various manufacturers and Mediatek Genio evaluations kits.

Four of these DSPs are in the market already, implemented via the
MT8195 ("Kompanio 1380", MT8186 ("Kompanio 520"), MT8188 ("Kompanio 838")
and MT8365 ("Genio 350 EVK") SOCs. Development has been done on and
validation performed on at least these devices, though more exist:

  ======  =============  ===================================  =================
  SOC     Product Name   Example Device                       ChromeOS Codename
  ======  =============  ===================================  =================
  MT8195  Kompanio 1380  HP Chromebook x360 13b               dojo
  MT8186  Kompanio 520   Lenovo 300e Yoga Chromebook Gen 4    steelix
  MT8188  Kompanio 838   Lenovo Chromebook Duet 11            ciri
  MT8365  Genio 350      Genio 350 EVK
  ======  =============  ===================================  =================

Hardware
********

These devices are Xtensa DSP cores, very similar to the Intel ADSP
series in concept (with the notable difference that these are all
single-core devices, no parallel SMP is available, but at the same
time there are fewer worries about the incoherent cache).

Their memory space is split between dedicated, fast SRAM and ~16MB of
much slower system DRAM.  Zephyr currently loads and links into the
DRAM area, a convention it inherits from SOF (these devices have
comparatively large caches which are used for all accesses, unlike
with intel_adsp).  SRAM is used for interrupt vectors and stacks,
currently.

There is comparatively little on-device hardware.  The architecture is
that interaction with the off-chip audio hardware (e.g. I2S codecs,
DMIC inputs, etc...) is managed by the host kernel.  The DSP receives
its data via a single array of custom DMA controllers.

Beyond that the Zephyr-visible hardware is limited to a bounty of
timer devices (of which Zephyr uses two), and a "mailbox"
bidirectional interrupt source it uses to communicate with the host
kernel.

Programming and Debugging
*************************

These devices work entirely in RAM, so there is no "flash" process as
such.  Their memory state is initialized by the host Linux
environment.  This process works under the control of a
``mtk_adsp_load.py`` python script, which has no dependencies outside
the standard library and can be run (as root, of course) on any
reasonably compatible Linux environment with a Python 3.8 or later
interpreter.  A chromebook in development mode with the dev packages
installed works great.  See the ChromiumOS developer library for more
detail:

* `Developer mode <https://www.chromium.org/chromium-os/developer-library/guides/device/developer-mode/>`__
* `Dev-Install: Installing Developer and Test packages onto a Chrome OS device <https://www.chromium.org/chromium-os/developer-library/guides/device/install-software-on-base-images/>`__

Once you have the device set up, the process is as simple as copying
the ``zephyr.img`` file from the build directory to the device
(typically via ssh) and running it with the script.  For example for
my mt8186 device named "steelix":

.. code-block:: console

   user@dev_host:~$ west build -b mt8186/mt8186/adsp samples/hello_world
   ...
   ... # build output
   ...
   user@dev_host:~$ scp build/zephyr/zephyr.img root@steelix:
   user@dev_host:~$ scp soc/mediatek/mt8xxx/mtk_adsp_load.py root@steelix:
   user@dev_host:~$ ssh steelix

   root@steelix:~ # ./mtk_adsp_load.py load zephyr.img
   *** Booting Zephyr OS build v4.2.0-4743-g80fdfabcba48 ***
   Hello World! mt8186/mt8186/adsp

Debugging
=========

Given the limited I/O facilities, debugging support remains limited on
these platforms.  Users with access to hardware-level debug and trace
tools (e.g. from Cadence) will be able to use them as-is.  Zephyr
debugging itself is limited to printk/logging techniques at the
moment.  In theory a bidirectional console like winstream can be used
with gdb_stub, which has support on Xtensa and via the SDK debuggers,
but this is still unintegrated.

Toolchains
**********

The MT8195, MT818X and MT8365 toolchains are already part of the Zephyr
SDK, so builds for the ``mt8195/mt8195/adsp``, ``mt8186/mt8186/adsp``,
``mt8188/mt8188/adsp`` and ``mt8365/mt8365/adsp`` board targets should
work out of the box simply following the generic Zephyr build instructions
in the Getting Started guide.

Closed-source Tools
===================

Zephyr can also be built by the proprietary Cadence xcc and xt-clang
toolchains.  Support for those tools is beyond the scope of this
document, but it works similarly, by specifying your toolchain and
core identities and paths via the environment, for example:

.. code-block:: shell

   export XTENSA_TOOLS_ROOT=/path/to/XtDevTools
   export XTENSA_CORE=hifi5_7stg_I64D128
   export TOOLCHAIN_VER=RI-2021.6-linux
   export ZEPHYR_TOOLCHAIN_VARIANT=xt-clang
   export XTENSA_TOOLCHAIN_PATH=$XTENSA_TOOLS_ROOT/install/tools
   west build -b mt8186_adsp samples/hello_world
