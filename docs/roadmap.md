# Roadmap

## Sumário
- [Fase 1 — Estrutura Base](#fase-1--estrutura-base)
- [Fase 2 — Remoção de Features](#fase-2--remoção-de-features)
- [Fase 3 — Otimizações Airband](#fase-3--otimizações-airband)
- [Fase 4 — UI e Modos de Operação](#fase-4--ui-e-modos-de-operação)
- [Fase 5 — Testes e Calibração](#fase-5--testes-e-calibração)

## Fase 1 — Estrutura Base

- [ ] Forkar `miramir/uv-k5-firmware` como base;
- [ ] Configurar build local (arm-none-eabi-gcc 10.3.1);
- [ ] Validar build e flash do firmware base sem modificações;
- [ ] Confirmar recepção AM funcional em 118–136 MHz com firmware base.

## Fase 2 — Remoção de Features

- [ ] Desabilitar TX completamente (PTT remapeado para start/stop scan);
- [ ] Remover FM broadcast receiver;
- [ ] Remover NOAA;
- [ ] Remover VOICE/Beep;
- [ ] Remover DTMF calling/paging;
- [ ] Remover AIRCOPY;
- [ ] Remover VOX;
- [ ] Remover Alarm;
- [ ] Medir flash utilizado após remoções;
- [ ] Validar que recepção AM continua funcional.

## Fase 3 — Otimizações Airband

- [ ] Implementar AM AGC avançado (32 níveis, histerese, attack/decay);
- [ ] Implementar filtro DSP de áudio passa-banda (ponto fixo Q15);
- [ ] Implementar squelch baseado em SNR;
- [ ] Implementar scan com dwell adaptativo e canais de prioridade;
- [ ] Criar tabela de ganho calibrada para 118–136 MHz;
- [ ] Forçar passo padrão 8.33 kHz (com opção 25 kHz).

## Fase 4 — UI e Modos de Operação

### Modos de escuta
- [ ] Modo frequência única: exibe 1 frequência no visor, escuta fixa;
- [ ] Modo scan: percorre lista de frequências com dwell adaptativo;
- [ ] Modo tri-frequência: exibe 3 frequências simultâneas na tela, monitora todas;
- [ ] Menu para adicionar frequência atual à lista de scan;

### Comportamento de inicialização
- [ ] Opção no menu: iniciar já scaneando a lista ao ligar;
- [ ] Opção no menu: iniciar na última frequência usada (modo fixo);
- [ ] Persistir última config em EEPROM;

### Tecla PTT (remapeada)
- [ ] PTT inicia scan se parado;
- [ ] PTT para scan e fica na frequência atual se scaneando;

### Tela e interface
- [ ] Tela principal: frequência atual + S-meter grande;
- [ ] Tela tri-freq: 3 linhas com freq + indicador de sinal;
- [ ] Lista de scan com indicador de atividade;
- [ ] Modo "monitor" (squelch aberto) com tecla dedicada;
- [ ] Display de canais com labels (ex: "TWR SBGR", "APP SP");
- [ ] Simplificar menu (apenas opções relevantes para scanner).

## Fase 5 — Testes e Calibração

- [ ] Testar em aeroporto/aeródromo local;
- [ ] Calibrar tabela de ganho com sinais reais;
- [ ] Ajustar thresholds de squelch SNR;
- [ ] Medir consumo de bateria vs firmware original;
- [ ] Documentar performance (sensibilidade, seletividade prática).
