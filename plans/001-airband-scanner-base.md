# Plano de Implementação — PU5XRM Airband Scanner

Executar cada batch na ordem, usando o agente **Firmware UV-K5 (Modo Seguro)**.
Copie o bloco de prompt e cole no chat. Cada batch termina com build Docker obrigatório.

---

## Batch 01 — Validar Build Base [OK] 🟢

```
Valide o build do firmware base sem nenhuma modificação de código.

1. Rode o fluxo de build canônico (Docker) a partir de firmware/.
2. Registre o tamanho do firmware base (saída do arm-none-eabi-size: text, data, bss).
3. Confirme que firmware/build/firmware.packed.bin foi gerado.
4. Liste todas as features habilitadas no Makefile (defines ENABLE_*).
5. Reporte o resultado.
```

**Aceite:** build sem erros, `firmware.packed.bin` gerado, baseline de flash registrado.

---

## Batch 02 — Desabilitar TX + Remapear PTT [OK] 🔴

```
Desabilite completamente a transmissão (TX). O rádio deve ser RX-only.
Remapeie o botão PTT para start/stop scan.

1. Leia radio.c (RADIO_SetTxParameters, RADIO_SetupRegisters) por completo.
2. Leia app/app.c — identifique onde o PTT dispara transmissão.
3. Neutralize o caminho de TX:
   - RADIO_SetTxParameters() deve retornar imediatamente sem habilitar PA.
   - BK4819_GPIO1_PIN29_PA_ENABLE permanece false em RADIO_SetupRegisters.
   - REG_36 nunca recebe bias != 0.
4. Remapeie o handler do PTT em app/app.c:
   - Scaneando → para scan, fica na frequência atual.
   - Parado → inicia scan.
5. Desabilite ENABLE_TX1750 no Makefile.
6. Build Docker. Zero erros.
7. Reporte: arquivos tocados, diff de flash vs baseline.
```

**Aceite:** PA nunca habilitado, REG_36 bias=0 sempre, PTT = start/stop scan, build limpo.

---

## Batch 03 — Remover Features Desnecessárias [OK] 🟡

```
Remova features para liberar flash. Faça UMA remoção por vez, compilando após cada.

Ordem:
1. FM broadcast (app/fm.c, ui/fmradio.c, driver/bk1080.c, refs no menu)
2. NOAA (ENABLE_NOAA=0 ou remover #ifdef)
3. VOICE/Beep (ENABLE_VOICE=0)
4. DTMF calling/paging (app/dtmf.c, refs em app/app.c e menu)
5. AIRCOPY (ENABLE_AIRCOPY=0)
6. VOX (ENABLE_VOX=0)
7. Alarm (handler no menu e app)

Prefira desabilitar via Makefile (ENABLE_*=0) quando possível.
Compile via Docker após cada remoção.
Reporte: flash após todas as remoções vs baseline, espaço liberado total.
```

**Aceite:** 7 features removidas, build limpo, código AM/RX intacto, ~15-18 KB liberados.

---

## Batch 04 — Forçar Faixa Airband + Passo 8.33 kHz [OK] 🟡

```
Configure o firmware para operar exclusivamente na faixa aeronáutica.

1. Leia frequencies.c/h — tabelas de bandas, limites, steps.
2. Restrinja a faixa para 118.000–136.975 MHz.
3. Adicione passo 8.33 kHz (8330 Hz) se não existir. Defina como padrão.
4. Mantenha 25 kHz como opção secundária.
5. Force modulação AM como padrão.
6. Build Docker. Zero erros.
```

**Aceite:** faixa 118–136.975 MHz, step 8.33 kHz padrão, AM padrão, build limpo.

---

## Batch 05 — AM AGC Avançado (32 Níveis) [OK] 🟡

```
Implemente AM AGC avançado substituindo o AM fix existente (~8 degraus).

1. Leia am_fix.c/h por completo.
2. Crie airband_agc.c/h com:
   - 32 níveis de ganho, histerese 3 dB
   - Attack 5ms, decay 200ms
   - gain_table[32] com valores de REG_10 para 118–136 MHz
   - am_agc_update() chamada periodicamente
3. Interpole os valores do AM fix existente para 32 níveis.
4. Substitua a chamada ao AM fix pela am_agc_update().
5. Somente REG_10 e REG_13. NÃO toque em REG_36 (PA).
6. Build Docker. Zero erros.
```

**Aceite:** 32 níveis, histerese, attack/decay, somente REG_10/13, build limpo.

---

## Batch 06 — Squelch Baseado em SNR [OK] 🟡

```
Implemente squelch SNR otimizado para AM.

1. Leia o código de squelch existente — como lê RSSI (REG_67), abre/fecha áudio.
2. Crie airband_squelch.c/h:
   - SNR open: 10 dB, close: 7 dB (histerese 3 dB)
   - Noise floor: média das 16 leituras RSSI mais baixas
   - am_squelch_check() retorna bool
3. Integre onde o squelch original era avaliado.
4. Build Docker. Zero erros.
```

