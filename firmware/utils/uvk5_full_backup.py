#!/usr/bin/env python3
"""Backup completo do Quansheng UV-K5: EEPROM + firmware + frequências parseadas."""

from __future__ import annotations

import struct
import sys
import os
import json
import time
from datetime import datetime
from typing import Optional

try:
    import serial
except ImportError:
    print("pyserial nao encontrado. Rode: ./setup_backup_env.sh")
    sys.exit(1)

MEM_SIZE = 0x2000
MEM_BLOCK = 0x80
FLASH_SIZE = 0xF000  # 60 KB de flash do DP32G030
FLASH_BLOCK = 0x100
XOR_TABLE = [22, 108, 20, 230, 46, 145, 13, 64, 33, 53, 213, 64, 19, 3, 233, 128]

STEPS_HZ = [250, 500, 625, 1000, 1250, 2500, 833, 1000, 5000, 10000, 12500, 25000, 50000]
MOD_NAMES = ["FM", "AM", "USB"]
BW_NAMES = ["Wide", "Narrow"]
POWER_NAMES = ["Low", "Mid", "High"]
OFFSET_DIR = ["Off", "+", "-"]


def xorarr(data: bytes) -> bytes:
    return bytes([b ^ XOR_TABLE[i % 16] for i, b in enumerate(data)])


