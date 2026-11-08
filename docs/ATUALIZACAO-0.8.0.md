# Atualização 0.8.0 — correção do mapa azul

Firmware 0.8.0 / engine 1 / arte 2 / save 2 / preferências 1.

## Causa e correção

Em hardware onde a montagem de um frame levasse mais que o intervalo de atualização diária, cada tick cancelava o desenho antes da publicação. O canvas permanecia no fundo azul. Agora ticks apenas agendam o frame seguinte; somente ações interativas substituem um backbuffer obsoleto.

## Cinco etapas

1. Publicação garantida do frame em andamento.
2. Percentual de carregamento do mapa e atraso seguro do tutorial.
3. Congelamento da simulação durante painéis modais, sem catch-up posterior.
4. Home/Centralizar mapa para recuperar câmera e zoom.
5. Navegador com felicidade e déficits de energia/água.

Atualize somente `/microcity/microcity.bin`. Preserve assets, settings e todos os diretórios de saves. A correção foi validada estruturalmente, em testes host e no build ESP32-P4; a duração física do primeiro frame ainda deve ser observada no Tab5.
