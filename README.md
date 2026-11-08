# MicroCity do Paludo

Jogo independente de construção de cidades para M5Stack Tab5, escrito em C++17 com ESP-IDF. A especificação original está em [prompt-microcity-tab5.md](prompt-microcity-tab5.md).

**Estado:** versão 0.11.0 aceita fisicamente no Tab5 em 07/11/2026. Jogo, touch, teclado, áudio, persistência, três slots, funcionamento offline, desastres, ciclo mensal e rejeição de saves corrompidos foram testados pelo usuário. A distribuição adotada é pelo Web-GUI do launcher.

## Atualização 0.11.0: desastres, ciclo mensal e interface

Incêndios, tempestades e tremores atingem somente construções, utilidades e serviços; ruas, avenidas e pontes nunca são alvos. Bombeiros extinguem incêndios cobertos. O mapa percorre um ciclo visual completo entre noite, crepúsculo e dia a cada mês do calendário. Finanças, Ajustes, Guia e Teclas foram reorganizados em cartões maiores e colunas legíveis. Saves novos usam formato v5 e os anteriores continuam carregáveis. A arte v10 acrescenta tempestade e tremor.

## Atualização 0.10.3: grade de ocupação

Construções multicélula recebem um contorno isométrico tracejado por célula, desenhado como overlay transparente após o campus. A delimitação mostra exatamente o footprint 2×2, 3×3 ou 3×2 sem cobrir prédio ou vias. Requer o BIN 0.10.3 e a arte v9.

## Correção 0.10.2: composição multicélula

As células reservadas de escolas, universidade, parque municipal e porto não repintam mais terreno depois do sprite âncora. Isso preserva todas as alas presentes na arte v8; no modo Obras, as células continuam aparecendo separadamente para indicar o footprint. Para quem já instalou a arte v8, basta atualizar `microcity.bin`.

## Correção visual: arte 8

Ruas e avenidas estendem a faixa alguns pixels além da borda de cada tile, eliminando a emenda serrilhada repetida. Equipamentos multicélula deixaram de ampliar um prédio 1×1: agora são campi em resolução nativa, com alas distribuídas pelo lote. Para quem já instalou o firmware 0.10.1, envie somente `slot-b.mcap` e `slot-b.ready`, nesta ordem.

## Correção 0.10.1: desempenho e arte 7

Veículos nas ruas foram completamente desativados após o teste físico indicar perda severa de jogabilidade. Equipamentos 2×2 usam prédio em escala 1,5× e universidade 3×3 em 2×, sobre um lote desenhado célula por célula; assim deixam de parecer 1×1 sem ampliar o piso além da área reservada. Envie o novo `microcity.bin`, depois `slot-a.mcap` e `slot-a.ready` por último. Saves não mudam.

## Atualização 0.10.0: mapa 128×128, pontes, portos e transporte

As cidades novas usam mapa plano 128×128. Saves 64×64 continuam legíveis e são migrados para o formato novo ao salvar. Vias sobre água viram pontes automaticamente; portos 3×2 exigem margem, geram empregos e receita quando atendidos. Há até 12 veículos e 4 navios visíveis, sem persistência nem custo de simulação. Os indicadores de demanda R/C/I e os marcadores do modo Obras agora variam por categoria e estágio.

Prédios multicélula usam sprites do tamanho do footprint. Água, energia e acesso alcançam até duas células da via, incluindo esquinas diagonais, e o estado é compartilhado por toda a construção. O renderer percorre somente as diagonais isométricas visíveis; a divisão concorrente do framebuffer entre núcleos não foi habilitada sem medição física e sincronização segura.

Para atualizar pelo launcher, envie `dist/launcher/microcity/microcity.bin`, depois `slot-a.mcap` e por último `slot-a.ready`. Preserve `slot-b.*`, `saves/` e `settings/`.

## Atualização visual: arte 4

Escola primária, escola secundária, universidade e parque municipal agora têm sprites isométricos próprios e reconhecíveis, gerados por `tools/civic_art.py`. A prévia está em [assets/civic-preview.png](assets/civic-preview.png). É uma troca puramente visual: firmware, engine e saves permanecem compatíveis.

## Atualização 0.9.0: modo Obras, educação e teclado Tab5

