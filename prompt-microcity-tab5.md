# Prompt de desenvolvimento — MicroCity, jogo independente para M5Stack Tab5
Versão 2.0 — 07/11/2026

Este arquivo é uma instrução completa para criar um jogo independente e seu próprio firmware para o M5Stack Tab5. O agente deve poder iniciar em um repositório vazio ou usar uma base técnica mínima da placa. As metas de memória, desempenho e tamanho de pacote abaixo são metas de engenharia a validar, não medições já realizadas no dispositivo.

---

Você é responsável por criar **MicroCity** — nome provisório —, um jogo original e independente de construção e gestão de cidades para o **M5Stack Tab5**. Desenvolva um projeto próprio, com firmware, inicialização, menu principal, configurações, renderização, entrada, armazenamento, áudio e distribuição de assets. O jogo será a aplicação principal do dispositivo, e não um módulo de outra aplicação.

A referência funcional é a experiência de construir uma cidade de SimCity clássico, com apresentação isométrica inspirada em jogos como SimCity 3000. Desenvolva uma implementação nativa, com escala adequada ao hardware e identidade visual própria.

**Requisito obrigatório: você deve criar os sprites e as ferramentas que os geram. Os assets do jogo devem ficar no microSD, em pacote separado, enviado e atualizado pela rede/OTA. Eles não devem ser incorporados ao binário principal do firmware.**

## 1. Forma de trabalho e resultado esperado

1. Examine o diretório/repositório de destino antes de modificar arquivos. Leia AGENTS.md e instruções aplicáveis. Se estiver vazio, inicialize a estrutura do projeto e as instruções de build.
2. Identifique o hardware e a base técnica disponíveis; selecione e fixe versões compatíveis de SDK, BSP e bibliotecas. Não pressuponha que já exista aplicativo, infraestrutura de OTA ou framework configurado.
3. Apresente um diagnóstico inicial curto, um plano executável e os riscos técnicos concretos.
4. Implemente o trabalho em marcos verificáveis. Não encerre a tarefa apenas com um plano, pseudocódigo ou telas sem funcionamento.
5. Tome decisões técnicas reversíveis dentro do escopo e registre as premissas. Pergunte somente quando faltar uma informação que impeça uma decisão material.
6. Preserve eventuais arquivos e mudanças preexistentes do usuário. Crie o jogo em uma estrutura própria, sem importar dependências de projetos não fornecidos.
7. Implemente o firmware completo necessário ao jogo e seus serviços. Reutilize drivers e componentes de plataforma mantidos, em vez de reescrever drivers de display, touch, SD ou rede sem necessidade.
8. Quando partir de uma base técnica existente, evite atualizações indiscriminadas. Quando começar do zero, escolha versões compatíveis e registre-as para reproduzir o build.
9. Registre comandos executados, resultados de compilação e testes. Distinga testes de host, compilação para a placa e testes físicos.
10. Se o hardware ou uma credencial não estiver disponível, conclua o que puder ser construído e validado no ambiente e descreva precisamente a validação restante.
11. Não afirme que o jogo está fluido, que OTA funcionou ou que o touch está correto sem evidência correspondente.
12. Se a tarefa atravessar sessões, mantenha documentação de progresso com decisões, arquivos relevantes e próximo passo executável.

O resultado deve ser um jogo independente jogável, com inicialização e menu próprios, economia e crescimento reais, salvamento, assets originais no SD e um fluxo funcional de instalação/atualização desses assets. Deve ser possível compilar e instalar o projeto sem possuir qualquer outra aplicação.

## 2. Contexto e restrições de plataforma

Este é um projeto novo. Não há catálogo de jogos, menu hospedeiro, identidade visual, framework ou infraestrutura de atualização herdados a assumir. Crie a aplicação dedicada de construção de cidades, reutilizando apenas os componentes técnicos de plataforma escolhidos e suas interfaces documentadas.

Referência de hardware do Tab5:

- ESP32-P4, processador RISC-V de dois núcleos; a ficha do Tab5 informa 360 MHz.
- 32 MB de PSRAM e 16 MB de flash.
- Tela de 1280 × 720, com orientação efetiva a verificar na aplicação.
- Touch e slot microSD.
- Rede por meio do conjunto ESP32-C6 e do suporte correspondente da plataforma; a configuração faz parte do projeto.
- Possível teclado ou mouse USB: considerar suporte adicional quando disponível no BSP, sem torná-los obrigatórios.

Confirme revisão da placa, display/touch, memória livre, configuração de PSRAM, partições, sistema de arquivos e ambiente de build reais.

Não assuma que os 32 MB de PSRAM estejam disponíveis para a simulação: framebuffer, drivers, rede, áudio, interface e caches compartilham os recursos. Não trate PSRAM como substituto indistinto de memória interna, buffers de DMA ou memória exigida por callbacks.

Não execute a versão PC, Windows ou emulador x86. O jogo deve ser nativo.

## 3. Fundação do projeto e validação da plataforma

Comece pelo que houver no diretório de destino. Se o repositório estiver vazio, crie a base do firmware; se houver um exemplo/BSP da placa, use-o como referência técnica. Não dependa de arquivos, serviços ou módulos de outro projeto.

Preferência inicial: C++ sobre ESP-IDF, com suporte mantido para o Tab5. Um ambiente Arduino/PlatformIO ou outro caminho suportado pode ser escolhido se houver vantagem técnica demonstrada. Registre a decisão e as versões, sem prometer compatibilidade de bibliotecas que não tenha verificado.

Configure ou identifique e documente:

