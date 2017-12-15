#!/usr/bin/python3

# cec1702-image.py - CEC1702 SPI flash image creater utility
# Copyright (c) 2019 Crypta Labs Ltd.
#
# SPDX-License-Identifier: Apache-2.0

import os, argparse, struct, sys, crcmod
from cryptography import x509
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils

backend = default_backend()

def int_to_bytes(val, num_bytes):
	# big-endian representation (ROM Addendum documentation is incorrect)
	return [(val >> (num_bytes-pos-1)*8) & 0xff for pos in range(num_bytes)]

def digest(hashalg, blob):
	d = hashes.Hash(hashalg, backend=backend)
	d.update(blob)
	return d.finalize()

def sign(blob, sign_key):
	# Sign or add checksum according to ROM Addendum
	if sign_key:
		# Raw EC-DSA signature using secp256R1 EC-key
		rfc = sign_key.sign(blob, ec.ECDSA(hashes.SHA256()))
		r, s = utils.decode_dss_signature(rfc)
		return bytes(bytearray(int_to_bytes(r, 32) + int_to_bytes(s, 32)))
	else:
		# Raw SHA256 digest
		return digest(hashes.SHA256(), blob) + b'\x00'*32

def private_key_to_raw_public_key(p):
	# Return RAW Uncompressed format without the leading 0x04 format byte of X962
	pubkey = p.public_key()
	if not hasattr(serialization.Encoding, 'X962'):
		# Old API, deprecated in cryptography 2.5
		return pubkey.public_numbers().encode_point()[1:]
	return pubkey.public_bytes(serialization.Encoding.X962, serialization.PublicFormat.UncompressedPoint)[1:]

def encrypt(blob, peer_public_key):
	from cryptography.hazmat.primitives.kdf.x963kdf import X963KDF
	from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

	# Encrypt firmware image according to ROM Addendum:
	# Ephemeral EC-DH over secp256R1, with ANSI X9.63 KDF to generate AES-256-CBC key and IV
	# The uncompressed public ephemeral key is appended to the end
	ephemeral_key = ec.generate_private_key(ec.SECP256R1(), backend)
	shared_secret = ephemeral_key.exchange(ec.ECDH(), peer_public_key)
	derived_key = X963KDF(algorithm=hashes.SHA256(), length=48, sharedinfo=None, backend=backend).derive(shared_secret)
	enc = Cipher(algorithms.AES(derived_key[0:32]), modes.CBC(derived_key[32:48]), backend=backend).encryptor()
	return enc.update(blob) + enc.finalize() + private_key_to_raw_public_key(ephemeral_key)

def bootable_image(opts):
	# image size needs to be modulo 64 due to block size requirement
	# try to workaround bootloader bug in part C2 by increasing padding as needed
	img = opts.image.read()
	if opts.encrypt:
		img_pad = 256
	elif opts.sign:
		img_pad = 128
	else:
		img_pad = 64
	x = len(img) % img_pad
	if x != 0:
		img = img + b'\0' * (img_pad - x)

	# header
	entry = struct.unpack_from("<L", img, 4)[0]	# entry point
	vtr_byte = 0x00					# encryption & vtr control
	img_len = len(img)
	if opts.encrypt:
		vtr_byte |= 0x80
		img = encrypt(img, opts.encrypt)

	# Create CEC boot header
	hdr = bytearray(b'\0'*0x40)
	struct.pack_into("<4sBBBBLLHHL", hdr, 0,
		b"PHCM",	# magic
		0x00,		# version
		opts.spi_clock + (opts.spi_drive << 2), # SPI configuration
		vtr_byte,		# VTR / encryption control
		opts.spi_command,	# Flash read command
		opts.load_address,	# Load Address (SRAM)
		entry,			# Entry Address
		img_len // 64,		# Image Length (blocks of 64 bytes)
		0x0000,			# Reserved
		opts.image_offset)	# HDR size (offset to firmware image)
	hdr = bytes(hdr)
	hdr_sign = sign(hdr, opts.sign)
	img_sign = sign(img, opts.sign)
	return hdr + hdr_sign + b'\x00'*(opts.image_offset-len(hdr)-len(hdr_sign)) + img + img_sign

