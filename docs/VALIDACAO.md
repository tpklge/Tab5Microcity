# Validação no Tab5

Nenhum flash, monitor, erase-flash ou acesso serial foi executado pelo agente. A revisão física da placa não foi identificada. As imagens de assets são previews de computador, não capturas do dispositivo.

## Evidências disponíveis

O usuário já confirmou fisicamente jogo contínuo por cerca de 30 minutos, touch, teclado Tab5, três slots, salvar/carregar, persistência após remover a bateria, ajustes de impostos, sons em ações e funcionamento offline. Também confirmou a correção das emendas das ruas. Isso não substitui medições numéricas de heap/FPS nem testes destrutivos de atualização.

Em 07/11/2026, o usuário confirmou a revisão 0.11.0 após testar desastres simples, ciclo mensal, interfaces de Finanças/Ajustes/Guia/Teclas e continuidade dos saves. Também adulterou saves diretamente em editor de texto, removendo bytes do final; os arquivos corrompidos foram rejeitados sem comprometer a aplicação. Combinada ao esquema A/B e aos testes host, essa é evidência física do tratamento de truncamento/corrupção. O fluxo Wi‑Fi de assets permanece deliberadamente não testado, pois a distribuição escolhida é o Web-GUI do launcher.

## Estado de aceite

O ciclo funcional previsto está aceito pelo usuário no Tab5: boot pelo launcher, jogo prolongado, touch, teclado físico, áudio, três slots, save/load, bateria removida, funcionamento offline, assets ausentes, arte atualizada, mapa 128×128, serviços, educação, pontes, porto, ciclo dia/noite, desastres e recuperação de save corrompido. As medições seriais de memória, cache, tick, frame e autosave estão registradas abaixo.

Permanecem fora do aceite somente expansões opcionais e o teste Wi‑Fi dispensado. O timeout de probe I²C observado apenas durante sessões seriais continua documentado, mas não impediu o teste normal prolongado nem a segunda sessão monitorada de 88 segundos.

### Sessão serial de 07/11/2026

Monitor autorizado explicitamente em `/dev/cu.usbmodem11201`, sem flash. Abrir a USB serial reiniciou o P4 com `CHIP_USB_UART_RESET`. O boot identificou Tab5 revisão 2, LCD/touch ST7123 firmware 3, ESP32-P4 revisão 1.3 a 360 MHz e PSRAM 32 MB a 200 MHz. O firmware instalado era 0.10.2.

- Após display: heap interno livre 187.691 bytes; PSRAM livre 31.706.496 bytes.
- Após aplicação: heap interno 152.483 bytes; PSRAM 28.854.220 bytes.
- Cidade aberta: mínimo interno 150.563, maior bloco 102.400; PSRAM livre/mínima 26.756.240 bytes, maior bloco 26.738.688.
- Cache: 1.706.368 bytes, 439 hits/39 misses (91,8% hits).
- Máximos acumulados: frame frio 1.927.546 µs, slice 80.189 µs, fila 20 µs, tick 67.818 µs. Save não foi exercitado nessa janela.
- Frames publicados na segunda janela estável: 10 em 10 s. Esse contador mede publicações solicitadas, não taxa contínua de refresh, portanto não deve ser rotulado como FPS da tela.
- Stack high-water da task UI: 16.588 bytes reportados pelo FreeRTOS.

Após aproximadamente 39,5 s ocorreu `HP_SYS_HP_WDT_RESET` e o launcher voltou a iniciar. Antes do reset havia timeouts de probe I²C a cada dois segundos. A causa ainda não foi isolada; a sessão monitorada não comprova estabilidade, apesar do teste anterior de 30 minutos sem serial ter permanecido jogável.

### Segunda sessão serial — firmware 0.10.3

O boot confirmou a versão 0.10.3. A abertura da USB voltou a provocar `CHIP_USB_UART_RESET`, mas a aplicação permaneceu ativa por aproximadamente 88 segundos até o encerramento voluntário do monitor, sem watchdog ou panic. Os timeouts de probe I²C continuaram a cada dois segundos.

