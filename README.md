<div align="center">

# 📻 QAP — Quansheng Airband Project

**RX-only firmware for the Quansheng UV-K5(8), dedicated to listening to air traffic**

![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)
![RX-only](https://img.shields.io/badge/RX--only-no%20TX-green.svg)
![Band](https://img.shields.io/badge/Band-118%E2%80%93136%20MHz-orange.svg)
![Modulation](https://img.shields.io/badge/Modulation-AM-red.svg)
![Build](https://img.shields.io/badge/build-2026--08--16%2021%3A08-brightgreen.svg)

**QAP** = **Q**uansheng **A**irband **P**roject · **Callsign:** PU5XRM

**🌐 [Leia este README em Português (Brasil)](./README.pt-BR.md)**

![QAP UI](qap.jpg)

*From left to right: **booting**, **scanning** and **hold**.*

</div>

---

Turn your **Quansheng UV-K5(8)** into a **dedicated aviation scanner**, focused exclusively on the **118–136 MHz (AM)** band — tower (TWR), approach (APP), en-route (ACC), ATIS, ground (GND) and the 121.500 MHz emergency frequency.

> ⚠️ **Receive-only.** This firmware never transmits. Transmitting on aviation frequencies is **illegal**.

> 🧪 **Evolving / experimental.** This firmware is under active development — use at **your own risk**. Back up the factory firmware before flashing.

> 📡 **By default**, the firmware ships with **Foz do Iguaçu (SBFI/IGU)** frequencies pre-loaded in the scanner.

- 🎧 **Dedicated AM reception** on 118.000–136.975 MHz, with **8.33 / 25 kHz** steps;
- 📡 **Advanced AM AGC** (32 levels, 3 dB hysteresis) — clean audio from aircraft close by or on the ground;
- 🔊 **300–3400 Hz DSP band-pass filter** — brings out the voice in ATC communications;
- 🔇 **SNR-based squelch**, not RSSI — the right approach for AM (carrier is always present);
- 🔄 **Smart scan** with ATC-optimized dwell time and priority channels (TWR/APP);
- 🖥️ **Clean, dedicated UI** on the ST7565 128×64 display (large frequency font, no visual clutter).

## 📋 Table of Contents

- [📻 QAP — Quansheng Airband Project](#-qap--quansheng-airband-project)
  - [📋 Table of Contents](#-table-of-contents)
  - [🖥️ Target Hardware](#️-target-hardware)
  - [⚙️ Features](#️-features)
    - [✅ Included](#-included)
    - [🗑️ Removed (to free up flash)](#️-removed-to-free-up-flash)
  - [🔌 Installation — Flashing the Radio](#-installation--flashing-the-radio)
    - [1. Download the firmware](#1-download-the-firmware)
    - [2. Back up the original firmware](#2-back-up-the-original-firmware)
    - [3. Flash with the Web Flasher](#3-flash-with-the-web-flasher)
    - [4. Power on and listen](#4-power-on-and-listen)
  - [ Building the Firmware](#-building-the-firmware)
    - [Build with Docker (recommended)](#build-with-docker-recommended)
    - [Build natively](#build-natively)
  - [📁 Repository Structure](#-repository-structure)
  - [📚 Documentation](#-documentation)
  - [📥 Downloads](#-downloads)
  - [⚖️ License and Credits](#️-license-and-credits)

## 🖥️ Target Hardware

| Item | Specification |
|------|--------------|
| Radio | Quansheng **UV-K5(8)** (and compatible variants) |
| RF chip | **BK4819** (18 MHz – 1.3 GHz receive) |
| MCU | DP32G030 (ARM Cortex-M0, 64 KB flash) |
| Display | ST7565 **128×64** (8 pages of 8 px) |
| Target band | **118.000 – 136.975 MHz** |
| Modulation | **AM** |
| Step | 8.33 kHz / 25 kHz |

## ⚙️ Features

### ✅ Included
- Full AM reception on the aviation band;
- Fast scan with ATC-optimized dwell time;
- Advanced AM AGC (32 levels with 3 dB hysteresis);
- 300–3400 Hz DSP audio band-pass filter;
- SNR-based squelch (optimized for AM);
- Airband-calibrated gain table;
- Priority channels (TWR/APP);
- S-meter / RSSI indicator;
- Configurable scan lists (`1/2/3/ALL`);
- Tri-frequency mode and remapped PTT key.

### 🗑️ Removed (to free up flash)
- **Transmission (TX)** — completely disabled in hardware;
- FM broadcast, NOAA, VOICE/Beep;
- DTMF calling/paging, AIRCOPY (RF clone);
- VOX, Alarm and other non-aviation features.

> This frees **~23–30 KB** of flash for the AM reception algorithms.

## 🔌 Installation — Flashing the Radio

### 1. Download the firmware
Grab the latest binary from the **[Releases](https://github.com/romildodcm/qap/releases)** section (the `...-firmware.packed.bin` file).

### 2. Back up the original firmware
Use a flasher to **save the factory firmware** before any flashing. You will need it to restore the radio.

### 3. Flash with the Web Flasher
Use the [web flasher](https://egzumer.github.io/uvtools) (Chrome/Edge via Web Serial) with the downloaded `.packed.bin` file.

### 4. Power on and listen
Done — the radio boots straight into airband scanner mode.

> ⚠️ Use a **quality data cable**. An interrupted flash may require DFU recovery.

##  Building the Firmware

The C source lives in [`firmware/src/`](./firmware/src/). The build is done with **Docker** (works on macOS Intel/Apple Silicon and Linux).

### Build with Docker (recommended)
```bash
cd firmware
docker build -t uvk5-build .
docker run --rm -v "$PWD:/app" -w /app uvk5-build /bin/sh -c "make clean && make"
```

### Build natively
Requirements: `arm-none-eabi-gcc` + `python3` with `crcmod`.
```bash
cd firmware
make clean && make
```

The flashable binary is produced at `firmware/build/firmware.packed.bin` (flash < 64 KB, zero warnings with `-Werror`).

> 🔍 Full build + release guide in [`docs/build-release.md`](./docs/build-release.md).

## 📁 Repository Structure

```
qap/
├── firmware/          # Firmware (C source, Makefile, Dockerfile)
│   └── src/           #   Source code (BK4819 drivers, UI, scan, AGC…)
├── docs/              # Architecture, UI, Brazilian airband frequencies
├── plans/             # Implementation plans (roadmap)
├── CHANGELOG.md       # Build history
├── CNAME              # qap.romildo.net domain
├── LICENSE            # Apache 2.0
└── NOTICE             # Credits to the base projects
```

## 📚 Documentation

| Document | Contents |
|----------|----------|
| [`docs/arquitetura.md`](./docs/arquitetura.md) | Technical decisions, BK4819, flash budget |
| [`docs/ui-design.md`](./docs/ui-design.md) | ST7565 128×64 UI specification |
| [`docs/frequencias-airband-brasil.md`](./docs/frequencias-airband-brasil.md) | Brazilian ATC frequencies (TWR/APP/ACC) + suggested scan |
| [`CHANGELOG.md`](./CHANGELOG.md) | Build history with hash and dated binary |

## 📥 Downloads

Each build publishes a **dated binary** with sha256 hash in the Releases section:

- **Current build (2026-08-16):** `20260816T21h08-firmware.packed.bin`
- **Hash:** `0054e37e2fee0c2c80f99567e39d9f81c7702b481dff2efd55f97f7886a2614c`
- **Size:** 51,938 bytes (flash < 64 KB ✓)

Direct link to the latest release:
```
https://github.com/romildodcm/qap/releases/latest/download/firmware.packed.bin
```

## ⚖️ License and Credits

Distributed under the **Apache License 2.0** — see [`LICENSE`](./LICENSE) and [`NOTICE`](./NOTICE).

Fork based on:
- [DualTachyon](https://github.com/DualTachyon/uv-k5-firmware) — original implementation;
- [OneOfEleven](https://github.com/OneOfEleven/uv-k5-firmware-custom);
- [egzumer](https://github.com/egzumer/uv-k5-firmware-custom);
- [Armel/F4HWN](https://github.com/armel/uv-k5-firmware-custom);
- [miramir/uv-k5-firmware](https://github.com/miramir/uv-k5-firmware) — base of this fork.

---

<div align="center">

**QAP — Quansheng Airband Project** · Aviation reception with the UV-K5 · **PU5XRM**

*This project is for listening only. Do not transmit on aviation frequencies — it is illegal and dangerous.*

</div>


