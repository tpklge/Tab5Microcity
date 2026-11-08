# Progresso — 07/11/2026

## Estado entregue

Projeto independente em SimCitydoPaludo. A pasta inicialmente continha somente a especificação do usuário; nenhum código de outra aplicação foi importado. Firmware 0.1.0, engine 1, save 2, arte 1. Implementação inicial de A–E em código; acabamento/profiling de F e validação física permanecem abertos. Os critérios de aceite que exigem dispositivo ainda não foram demonstrados.

Implementados: bootstrap BSP, menu, recuperação sem assets, mapa 64×64, terreno reproduzível, câmera/zoom/picking, prévia e transação de estrada, construção/demolição/zonas, três estágios R/C/I, componentes locais e capacidade de utilidades, serviços/coberturas, parques/poluição, felicidade/demanda, economia/calendário/pausa/velocidades/impostos, incêndio simples, save manual/automático A/B com CRC, pacote original de sprites/PCM no SD, cache limitado, instalação manual e receptor autenticado de assets por AP temporário, ferramenta de envio, controle persistido de volume/mute e instrumentação inicial.

## Evidências

- ESP-IDF local: v5.4.4; dependências reproduzíveis em dependencies.lock.
- `./tools/build.sh build`: passou para ESP32-P4. Binário 1,136,608 bytes (1.084 MiB), partição de 7 MiB, aproximadamente 84.5% de margem. [Log](evidencias/build-esp32p4.log).
- `./tools/test.sh`: passou, sem erro ASan/UBSan. Regras, projeção, rollback, rede separada, crescimento, economia, relógio/velocidades/pausa, determinismo, PRNG/save/load/corrupção, versão de cidade nova, catálogo real C++, recuperação A/B e limite de cache; quatro testes Python de falhas, versões de arte e transmissão HTTP. [Log](evidencias/testes-host.log).
- `python3 tools/validate_assets.py`: 84 entradas, das quais 81 sprites e 3 sons; 1,509,132 bytes (1.439 MiB). Arte gerada efetivamente; contact sheet e demo inspecionadas.
- `python3 -m py_compile tools/*.py tests/*.py`: passou.
- `python3 tools/package_release.py`: gerou aplicação separada, pacote USB, assets e licenças, sem gravação. Comparação byte a byte entre build/distribuição e SHA do manifesto: passou.
- Estruturas no host: Cell=9 bytes, City=94.336 bytes, incluindo scratch de conectividade; save=24.655 bytes. Isso não é medição de heap total no Tab5.

## Cidades de teste (host)

| Cenário | Moradores | Empregos | Receita mensal | Manutenção | Caixa |
|---|---:|---:|---:|---:|---:|
| Pequena equilibrada | 160 | 216 | 1.692 | 209 | 31.422 |
| Média com bairros | 400 | 540 | 4.230 | 241 | 42.293 |
| Déficit após remover água | 232 | 144 | 1.692 | 201 | −9 |

Fixtures em `tests/fixtures/`; geradas por `tests/scenarios.cpp`. A cidade em déficit conserva infraestrutura e zonas; exige recompor caixa/água para recuperar ocupação. Esses dados descrevem a simulação automatizada, não uma sessão física de jogo.

## Arquivos principais

- `main/main.cpp`: inicialização da placa e orçamento de buffers/stacks.
- `main/ui/application.cpp`: estados, interface, fila, gestos, câmera e renderer.
- `main/core/city.*`, `clock.hpp`: regras e passo fixo testáveis.
- `main/storage/save.*`, `assets.*`: formatos, integridade, caches e recuperação.
- `main/network/receiver.*`: AP/HTTP streaming, autenticação e publicação A/B.
- `main/audio/audio.*`: task opcional de PCM e codec BSP.
- `tools/generate_assets.py`, `validate_assets.py`, `install_sd.py`, `upload_assets.py`: pipeline completo.
- `dist/microcity-app-ota.bin`, `dist/usb/`, `dist/microcity-assets.mcap`: distribuição separada.
- `docs/ARQUITETURA.md`, `VALIDACAO.md`, `TERCEIROS.md`: decisões, formatos, limites e checklist.

