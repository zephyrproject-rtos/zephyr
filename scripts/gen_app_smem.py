#!/usr/bin/env python3
import sys
import argparse
import os
import re
import string
from elftools.elf.elffile import ELFFile

partitions=[]
variables=[]

app_smem = "/*\n * The following is a dynamically created linker section.\n" \
            " * Please do not modify or delete.\n */\n"
dRegion_size = { "0x20":32 , \
        "0x40":64, \
        "0x80":128, \
        "0x100":256, \
        "0x200":512, \
        "0x400":1024, \
        "0x800":2048, \
        "0x1000":4096, \
        "0x2000":8192, \
        "0x4000":16384, \
        "0x8000":32768, \
        "0x10000":65536, \
        "0x20000":131072, \
        "0x40000":262144, \
        "0x80000":524288, \
        "0x100000":1048576, \
        "0x200000":2097152, \
        "0x400000":4194304, \
        "0x800000":8388608, \
        "0x1000000":16777216, \
        "0x2000000":33554432, \
        "0x4000000":67108864, \
        "0x8000000":134217728, \
        "0x10000000":268435456, \
        "0x20000000":536870912, \
        "0x40000000":1073741824, \
        "0x80000000":2147483648, \
        "0x100000000":4294967296}

def build_linker_section(filename, regionSizes):
    with open(filename, 'w') as f:
        f.write(app_smem)
        for part in partitions:
            psize = "0"
            if (part.endswith("b")):
                continue
            for partition, size in regionSizes:
                if (partition == str(part).strip("b'")):
                    f.write("\t\t. = ALIGN(" + size + ");\n");
                    psize = size
            f.write("\t\t" + str(part).strip("b'") + " = .;\n")
            f.write("\t\t*(SORT(" + str(part).strip("b'") + "*))\n")
            f.write("\t\t. = ALIGN(_app_data_align);\n")
            f.write("\t\t" + str(part).strip("b'") + "b_end = .;\n")
            f.write("\t\t. = ALIGN(" + psize + ");\n")


def find_variables(filename):
    flag = 0
    with open(filename, 'rb') as f:
        objF = ELFFile( f)
        if (not objF):
            print("Error parsing file: ",filename)
            os.exit(1)
        sec = [ x for x in objF.iter_sections()]
        for s in sec:
            if ("smem" in  s.name and not ".rel" in s.name):
                variables.append( s)


def build_partitions():
    global partitions
    s = set()
    for var in variables:
        s.add(var.name)
    for sec in sorted(s):
        partitions.append(sec)

def calc_sec_size():
    d = dict()
    if not variables:
        return None
    for v in variables:
        if( v.name in d):
            d[v.name] += v.header.sh_size
        else:
            d[v.name] = v.header.sh_size
    return d

def genRegionDef():
    res = []
    dSecs = calc_sec_size()
    if(dSecs is None):
        print("\n***\nError: Failed to identify smem regions in the object files\n***\n")
        sys.exit(1)
    ordered_defs = []
    for key, value in sorted(dRegion_size.items(), key=lambda o: (o[1],o[0]) ):
        ordered_defs.append( ( value, key))

    ltSecs = sorted(dSecs.items())
    szltSecs = len(ltSecs)
    if( szltSecs % 2 == 1):
        szltSecs += 1
    for r in range(int(szltSecs /2 )):
        i = r*2
        sz = ltSecs[i][1] + ltSecs[1+i][1]
        for d in ordered_defs:
            if( sz < d[0]):
                res.append( ( ltSecs[i][0], d[1]))
                break
    return res


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-d", "--directory", required=True,
                        help="Root build directory")
    parser.add_argument("-o", "--output", required=True,
                        help="Output ld file")
    args = parser.parse_args()


def main():
    parse_args()
    startIndex = args.directory
    fileOutput = args.output
    for dirpath, dirs, files in os.walk(startIndex):
        for filename in files:
            if (filename.endswith(".obj") or filename.endswith(".OBJ")):
                fullname = os.path.join(dirpath, filename)
                find_variables(fullname)
    build_partitions()
    build_linker_section(fileOutput, genRegionDef())

if __name__ == '__main__':
    main()
