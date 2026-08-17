# Guia de Design de UI — UV-K5 (PU5XRM Airband Scanner)

## 1. Anatomia do Display

O display ST7565 tem **128 pixels de largura × 64 de altura**, organizado em **8 páginas de 8px**:

```
  0    8    16   24   32   40   48   56   64   72   80   88   96   104  112  120  128
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
│ Página 0 (y 0–7)   → topo / status                                        │ 8px
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
│ Página 1 (y 8–15)                                                          │ 8px
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
│ Página 2 (y 16–23)                                                         │ 8px
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
│ Página 3 (y 24–31)  → frequência grande                                    │ 8px
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
│ Página 4 (y 32–39)  → frequência grande (cont.)                            │ 8px
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
│ Página 5 (y 40–47)  → label do canal                                       │ 8px
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
│ Página 6 (y 48–55)  → S-meter                                               │ 8px
├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
│ Página 7 (y 56–63)  → barra inferior                                       │ 8px
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘
```

**Regra de ouro:** cada linha de texto ocupa 8px. São **8 linhas** por tela. Não estoure.

## 2. Fontes Disponíveis

| Função | Altura | Uso |
|--------|--------|-----|
| `PrintSmall(x, y, fmt, ...)` | 8px | Ícones, texto pequeno (modo, bw, status) |
| `PrintMedium(x, y, fmt, ...)` | 16px | Texto médio (SQL, step, AGC) |
| `PrintMediumBoldEx(...)` | 16px | Labels em destaque (nome do canal) |
| `PrintBiggestDigitsEx(...)` | 16px+ | **Frequência grande** (dígitos) |

> `PrintMedium` e `PrintBiggestDigitsEx` ocupam **2 páginas** (16px).

## 3. Funções de Desenho Essenciais

```c
FillRect(x, y, w, h, C_FILL);                 // preenche retângulo (badge)
FillRect(x, y, w, h, C_CLEAR);                // limpa área
DrawHLine(x, y, w, C_FILL);                   // linha horizontal (separador)
PrintMediumEx(1, 16, POS_L, C_INVERT, "SCAN>"); // texto com fundo invertido
UI_RSSIBar(BK4819_GetRSSI(), y);              // S-meter pronto (dBm + S + barra)
ST7565_BlitFullScreen();                      // envia buffer pro display
```

## 4. Esqueleto de uma Tela

Todas as telas seguem o mesmo padrão no `ui/main.c`:

```c
static void DisplayScanScreen(void) {
    // 1. Limpa só o que muda (evita flicker)
    FillRect(0, 0, 128, 8, C_CLEAR);

    // 2. Topo (badge + status)
    FillRect(0, 0, 27, 8, C_FILL);                    // badge fundo
    PrintMediumEx(2, 8, POS_L, C_INVERT, "SCAN>");   // texto invertido

    // 3. Corpo (frequência, label)
    PrintBiggestDigitsEx(127, 34, POS_R, C_FILL, "%4u.%03u", ...);
    PrintMediumBoldEx(127, 41, POS_R, C_FILL, "%10s", "APP FOZ");

    // 4. S-meter
    UI_RSSIBar(BK4819_GetRSSI(), LCD_HEIGHT - 18);

    // 5. Barra inferior
    DrawHLine(0, LCD_HEIGHT - 8, 128, C_FILL);
    PrintSmall(1, LCD_HEIGHT - 2, "AM");
}
```

## 5. Layout das Telas do Projeto

### 5.1 Tela de Frequência Única (Batch 08)

```
┌────────────────────────────────┐
│ VA2   RX       8.33            │  8px - badge VFO + RX + step
│ SQL3  AGC-12dB                 │  8px - squelch + nível AGC
│        118.800 MHz             │  16px - frequência grande
│        TWR FOZ                 │  8px - label do canal
│ -87dBm S7 ████████████░░░░░░  │  8px - S-meter
│ AM   W                        │  8px - modulação + bandwidth
└────────────────────────────────┘
```

### 5.2 Tela de Scan (Batch 09)

```
┌────────────────────────────────┐
│ SCAN▶  RX       8.33          │  8px - badge SCAN + RX + step
│ SQL3  AGC-12dB                │  8px
│        120.300 MHz             │  16px - freq muda conforme scan
│        APP FOZ                 │  8px - label muda junto
│ -87dBm S7 ████████████░░░░░░  │  8px - S-meter do canal atual
│ AM   W                        │  8px
└────────────────────────────────┘
```

- `SCAN▶` = scaneando ativamente
- `SCAN■` = parou em canal com sinal (hold)

### 5.3 Tela Tri-Frequência (Batch 10)

```
┌────────────────────────────────┐
│ TRI-FREQ  AGC-5dB       8.33  │  8px
│ 118.800  TWR FOZ    ████████ │  12px - slot 1 com barra sinal
│ 120.300  APP FOZ    ███      │  12px - slot 2
│ 121.500  EMERG      ░        │  12px - slot 3
│ >> Slot 1 ativo   -87dBm  S7 │  8px - qual slot tem áudio
│ AM   SQL3                     │  8px
└────────────────────────────────┘
```

- Barras proporcionais ao RSSI: `████████` forte, `███` médio, `░` fraco
- Slot com áudio ativo indicado por `>>`

## 6. Boas Práticas

1. **Pixels acesos (1)** aparecem **escuros** no display real; fundo = cor do backlight
2. **Badge invertido** = `FillRect` preenchido + `Print*Ex(..., C_INVERT)`
3. **NUNCA** desenhe texto por cima de outro sem `C_CLEAR` antes
4. Chame `ST7565_BlitFullScreen()` **depois** de montar a tela inteira (evita rasgo)
5. Limpe **só a área que muda** — não a tela toda (evita flicker)
6. Canais de texto em 8px: alinhe no grid de 8 (0, 8, 16, 24, 32, 40, 48, 56)
7. Elementos de 16px: posicione em y pares (16, 24, 32, 40) para centralizar

## 7. Fluxo de Trabalho

1. Planeje o layout no papel (8 linhas × 128px)
2. Use o **editor web** (`tools/ui_editor.html`) para criar ELEMENTOS (ícones, ★, fontes)
3. Gere **HEX** → cole em `bitmaps.c` como `const uint8_t gNome[] = {...};`
4. Monte a tela em código com as funções de desenho
5. Compile via Docker e valide
