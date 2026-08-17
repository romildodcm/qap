# Plano de Implementação — AGC de Áudio + Noise Gate (Fonia Limpa)

**Objetivo:** tornar a fonia aeronáutica AM limpa e uniforme: (1) **noise gate** que
elimina o chiado de fundo no silêncio e (2) **AGC de áudio** que nivela o volume entre
canais fortes e fracos. Usa apenas recursos nativos do BK4819 — não exige DSP de áudio
no MCU.

> ⚠️ Executar na ordem, agente **Firmware UV-K5 (Modo Seguro)**. Cada batch termina com
> **build Docker** (`make clean && make`). Risco 🟡 (registradores de áudio BK4819):
> NUNCA tocar em PA (REG_36 bias) nem em EEPROM fora de `0x0E40–0x1F7F`.

---

## Contexto Técnico (arquitetura de áudio)

Fluxo de áudio no UV-K5:
```
BK4819 (AF out) → amplificador (GPIO AUDIO_PATH, PC4) → speaker
```

O MCU **não** intercepta o áudio. Porém controla, via registradores do BK4819:
- **REG_47** (`BK4819_SetAF`): modo da saída AF — `AF_MUTE` (mudo), `AF_NORMAL`, `AF_BEEP`.
- **REG_28** (`BK4819_SetCompander` mode 2 = RX): **expander 1:2** → atua como **noise gate**
  em hardware: reduz o ganho quando o áudio é fraco (ruído), deixando passar só a voz.
- **REG_29** (compressor): desabilitado no modo RX (só expander ativo).
- **REG_48** (AF gain): `Gain-1`, `Gain-2`, `DAC Gain` — controla o nível de áudio de RX.
- **RSSI** (`BK4819_GetRSSI`): disponível para medir força do sinal (usado no AM fix).

**Estratégia:** noise gate via expander (REG_28) + AGC de áudio ajustando o **REG_48**
dinamicamente conforme o RSSI (sinal fraco → +ganho; sinal forte → −ganho).

---

## Batch 01 — Noise Gate via Expander do BK4819 🟡

```
Ative o noise gate nativo (expander 1:2) no RX para eliminar chiado no silêncio.

1. Leia driver/bk4819.c -> BK4819_SetCompander() (REG_28 expander / REG_29 compressor)
   e onde o RX é configurado (radio.c -> RADIO_SetupRegisters / BK4819_SetupSquelch).
2. Confirme o mapeamento: mode 2 (RX) => expand_ratio=1 (1:2), 0dB=86, noise=56
   (REG_28 = 0x6B38) e REG_31 bit 3 habilita o compander.
3. Ative o expander apenas para RX AM airband (não ativar em outras telas/modos).
   NÃO habilitar o compressor (REG_29) — só o expander (é o noise gate).
4. Compile, teste a fonia: o chiado de fundo no silêncio deve sumir sem cortar a voz.
5. Se a voz ficar "comprimida" demais, ajuste expand_noise_dB / 0dB (REG_28) de forma
   conservadora (uma mudança por vez, compile/teste).
6. Build Docker. Zero erros.
7. Reporte: valor final de REG_28, comportamento percebido.
```

**Aceite:** chiado de fundo ausente no silêncio, voz intacta, build limpo.

---

## Batch 02 — AGC de Áudio (REG_48 dinâmico por RSSI) 🟡

```
Nivele o volume entre canais fortes e fracos ajustando o ganho AF pelo RSSI.

1. Leia REG_48 (AF gain) em driver/bk4819.c e radio.c (RADIO_SetupRegisters).
   Relembre os campos: Gain-1 (bits 11:10), Gain-2 (bits 9:4), DAC (bits 3:0).
2. Crie uma função (ex.: `AM_AudioAGC_apply(uint8_t rssi)`) que mapeia RSSI -> ganho:
   - Sinal forte (RSSI alto) -> reduzir ganho AF (REG_48).
   - Sinal fraco (RSSI baixo) -> aumentar ganho AF (REG_48).
   - Use uma tabela/tabela lookup (NVRAM/const) com histerese p/ evitar "bombeio".
   - Limite o range de REG_48 para nunca chegar em mudo nem em saturação (clip).
3. Chame essa função no tick de RX (ex.: junto ao AM_fix_10ms, a cada ~50-100ms,
   não a cada 10ms, para suavidade). Não escrever REG_48 se o valor não mudou.
4. Mantenha o nível do usuário (volume) como teto: o AGC não deve passar do volume
   configurado pelo usuário (gEeprom.VOLUME_GAIN/DAC_GAIN) em sinal fraco.
5. UMA mudança por vez, compile/teste (forte e fraco, vários canais).
6. Build Docker. Zero erros.
7. Reporte: tabela RSSI->ganho, histerese, comportamento.
```

**Aceite:** volume uniforme entre canais fortes/fracos, sem bombeio nem clip, build limpo.

---

## Batch 03 — Integração e Validação 🟡

```
Junte noise gate + AGC e valide ponta a ponta.

1. Verifique que noise gate (Batch 01) e AGC (Batch 02) coexistem sem conflito:
   o AGC mexe em REG_48; o noise gate em REG_28/REG_31 — registradores distintos.
2. Valide em vários cenários: canal forte, canal fraco, silêncio, transição scan/hold.
3. Confirme que o PA segue desligado no RX e o TX inativo (RX-only preservado).
4. Build Docker limpo; confira flash < 64KB.
5. Gere binário datado:
   cp build/firmware.packed.bin build/$(date +%Y%m%dT%Hh%M)-firmware.packed.bin
6. Reporte: resumo, tamanho, sha256, binário datado.
```

**Aceite:** fonia limpa e uniforme, PA off no RX, binário datado + hash registrado.

---

## Critérios de Aceite Globais

- [ ] Noise gate ativo só em RX AM airband; voz não fica cortada/comprimida.
- [ ] AGC nivela volume entre canais fortes/fracos sem bombeio nem clip.
- [ ] Não excede o volume configurado pelo usuário em sinal fraco.
- [ ] PA desligado em RX, TX inativo (RX-only preservado).
- [ ] Build `make clean && make` sem erros/warnings novos.
- [ ] Binário datado gerado e hash sha256 registrado.