| Área | O que identificar |
|---|---|
| Build | ESP-IDF, Arduino, PlatformIO ou outro fluxo; versões e comandos |
| Gráficos | Framework, framebuffer, formato de pixel, flush, rotação e sincronização |
| Entrada | Coordenadas de touch, gestos, teclado e mecanismo de eventos |
| Aplicação | Máquina de estados, boot, menu próprio, partida, pausa, recuperação e foco |
| Áudio | Backend, mixer, volume, mute e propriedade do dispositivo |
| microSD | Mount, filesystem, caminhos, locks, tratamento de remoção e espaço livre |
| Rede/OTA | Configuração Wi-Fi, servidor de transferência, autenticação, upload de assets e limites; OTA de firmware se adotado |
| Concorrência | Tasks, threads, timers, mutexes e watchdog |
| Recursos | Heap interno, PSRAM, tamanho do firmware e partições OTA |
| Convenções | Estilo, logs, nomes, testes e documentação |

Implemente um serviço próprio e pequeno de recepção dos assets pela rede, apoiado nas bibliotecas da plataforma. Caso o BSP forneça um exemplo útil, adapte-o. **Transferir assets para o SD não deve exigir recompilar nem reflashear o firmware.**

Preserve a separação:

- Firmware: código da aplicação, drivers e UI mínima de inicialização/recuperação.
- OTA de firmware, se adotado: atualização de código por fluxo e particionamento próprios.
- OTA de assets: atualização obrigatória dos arquivos do jogo no microSD.
- Savegame: cidade do jogador, independente das versões visuais.

Entregue tabela de partições compatível com a flash real e o método de instalação escolhido. O serviço de assets não pode depender da existência de OTA de firmware.

### 3.1 Inicialização e estados próprios

Implemente uma máquina de estados que cubra boot, diagnóstico, menu principal, nova cidade, carregamento, partida, pausa, configuração de rede, instalação de assets e recuperação.

Na primeira inicialização:

1. Inicializar os periféricos essenciais por meio do BSP escolhido.
2. Disponibilizar uma UI mínima legível, mesmo sem cartão ou pacote visual.
3. Verificar microSD e versão válida de assets.
4. Se faltar pacote, oferecer instalação por rede ou cópia manual documentada para o SD.
5. Abrir o menu principal do jogo quando os recursos obrigatórios estiverem prontos.

O menu próprio deve oferecer Nova cidade, Continuar/Carregar, Configurações, Gerenciar assets e Informações. A ação de sair de uma partida retorna a esse menu. Se houver encerramento do dispositivo, implemente-o apenas pelo mecanismo suportado pela placa, protegendo operações de gravação.

### 3.2 Configuração de rede

Crie uma tela/fluxo para configurar Wi-Fi e abrir o modo de instalação de assets. Defina armazenamento de credenciais, indicação de estado, timeout e recuperação de conexão conforme o SDK. Um modo de provisionamento/AP temporário pode ser usado se necessário, com autenticação apropriada e encerramento explícito.

Depois de instalado o pacote, o jogo deve funcionar offline. Não condicione uma partida, o carregamento da cidade ou os sprites a um servidor externo.

## 4. Escopo funcional e prioridades

### 4.1 Versão jogável obrigatória

Implemente:

- Cidade em grade de **64 × 64 células**.
- Terreno plano com terra, água e vegetação.
- Visual isométrico por sprites 2D.
- Câmera com deslocamento e zoom limitado.
- Construção e demolição.
- Estradas com conexões visuais automáticas.
- Zoneamento residencial, comercial e industrial.
- Crescimento de construções em zonas válidas.
- Energia e água com capacidade e conectividade.
- Bombeiros, polícia, saúde e parques com cobertura simplificada.
- População, empregos, demanda, felicidade, poluição e risco de incêndio.
- Receita de impostos, custos de manutenção e saldo.
- Calendário, pausa e velocidades 1×, 2× e 4×.
- Inspeção de terrenos/construções e overlays informativos.
- Salvamento manual e automático, com recuperação.
- Menu principal próprio e retorno seguro ao encerrar uma partida.
- Assets próprios, gerados por você e carregados do microSD.
- Instalação e atualização OTA desses assets com integridade e recuperação.

A primeira cidade precisa permitir um ciclo completo: construir infraestrutura → zonear → receber moradores/empregos → arrecadar → ampliar serviços → observar consequências.

### 4.2 Expansões condicionadas à validação

Somente após a versão básica estável:

- Mapas 128 × 128.
- Prédios de múltiplas células.
- Pontes, relevo, metrô ou rodovias.
- Clima, ciclo visual de dia/noite e desastres complexos.
- Veículos decorativos adicionais.
- Cenários e metas de campanha.

Não comprometa o ciclo básico tentando reproduzir todos os sistemas de SimCity 3000. Não anuncie expansões como implementadas.

## 5. Arquitetura de software

Defina uma arquitetura de firmware dedicada, de acordo com a base técnica selecionada na seção 3. Prefira lógica do jogo testável sem placa e interfaces pequenas entre a engine e os drivers.

Separe, conceitualmente:

- Modelo do mundo e definições de construções.
- Comandos do jogador.
- Simulação e economia.
- Redes e coberturas.
- Renderização e câmera.
- Entrada e interface.
- Catálogo/carregador/cache de assets.
- Savegame.
- Instalação OTA do pacote de assets.
- Inicialização, navegação e configurações da aplicação.
- Adaptadores da plataforma e dos drivers escolhidos.

