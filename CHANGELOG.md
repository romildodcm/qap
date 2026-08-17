# Changelog

Todas as mudanças notáveis do firmware e do projeto. O formato segue o
[Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/).

Cada build publica um registro com: data/hora, binário datado, hash sha256 e
mudanças (Added/Changed/Fixed).

---

## [2026-08-16 21:08]

- **Binário**: `20260816T21h08-firmware.packed.bin`
- **Hash sha256**: `0054e37e2fee0c2c80f99567e39d9f81c7702b481dff2efd55f97f7886a2614c`
- **Tamanho**: 51938 bytes (flash < 64KB ✓)

### Changed
- Tela de scan usa o mesmo layout do modo single (`DisplaySingleFreq`).
- Linha 0 (status): AGC único e colado no início; removidos `SCAN`, `PS` e a seta.
- Badge do corpo (`SC▶ RX`) removido durante o scan.
- Nome do canal fixo na página 2 (y 16-22), nunca sobrepõe a frequência.

### Fixed
- AGC duplicado na linha 0 (buffer `str` não era limpo antes do bloco de bateria).
- Sobreposição nome × frequência durante o scan.

### Removed
- Menu "AM Fix" (ficava sempre ativo; item órfão).
- Menu "ScAdd1/2/3" (scan lists já configuradas por lista).

### Changed
- `SCAN_LIST_DEFAULT = 5` (ALL): o scan varre todos os canais MR airband.

---

## [2026-08-15] — Base inicial

- Primeira versão do firmware RX-only airband (PU5XRM v0.1).
