# Copyright 2022-2024 NXP
# SPDX-License-Identifier: Apache-2.0

from collections import namedtuple
import re
import argparse
import subprocess
import struct
import binascii
import os
import sys

from io import StringIO


def auto_int(x):
    return int(x, 0)


def get_symbol_value(file, symb_name):
    val = 0

    objdump = subprocess.check_output([args.gnu + 'objdump', '--syms', file])
    objdump = objdump.decode('utf-8')

    symb_re = re.compile(r'^([0-9a-f]{8})[\s\S]+[\s]+([0-9a-f]{8})\s([\w\.]+)')

    for ln in StringIO(objdump):
        m = symb_re.match(ln)
        if m:
            if m.group(3) == symb_name:
                val = int(m.group(1), 16)
                break

    return val


def parse_sections(file):
    sections = {}

    Section = namedtuple('Section', ['idx', 'name', 'size', 'vma', 'lma', 'offset', 'align', 'flags'])

    objdump = subprocess.check_output([args.gnu + 'objdump', '-h', file])
    objdump = objdump.decode('utf-8')

    section_re = re.compile(r'(?P<idx>[0-9]+)\s'
                            r'(?P<name>.{13,})s*'
                            r'(?P<size>[0-9a-f]{8})\s*'
                            r'(?P<vma>[0-9a-f]{8})\s*'
                            r'(?P<lma>[0-9a-f]{8})\s*'
                            r'(?P<offset>[0-9a-f]{8})\s*'
                            r'(?P<align>[0-9*]*)\s*'
                            r'(?P<flags>[\[[\w]*[, [\w]*]*)')

    for match in re.finditer(section_re, objdump):
        sec_dict = match.groupdict()

        sec_dict['idx'] = int(sec_dict['idx'])

        for attr in ['vma', 'lma', 'size', 'offset']:
            sec_dict[attr] = int(sec_dict[attr], 16)

        sec_dict['align'] = eval(sec_dict['align'])

        sections[sec_dict['name']] = Section(**sec_dict)

    return sections


def reverseString2by2(string, stringLength):
    result = ""
    i = stringLength - 1
    while i >= 0:
        result = result + string[i - 1]
        result = result + string[i]
        i = i - 2
    return result


def print_sota_img_directory_cmd(blobNumber):
    nbBlobFound = 0
    sota_final_print = ""
    sota_final_print += "=================> SOTA information\n"
    # Generate the image directory command that will be used to provision the device
    valueToPrint = ".\\DK6Programmer.exe -V5 -s <COM_PORT> -P 1000000 -w PSECT:64@0x160="
    for i in range(1, 9):
        bootable = "00"
        if i == 1:
            bootable = "01"
        position_in_flash = get_symbol_value(elf_file_name, "m_blob_position" + str(i))
        position_in_flash = '%0*X' % (2, position_in_flash)
        blobTargetAddr = get_symbol_value(elf_file_name, "m_blob" + str(i) + "_start")
        blobTargetAddr = '%0*X' % (8, blobTargetAddr)
        blobStatedSize = get_symbol_value(elf_file_name, "m_blob" + str(i) + "_size")
        if position_in_flash != "00" and blobStatedSize != 0:
            nbBlobFound = nbBlobFound + 1
            sota_final_print += "Position in flash = " + position_in_flash + " - targetAddr = 0x" + blobTargetAddr + "\n"
        blobTargetAddr = reverseString2by2(blobTargetAddr, len(blobTargetAddr))
        blobNbPage = blobStatedSize / 512
        blobNbPage = '%0*X' % (4, int(blobNbPage))
        blobNbPage = reverseString2by2(blobNbPage, len(blobNbPage))
        if blobStatedSize != 0:
            valueToPrint = valueToPrint + blobTargetAddr + blobNbPage + bootable + position_in_flash
        else:
            valueToPrint = valueToPrint + "0000000000000000"
    sota_final_print += "=====> Image directory command"
    sota_final_print += valueToPrint
    sota_final_print += "=================> (end) SOTA information"
    if nbBlobFound == blobNumber:
        print(sota_final_print)


