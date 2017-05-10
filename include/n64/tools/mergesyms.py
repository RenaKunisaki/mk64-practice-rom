#!/usr/bin/env python3
# Read multiple symbol files and output all symbols from them without duplicates.
# Input files can be in .sym format, eg:
# foo = 0xABCD0123;
# or in `nm` output format, eg:
# ABCD0123 X foo

import argparse
import sys

ignore_syms = (
    '.text', '_stext', '_etext', '_ltext',
    '.data', '_sdata', '_edata', '_ldata',
    '.bss',  '_sbss',  '_ebss',  '_lbss',
    '.boot', '_sboot', '_eboot', '_lboot',
)

def parseLineFromSymFile(line):
    line = line.split('=', 1)
    if len(line) != 2: return None, None
    name = line[0].strip()
    addr = line[1].strip()
    addr = addr.split(';')[0]
    return name, addr


def parseLineFromNm(line):
    line = line.split()
    if len(line) != 3: return None, None
    name = line[2].strip()
    addr = line[0].strip()
    return name, addr


def readSyms(file):
    syms = {}
    while True:
        line = file.readline()
        if line == '': break
        elif '=' in line: name, addr = parseLineFromSymFile(line)
        else: name, addr = parseLineFromNm(line)
        if (name is None) or (addr is None): continue

        syms[name] = addr
    return syms


def generateCodFile(syms):
    for name, addr in syms.items():
        print("0x%08X,%s" % (int(addr, 16), name))


def generateSymFile(syms):
    # sort by address
    items = sorted(syms.items(), key=lambda i: int(i[1], 16))
    for name, addr in items:
        if name not in ignore_syms:
            print("%-32s = 0x%08X;" % (name, int(addr, 16)))


def getArgs():
    parser = argparse.ArgumentParser(
        description="Parse and merge symbol files.",
        epilog="Accepts files in .sym format (as used by `ld`) or "
            "in `nm` output format.")
    A = parser.add_argument

    A('-c', '--cod-file', default=False, action='store_true',
        help="Generate Nemu64 .cod file output. Otherwise, generate .sym file.")

    A('file', type=argparse.FileType('r'), nargs='+',
        help="Symbol files to read.")

    return parser.parse_args()


def main():
    args = getArgs()
    syms = {}
    for file in args.file:
        syms.update(readSyms(file))

    if args.cod_file: generateCodFile(syms)
    else: generateSymFile(syms)


if __name__ == '__main__':
    sys.exit(main())