## Próximo passo executável

No dispositivo, começar pelo checklist de `docs/VALIDACAO.md`: primeira instalação pelo usuário, boot mínimo, revisão/display/touch/SD e logs de baseline. Depois instalar assets manualmente, jogar, testar save/load e somente então AP/transferência v2, interrupções e profiling. Não houve flash, monitor, erase-flash, acesso serial nem publicação no GitHub.

Autotestes não comprovam touch, áudio, convivência SDIO/SDMMC, resistência do cartão à perda de energia, FPS ou latência real. A implementação mantém saves síncronos; sua latência precisa ser medida. Múltiplas cidades, edição de nome/seed, câmera persistida e OTA de firmware não foram anunciados como concluídos.

## Revisão 0.2.0 — retorno da primeira execução

O usuário relatou quadrados nos caracteres brasileiros, seleção ruim por dropdown e pediu câmera/atalhos pelo teclado. Corrigido o uso da fonte Montserrat restrita a ASCII: fonte mínima Noto Sans + Noto Sans Math, 22 px, 200 glifos e 19.781 bytes de bitmap, com cobertura real verificada no cmap e rejeição de .notdef. Fonte C gerada incluída; recuperação continua legível sem SD. Prévia inspecionada em `assets/fonts/preview-ptbr.png`, sem quadrados em acentos, setas ou menos.

Dropdowns removidos. Paletas de botões para Infra/Zonas/Serviços e overlays, com custo, atalho e seleção ativa. Barra mostra ferramenta atual; botão Teclas abre ajuda. Fonte maior acompanhada de ajuste da barra de status e largura dos rótulos.

Entrada implementada para Tab5 Keyboard I²C (driver oficial MIT obtido do upstream, commit b4c36227e2f3373e4e1f8cd7063aefa05cbf72bf) e USB HID Host 1.1.0. Módulo testável comum trata modificadores, bordas de tecla, pressão contínua e soltura. Atalhos não constroem/demolem por si. Movimento/alteração de ferramenta cancela gesto touch em andamento. Ver `docs/CONTROLES.md`.

Validações desta revisão: `./tools/test.sh` passou (ASan/UBSan; cidade/armazenamento/asset/cache; novo teste C++ de teclado; seis testes Python, incluindo cobertura da fonte); `./tools/build.sh build` passou para ESP32-P4; `python3 -m py_compile tools/*.py tests/*.py` passou. Logs separados em `docs/evidencias/*0.2.0.log`. O gerador de fonte foi executado e a prévia visual inspecionada.

Aplicação: 1,215,264 bytes (1.159 MiB). Distribuição e build comparados byte a byte. Arte runtime permanece v1, save permanece v2, engine permanece 1; nenhum save foi alterado. Atualizar somente o BIN e reinstalá-lo pelo launcher. Slots de assets e saves existentes devem ser preservados.

Não foram executados flash, monitor ou serial. A primeira execução anterior foi relatada pelo usuário; os novos glifos, controles e adaptadores ainda precisam de confirmação física no Tab5. Próximo passo: roteiro de regressão 0.2.0 em `docs/VALIDACAO.md`, começando por acentos/paletas e detecção do teclado em Informações.

## Revisão 0.3.0 — serviços reconhecíveis e resposta ao toque

O usuário confirmou funcionamento geral até aqui e pediu arte própria dos serviços, relatando necessidade de segurar/repetir alguns toques. Redesenhados somente os IDs 100–105, em arte v2, mantendo schema, IDs, dimensões, anchors, áudio e todos os demais sprites. Gerador em `tools/service_art.py`; prévia `assets/services-preview.png` inspecionada no computador. Uma regressão compara os hashes de v1/v2 e exige mudança somente desses seis serviços.

