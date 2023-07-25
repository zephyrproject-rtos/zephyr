#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
import subprocess
import tempfile
import argparse
import os
import string
import sys

quartus_cpf_template = """<?xml version="1.0" encoding="US-ASCII" standalone="yes"?>
<cof>
	<output_filename>${OUTPUT_FILENAME}</output_filename>
	<n_pages>1</n_pages>
	<width>1</width>
	<mode>14</mode>
	<sof_data>
		<user_name>Page_0</user_name>
		<page_flags>1</page_flags>
		<bit0>
			<sof_filename>${SOF_FILENAME}<compress_bitstream>1</compress_bitstream></sof_filename>
		</bit0>
	</sof_data>
	<version>10</version>
	<create_cvp_file>0</create_cvp_file>
	<create_hps_iocsr>0</create_hps_iocsr>
	<auto_create_rpd>0</auto_create_rpd>
	<rpd_little_endian>1</rpd_little_endian>
	<options>
		<map_file>1</map_file>
	</options>
	<MAX10_device_options>
		<por>0</por>
		<io_pullup>1</io_pullup>
		<config_from_cfm0_only>0</config_from_cfm0_only>
		<isp_source>0</isp_source>
		<verify_protect>0</verify_protect>
		<epof>0</epof>
		<ufm_source>2</ufm_source>
		<ufm_filepath>${KERNEL_FILENAME}</ufm_filepath>
	</MAX10_device_options>
	<advanced_options>
		<ignore_epcs_id_check>2</ignore_epcs_id_check>
		<ignore_condone_check>2</ignore_condone_check>
		<plc_adjustment>0</plc_adjustment>
		<post_chain_bitstream_pad_bytes>-1</post_chain_bitstream_pad_bytes>
		<post_device_bitstream_pad_bytes>-1</post_device_bitstream_pad_bytes>
		<bitslice_pre_padding>1</bitslice_pre_padding>
	</advanced_options>
</cof>
"""

# XXX Do we care about FileRevision, DefaultMfr, PartName? Do they need
# to be parameters? So far seems to work across 2 different boards, leave
# this alone for now.
quartus_pgm_template = """/* Quartus Prime Version 16.0.0 Build 211 04/27/2016 SJ Lite Edition */
JedecChain;
	FileRevision(JESD32A);
	DefaultMfr(6E);

	P ActionCode(Cfg)
		Device PartName(10M50DAF484ES) Path("${POF_DIR}/") File("${POF_FILE}") MfrSpec(OpMask(1));

ChainEnd;

AlteraBegin;
	ChainType(JTAG);
AlteraEnd;"""


def create_pof(input_sof, kernel_hex):
    """given an input CPU .sof file and a kernel binary, return a file-like
    object containing .pof data suitable for flashing onto the device"""

    t = string.Template(quartus_cpf_template)
    output_pof = tempfile.NamedTemporaryFile(suffix=".pof")

    input_sof = os.path.abspath(input_sof)
    kernel_hex = os.path.abspath(kernel_hex)

    # These tools are very stupid and freak out if the desired filename
    # extensions are used. The kernel image must have extension .hex

    with tempfile.NamedTemporaryFile(suffix=".cof") as temp_xml:

        xml = t.substitute(SOF_FILENAME=input_sof,
                           OUTPUT_FILENAME=output_pof.name,
                           KERNEL_FILENAME=kernel_hex)

        temp_xml.write(bytes(xml, 'UTF-8'))
        temp_xml.flush()

        cmd = ["quartus_cpf", "-c", temp_xml.name]
        try:
            subprocess.check_output(cmd)
        except subprocess.CalledProcessError as cpe:
            sys.exit(cpe.output.decode("utf-8") +
                     "\nFailed to create POF file")

    return output_pof


def flash_kernel(device_id, input_sof, kernel_hex):
    pof_file = create_pof(input_sof, kernel_hex)

    with tempfile.NamedTemporaryFile(suffix=".cdf") as temp_cdf:
        dname, fname = os.path.split(pof_file.name)
        t = string.Template(quartus_pgm_template)
        cdf = t.substitute(POF_DIR=dname, POF_FILE=fname)
        temp_cdf.write(bytes(cdf, 'UTF-8'))
        temp_cdf.flush()
        cmd = ["quartus_pgm", "-c", device_id, temp_cdf.name]
        try:
            subprocess.check_output(cmd)
        except subprocess.CalledProcessError as cpe:
            sys.exit(cpe.output.decode("utf-8") +
                     "\nFailed to flash image")
    pof_file.close()

def main():
    parser = argparse.ArgumentParser(description="Flash zephyr onto Altera boards", allow_abbrev=False)
    parser.add_argument("-s", "--sof",
            help=".sof file with Nios II CPU configuration")
    parser.add_argument("-k", "--kernel",
            help="Zephyr kernel image to place into UFM in Intel HEX format")
    parser.add_argument("-d", "--device",
            help="Remote device identifier / cable name. Default is "
                 "USB-BlasterII. Run jtagconfig -n if unsure.",
            default="USB-BlasterII")

    args = parser.parse_args()

    flash_kernel(args.device, args.sof, args.kernel)


if __name__ == "__main__":
    main()