def crc16_xmodem(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc <<= 1
            if crc & 0x10000:
                crc = (crc ^ 0x1021) & 0xFFFF
    return crc & 0xFFFF


def send_cmd(ser, data: bytes):
    crc = crc16_xmodem(data)
    pkt = struct.pack(">HBB", 0xABCD, len(data), 0) + xorarr(data + struct.pack("<H", crc)) + struct.pack(">H", 0xDCBA)
    ser.write(pkt)


def recv_reply(ser) -> bytes:
    hdr = ser.read(4)
    if len(hdr) != 4 or hdr[0] != 0xAB or hdr[1] != 0xCD:
        return b""
    body = ser.read(int(hdr[2]))
    footer = ser.read(4)
    if len(footer) != 4 or footer[2] != 0xDC or footer[3] != 0xBA:
        return b""
    return xorarr(body)


def say_hello(ser) -> str:
    hello = b"\x14\x05\x04\x00\x6a\x39\x57\x64"
    for _ in range(5):
        send_cmd(ser, hello)
        rep = recv_reply(ser)
        if rep:
            if rep.startswith(b'\x18\x05'):
                return "DFU"
            fw = ""
            for i in range(4, min(28, len(rep))):
                if rep[i] < 0x20 or rep[i] > 0x7E:
                    break
                fw += chr(rep[i])
            return fw
        time.sleep(0.1)
    return ""


def read_eeprom_block(ser, offset: int, length: int) -> bytes:
    cmd = b"\x1b\x05\x08\x00" + struct.pack("<HBB", offset, length, 0) + b"\x6a\x39\x57\x64"
    send_cmd(ser, cmd)
    rep = recv_reply(ser)
    if rep and len(rep) >= 8:
        return rep[8:]
    return b""


def read_full_eeprom(ser) -> bytes:
    print("Lendo EEPROM (8 KB)...")
    eeprom = b""
    addr = 0
    while addr < MEM_SIZE:
        data = read_eeprom_block(ser, addr, MEM_BLOCK)
        if data and len(data) == MEM_BLOCK:
            eeprom += data
            addr += MEM_BLOCK
            print(f"\r  EEPROM: {addr * 100 // MEM_SIZE}%", end="", flush=True)
        else:
            print(f"\n  ERRO no bloco 0x{addr:04X}")
            return b""
    print()
    return eeprom


def bootloader_handshake(ser) -> str:
    """Handshake com o bootloader em modo DFU."""
    ser.timeout = 1
    for _ in range(50):
        ser.write(b'\x05')
        time.sleep(0.02)

    time.sleep(0.1)
    ser.timeout = 2

    resp = ser.read(36)
    if len(resp) >= 4:
        ver = ""
        for b in resp[4:]:
            if 0x20 <= b <= 0x7E:
                ver += chr(b)
            else:
                break
        return ver if ver else "bootloader"
    return ""


def read_flash_block(ser, addr: int, size: int) -> bytes:
    """Le um bloco de flash via bootloader DFU."""
    cmd = struct.pack("<HHH", 0x0B19, addr, size)
    ser.write(cmd)
    time.sleep(0.05)
    resp = ser.read(size + 8)
    if len(resp) >= size:
        return resp[-size:]
    return b""


def read_full_flash(ser) -> bytes:
    """Le firmware flash completo em modo DFU."""
    print("Lendo firmware flash (60 KB)...")
    flash = b""
    addr = 0
    while addr < FLASH_SIZE:
        data = read_flash_block(ser, addr, FLASH_BLOCK)
        if data and len(data) == FLASH_BLOCK:
            flash += data
            addr += FLASH_BLOCK
            print(f"\r  Flash: {addr * 100 // FLASH_SIZE}%", end="", flush=True)
        else:
            print(f"\n  ERRO no bloco flash 0x{addr:04X} - pulando")
            flash += b'\xFF' * FLASH_BLOCK
            addr += FLASH_BLOCK
    print()
    return flash


def parse_channel(data: bytes, index: int) -> Optional[dict]:
    """Parseia um canal de 16 bytes da EEPROM."""
    if len(data) < 16:
        return None
    freq_raw = struct.unpack_from("<I", data, 0)[0]
    if freq_raw == 0 or freq_raw == 0xFFFFFFFF:
        return None
    offset_raw = struct.unpack_from("<I", data, 4)[0]
    freq_mhz = freq_raw * 10 / 1_000_000
    offset_mhz = offset_raw * 10 / 1_000_000

    flags_c = data[0x0C]
    flags_b = data[0x0B]
    step_idx = data[0x0E]

    mod = (flags_b >> 0) & 0x0F
    offset_dir = (flags_b >> 4) & 0x0F
    bw = (flags_c >> 1) & 0x01
    power = (flags_c >> 2) & 0x07

    return {
        "ch": index,
        "freq_mhz": round(freq_mhz, 5),
        "offset_mhz": round(offset_mhz, 5),
        "mod": MOD_NAMES[mod] if mod < len(MOD_NAMES) else f"?{mod}",
        "bw": BW_NAMES[bw] if bw < len(BW_NAMES) else "?",
        "power": POWER_NAMES[power] if power < len(POWER_NAMES) else f"?{power}",
        "offset_dir": OFFSET_DIR[offset_dir] if offset_dir < len(OFFSET_DIR) else "Off",
        "step_idx": step_idx,
    }


def parse_channel_name(eeprom: bytes, index: int) -> str:
    """Le o nome do canal (10 bytes, offset 0x0F50 + index*16)."""
    name_base = 0x0F50
    offset = name_base + index * 16
    if offset + 16 > len(eeprom):
        return ""
    raw = eeprom[offset:offset + 10]
    name = ""
    for b in raw:
        if b == 0x00 or b == 0xFF:
            break
        if 0x20 <= b <= 0x7E:
            name += chr(b)
    return name.strip()


def parse_all_channels(eeprom: bytes) -> list:
    """Parseia todos os canais da EEPROM (ate 200)."""
    channels = []
    for i in range(200):
        offset = i * 16
        if offset + 16 > len(eeprom):
            break
        ch = parse_channel(eeprom[offset:offset + 16], i)
        if ch:
            ch["name"] = parse_channel_name(eeprom, i)
            channels.append(ch)
    return channels


def parse_vfo(eeprom: bytes) -> list:
    """Parseia VFO A e B (canais 200-213)."""
    vfos = []
    for i in range(200, 214):
        offset = i * 16
        if offset + 16 > len(eeprom):
            break
        ch = parse_channel(eeprom[offset:offset + 16], i)
        if ch:
            label = f"VFO-{'A' if (i - 200) < 7 else 'B'} Band{(i - 200) % 7}"
            ch["name"] = label
            vfos.append(ch)
    return vfos


def parse_settings(eeprom: bytes) -> dict:
    """Parseia configuracoes gerais."""
    if len(eeprom) < 0xEA0:
        return {}
    return {
        "squelch": eeprom[0x0E71],
        "max_talk_time": eeprom[0x0E72],
        "vox_switch": eeprom[0x0E75],
        "vox_level": eeprom[0x0E76],
        "mic_gain": eeprom[0x0E77],
        "backlight_min": eeprom[0x0E78] & 0x0F,
        "backlight_max": (eeprom[0x0E78] >> 4) & 0x0F,
        "channel_display_mode": eeprom[0x0E79],
        "battery_save": eeprom[0x0E7B],
        "dual_watch": eeprom[0x0E7C],
        "backlight_time": eeprom[0x0E7D],
        "scan_resume_mode": eeprom[0x0E95],
        "auto_keypad_lock": eeprom[0x0E96],
        "power_on_display": eeprom[0x0E97],
    }


def parse_calibration(eeprom: bytes) -> dict:
    """Extrai dados de calibracao (area critica)."""
    if len(eeprom) < 0x2000:
        return {}
    cal = {}
    cal["battery_cal"] = eeprom[0x1F40:0x1F48].hex()
    cal["has_tx_cal"] = eeprom[0x1ED0:0x1ED8] != b'\xFF' * 8
    cal["header"] = eeprom[0x1FF0:0x2000].hex()
    return cal


def format_channel_table(channels: list) -> str:
    """Formata lista de canais como tabela texto."""
    if not channels:
        return "  (nenhum canal programado)\n"
    lines = []
    lines.append(f"  {'CH':>4}  {'Nome':<10}  {'Freq MHz':>12}  {'Mod':>3}  {'BW':>5}  {'Offset':>10}")
    lines.append(f"  {'----':>4}  {'----------':<10}  {'--------':>12}  {'---':>3}  {'-----':>5}  {'------':>10}")
    for ch in channels:
        offset_str = ""
        if ch["offset_dir"] != "Off" and ch["offset_mhz"] > 0:
            offset_str = f"{ch['offset_dir']}{ch['offset_mhz']:.4f}"
        lines.append(
            f"  {ch['ch']:>4}  {ch['name']:<10}  {ch['freq_mhz']:>12.5f}  {ch['mod']:>3}  {ch['bw']:>5}  {offset_str:>10}"
        )
    return "\n".join(lines) + "\n"


def main():
    port = "/dev/cu.usbserial-210"
    obs = ""

    args = sys.argv[1:]
    for i, arg in enumerate(args):
        if arg in ("-p", "--port") and i + 1 < len(args):
            port = args[i + 1]
        elif arg in ("-o", "--obs") and i + 1 < len(args):
            obs = args[i + 1]

    if not obs:
        obs = input("Observacao para o backup (ex: 'ijv pre-airband'): ").strip()
    if not obs:
        obs = "backup"

    date_str = datetime.now().strftime("%Y%m%d")
    folder_name = f"{date_str} - {obs}"
    base_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "firmware-backup")
    backup_dir = os.path.join(base_dir, folder_name)
    os.makedirs(backup_dir, exist_ok=True)

    print(f"Porta: {port}")
    print(f"Backup: firmware-backup/{folder_name}/")
    print()

    try:
        ser = serial.Serial(port, 38400, timeout=2)
    except Exception as e:
        print(f"ERRO ao abrir porta: {e}")
        sys.exit(1)

    # --- EEPROM (modo normal) ---
    print("=== FASE 1: EEPROM (radio ligado normalmente) ===")
    fw = say_hello(ser)
    if fw == "DFU":
        print("Radio em modo DFU. Desligue, ligue normalmente e tente de novo.")
        ser.close()
        sys.exit(1)
    if not fw:
        print("Nao conseguiu comunicar. Radio ligado? Cabo conectado?")
        ser.close()
        sys.exit(1)

    print(f"Firmware: {fw}")

    eeprom = read_full_eeprom(ser)
    ser.close()

    if not eeprom:
        print("Falha na leitura da EEPROM.")
        sys.exit(1)

    eeprom_file = os.path.join(backup_dir, "eeprom.bin")
    with open(eeprom_file, "wb") as f:
        f.write(eeprom)
    print(f"  Salvo: eeprom.bin ({len(eeprom)} bytes)")

    # --- Parse canais ---
    channels = parse_all_channels(eeprom)
    vfos = parse_vfo(eeprom)
    settings = parse_settings(eeprom)
    calibration = parse_calibration(eeprom)

    report = []
    report.append(f"Backup UV-K5 - {folder_name}")
    report.append(f"Data: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    report.append(f"Firmware: {fw}")
    report.append(f"Porta: {port}")
    report.append(f"EEPROM: {len(eeprom)} bytes")
    report.append("")
    report.append(f"=== CANAIS PROGRAMADOS ({len(channels)}) ===")
    report.append(format_channel_table(channels))
    report.append(f"=== VFOs ({len(vfos)}) ===")
    report.append(format_channel_table(vfos))
    report.append("=== CONFIGURACOES ===")
    for k, v in settings.items():
        report.append(f"  {k}: {v}")
    report.append("")
    report.append("=== CALIBRACAO ===")
    report.append(f"  Bateria: {calibration.get('battery_cal', '?')}")
    report.append(f"  TX cal presente: {calibration.get('has_tx_cal', '?')}")
    report.append(f"  Header: {calibration.get('header', '?')}")

    report_text = "\n".join(report)

    report_file = os.path.join(backup_dir, "relatorio.txt")
    with open(report_file, "w") as f:
        f.write(report_text)
    print(f"  Salvo: relatorio.txt")

    channels_file = os.path.join(backup_dir, "canais.json")
    with open(channels_file, "w") as f:
        json.dump({"firmware": fw, "channels": channels, "vfos": vfos, "settings": settings}, f, indent=2)
    print(f"  Salvo: canais.json")

    # --- Mostrar resumo ---
    print()
    print(f"Canais programados: {len(channels)}")
    if channels:
        print(format_channel_table(channels[:15]))
        if len(channels) > 15:
            print(f"  ... e mais {len(channels) - 15} canais (ver relatorio.txt)")

    # --- Flash/firmware (modo DFU) ---
    print()
    print("=== FASE 2: FIRMWARE FLASH (opcional, requer modo DFU) ===")
    resp = input("Quer fazer backup do firmware flash tambem? (s/n): ").strip().lower()
    if resp == "s":
        print()
        print("Agora coloque o radio em modo DFU:")
        print("  1. Desligue o radio")
        print("  2. Segure PTT + ligue (Power)")
        print("  3. Tela fica preta = modo DFU")
        input("Pressione Enter quando estiver em modo DFU...")

        try:
            ser = serial.Serial(port, 38400, timeout=2)
        except Exception as e:
            print(f"ERRO ao abrir porta: {e}")
            sys.exit(1)

        bl_ver = bootloader_handshake(ser)
        if not bl_ver:
            print("Nao conseguiu conectar ao bootloader.")
            print("Verifique se o radio esta em modo DFU (tela preta).")
            ser.close()
        else:
            print(f"Bootloader: {bl_ver}")
            flash = read_full_flash(ser)
            ser.close()

            if flash:
                flash_file = os.path.join(backup_dir, "firmware_flash.bin")
                with open(flash_file, "wb") as f:
                    f.write(flash)
                print(f"  Salvo: firmware_flash.bin ({len(flash)} bytes)")
            else:
                print("  Falha na leitura do flash.")
    else:
        print("  Pulando backup do flash.")

    print()
    print(f"Backup completo em: firmware-backup/{folder_name}/")
    print("Arquivos:")
    for f in sorted(os.listdir(backup_dir)):
        size = os.path.getsize(os.path.join(backup_dir, f))
        print(f"  {f} ({size} bytes)")


if __name__ == "__main__":
    main()
