#!/usr/bin/env python3
# Copy ELF sections into ROM image.
# Reads the given ELF, and if any sections have nonzero VMA and LMA, it copies
# them into the given ROM file at their LMA. This is how we inject our code
# into the ROM.
# It also pads the ROM to 16MB because some things don't like dealing with ROMs
# with unusual sizes.

import argparse
import struct
import sys


def tryhex(n): # TriHard
    try: return hex(n)
    except TypeError: return str(n)


def readSize(sz):
    suffixes = {
        'K': 1024,
        'M': 1024 ** 2,
        'G': 1024 ** 3,
    }
    if sz[-1] in suffixes:
        return int(sz[0:-1]) * suffixes[sz[-1]]
    else:
        return int(sz)


class ELF:
    ei_class_values  = {1:32, 2:64}
    ei_data_values   = {1:'little', 2:'big'}
    e_type_values    = {1:'reloc', 2:'exec', 3:'shared', 4:'core'}
    e_machine_values = {
        0x00: 'generic',
        0x02: 'SPARC',
        0x03: 'x86',
        0x08: 'MIPS',
        0x14: 'PowerPC',
        0x28: 'ARM',
        0x2A: 'SuperH',
        0x32: 'IA-64',
        0x3E: 'x86-64',
        0xB7: 'AArch64',
        0xF3: 'RISC-V',
    }

    p_type_values = {
        0x00: 'null',
        0x01: 'load',
        0x02: 'dynamic',
        0x03: 'interp',
        0x04: 'note',
        0x05: 'shlib',
        0x06: 'phdr',
    }

    p_flags_values = {
        0x01: 'exec',
        0x02: 'write',
        0x04: 'read',
    }

    sh_type_values = {
        0x00: 'null',          # unused entry
        0x01: 'progbits',      # program data
        0x02: 'symtab',        # symbol table
        0x03: 'strtab',        # string table
        0x04: 'rela',          # relocation entries with addends
        0x05: 'hash',          # symbol hash table
        0x06: 'dynamic',       # dynamic linking information
        0x07: 'note',          # notes
        0x08: 'nobits',        # program space with no data (bss)
        0x09: 'rel',           # relocation entries, no addends
        0x0A: 'shlib',         # reserved
        0x0B: 'dynsym',        # dynamic linker symbol table
        0x0E: 'init_array',    # array of constructors
        0x0F: 'fini_array',    # array of destructors
        0x10: 'preinit_array', # array of pre-constructors
        0x11: 'group',         # section group
        0x12: 'symtab_shndx',  # extended section indeces
    }

    sh_flags_values = {
        0x0001: 'write',            # writable
        0x0002: 'alloc',            # occupies memory during execution
        0x0004: 'execinstr',        # executable
        0x0010: 'merge',            # might be merged
        0x0020: 'strings',          # contains null-terminated strings
        0x0040: 'info_link',        # 'sh_info' contains SHT index
        0x0080: 'link_order',       # preserve order after combining
        0x0100: 'os_nonconforming', # non-standard OS-specific handling required
        0x0200: 'group',            # section is member of a group
        0x0400: 'tls',              # section holds thread-local data
    }


    def __init__(self, file):
        self.file  = file
        self._word = ''
        self.readHeader()


    def read(self, offset, length):
        self.file.seek(offset, 0)
        return self.file.read(length)


    def readFields(self, offset, fields, endian=''):
        result = {}
        fmt    = endian + (''.join([field[0] for field in fields]))
        fmt    = fmt.replace('~', self._word)
        length = struct.calcsize(fmt)
        data   = self.read(offset, length)
        data   = struct.unpack(fmt, data)
        for i, field in enumerate(fields):
            result[field[1]] = data[i]
        return result


    def readFlags(self, flags, value):
        result = {}
        for mask, name in flags.items():
            result[name] = (value & mask) != 0
        return result


    def _readHeader1(self):
        fields = (
            ('4s', 'ei_magic'),
            ('B',  'ei_class'),
            ('B',  'ei_data'),
            ('B',  'ei_version'),
            ('B',  'ei_osabi'),
            ('B',  'ei_abiversion'),
        )
        data = self.readFields(0, fields)
        for k, v in data.items():
            setattr(self, k, v)

        # decode data
        if self.ei_magic != b'\x7FELF':
            raise TypeError("Not an ELF file")
        try:
            self.bits    = self.ei_class_values[self.ei_class]
            self.endian  = self.ei_data_values [self.ei_data]
        except KeyError:
            raise ValueError("Unsupported or invalid ELF header")

        # set struct types
        self._word = 'Q' if self.bits == 64 else 'L'
        self._endian = '<' if self.endian == 'little' else '>'

        # print header
        print("ELF class:", self.ei_class, "(%d bits)" % self.bits)
        print("   endian:", self.ei_data, self.endian)
        print("  version:", self.ei_version)
        print("   OS ABI:", self.ei_osabi)
        print("  ABI ver:", self.ei_abiversion)


    def _readHeader2(self):
        fields = (
            # '~' is self._word, determined by `ei_data` from first header
            ('H', 'e_type'),
            ('H', 'e_machine'),
            ('I', 'e_version'),
            ('~', 'e_entry'),
            ('~', 'e_phoff'),
            ('~', 'e_shoff'),
            ('I', 'e_flags'),
            ('H', 'e_ehsize'),
            ('H', 'e_phentsize'),
            ('H', 'e_phnum'),
            ('H', 'e_shentsize'),
            ('H', 'e_shnum'),
            ('H', 'e_shstrndx'),
        )
        data = self.readFields(0x10, fields, self._endian)
        for k, v in data.items():
            setattr(self, k, v)

        # decode data
        self.type    = self.e_type_values   [self.e_type]
        self.machine = self.e_machine_values[self.e_machine]

        print("     type:", self.e_type, self.type)
        print("  machine:", self.e_machine, self.machine)
        print("  version:", self.e_version)
        print("    entry:", hex(self.e_entry))
        print("    phoff:", hex(self.e_phoff))
        print("    shoff:", hex(self.e_shoff))
        print("    flags:", hex(self.e_flags))
        print("   ehsize:", hex(self.e_ehsize))
        print("phentsize:", hex(self.e_phentsize))
        print("    phnum:", hex(self.e_phnum))
        print("shentsize:", hex(self.e_shentsize))
        print("    shnum:", hex(self.e_shnum))
        print(" shstrndx:", hex(self.e_shstrndx))


    def _readProgramHeaders(self):
        fields = [
            ('I', 'type'),
        ]
        if self.bits == 64: fields.append(('I', 'flags')) # WTF
        fields += [
            ('~', 'offset'),
            ('~', 'vaddr'),
            ('~', 'paddr'),
            ('~', 'filesz'),
            ('~', 'memsz'),
        ]
        if self.bits == 32: fields.append(('I', 'flags')) # WTF
        fields.append(('~', 'align'))

        self.programHeaders = []
        offset = self.e_phoff
        for i in range(self.e_phnum):
            data = self.readFields(offset, fields, self._endian)
            try: data['type'] = self.p_type_values[data['type']]
            except KeyError: pass
            data['flags'] = self.readFlags(self.p_flags_values, data['flags'])
            offset += self.e_phentsize
            self.programHeaders.append(data)


    def _readSectionHeaders(self):
        fields = [
            ('I', 'nameoffs'),
            ('I', 'type'),
            ('~', 'flags'),
            ('~', 'addr'),
            ('~', 'offset'),
            ('~', 'size'),
            ('I', 'link'),
            ('I', 'info'),
            ('~', 'addralign'),
            ('~', 'entsize'),
        ]
        self.sectionHeaders = []
        offset = self.e_shoff
        for i in range(self.e_shnum):
            data = self.readFields(offset, fields, self._endian)
            try: data['type'] = self.sh_type_values[data['type']]
            except KeyError: pass

            data['flags'] = self.readFlags(self.sh_flags_values, data['flags'])
            offset += self.e_shentsize
            self.sectionHeaders.append(data)


    def _readSectionNames(self):
        table = self.sectionHeaders[self.e_shstrndx]
        data  = self.read(table['offset'], table['size'])
        for i, sec in enumerate(self.sectionHeaders):
            try:
                offs = sec['nameoffs']
                zero = data[offs:].find(b'\x00')
                sec['name'] = data[offs : offs+zero].decode('utf-8')
            except:
                sec['name'] = '<unable to read name at 0x%X>' % offs


    def getProgHdrForAddr(self, addr):
        for prg in self.programHeaders:
            if prg['vaddr'] >= addr and (prg['vaddr']+prg['memsz']) < addr:
                return prg
        return None


    def printProgramHeaders(self):
        print("\nProgram headers:")
        print("\x1B[4m  #|type    |offset  |vaddr   |paddr   |filesz  "
            "|memsz   |flags        \x1B[0m")
        for i, prg in enumerate(self.programHeaders):
            flags = [name for name,flag in prg['flags'].items() if flag]
            print("%3d|%-8s|%8X|%8X|%8X|%8X|%8X|%s" % (i,
                tryhex(prg['type']),
                prg['offset'],
                prg['vaddr'],
                prg['paddr'],
                prg['filesz'],
                prg['memsz'],
                ','.join(flags),
            ))


    def printSectionHeaders(self):
        print("\nSection headers:")
        print("\x1B[4m  #|name      |type      |addr    |offset|size  |link|"
            "info|algn|entsiz|flags          \x1B[0m")
        for i, sec in enumerate(self.sectionHeaders):
            flags = [name for name,flag in sec['flags'].items() if flag]
            print("%3d|%-10s|%-10s|%8X|%6X|%6X|%4X|%4X|%4X|%6X|%s" % (i,
                sec['name'],
                tryhex(sec['type']),
                sec['addr'],
                sec['offset'],
                sec['size'],
                sec['link'],
                sec['info'],
                sec['addralign'],
                sec['entsize'],
                ','.join(flags),
            ))

    def readHeader(self):
        self._readHeader1()
        self._readHeader2()
        self._readProgramHeaders()
        self._readSectionHeaders()
        self._readSectionNames()


