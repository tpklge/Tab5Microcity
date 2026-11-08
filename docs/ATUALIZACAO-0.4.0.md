# Atualização 0.4.0 — interação e diagnóstico

Firmware 0.4.0 / engine 1 / arte 2 / save 2.

## Etapa 1 — menos trabalho por interação

- Um toque simples não gera mais um frame ao pressionar e outro ao confirmar.
- Arraste só redesenha quando a câmera realmente muda.
- A prévia de estrada só redesenha ao entrar em outra célula.
- Cada passagem cooperativa pinta no máximo 24 sprites, mantendo o limite temporal anterior.

## Etapa 2 — infraestrutura compreensível

- Barra de status: consumo/capacidade de energia e água.
- Consultar: ocupação, zona, nível, impedimento de crescimento, terreno, poluição e coberturas.
- Capacidades contam apenas instalações ligadas a uma rede viária.

Para atualizar pelo launcher, substitua somente `/microcity/microcity.bin`. Preserve assets e saves. A melhora foi validada estruturalmente e por testes host/build; tempo de toque e frame ainda deve ser medido no Tab5.