O mapa deixou de ser pintado inteiro em uma única chamada da UI. Há limpeza/prefetch/blit por etapas cooperativas, dois buffers de canvas e publicação apenas do frame concluído. Seleção usa a câmera do frame exibido. `main/display/blitter.*` elimina divisões por pixel, usa RGB565 derivado e intervalos opacos e foi comparado pixel a pixel ao renderer anterior nos cinco zooms, incluindo clipping e transparência com furos. Cache tem índices diretos e inclui memória derivada no limite de 4 MiB.

Timer da UI: 10 ms; alvo cooperativo de pintura: 3 ms por passagem; cold reads: no máximo um asset por passagem. Ticks sem mudança diária deixam de redesenhar o mapa; HUD e paletas atualizam sem depender da pintura. Root/botões não rolam, transições foram removidas e hit areas ganharam 3 px. Tolerância de construção/consulta: 18 px euclidianos, câmera: 6 px. Ajustes de volume/impostos não recriam a tela, imposto não recalcula redes, preferências de áudio agrupam gravações após 750 ms.

Validações: `./tools/build.sh build` passou; `./tools/test.sh` passou com ASan/UBSan (quatro suítes C++, sete testes Python e três cidades de cenário); `python3 tools/validate_assets.py` passou; `python3 -m py_compile tools/*.py tests/*.py` passou. Logs em `docs/evidencias/*0.3.0.log`. Cenários econômicos mantiveram os mesmos resultados da revisão anterior.

Firmware: 1,218,304 bytes (1.162 MiB), em partição de 7 MiB. Arte: 1,509,132 bytes. Dois canvas próprios: 2.693.120 bytes; cache completo atual com derivados: 2,533,824 bytes. Não são medições de heap total no hardware. Distribuição/build comparados byte a byte, manifesto SHA e registros A/B verificados.

`dist/launcher/microcity/` contém BIN 0.3.0, slot A v1 (sequência 1) e slot B v2 (sequência 2). Para o cartão inicial, enviar BIN e slot-b.mcap, enviar slot-b.ready por último, preservar A/saves e reinstalar BIN no launcher. Instruções completas em `docs/ATUALIZACAO-0.3.0.md`. Instalações com outra sequência devem usar receptor/instalador que calcula o próximo slot.

## Correção 0.3.1 — reinício no primeiro toque

A revisão 0.3.0 instalava `LV_STYLE_TRANSITION` com descritor nulo para tentar remover a animação dos botões. O LVGL consulta e desreferencia esse descritor na primeira mudança para o estado pressionado, causando falha fatal e reinício. A propriedade inválida foi removida. O callback do canvas também ignora eventos que não sejam pressionar, mover, soltar ou perder pressão antes de consultar o dispositivo de entrada. Um teste de regressão impede a reintrodução do descritor nulo. Engine, arte e save não mudaram.

Não houve gravação, monitor nem acesso serial. A melhoria de latência precisa ser confirmada fisicamente. O alvo de etapa não garante duração máxima de I/O/flush; save, validação de abertura e início de rede ainda têm operações síncronas. Próximo passo: testar toques curtos únicos, paletas, ajustes rápidos, seleção após movimento/zoom e arte dos seis serviços, anotando tempos/memória no Tab5.

## Revisão 0.4.0 — duas etapas adicionais

**Etapa 1, resposta e renderização:** toques simples de construir/consultar deixaram de pedir um frame completo no `PRESSED`; a alteração é desenhada após o comando em `RELEASED`. Movimento de câmera só pede frame quando as coordenadas mudam, e prévia de estrada só quando muda a célula final. Cada passagem cooperativa pinta no máximo 24 sprites, além do teto temporal existente, limitando o período contínuo de blit antes da próxima leitura de entrada. Arraste e estrada conservam atualização visual contínua.