def tag(offset):
	offset = offset >> 8
	h = crcmod.predefined.Crc('crc-8-itu')
	h.update(struct.pack('BBB', offset & 0xff, (offset >> 8) & 0xff, (offset >> 16) & 0xff))
	return offset | (ord(h.digest()) << 24)

def flashable_image(args, img0, img1):
	img = bytearray(b'\xff' * args.flash_size)
	struct.pack_into("<LL", img, 0, tag(args.img0_offset), tag(args.img1_offset))
	img[args.img0_offset:args.img0_offset+len(img0)] = img0
	img[args.img1_offset:args.img1_offset+len(img1)] = img1
	return bytes(img)

def load_file(fn):
	return open(fn, 'rb').read()

def load_pem_private_key(fn):
	blob = load_file(fn)
	for pwd in [ "ECPRIVKEY001", os.getenv("PEM_PASSPHRASE") ]:
		try:
			return serialization.load_pem_private_key(blob, password=pwd.encode(), backend=backend)
		except:
			pass
	raise argparse.ArgumentTypeError('Invalid private key passphrase')

def load_pem_certificate(fn):
	return x509.load_pem_x509_certificate(load_file(fn), backend).public_key()

def get_image_opts(args, n):
	opts = dict()
	vargs = vars(args)
	prefix = "img%d_" % n
	for k in vargs:
		if k.startswith(prefix):
			opts[k[len(prefix):]] = vargs[k]
	return argparse.Namespace(**opts)

def main():
	ap = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
	ap.add_argument("--flash-size", help="Size of the flash", type=int, default=0x200000, metavar="SIZE")
	ap.add_argument("--image-out", help="Write raw flash image to file (out)", type=argparse.FileType('wb'), metavar="FILE")
	for x, req, offs in ('img0', True, 0x001000), ('img1', False, 0x081000):
		g = ap.add_argument_group(x, "Options for %s" % x)
		g.add_argument("--%s-image" % x, help="Executable image", type=argparse.FileType('rb'), metavar="IMG", required=req)
		g.add_argument("--%s-image-offset" % x, help="Firmware image offset from header", type=int, default=0x100, metavar="OFFSET")
		g.add_argument('--%s-load-address' % x, help="Image SRAM load address", type=int, default=0xB0000, metavar="ADDR")
		g.add_argument("--%s-offset" % x, help="Image offset in SPI flash", type=int, default=offs, metavar="OFFS")
		g.add_argument("--%s-encrypt" % x, help="Encrypt using certificate public key (X.509 PEM)", type=load_pem_certificate, metavar="PEM")
		g.add_argument("--%s-sign" % x, help="Signing private key (PEM)", type=load_pem_private_key, metavar="PEM")
		g.add_argument("--%s-spi-clock" % x, help="SPI clock (0=48MHz, 1=24MHz, 2=16MHz, 3=12MHz)", choices=range(4), type=int, default=3, metavar="CLK")
		g.add_argument("--%s-spi-drive" % x, help="SPI drive strength (0=2mA, 1=4mA, 2=8mA, 3=12mA)", choices=range(4), type=int, default=1, metavar="DRV")
		g.add_argument("--%s-spi-command" % x, help="SPI read command (0=normal, 1=fast, 2=double, 3=quad)", choices=range(4), type=int, default=1, metavar="CMD")
		g.add_argument("--%s-out" % x, help="Write OTA image to file (out)", type=argparse.FileType('wb'), metavar="FILE")

	args = ap.parse_args()

	img0 = bootable_image(get_image_opts(args, 0))
	if args.img1_image:
		img1 = bootable_image(get_image_opts(args, 1))
	else:
		img1 = img0

	if args.image_out:
		args.image_out.write(flashable_image(args, img0, img1))
	if args.img0_out:
		args.img0_out.write(img0)
	if args.img1_out:
		args.img1_out.write(img1)

if __name__ == "__main__":
	main()