Crie uma estrutura de diretórios própria e documentada para firmware, lógica testável, ferramentas de assets, testes e documentação. Use as convenções do SDK escolhido; não crie uma arquitetura genérica extensa sem necessidade.

Regras:

- Não instanciar um objeto de UI pesado por célula.
- Evitar alocações por frame ou por tick.
- Usar IDs estáveis em vez de ponteiros na representação persistida.
- Preferir estruturas compactas e buffers reutilizáveis.
- Separar dados autoritativos de caches derivados.
- Evitar dependência circular entre renderer e simulação.
- Não alterar diretamente o mundo dentro do callback de touch.
- Processar ações por uma fila de comandos validada.
- Preferir um proprietário do estado da simulação; só adicionar tasks quando houver ganho comprovado.
- Não presumir que os dois núcleos estejam livres; respeitar tasks do sistema.
- Liberar tasks, timers, buffers, arquivos e assinaturas de eventos ao sair do jogo.

Implemente ciclo de vida equivalente a inicializar a aplicação, abrir a partida, atualizar, desenhar, pausar, retomar, salvar e voltar ao menu próprio. O menu, a engine e os serviços de rede/armazenamento devem ter transições explícitas.

## 6. Modelo do mundo e regras de construção

### 6.1 Dados

Uma célula deve representar terreno, zoneamento e ocupação sem confundi-los:

- Zonear uma célula não equivale a criar um prédio instantaneamente.
- Uma célula residencial vazia continua sendo zona residencial.
- Uma construção pode ser abandonada sem perder obrigatoriamente o zoneamento.
- Estradas, água, vegetação e construções precisam de regras de compatibilidade explícitas.

Use arrays compactos. Calcule e documente sizeof das estruturas principais. Não salve pointers, handles de arquivo ou offsets do atlas como identidade de uma construção.

Mantenha IDs estáveis para terreno, zona e tipo de prédio. O save deve continuar válido quando uma nova versão apenas trocar os sprites.

### 6.2 Construção e demolição

Cada ação deve:

1. Validar limites e terreno.
2. Validar ocupação/zoneamento.
3. Calcular custo sem modificar o estado.
4. Verificar saldo e demais requisitos.
5. Aplicar uma alteração consistente.
6. Atualizar redes/caches afetados.
7. Emitir feedback e marcar o save como alterado.

Falha não pode cobrar dinheiro nem deixar alterações parciais.

No arraste para construir estrada:

- Mostrar prévia da rota e custo total.
- Escolher um comportamento claro para obstáculos: rejeitar a rota completa ou aplicar apenas um trecho previamente indicado.
- Confirmar a operação ao soltar, sem cobrar várias vezes pela mesma célula.
- Não transformar gesto de câmera em construção acidental.

Demolição mostra custo e alvo. Construção selecionada não pode ser demolida por um toque no botão de navegação.

### 6.3 Geração de mapa

Gere um mapa reproduzível por seed:

- Área inicial utilizável.
- Água e vegetação distribuídas de forma simples.
- Sem bloqueio obrigatório logo no início.
- Sem ruído pesado calculado por pixel em runtime.
- Seed e parâmetros registrados no save.

Mantenha visualização de seed opcional na tela de nova cidade; não obrigue digitação para começar.

## 7. Simulação determinística

### 7.1 Tempo

- Renderização e simulação devem ser independentes.
- Use passo fixo de simulação; ponto de partida: 250 ms de tempo de jogo.
- As velocidades alteram a progressão de tempo, não as regras.
- Como referência de calendário, use 1 dia de jogo a cada 2 segundos reais em 1×, 30 dias por mês e 12 meses por ano; mantenha isso configurável e documentado.
- Pausar interrompe a simulação, mas mantém navegação e consulta.
- Limite trabalho acumulado por frame para evitar espiral de atraso.
- Ao retomar após uma suspensão longa, não tente processar minutos de ticks de uma vez.
- Não aplicar progresso offline por padrão.

Use PRNG com seed e estado persistidos. Comandos e tick, não taxa de renderização ou ordem arbitrária de threads, devem determinar os eventos.

### 7.2 Zonas e demanda

Implemente regras compreensíveis:

- Residências dependem de acesso, utilidades, demanda e condições locais.
- Comércio depende de população/consumidores e acesso.
- Indústria gera empregos e poluição.
- Falta de emprego, serviços ou utilidades reduz a atratividade.
- Impostos altos reduzem demanda com efeito limitado e explícito.
- Edifícios crescem em estágios com capacidade e aparência distintas.
- Insatisfação persistente pode causar abandono; não ocorrer por uma oscilação de um tick.

Use histerese ou períodos mínimos para evitar alternância contínua entre crescer e abandonar.

Mostre motivos de bloqueio: “sem energia”, “sem água”, “sem acesso”, “baixa demanda”. Não deixar o jogador adivinhar por que nada cresce.

Não simule individualmente todos os moradores ou carros. Use agregados por construção/zona.

### 7.3 Redes e cobertura

Para a primeira versão, adote uma simplificação deliberada: **energia e água são distribuídas pela rede viária**. Explique essa regra no tutorial e na interface.

- Usinas e estações de água devem estar conectadas ao componente viário atendido.
- Capacidade é contabilizada por rede conectada, não globalmente para toda a cidade.
- Bairros desconectados não recebem utilidades de uma instalação distante apenas por existir no mapa.
- Excesso de demanda deve gerar déficit observável e uma política determinística de atendimento.
- Demolir uma via deve invalidar a conectividade pertinente.
- Cobertura de polícia, saúde e bombeiros pode usar distância limitada pela rede viária.
- Parques podem influenciar uma vizinhança geométrica delimitada.
- Os efeitos precisam corresponder ao overlay mostrado.