- Heap interno mínimo 150.495 bytes; maior bloco 102.400.
- PSRAM mínima livre 25.858.100 bytes; maior bloco observado 26.214.400.
- Cache final 2.079.616 bytes; 2.032 hits/51 misses (97,6% de acerto).
- Stack high-water da UI 16.588 bytes.
- Máximos acumulados: frame frio 1.890.554 µs, slice 72.667 µs, fila 11.781 µs, tick 68.041 µs e save 511.230 µs.
- Janelas posteriores publicaram entre 4 e 10 frames solicitados por 10 s; isso continua não sendo uma medição do refresh físico do LCD.

A regressão de watchdog da primeira sessão não se repetiu. A latência de fila ficou abaixo de 12 ms, mas frame frio, tick e autosave ainda são operações longas. O timeout I²C periódico deve ser eliminado ou reduzido antes de encerrar o profiling, pois gera log e trabalho inútil.

- Compilação cruzada ESP32-P4 com ESP-IDF 5.4.4 e dependências fixadas.
- Testes C++ de regras e dos módulos reais de persistência/catálogo/cache, com ASan e UBSan.
- Testes Python de instalação manual interrompida/falta de espaço, catálogo incompleto/truncado/incompatível, atualização de arte com IDs preservados e envio HTTP em streaming para servidor de teste.
- Três cidades geradas por `tests/scenarios.cpp`, em `tests/fixtures/`; não são cidades jogadas no Tab5.
- Contact sheet e cena isométrica inspecionadas visualmente no computador.

Os testes de host exercitam o mesmo leitor MCAP/slots do firmware com backend SHA-256 OpenSSL. O servidor ESP-IDF, rádio C6, FAT físico, codec, display e touch não são emulados por esses testes. Remoção de cartão e reinício são representados por truncamento/ausência de arquivos e registros em estados persistentes; não simulam falhas elétricas reais. O caso de cidade aberta é impedido pela máquina de estados da UI, ainda sem automação de interface física.

## Checklist antes de considerar os critérios de aceite concluídos

1. Registrar revisão da placa, controlador detectado pelo BSP, boot frio, resolução 1280×720 efetiva e orientação do touch. Se rotação estiver incorreta, ajustar somente o adaptador de bootstrap, mantendo teste de projeção.
2. Medir memória após display/SD, após menu, após abrir cidade e durante upload: livre, mínimo e maior bloco interno/PSRAM. Conferir PSRAM 32 MB e flash 16 MB reais. Não interpretar os orçamentos como medições.
3. Sem cartão e com pacote corrompido, acessar menu/instalação/informações sem reinícios. Conferir mensagem de SD ausente ao tentar nova cidade.
4. Instalar pacote pelo cartão. Abrir cidade, verificar sprites/anchors, bordas, clipping, ordem de prédios, seleção em todos os cantos e zoom. Verificar que botões não constroem/demolem células sob a UI.
5. Construir estrada por toque e arraste; conferir rota L, custo sem duplicar células, rejeição integral quando há obstáculo e ausência de construção em arraste de câmera.
6. Estabelecer cidade com estradas/usina/água e zonas R/I. Acompanhar 20 minutos: crescimento, comércio, renda, manutenção, serviços e efeitos de poluição. Retirar água e cortar via para verificar déficit por componente. Repor e confirmar recuperação.
7. Alternar pausa, 1×, 2× e 4×; conferir calendário e uma cobrança por mês. Ajustar impostos e verificar demanda. Conferir que telas de ajuste pausam a partida.
8. Salvar manualmente, aguardar autosave, reiniciar e carregar. Corromper apenas o save mais novo em cópia de cartão para testar fallback. Nova cidade salva precisa substituir a anterior por sequência, mesmo com menos ticks.
9. Fechar/abrir pelo menos 20 partidas e comparar mínimos históricos/maiores blocos, sem perda progressiva. Registrar tempos de draw/tick/save e FPS real, sobretudo com cidade média e movimentos. A meta é 20–30 FPS, não uma medição existente. Validar alvos touch de 54 px e latência <100 ms.
10. Ativar som e volume: construção, erro e incêndio. Conferir ausência de interferência com outros periféricos e comportamento ao pausar/sair. Som é opcional e não validado no hardware.
11. Entrar em recepção: validar senha/token por sessão, erro de token incorreto, expiração de 10 minutos e fechamento ao voltar. Confirmar que o C6 embarcado é compatível com ESP-Hosted 1.4.0 e suporta AP nessa configuração, sem gravá-lo automaticamente.
12. Enviar pacote v2 distinguível e confirmar versão na resposta. Abrir a mesma cidade salva, sem recompilar firmware. Confirmar que rede desativada não interfere no jogo.
13. Interromper upload em várias posições, causar timeout, falta de espaço e retirar cartão durante recepção. Reiniciar antes/depois de `.ready`. Conferir que slot anterior válido abre ou que recuperação fica acessível. Preservar cópia externa do cartão durante esses testes destrutivos.
14. Remover/reinserir SD com cidade em RAM, usar Ajustes → Reabrir cartão SD, tentar salvar e retornar à partida. Não reutilizar handles de uma montagem anterior.
15. Medir baseline e picos totais incluindo buffers da plataforma, stacks e rede. Autosave e validação SHA são síncronos; se gerarem pausas perceptíveis, mover trabalho em blocos/worker com snapshot antes do aceite de desempenho.

