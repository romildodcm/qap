#!/usr/bin/env python3
"""Reseta o magic byte da config airband para forcar primeiro boot."""

import struct
import sys
import time

try:
    import serial
except ImportError:
    print("pyserial nao encontrado.")
    sys.exit(1)

XOR_TABLE = [22, 108, 20, 230, 46, 145, 13, 64, 33, 53, 213, 64, 19, 3, 233, 128]

def xorarr(data):
    return bytes([b ^ XOR_TABLE[i % 16] for i, b in enumerate(data)])

def crc16(data):
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc <<= 1
            if crc & 0x10000:
                crc = (crc ^ 0x1021) & 0xFFFF
    return crc & 0xFFFF

def send_cmd(ser, data):
    crc = crc16(data)
    pkt = struct.pack(">HBB", 0xABCD, len(data), 0) + xorarr(data + struct.pack("<H", crc)) + struct.pack(">H", 0xDCBA)
    ser.write(pkt)

def recv_reply(ser):
    hdr = ser.read(4)
    if len(hdr) != 4 or hdr[0] != 0xAB or hdr[1] != 0xCD:
        return b""
    body = ser.read(int(hdr[2]))
    ser.read(4)
    return xorarr(body)

def say_hello(ser):
    for _ in range(5):
        send_cmd(ser, b"\x14\x05\x04\x00\x6a\x39\x57\x64")
        rep = recv_reply(ser)
        if rep:
            return True
        time.sleep(0.1)
    return False

def write_mem(ser, offset, data):
    dlen = len(data)
    cmd = b"\x1d\x05" + struct.pack("<BBHBB", dlen+8, 0, offset, dlen, 1) + b"\x6a\x39\x57\x64" + data
    send_cmd(ser, cmd)
    recv_reply(ser)

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/cu.usbserial-210"
    ser = serial.Serial(port, 38400, timeout=2)

    if not say_hello(ser):
        print("Nao conectou ao radio. Ligue normalmente.")
        ser.close()
        sys.exit(1)

    # Reseta o magic byte em 0x0E42 (offset 2 do airband_config_t em 0x0E40)
    # Escreve 8 bytes de 0xFF no endereco 0x0E40 (limpa toda a config airband)
    write_mem(ser, 0x0E40, b'\xFF' * 8)
    print("Magic resetado. No proximo boot, defaults airband serao carregados.")
    ser.close()

if __name__ == "__main__":
    main()
