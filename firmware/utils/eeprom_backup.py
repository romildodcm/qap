#!/usr/bin/env python3
"""Backup raw da EEPROM do Quansheng UV-K5 (funciona com qualquer firmware)."""

import struct
import sys
import time
from datetime import datetime

try:
    import serial
except ImportError:
    print("Instale pyserial: pip3 install pyserial")
    sys.exit(1)

MEM_SIZE = 0x2000
MEM_BLOCK = 0x80
XOR_TABLE = [22, 108, 20, 230, 46, 145, 13, 64, 33, 53, 213, 64, 19, 3, 233, 128]


def xorarr(data: bytes) -> bytes:
    ret = b""
    for i, byte in enumerate(data):
        ret += bytes([byte ^ XOR_TABLE[i % len(XOR_TABLE)]])
    return ret


def crc16_xmodem(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc <<= 1
            if crc & 0x10000:
                crc = (crc ^ 0x1021) & 0xFFFF
    return crc & 0xFFFF


def send_command(ser, data: bytes):
    crc = crc16_xmodem(data)
    data2 = data + struct.pack("<H", crc)
    command = struct.pack(">HBB", 0xABCD, len(data), 0) + xorarr(data2) + struct.pack(">H", 0xDCBA)
    ser.write(command)


def receive_reply(ser) -> bytes:
    header = ser.read(4)
    if len(header) != 4 or header[0] != 0xAB or header[1] != 0xCD:
        return b""
    body = ser.read(int(header[2]))
    footer = ser.read(4)
    if len(footer) != 4 or footer[2] != 0xDC or footer[3] != 0xBA:
        return b""
    return xorarr(body)


def say_hello(ser) -> str:
    hello = b"\x14\x05\x04\x00\x6a\x39\x57\x64"
    for attempt in range(5):
        send_command(ser, hello)
        rep = receive_reply(ser)
        if rep:
            if rep.startswith(b'\x18\x05'):
                print("ERRO: Radio em modo programacao (DFU). Reinicie normalmente.")
                sys.exit(1)
            fw = ""
            for i in range(4, min(28, len(rep))):
                c = rep[i]
                if c < 0x20 or c > 0x7E:
                    break
                fw += chr(c)
            return fw
        time.sleep(0.1)
    return ""


def read_mem(ser, offset: int, length: int) -> bytes:
    cmd = b"\x1b\x05\x08\x00" + struct.pack("<HBB", offset, length, 0) + b"\x6a\x39\x57\x64"
    send_command(ser, cmd)
    rep = receive_reply(ser)
    if rep and len(rep) >= 8:
        return rep[8:]
    return b""


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/cu.usbserial-210"
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    outfile = f"eeprom_backup_{timestamp}.bin"

    print(f"Porta: {port}")
    print(f"Ligue o radio NORMALMENTE (NAO em modo DFU)")
    print()

    try:
        ser = serial.Serial(port, 38400, timeout=2)
    except Exception as e:
        print(f"ERRO ao abrir porta: {e}")
        sys.exit(1)

    fw = say_hello(ser)
    if not fw:
        print("ERRO: Nao conseguiu comunicar com o radio.")
        print("Verifique: radio ligado? cabo conectado?")
        ser.close()
        sys.exit(1)

    print(f"Firmware detectado: {fw}")
    print(f"Lendo EEPROM (0x0000-0x{MEM_SIZE:04X})...")

    eeprom = b""
    addr = 0
    while addr < MEM_SIZE:
        data = read_mem(ser, addr, MEM_BLOCK)
        if data and len(data) == MEM_BLOCK:
            eeprom += data
            addr += MEM_BLOCK
            pct = (addr * 100) // MEM_SIZE
            print(f"\r  Progresso: {pct}% (0x{addr:04X}/0x{MEM_SIZE:04X})", end="", flush=True)
        else:
            print(f"\nERRO na leitura do bloco 0x{addr:04X}")
            ser.close()
            sys.exit(1)

    ser.close()
    print()

    with open(outfile, "wb") as f:
        f.write(eeprom)

    print(f"Backup salvo: {outfile} ({len(eeprom)} bytes)")
    print("Guarde este arquivo em local seguro!")


if __name__ == "__main__":
    main()