**Etapa 2, informação de jogo:** o resumo agora calcula consumo e capacidade conectada de energia/água. A barra exibe ambos; Consultar informa prédio, zona, estágio, motivo de bloqueio, terreno, poluição e coberturas B/P/S/V. A lógica e a descrição são cobertas por testes host. Engine, arte e save permanecem nas versões 1, 2 e 2; cidades existentes continuam compatíveis.

## Revisão 0.5.0 — mais duas etapas

**Etapa 3, cancelamento de frames obsoletos:** mudanças de câmera, zoom, overlay, mundo e simulação interrompem a preparação do backbuffer anterior. A próxima passagem limpa e monta diretamente o estado mais recente, em vez de concluir/publicar um frame intermediário e iniciar outro. Consultas e construções rejeitadas não solicitam redesenho do mapa. O framebuffer publicado permanece intacto durante o cancelamento.

**Etapa 4, guia inicial:** a primeira nova cidade apresenta três páginas curtas, somente após existir um frame completo do mapa. O conteúdo explica rede viária/utilidades, crescimento e serviços/overlays. Fechar ou concluir grava a preferência no NVS; o botão Guia reabre o conteúdo a qualquer momento. O guia pausa apenas a pintura do mapa enquanto está aberto, mantendo a interface responsiva.

## Revisão 0.6.0 — persistência no microSD e mais três etapas

**Etapa 5, preferências transacionais no cartão:** `/microcity/settings/prefs-a.bin` e `prefs-b.bin` alternam registros de 40 bytes com magic, versão, sequência, limites validados e CRC32. Cada gravação executa flush, sincronização e releitura byte a byte; corrupção/truncamento do registro mais novo recua para o anterior. NVS é apenas fonte de migração quando ainda não existe configuração no SD.

**Etapa 6, sessão restaurada:** volume, mute, conclusão do tutorial, câmera, zoom, ferramenta, overlay e velocidade são agrupados após 750 ms de inatividade e persistidos no SD. Câmera e controles só são aplicados se a seed da configuração corresponder à cidade carregada. Salvar manualmente inclui cidade e sessão; reinserir o SD recarrega preferências sem descartar a cidade em RAM.

**Etapa 7, criação reproduzível:** Nova cidade abre uma tela própria com seed decimal e hexadecimal, botão para sortear outra seed e confirmação explícita. O terreno continua determinístico e a seed segue dentro do save. Firmware permanece na flash por ser o executável de boot; assets, áudio, saves e dados mutáveis ficam no microSD.

## Revisão 0.7.0 — mais cinco etapas

**Etapa 8, três slots de cidade:** cada slot usa seu próprio diretório A/B em `/microcity/saves/slot-N/`. O slot ativo também fica nas preferências do SD. A estrutura antiga `/microcity/saves/city-*.sav` permanece como fallback de leitura do slot 1; ao salvar, a cidade passa ao diretório novo sem alterar o formato save 2.

**Etapa 9, navegador de cidades:** Continuar abre três cartões com população, caixa, ano e seed. Os resumos passam pela mesma validação/CRC e fallback A/B usados no carregamento real; slot inválido aparece vazio sem afetar os demais.

**Etapa 10, proteção contra sobrescrita:** Nova cidade permite escolher o slot. Um slot ocupado exige uma segunda ação explícita de confirmação. A cidade inicial é gravada imediatamente; falha no SD retorna ao menu sem anunciar criação concluída.

**Etapa 11, painel de gestão:** o novo botão Finanças abre receita, manutenção, resultado mensal, impostos, demandas, utilidades, locais sem atendimento e quantidades de vias/serviços. O painel deriva apenas do estado autoritativo da cidade.

**Etapa 12, alertas de utilidades:** a simulação conta locais consumidores sem energia e sem água. Mudanças no déficit geram aviso com quantidades; a normalização também é informada. Os contadores são derivados e não mudam o formato do save.

## Revisão 0.8.0 — correção do mapa azul e mais cinco etapas

