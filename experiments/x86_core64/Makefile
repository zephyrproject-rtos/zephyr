# No unwind tables is just to save size. No SSE is allowed because GCC
# uses it for miscellaneous optimizations that aren't related to
# floating point, and we don't want to take the traps except on
# threads that definitely need it.  No red zone becuase it's
# incompatible with traditional stack-based interrupt entry.
CFLAGS =  -Os -std=c11 -ffreestanding -fno-pic -fno-asynchronous-unwind-tables -mno-sse -mno-red-zone

LDFLAGS = -Wl,--build-id=none -nostdlib -nodefaultlibs -nostartfiles

# This works great.  But note that Fedora ships no libgcc for the
# target, so once we start to need stuff from that we'll need to move
# to a custom cross compiler.
ARCHFLAGS = -mx32

all: core64.elf

# First link the initial 32 bit stub, which goes at the front of our
# image.
stub32.bin: stub32.c *.h stub32.ld
	gcc -Wall -m32 $(CFLAGS) -c stub32.c
	gcc -m32 -T stub32.ld $(LDFLAGS) -o stub32.elf $(CFLAGS) stub32.o
	objcopy -O binary stub32.elf $@

# This is the main OS image, starting with the 32 bit stub and
# containing all the 64 bit code.
core64.elf64: stub32.bin stub16.bin core64.c demo-kernel.c *.h core64.ld
	gcc $(ARCHFLAGS) -Wall $(CFLAGS) -c core64.c
	gcc $(ARCHFLAGS) -Wall $(CFLAGS) -c demo-kernel.c
	gcc $(ARCHFLAGS) -T core64.ld $(LDFLAGS) -o $@ $(CFLAGS) core64.o demo-kernel.o

# Final step.  We now have an x86_64 ELF binary, which is not a valid
# multiboot image as the entry point is of course 32 bit.  It needs to
# be a i386 image, so copy out the segment and relink the blob one
# last time.
core64.elf: core64.elf64 core64.ld
	objcopy -O binary $< core64.bin
	echo '.incbin "core64.bin"' | as --32 -c - -o core6432.o
	gcc -m32 -T core64.ld $(LDFLAGS) -o $@ $(CFLAGS) core6432.o

# We can rely on the bootloader to handover a machine running in 386
# protected mode, but SMP cores start in real mode and need a tiny
# bootstrap layer of 16 bit code.
stub16.bin: stub16.c
	gcc -m16 $(CFLAGS) -c $<
	objcopy -O binary -j .text stub16.o stub16.bin
	objdump -b binary -m i386  -M i8086 -D stub16.bin > stub16.disasm

run: core64.elf
	qemu-system-x86_64 -smp cpus=2 -icount shift=1 -no-reboot -no-shutdown -serial stdio -d unimp,pcall,guest_errors -kernel $<

clean:
	rm -f *.elf *.elf64 *.o *~ *.bin *.disasm