class ROM:
    patchTableOffset  = 0xBF4000
    firstPatchRomAddr = 0xB0C00000
    firstPatchRamAddr = 0x80400000

    def __init__(self, file):
        self.file = file


    def getFreePatchSlot(self):
        prevPatch = self.patchTableOffset
        prevRom   = self.firstPatchRomAddr
        prevRam   = self.firstPatchRamAddr

        self.file.seek(prevPatch)
        while True:
            patch = self.readPatchEntry()
            if patch is None: break
            prevPatch += 16
            romEnd = patch['romAddr'] + patch['size']
            ramEnd = patch['ramAddr'] + patch['size']
            if ramEnd > prevRam: prevRam = ramEnd
            if romEnd > prevRom and patch['romAddr'] != 0: prevRom = romEnd
        return prevPatch, prevRom, prevRam


    def readPatchEntry(self, offset=None):
        if offset is not None: self.file.seek(offset)
        data = self.file.read(16)
        size, romAddr, ramAddr, entry = struct.unpack('>4I', data)
        if size == 0 or size == 0xFFFFFFFF: return None
        return {
            'size':   size,
            'romAddr': romAddr,
            'ramAddr': ramAddr,
            'entry':   entry,
        }


    def writePatchEntry(self, patchAddr, romAddr, ramAddr, entry, data):
        # add a 0 to ensure next entry's size is zero
        entry = struct.pack('>5I', len(data), romAddr, ramAddr, entry, 0)
        self.file.seek(patchAddr)
        self.file.write(entry)


    def printPatchTable(self):
        self.file.seek(self.patchTableOffset)
        nPatch = 0
        print("\nPatch table entries:")
        print("\x1B[4m  #|size  |ROM Addr|RAM Addr|Entry   \x1B[0m")
        while True:
            patch = self.readPatchEntry()
            if patch is None: break
            print("%3d|%06X|%08X|%08X|%08X" % (
                nPatch, patch['size'], patch['romAddr'],
                patch['ramAddr'], patch['entry']))
            nPatch += 1
        print(str(nPatch) + " entries.\n")



    def copyElf(self, elf, entry, addPatchEntry=True):
        if entry is None or entry.lower() == 'none': entry = 0xFFFFFFFF
        else: # XXX allow symbols
            entry = int(entry, 16)

        for i, prg in enumerate(elf.programHeaders):
            if prg['vaddr'] != 0 and prg['paddr'] != 0:
                print(" * program header %d: ROM=0x%06X RAM=0x%08X LEN=0x%08X" % (
                    i, prg['paddr'], prg['vaddr'], prg['filesz'] ))
                data = elf.read(prg['offset'], prg['filesz'])
                #print("0x%06X: %s" % (prg['paddr'], data.hex()))

                if not addPatchEntry:
                    self.file.seek(prg['paddr'])

                else:
                    patchAddr, romAddr, ramAddr = self.getFreePatchSlot()
                    print(" * Patch: tbl=0x%06X rom=0x%06X ram=0x%08X "
                        "len=0x%06X entry=0x%08X" % (
                        patchAddr, romAddr, ramAddr, len(data), entry))
                    self.writePatchEntry(patchAddr, romAddr, ramAddr, entry, data)
                    self.file.seek(romAddr & 0x0FFFFFFF) #256M oughta be enough for anyone

                self.file.write(data)

        #for sec in elf.sectionHeaders: # HACK
        #    print(" * section 0x%08X len=0x%06X: \"%s\"" % (
        #        sec['addr'], sec['size'], sec['name']))
        #    if sec['addr'] > 0 and sec['addr'] < 0x10000000:
        #        data = elf.read(sec['offset'], sec['size'])
        #        file.seek(sec['addr'])
        #        file.write(data)


    def padTo(self, size):
        self.file.seek(size - 1)
        self.file.write(b'\x00')


