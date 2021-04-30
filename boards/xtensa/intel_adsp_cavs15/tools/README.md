For various legacy reasons this directory has two similar log tools:
logtool.py and adsplog.py

Both may be used in automation so merging them would require some
coordination.

They both read from the same data from the exact same shared memory
yet they have significant differences:

- logtool.py reads /sys/kernel/debug/sof/etrace which requires the
  kernel driver to be loaded.
- adsplog.py finds the memory address by scanning
  /sys/bus/pci/devices/; this does not require a driver.

- logtool.py supports reading from a special QEMU location.

- logtool.py performs a raw dump of the memory and exits immediately.
- adsplog.py parses the data, understands the ring buffer and reads
  continuously. Its output is much more human-readable.

- adsplog.py has technical details explained in a comment at the top.