## Limites desta primeira versão

- Firmware OTA não tem receptor; os slots estão reservados. A atualização implementada é de assets.
- Há uma cidade ativa e dois arquivos de recuperação, sem navegador de múltiplas cidades nem teclado USB.
- O nome inicial é Nova Paludo; edição de nome/seed e preferências de câmera no save ficam para refinamento da interface.
- Modo Wi-Fi é AP temporário, sem cliente de rede doméstica; não persiste SSID/senha domésticos.
- Não há TLS ou assinatura dos pacotes; senha WPA2 e token autenticam a sessão local. SHA-256 não autentica autoria.
- O renderer ainda não tem medição de flush separado, a instrumentação contabiliza desenho+SD e simulação/save. Refinar profiling no Tab5.
- Ainda é necessário validar se a tipografia embutida de recuperação e as listas de ferramentas são confortáveis na tela de cinco polegadas.
- Teclados Tab5 I²C e USB HID boot foram implementados em 0.2.0 e precisam de validação física. Mouse, pontes, relevo, mapas 128×128 e veículos não estão implementados.

## Regressão física 0.2.0

- O usuário relatou a primeira execução da versão anterior, com quadrados nos acentos e dificuldade com dropdown. Isso confirma a execução da UI anterior, sem fornecer medição de desempenho ou validação dos novos adaptadores.
- Conferir ação, construção, água, polícia, saúde, população, setas, × e − em menu, jogo e configurações. Comparar com `assets/fonts/preview-ptbr.png` (prévia do computador).
- Abrir Infra/Zonas/Serviços; selecionar por toque; conferir nome/custo/seleção ativa, fechar e tocar no mapa. Botões não podem construir sob o painel.
- Abrir overlays e alternar os oito sem dropdown. Conferir conteúdo legível e ausência de sobreposição na barra de status.
- Com Tab5 Keyboard, conferir Informações → detecção e última tecla, setas/WASD, combinações, teclas mantidas e desconexão/reconexão. Repetir com USB HID boot.
- Usar Ctrl+S e soltar Ctrl antes de S; a câmera não deve se mover pelo S usado no atalho. Segurar Espaço não pode alternar pausa repetidamente.
- Iniciar toque de construção e mover câmera/selecionar ferramenta pelo teclado antes de soltar: nenhuma construção deve ocorrer na célula antiga.
- Carregar save criado em 0.1.0 e confirmar que os assets v1 continuam funcionando sem reinstalação.

## Regressão física 0.3.0

O usuário confirmou funcionamento geral de 0.2.0, mas relatou necessidade de segurar/repetir alguns toques. Conferir agora um toque curto por ação em menu, paletas, overlays, zoom, salvar e ajustes. Testar drift natural do dedo sem classificar todo toque como arraste; arraste de câmera deve iniciar com deslocamento curto. Verificar frame estável durante pintura incremental e seleção usando a câmera efetivamente mostrada, inclusive ao tocar logo após zoom/movimento. Conferir sprites de água/usina/bombeiros/polícia/saúde/parque com a nova arte v2. Medir slice, frame, fila, heap/PSRAM e flush real; ainda não há medição de hardware nesta revisão.
