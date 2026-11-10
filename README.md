# Cobra Arena v2

Jogo da cobra para PS3 (PSL1GHT), offline, sem áudio.

## Estrutura
- `main.c` – o jogo
- `Makefile` – compila e monta o `.pkg` (`make pkg`)
- `tools/make_sfo.py` – gera o `PARAM.SFO` com validação de TITLE_ID e CONTENT_ID
- `tools/make_icon.py` – gera o `ICON0.PNG` (obrigatório para instalar)
- `.github/workflows/build.yml` – build automático no GitHub Actions

## Checklist para o pacote instalar no PS3
- `TITLE_ID` com 9 caracteres (`COBRA0001`)
- `CONTENT_ID` com 36 caracteres e começando com `UP0001-<TITLE_ID>`
- `ICON0.PNG` presente na pasta do pacote
- `PARAM.SFO` gerado pelo script (não editado à mão)
- `EBOOT.BIN` em `USRDIR/`

## Antes de rodar o build
Copie para o workflow o passo de instalação do toolchain que já funciona
(no lugar do `TODO`). Sem ele, o build não tem como compilar.

## Controles
- D-Pad: direção (sem meia-volta)
- START: recomeçar após morrer