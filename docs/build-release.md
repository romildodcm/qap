# Fluxo de Build + Release + Changelog

Processo padronizado para gerar um binário, registrá-lo e publicá-lo para download.

## 1. Build

A partir de `firmware/`, compilar via Docker (limpo):

```bash
cd firmware
docker build -t uvk5-build .   # idempotente
docker run --rm -v "$PWD:/app" -w /app uvk5-build /bin/sh -c "make clean && make"
```

Critérios: termina com `arm-none-eabi-size`, gera `firmware/build/firmware.packed.bin`,
flash < 64KB, zero warnings com `-Werror`.

## 2. Binário datado + hash

Cada build gera uma **cópia datada** e o hash sha256:

```bash
cd firmware
NOW=$(date +%Y%m%dT%Hh%M)
cp build/firmware.packed.bin "build/${NOW}-firmware.packed.bin"
shasum -a 256 build/firmware.packed.bin
```

Convenção de nome: `YYYYMMDDTHHhMM-firmware.packed.bin` (ex.: `20260816T21h08-firmware.packed.bin`).

## 3. Changelog

Antes de qualquer commit/publicação, registrar a build no `CHANGELOG.md` (raiz):

```markdown
## [YYYY-MM-DD HH:MM]

- **Binário**: `YYYYMMDDTHHhMM-firmware.packed.bin`
- **Hash sha256**: `...`
- **Tamanho**: NNN bytes

### Added / Changed / Fixed / Removed
- ...
```

**Regra:** nunca publicar build sem registro no changelog.

## 4. Release no GitHub

Publicar o binário como **GitHub Release** (download direto na página):

- Título: data/hora (ex.: `2026-08-16 21:08`).
- Asset: `...-firmware.packed.bin`.
- Corpo: trecho do changelog (ou link para a seção).

> O `firmware.packed.bin` e os binários datados **não** são versionados no git
> (estão no `.gitignore`); ficam como assets das Releases.

## 5. Download na página web

A ferramenta web / página do projeto lista os últimos releases via GitHub API
(`/repos/<user>/qap/releases`) com link de download do `.bin` e do changelog.

## Exemplo de registro (CHANGELOG.md)

```markdown
## [2026-08-16 21:08]

- **Binário**: `20260816T21h08-firmware.packed.bin`
- **Hash sha256**: `0054e37e...`
- **Tamanho**: 51938 bytes

### Changed
- Tela de scan no padrão single; linha 0 com AGC único.
```
