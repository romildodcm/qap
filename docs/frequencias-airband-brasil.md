# Frequências Airband Brasil — Referência

## Sumário
- [Faixa Aeronáutica VHF](#faixa-aeronáutica-vhf)
- [Frequências Comuns por Tipo](#frequências-comuns-por-tipo)
- [Exemplos Regionais SC/PR/RS](#exemplos-regionais-scprrs)
- [Configuração de Scan Sugerida](#configuração-de-scan-sugerida)

## Faixa Aeronáutica VHF

| Segmento | Faixa (MHz) | Uso |
|----------|-------------|-----|
| COM | 118.000 – 136.975 | Comunicação aeronáutica |
| NAV (VOR/ILS) | 108.000 – 117.975 | Navegação (não voz) |
| Emergency | 121.500 | Frequência de emergência internacional |
| Guard | 243.000 | Emergência militar (UHF, fora do range) |

## Frequências Comuns por Tipo

| Tipo | Faixa típica (MHz) | Descrição |
|------|---------------------|-----------|
| TWR (Torre) | 118.0 – 121.4 | Controle de aeródromo |
| APP (Aproximação) | 119.0 – 125.0 | Controle de aproximação |
| ACC (Rota) | 124.0 – 132.0 | Controle em rota (área) |
| ATIS | 118.0 – 128.0 | Informação automática |
| GND (Solo) | 121.6 – 121.9 | Controle de solo |
| UNICOM | 122.750 | Aeródromos sem torre |
| Emergência | 121.500 | Guard frequency |

## Exemplos Regionais SC/PR/RS

| Aeroporto | ICAO | Frequência | Serviço |
|-----------|------|-----------|---------|
| Florianópolis | SBFL | 118.100 | TWR |
| Florianópolis | SBFL | 119.600 | APP |
| Curitiba | SBCT | 118.300 | TWR |
| Curitiba | SBCT | 127.600 | APP |
| Porto Alegre | SBPA | 118.100 | TWR |
| Porto Alegre | SBPA | 120.600 | APP |
| Navegantes | SBNF | 118.900 | TWR |
| Joinville | SBJV | 118.500 | TWR |
| Londrina | SBLO | 118.700 | TWR |
| Foz do Iguaçu | SBFI | 118.500 | TWR |

> **Fonte:** AIP Brasil / DECEA. Verificar NOTAM para atualizações.

## Configuração de Scan Sugerida

### Scan completo (faixa inteira)
```
Start: 118.000 MHz
End:   136.975 MHz
Step:  25 kHz (legacy) ou 8.33 kHz (ICAO moderno)
Mode:  AM
```

### Scan por canais prioritários (recomendado)
Programar canais específicos do aeródromo local com prioridade diferenciada:

```
Prioridade ALTA:
  - TWR local
  - APP local
  - 121.500 (emergência)

Prioridade NORMAL:
  - ACC (rota)
  - ATIS
  - GND
  - Aeródromos vizinhos
```

### Passo de frequência

| Padrão | Passo | Uso |
|--------|-------|-----|
| ICAO antigo | 25 kHz | Maioria dos canais BR atuais |
| ICAO novo | 8.33 kHz | Europa, futuramente BR |
| Scan rápido | 25 kHz | Varredura inicial |
| Scan preciso | 8.33 kHz | Busca de canais específicos |
