---
applyTo: "firmware/**"
description: "Regras de segurança obrigatórias para editar o firmware do Quansheng UV-K5. Erros neste código podem danificar fisicamente ('brickar') ou queimar o amplificador de potência (PA) do rádio. Aplicar sempre que mexer em código C, registradores BK4819, EEPROM, GPIO ou processo de build/flash."
---

# Segurança do Firmware — Quansheng UV-K5 (PU5XRM Airband Scanner)

> ⚠️ **AVISO CRÍTICO:** Este é firmware de baixo nível para hardware real. Um erro pode
> **queimar o PA (amplificador de potência)** ou **brickar o rádio permanentemente**
> (bootloader corrompido = irrecuperável). Trate cada alteração como se fosse embarcar
> em hardware físico — porque é.

## Princípios Inegociáveis

1. **NUNCA reabilite TX.** O projeto é RX-only. Transmitir na faixa aeronáutica é ilegal
   e transmitir em hardware não preparado queima o PA.
2. **NUNCA deixe o PA (Power Amplifier) ligado no modo RX.** É a causa #1 de queima.
3. **Leia o arquivo inteiro antes de editar.** Nunca edite registradores ou sequências de
   init "às cegas".
4. **Uma mudança por vez.** Nada de refactors amplos em código de driver/hardware.
5. **Explique o risco antes de aplicar** qualquer mudança em `driver/`, `board.c`,
   `radio.c`, `settings.c` ou no processo de flash.

## Zonas Críticas — Requerem Confirmação Explícita do Humano

### 1. Amplificador de Potência (PA) — RISCO DE QUEIMA
- `radio.c` → `RADIO_SetTxParameters()` habilita o PA (`BK4819_GPIO1_PIN29_PA_ENABLE`).
- `radio.c` → `RADIO_SetupRegisters()` **desliga** o PA no RX. Este desligamento
  **JAMAIS** pode ser removido ou pulado.
- `driver/bk4819.c` → `BK4819_SetupPowerAmplifier()` controla o bias do PA (REG_36).
  Bias = 0 desliga; bias alto (>200) causa runaway térmico.
- **Regra:** PA bias deve ser zero em RX. Nunca escrever no bit PA_ENABLE de REG_36 fora
  da máquina de estados de TX (que estamos removendo, não alterando).

### 2. Registradores BK4819 — RISCO DE TRAVAR RF / EMISSÃO ESPÚRIA
- Escritas via `BK4819_WriteRegister()` em `driver/bk4819.c`.
- Registradores sensíveis: **REG_36** (PA), **REG_30** (PA gain), **REG_38/39**
  (frequência), **REG_33** (GPIO/PA enable), **REG_3F** (máscara de interrupção RX).
- Frequência (REG_38/39) deve ser escrita **antes** de qualquer setup de PA.
- Não fazer read-modify-write incorreto em REG_33 (estado de GPIO deve persistir).

### 3. EEPROM — RISCO DE BRICK / PERDA DE CALIBRAÇÃO
- Escritas via `driver/eeprom.c` → `EEPROM_WriteBuffer()` (respeita bound < 0x2000).
- **NUNCA** escrever fora da faixa `0x0E40–0x1F7F`.
- **NUNCA** zerar a calibração de TX (`0x1ED0–0x1F30`) nem a calibração de bateria
  (`0x1F40–0x1F47`).
- Header de settings (`0x1FF0`) deve sempre persistir.
- Mantenha o check read-before-write existente.

### 4. GPIO / Init de Hardware — RISCO DE TRAVAR I2C / DISPLAY
- `board.c` → `BOARD_GPIO_Init()` e `BOARD_PORTCON_Init()`.
- I2C do BK4819 (C0/C1/C2) nunca como input/floating.
- Não desabilitar SPI0 (display) nem reconfigurar o pino PTT (C5).

### 5. Bateria / Power — RISCO DE HARDWARE
- `helper/battery.c`, `BOARD_ADC_GetBatteryInfo()` — não alterar canais ADC (A9/A14).
- O carregamento é feito por IC externo (TP4056); firmware **não** controla carga.

## Build & Flash Seguro

### Fluxo de build canônico (para o Copilot executar)

Sempre compilar via **Docker** a partir da pasta `firmware/`. Este é o comando padrão
para validar qualquer alteração antes de considerar a tarefa concluída:

