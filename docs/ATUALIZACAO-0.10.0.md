# Atualização 0.10.0

## Web-GUI do launcher

Com o jogo fechado, envie para `/microcity/`, nesta ordem:

1. `dist/launcher/microcity/microcity.bin`;
2. `dist/launcher/microcity/slot-a.mcap`;
3. `dist/launcher/microcity/slot-a.ready` **por último**.

Preserve `slot-b.*`, `saves/` e `settings/`. Reinstale/inicie o BIN pelo launcher. O registro novo é o slot A, sequência 5, arte v5. Se o cartão já tiver uma sequência 5 ou maior criada por outra atualização, use o receptor de assets do jogo, que seleciona o slot inativo e a próxima sequência.

O firmware novo lê saves 64×64 antigos e os grava como 128×128. A migração não substitui os três slots nem apaga a cidade original antes de uma gravação válida.

## Correção visual posterior — arte 6

Se o firmware 0.10.0 e a arte 5 já estão instalados, envie somente `slot-b.mcap` e `slot-b.ready`, nessa ordem. O slot B sequência 6 corrige a escala e a âncora dos lotes multicélula. Não é necessário reenviar o BIN.

## Correção 0.10.1 — arte 7 e remoção dos veículos

Envie `microcity.bin`, `slot-a.mcap` e `slot-a.ready`, nessa ordem. O BIN remove totalmente os veículos das ruas; o slot A sequência 7 aumenta somente o prédio dentro do lote multicélula, mantendo a base limitada ao footprint. Navios continuam com limite de quatro. Preserve saves, settings e o slot B anterior.

## Correção visual — arte 8

Com o firmware 0.10.1 instalado, envie apenas `slot-b.mcap` e depois `slot-b.ready`. A sequência 8 corrige as emendas das vias e substitui os prédios esticados por campi multicélula. BIN e saves não mudam.

## Correção 0.10.2 — composição do campus

Com a arte 8 instalada, substitua somente `microcity.bin`. O renderer não desenha mais grama sobre as alas do sprite âncora ao processar as demais células reservadas. Saves e assets não mudam.

## Atualização 0.10.3 — grade ocupada

Envie `microcity.bin`, depois `slot-a.mcap` e `slot-a.ready` por último. O slot A sequência 9 contém o overlay tracejado de footprint. Preserve o slot B, saves e settings.

## Atualização 0.11.0 — desastres e ciclo mensal

Envie `microcity.bin`, depois `slot-a.mcap` e `slot-a.ready` por último. O slot A sequência 10 contém os overlays de tempestade e tremor. Saves v2–v4 permanecem legíveis e a próxima gravação usa v5. Preserve o slot B, saves e settings.

## Validação local

Testes host passaram com sanitizadores, incluindo pontes, raio de utilidades, footprints, porto, RCI, corrupção/fallback e migração. O build ESP32-P4 passou. Não foi realizado flash, monitor serial ou teste OTA físico.
