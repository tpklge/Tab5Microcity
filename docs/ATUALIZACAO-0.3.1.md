# Atualização corretiva 0.3.1

Firmware 0.3.1 / engine 1 / arte 2 / save 2.

Corrige o reinício provocado pelo primeiro toque em qualquer botão da interface. A causa era um descritor nulo de transição visual instalado no LVGL. A correção não altera cidades salvas nem o pacote de assets.

Para atualizar pelo launcher, substitua somente `/microcity/microcity.bin` e reinstale/inicie o aplicativo. Preserve `slot-a.*`, `slot-b.*` e `saves/`.

Validação local cobre build cruzado ESP32-P4 e testes host. O toque real deve ser confirmado no Tab5; nenhum flash ou monitor serial foi executado durante a preparação.