**Aceite:** SNR calculado, histerese 10/7 dB, noise floor estimado, build limpo.

---

## Batch 07 — Scan Inteligente com Prioridade [OK] 🟢

```
Implemente scan inteligente para comunicação ATC.

1. Leia app/chFrScanner.c por completo.
2. Crie airband_scan.c/h com:
   - SCAN_DWELL_MS=90, SCAN_HOLD_MS=3000, SCAN_RESUME_DELAY_MS=2000
   - PRIORITY_REVISIT_RATIO=3
   - scan_channel_t: freq_hz, priority, activity_score, label[8]
   - Dwell adaptativo por activity_score
   - Canais prioridade revisitados 3x mais
   - Resume delay 2s após perder sinal
   - Integração com am_squelch_check()
3. Lista de scan: até 30 canais.
4. Build Docker. Zero erros.
```

**Aceite:** dwell 90ms, hold 3s, resume 2s, prioridade 3x, max 30 canais, build limpo.

---

## Batch 08 — UI: Tela Frequência Única + S-Meter + AGC [OK] 🟢

```
Modifique a tela de frequência única (DisplaySingleVfo em ui/main.c) para o
modo airband scanner. Não crie arquivos novos — adapte a tela existente.

1. Leia ui/main.c — funções DisplaySingleVfo() e UI_RSSIBar().
2. Na tela single-VFO, altere para este layout (128x64 pixels):

   ┌────────────────────────────────┐
   │ VA2   RX       8.33            │ 8px - banda, RX indicator, step
   │ SQL3  AGC-12dB                 │ 8px - squelch + nível AGC
   │                                │
   │        118.800 MHz             │ 16px - frequência (fonte grande)
   │        TWR FOZ                 │ 8px - label do canal (se MR)
   │                                │
   │ -87dBm S7 ████████████░░░░░░  │ 8px - S-meter: dBm + S + barra
   │ AM   W                        │ 8px - modulação, bandwidth
   └────────────────────────────────┘

3. Detalhes por linha:
   - Linha 1: badge VA2 (invertido), "RX" (invertido, só quando recebendo),
     step "8.33" à direita
   - Linha 2: "SQL%d" + "AGC%+ddB" usando AM_fix_get_gain_diff()
     Se monitor: "MONI" no lugar de SQL
   - Linha 3-4: frequência em fonte grande (PrintBiggestDigitsEx), centralizada
   - Linha 5: nome do canal se MR, senão vazio
   - Linha 6: S-meter (UI_RSSIBar já faz: dBm + S-level + barra gráfica)
   - Linha 7: "AM" + bandwidth "W"/"N" — remover TX power, offset, scramble,
     compander, DCS (irrelevantes para RX-only)

4. Build Docker. Zero erros.
```

**Aceite:** layout conforme protótipo, AGC visível, campos TX removidos, build limpo.

---

## Batch 09 — UI: Scan usa mesma tela + Menu Simplificado [OK] 🟢

```
O modo scan reutiliza o MESMO layout do batch 08 (frequência única).
A diferença é que a frequência e o label vão trocando conforme o scan avança.

1. Leia ui/main.c — DisplaySingleVfo().
2. Comportamento durante scan (gScanStateDir != SCAN_OFF):
   - Mesmo layout do batch 08 (freq grande + S-meter + AGC)
   - A frequência e label atualizam automaticamente conforme o scanner
     troca de canal (o firmware já faz isso via gRxVfo)
   - Na linha 1, trocar badge "VA2" por "SCAN ▶" quando scaneando
   - Quando scan para em um canal com sinal, mostrar "SCAN ■" (parado)

   Layout durante scan (mesmo do batch 08, só muda o badge):
   ┌────────────────────────────────┐
   │ SCAN▶  RX       8.33          │ 8px - "SCAN▶" em vez de "VA2"
   │ SQL3  AGC-12dB                │ 8px
   │                                │
   │        120.300 MHz             │ 16px - freq muda conforme scan
   │        APP FOZ                 │ 8px - label muda junto
   │                                │
   │ -87dBm S7 ████████████░░░░░░  │ 8px - S-meter do canal atual
   │ AM   W                        │ 8px
   └────────────────────────────────┘

   - "SCAN▶" = scaneando ativamente
   - "SCAN■" = parou em canal com sinal (hold)
   - Freq e label trocam a cada 90ms (dwell) durante scan
   - Quando detecta sinal, fica parado mostrando aquele canal

3. Simplifique menu: remova itens TX (power, CTCSS encode, shift, bandwidth FM).
   Mantenha: squelch, backlight, step, scan channels, startup mode, about.
4. Build Docker. Zero erros.
```

**Aceite:** scan usa layout idêntico ao batch 08, badge "SCAN▶/■", freq/label trocam automaticamente, menu simplificado, build limpo.

---

## Batch 10 — Modo Tri-Frequência [OK] 🟢

