# Atualização 0.3.0 — resposta ao toque e serviços

Firmware 0.3.0 / engine 1 / arte 2 / save 2. Os saves existentes continuam válidos.

## Atualizar pelo Web-GUI do launcher

Para o cartão que recebeu a primeira distribuição (`slot-a.mcap` e `slot-a.ready`, sequência 1):

1. Encerre o jogo para salvar a cidade e entre no Web-GUI do launcher.
2. Envie `dist/launcher/microcity/microcity.bin` para `/microcity/microcity.bin`.
3. Envie `dist/launcher/microcity/slot-b.mcap` para `/microcity/slot-b.mcap`.
4. **Envie `dist/launcher/microcity/slot-b.ready` por último**, depois que o arquivo MCAP terminar de subir. O registro tem sequência 2 e ativa a arte nova no próximo início do jogo.
5. Reinstale/inicie o novo `microcity.bin` pela ação do launcher. Somente substituir o arquivo no cartão não muda a aplicação já instalada.

Preserve `slot-a.*` e `saves/`. A pasta pronta conserva a arte anterior no slot A e a nova no slot B. Não substitua os saves pelos fixtures de teste. Se você já atualizou assets por outro caminho e possui registros com sequência maior, use **Gerenciar assets → Abrir recepção Wi-Fi** e a ferramenta de envio: o receptor escolhe o slot inativo e a próxima sequência corretos.

Alternativas: `python3 tools/install_sd.py /Volumes/SEU_CARTAO` com o cartão fora do Tab5, ou `python3 tools/upload_assets.py http://192.168.4.1/assets --token TOKEN_DA_TELA` pelo receptor do próprio MicroCity. O pacote de entrada continua sendo `assets/package/microcity.mcap`.

## Arte dos serviços

Os IDs 100–105 foram redesenhados, mantendo dimensões 64×96 e anchor (32,79):

- Usina com salão de turbinas, transformadores, barramento e chaminé.
- Estação de água com reservatório elevado, bomba/motor, tubulação, flange e válvula.
- Bombeiros com garagem, torre de mangueiras e caminhão com escada.
- Delegacia com distintivo, antena e viatura.
- Hospital com cruzes, entrada de emergência e ambulância.
- Parque com fonte, caminhos, bancos, árvore e flores.

Prévia de computador: `assets/services-preview.png`. Fontes procedurais em `tools/service_art.py`; regenerar com `.venv/bin/python tools/generate_assets.py --version 2`. Não são imagens incorporadas ao firmware. Um teste compara hashes com a arte v1 e exige que somente esses seis sprites tenham mudado; os demais IDs, áudio, dimensões e anchors são preservados.

## Resposta da interface

A investigação encontrou fatores que podem explicar a necessidade de segurar/repetir toques: o mapa inteiro era desenhado no timer LVGL, havia divisões em ponto flutuante por pixel e o mapa era redesenhado em ticks sem mudança diária. Telas/botões também podiam participar da rolagem, que suprime eventos de clique quando o dedo se desloca.

Correções em código:

- Timer da aplicação de 50 para 10 ms; limite de sono da task LVGL em 10 ms.
- Desenho em etapas: limpeza em blocos de 16 linhas, uma leitura fria de asset por etapa e pintura de sprites com alvo de 3 ms de CPU por passagem. A task devolve o controle entre etapas, permitindo processar entrada.
- Dois buffers próprios de mapa, em PSRAM. A UI continua mostrando o frame anterior enquanto o próximo é preparado; frames parciais não são publicados. A câmera usada para selecionar células é a do frame realmente apresentado, e não a de um frame ainda em preparação.
- Cache deriva cores RGB565 e intervalos opacos por linha uma vez. Zoom usa tabelas inteiras; escala 1× copia trechos opacos com memcpy. Não há divisão por pixel nem busca linear de catálogo/cache no blit.
- HUD/feedback separados do desenho do mapa, atualizados somente quando o texto muda. Ticks sem mudança de dia não solicitam redesenho. Paletas suspendem o desenho de fundo, mantendo controles ativos.
- Rolagem desabilitada na raiz e nos botões, transições removidas e área de toque dos botões ampliada em 3 px. A tolerância de toque do mapa usa distância euclidiana: 18 px para construir/consultar e 6 px para iniciar câmera.
- Volume e impostos atualizam rótulos sem recriar a tela. Impostos recalculam indicadores, sem refazer conectividade. Volume/mute são persistidos após 750 ms sem alteração, agrupando cliques rápidos. Save sem alterações já persistidas evita uma nova escrita.

Isso reduz trabalho e pontos de bloqueio demonstráveis no código; **a latência física após a correção ainda precisa ser medida no Tab5**. A meta de 3 ms é cooperativa: uma operação isolada, leitura do SD ou flush do display pode excedê-la. Saves, validação inicial do pacote e início de rede ainda usam chamadas síncronas e não foram apresentados como livres de pausas.

## Memória e evidências

- Dois canvas RGB565 1280×526: 2.693.120 bytes, em PSRAM, separados dos buffers do BSP.
- Cache completo da arte atual + áudio e derivados: 2.533.824 bytes; limite permanece 4 MiB e inclui os derivados. Cores e intervalos compartilham uma alocação para evitar dezenas de pequenas alocações internas.
- Só esses buffers/cache somam aproximadamente 4,985 MiB; City, App, stacks e plataforma devem ser contabilizados adicionalmente. O alvo da engine continua 8 MiB de PSRAM, sujeito à medição total real.
- Logs incluem tempo de frame completo, maior etapa de desenho, espera na fila de comandos, tick/save e heap. Espera na fila mede enfileiramento→execução, não dedo→imagem.
- Testes C++ com ASan/UBSan comparam cada pixel do blitter novo com o anterior em cinco zooms, clipping negativo, transparência com furos e dimensões máximas. Cache real é verificado contra os pixels do pacote e seu limite de memória.
- Regressões de cidade, economia, teclado, save/load, instalador, fonte e troca de arte continuam disponíveis em `tools/test.sh`.

Próximo teste físico: toques curtos únicos em menus, paletas e mapa; mover a câmera durante desenho; clicar repetidamente volume/impostos; verificar seleção após zoom e confirmar arte v2. Nenhum flash, monitor ou acesso serial foi executado pelo agente.
