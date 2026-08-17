# Plano de Implementação — Ferramenta Web de Configuração de Scan Lists (Web Serial)

**Objetivo:** uma **ferramenta HTML** (arquivo único) que roda no navegador via **Web
Serial API** — como https://egzumer.github.io/uvtools/ ou
https://iu2frl.github.io/uvtools-frl/. Ela pede acesso à porta serial, tem uma **aba para
cada scan list (1/2/3)** com edição de frequência + nome, **envia** as listas ao rádio,
faz **download/backup** das listas (CSV), **backup do rádio** (EEPROM), **backup e
atualização de firmware** (ler/flashear a flash do MCU via UART), como os uvtools.

> ⚠️ Executar na ordem, agente **Firmware UV-K5 (Modo Seguro)**. Build Docker a cada batch.
> Gravamos **somente** nas áreas usadas pelo próprio `AIRBAND_LoadDefaults()` (dados do
> canal `0x0000+ch*16`, nomes `0x0F50+ch*16`, atributos `0x0D60+ch`). **NUNCA** tocar em
> calibração TX (`0x1ED0–0x1F30`), bateria (`0x1F40–0x1F47`) ou header (`0x1FF0`).
> Backup/leitura é **read-only** (nunca escreve EEPROM).

---

## Contexto Técnico (layout dos canais MR no UV-K5)

- Canais MR: índice `0..199` (`MR_CHANNEL_FIRST=0`, `MR_CHANNEL_LAST=199`).
- Projeto airband usa até `AIRBAND_MAX_CHANNELS = 30` (`airband_channels.h`).
- **Dados do canal** (`AIRBAND_LoadDefaults`): `0x0000 + (ch * 16)`, 16 bytes:
  - bytes 0-3 freq RX (formato interno: MHz × 100000, ex.: `11880000` = 118.800 MHz)
  - bytes 4-7 freq TX (igual, RX-only não usa)
  - byte 0x0B = `MODULATION_AM << 4` (nibble alto = AM)
  - byte 0x0C = bandwidth/wide
  - byte 0x0E = `STEP_8_33kHz`
- **Nome do canal**: `0x0F50 + (ch * 16)`, 10 bytes (via `SETTINGS_SaveChannelName`).
- **Atributos/scan lists**: `0x0D60 + ch`, 1 byte `ChannelAttributes_t`
  (`band`, `scanlist1/2/3` bits). Scan usa esses bits + `SCAN_LIST_DEFAULT`.
- **Menu que escolhe a lista**: já existe `SList` (`SCAN_LIST_DEFAULT` 0-5:
  NO LIST / LIST[1] / LIST[2] / LIST[3] / LISTS[1,2,3] / ALL).

---

## Batch 01 — Ferramenta HTML (Web Serial + Abas) 🟢

```
Crie a ferramenta web em tools/scanlist_config.html (arquivo único, sem build).

1. Arquivo único HTML/CSS/JS com Web Serial API (navigator.serial), Chrome/Edge.
2. Botão "Conectar" -> requestPort() + abrir com baud 9600 (padrão UV-K5).
3. UI inspirada em uvtools: cabeçalho com status de conexão, abas de lista.
4. Uma aba por scan list (1, 2 e 3):
   - tabela de até 20 linhas: [frequência MHz] [nome] (nome <= 10 chars).
   - botões: "Ler do rádio", "Enviar ao rádio", "Exportar CSV", "Importar CSV".
   - limpeza/validação por linha (freq 118.000-136.975; nome ASCII <= 10).
5. Exportar CSV: gera scanlist1.csv / scanlist2.csv / scanlist3.csv (download).
   Importar CSV: lê arquivo e preenche a aba.
6. Aba "Firmware":
   - "Ler Firmware" -> baixa a flash/ROM do MCU (.bin, ex.: 64KB) para backup.
   - "Atualizar Firmware" -> selecionar um .bin (ex.: firmware.packed.bin) e
     flashear via UART, com barra de progresso, verificação e aviso de risco.
   - Fluxo de flash: rádio no modo boot (DFU: PTT+Power ao ligar) OU reinício
     para o bootloader por comando; usa o protocolo de bootloader do UV-K5.
7. Aba "Backup do rádio": botão "Ler EEPROM" -> baixa dump da EEPROM (.bin).
8. Não requer venv/instalação — abre o HTML direto no navegador.
9. Valide no navegador (sem rádio): abas funcionam, CSV import/export OK.
```

