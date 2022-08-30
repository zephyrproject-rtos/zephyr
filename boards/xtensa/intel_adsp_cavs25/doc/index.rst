.. _zephyr-audio-dsp-development-on-chromebooks:

Zephyr Audio DSP Development on Chromebooks
###########################################

The Audio DSP on Intel Chromebooks is configured to use the SOF
"Community" key for firmware signing, and can therefore accept
arbitrary user-developed firmware like Zephyr applications (of which
SOF is one), including the Zephyr samples and test suite.

Initial TGL Chromebook Setup
****************************

(These instructions were written specifically to the Asus Flip CX5
device code named "delbin".  But they should be reasonably applicable
to any recent Intel device.)

Power the device on and connect it to a wireless network.  It will
likely want to download a firmware update (mine did).  Let this finish
first, to ensure you have two working OS images.

Enable Developer Mode
=====================

Power the device off (menu in lower right, or hold the power button
on the side)

Hold Esc + Refresh (the arrow-in-a-circle "reload" key above "3") and
hit the power key to enter recovery mode.  Note: the touchscreen and
pad don't work in recovery mode, use the arrow keys to navigate.

Select "Advanced Options", then "Enable Developer Mode" and confirm
that you really mean it.  Select "Boot from Internal Storage" at the
bootloader screen.  You will see this screen every time the machine
boots now, telling you that the boot is unverified.

Wait while the device does the required data wipe.  My device takes
about 15 minutes to completely write the stateful partition.  On
reboot, select "Boot from Internal Storage" again and set it up
(again) with Google account.

Make a Recovery Drive
=====================

You will at some point wreck your device and need a recovery stick.
Install the Chromebook Recovery Utility from the Google Web Store and
make one.

You can actually do this on any machine (and any OS) with Chrome
installed, but it's easiest on the Chromebook because it knows its
device ID (for example "DELBIN-XHVI D4B-H4D-G4G-Q9A-A9P" for the Asus
Tiger Lake board).  Note that recovery, when it happens, will not
affect developer mode or firmware settings but it **will wipe out the
root filesystem and /usr/local customizations you have made**.  So
plan on a strategy that can tolerate data loss on the device you're
messing with!

Make the root filesystem writable
=================================