def getArgs():
    parser = argparse.ArgumentParser(
        description="Patch ELF file into N64 ROM.",
        epilog="This script modifies the given ROM file, so make a backup. "
            "It does NOT correct the ROM header CRC; use crc.py for that.")
    A = parser.add_argument
    A('--get-free', default=False, action='store_true',
        help="Print addresses of next free space and exit.")
    A('--no-load', default=False, action='store_true',
        help="Don't add to patch table. Used for patches that don't need to be "
        "copied into memory at startup.")
    A('--entry', default=None, metavar='OFFSET', help="Patch entry offset. "
        "If specified, loader will call this offset in the patch.")
    A('--pad', default=None, type=readSize, metavar='SIZE',
        help="Pad ROM to this size. eg '--pad 16M'")
    A('rom', type=argparse.FileType('r+b'), nargs='?',
        help="ROM file to patch.")
    A('patch', nargs='?', type=argparse.FileType('rb'),
        help="ELF file to patch into ROM.")

    return parser.parse_args()


def main():
    args = getArgs()

    if args.get_free:
        rom = ROM(args.rom)
        _, romAddr, ramAddr = rom.getFreePatchSlot()
        print("0x%06X 0x%08X" % (romAddr & 0x0FFFFFFF, ramAddr))
        return 0

    elf = ELF(args.patch)
    elf.printProgramHeaders()
    elf.printSectionHeaders()

    rom = ROM(args.rom)
    rom.copyElf(elf, args.entry, not args.no_load)
    rom.printPatchTable()
    if args.pad is not None: rom.padTo(args.pad)


if __name__ == '__main__':
    sys.exit(main())
