# Fonte mínima de boot/interface

Noto Sans Regular para texto em português e Noto Sans Math Regular para setas/símbolos. Fontes do projeto Noto, sob SIL OFL-1.1, com aviso completo em OFL.txt. Arquivos obtidos de:

- https://github.com/notofonts/noto-fonts/tree/main/hinted/ttf/NotoSans
- https://github.com/notofonts/noto-fonts/tree/main/hinted/ttf/NotoSansMath

Os SHA-256 dos TTF estão fixados no gerador; o checkout inclui os arquivos, sem precisar baixá-los ao compilar.

```sh
.venv/bin/pip install -r tools/requirements.txt
.venv/bin/python tools/generate_ui_font.py
```

Gera `main/ui/fonts/mc_font_ui.c`, `coverage.json` e `preview-ptbr.png`. Tamanho de 22 px, bitmap LVGL 4bpp, 200 glifos de texto básico/Latin-1 e símbolos usados pela interface. O gerador consulta o cmap real de cada fonte e rejeita ausência ou glyph ID zero (.notdef). Fontes padrão do LVGL Montserrat não incluem todos os acentos necessários.

A fonte mínima é incorporada ao firmware para manter menu, configurações e recuperação legíveis sem SD. Não inclui arte de construções ou ícones do jogo; esses continuam no pacote separado. Os TTF completos e a prévia não são incorporados ao binário.