Recalcule conectividade quando a topologia mudar; distribua o trabalho se necessário. Evite executar busca completa para cada prédio em cada tick.

### 7.4 Economia

Use inteiros ou ponto fixo para dinheiro e indicadores principais, com unidades documentadas.

Inclua:

- Caixa inicial suficiente para uma cidade funcional.
- Custos de construção e demolição.
- Despesas mensais de vias e serviços.
- Arrecadação mensal por atividade.
- Alíquotas ajustáveis em intervalo seguro.
- Resumo de receitas/despesas.
- Advertência para caixa negativo.

Caixa negativo por manutenção pode existir conforme as regras, mas novas construções pagas ficam bloqueadas. Não faça wrap de valores nem transforme dívida em saldo positivo por overflow.

Se houver derrota por insolvência, use um período de tolerância documentado. Ajuste o equilíbrio usando cidades de teste; a cidade inicial não deve quebrar antes de o jogador aprender os controles.

### 7.5 Eventos

Implemente incêndio simples apenas após o ciclo econômico básico:

- Probabilidade limitada e ligada ao risco local.
- Bombeiros reduzem risco/impacto conforme cobertura real.
- Feedback visual e mensagem.
- Sem bloquear permanentemente a interface com alertas repetidos.
- Eventos decorativos não alteram a lógica de modo dependente do FPS.

## 8. Interface, câmera e renderização

### 8.1 Interface

Interface em português do Brasil, legível na tela de 5 polegadas:

- Barra de status com saldo, população, data e alertas essenciais.
- Ferramentas de estrada, zoneamento, serviços, demolir e consultar.
- Controles de pausa/velocidade.
- Menus de salvar, carregar, nova cidade e sair.
- Painéis de informações com fechamento evidente.
- Confirmação para descartar cidade alterada.

Evite excesso de botões simultâneos. Use grupos ou painéis recolhíveis. Defina a área útil do mapa após as barras e os painéis da própria interface do jogo.

Tamanho inicial de alvo touch: aproximadamente 44–56 pixels, a validar fisicamente. Não depender apenas de cor para indicar erro, seleção ou falta de serviço.

Crie uma identidade visual própria para o jogo e seu menu, com fontes legíveis e boa distinção entre zonas. A UI mínima de boot/recuperação pode usar uma fonte pequena embutida; fontes e recursos visuais adicionais seguem o pacote no SD.

### 8.2 Coordenadas

Implemente conversão coerente entre:

- Índice de célula.
- Coordenada isométrica do mundo.
- Tela após câmera, zoom e offset da UI.

A projeção pode seguir:

- x = (coluna − linha) × metade da largura do losango.
- y = (coluna + linha) × metade da altura do losango.

A seleção deve testar o losango correto nas bordas. Documente os anchors dos sprites. Prédios altos não podem deslocar a posição da célula-base.

Teste viewport parcialmente fora do mapa, cantos, zoom e posição negativa da câmera. Eventos capturados pela UI não devem alcançar o terreno.

### 8.3 Desenho

- Comece com tiles de base de 64 × 32 pixels; ajuste apenas com justificativa.
- Renderize somente células e sprites potencialmente visíveis.
- Considere a altura dos prédios no culling.
- Ordene a sobreposição de forma determinística, respeitando profundidade isométrica.
- Não redesenhe área oculta por painéis desnecessariamente.
- Não leia arquivo ou decodifique PNG durante o desenho de cada tile.
- Não faça operações de rede no caminho de renderização.
- Gerencie transparência, clipping e overlays sem alocações repetidas.

Escolha um backend gráfico compatível com a base técnica do projeto e valide seu funcionamento. PPA do ESP32-P4 pode ser aproveitado para operações compatíveis de pixels, se disponível na versão usada e se a medição demonstrar ganho; não o trate como GPU 3D nem invente uma API de aceleração.

Não aloque um framebuffer completo adicional sem contabilizar o que o framework já reserva.

## 9. Criação dos sprites pelo agente — requisito obrigatório

Você deve entregar a arte necessária, e não apenas escrever “adicionar sprites depois”.

### 9.1 Processo de autoria

Crie um gerador de sprites executável no computador de desenvolvimento, com saída reproduzível:

- Uma opção adequada é Python + Pillow, ou ferramenta equivalente já disponível.
- Inclua scripts, parâmetros, seed e instruções de reprodução.
- Gere efetivamente os arquivos e execute o gerador.
- Não se limite a ícones genéricos: desenhe volumes, telhados, fachadas, janelas, pavimento, vegetação e detalhes coerentes.
- Use paleta e iluminação consistentes, com contraste para a tela pequena.
- Preserve transparência e anchor na base de cada sprite.
- Não usar imagens ou marcas de SimCity/EA/Maxis.
- Não exigir acesso a serviço pago para gerar ou regenerar o pacote.
- Se houver ferramenta de geração de imagens disponível, ela pode auxiliar na autoria, mas o pipeline final precisa produzir assets recortados, dimensionados e utilizáveis.

Organize a arte por categorias e IDs estáveis. Separe a fonte de autoria do formato runtime. PNG pode ser o formato fonte; o formato enviado ao SD deve atender ao renderer real.

### 9.2 Conjunto mínimo de arte