O modo Obras (`B`) substitui prédios altos por marcadores baixos para revelar vias e células. Zonas agora são definidas por retângulo de toque/arrasto. Foram adicionadas escola primária 2×2, escola secundária 2×2, universidade 3×3, parque municipal 2×2 e avenidas. O teclado físico usa o controlador I²C indicado pelo exemplo oficial, e os atalhos principais existem na matriz do próprio Tab5. Saves v2 são migrados ao gravar para v3 sem alterar o tamanho do arquivo.

Se os assets forem removidos, Nova cidade/Carregar abre a tela de recuperação. Pelo launcher, envie `slot-a.mcap` e depois `slot-a.ready` para `/microcity`; o `.ready` deve ser o último arquivo.

## Correção 0.8.0: mapa azul e mais cinco etapas

O primeiro frame não pode mais ser cancelado indefinidamente pelos ticks diários; ele é publicado e a atualização posterior fica enfileirada. A interface mostra progresso de carregamento, painéis congelam o relógio sem alterar a velocidade, Home centraliza câmera/zoom e o navegador de cidades mostra felicidade e déficits de utilidades.

## Atualização 0.7.0: três cidades e gestão

O microSD agora comporta três slots independentes, com navegador que mostra população, caixa, ano e seed. Criar sobre um slot ocupado exige confirmação. A partida ganhou painel de Finanças/infraestrutura e avisos quando locais perdem energia ou água. O save legado continua carregável como slot 1 e é migrado na próxima gravação.

## Atualização 0.6.0: persistência completa no microSD

Preferências e sessão agora usam dois arquivos alternados com versão, CRC, verificação e fallback em `/microcity/settings/`. Volume, mute, tutorial, câmera, zoom, ferramenta, overlay e velocidade voltam após reiniciar. Nova cidade ganhou uma tela de confirmação com seed decimal/hexadecimal e opção de sortear outra seed.

## Atualização 0.5.0: frames atuais e guia inicial

Renderizações incompletas são descartadas quando câmera, zoom, overlay, simulação ou construção tornam o frame obsoleto. Uma nova cidade mostra, após o primeiro mapa pronto, um guia de três páginas sobre infraestrutura, crescimento e serviços. O botão Guia permite reabri-lo; a conclusão é lembrada no NVS.

## Atualização 0.4.0: interação e diagnóstico

Toques simples no mapa agora solicitam somente o frame resultante, em vez de redesenhar ao pressionar e novamente ao soltar. A pintura também limita cada passagem a 24 sprites para devolver o controle à entrada com maior frequência. A barra mostra consumo/capacidade de energia e água; Consultar descreve ocupação, zona, nível, bloqueio, terreno, poluição e coberturas.

Implementação inicial: mapa isométrico, câmera, estradas, zonas R/C/I com crescimento, redes locais de água/energia pelas vias, serviços, economia, calendário, incêndios simples, salvamento A/B e instalação separada de arte pelo SD ou Wi-Fi temporário. Jogo, saves, touch, áudio e teclado físico foram validados pelo usuário no Tab5; desempenho das novas cidades 128×128 ainda requer medição física.

## Atualização 0.3.0: serviços e resposta ao toque

Os serviços agora têm desenhos próprios: bomba/reservatório, usina, bombeiros com caminhão, delegacia com viatura, hospital com ambulância e parque com fonte. Veja a [prévia](assets/services-preview.png).

O desenho foi otimizado e dividido em etapas curtas, separado da atualização dos controles. Rolagem acidental foi removida; volume/impostos atualizam os rótulos sem reconstruir a tela. A melhora física da latência ainda precisa ser confirmada no Tab5.

Pelo Web-GUI, envie para `/microcity/` o **novo `microcity.bin` e `slot-b.mcap`**, seguidos por **`slot-b.ready` por último**. Os três estão em `dist/launcher/microcity/`. Reinstale/inicie o BIN pelo launcher. Preserve `slot-a.*` e `saves/`. O registro B pronto tem sequência 2 e foi preparado para a instalação inicial A com sequência 1; se já houver atualização mais recente, use o receptor de assets do jogo ou o instalador de SD para escolher a sequência correta.

Veja [instruções e evidências da revisão](docs/ATUALIZACAO-0.3.0.md) e [todos os atalhos](docs/CONTROLES.md). O suporte de teclado e as paletas introduzidos em 0.2.0 foram mantidos. Engine e formato de save continuam iguais; a arte passa para versão 2.

## Compilar

