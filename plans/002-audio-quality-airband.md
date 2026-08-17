# Plano de Implementação — Qualidade de Áudio/Recepção Airband

**Objetivo:** reduzir o chiado e melhorar a inteligibilidade da fonia aeronáutica AM
(118–136 MHz), sem tocar em TX/PA/EEPROM fora da faixa segura. Foco em filtros IF,
filtro DC, ganho de áudio, squelch e AM fix.

> ⚠️ Executar cada batch na ordem, usando o agente **Firmware UV-K5 (Modo Seguro)**.
> Cada batch termina com **build Docker obrigatório** (`make clean && make`).
> Mudanças de registrador do BK4819 = risco 🟡 (médio): nunca alterar PA (REG_36 bias),
> nunca escrever em EEPROM fora de `0x0E40–0x1F7F`.

---

## Contexto Técnico (estado atual)

| Área | Registrador | Estado atual |
|------|-------------|--------------|
| Filtro IF (bandwidth RX) | REG_43 | WIDE (25k) / NARROW (12.5k), via `gRxVfo->CHANNEL_BANDWIDTH` |
| Filtro DC | REG_7E | DC filter bandwidth Rx = 4 (0x302C) |
| Ganho AF | REG_48 | AF Gain-1=0dB, Gain-2=46, DAC=6 |
| AGC do front-end | REG_10/13/7E | AM fix ativo (`gSetting_AM_fix=true`), controla ganho LNA/mixer/PGA |
| Squelch | REG_4F/78/4E | Squelch AM baseado em ruído (SNR proxy), delay padrão |
| AGC decay | app.c `AM_fix_10ms` | hold ~500ms |

**Mecanismo principal do chiado:** banda IF larga (25k/12.5k) deixa passar ruído fora da
faixa de voz AM (~300Hz–3kHz). O maior ganho é estreitar o filtro IF.

---

## Batch 01 — Filtro IF NARROWER para AM airband 🟡

```
Estreite o filtro IF para AM airband (maior impacto no chiado).

1. Leia driver/bk4819.c -> BK4819_SetFilterBandwidth() (REG_43) e radio.c ->
   RADIO_SetupRegisters() onde o bandwidth é aplicado.
2. Entenda: AM aeronáutico usa espaçamento 8.33kHz; o filtro NARROWER (6.25kHz)
   cobre a voz (~300Hz-3kHz) e corta o ruído fora da banda.
3. No RADIO_SetupRegisters(), quando a modulação for AM e a frequência estiver na
   faixa airband (118-137MHz), force BK4819_FILTER_BW_NARROWER em vez de
   CHANNEL_BANDWIDTH.
4. Mantenha o WIDE/NARROW apenas para FM (se usado) e demais faixas.
5. Build Docker. Zero erros/warnings.
6. Reporte: trecho alterado, registro REG_43 resultante, tamanho do binário.
```

**Aceite:** AM airband usa filtro 6.25kHz, FM inalterado, build limpo, binário datado
(`YYYYMMDDThh\hhmm-firmware.packed.bin`).

---

## Batch 02 — Afinar o Filtro DC (REG_7E) 🟡

```
Reduza ruído de baixa frequência ("boom"/hum) sem distorcer a voz.

1. Leia driver/bk4819.c: BK4819_Init() -> REG_7E (0x302C) e BK4819_SetAGC().
   Identifique os bits do DC filter bandwidth (Rx).
2. Estado atual = 4. Teste valores menores (3, 2) para estreitar o passa-baixa
   do DC filter e reduzir ruído de baixa frequência.
3. Faça UMA alteração por vez (3 -> compile/teste, 2 -> compile/teste).
4. NUNCA altere o bit 15 (AGC fix) nem o AGC fix index (bits 12-14) aqui.
5. Build Docker após cada tentativa.
6. Reporte: valores testados, escolhido, trade-off percebido.
```

**Aceite:** filtro DC ajustado, voz sem distorção evidente, build limpo.