| Categoria | Conteúdo mínimo |
|---|---|
| Solo | Terra/grama com variações discretas |
| Água | Interior e bordas suficientes para transições coerentes |
| Vegetação | Pelo menos 3 árvores/variações |
| Estradas | As 16 combinações de vizinhança cardinal; documentar máscara e direções |
| Zoneamento | Indicação de zonas R/C/I vazias |
| Residências | 3 estágios, com pelo menos 2 variantes por estágio |
| Comércio | 3 estágios, com pelo menos 2 variantes por estágio |
| Indústria | Pelo menos 2 estágios e 2 variantes por estágio |
| Utilidades | Usina e estação de água |
| Serviços | Bombeiros, polícia, saúde e parque |
| Estados | Abandono, incêndio e seleção/construção inválida |
| Interface específica | Ícones essenciais das ferramentas e indicadores |

Na primeira versão, construções de 1 × 1 célula são suficientes. Não simule footprint maior apenas com um desenho que invada células vizinhas sem regras de ocupação.

Sprites altos podem usar imagens maiores que 64 × 32. Registre dimensões, anchor e limites para clipping. Verifique alinhamento, sombras e sobreposição em uma cena isométrica real.

Gere também:

- Uma contact sheet legível com IDs.
- Uma cena de demonstração contendo todos os tipos de terreno e construções.
- Prévia de bairros residencial, comercial e industrial.
- Relatório com quantidade de sprites, tamanho do pacote e estimativa da memória decodificada.

Inspecione as imagens geradas e corrija assets desalinhados, ilegíveis ou com transparência incorreta. A existência do arquivo não comprova qualidade visual.

## 10. Assets no microSD, fora do firmware

### 10.1 Regras de armazenamento

Todos os recursos gráficos específicos do jogo ficam no microSD:

- Sprites e atlas.
- Overlays texturizados e ícones próprios.
- Efeitos sonoros/trilhas adicionados ao jogo, quando existirem.

O firmware pode conter código de renderização, IDs, schemas e a UI mínima de mensagem de erro. Não deve conter uma segunda cópia dos sprites em arrays C/C++, PROGMEM, base64, arquivos embutidos ou mecanismos de embed do linker.

Se o pacote não estiver instalado:

- Mostrar uma mensagem clara no menu.
- Oferecer instrução/ação de instalação.
- Não travar, reiniciar ou abrir um mapa com assets faltando.
- Não reconstruir secretamente uma biblioteca inteira de sprites no firmware como fallback.

O pipeline de build de firmware deve permanecer independente do pacote visual.

### 10.2 Formato runtime e cache

Escolha o formato após examinar as bibliotecas reais:

1. Reutilize um formato binário/atlas suportado pelo renderer, quando adequado.
2. Caso precise de formato próprio, mantenha-o pequeno, documentado e versionado.
3. Considere RGB565 com transparência/máscara ou outra representação compatível e mensurável.
4. Não carregue todos os PNGs decodificados na RAM ao iniciar.

O catálogo deve fornecer, no mínimo:

- ID estável.
- Arquivo/atlas e região ou offset.
- Largura e altura.
- Anchor.
- Formato de pixels.
- Informações de transparência.
- Comprimento e validação dos dados.

Valide todos os tamanhos, índices e offsets antes de ler. Trate overflow de multiplicação e arquivos truncados.

Implemente cache limitado:

- Limite explícito em bytes.
- Política de substituição documentada.
- Contagem de hits/misses e memória ocupada.
- Recursos comuns fixados apenas quando houver vantagem.
- Buffer reutilizável para leitura/conversão.
- Prefetch opcional para o viewport, sem requisito de carregar o mundo inteiro.

Não abrir/fechar arquivos a cada sprite desenhado. Se usar atlas grande, demonstre que o acesso por região não exige decodificá-lo integralmente para cada leitura.

Ative uma versão de pacote por vez. Um frame não pode misturar arquivos de duas versões.

### 10.3 Manifesto e compatibilidade

Crie um manifesto com schema versionado e campos equivalentes a:

- package_id e package_version.
- schema_version e runtime_format_version.
- intervalo de versões de engine compatíveis.
- dimensões nominais dos tiles.
- inventário de arquivos, tamanhos e SHA-256.
- IDs de sprites e respectivos metadados, diretamente ou por catálogo.
- total de bytes e contagens.
- versão do gerador de assets.

Não use só a comparação lexicográfica de strings de versão. Defina a regra.

Versão da arte, versão do save e versão da engine são conceitos separados:

- Trocar uma fachada não deve invalidar a cidade salva.
- Sprite ausente deve ter tratamento explícito sem corromper o save.
- Adicionar comportamento novo de construção pode exigir atualização de engine.
- Atualizar imagens não deve executar código recebido pelo SD.

Hash comprova integridade comparada a um manifesto confiável; não comprova sozinho a autoria. Documente a autenticação do transporte e, se houver suporte de assinatura no projeto, reutilize-o sem criar criptografia própria.

## 11. Envio e atualização OTA do pacote no microSD

Implemente **o caminho completo**, incluindo ferramenta do computador e recepção no Tab5. Não entregue apenas uma função hipotética de download.

### 11.1 Experiência do usuário

O usuário deve conseguir:

1. Gerar o pacote de sprites no computador.
2. Abrir, no menu próprio do jogo, o modo de receber atualização de assets.
3. Enviar o pacote pela rede.
4. Acompanhar progresso, validação e resultado.
5. Abrir o jogo com a nova arte sem recompilar ou atualizar o firmware.

