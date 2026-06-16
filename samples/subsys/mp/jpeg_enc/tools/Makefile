# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Meta Platforms, Inc.
#
# Native (Linux) build and smoke test for the libMP JPEG encoder.
#
# jpeg_enc_tool links directly against the in-tree, standalone bitbank-JPEGENC
# port (subsys/mp/plugins/zjpeg/jpeg_enc.c) and encodes a binary P6 PPM image
# to JPEG outside Zephyr. "make test" generates SMPTE HD colour bars with
# ffmpeg and encodes them.
#
# Usage:
#   make                 # build ./jpeg_enc_tool
#   make test            # generate bars.ppm via ffmpeg (aligned), encode to bars.jpg
#   make test-edge       # odd, non-MCU-aligned size -> exercises edge padding
#   ./jpeg_enc_tool --in your.ppm --out your.jpg
#   (convert any image first, e.g. 'ffmpeg -i in.png in.ppm')

HERE        := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ZEPHYR_BASE ?= $(abspath $(HERE)../../../../..)
LIB         := $(ZEPHYR_BASE)/subsys/mp/plugins/zjpeg/jpeg_enc.c
CFLAGS      := -O2 -Wall -Wextra -I$(ZEPHYR_BASE)/include

# The encoder handles any resolution: MCU-aligned frames take the fast path, and
# non-MCU-multiple frames are edge-padded (see JPEGAddMCU in jpeg_enc.c).
BARS_SIZE      ?= 1280x720   # MCU-aligned (multiple of 16)
BARS_EDGE_SIZE ?= 1277x717   # odd, NOT a multiple of 16: exercises edge padding

jpeg_enc_tool: jpeg_enc_tool.c $(LIB)
	$(CC) $(CFLAGS) $^ -o $@

# Generate SMPTE HD colour bars as a binary P6 PPM using ffmpeg.
bars.ppm:
	ffmpeg -y -f lavfi -i smptehdbars=size=$(BARS_SIZE) -frames:v 1 $@

# Smoke test: encode the generated bars to JPEG.
test: jpeg_enc_tool bars.ppm
	./jpeg_enc_tool --in bars.ppm --out bars.jpg

# Edge-case test: a non-MCU-multiple (odd) resolution that exercises the encoder's
# edge-MCU padding. The decoded JPEG should report the exact odd dimensions.
test-edge: jpeg_enc_tool
	ffmpeg -y -f lavfi -i smptehdbars=size=$(BARS_EDGE_SIZE) -frames:v 1 bars-edge.ppm
	./jpeg_enc_tool --in bars-edge.ppm --out bars-edge.jpg

clean:
	rm -f jpeg_enc_tool bars.ppm bars.jpg bars-edge.ppm bars-edge.jpg

.PHONY: test test-edge clean