**Aceite:** HTML único abre no navegador, pede acesso serial, abas 1/2/3 funcionais,
CSV import/export OK, botões de leitura/backup presentes.

---

## Batch 02 — Protocolo UART de Importação (firmware) 🟡

```
Adicione um handler UART para receber a configuração das scan lists.

1. Leia driver/uart.c e app/uart.c (ENABLE_UART já ativo).
2. Defina um protocolo simples e seguro (ex.):
   - Comando 0xAA 'S' 'L' (start), depois registros de canal, depois fim + CRC16.
   - Registro: `lista(1..3) | slot(0..19) | freq_interna(4B) | nome(10B)`.
3. Implemente o parse: valida lista(1-3), slot(0-19), faixa de frequência,
   e CRC; ignora pacotes inválidos (sem gravar nada parcial).
4. Mantenha o handler compatível com o UART_Version existente.
5. Build Docker. Zero erros.
```

**Aceite:** protocolo parseado e validado; pacotes inválidos rejeitados; build limpo.

---

## Batch 03 — Gravar Canais + Atributos de Scan List (firmware) 🟡

```
Grave os canais recebidos nos canais MR usando as MESMAS APIs do AIRBAND_LoadDefaults.

1. Para cada registro válido (lista L, slot S, freq, nome):
   - canal MR = (L-1) * 20 + S  (lista1: 0-19, lista2: 20-39, lista3: 40-59)
   - grave dados do canal em 0x0000 + canal*16 (freq RX/TX, AM, step 8.33)
     via EEPROM_WriteBuffer (mesma lógica do AIRBAND_LoadDefaults).
   - grave nome em 0x0F50 + canal*16.
   - atualize gMR_ChannelAttributes[canal]: band=BAND2_108MHz e o bit da lista
     (lista1 -> scanlist1=1; lista2 -> scanlist2=1; lista3 -> scanlist3=1);
     zere os demais bits de scanlist deste canal.
   - grave atributos em 0x0D60.
2. Grave SÓ depois que o pacote inteiro for validado (CRC OK) — nada de gravação parcial.
3. Atualize RAM (gEeprom.VfoInfo) e dê gUpdateDisplay/gUpdateStatus.
4. Build Docker. Zero erros.
```

**Aceite:** canais gravados corretamente, atributos de scanlist coerentes, build limpo.

---

## Batch 04 — Protocolo UART de Leitura/Backup + Flash (firmware) 🟡

```
Adicione leitura (read-only) p/ scan lists, EEPROM e firmware; e a ESCRITA de
firmware via protocolo de bootloader do UV-K5 (modo DFU).

1. Comando de LEITURA das scan lists:
   - Cliente pede "lista L" -> firmware responde os registros (freq + nome)
     dos canais daquela lista (lidos de EEPROM/RAM).
   - NUNCA escreve nada; só responde.
2. Comando de LEITURA de EEPROM (backup do rádio):
   - Cliente pede faixa de endereço + tamanho -> firmware envia os bytes
     (apenas LEITURA, driver/eeprom.c -> EEPROM_ReadBuffer).
   - Limitar faixa à região de dados (ex.: 0x0000-0x1FFF) e nunca sobrepor
     o bootloader; apenas leitura.
3. LEITURA de firmware (backup):
   - Firmware lê a região de código (somente leitura) e envia em blocos com
     checksum; o download gera um .bin de backup. Read-only.
4. ESCRITA/ATUALIZAÇÃO de firmware (flash, como uvtools):
   - O flash NÃO é feito pelo firmware em execução; é feito pelo BOOTLOADER
     do UV-K5 no modo DFU (PTT+Power ao ligar).
   - Implemente no lado do HOST (JS) o protocolo de bootloader do UV-K5
     (comandos de begin/write/end + checkcode), o mesmo usado por
     k5prog/egzumer/iu2frl.
   - Fluxo: usuário coloca o rádio em modo DFU; a ferramenta detecta, grava
     o .bin em blocos, verifica e confirma.
   - RISCOS ALTO: exige confirmação explícita, backup prévio e nunca
     interromper o flash (pode brickar). Nunca flashear binário errado.
5. Todos os comandos com CRC/validação e compatíveis com o UART_Version.
6. Build Docker. Zero erros.
```