---

## Batch 03 — Reduzir Ganho de Fundo (REG_48) 🟡

```
Abaixe o chiado de fundo sem sinal usando AF Rx Gain-1.

1. Leia REG_48 em driver/bk4819.c (BK4819_Init) e radio.c (RADIO_SetupRegisters).
2. AF Rx Gain-1 (bits 11:10) está em 0 (0dB). Teste -6dB (1) e -12dB (2).
3. Compense o volume subindo o AF Rx Gain-2 (bits 9:4) e/ou DAC Gain (bits 3:0),
   mas preserve a relação sinal/ruído (não subir o ganho de fundo).
4. UMA alteração por vez, compile/teste a cada passo.
5. NUNCA toque nos bits 15:12 (undocumented) do REG_48.
6. Build Docker após cada tentativa.
7. Reporte: valores escolhidos, resultado de áudio.
```

**Aceite:** chiado de fundo menor, voz com volume adequado, build limpo.

---

## Batch 04 — Suavizar Transição do Squelch 🟡

```
Evite "estouro" de ruído nas transições de abertura/fechamento do squelch AM.

1. Leia radio.c -> RADIO_ConfigureSquelchAndOutputPower() e
   driver/bk4819.c -> BK4819_SetupSquelch() (REG_4E: open/close delay).
2. Aumente o delay de abertura (REG_4E bits 13:11) para não abrir em picos de
   ruído, e o delay de fechamento (bits 10:9) para não cortar a fala abruptamente.
3. Ajuste também o glitch threshold (REG_4D/REG_4E <7:0>) se necessário.
4. UMA alteração por vez, compile/teste.
5. Build Docker. Zero erros.
6. Reporte: delays escolhidos, comportamento percebido.
```

**Aceite:** transições de squelch suaves, sem estouro de ruído, build limpo.

---

## Batch 05 — Otimizar AM fix (tabela de ganho) 🟡

```
Revise o controle de ganho do front-end do AM fix para melhorar sinais fracos.

1. Leia am_fix.c: gain_table[] (REG_10) e AM_fix_10ms().
2. Verifique se a tabela cobre bem sinais fracos (não cortar) sem saturar fortes.
3. Ajuste a histerese / ponto de troca de ganho se houver "bombeio" de volume.
4. Confirme que o AGC decay (hold_counter) continua ~500ms (estável).
5. UMA alteração por vez, compile/teste.
6. Build Docker. Zero erros.
7. Reporte: o que foi ajustado, comportamento em sinais fracos/fortes.
```

**Aceite:** AM fix estável, sem bombeio, sinais fracos inteligíveis, build limpo.

---

## Batch 06 — Validar Integral (regressão) 🟢

```
Valide a recepção de ponta a ponta e a segurança do firmware após os batches.

1. Build Docker limpo; confira tamanho do flash < 64KB.
2. Confira que o PA segue desligado no RX (REG_36 bias=0) e que TX continua inativo.
3. Confira strings de versão no binário (PU5XRM, SCANNER v0.1).
4. Gere o binário datado: cp build/firmware.packed.bin
   build/$(date +%Y%m%dT%Hh%M)-firmware.packed.bin
5. Reporte: resumo das mudanças, tamanho, hash sha256, binário datado.
```

**Aceite:** build limpo, PA off no RX, binário datado gerado, hash registrado.

---

## Critérios de Aceite Globais

- [ ] Filtro IF estreito ativo apenas em AM airband (FM/outras faixas intactos).
- [ ] Filtro DC e ganho AF ajustados sem distorção de voz.
- [ ] Squelch AM suave, sem estouro de ruído.
- [ ] AM fix estável (sem bombeio de volume).
- [ ] PA desligado em RX, TX inativo (RX-only preservado).
- [ ] Build `make clean && make` sem erros/warnings novos.
- [ ] Binário datado gerado e hash sha256 registrado.