For security, ChromeOS signs and cryptographically verifies (using
Linux's dm-verity feature) all access to the read-only root
filesystem.  Mucking with the rootfs (for example, to install modules
for a custom kernel) requires that the dm-verity layer be turned off:

First open a terminal with Ctrl-Alt-T.  Then at the "crosh> " prompt
issue the "shell" command to get a shell running as the "chronos"
user.  Finally (in developer mode) a simple "sudo su -" will get you a
root prompt.

.. code-block:: console

    crosh> shell
    chronos@localhost / $ sudo su -
    localhost ~ #

Now you need to turn of signature verification in the bootloader
(because obviously we'll be breaking whatever signature existed).
Note that signature verification is something done by the ROM
bootloader, not the OS, and this setting is a (developer-mode-only)
directive to that code:

.. code-block:: console

    cros# crossystem dev_boot_signed_only=0

(*Note: for clarity, commands in this document entered at the ChromeOS
core shell will be prefixed with a hostname of cros.*)

Next you disable the validation step:

.. code-block:: console

    cros# /usr/share/vboot/bin/make_dev_ssd.sh --remove_rootfs_verification

**THIS COMMAND WILL FAIL**, give you an error that you are changing
the setting for the entire running system, and suggest an alternative
"--partitions X" argument to use that modifies only the currently used
partition.  Run that modified command, then reboot.

After rebooting, you will notice that your chromebook boots with the
raw storage device (e.g. /dev/nvme0n1p5) mounted as root and not the
"dm-0" verity device, and that the rootfs is read-write.

Note: What this command actually does is modify the command line of
the installed kernel image (it saves a backup in
/mnt/stateful_partition/cros_sign_backups) so that it specifies
"root=<guid>" and not "root=dm-0".  It does seem to leave the other
verity configuration in place though, it just doesn't try to mount the
resulting (now-invalid!) partition.

Metanote: The astute will note that we're probably going to throw this
kernel out, and that we could probably have just edited the command
line of the new kernel instead of flashing and rebooting into this
modified one.  But that's too many balls to juggle at once for me.

Enable ChromeOS SSH
===================

Once you are booted with a writable partition, you can turn on the
built-in ssh server with:

.. code-block:: console

    cros# /usr/libexec/debugd/helpers/dev_features_ssh

By default neither the "chronos" user nor root accounts have
passwords, so unless you want to type a ssh key in by hand, you
probably want to set a password for the first login (before you run
ssh-copy-id, of course):

.. code-block:: console

    cros# passwd

Now ssh into the chromebook and add your key to
``.ssh/authorized_keys`` as you do for any Linux system.

Install Crouton
***************

The Zephyr integration tools require a proper Linux environment and
won't run on ChromeOS's minimal distro.  So we need to install a Linux
personality.  **DO NOT** bother installing the "Linux Development
Environment" (Crostini) from the ChromeOS Developer settings.  This
personality runs inside a VM, where our tools need access to the real
kernel running on the real hardware.  Instead install Crouton
(https://github.com/dnschneid/crouton), which is a community
chroot-based personality that preserves access to the real hardware
sysfs and /dev filesystem.  These instructions install the "cli-extra"
package list, there are X11-enabled ones available too if you prefer
to work on the device screen directly.  See the project page, etc...

At a root shell, grab the installer and run it (note: /usr/local is
the only writable filesystem without noexec, you must place the binary
there for it to run!):

.. code-block:: console

    cros# mkdir -p /usr/local/bin
    cros# curl -L https://github.com/dnschneid/crouton/raw/master/installer/crouton \
                  > /usr/local/bin/crouton
    cros# chmod 755 /usr/local/bin/crouton
    cros# crouton -r focal -t cli-extra

Start the Crouton chroot environment:

.. code-block:: console

    cros# startcli

Now you are typing commands into the Ubuntu environment.  Enable
inbound ssh on Crouton, but on a port other than 22 (which is used for
the native ChromeOS ssh server).  I'm using 222 here (which is easy to
remember, and not a registered port in /etc/services):

.. code-block:: console

    crouton# apt install iptables openssh-server
    crouton# echo "Port 222" >> /etc/ssh/sshd_config
    crouton# mkdir /run/sshd
    crouton# iptables -I INPUT -p tcp --dport 222 -j ACCEPT
    crouton# /usr/sbin/sshd

(*As above: note that we have introduced a hostname of "crouton" to
refer to the separate Linux personality.*)

NOTE: the mkdir, iptables and sshd commands need to be run every time
the chroot is restarted.  You can put them in /etc/rc.local for
convenience.  Crouton doesn't run systemd (because it can't -- it
doesn't own the system!) so Ubuntu services like openssh-server don't
know how to start themselves.

Building and Installing a Custom Kernel
***************************************

On your build host, grab a copy of the ChromeOS kernel tree.  The
shipping device is using a 5.4 kernel, but the 5.10 tree works for me
and seems to have been backporting upstream drivers such that its main
hardware is all quite recent (5-6 weeks behind mainline or so).  We
place it in the home directory here for simplicity:

.. code-block:: console

    dev$ cd $HOME
    dev$ git clone https://chromium.googlesource.com/chromiumos/third_party/kernel
    dev$ cd kernel
    dev$ git checkout chromeos-5.10

(*Once again, we are typing into a different shell.  We introduce the
hostname "dev" here to represent the development machine on which you
are building kernels and Zephyr apps. It is possible to do this on the
chromebook directly, but not advisable.  Remember the discussion above
about requiring a drive wipe on system recovery!*)

Note: you probably have an existing Linux tree somewhere already.  If
you do it's much faster to add this as a remote there and just fetch
the deltas -- ChromeOS tracks upstream closely.

Now you need a .config file.  The Chromebook kernel ships with the
"configs" module built which exposes this in the running kernel.  You
just have to load the module and read the file.

.. code-block:: console

    dev$ cd /path/to/kernel
    dev$ ssh root@cros modprobe configs
    dev$ ssh root@cros zcat /proc/config.gz > .config

You will need to set some custom configuration variables differently
from ChromeOS defaults (you can edit .config directly, or use
menuconfig, etc...):

+ ``CONFIG_HUGETLBFS=y`` - The Zephyr loader tool requires this
+ ``CONFIG_EXTRA_FIRMWARE_DIR=n`` - This refers to a build directory
    in Google's build environment that we will not have.
+ ``CONFIG_SECURITY_LOADPIN=n`` - Pins modules such that they will
    only load from one filesystem.  Annoying restriction for custom
    kernels.
+ ``CONFIG_MODVERSIONS=n`` - Allow modules to be built and installed
    from modified "dirty" build trees.

Now build your kernel just as you would any other:

.. code-block:: console

    dev$ make olddefconfig     # Or otherwise update .config
    dev$ make bzImage modules  # Probably want -j<whatever> for parallel build

The modules you can copy directly to the (now writable) rootfs on the
device.  Note that this filesystem has very limited space (it's
intended to be read only), so the INSTALL_MOD_STRIP=1 is absolutely
required, and you may find you need to regularly prune modules from
older kernels to make space:

.. code-block:: console

    dev$ make INSTALL_MOD_PATH=mods INSTALL_MOD_STRIP=1 modules_install
    dev$ (cd mods/lib/modules; tar cf - .) | ssh root@cros '(cd /lib/modules; tar xfv -)'

Pack and Install ChromeOS Kernel Image
======================================

The kernel bzImage file itself needs to be signed and packaged into a
ChromeOS vboot package and written directly to the kernel partition.
Thankfully the tools to do this are shipped in Debian/Ubuntu
repositories already:

.. code-block:: console

    $ sudo apt install vboot-utils vboot-kernel-utils

Find the current kernel partition on the device.  You can get this by
comparing the "kernel_guid" command line parameter (passed by the
bootloader) with the partition table of the boot drive, for example:

.. code-block:: console

    dev$ KPART=`ssh root@cros 'fdisk -l -o UUID,Device /dev/nvme0n1 | \
                               grep -i $(sed "s/.*kern_guid=//" /proc/cmdline \
                                         | sed "s/ .*//") \
                               | sed "s/.* //"'`
    dev$ echo $KPART
    /dev/nvme0n1p4

Extract the command line from that image into a local file:

.. code-block:: console

    dev$ ssh root@cros vbutil_kernel --verify /dev/$KPART | tail -1 > cmdline.txt

Now you can pack a new kernel image using the vboot tooling.  Most of
these arguments are boilerplate and always the same.  The keys are
there because the boot requires a valid signature, even though as
configured it won't use it.  Note the cannot-actually-be-empty dummy
file passed as a "bootloader", which is a holdover from previous ROM
variants which needed an EFI stub.

.. code-block:: console

    dev$ echo dummy > dummy.efi
    dev$ vbutil_kernel --pack kernel.img --config cmdline.txt \
           --vmlinuz arch/x86_64/boot/bzImage \
           --keyblock /usr/share/vboot/devkeys/kernel.keyblock \
           --signprivate /usr/share/vboot/devkeys/kernel_data_key.vbprivk \
           --version 1 --bootloader dummy.efi --arch x86_64

You can verify this image if you like with "vbutil_kernel --verify".

Now just copy up the file and write it to the partition on the device:

.. code-block:: console

    $ scp kernel.img root@cros:/tmp
    $ ssh root@cros dd if=/tmp/kernel.img of=/dev/nvme0n1p4

Now reboot, and if all goes well you will find yourself running in
your new kernel.

Wifi Firmware Fixup
===================

On the Tiger Lake Chromebook, the /lib/firmware tree is a bit stale
relative to the current 5.10 kernel.  The iwlwifi driver requests a
firmware file that doesn't exist, leading to a device with no network.
It's a simple problem, but a catastrophic drawback if uncorrected.  It
seems to be sufficient just to link the older version to the new name.
(It would probably be better to copy the proper version from
/lib/firmware from a recent kernel.org checkout.):

.. code-block:: console

    cros# cd /lib/firmware
    cros# ln -s iwlwifi-QuZ-a0-hr-b0-62.ucode iwlwifi-QuZ-a0-hr-b0-64.ucode

Build and Run a Zephyr Application
**********************************

Finally, with your new kernel booted, you are ready to run Zephyr
code.

Build rimage Signing Tool
=========================

First download and build a copy of the Sound Open Firmware "rimage"
tool (these instructions put it in your home directory for clarity,
but anywhere is acceptable):

.. code-block:: console

     dev$ cd $HOME
     dev$ git clone https://github.com/thesofproject/rimage
     dev$ cd rimage/
     dev$ git submodule init
     dev$ git submodule update
     dev$ cmake .
     dev$ make

Copy Integration Scripting to Chromebook
========================================

There is a python scripts needed on the device, to be run inside
the Crouton environment installed above.  Copy them:

.. code-block:: console

    dev$ scp boards/xtensa/intel_adsp_cavs15/tools/cavstool.py user@crouton:

Then start the service in the Crouton environment:

.. code-block:: console

    crouton$ sudo ./cavstool.py user@crouton:


Build and Sign Zephyr App
=========================

Zephyr applications build conventionally for this platform, and are
signed with "west flash" with just a few extra arguments.  Note that
the key in use for the Tiger Lake DSP is the "3k" key from SOF, not
the original that is used with older hardware.  The output artifact is
a "zephyr.ri" file to be copied to the device.

.. code-block:: console

    dev$ west build -b intel_adsp_cavs25 samples/hello_world
    dev$ west sign --tool-data=~/rimage/config -t ~/rimage/rimage -- \
                -k $ZEPHYR_BASE/../modules/audio/sof/keys/otc_private_key_3k.pem

Run it!
=======

The loader script takes the signed rimage file as its argument.  Once
it reports success, the application begins running immediately and its
console output (in the SOF shared memory trace buffer) can be read by
the logging script.

.. code-block:: console

    dev$ west flash --remote-host crouton
    Hello World! intel_adsp_cavs25

Misc References
***************

Upstream documentation from which these instructions were drawn:

This page has the best reference for the boot process:

http://www.chromium.org/chromium-os/chromiumos-design-docs/disk-format

This is great too, with an eye toward booting things other than ChromeOS:

https://www.chromium.org/chromium-os/developer-information-for-chrome-os-devices/custom-firmware
