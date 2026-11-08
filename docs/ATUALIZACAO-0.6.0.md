# Atualização 0.6.0 — dados mutáveis no microSD

Firmware 0.6.0 / engine 1 / arte 2 / save 2 / preferências 1.

## Etapa 5 — configuração A/B

Preferências ficam em `/microcity/settings/prefs-a.bin` e `prefs-b.bin`. O formato possui versão, sequência, CRC, validação de campos e fallback. A escrita é sincronizada e verificada antes de ser aceita.

## Etapa 6 — sessão persistente

O cartão conserva volume, mute, tutorial, posição e zoom da câmera, ferramenta, overlay e velocidade. A sessão visual é vinculada à seed para não aplicar uma câmera antiga sobre outra cidade. Alterações rápidas são agrupadas após 750 ms.

## Etapa 7 — seed da nova cidade

Nova cidade agora mostra a seed em decimal e hexadecimal, permite sortear outra e exige confirmação para gerar o mapa.

O firmware executável continua obrigatoriamente na flash; assets, áudio, cidades, preferências e sessão ficam no microSD. Para atualizar pelo launcher, substitua somente `/microcity/microcity.bin`, preservando toda a pasta de dados.
