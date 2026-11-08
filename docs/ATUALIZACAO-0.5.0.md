# Atualização 0.5.0 — frames atuais e guia inicial

Firmware 0.5.0 / engine 1 / arte 2 / save 2.

## Etapa 3 — descartar trabalho obsoleto

Quando câmera, zoom, overlay, simulação ou uma construção alteram a cena, o renderer cancela o backbuffer incompleto e começa diretamente no estado atual. O frame já visível não é modificado. Consultas e comandos rejeitados não redesenham o mapa.

## Etapa 4 — tutorial curto

Uma nova instalação mostra um guia de três páginas na primeira nova cidade, depois do primeiro frame completo. Ele cobre infraestrutura, crescimento e qualidade/serviços. A conclusão fica no NVS e o botão Guia permite consultar novamente.

Para atualizar pelo launcher, substitua somente `/microcity/microcity.bin`. Assets e saves permanecem compatíveis. A redução de trabalho foi verificada no fluxo do renderer e por build/testes; tempos reais continuam dependentes de medição no Tab5.
