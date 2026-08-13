# PU5XRM Quansheng Airband Scanner Firmware

Firmware customizado para o rádio **Quansheng UV-K5(8)** focado exclusivamente em **escaneamento da faixa aeronáutica (118–136 MHz)**.

## Sumário
- [Objetivo](#objetivo)
- [Hardware Alvo](#hardware-alvo)
- [Funcionalidades](#funcionalidades)
- [Base do Firmware](#base-do-firmware)
- [Build](#build)
- [Documentação](#documentação)
- [Licença](#licença)

## Objetivo

Transformar o Quansheng UV-K5(8) em um **scanner airband dedicado**, removendo todas as funcionalidades desnecessárias (TX, FM broadcast, DTMF, etc.) e usando o espaço de flash liberado para otimizações de recepção AM na faixa aeronáutica.

## Hardware Alvo

| Item | Especificação |
|------|--------------|
| Rádio | Quansheng UV-K5(8) |
| Chip RF | BK4819 |
| MCU | DP32G030 (ARM Cortex-M0) |
| Flash | 64 KB |
| Faixa alvo | 118.000 – 136.975 MHz |
| Modulação | AM |
| Passo | 8.33 kHz / 25 kHz |

## Funcionalidades

### Incluídas
- Recepção AM na faixa 118–136 MHz;
- Scan rápido com dwell time otimizado para ATC;
- AM AGC avançado (32 níveis com histerese);
- Filtro DSP de áudio passa-banda (300–3400 Hz);
- Squelch baseado em SNR (otimizado para AM);
- Tabela de ganho calibrada para airband;
- Canais de prioridade (TWR/APP);
- S-meter / indicador RSSI.

### Removidas (para liberar flash)
- Transmissão (TX completamente desabilitado);
- FM broadcast;
- NOAA;
- VOICE/Beep;
- DTMF calling;
- AIRCOPY;
- VOX;
- Alarm.

## Base do Firmware

Fork baseado em [`miramir/uv-k5-firmware`](https://github.com/miramir/uv-k5-firmware), que por sua vez é derivado de:
- [Armel/F4HWN](https://github.com/armel/uv-k5-firmware-custom)
- [egzumer](https://github.com/egzumer/uv-k5-firmware-custom)
- [OneOfEleven](https://github.com/OneOfEleven/uv-k5-firmware-custom)
- [DualTachyon](https://github.com/DualTachyon/uv-k5-firmware) (implementação original)

## Build

### Requisitos
- `arm-none-eabi-gcc` 10.3.1+
- Docker (opcional)

### Compilar
```bash
git submodule update --init --recursive --depth=1
make
```

### Flashear
Usar o [web flasher](https://egzumer.github.io/uvtools) com o arquivo `firmware.packed.bin`.

## Documentação

Detalhes técnicos, decisões de projeto e roadmap disponíveis em [`docs/`](./docs/).

## Licença

Apache License 2.0 — veja [LICENSE](./LICENSE).

---

**Indicativo:** PU5XRM
**Nota:** Este firmware é somente para recepção. Transmitir em frequências aeronáuticas é ilegal.
