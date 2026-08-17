# Plano de Implementação — Publicar o Repositório no GitHub (Licença, README, Pages, Releases)

**Objetivo:** preparar o repositório para publicação pública no GitHub: licença correta,
créditos aos projetos base, README profissional, limpeza de artefatos, hospedagem da
ferramenta web (Web Serial) via **GitHub Pages** e **fluxo padronizado de builds +
releases + changelog** para download dos binários na página.

> ⚠️ Executar na ordem, agente **Firmware UV-K5 (Modo Seguro)**. Nenhuma mudança de
> código C/hardware aqui (só arquivos do repositório). Build Docker ao final para garantir
> que nada quebrou.

---

## Contexto (estado atual)

- README.md já existe (bom), mas a seção "Licença" aponta `./LICENSE` que **não existe**
  na raiz (só em `firmware/LICENSE`). Link quebrado.
- O firmware deriva de código **Apache License 2.0** (DualTachyon/miramir/egzumer/
  OneOfEleven). Headers já carregam "Copyright ... Dual Tachyon/OneOfEleven".
- Existem `.venv` e binários em `ui/` e `firmware/utils/` que não devem ir para o git.
- Repositório local **sem remote configurado**; branch `main`.
- GitHub user: **romildodcm** (`romildodcm.github.io`). CNAME `qap` já aponta para
  `romildodcm.github.io` no DNS.

---

## Batch 00 — Renomear Repositório para `qap` 🟢

```
Renomeie o repositório para o nome final `qap`.

1. Renomear a pasta local (fora do editor):
   cd ~/Documents/GitHub && mv pu5xrm-quansheng-airband-scan qap
2. No GitHub: criar o repositório `qap` (ou renomear o existente) sob o usuário
   romildodcm.
3. Página do projeto: https://romildodcm.github.io/qap/
   Domínio próprio (opcional): https://qap.romildo.net/
4. Ajuste no README/links referências a URLs antigas se houver (nome do projeto
   "PU5XRM" no título pode permanecer — é a marca, não o nome do repositório).
   (A conexão do remote/push é feita pelo próprio humano.)
```

**Aceite:** repositório chamado `qap` no GitHub, página em `/qap/`.

- A ferramenta web (plano 004, `scanlist_config.html`) pode ser hospedada em GitHub Pages
  (HTTPS → Web Serial API funciona).

---

## Batch 01 — Limpeza do Repositório 🟢

```
Limpe arquivos que não devem ser versionados/publicados.

1. Revise o .gitignore da raiz (e crie se não existir). Garanta que ignora:
   - build/ (firmware), obj/, *.packed.bin, *.bin
   - .venv/ e venv/ (raiz, ui/, firmware/utils/, tools/**)
   - src/external/ (printf, CMSIS_5) — dependências clonadas
   - __pycache__/, *.pyc
   - arquivos temporários e dumps de backup
2. Verifique o que o git está rastreando hoje:
   `git status` e `git ls-files | grep -E '\.venv|build/|\.bin$|external/'`
3. Remova do índice (sem apagar do disco) o que não deve ir:
   `git rm -r --cached` para .venv, build, binários, external.
4. Confirme: `git status` limpo de artefatos indesejados.
```

**Aceite:** git não rastreia venv/build/binários/external; repositório pronto para público.

---

## Batch 02 — Licença e Direitos Autorais 🟢

```
Defina a licença correta e os créditos aos projetos base. TUDO é Apache 2.0.

1. Crie `LICENSE` na raiz com o texto integral da **Apache License 2.0**
   (copiar de firmware/LICENSE, que já é Apache 2.0).
2. DECISÃO: o projeto inteiro (firmware + ferramenta web) é **Apache License 2.0** —
   licença permissiva, permite uso comercial. NÃO usar licenças restritivas
   (a ferramenta web, mesmo sendo original, segue Apache 2.0 para consistência).
3. Crie `NOTICE` creditando os autores base:
   - DualTachyon/uv-k5-firmware (implementação original)
   - OneOfEleven/uv-k5-firmware-custom
   - egzumer/uv-k5-firmware-custom
   - miramir/uv-k5-firmware (fork base)
   - Armel/F4HWN (se aplicável)
   - egzumer/uvtools e iu2frl/uvtools-frl (referência/design da ferramenta web)
4. Confirme que os cabeçalhos de copyright originais nos .c/.h foram
   preservados (Apache 2.0 exige manter o aviso).
5. Atualize a seção "Licença" do README (link para ./LICENSE, declarando
   Apache 2.0 para o projeto todo).
```

**Aceite:** LICENSE na raiz (Apache 2.0), NOTICE com créditos, cabeçalhos preservados,
link do README corrigido.

---

## Batch 03 — README Profissional 🟢