Implemente o servidor de recepção, sua tela de controle e autenticação como partes da aplicação dedicada. Use as bibliotecas de rede e HTTP do SDK escolhido e mantenha o fluxo pequeno e documentado. O modo de envio deve ter ativação e encerramento claros e ser apropriado ao uso na rede local.

Não abra um serviço sem autenticação como default permanente. Não coloque senhas/tokens reais em código, manifestos ou logs.

### 11.2 Transporte

Escolha entre um pacote empacotado e envio de manifesto + arquivos conforme as bibliotecas existentes. Não imponha ZIP se isso exigir biblioteca pesada ou descompressão integral em RAM.

Requisitos:

- Escrita em streaming no SD.
- Buffers pequenos e contabilizados.
- Tamanho máximo e quantidade de arquivos limitados.
- Verificação de espaço para nova versão e retenção da anterior.
- Timeouts e cancelamento.
- Erros diferenciados de rede, espaço, filesystem, integridade e incompatibilidade.
- Nomes/caminhos normalizados dentro da raiz do jogo.
- Rejeição de caminhos absolutos, traversal, links e entradas duplicadas/conflitantes.
- Se houver compressão, impor limite de tamanho descomprimido.
- Não confiar somente na extensão do arquivo ou no Content-Type.

Comece com um limite de pacote de 32 MiB, ajustável por configuração fundamentada. Esse limite não significa carregar 32 MiB em RAM.

Entregue uma ferramenta de envio no computador — script Python, comando ou UI compatível com o fluxo do projeto — e um exemplo de uso com endereço e token fictícios. Ela deve informar claramente sucesso, erro e versão instalada.

A retomada de upload pode ser uma melhoria posterior. A interrupção, porém, deve ser segura desde a primeira versão.

### 11.3 Instalação transacional e recuperação

Use slots A/B de assets ou diretórios de versão com área de staging. Adapte nomes ao filesystem real. Exemplo conceitual:

- raiz do jogo/slots/A.
- raiz do jogo/slots/B.
- registros de ativação com sequência e integridade.
- saves em diretório independente.

Sequência obrigatória:

1. Confirmar SD montado, compatibilidade declarada e espaço.
2. Preparar o destino inativo.
3. Receber todos os arquivos sem modificar a versão ativa.
4. Verificar tamanhos, hashes, formato e catálogo completo.
5. Validar presença dos sprites obrigatórios.
6. Concluir a gravação usando flush/sync disponíveis.
7. Publicar um registro de versão pronta.
8. Ativar em um ponto seguro.
9. Abrir e validar o pacote ativo.
10. Manter a versão anterior para recuperação.

Não prometa que um rename isolado em FAT garante consistência após perda de energia. Projete registros redundantes/verificáveis e uma rotina de startup que encontre a versão completa válida mais recente ou retorne à anterior.

Se a atualização chegar durante o jogo:

- Preferencialmente bloquear a instalação com mensagem ou receber em staging e adiar a ativação.
- Nunca substituir arquivos usados pelo renderer.
- Não liberar o catálogo antigo enquanto houver referências em uso.
- Não permitir atualização, autosave e renderização disputarem o SD sem uma política de acesso.

Na remoção/reinserção do SD, não reutilizar handles antigos. O jogo deve entrar em estado seguro, preservar o estado que ainda estiver na RAM e permitir recuperação quando possível.

Um pacote inválido não pode impedir a inicialização da UI mínima, o acesso às configurações de rede ou a abertura da tela de recuperação/instalação de assets.

### 11.4 Testes de falha do instalador

Cobrir pelo menos:

- Transferência interrompida.
- Hash divergente.
- Manifesto incompleto.
- Versão incompatível.
- Arquivo de sprite truncado.
- Falta de espaço.
- SD removido durante recepção.
- Falha entre “pronto” e ativação.
- Registro de ativação corrompido.
- Atualização enquanto há cidade aberta.
- Reinicialização em cada fase persistente relevante.

Simule as falhas no host quando o hardware não estiver disponível. Não diga que um teste de host comprova resistência física do cartão à perda de energia.

## 12. Salvamento e continuidade da cidade

Implemente formato de save próprio, explícito e versionado, sem despejo cru de structs da memória.

Persistir:

- Versão de formato e dimensões.
- Nome da cidade, seed e estado do PRNG.
- Terrenos, zonas, ocupação e construções.
- Saldo, impostos, calendário e progresso econômico.
- Estado dos eventos que precisem continuar.
- IDs estáveis dos tipos.
- Preferências de câmera, quando útil.

Não é necessário persistir caches de conectividade: podem ser reconstruídos.

Requisitos:

- Comprimentos e limites validados antes de alocar.
- Endianness e unidades definidas.
- Checksum de corrupção.
- Arquivo temporário e recuperação por slot/backup, conforme filesystem.
- Nunca destruir o último save válido antes de confirmar o novo.
- Autosave controlado, sem salvar a cada tick.
- Mensagem para SD ausente/cheio/corrompido.
- Save antigo incompatível deve gerar diagnóstico; não tentar interpretar bytes arbitrariamente.
- Falha ao carregar não apaga o arquivo.
- Atualização de sprites não sobrescreve saves.

Ponto inicial para autosave: a cada 2 minutos de jogo ativo, se houver alterações, e ao sair quando solicitado. Ajuste à latência real do SD.

Gere snapshot consistente para salvar; não manter o mundo parcialmente atualizado durante a serialização. Evite cópias integrais sucessivas se não couberem no orçamento.

## 13. Metas de memória, desempenho e armazenamento

Meça a base mínima da plataforma — drivers, display e UI inicial — antes de acrescentar simulação e assets, e repita as medições após cada marco.

