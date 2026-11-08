# Dependências e licenças

Código e sprites originais: MIT, conforme LICENSE. Não foi copiada implementação de aplicações vizinhas nem código do Tab5Meshtastic.

Dependências são baixadas pelo ESP-IDF Component Manager. `dependencies.lock` registra versões e hashes. Não remover avisos de licença ao redistribuir:

- ESP-IDF / ESP-BSP M5Stack Tab5 e componentes Espressif: consultar LICENSE/SPDX de cada componente, em geral Apache-2.0.
- LVGL: MIT.
- ESP-IDF inclui componentes de terceiros como FreeRTOS, lwIP e mbedTLS, com respectivos avisos no SDK.
- Pillow (somente ferramentas): HPND, ver LICENSE da distribuição.
- OpenSSL (somente testes de host): Apache-2.0 na versão 3.

O script de empacotamento USB deve incluir as licenças dos componentes usados e do SDK junto aos binários. O pacote visual contém exclusivamente arte/sons originais e metadados próprios.

- Driver Tab5 Keyboard: M5Stack MIT, obtido do upstream e fixado em commit, com LICENSE/UPSTREAM.md no componente local.
- Noto Sans e Noto Sans Math: The Noto Project Authors, SIL OFL-1.1; ver assets/fonts/OFL.txt. As fontes derivadas para UI preservam o aviso no código gerado e na distribuição.
- USB HID Host Espressif: Apache-2.0, versão 1.1.0 fixada no manifesto/lock.
- fonttools: MIT, ferramenta usada apenas para verificar cobertura real dos TTF durante a geração.