#
# JN518x ES1 version
######################
def BuildImageElfDeprecated(args, elf_file_name, bin_file_name, verbose):
    error = 0
    BOOT_BLOCK_MARKER = 0xBB0110BB

    header_struct = struct.Struct('<7LLLLL')
    boot_block_struct = struct.Struct('<6LQ')

    sections = parse_sections(args.in_file)

    last_section = None

    for _, section in sections.items():
        if 'LOAD' in section.flags:
            if last_section is None or section.lma > last_section.lma:
                if section.size > 0:
                    last_section = section

    if args.appcore_image is True:
        image_size = last_section.lma + last_section.size - args.target_appcore_addr
    else:
        image_size = last_section.lma + last_section.size

    dump_section = subprocess.check_output(
        [args.gnu + 'objcopy', '--dump-section', '%s=data.bin' % last_section.name, args.in_file])

    if args.appcore_image is True:
        boot_block = boot_block_struct.pack(BOOT_BLOCK_MARKER, 1, args.target_appcore_addr,
                                            image_size + boot_block_struct.size, 0, 0, 0)
    else:
        boot_block = boot_block_struct.pack(BOOT_BLOCK_MARKER, 0, 0, image_size + boot_block_struct.size, 0, 0, 0)

    with open('data.bin', 'ab') as out_file:
        out_file.write(boot_block)

    update_section = subprocess.check_output(
        [args.gnu + 'objcopy', '--update-section', '%s=data.bin' % last_section.name, args.in_file, args.out_file])

    first_section = None

    for _, section in sections.items():
        if 'LOAD' in section.flags:
            if first_section is None or section.lma < first_section.lma:
                first_section = section

    with open(args.out_file, 'r+b') as elf_file:
        elf_file.seek(first_section.offset)
        vectors = elf_file.read(header_struct.size)

        fields = list(header_struct.unpack(vectors))

        vectsum = 0

        for x in range(7):
            vectsum += fields[x]

        fields[7] = (~vectsum & 0xFFFFFFFF) + 1
        if args.appcore_image is True:
            fields[9] = 0x02794498
        else:
            fields[9] = 0x98447902
        # fields[9]  = 0x98447902
        fields[10] = image_size

        print("Writing checksum {:08x} to file {:s}".format(vectsum, args.out_file))

        elf_file.seek(first_section.offset)
        elf_file.write(header_struct.pack(*fields))
    return error