Metas iniciais de engenharia:

- Mapa e estruturas de simulação: buscar até 512 KiB no cenário inicial.
- Cache de sprites: alvo de até 4 MiB, com limite configurável.
- Estado da partida, cache e buffers próprios da engine: buscar até 8 MiB de PSRAM, além dos buffers de plataforma previamente medidos.
- RAM interna adicional da engine em relação à base mínima: buscar até 96 KiB, explicitando stacks e exceções necessárias.
- Medir também o pico total da aplicação, incluindo drivers, UI, rede e instalação OTA; os limites dos subsistemas não substituem esse total.
- Renderização: alvo de 20–30 FPS durante navegação da cidade inicial.
- Resposta de entrada: alvo inferior a 100 ms nas operações comuns.
- Evitar pausas perceptíveis durante simulação/autosave.
- Pacote de assets: alvo até 32 MiB na versão inicial, sem tratá-lo como consumo de RAM.

Esses números são alvos, não garantias. Se o projeto já tiver restrições mais severas, adapte e registre. Não aumente limites para esconder vazamento ou alocação indevida.

Referência para orçamento: um framebuffer RGB565 de 1280 × 720 ocupa 1.843.200 bytes; dois ocupam 3.686.400 bytes, antes de alinhamentos e metadados. Não confundir MB com MiB ao relatar.

Instrumentação de desenvolvimento:

- Heap interno livre, mínimo histórico e maior bloco.
- PSRAM livre, mínimo e maior bloco.
- Uso das stacks relevantes.
- Tempo de tick, desenho, flush, leitura SD e autosave.
- FPS e cache misses.
- Número de arquivos abertos e ocupação do cache.
- Tamanho de firmware e margem nas partições definidas para o projeto.

Se precisar reduzir custo, priorize:

1. Desenhar somente área visível.
2. Limitar animações e sprites decorativos.
3. Melhorar cache e invalidação.
4. Distribuir cálculos e reduzir frequência de simulações secundárias.
5. Reduzir detalhe visual antes de remover as regras essenciais.
6. Avaliar renderização interna menor apenas se a escala e a UI continuarem legíveis.

Não desabilite watchdog para esconder bloqueios. Não use delays longos no loop da UI.

## 14. Áudio e ciclo de vida da aplicação

Implemente controle próprio de volume e mute nas configurações. Adicione efeitos curtos de construção, erro e evento, respeitando essas preferências. Todos os arquivos de áudio do jogo devem seguir a distribuição por SD.

Som deve ser opcional. A ausência de um efeito sonoro não pode impedir a lógica.

Ao encerrar uma partida e voltar ao menu:

- Parar os efeitos da partida e liberar seu estado transitório.
- Concluir ou cancelar operações locais com segurança.
- Fechar handles do save e liberar recursos da cidade que não precisam permanecer no menu.
- Desregistrar callbacks/timers exclusivos da partida.
- Devolver foco e estado gráfico ao menu próprio.
- Manter apenas os serviços de plataforma deliberadamente compartilhados.

Inicializar e encerrar partidas repetidamente não pode produzir perda progressiva de memória. A aplicação deve definir separadamente pausa de partida, retorno ao menu e encerramento/reinício do dispositivo.

## 15. Configuração e balanceamento

Centralize custos, capacidades, consumo, produção e cobertura em definições documentadas. Não espalhe números mágicos pelos módulos.

Se essas definições ficarem no SD:

- Validar schema, limites e versão.
- Definir fallback seguro para arquivos inválidos.
- Separar atualização visual de mudança nas regras de simulação.
- Documentar como uma mudança de balanceamento afeta saves existentes.

Não use o pacote de sprites como forma implícita de executar scripts ou alterar regras sem validação.

Produza pelo menos três cidades de teste:

- Cidade pequena equilibrada.
- Cidade média com bairros de demandas diferentes.
- Cidade sob déficit de água/energia e pressão financeira.

Use-as para avaliar se as consequências são compreensíveis e se existe caminho de recuperação.

## 16. Testes e evidências de qualidade

Teste a lógica desacoplada no host usando o framework existente ou uma solução pequena.

### 16.1 Testes importantes

- Projeção e seleção isométrica nos centros, bordas e cantos.
- Câmera e zoom sem selecionar célula errada.
- Construção/demolição com custo correto e rollback em falha.
- Máscaras de estrada coerentes com vizinhos.
- Separação de redes ao remover uma via.
- Consumo/produção por componente conectado.
- Crescimento bloqueado pelo motivo correto.
- Taxas e manutenção cobradas uma única vez no período.
- Pausa e velocidades sem duplicar cálculos econômicos.
- Determinismo: mesma seed + mesmos comandos nos mesmos ticks → mesmo estado.
- Save/load preserva estado; PRNG continua corretamente.
- Save truncado ou adulterado é rejeitado sem apagar backup.
- Asset ausente ou inválido gera tratamento controlado.
- Cache não ultrapassa seu limite.
- Instalador mantém o pacote anterior durante falhas.
- IDs de sprites continuam compatíveis após atualização puramente visual.

Não crie centenas de testes triviais que apenas repetem getters. Priorize regras e falhas que podem corromper a cidade ou travar a aplicação.

### 16.2 Validação integrada

Quando houver dispositivo:

