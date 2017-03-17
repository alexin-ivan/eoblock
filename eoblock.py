#!/usr/bin/env python

import fcntl
import os
import struct
import subprocess
import time
import sys
# import eoblock_packet as epkt
# from array import array
import multiprocessing
from zlib import crc32

# Some constants used to ioctl the device file. I got them by a simple C
# program.
TUNSETIFF = 0x400454ca
TUNSETOWNER = TUNSETIFF + 2
IFF_TUN = 0x0001
IFF_TAP = 0x0002
IFF_NO_PI = 0x1000

# Max packet len. Should be ~= MTU.
MAX_PKT_LEN = 32 * (1 << 10)


class epkt:
    @staticmethod
    def empty_pkt():
        return '\x00' * MAX_PKT_LEN

    @staticmethod
    def parse_raw(data):
        if len(data) <= 8:
            return None
        datalen = struct.unpack(">L", data[:4])[0]

        if datalen == 0:
            return None

        cdata = data[:datalen + 4]
        crc_data = data[datalen + 4:datalen + 8]
        if len(crc_data) < 4:
            return None
        pkt_crc = struct.unpack(">L", crc_data)[0]
        calc_crc = crc32(cdata) & ((1 << 30) - 1)
        if calc_crc != pkt_crc:
            print 'crc_mismatch:', hex(calc_crc), hex(pkt_crc)
            return None
        return cdata[4:]

    @staticmethod
    def build(data):
        datalen = len(data) & ((1 << 30) - 1)
        pkt_datalen = struct.pack(">L", datalen)
        pkt_data = data
        pkt_without_crc = pkt_datalen + pkt_data
        crc_value = crc32(pkt_without_crc) & ((1 << 30) - 1)
        pkt_crc = struct.pack(">L", crc_value)
        return pkt_without_crc + pkt_crc


class FTunnel(object):
    def __init__(self, dev_name, node_id, file_name):
        self.__dev_name = dev_name
        self.__file_name = file_name

        if node_id == 1:
            self.my_ip = '192.168.7.1'
            self.f_ip = '192.168.7.2'
            self.my_offset = 0
            self.f_offset = MAX_PKT_LEN

        if node_id == 2:
            self.my_ip = '192.168.7.2'
            self.f_ip = '192.168.7.1'
            self.my_offset = MAX_PKT_LEN
            self.f_offset = 0

    def open(self):
        dev_name = self.__dev_name
        # Open TUN device file.
        tun = open('/dev/net/tun', 'r+b', buffering=0)
        # Tall it we want a TUN device named tun0.
        ifr = struct.pack('16sH', dev_name, IFF_TUN | IFF_NO_PI)
        fcntl.ioctl(tun, TUNSETIFF, ifr)
        # Optionally, we want it be accessed by the normal user.
        # fcntl.ioctl(tun, TUNSETOWNER, 1000)
        # Bring it up and assign addresses.
        subprocess.check_call(
            'ifconfig %s %s pointopoint %s up' % (
                dev_name, self.my_ip, self.f_ip
            ),
            shell=True
        )

        self.tun = tun

    def close(self):
        self.fd.close()

    def main_loop(self):
        q = multiprocessing.Queue()
        pdisk = multiprocessing.Process(
            target=self.loop_gen_disk_events, args=(q,)
        )

        piface = multiprocessing.Process(
            target=self.loop_gen_iface_events, args=(q,)
        )

        pdisk.start()
        piface.start()

        while True:
            e, packet = q.get()
            if e == 'disk':
                print 'file -> iface ', len(packet)
                # transfer message from disk to iface
                os.write(self.tun.fileno(), packet)

            elif e == 'iface':
                # if len(packet) < MAX_PKT_LEN:
                #    packet = packet + '\x00' * (MAX_PKT_LEN - len(packet))
                # transfer message from iface to disk
                fd = open(self.__file_name, "w+b", buffering=0)
                fd.seek(self.f_offset)
                fd.write(packet)
                fd.close()
                print ('iface -> file %d' % (len(packet)))

            else:
                raise Exception("Unknown event %s" % e)

        pdisk.join()
        piface.join()

    def __write_ack(self):
        packet = epkt.empty_pkt()
        fd = open(self.__file_name, "w+b", buffering=0)
        fd.seek(self.my_offset)
        fd.write(packet)
        fd.close()
        # time.sleep(500 / 1000.0)

    def loop_gen_disk_events(self, q):
        while True:
            fd = open(self.__file_name, "r+b", buffering=0)
            fd.seek(self.my_offset)
            d = ''
            while len(d) < MAX_PKT_LEN:
                d += os.read(fd.fileno(), 512)
                pkt = epkt.parse_raw(d)
                if pkt is not None:
                    break
            fd.close()

            if pkt:
                q.put(('disk', pkt))
                self.__write_ack()
                # time.sleep(0.001)
            else:
                time.sleep(0.005)

    def loop_gen_iface_events(self, q):
        while True:
            packet = os.read(self.tun.fileno(), MAX_PKT_LEN)
            pkt = epkt.build(packet)
            q.put(('iface', pkt))


def main():
    name = sys.argv[1]
    node_id = int(sys.argv[2])
    file_name = sys.argv[3]
    tun = FTunnel(name, node_id, file_name)
    tun.open()
    tun.main_loop()
    tun.close()


if __name__ == '__main__':
    main()
