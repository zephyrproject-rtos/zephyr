import sys

# For some baffling reason IAMCU sets the instruction set architecture
# in the ELF header to 0x06 instead of 0x03 even though it is just
# 386 code. This gives QEMU fits. Hack it!
fd = open(sys.argv[1], "r+b")
fd.seek(0x12)
# Write 0x03 which is EM_386 to e_machine
fd.write(b'\x03')
fd.close()

