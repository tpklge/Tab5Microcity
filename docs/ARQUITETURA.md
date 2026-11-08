# Arquitetura, formatos e decisões

## Plataforma

ESP-IDF 5.4.4 instalado localmente; C++17; ESP32-P4. BSP oficial 1.3.2 suporta os drivers ST7123/ILI9881C e touch pelo próprio BSP. A revisão do Tab5, controlador efetivo, orientação e firmware do C6 ainda precisam ser conferidos fisicamente. Referências: [BSP oficial](https://github.com/espressif/esp-bsp/tree/master/bsp/m5stack_tab5), [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5).

Display por LVGL 9, rotação 90°, framebuffer do BSP e dois buffers parciais DMA. A aplicação usa um canvas RGB565 de 1280×526 (1.346.560 bytes em PSRAM). Não há um objeto LVGL por célula. Tiles nominais 64×32; imagens 64×96, anchor (32,79) no centro da base. Transformação inversa e arredondamento ao centro selecionam o losango. Ordenação por coluna+linha, depois linha. Culling considera até 140 pixels de altura. Zoom 0,5–1,5, em passos de 0,25, mantém o centro da tela.

`main/main.cpp`: inicialização. `main/ui/`: estados, comandos, entrada e renderer. `main/core/`: regras independentes de hardware. `main/storage/`: saves, catálogo, cache e slots. `main/network/`: AP e servidor. `main/audio/`: fila PCM e codec. `tools/`: arte e instalação. `tests/`: regras e falhas. Assets não são incluídos por CMake.

O timer LVGL (50 ms) é proprietário da cidade. Callbacks de touch/botões enfileiram ações; callbacks de seleção só mudam a ferramenta. A simulação tem tick fixo de 250 ms e até quatro ticks por atualização. Pausa mantém consulta/câmera. Velocidade altera somente o acumulador. Um dia = 8 ticks, mês = 30 dias, ano = 360 dias. Suspensão longa descarta excesso, sem progresso offline. Configurações interrompem ticks.

O receptor HTTP tem task própria, opera somente com a cidade encerrada e o catálogo fechado. A task de áudio recebe cópias de PCM; não lê o SD e não mantém referências ao cache. Codecs e serviços de plataforma são compartilhados durante toda a aplicação. Cada partida libera City e cache ao sair. Stacks configuradas: LVGL 24 KiB, HTTP 24 KiB, áudio 6 KiB, main 24 KiB (liberada quando app_main retorna). Fila de áudio: dois blocos de 4.800 bytes. Fila de ações: 31 eventos úteis. Não há task adicional de simulação.

O SD usa mount FAT do BSP (SDMMC slot 0); ESP-Hosted usa SDIO para C6 nos GPIO 8–15. A convivência real exige teste. Nomes longos FAT habilitados; formatação automática desabilitada. Remontagem fecha catálogo/receptor, preserva City em RAM e reabre handles. O renderer pausa após falha de leitura. Leituras já no cache não detectam imediatamente remoção física; o BSP não disponibiliza pino de detecção.

## Regras e equilíbrio

Valores centralizados em `Definitions` em `main/core/city.cpp`. Dinheiro em unidades inteiras, caixa inicial 30.000, sem derrota permanente por insolvência. Construções pagas exigem saldo. Impostos 1–20%, padrão 9%; cada ponto acima de 9 reduz demanda em 5, limitado a 0–100. Receita/manutenção cobradas apenas a cada 30 dias. Dívida é saturada em ±1 bilhão para impedir overflow.

Estradas custam 10; remoção de vegetação adiciona 5. Zonas custam 10; demolição custa 5. Usina 1.200 e manutenção 60; água 800/40; bombeiros e polícia 600/25; saúde 800/35; parques 100/3. Capacidade de cada utilidade: 800 por componente. Consumo: 4 por estágio; zonas vazias/serviços consomem 4, vias e terreno vazio não consomem. Atendimento em ordem crescente de índice, explicitamente determinístico. Fontes produzem quando adjacentes à via; não há rede global. Um prédio adjacente a componentes distintos pertence ao de menor ID, sem conectar redes implicitamente.

Serviços: BFS até 12 passos viários, inclui as células adjacentes à via alcançada. Necessitam de acesso/água/energia. Parques usam distância Manhattan ≤5. Poluição industrial se espalha até distância 5. Overlays usam as mesmas flags/campos da simulação. Residências: 8 moradores/estágio, comércio 8 empregos/estágio e indústria 12. Três estágios por categoria e duas fachadas por estágio. Crescimento a cada cinco dias, abandono após 30 dias ruins, sem apagar zona. Demanda residencial responde a empregos; comercial a população; indústria a desemprego. Felicidade considera utilidades, serviços, parques e poluição. Incêndios: uma célula sorteada por dia, chance limitada por poluição; cobertura evita ignição e apaga fogo, sem cobertura um estágio é perdido em três dias.

Não há moradores/carros individuais. Parâmetros estão no firmware, separados da atualização visual. Ajustes de balanceamento exigem avaliar saves existentes.

## Pacote MCAP v1

Inteiros little-endian. Cabeçalho de 60 bytes: magic `MCAP`, runtime u32=1, engine mínima u32, máxima u32, versão de arte u32, número de entradas u32, tamanho do payload u32 e SHA-256 do payload (32 bytes). Versões são inteiros, não strings lexicográficas. Engine atual=1. Limite total 32 MiB, até 256 entradas.

Catálogo: entradas de 20 bytes `<6H2I`: ID, largura, altura, anchorX, anchorY, formato, offset relativo ao payload, comprimento. Catálogo vem antes dos dados; offsets precisam ser contíguos, sem sobreposição. IDs únicos de 0–255. Imagens: formato 1, RGB565 LE + alpha de 8 bits intercalados, máximo 128×192. Blitter usa alpha binário (>=128). Sons: formato 2, IDs 200–202, 2.400 amostras mono PCM signed 16-bit LE a 16 kHz, 4.800 bytes. Nenhum script executável é aceito.

IDs: 0–2 solo; 10–25 água/bordas; 30–32 árvores; 40–55 vias; 60–62 zonas; 70–75 casas; 80–85 comércio; 90–95 indústria; 100–105 serviços/utilidades; 110–113 seleção/erro/abandono/fogo; 120–131 ícones; 200–202 áudio. Máscara viária: bit 0 norte (linha−1), 1 leste, 2 sul, 3 oeste. PNG/WAV são fontes autorais; o pacote runtime é MCAP. Manifesto JSON externo fornece inventário, hashes, dimensões e versão do gerador. O receptor valida o catálogo binário autoritativo, não interpreta o JSON.

Cache LRU de até 128 entradas e no máximo 4 MiB. Carga preguiçosa dos recursos visíveis em uma passada anterior ao blit, arquivo do pacote permanece aberto. Sem decode PNG no dispositivo. Um catálogo pertence a uma única versão durante toda a partida. Contadores de hits/misses e bytes na tela de informações e logs. Uma cena com mais recursos distintos que a capacidade do cache precisaria de batching; o pacote atual cabe integralmente abaixo do limite, sem exigir pré-carga.

## Instalação transacional

`microcity/slot-a.mcap`, `slot-b.mcap`; cada slot tem `.ready` independente. Registro ready de 12 bytes: magic u32 `0x59444152`, sequência u32, CRC32 dos primeiros 8 bytes. Boot ordena registros válidos por sequência e verifica catálogo, tamanhos, compatibilidade, sprites obrigatórios e SHA-256 de cada candidato. Se o novo falha, tenta anterior. Sem candidato, a UI mínima permanece disponível.

Cidades usam três diretórios independentes `saves/slot-1..3`, cada qual com `city-a.sav` e `city-b.sav`. A raiz antiga de saves é fallback somente do slot 1. `inspectCity` valida e reconstrói o save antes de produzir metadados para o navegador. Preferências A/B guardam o slot ativo junto da sessão.

Upload: selecionar inativo em relação ao candidato completo ativo; remover somente seu ready; verificar espaço (pacote+64 KiB); receber em blocos de 4 KiB; flush/fsync; validar pacote; gravar/flush/fsync ready; reabrir e validar. Sem rename assumido atômico. O slot anterior permanece. Falhas físicas de FAT/energia ainda podem afetar o cartão; testes de host não comprovam resistência elétrica.

Servidor só inicia por ação explícita, AP WPA2 com senha aleatória, token Bearer aleatório de 128 bits, sem logs de credenciais, timeout de leitura de 5s e sessão de 10 minutos. Sem caminhos enviados pelo cliente: um único corpo binário, destino fixo, sem ZIP, traversal ou links. HTTP local não usa TLS; a proteção de transporte depende da rede WPA2 temporária. SHA-256 detecta corrupção comparada ao cabeçalho; não é assinatura de autoria. Quem conhece senha e token pode instalar um pacote compatível. Não há assinatura própria, retomada ou servidor permanente. Modo AP dispensa guardar credenciais de rede doméstica. Voltar encerra a sessão e limpa as credenciais da instância.

## Save v2

Arquivo exato de 24.655 bytes, little-endian; magic `MCSV` (`0x5653434d`), versão u32=2, lado u32=64, seed u32, PRNG xorshift32 u32, ticks u32, dias u32, dinheiro i64, três impostos u8, sequência de save u32, nome de 32 bytes terminado em zero, 4.096 células × seis bytes (terreno, zona, ocupação, estágio, insatisfação, fogo), CRC32 final. Nenhum ponteiro, offset de sprite ou cache persistido.

`microcity/saves/city-a.sav` e `city-b.sav` alternam preservando o último válido. Validação completa precede alteração de City. Save novo usa sequência maior que ambos os arquivos, inclusive ao criar outra cidade; escrita completa, flush/fsync e releitura comparam bytes. Carregamento tenta maior sequência e depois backup, sem apagar arquivo inválido. Autosave: 120s reais de partida ativa (pausa não conta), se alterada; a velocidade não acelera a frequência de escrita. Save manual/saída usa snapshot consistente porque a task dona da cidade serializa entre ticks. Gravação é síncrona e instrumentada; latência precisa de medição e pode exigir worker de snapshot em hardware.

## Revisão de interface e entrada 0.2.0

`main/input/controls.*` é testável no host: matriz → usage HID, estados combinados de teclas, bordas retidas entre frames e atalhos. `keyboard.*` contém adaptadores I²C e USB, com task de 6 KiB, fila de 64 registros e fila de oito conexões USB. O BSP adiciona a task de host USB de 4 KiB e HID Host usa mais 4 KiB. São serviços compartilhados da aplicação, preservados ao trocar de partida. Medir o novo pico interno no Tab5.

A UI recebe estado/comandos somente em seu timer; câmera usa tempo transcorrido limitado e velocidade de 420 px/s (850 com Shift). Teclas de ação têm uma borda por pressionamento; relatórios repetidos não duplicam ações. Modificadores ficam associados ao pressionamento, inclusive quando o pacote de soltura chega antes do próximo frame. Uma ação de teclado que altera câmera/ferramenta cancela o gesto touch em curso.

Paletas de construção por categoria e de overlays substituem os dropdowns. Fonte mínima Noto Sans/Noto Sans Math, OFL-1.1, 22 px e 200 glifos, compilada para menus e recuperação. Geração reproduzível em `tools/generate_ui_font.py`, 19.781 bytes de bitmap, sem TTF completo no firmware. O gerador verifica cmap real e não admite glifo .notdef. Sprites/áudio e formatos MCAP/save permanecem inalterados.

## Revisão 0.3.0 — renderização cooperativa

A implementação atual substitui o desenho síncrono descrito nas seções históricas: a aplicação é atendida a cada 10 ms; beginDraw gera uma lista estável e captura a câmera; drawStep limpa/prefetch/pinta em pequenas etapas. A apresentação troca os dois buffers próprios de canvas somente depois da conclusão. A seleção usa displayedCamera, correspondente ao frame publicado. Trocas de tela cancelam jobs antes da liberação de catálogo/partida. O proprietário de City/SD/cache continua sendo a task LVGL, sem nova task concorrente de renderização.

`main/display/blitter.*` contém desenho testável sem hardware. `Assets` acrescenta índices diretos por ID e cache de cores RGB565/limites opacos por linha, com memória derivada incluída nos 4 MiB. No conjunto atual, cache completo=2.533.824 bytes; dois canvas=2.693.120 bytes. A alocação de cores inclui os limites de linha, para manter esses blocos maiores na política de PSRAM já configurada.

A UI foi separada do map draw. Rolagem acidental dos botões/raiz foi removida; tolerância de construção=18 px euclidianos, câmera=6 px. Settings atualiza widgets existentes e agrupa gravações NVS de volume/mute após 750 ms ociosos. O limite de 3 ms por etapa é alvo cooperativo; não cobre garantias de tempo de SD/flush. Detalhes, limitações e procedimento A/B para arte v2 em `ATUALIZACAO-0.3.0.md`.