#
# JN518x ES2 version
######################
def BuildImageElf(args, elf_bin_file, bin_file_name, verbose):
    is_signature = False
    error = 0
    if args.signature_path is not None:
        sign_dir_path = args.signature_path
        if os.path.isdir(sign_dir_path) == False:
            sign_dir_path = os.path.join(os.path.dirname(__file__), args.signature_path)
            if os.path.isdir(sign_dir_path) == False:
                error = 1
            else:
                error = 0
        else:
            error = 0
        priv_key_file_path = os.path.join(sign_dir_path, 'priv_key.pem')
        cert_file_path = os.path.join(sign_dir_path, 'cert.bin')
    else:
        sign_dir_path = os.path.join(os.path.dirname(__file__), '')
        priv_key_file_path = os.path.join(sign_dir_path, 'testkey_es2.pem')
        cert_file_path = os.path.join(sign_dir_path, 'certif_es2')

    if args.key is True:
        from Crypto.Signature import pkcs1_15
        from Crypto.PublicKey import RSA
        from Crypto.Hash import SHA256
        from Crypto.Util import number

        key_file_path = priv_key_file_path
        if verbose:
            print("key path is " + key_file_path)
        if (os.path.isfile(key_file_path)):
            key_file = open(key_file_path, 'r')
            key = RSA.importKey(key_file.read(), args.password)
            print("Private RSA key processing...")
            is_signature = True
        else:
            print("Private key file not found")
            error = 1

    compatibility_struct = struct.Struct('<2L')
    compatibility_len_struct = struct.Struct('<L')
    if args.compatibility_list is not None:
        print("Compatibility list:")
        if verbose:
            print("    {}".format(args.compatibility_list))
        compatibility_list = [list(map(auto_int, compatibility_item.split(","))) for compatibility_item in
                              args.compatibility_list.split(";")]
        print("    Length: {}".format(len(compatibility_list)))
        for i in range(len(compatibility_list)):
            item = compatibility_list[i]
            for j in range(len(item)):
                if j == 1:
                    blobIdCompatibilityList = "     Blob ID 0x=" + '%0*X' % (8, item[j - 1])
                    print("Blob ID =0x" + '%0*X' % (4, item[j - 1]) + " - version =0x" + '%0*X' % (8, item[j]))
        compatibility_len = len(compatibility_list) * compatibility_struct.size + compatibility_len_struct.size
        if verbose:
            print("    {}".format(compatibility_len))
    else:
        compatibility_list = []
        compatibility_len = 0
        print("No compatibility list")

    # make sure that the compatibility list is added and equals to nb_blob - 1
    if args.sota_number_of_blob is not None and (
            compatibility_len // compatibility_struct.size) != args.sota_number_of_blob - 1:
        print("!!! Error the compatibility list length must be = to the number of blobs -1 : " + str(
            compatibility_len / compatibility_struct.size) + "(len) != " + str(args.sota_number_of_blob - 1))
        error = 1
    # make sure that the blob ID is given
    if args.sota_number_of_blob is not None and args.blob_id is None:
        print("!!! Error the blob ID is missing")
        error = 1

    bin_output = subprocess.check_output([args.gnu + 'objcopy', '-O', 'binary', elf_file_name, bin_file_name])

    with open(bin_file_name, 'rb') as in_file:
        input_file = in_file.read()

    BOOT_BLOCK_MARKER = 0xBB0110BB
    IMAGE_HEADER_MARKER = 0x98447902
    IMAGE_HEADER_APP_CORE = 0x02794498
    IMAGE_HEADER_ESCORE = IMAGE_HEADER_MARKER
    SSBL_OR_LEGACY_ADDRESS = 0x00000000
    SSBL_STATED_SIZE = 0x2000
    ZB_TARGET_ADDRESS = SSBL_STATED_SIZE * 2
    ZB_STATED_SIZE = 0x4f000
    BLE_TARGET_ADDRESS = ZB_TARGET_ADDRESS + ZB_STATED_SIZE
    BLE_STATED_SIZE = 0x40000

    header_struct = struct.Struct('<7LLLLL')
    boot_block_struct = struct.Struct('<8L')

    boot_block_marker = BOOT_BLOCK_MARKER
    if args.image_identifier is not None:
        image_iden = args.image_identifier
    else:
        image_iden = 0

    if verbose:
        print("Image Identifier is {:d}".format(image_iden))

    # Default value initialization
    image_addr = 0
    stated_size = 0

    # Default value for SSBL, ZigbeeFull and BleFull image id
    if image_iden == 0:
        image_addr = SSBL_OR_LEGACY_ADDRESS
        stated_size = SSBL_STATED_SIZE
    elif image_iden == 1:
        image_addr = ZB_TARGET_ADDRESS
    elif image_iden == 2:
        image_addr = BLE_TARGET_ADDRESS

    image_addr = get_symbol_value(elf_file_name, 'm_app_start')

    stated_size = get_symbol_value(elf_file_name, 'm_app_size')

    vector_offset = 0

    # compile for Zephyr instead of MCUXpresso?
    if stated_size == 0:
        # Stated-size is limited to available flash
        stated_size = get_symbol_value(elf_file_name, 'CONFIG_FLASH_SIZE') * 1024

        # CONFIG_FLASH_LOAD_SIZE is set when MCUBOOT is enabled and the application
        # should fit within a sub-range of the full flash.
        flash_load_size = get_symbol_value(elf_file_name, 'CONFIG_FLASH_LOAD_SIZE')
        if flash_load_size != 0:
            stated_size = flash_load_size

        image_addr = get_symbol_value(elf_file_name, '__rom_region_start')
        vector_start_addr = get_symbol_value(elf_file_name, '_vector_start')
        vector_offset = vector_start_addr - image_addr

    # In case of SOTA get the position in flash from the linker it should be the app_id
    if args.sota_number_of_blob is not None:
        if args.image_identifier is None:
            image_iden = get_symbol_value(elf_file_name, '__blob_position__')
            print("Blob position in flash = #" + str(image_iden))

    img_header_marker = IMAGE_HEADER_MARKER + image_iden

    # Overwrite defaults for image address, stated size and header marker (-t, -s, -a options)
    if args.target_addr is not None:
        image_addr = args.target_addr
    if args.stated_size is not None:
        stated_size = args.stated_size
    if args.appcore_image is True:
        img_header_marker = IMAGE_HEADER_APP_CORE

    if verbose:
        print("image_iden=%d image_addr=0x%0*X" % (image_iden, 8, image_addr))
        print("stated_size=%d" % (stated_size))
        print("version=0x%0*X" % (8, args.version))
        print("vector_offset=0x%0*X" % (8, vector_offset))
        print("boot_block_marker=0x%0*X" % (8, boot_block_marker))

    sections = parse_sections(elf_file_name)

    last_section = None
    for _, section in sections.items():
        if 'LOAD' in section.flags:
            if last_section is None or section.lma > last_section.lma:
                if section.size > 0:
                    last_section = section

    # IAR toolchain uses odd section names that contain spaces
    # the regexp now may now return trailing spaces too, need to strip them
    last_section_name = last_section.name.rstrip()
    # and add quotes around the section name
    last_section_name = r"%s" % (last_section_name)

    boot_block_offset = last_section.lma + last_section.size + compatibility_len - image_addr

    # Correction for image size not being multiple of 4 (IAR)
    padding_len = ((4 - (boot_block_offset % 4)) & 3)
    padding_bytes = bytearray(padding_len)
    boot_block_offset = boot_block_offset + padding_len

    print("boot block offset = %x" % (boot_block_offset))
    if verbose:
        print("Last Section LMA={:08x} Size={:08x}".format(last_section.lma, last_section.size))
        print("ImageAddress={:08x}".format(image_addr))

    first_section = None

    for _, section in sections.items():
        # print("Section: {:s} {:s} {:x} {:x}".format(name, section.flags, section.lma, section.size))
        if 'LOAD' in section.flags:
            if first_section is None or section.lma < first_section.lma:
                first_section = section

    header = ""
    with open(args.out_file, 'r+b') as elf_file:
        elf_file.seek(first_section.offset + vector_offset)
        vectors = elf_file.read(header_struct.size)

        fields = list(header_struct.unpack(vectors))

        vectsum = 0
        for x in range(7):
            vectsum += fields[x]

        fields[7] = (~vectsum & 0xFFFFFFFF) + 1
        fields[8] = img_header_marker
        fields[9] = boot_block_offset

        # Compute crc
        head_struct = struct.Struct('<10L')
        try:
            if verbose:
                for i in range(10):
                    print("Header[{:d}]= {:08x}".format(i, fields[i]))
            values = head_struct.pack(fields[0],
                                      fields[1],
                                      fields[2],
                                      fields[3],
                                      fields[4],
                                      fields[5],
                                      fields[6],
                                      fields[7],
                                      fields[8],
                                      fields[9])
            fields[10] = binascii.crc32(values) & 0xFFFFFFFF
        except:
            error = 1

        print("Writing checksum {:08x} to file {:s}".format(vectsum, args.out_file))
        print("Writing CRC32 of header {:08x} to file {:s}".format(fields[10], args.out_file))

        elf_file.seek(first_section.offset + vector_offset)
        header = header_struct.pack(*fields)
        elf_file.write(header)

    dump_section = subprocess.check_output([args.gnu + 'objcopy',
                                            '--dump-section',
                                            '%s=data.bin' % last_section_name,
                                            args.out_file])

    compatibility = ""
    compatibility_offset = 0

    if args.compatibility_list is not None:
        compatibility_offset = last_section.lma + last_section.size - image_addr
        compatibility = compatibility_len_struct.pack(len(compatibility_list)) + b''.join(
            [compatibility_struct.pack(*compatibility_item) for compatibility_item in compatibility_list])
        print("Compatibility processing...")

    certificate = ""
    certificate_offset = 0
    signature = ""
    if (args.certificate is True):
        certificate_offset = boot_block_offset + boot_block_struct.size
        certif_file_path = cert_file_path
        if verbose:
            print("Cert key path is " + cert_file_path)
        if (os.path.isfile(certif_file_path)):
            certif_file = open(certif_file_path, 'rb')
            certificate = certif_file.read()

            print("Certificate processing...")
            expected_cert_len = 40 + 256 + 256
            actual_len = len(certificate)
            if actual_len != expected_cert_len:
                print("Certificate error expected expected %d got %d" % (expected_cert_len, actual_len))
                error = 1

    if verbose:
        print("stated size is {:08x} ({})".format(stated_size, stated_size))

    if args.appcore_image is True:
        boot_block_id = 1
    else:
        boot_block_id = 0

    if args.blob_id is not None:
        boot_block_id = args.blob_id

    img_total_len = boot_block_offset + boot_block_struct.size + len(certificate)

    boot_block = boot_block_struct.pack(boot_block_marker,
                                        boot_block_id,
                                        image_addr,
                                        img_total_len,  # padding already included in the boot_block_offset
                                        stated_size,
                                        certificate_offset,
                                        compatibility_offset,
                                        args.version)

    if (is_signature == True):
        # Sign the complete image
        message = header + input_file[header_struct.size:] + boot_block + certificate
        hash = SHA256.new(message)

        out_file_path = os.path.join(os.path.dirname(__file__), 'dump_python.bin')
        file_out = open(out_file_path, 'wb')
        file_out.write(message)

        signer = pkcs1_15.new(key)
        signature = signer.sign(hash)
        img_total_len = img_total_len + len(signature)
        print("Signature processing...")

    with open('data.bin', 'ab') as out_file:
        if isinstance(compatibility, str):
            compatibility = bytes(compatibility, 'utf-8')
        if isinstance(certificate, str):
            certificate = bytes(certificate, 'utf-8')
        if isinstance(signature, str):
            signature = bytes(signature, 'utf-8')
        out_file.write(compatibility + padding_bytes + boot_block + certificate + signature)
    if verbose:
        print("Updating last section " + last_section.name)

    update_section = subprocess.check_output([args.gnu + 'objcopy',
                                              '--update-section',
                                              '%s=data.bin' % last_section_name,
                                              elf_file_name,
                                              args.out_file])

    if (is_signature == True):
        file_out.close()

    bin_output = subprocess.check_output([args.gnu + 'objcopy',
                                          '-O',
                                          'binary',
                                          elf_file_name,
                                          bin_file_name])

    out_file_sz = os.stat(bin_file_name).st_size
    print("Binary size is {:08x} ({})".format(out_file_sz, out_file_sz))

    if args.sota_number_of_blob is not None:
        print_sota_img_directory_cmd(args.sota_number_of_blob)

    if out_file_sz > stated_size:
        print("Error: Binary file size ({:08x}) must be less or equal to stated size {:08x}".format(
            os.stat(bin_file_name).st_size, stated_size))
        error = 1
    if out_file_sz != img_total_len:
        print("File size %d different from expected %d" % (out_file_sz, img_total_len))
        error = 1
    return error


parser = argparse.ArgumentParser(description='DK6 Image Header Generator')
parser.add_argument('in_file',
                    help="Binary to be post-processed: generating header and optionally appending certificate and/or signature.")
parser.add_argument('out_file', nargs='?')
parser.add_argument('-g', '--signature_path',
                    help="Sets directory from which certificate and private key are to be retrieved")
parser.add_argument('-k', '--key', action='store_true',
                    help="2048 bits RSA private key in PEM format used to sign the full image. If -c option is used the full image includes the certificate + the signature of the certificate. The key shall be located in the same directory as the image_tool script. See priv_key.pem example.")
parser.add_argument('-p', '--password',
                    help="This is the pass phrase from which the encryption key is derived. This parameter is only required if key provided through the -k option is a PEM encrypted key.")
parser.add_argument('-c', '--certificate', action='store_true',
                    help="When option is selected, the certificate cert.bin is appended to the image.")
parser.add_argument('-i', '--image_identifier', type=int,
                    help="This parameter is to set the archive identifier. 0: SSBL or legacy JN518x/QN9090 applications, loaded at 0x00000000. 1: application (ZB) loaded at address 0x00004000 by default. 2: application (BLE) image loaded at address 0x00053000 by default")
parser.add_argument('-a', '--appcore_image', action='store_true',
                    help="This parameter is only relevant if dual application (app1 image) shall reside in flash. Do not use in conjunction with -i option.")
parser.add_argument('-t', '--target_addr', type=int,
                    help="Target address of image. Used in conjunction with -i option to override the default set by image identifier, or with -a option to specify address of the appcore image (app1 image).")
parser.add_argument('-s', '--stated_size', type=int,
                    help="This is the stated size of the image in bytes. Default is 0x48000.")
parser.add_argument('-v', '--version', type=auto_int, default=0, help="Image version. Default is 0.")
parser.add_argument('-b', '--verbose', type=int, default=0, help="verbosity level. Default is 0.")
parser.add_argument('-cl', '--compatibility_list', help="Compatibility list")
parser.add_argument('-sota', '--sota_number_of_blob', type=int,
                    help="This parameter is used to generate the image directory command to be provisioned")
parser.add_argument('-bid', '--blob_id', type=auto_int,
                    help="This parameter is to add a blob id. Can be used only if the sota arg is given")
parser.add_argument('-gnu', '--gnu', default="arm-none-eabi-",
                    help="Path and prefix for gnu-tools. When none is provided 'arm-none-eabi-' is used")

args = parser.parse_args()

elf_file_name = args.in_file
bin_file_name = elf_file_name.split(".")[0] + '_temp.bin'

if args.out_file is None:
    args.out_file = elf_file_name
verbose = args.verbose != 0

out_file_path = args.out_file

error = BuildImageElf(args, elf_file_name, bin_file_name, verbose)

if error != 0:
    os.remove(elf_file_name)
    os.remove(out_file_path)
os.remove(bin_file_name)
