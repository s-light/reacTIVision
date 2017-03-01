#!/usr/bin/env python
from argparse import ArgumentParser
from tempfile import NamedTemporaryFile
import os
import filecmp
import shutil

parser = ArgumentParser(description="bin2h")
parser.add_argument("fin", help="file to convert")
parser.add_argument("fout", help="output file")
parser.add_argument("varname", help="variable name")
args = parser.parse_args()

fin = open(args.fin, "rb")
tmpout = NamedTemporaryFile("w", delete=False)

tmpout.write("const char %s[] = {\n" % args.varname)
pos = 0
while True:
	c = fin.read(1)
	if not c: break
	if pos != 0: tmpout.write(",")
		
	tmpout.write("0x%02X" % ord(c))
	pos = pos + 1
	if pos % 20== 0: tmpout.write("\n")

tmpout.write("};\n\n");
tmpout.write("unsigned const int %s_size = %i;\n" % (args.varname, pos))

fin.close()
tmpout.close();

if not os.path.isfile(args.fout) or not filecmp.cmp(tmpout.name, args.fout):
	shutil.copy(tmpout.name, args.fout)

os.remove(tmpout.name)