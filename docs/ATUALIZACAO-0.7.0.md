# Atualização 0.7.0 — slots e gestão

Firmware 0.7.0 / engine 1 / arte 2 / save 2 / preferências 1.

## Etapa 8 — três slots

As cidades ficam em `/microcity/saves/slot-1`, `slot-2` e `slot-3`, cada uma com alternância A/B e CRC. O save legado continua disponível no slot 1 até a próxima gravação.

## Etapa 9 — navegador

Continuar mostra população, caixa, ano e seed. Cada resumo é obtido somente de um save integralmente validado.

## Etapa 10 — confirmação

Criar em slot ocupado exige confirmação e a cidade inicial é gravada imediatamente.

## Etapa 11 — finanças

O botão Finanças mostra orçamento, impostos, demandas, capacidade/consumo, déficits e quantidade de serviços.

## Etapa 12 — alertas

O jogo informa quantos locais estão sem energia/água e avisa quando o abastecimento normaliza.

Atualize somente `/microcity/microcity.bin`; preserve toda a pasta `microcity`. Saves antigos continuam compatíveis.
