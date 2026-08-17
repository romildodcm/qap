---
applyTo: "firmware/src/ui/**"
description: "Regras e convenções para desenhar a interface do UV-K5 (display ST7565 128x64, 8 páginas de 8px). Aplicar sempre que mexer em telas, fontes, bitmaps ou layout no firmware. Referência completa: docs/ui-design.md"
---

# Guia de UI do UV-K5 — PU5XRM Airband

Regras para criar/alterar telas do rádio. O guia completo está em `docs/ui-design.md`.

## Anatomia do display

- Resolução: **128 × 64 px**, organizado em **8 páginas de 8px**.
- Cada linha de texto ocupa 8px (`PrintSmall`) ou 16px (`PrintMedium`/`PrintBiggestDigits`).
- São **8 linhas** por tela. Não estoure a área.

## Funções de desenho (firmware/src/ui/main.c)

- `PrintSmall(x, y, fmt, ...)` — 8px, texto pequeno.
- `PrintMedium(x, y, fmt, ...)` — 16px, texto médio.
- `PrintMediumBoldEx(...)` — 16px, destaque (label de canal).
- `PrintBiggestDigitsEx(...)` — fonte grande, **exclusiva para a frequência**.
- `FillRect(x, y, w, h, C_FILL | C_CLEAR)` — preencher/limpar área (badge).
- `DrawHLine(x, y, w, C_FILL)` — separador horizontal.
- `UI_RSSIBar(BK4819_GetRSSI(), y)` — S-meter pronto (dBm + S + barra).
- `ST7565_BlitFullScreen()` — envia o buffer ao display (uma vez por frame).

## Regras obrigatórias

1. **Pixels acesos (1) = escuro** no display real; o fundo é a cor do backlight (tema O/W/B).
2. **Badge invertido** = `FillRect` preenchido + `Print*Ex(..., C_INVERT)`.
3. **NUNCA** desenhar texto por cima de outro sem limpar (`C_CLEAR`) antes.
4. Chamar `ST7565_BlitFullScreen()` **só depois** de montar a tela inteira.
5. Limpar **apenas a área que muda** — não a tela toda (evita flicker).
6. Alinhar texto no grid de 8px (y = 0, 8, 16, 24, 32, 40, 48, 56).
7. Elementos de 16px: posicionar em y pares (16, 24, 32, 40) para centralizar.

## Layouts do projeto (não alterar sem necessidade)

- **Frequência única** (Batch 08): badge + RX + step / SQL + AGC / freq grande / label / S-meter / barra.
- **Scan** (Batch 09): mesmo layout do single, trocando badge por `SCAN▶`/`SCAN■`; freq/label mudam com o scan.
- **Tri-freq** (Batch 10): 3 linhas de slot (freq + label + barra) + linha de slot ativo.

## Criando elementos (ícones, símbolos)

- Use o editor web `tools/ui_editor.html` (128x64, fonte 5x7 embutida).
- Gere **HEX** — é o formato padrão do firmware (`const uint8_t gNome[] = {...};` em `bitmaps.c`).
- NÃO usar BINARY no fonte (não é o padrão do projeto).
- `gStatusLine[x] |= 0xNN` só para a barra de status dinâmica.

## Checklist antes de considerar a tela pronta

- [ ] Texto alinhado ao grid de 8px.
- [ ] Nenhum elemento sobreposto sem limpar antes.
- [ ] `ST7565_BlitFullScreen()` chamado uma vez por frame.
- [ ] Build via Docker sem erros/warnings.
- [ ] Tela legível nos 3 temas (O/W/B) — testar contraste.