```bash
cd firmware

# 1. Dependências externas (só na primeira vez / se ausentes)
[ -d src/external/printf/.git ] || git clone --depth=1 https://github.com/mpaland/printf src/external/printf
[ -d src/external/CMSIS_5/.git ] || git clone --depth=1 https://github.com/ARM-software/CMSIS_5.git src/external/CMSIS_5

# 2. Imagem de build (idempotente; reaproveita cache)
docker build -t uvk5-build .

# 3. Compilar (clean + build)
docker run --rm -v "$PWD:/app" -w /app uvk5-build /bin/sh -c "make clean && make"

# 4. Verificar o artefato de flash
ls -la build/firmware.packed.bin
```

Build nativo (alternativa, se `arm-none-eabi-gcc` + `python3`/`crcmod` instalados):
`cd firmware && make clean && make`.

### Regras de build/flash

- **Sempre** rodar `make clean && make` (nunca só `make`) ao validar uma mudança.
- Um build bem-sucedido termina com o `arm-none-eabi-size` e gera
  `firmware/build/firmware.packed.bin`. Se não gerar, a tarefa NÃO está concluída.
- O artefato de flash é `firmware.packed.bin` (empacotado por `fw-pack.py`: XOR + versão
  + CRC-16). **Nunca** flashear o `.bin` cru quando a ferramenta exige o packed.
- **Registrar datetime do build:** a cada novo binário gerado, anotar a data/hora no
  **início** do relatório entregue ao humano (formato ISO `YYYY-MM-DD HH:MM`, ex.:
  `firmware.packed.bin (2026-08-16 16:45)`). Isso rastreia qual build está no rádio e
  permite comparar iterações ao flashear.
- **Nomear o artefato com datetime:** ao concluir um build, copiar o artefato para um
  arquivo com o datetime no **início** do nome, preservando o `firmware.packed.bin`
  original (que o flasher espera). Formato:
  `YYYYMMDDThh\hhmm-firmware.packed.bin` (ex.: `20260816T16h45-firmware.packed.bin`).
- **O Copilot NÃO flasheia o rádio.** No máximo gera o `.packed.bin` e instrui o humano.
- **Backup obrigatório:** salvar o firmware de fábrica (`.bin` não ofuscado) antes do
  primeiro flash custom.
- **Recuperação:** modo DFU (segurar PTT + Power ao ligar) + re-flash via UART.
  Bootloader (primeiros ~16KB) é protegido — se corromper, o rádio **brica de vez**.

## Checklist Antes de Qualquer Alteração em Hardware

- [ ] Li o arquivo/função inteira e entendi a sequência.
- [ ] A mudança NÃO reabilita TX nem toca no PA em RX.
- [ ] A mudança NÃO escreve em EEPROM fora da faixa segura.
- [ ] A mudança NÃO altera init de GPIO/I2C/SPI sem necessidade.
- [ ] Expliquei o risco ao humano e obtive confirmação para zonas críticas.
- [ ] `make clean && make` compila sem erros/warnings novos.
- [ ] Há caminho de recuperação (backup + DFU) caso o flash falhe.

## Estilo

- Não gerar comentários que apenas repetem o que o código já diz.
- Preferir aritmética de ponto fixo (Q15/Q31) — o Cortex-M0 não tem FPU.

## Planos de Implementação (`plans/`)

Planos ficam em `plans/` com prefixo numérico sequencial: `001-nome.md`, `002-nome.md`.

### Estrutura de um plano

Cada plano contém **batches** numerados. Um batch é uma unidade de trabalho que o Copilot
executa em uma sessão de chat. Cada batch tem:

- **Prompt:** bloco de texto copiável para colar no chat com o agente.
- **Aceite:** critérios objetivos para considerar o batch concluído.
- **Build obrigatório:** todo batch termina com `make clean && make` via Docker.

### Como executar um plano

1. Abrir o plano em `plans/`.
2. Selecionar o agente **Firmware UV-K5 (Modo Seguro)**.
3. Copiar o prompt do batch e colar no chat.
4. O agente executa, compila e reporta.
5. Verificar critérios de aceite antes de avançar para o próximo batch.
6. Executar batches na ordem — cada um depende do anterior.

### Classificação de risco nos batches

- 🟢 Baixo: UI, lógica de scan, constantes, texto.
- 🟡 Médio: RSSI, squelch, AGC, filtros, EEPROM (faixa segura).
- 🔴 Alto: PA/TX, GPIO, init de hardware, build/flash.

Batches 🔴 exigem confirmação explícita do humano antes de aplicar.