**Etapa 13, publicação garantida:** a atualização diária deixava `redraw` pendente e cancelava o backbuffer ativo. Em um Tab5 onde o frame completo ultrapassasse o intervalo de um dia, isso podia repetir indefinidamente e manter apenas o fundo azul. Ticks agora nunca abortam o frame atual: ele é publicado integralmente e o estado mais novo é desenhado em seguida. Interações diretas ainda podem substituir trabalho obsoleto.

**Etapa 14, progresso inicial:** enquanto nenhum frame foi publicado, a barra inferior mostra percentual calculado das fases de limpeza, preparação dos assets e pintura. O tutorial aguarda pelo menos 500 ms após a primeira publicação, evitando disputar a primeira apresentação do canvas.

**Etapa 15, modais determinísticos:** Guia, Teclas, Finanças, construções e overlays congelam o avanço da simulação sem alterar 1×/2×/4×. Como o relógio-base continua sendo atualizado, fechar um painel não provoca processamento acumulado.

**Etapa 16, recuperação da câmera:** Home e o botão Centralizar mapa restauram posição e zoom conhecidos. A mudança cancela apenas o backbuffer obsoleto e é persistida no microSD.

**Etapa 17, resumos operacionais:** o navegador de slots acrescenta felicidade e quantidade de locais sem energia/água. Esses dados vêm do save totalmente validado e reconstruído, não de metadados não confiáveis.

## Revisão 0.9.0 — modo Obras, educação e vias maiores

**Etapa 18, entrada Tab5 prioritária:** o teclado A164 passa a usar I²C1 nos pinos oficiais 0/1, enquanto o BSP interno foi movido para I²C0. Atalhos novos usam teclas presentes na matriz física; USB continua secundário. A tela Informações distingue barramento iniciado, dispositivo encontrado e tecla recebida.

**Etapa 19, construção sem oclusão:** `B` alterna o modo Obras persistente. Prédios R/C/I e serviços são substituídos por marcadores baixos, distintos da linha de zona vazia, revelando ruas e células atrás das fachadas.

**Etapa 20, zoneamento por área:** R/C/I usa retângulo de toque/arrasto com prévia e custo. A operação é transacional: terreno, ocupação ou caixa inválidos rejeitam a área inteira.

**Etapa 21, equipamentos multicélula:** escolas primária/secundária ocupam 2×2, universidade 3×3 e parque municipal 2×2. Cada célula registra a posição no footprint; acesso é compartilhado e demolição por qualquer parte remove o conjunto. Custos/manutenção/capacidade são cobrados somente na âncora.

**Etapa 22, avenidas e arte 3:** avenidas têm desenho de duas faixas, custo/manutenção próprios e participam da mesma rede viária. O pacote visual adiciona 24 sprites para Obras, educação, parque e 16 conexões de avenida. Saves v2 continuam legíveis; gravações novas usam v3 com o mesmo tamanho de 24.655 bytes.

**Arte 4, acabamento cívico:** os IDs 132–135 deixaram de usar blocos genéricos. Escola primária ganhou prédio baixo, pátio e bandeira; escola secundária, dois pisos, relógio e quadra; universidade, alas, colunas e cúpula; parque municipal, lago, ponte, coreto, caminhos e árvores. O gerador separado `tools/civic_art.py` produz a prévia `assets/civic-preview.png`. Engine, firmware e save não mudaram.

## Revisão 0.10.0 — expansão plana e transporte

**Etapa 23, mapa 128×128:** cidades novas quadruplicam a área, sem relevo. O save v4 tem 98.383 bytes. Saves v2/v3 de 64×64 são lidos, preservados no quadrante original e migrados ao próximo salvamento.

**Etapa 24, utilidades multicélula:** a ligação com a rede viária usa raio Chebyshev 2, cobrindo fachadas, diagonais e esquinas de 90°. A âncora consome recursos uma vez e replica acesso/energia/água para todas as células reservadas.

**Etapa 25, demanda e Obras:** R/C/I usam fórmulas independentes limitadas a 5–95, considerando população, empregos próprios e impostos. Nove marcadores novos distinguem os três estágios de cada categoria.

