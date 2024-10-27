
# Jailhouse, a Linux-based partitioning hypervisor
#
# Copyright (c) Siemens AG, 2015-2016
#
# Authors:
#  Jan Kiszka <jan.kiszka@siemens.com>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.

import ctypes
import errno
import fcntl
import struct

def config_omnv_fpga():
    filename = os.path.dirname(os.path.abspath(__file__)) + "/../include/config.h"
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith(f"#define CONFIG_OMNV_FPGA"):
                return True
        return False


class JailhousePreloadBitstream(ctypes.Structure):
    _fields_ = [
        ('region', ctypes.c_uint32),
        ('name', ctypes.c_char * 32), 
    ]


class JailhouseCell:
    JAILHOUSE_CELL_CREATE = 0x40100002
    JAILHOUSE_CELL_LOAD = 0x40300003
    JAILHOUSE_CELL_START = 0x40280004

    JAILHOUSE_CELL_ID_UNUSED = -1

    def __init__(self, config):
        self.name = config.name.encode()

        self.dev = open('/dev/jailhouse')

        cbuf = ctypes.create_string_buffer(config.data)
        create = struct.pack('QI4x', ctypes.addressof(cbuf), len(config.data))
        try:
            fcntl.ioctl(self.dev, JailhouseCell.JAILHOUSE_CELL_CREATE, create)
        except IOError as e:
            if e.errno != errno.EEXIST:
                raise e

    def load_image(self, image, address):

        cbuf = ctypes.create_string_buffer(bytes(image))
        if config_omnv_fpga():
            load = struct.pack('i4x32sIIPQQQ8x',
                           JailhouseCell.JAILHOUSE_CELL_ID_UNUSED, self.name,
                           1,0,0,ctypes.addressof(cbuf), len(image), address)
        else:
            load = struct.pack('i4x32sI4xQQQ8x',
                           JailhouseCell.JAILHOUSE_CELL_ID_UNUSED, self.name,
                           1, ctypes.addressof(cbuf), len(image), address)
        fcntl.ioctl(self.dev, self.JAILHOUSE_CELL_LOAD, load)

    
    def load_bitstream(self, bitstream, region):
        bitstream = JailhousePreloadBitstream(region=region, name=bitstream)

        load = struct.pack('i4x32sIIPQQQ8x',
                           JailhouseCell.JAILHOUSE_CELL_ID_UNUSED, self.name,
                           0,1,ctypes.pointer(bitstream),0,0,0)
        fcntl.ioctl(self.dev, self.JAILHOUSE_CELL_LOAD, load)
    
    def start(self):
        start = struct.pack('i4x32s', JailhouseCell.JAILHOUSE_CELL_ID_UNUSED,
                            self.name)
        fcntl.ioctl(self.dev, JailhouseCell.JAILHOUSE_CELL_START, start)
