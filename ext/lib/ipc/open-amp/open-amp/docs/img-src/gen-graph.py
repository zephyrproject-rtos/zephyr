from graphviz import Digraph
import argparse
import os
import pydot
import sys
import warnings

def gen_graph_from_gv(ifile, odir, oformat="png"):
    (graph,) = pydot.graph_from_dot_file(ifile)
    gen_graph_func = getattr(graph, "write_" + oformat)
    filename = os.path.basename(ifile)
    ofile = odir + "/" + os.path.splitext(filename)[0] + "." + oformat
    gen_graph_func(ofile)

parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('-i', "--infile", action="append",
                    help="graphviz file path")
parser.add_argument('-o', '--outdir',
                    help='sum the integers (default: find the max)')
parser.add_argument('-f', '--outformat', default="png",
        help='output image format (default: png)')

args = parser.parse_args()

# Image source directory
img_src_dir = os.path.dirname(os.path.realpath(sys.argv[0]))

img_files = []
if args.infile:
    for f in args.infile:
        if not os.path.isfile(f):
            f = img_src_dir + "/" + f
        if not os.path.isfile(f):
            warnings.warn("Input file: " + f + " doesn't exist.")
        else:
            img_files.append(f)
else:
    for f in os.listdir(img_src_dir):
        if f.endswith(".gv"):
            img_files.append(img_src_dir + "/" + f)

if not img_files:
    sys.exist("ERROR: no found image files.")

oformat = args.outformat

if args.outdir:
    odir = args.outdir
    if not os.path.isdir(odir):
        sys.exit("--outdir " + odir + "doesn't exist")
else:
    odir = os.path.dirname(img_src_dir) + "/img"

for f in img_files:
    print("Generating " + oformat + " for " + f + " ...")
    gen_graph_from_gv(f, odir, oformat)