**Etapa 26, pontes e portos:** rotas sobre água criam ponte com 16 conexões visuais. Porto 3×2 só pode ser colocado em terra junto à água e, quando ligado e atendido, soma 120 empregos e $600 de receita mensal.

**Etapa 27, transporte limitado:** o frame admite no máximo 12 veículos e 4 navios decorativos. Navios aparecem somente perto de um porto. A posição é determinística, não entra no save e não cria agentes na simulação.

**Etapa 28, desenho visível:** o renderer inverte os cantos da tela e percorre apenas as diagonais isométricas no viewport, mantendo uma lista fixa de 4.096 itens. O lote por slice passou de 24 para 48 sprites sob o mesmo teto temporal de 3 ms. Não foi adicionada escrita concorrente no framebuffer: LVGL, cache SD e ordem alfa continuam em uma única tarefa até haver perfil físico que justifique particionamento seguro.

Arte v5 contém 138 sprites e 3 sons; pacote MCAP 2.855.808 bytes. Testes host com ASan/UBSan passaram. Build ESP32-P4 0.10.0 passou; binário OTA 1.231.872 bytes, SHA-256 `4ef2ee9e194a39c5f9636455a01a9619f93f337d29fb0b388c9f35f1b8630a1d`. O pacote do launcher usa slot A sequência 5; enviar `.mcap` antes de `.ready`. Nenhum flash, monitor ou porta serial foi usado.

**Arte 6, correção de escala:** a ampliação por nearest-neighbor do sprite inteiro também ampliava a base de uma célula, fazendo escolas, universidade, parque municipal e porto cobrirem vias próximas. Os assets agora usam canvas transparente do footprint, desenham cada célula reservada em escala nativa e centralizam a construção sem redimensioná-la. Pacote 2.818.944 bytes, SHA-256 `af70cfff7f8aee8fac0cc4008b539d025658d0a0844f59f36c03d9004d3c495e`; slot B sequência 6. Firmware e saves não mudaram.

**Revisão 0.10.1 / arte 7:** o teste no Tab5 mostrou que os veículos tornavam o jogo injogável; sua criação no renderer foi removida integralmente. Navios permanecem limitados a quatro. Para evitar que os equipamentos voltassem a parecer 1×1, somente a construção é ampliada: 1,5× nos lotes 2×2/3×2 e 2× na universidade 3×3, enquanto o piso continua composto de tiles nativos dentro do footprint. Testes host e build ESP32-P4 passaram. BIN SHA-256 `a184a5f2080eb0a9ab6c78faec477a85391442b807fac5095597a9dab99a2b17`; arte SHA-256 `57037a9aee7da76a3a00a2d6dfe21a92f80061179331eead3195b494dd06643e`; slot A sequência 7.

**Arte 8, vias contínuas e campi:** as conexões de ruas e avenidas ultrapassam a borda lógica do tile e se sobrepõem ao vizinho, removendo frestas e dentes por célula. Escolas, universidade, parque municipal e porto agora usam prédio central e alas nativas distribuídas pelo footprint, sem nearest-neighbor sobre portas, bandeiras ou contornos. Pacote SHA-256 `e15d0f0634094819e911c9fcff3a6274c3bcab39587067252e6ab46d155fa2b1`; slot B sequência 8. Firmware e saves não mudaram; testes host passaram.

**Revisão 0.10.2, ordem de composição:** o teste físico mostrou somente o núcleo dos campi porque as células `stage > 1`, desenhadas depois da âncora, cobriam as alas com tiles de terreno. No modo normal essas células agora omitem a camada base; o asset âncora fornece todo o lote. O modo Obras continua mostrando cada célula. Um teste de regressão verifica explicitamente essa guarda. Testes host e build ESP32-P4 passaram; BIN SHA-256 `bd2ae14f8cc9cd31b76a8c0b203fcd74d3b14ccbb1b171b05a07614ffdeb3608`. Assets e saves não mudaram.