```
Implemente modo tri-frequência via time-slicing no BK4819.

1. Leia docs/arquitetura.md seção 5.2.
2. Estenda airband_scan.c:
   - tri_freq_state_t: freqs[3], rssi[3], signal_present[3], active_slot
   - tri_freq_tick() chamada a cada 25ms (ciclo 75ms):
     - Sinal detectado → fica no slot
     - Sem sinal → rotaciona round-robin
     - Muda freq no BK4819, espera PLL lock (~3ms), lê RSSI

3. Layout tela tri-freq (128x64):

   ┌────────────────────────────────┐
   │ TRI-FREQ  AGC-5dB       8.33  │ 8px - modo + AGC + step
   │                                │
   │ 118.800  TWR FOZ    ████████ │ 12px - slot 1 com barra sinal
   │ 120.300  APP FOZ    ███      │ 12px - slot 2
   │ 121.500  EMERG      ░        │ 12px - slot 3
   │                                │
   │ >> Slot 1 ativo   -87dBm  S7  │ 8px - qual slot tem áudio
   │ AM   SQL3                     │ 8px
   └────────────────────────────────┘

   - Slot com áudio ativo indicado por ">>"
   - Barras proporcionais ao RSSI: ████████ forte, ███ médio, ░ fraco

4. Controles na tela tri-freq:
   - Teclas 1, 2, 3: seleciona o slot para editar (slot fica destacado/piscando)
   - Após selecionar slot, teclas UP/DOWN (lado do rádio): percorre a lista
     de canais de memória para escolher a frequência daquele slot
   - MENU: confirma a seleção e volta ao monitoramento
   - EXIT: cancela edição sem alterar

   Fluxo de edição de slot:
   ┌────────────────────────────────┐
   │ TRI-FREQ  EDIT SLOT 1         │
   │                                │
   │>118.800  TWR FOZ          <<<│ 12px - slot 1 em edição (<<<)
   │ 2: 120.300  APP FOZ      █   │ 12px
   │ 3: 121.500  EMERG        ░   │ 12px
   │                                │
   │ UP/DN: trocar   MENU: OK     │ 8px
   │ EXIT: cancelar                │ 8px
   └────────────────────────────────┘

   - UP/DOWN rola pelas frequências dos canais de memória (M001–M030)
   - A frequência e label do slot selecionado atualizam em tempo real
   - MENU confirma, EXIT cancela

5. Build Docker. Zero erros.
```

**Aceite:** time-slicing 25ms/slot, teclas 1/2/3 selecionam slot, UP/DOWN escolhem freq da memória, MENU confirma, build limpo.

---

## Batch 11 — Startup Mode + Persistência EEPROM [OK] 🟡

```
Implemente persistência de config em EEPROM e startup mode.

⚠️ RISCO MÉDIO: escrita em EEPROM. Somente faixa 0x0E40–0x1F7F.

1. Leia settings.c — como settings são salvos/carregados, endereços usados.
2. Leia driver/eeprom.c — EEPROM_WriteBuffer, limites.
3. Defina persistent_config_t:
   - startup_mode: STARTUP_SCAN ou STARTUP_LAST
   - last_freq_hz, last_op_mode (single/scan/tri)
   - scan_list[30]: freq, priority, label
4. Use endereço na faixa 0x0E40–0x0E67 (FM removido, livre) ou 0x0F00+.
   Verifique conflitos com settings existentes.
5. Salve ao mudar modo/freq. Carregue no boot.
6. STARTUP_SCAN: inicia scaneando. STARTUP_LAST: retoma última freq.
7. NÃO toque em 0x1ED0–0x1F30 (TX cal), 0x1F40–47 (bateria), 0x1FF0 (header).
8. Build Docker. Zero erros.
```

**Aceite:** config persistida em EEPROM (faixa segura), startup configurável, build limpo.

---

## Batch 12 — Frequências Default Pré-carregadas [OK] 🟢

```
Adicione frequências airband default na lista de memória do firmware.

1. Crie airband_channels.c/h com array constante (flash):

   {"TWR FOZ",  118800000, PRIORIDADE_ALTA},
   {"APP FOZ",  120300000, PRIORIDADE_ALTA},
   {"APP FOZ",  119150000, PRIORIDADE_ALTA},
   {"APP FOZ",  129100000, PRIORIDADE_NORMAL},
   {"UNICOM",   123450000, PRIORIDADE_NORMAL},
   {"LIVRE",    123400000, PRIORIDADE_NORMAL},
   {"ACC CWB",  124900000, PRIORIDADE_NORMAL},
   {"ACC CWB",  133800000, PRIORIDADE_NORMAL},
   {"EMERG",    121500000, PRIORIDADE_ALTA},

2. airband_channels_load_defaults() copia para lista de scan ativa (RAM).
3. No primeiro boot (sem config salva em EEPROM), carregue estes defaults.
4. Item no menu: "Reset defaults" para recarregar a lista original.
5. Build Docker. Zero erros.
```

**Aceite:** 9 canais pré-carregados, emergência como prioridade alta, carregados no
primeiro boot, reset via menu, build limpo.