```
Aprimore o README para público, com referências claras.

1. Mantenha: objetivo, hardware alvo, funcionalidades (incluídas/removidas).
2. Adicione/explicite na seção "Base do Firmware" os links:
   - miramir/uv-k5-firmware (fork base)
   - DualTachyon/uv-k5-firmware (original)
   - OneOfEleven, egzumer, armel (customizações)
   - egzumer/uvtools e iu2frl/uvtools-frl (flasher/ferramenta web de referência)
3. Adicione seção "Ferramenta Web" apontando para a página (GitHub Pages) e
   explicando o uso (Web Serial, Chrome/Edge).
4. Adicione seção "Aviso Legal" (somente recepção; TX ilegal em airband).
5. Adicione seção "Roadmap/Planos" linkando docs/ e plans/.
6. Revise markdown, badges opcionais, e o link da licença.
```

**Aceite:** README público completo, links dos projetos base, aviso legal, seção da
ferramenta web e roadmap.

---

## Batch 04 — GitHub Pages + Domínio Personalizado 🟢

```
Hospede a ferramenta web como página do repositório, com domínio próprio.

1. Coloque `scanlist_config.html` (plano 004) em local fixo, ex. `web/` ou raiz.
2. Configure **GitHub Pages** no repositório `qap` (Settings -> Pages):
   - Source: branch principal, pasta onde está o HTML (ex.: /root ou /web).
   - Endereço padrão: https://romildodcm.github.io/qap/
3. Configure o **Custom domain** `qap.romildo.net` (Settings -> Pages -> Custom domain).
   - O CNAME no DNS já aponta `qap` -> romildodcm.github.io (feito pelo usuário).
   - O GitHub cria o arquivo `CNAME` no repositório com `qap.romildo.net`.
   - O GitHub emite HTTPS automaticamente para `qap.romildo.net` (necessário p/ Web Serial).
4. A página fica acessível em https://qap.romildo.net/ (e em /qap/ em paralelo).
5. Valide que a página abre, pede acesso serial e as abas funcionam (sem rádio
   ao menos a UI).
6. Adicione link da página no README e (se quiser) um badge.
```

**Aceite:** ferramenta web acessível via GitHub Pages (HTTPS), link no README.

---

## Batch 05 — Fluxo Padronizado de Build + Release + Changelog 🟢

```
Padronize como cada build vira um binário baixável + changelog na página.

1. Padronize o binário por build (já temos a convenção):
   - `firmware/build/firmware.packed.bin` (artefato de flash) sempre atual.
   - Cópia datada: `YYYYMMDDTHHhMM-firmware.packed.bin` (mantida por build).
2. Crie um `CHANGELOG.md` na raiz (Keep a Changelog simplificado):
   - Seção por versão/build com data, hash sha256, e o que mudou (Added/Changed/Fixed).
   - Formato padronizado do registro de build:
     `## [2026-08-16 16:45]` / `- bin: 20260816T16h45-firmware.packed.bin` /
     `- hash: sha256:...` / `- mudanças: ...`
3. Cada novo build atualiza o CHANGELOG.md ANTES do commit (nunca publicar build
   sem registro de changelog).
4. Publique os binários via **GitHub Releases** (padrão GitHub, download direto):
   - Criar Release por build: título = data/hora, asset = `...-firmware.packed.bin`,
     corpo = trecho do changelog (ou link para a seção).
5. Na ferramenta web / página, adicione seção "Downloads/Releases":
   - Lista os últimos releases (via GitHub REST API `/releases`) com link de
     download do .bin e do changelog.
6. Documente o processo em `docs/build-release.md` (comandos, convenção, exemplo).
```

**Aceite:** todo build gera binário datado + registro no CHANGELOG; Releases com download;
página lista downloads; processo documentado.

---

## Batch 06 — Validação Final 🟢

```
Valide o repositório pronto para publicação.

1. `git status` limpo (sem venv/build/binários rastreados).
2. `git ls-files` revisado (só o que deve ser público).
3. Build Docker do firmware ainda funciona (nada quebrou):
   `cd firmware && docker run ... make clean && make`
4. `plans/`, `docs/` consistentes (005 = áudio AGC, 004 = ferramenta web,
   003 = publicação).
5. (Se aplicável) commit inicial com mensagem clara; NÃO dar push sem o
   humano confirmar (regra do agente).
```

**Aceite:** repositório limpo, build OK, pronta a publicação. Push só com confirmação
do humano.

---

## Critérios de Aceite Globais

- [ ] `LICENSE` na raiz (Apache 2.0) + `NOTICE` com créditos aos projetos base.
- [ ] README profissional com links dos repositórios base, aviso legal e seção web.
- [ ] Nenhum artefato (venv/build/binário/external) rastreado no git.
- [ ] Ferramenta web hospedada em GitHub Pages (HTTPS).
- [ ] Todo build gera binário datado + registro no CHANGELOG + Release (download na página).
- [ ] Build Docker do firmware continua OK.
- [ ] Push só com confirmação explícita do humano.
