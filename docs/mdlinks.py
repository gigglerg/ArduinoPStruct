#!/usr/bin/python
# script used to correct markdown links to published data that isnt in the same doxygen set
from __future__ import print_function
import sys
import re
import datetime
import getpass
import json
import argparse
import ntpath

# source: https://stackoverflow.com/questions/5574702/how-to-print-to-stderr-in-python
def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def load_file(fn,sp="\n"):
    # load file as array of strings, split by newline
    a = []
    with open(fn, "r") as text_file:
        a = text_file.read().split(sp)
    # last line empty?
    if len(a[-1]) == 0:
        del a[-1]
    return a

def save_file(fn, d):
    # save file as array of strings, split by newline
    a = "".join(str(x) + "\n" for x in d)
    with open(fn, "w") as text_file:
        text_file.write(a)

def reg_check(r):
    # must have subpattern
    if not("?P<replace>" in r):
        raise argparse.ArgumentTypeError
    return r

if __name__ == '__main__':
    # parse shell args
    parser = argparse.ArgumentParser(description="mdlinks.py correction script uses regular expressions to find and replace")
    parser.add_argument("-i", "--input", dest="infile", help="Input markdown file", action="store", required=True)
    parser.add_argument("-e", "--reg", dest="rex", help="Regular expression match named subpattern 'replace'", action="store", required=True, type=reg_check)
    parser.add_argument("-r", "--replace", dest="replace", help="Replace string", action="store", required=True)
    parser.add_argument("-o", "--overwrite", dest="overwrite", help="Overwrite input file with changes", action="store_const", const=True, required=False)
    parser.add_argument("-v", "--verbose", dest="verbose", help="Print extra or debug output", action="store_const", const=True, required=False)
    try:
        args = parser.parse_args()
    except:
        eprint("ERROR: argument parsing??")
        exit(1)

    md = load_file(args.infile)

    # search and replace using regex, line by line
    for i in range(len(md)):
        m = re.search(args.rex, md[i])
        if m:
            if True == args.verbose:
                print(md[i], "'", m.group("replace"), "'")
            lhs = ""
            rhs = ""
            if m.start("replace"):
                lhs = md[i][0:m.start("replace")]
            if m.end("replace") < len(md[i]):
                rhs = md[i][m.end("replace"):]
            md[i] = lhs + str(args.replace) + rhs
            if True == args.verbose:
                print(md[i])

    # optionally save
    if args.overwrite:
        save_file(args.infile, md)