**Revisão 0.10.3 / arte 9:** o novo sprite 189 é um losango tracejado transparente, aplicado a cada célula de equipamentos multicélula depois da composição do campus. Ele delimita a ocupação sem repintar terreno. Testes host e build ESP32-P4 passaram. BIN SHA-256 `b3c22dd636e0cbe59205488dce6908bd24a8dc26293277017c359083712fe5eb`; pacote SHA-256 `f9ac33aba3d443de0425fb2e5fdc8956d4bfbc0e2a43f1e8f2d1c4f21fe40ade`; slot A sequência 9.

**Primeiro perfil serial físico:** Tab5 rev. 2/P4 rev. 1.3/32 MB PSRAM confirmados. Em cidade aberta, mínimo interno 150.563 bytes, PSRAM mínima 26.756.240, cache 1.706.368 bytes com 439/39 hits/misses, maior frame frio 1,928 s, slice 80,2 ms, tick 67,8 ms e fila 20 µs. O monitor USB reiniciou o P4 ao abrir; após 39,5 s houve `HP_SYS_HP_WDT_RESET` precedido por probes I²C expirando a cada 2 s. Ver detalhes e ressalvas em `docs/VALIDACAO.md`.

**Segundo perfil, firmware 0.10.3:** 88 s monitorados sem watchdog/panic. Mínimo interno 150.495 bytes, PSRAM mínima 25.858.100, cache 2.079.616 bytes com 2.032/51 hits/misses, frame frio 1,891 s, slice 72,7 ms, fila 11,8 ms, tick 68,0 ms e autosave 511 ms. Timeouts I²C de probe persistem a cada 2 s. O monitor foi encerrado voluntariamente.

## Revisão 0.11.0 — riscos, iluminação e interface

**Desastres simples:** incêndio, tempestade e tremor são estados determinísticos de três dias. Há no máximo uma tentativa mensal adicional; somente construções, utilidades e serviços são elegíveis. Ruas, avenidas e pontes são rejeitadas pela própria regra. Bombeiros eliminam incêndio coberto; tempestade causa dano e tremor pode remover o prédio. Equipamentos multicélula são removidos como unidade. O save v5 persiste estados 1–9 no byte de evento e lê v2–v4.

**Ciclo mensal:** uma camada LVGL azul, não clicável, varia de 0 a 72 de opacidade conforme o dia. O ciclo vai de noite a dia e retorna à noite em 30 dias, sem depender do FPS. A barra informa Dia, Crepúsculo ou Noite.

**Interface:** Finanças usa cartões separados para economia, impostos/demanda e infraestrutura; Guia ganhou conteúdo em cartão e riscos; Teclas foi dividido em navegação e construção; Ajustes separa impostos de som/armazenamento. Botões de fechar e navegação foram ampliados.

Arte v10 contém overlays próprios de tempestade e tremor. Testes host, migração/save v5 e build ESP32-P4 passaram. BIN SHA-256 `b80b8c2ffed00ff144a5a763ad35b27c546bb4622444fd72f4556f997de89d64`; pacote SHA-256 `a072cc8a00bcfba956ce51bd55ea4716b1bbca72a4885d9256c2454d21f328e0`; slot A sequência 10.

## Aceite físico final

Em 07/11/2026, o usuário informou que todos os testes da 0.11.0 passaram e considerou o jogo pronto. Saves foram corrompidos manualmente em editor de texto, inclusive com remoção de bytes finais, e a aplicação tratou os arquivos inválidos. Somam-se às confirmações anteriores de meia hora de jogo, touch, teclado Tab5, áudio, três slots, persistência após remoção da bateria, funcionamento offline, impostos e atualização de assets pelo launcher.

O planejamento funcional está concluído. O OTA Wi‑Fi de assets não será testado por decisão do usuário; o Web-GUI do launcher é o caminho de distribuição adotado. Relevo, metrô, rodovias, clima, campanha, desastres complexos e veículos são expansões opcionais, não pendências desta versão.
