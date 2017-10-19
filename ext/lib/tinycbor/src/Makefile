obj-$(CONFIG_TINYCBOR) += cbor_buf_reader.o cborencoder.o cborerrorstrings.o \
cborparser_dup_string.o cbortojson.o open_memstream.o \
cbor_buf_writer.o cborencoder_close_container_checked.o \
cborparser.o cborpretty.o

ccflags-$(CONFIG_TINYCBOR) += -I${ZEPHYR_BASE}/ext/lib/tinycbor/include \
-DWITHOUT_OPEN_MEMSTREAM