- Jogar pelo menos 20 minutos em uma cidade com crescimento.
- Alternar pausa e velocidades.
- Navegar, construir e consultar nas bordas da tela.
- Abrir e fechar o jogo repetidamente.
- Testar nova cidade, salvar, reiniciar e carregar.
- Instalar uma segunda versão de assets visualmente distinguível.
- Confirmar que o firmware não precisou ser recompilado e que o save foi preservado.
- Confirmar recuperação de instalação interrompida.
- Verificar boot frio, menu próprio, configurações, recuperação sem assets e funcionamento offline.

Quando não houver dispositivo:

- Compilar para o alvo real.
- Executar testes de host e validador de assets.
- Gerar screenshots/cenas por ferramenta de preview, quando possível.
- Entregar checklist objetivo para teste físico.
- Não chamar screenshot de preview de captura do Tab5.

## 17. Marcos de implementação

### Marco A — diagnóstico e orçamento
Selecionar e documentar SDK/BSP/bibliotecas, criar o esqueleto de firmware quando necessário, inicializar display/touch/SD e entregar comandos reais de build/instalação e baseline de memória.

### Marco B — assets e caminho vertical
Gerar primeiro pacote real no computador; instalar no SD por cópia documentada inicialmente; iniciar o firmware dedicado no Tab5 e abrir mapa isométrico com sprites, câmera e seleção, retornando ao menu próprio. Esse primeiro fluxo já deve carregar a arte do SD; o envio OTA completo entra no Marco E.

### Marco C — ciclo de cidade
Estradas, zoneamento, utilidades, crescimento, população, empregos, impostos e manutenção funcionando em conjunto.

### Marco D — serviços e persistência
Coberturas, indicadores, eventos simples, salvamento, recuperação e autosave.

### Marco E — OTA de assets
Ferramenta de envio, recepção streaming, verificação, ativação e rollback. O fluxo deve funcionar sem rebuild de firmware para trocar imagens.

### Marco F — desempenho e acabamento
Profiling, correção de vazamentos, arte revisada, tutorial curto, regressão e documentação.

Após cada marco: compilar, executar as verificações relevantes, registrar resultados e continuar para o próximo dentro do escopo autorizado. Reavaliar decisões com base em medições.

## 18. Critérios de aceite

O trabalho está pronto para revisão quando:

1. O firmware dedicado inicializa diretamente a aplicação, apresenta seu menu próprio e permite iniciar/encerrar partidas corretamente.
2. Há uma cidade 64 × 64 com construção por toque e câmera utilizável.
3. O jogador consegue estabelecer infraestrutura, crescer e obter receita.
4. Falta de acesso, água, energia e serviços tem efeitos e explicação.
5. Pausa, velocidades, orçamento e calendário funcionam de forma coerente.
6. Salvar/carregar recupera a cidade com integridade.
7. Os sprites foram efetivamente criados por você e podem ser regenerados.
8. Nenhum sprite específico do jogo está embutido no firmware.
9. O carregamento usa microSD e cache limitado.
10. Uma nova arte pode ser enviada pela rede/OTA e ativada independentemente do firmware.
11. Atualização inválida/interrompida mantém recuperação pela versão anterior.
12. Atualização de imagens preserva saves compatíveis.
13. O build para o Tab5 e os testes disponíveis passam.
14. Consumo medido de memória e desempenho está documentado, ou claramente marcado como não medido em hardware.
15. O projeto pode ser compilado e instalado de forma independente; boot, configurações, recuperação de assets e funcionamento offline foram verificados no ambiente disponível.

## 19. Entregáveis

Entregue:

- Projeto independente de firmware, modular e compilável.
- Configuração de build, versões fixadas, tabela de partições e instruções de primeira gravação.
- Scripts de geração dos sprites.
- Fontes de arte e pacote runtime gerado.
- Contact sheets e cenas de preview.
- Manifesto e catálogo versionados.
- Validador do pacote.
- Ferramenta de envio OTA e instruções de instalação.
- Testes relevantes.
- Savegame documentado.
- Documentação de arquitetura, controles, balanceamento e limitações.
- Instruções para reproduzir o build e regenerar assets.
- Relatório final de arquivos alterados, validações executadas e pendências físicas.

Organize a documentação e os scripts na estrutura própria do projeto. Inclua instruções suficientes para alguém clonar o repositório, instalar dependências, compilar, gravar o firmware e instalar o pacote visual sem qualquer aplicativo adicional do autor. Não crie apenas documentos no lugar da implementação.

## 20. Instrução de início

Comece agora pela inspeção do diretório/repositório de destino. Se estiver vazio, crie a base técnica do projeto independente; se houver um exemplo técnico da placa, valide o que será aproveitado. Selecione e fixe as dependências, inicialize o hardware e crie o menu próprio.

Em seguida, implemente o primeiro caminho completo — sprite gerado no computador, arquivo no SD, firmware dedicado iniciando no Tab5, mapa isométrico e retorno ao menu do jogo — e prossiga pelos marcos, incluindo o serviço próprio de OTA de assets, até concluir o escopo.

---

## Notas de referência para quem utilizar este prompt

O autor deste prompt não inspecionou um repositório de implementação nem mediu o jogo no Tab5. O agente deve criar ou validar a base técnica do projeto e obter paths, APIs, memória livre e desempenho no ambiente real. As metas numéricas são propostas para orientar medição e ajustes.

Referências oficiais consultadas em 07/11/2026:

- M5Stack Tab5: https://docs.m5stack.com/en/core/Tab5
- ESP32-P4 PPA: https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ppa.html
- ESP32-P4 PSRAM: https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-guides/external-ram.html

As URLs “stable” podem mudar de versão. Para implementação, o agente deve usar a documentação correspondente ao SDK efetivamente fixado no repositório.