Dependências fixadas em `main/idf_component.yml` e `dependencies.lock`: ESP-IDF 5.4.4, BSP oficial `espressif/m5stack_tab5` 1.3.2, LVGL 9.5.0, ESP-Hosted 1.4.0, Wi-Fi Remote 0.8.5 e USB HID Host 1.1.0. O driver MIT do teclado I²C vem diretamente do upstream M5Stack, com commit fixado em `components/m5_tab5_keyboard_component/UPSTREAM.md`. Não depende de arquivos do Pipoca5.

Em uma instalação ESP-IDF 5.4.4 ativada:

```sh
idf.py set-target esp32p4
idf.py build
```

Nesta máquina há também `./tools/build.sh build`. Em outro computador, ative seu ESP-IDF normalmente ou informe `IDF_PATH` e `IDF_PYTHON_ENV_PATH` ao script. A aplicação fica em `build/microcity.bin`. Não há assets embutidos. O firmware pode ser compilado sem instalar Pillow e sem gerar sprites.

A primeira instalação USB usa bootloader, tabela de partições e aplicação nas posições descritas em `build/flash_args`. A primeira instalação deve ser feita pelo usuário com o ESP-IDF, quando desejado; nenhuma gravação é feita pelos scripts de build. Os dois slots OTA de 7 MiB estão reservados, mas esta versão implementa atualização **dos assets**, não um receptor OTA de firmware.

## Gerar e instalar a arte

```sh
python3 -m venv .venv
.venv/bin/pip install -r tools/requirements.txt
.venv/bin/python tools/generate_assets.py
python3 tools/validate_assets.py
python3 tools/install_sd.py /Volumes/SEU_CARTAO
```

Use cartão FAT32. O instalador recebe a raiz do cartão e cria `microcity/`. Instale com o cartão fora do Tab5. Para reinserção com o dispositivo ligado, use **Gerenciar assets → Reabrir cartão SD**. Nunca remova o cartão durante gravação.

No Tab5, **Gerenciar assets → Abrir recepção Wi-Fi** cria o AP temporário `MicroCity-Tab5`. Conecte o computador usando a senha mostrada na tela e envie:

```sh
python3 tools/upload_assets.py http://192.168.4.1/assets --token TOKEN_FICTICIO_DA_TELA
```

O token real é gerado a cada sessão; não está no código. Também é possível passá-lo pela variável `MICROCITY_TOKEN`. A tela informa progresso e resultado. Voltar fecha o serviço, que também expira após 10 minutos. A cidade precisa estar encerrada para instalar arte. O jogo funciona sem rede depois disso.

A arte atual é a versão 8. Para produzir outra versão visual, ajuste os geradores e incremente `--version`, atualizando também as expectativas da regressão visual. O formato permanece estável. O upload não recompila firmware e não acessa `saves/`.

## Jogar

Comece com uma estrada, uma usina e uma estação de água em células vizinhas à mesma via. Zoneie residências e indústria junto às estradas. As zonas crescem a cada cinco dias quando atendidas. Comércio precisa de consumidores. A água e a energia são distribuídas por componentes viários; uma estrada removida pode separar bairros.

Selecione **Mover câmera** para arrastar. Os botões −/+ controlam zoom; **Consultar** explica bloqueios. Estradas e avenidas têm rota em L; zonas usam retângulo por arrasto. Obstáculos rejeitam toda a operação. Escolas, universidade e parque municipal reservam toda a área indicada. O menu retorna após salvar; uma falha preserva a cidade na RAM. **Ajustes** pausa a partida para impostos, som e recuperação do cartão. O som inicia desativado e é opcional.

## Verificar

```sh
./tools/test.sh
```

Testes C++ usam AddressSanitizer/UBSan, compilador C++17 e OpenSSL apenas no host para SHA-256. No macOS desta máquina, o SDK 26.5 evita uma incompatibilidade do linker local com o SDK 27. No Linux, o script usa o compilador padrão e OpenSSL via pkg-config; também aceita `CXX` e `OPENSSL_ROOT`. Python 3.9+ é suficiente para as ferramentas sem Pillow; o gerador usa o ambiente acima.

Veja [arquitetura e formatos](docs/ARQUITETURA.md), [validação física](docs/VALIDACAO.md) e [progresso](docs/PROGRESSO.md). Prévia gerada no computador: [cena](assets/demo-scene.png), [catálogo visual](assets/contact-sheet.png). Não são capturas do Tab5.

Código e arte originais MIT. As dependências mantêm suas próprias licenças; veja [terceiros](docs/TERCEIROS.md).
