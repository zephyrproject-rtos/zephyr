#!/usr/bin/env python3
#
# Copyright (c) 2018 Henrik Brix Andersen <henrik@brixandersen.dk>
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import sys

from PIL import ImageFont
from PIL import Image
from PIL import ImageDraw

PRINTABLE_MIN = 32
PRINTABLE_MAX = 127

def generate_element(image, charcode):
    """Generate CFB font element for a given character code from an image"""
    blackwhite = image.convert("1", dither=Image.NONE)
    pixels = blackwhite.load()

    if args.dump:
        blackwhite.save("{}_{}.png".format(args.name, charcode))

    if PRINTABLE_MIN <= charcode <= PRINTABLE_MAX:
        char = " ({:c})".format(charcode)
    else:
        char = ""

    args.output.write("""\t/* {:d}{} */\n\t{{\n""".format(charcode, char))

    for col in range(0, args.width):
        args.output.write("\t\t")
        for octet in range(0, int(args.height / 8)):
            value = ""
            for bit in range(0, 8):
                row = octet * 8 + bit
                if pixels[col, row]:
                    value = "0" + value
                else:
                    value = "1" + value
            args.output.write("0x{:02x},".format(int(value, 2)))
        args.output.write("\n")
    args.output.write("\t},\n")

def extract_font_glyphs():
    """Extract font glyphs from a TrueType/OpenType font file"""
    font = ImageFont.truetype(args.input, args.size)
    for i in range(args.first, args.last + 1):
        image = Image.new("RGB", (args.width, args.height), (255, 255, 255))
        draw = ImageDraw.Draw(image)
        draw.text((0, 0), chr(i), (0, 0, 0), font=font)
        generate_element(image, i)

def extract_image_glyphs():
    """Extract font glyphs from an image file"""
    image = Image.open(args.input)

    x_offset = 0
    for i in range(args.first, args.last + 1):
        glyph = image.crop((x_offset, 0, x_offset + args.width, args.height))
        generate_element(glyph, i)
        x_offset += args.width

def generate_header():
    """Generate CFB font header file"""
    guard = "__CFB_FONT_{:s}_{:d}{:d}_H__".format(args.name.upper(), args.width,
                                                  args.height)

    args.output.write("""/*
 * This file was automatically generated using the following command:
 * {cmd}
 *
 */

#ifndef {guard}
#define {guard}

#include <zephyr.h>
#include <display/cfb.h>

const u8_t cfb_font_{name:s}_{width:d}{height:d}[{elem:d}][{b:.0f}] = {{\n"""
                      .format(cmd=" ".join(sys.argv),
                              guard=guard,
                              name=args.name,
                              width=args.width,
                              height=args.height,
                              elem=args.last - args.first + 1,
                              b=args.width / 8 * args.height))

    if args.type == "font":
        extract_font_glyphs()
    elif args.type == "image":
        extract_image_glyphs()
    elif args.input.name.lower().endswith((".otf", ".otc", ".ttf", ".ttc")):
        extract_font_glyphs()
    else:
        extract_image_glyphs()

    args.output.write("""
}};

FONT_ENTRY_DEFINE({name}_{width}{height},
		  {width},
		  {height},
		  CFB_FONT_MONO_VPACKED,
		  cfb_font_{name}_{width}{height},
		  {first},
		  {last}
);

#endif /* {guard} */""" .format(name=args.name, width=args.width,
                                height=args.height, first=args.first,
                                last=args.last, guard=guard))

def parse_args():
    """Parse arguments"""
    global args
    parser = argparse.ArgumentParser(
        description="Character Frame Buffer (CFB) font header file generator",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        "-d", "--dump", action="store_true",
        help="dump generated CFB font elements as images for preview")

    group = parser.add_argument_group("input arguments")
    group.add_argument(
        "-i", "--input", required=True, type=argparse.FileType('rb'), metavar="FILE",
        help="TrueType/OpenType file or image input file")
    group.add_argument(
        "-t", "--type", default="auto", choices=["auto", "font", "image"],
        help="Input file type (default: %(default)s)")

    group = parser.add_argument_group("font arguments")
    group.add_argument(
        "-s", "--size", type=int, default=10, metavar="POINTS",
        help="TrueType/OpenType font size in points (default: %(default)s)")

    group = parser.add_argument_group("output arguments")
    group.add_argument(
        "-o", "--output", type=argparse.FileType('w'), default="-", metavar="FILE",
        help="CFB font header file (default: stdout)")
    group.add_argument(
        "-x", "--width", required=True, type=int,
        help="width of the CFB font elements in pixels")
    group.add_argument(
        "-y", "--height", required=True, type=int, choices=range(8, 128, 8),
        help="height of the CFB font elements in pixels")
    group.add_argument(
        "-n", "--name", default="custom",
        help="name of the CFB font entry (default: %(default)s)")
    group.add_argument(
        "--first", type=int, default=PRINTABLE_MIN, metavar="CHARCODE",
        help="character code mapped to the first CFB font element (default: %(default)s)")
    group.add_argument(
        "--last", type=int, default=PRINTABLE_MAX, metavar="CHARCODE",
        help="character code mapped to the last CFB font element (default: %(default)s)")

    args = parser.parse_args()

def main():
    """Parse arguments and generate CFB font header file"""
    parse_args()
    generate_header()

if __name__ == "__main__":
    main()