**Aceite:** leitura de scan lists/EEPROM/firmware funcionam (read-only) e a
atualização de firmware via modo DFU funciona; build limpo.

---

## Batch 05 — Menu para Escolher Qual Lista Escanear 🟢

```
Garanta um item de menu claro para escolher qual scan list escanear.

1. Revise o menu "SList" (SCAN_LIST_DEFAULT: NO LIST / LIST[1]/[2]/[3] /
   LISTS[1,2,3] / ALL) — ele já escolhe a lista que o scan usa.
2. Ajuste o texto das opções para o contexto airband, ex.:
   - "SL1" (scan list 1), "SL2", "SL3", "SL1+2+3", "ALL".
3. Confirme que o scan (CHFRSCANNER -> RADIO_CheckValidChannel) respeita a
   escolha: LIST[1] varre canais com scanlist1=1, etc.
4. Build Docker. Zero erros.
```

**Aceite:** menu escolhe claramente a lista; scan respeita a seleção; build limpo.

---

## Batch 06 — Integração e Validação 🟢

```
Valide o fluxo completo: HTML -> Web Serial -> firmware -> scan.

1. No navegador: conecte ao rádio, preencha listas de exemplo nas 3 abas.
2. Envie ao rádio; confirme que o scan usa a lista escolhida no menu SList.
3. Leia de volta as listas (deve bater com o enviado) e exporte os 3 CSVs.
4. Faça backup do rádio (dump EEPROM) e backup de firmware (flash .bin);
   confira os arquivos baixados.
5. Atualize o firmware pela ferramenta (modo DFU): confira que o rádio boota
   com o novo binário e que o backup de firmware bate com o que foi flasheado.
5. Confirme que o PA segue desligado no RX e o TX inativo (RX-only).
6. Build Docker limpo; flash < 64KB.
7. Gere binário datado:
   cp build/firmware.packed.bin build/$(date +%Y%m%dT%Hh%M)-firmware.packed.bin
8. Reporte: resumo, tamanho, sha256, binário datado.
```

**Aceite:** importação/leitura/backup funcionais via navegador, scan respeita a lista,
PA off no RX, binário datado + hash.

---

## Critérios de Aceite Globais

- [ ] Ferramenta HTML única com Web Serial, abas 1/2/3, backup do rádio, backup e atualização de firmware.
- [ ] Export/import de CSVs (scanlist1/2/3.csv) funcionando.
- [ ] Protocolo UART validado (CRC, faixa de freq, limites por lista) p/ ler e gravar.
- [ ] Canais e atributos gravados só com pacote íntegro (sem gravação parcial).
- [ ] Leitura/backup (EEPROM e firmware) é read-only; flash só via modo DFU (bootloader) com confirmação e backup prévio.
- [ ] Menu escolhe qual lista escanear e o scan respeita.
- [ ] Gravação apenas nas áreas usadas pelo AIRBAND_LoadDefaults (sem calibração).
- [ ] PA off em RX, TX inativo (RX-only preservado).
- [ ] Build `make clean && make` sem erros/warnings novos; binário datado + hash.

