# Zephyr with the Xtensa CALL0 ABI

The Xtensa register window mechanism is a poor fit for memory
protection.  The existing Zephyr optimizations, in particular the
cross-stack call we do on interrupt entry, can't be used.  The way
Xtensa windows spill is to write register state through the stack
pointers it finds in the hidden caller registers, which are wholely
under the control of the userspace app that was interrupted.  The
kernel can't be allowed to do that, it's a huge security hole.

Naively, the kernel would have to write out the entire 64-entry
register set on every entry from a context with a PS.RING value other
than zero, including system calls.  That's not really an acceptable
performance or complexity cost.

Instead, for userspace apps, Zephyr builds using the "call0" ABI,
where only a fixed set of 16 GPRs (see section 10 of the Cadence
Xtensa Instruction Set Architecture Summary for details on the ABI).
Kernel traps can then use the remaining GPRs for their own purposes,
greatly speeding up entry speed.

## Toolchain

Existing Xtensa toolchains support a ``-mabi=call0`` flag to generate
code with this ABI, and it works as expected.  It sets a
__XTENSA_CALL0_ABI__ preprocessor flag and the Xtensa HAL code and our
crt1.S file are set up to honor it appropriately (though there's a
glitch in the Zephyr HAL integration, see below).

Unfortunately that doesn't extend to binary artifacts.  In particular
the libgcc.a files generated for our existing SDK toolchains are using
the windowed ABI and will fail at runtime when you hit 64 bit math.

Cadence toolchains have an automatic multilib scheme and will select a
compatible libgcc automatically.

But for now, you have to have a separate toolchain (or at least a
separately-built libgcc) for building call0 apps.  I'm using this
script and it works fine, pending proper SDK integration:

.. code-block:: bash
    #!/bin/sh
    set -ex

    TC=$1
    if [ -z "$TC" ]; then
        TC=dc233c
    fi

    # Grab source (these are small)
    git clone https://github.com/zephyrproject-rtos/sdk-ng
    git clone https://github.com/crosstool-ng/crosstool-ng

    # Build ct-ng itself
    cd crosstool-ng
    ./bootstrap
    ./configure --enable-local
    make -j$(nproc)

    # Configure for the desired toolchain
    ln -s ../sdk-ng/overlays
    cp ../sdk-ng/configs/xtensa-${TC}_zephyr-elf.config .config

    grep -v CT_TARGET_CFLAGS .config > asdf
    mv asdf .config
    echo CT_TARGET_CFLAGS="-mabi=call0" >> .config
    echo CT_LIBC_NONE=y >> .config

    ./ct-ng olddefconfig
    ./ct-ng build.$(nproc)

    echo "##"
    echo "## Set these values to enable this toolchain:"
    echo "##"
    echo export CROSS_COMPILE=$HOME/x-tools/xtensa-${TC}_zephyr-elf/bin/xtensa-${TC}_zephyr-elf-
    echo export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile

Note you don't really need to use the toolchain that was built, it's
enough to take the libgcc.a and drop it on top of the one in your SDK,
just be careful to save the original, etc...

## Bugs and Missing Features

No support in the SDK.  See above about toolchains.

For simplicity, this code is written to save context to the arch
region of the thread struct and not the stack (except for nested
interrupts, obviously).  That's a common pattern in Zephyr, but for
KERNEL_COHERENCE (SMP) platforms it's actually a performance headache,
because the stack is cached where the thread struct is not.  We should
move this back to the stack, but that requires doing some logic in the
assembly to check that the resulting stack pointer doesn't overflow
the protected region, which is slightly non-trivial (or at least needs
a little inspiration).

Right now the Zephyr HAL integration doesn't build when the call0 flag
is set because of a duplicated symbol.  I just disabled the build at
the cmake level for now, but this needs to be figured out.

The ARCH_EXCEPT() handling is a stub and doesn't actually do anything.
Really this isn`t any different than the existing code, it just lives
in the asm2 files that were disabled and I have to find a shared
location for it.

Backtrace logging is likewise specific to the older frame format and
ABI and doesn't work.  The call0 ABI is actually much simpler, though,
so this shouldn't be hard to make work.

FPU support isn't enabled.  Likewise this isn't any different (except
trivially -- the context struct has a different layout).  We just need
to copy code and find compatible hardware to test it on.

I realized when writing this that our existing handling for NMI
exceptions (strictly any level > EXCM_LEVEL) isn't correct, as we
generate ZSR-style EPS/EPC register usage in those handlers that isn't
strictly separated from the code it may/will have interrupted.  This
is actually a bug with asm2 also, but it's going to make
high-priority/DEBUG/NMI exceptions unreliable unless fixed (I don't
think there are any Zephyr users currently though).
