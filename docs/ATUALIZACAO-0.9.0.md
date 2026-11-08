# Atualização 0.9.0 — modo Obras e equipamentos urbanos

Firmware 0.9.0 / engine 1 / arte 4 / save 3 (leitura de save 2) / preferências 2.

## Implementado

1. Teclado A164 no I²C1, como no exemplo oficial M5Stack, mantendo USB secundário. A tela Informações separa falha do barramento, ausência do dispositivo e última tecla.
2. Modo Obras pelo botão inferior ou tecla `B`: prédios viram marcadores baixos e distintos das delimitações de zona.
3. Zoneamento residencial, comercial e industrial por retângulo de toque/arrasto, com prévia, custo e aplicação atômica.
4. Escola primária 2×2, escola secundária 2×2, universidade 3×3 e parque municipal 2×2. Demolir qualquer parte remove a construção inteira.
5. Avenida com visual e custo próprios, construída por arrasto como estrada. Menus foram separados em quatro categorias legíveis.

## Atualização pelo launcher

Envie `dist/launcher/microcity/microcity.bin`. Como esta revisão usa sprites novos obrigatórios, envie também o par de asset mais recente indicado no diretório do launcher, sempre `.mcap` primeiro e `.ready` por último. Preserve `/microcity/saves` e `/microcity/settings`.

Arte 4 redesenha os IDs 132–135 com escola primária, escola secundária, universidade e parque municipal próprios. A alteração não muda regras nem saves.

O firmware não apaga cidades quando o pacote visual falta. Nova cidade e Carregar direcionam à tela de recuperação, que explica a ordem dos arquivos. Não foi executado flash, monitor, serial nem teste físico nesta revisão.
