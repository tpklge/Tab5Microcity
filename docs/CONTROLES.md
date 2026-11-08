# Controles — firmware 0.9.0

## Toque

- O botão com a ferramenta atual abre a paleta. Categorias: **Infra**, **Zonas**, **Serviços** e **Educação**. Botões grandes mostram nome, custo, atalho e seleção ativa.
- **Mapa: …** abre os oito overlays em botões; não há dropdown.
- A seleção de ferramenta fecha a paleta e aparece na barra inferior. Selecionar demolição não remove nada por si só.
- **Teclas** abre a ajuda no próprio jogo, inclusive sem teclado.
- Com **Mover câmera**, arrastar desloca a visão. Estrada e avenida desenham uma rota; zonas desenham um retângulo. A prévia mostra custo/erro e soltar confirma.
- **Obras [B]** troca prédios altos por marcadores baixos de ocupação, deixando ruas e células atrás deles visíveis.
- Alterar ferramenta, overlay ou deslocar a câmera pelo teclado cancela um toque já em andamento. Soltar esse toque não pode construir na célula anterior.

## Teclado

| Teclas | Ação |
|---|---|
| Setas ou W/A/S/D | Mover a câmera continuamente enquanto pressionadas |
| Shift + movimento | Deslocar mais rápido |
| V / I | Mover câmera / consultar |
| 1 / R | Estrada |
| 2 / 3 / 4 | Zona residencial / comercial / industrial |
| 5 / E | Usina |
| 6 / U | Água |
| 7 / F | Bombeiros |
| 8 / P | Polícia |
| 9 / H | Saúde |
| 0 / K | Parque |
| Q / L / Y | Educação primária / secundária / superior |
| G / N | Parque municipal 2×2 / avenida |
| B | Alternar modo Obras |
| C | Centralizar mapa e restaurar zoom |
| X ou Delete | Selecionar demolição, com confirmação posterior por toque |
| O | Próximo overlay |
| Alt+0 | Mapa normal |
| Alt+1…7 | Energia, água, bombeiros, polícia, saúde, parques, poluição |
| F2…F9 (USB) | Os oito overlays, começando pelo mapa normal |
| Espaço | Alternar pausa / velocidade 1× |
| + / − | Zoom |
| Ctrl+S | Salvar |
| Tab | Abrir/fechar paleta de construção |
| Esc | Fechar painel; sem painel, voltar à ferramenta de câmera |
| F1 | Mostrar ajuda |

Setas/WASD movem a câmera mesmo em pausa. Com painel aberto, o movimento fica suspenso. Ctrl/Alt evitam movimento de câmera durante atalhos; a tecla S usada em Ctrl+S continua bloqueada para movimento até ser solta. Atalhos de seleção disparam uma vez por pressionamento, enquanto movimento usa o estado mantido. Toques rápidos são retidos mesmo quando pressionar e soltar ocorrem entre dois frames.

## Adaptadores e diagnóstico

- **Tab5 Keyboard I²C**: endereço padrão 0x6D, SDA GPIO0, SCL GPIO1, controlador I²C1 como no exemplo oficial; o BSP interno usa I²C0. Driver oficial M5Stack MIT, fixado em commit e incluído em `components/m5_tab5_keyboard_component`; veja UPSTREAM.md. Usa modo Normal para receber pressionar/soltar da matriz 5×14. Tentativa de detecção a cada dois segundos e leitura em task própria.
- **USB HID**: BSP gerencia o host/alimentação; `espressif/usb_host_hid` 1.1.0 gerencia teclados com protocolo boot, relatórios padrão de oito bytes e até seis teclas simultâneas. Teclados exclusivamente NKRO/report protocol, Bluetooth e mouse não estão implementados. Não pressupor suporte universal a qualquer periférico/hub.
- As duas entradas passam por fila limitada até a task da UI. Nenhum callback USB/I²C altera a cidade. Overflow/erro/desconexão limpa teclas mantidas para evitar câmera presa. USB e I²C podem coexistir; desconectar um não libera tecla ainda mantida pelo outro.
- **Menu → Informações** mostra barramento, detecção I²C/USB e último usage HID recebido. Esses indicadores ajudam no teste físico; “detectado” não substitui testar cada tecla.

O usuário confirmou no Tab5 que o teclado físico, os atalhos e a latência das teclas funcionam. Testes de host também cobrem mapeamento, modificadores, toques rápidos, repetição, liberação, desconexão, rollover e atalhos. Essa validação não pressupõe compatibilidade universal com teclados USB.

Referências oficiais: [Tab5 Keyboard](https://docs.m5stack.com/en/tab5/Tab5_Keyboard), [driver M5Stack](https://github.com/m5stack/M5Tab5-Keyboard-UserDemo/tree/b4c36227e2f3373e4e1f8cd7063aefa05cbf72bf/components/m5_tab5_keyboard_component), [HID Host 1.1.0](https://components.espressif.com/components/espressif/usb_host_hid/versions/1.1.0/readme).

Na versão 0.3.0, o desenho é incremental e não publica frames parciais; a seleção usa a câmera do frame visível. O toque tolera até 18 px de desvio antes de cancelar uma construção/consulta; a câmera inicia arraste após 6 px. Botões não rolam a tela. Não há tempo mínimo exigido de pressionamento.

Na versão 0.4.0, um toque simples não solicita um frame intermediário ao pressionar. A ferramenta Consultar informa zona, prédio, nível, motivo de bloqueio, terreno, poluição e coberturas B/P/S/V (bombeiros, polícia, saúde e área verde).

O botão **Guia** abre o tutorial de três páginas. Ele aparece automaticamente na primeira nova cidade até ser fechado ou concluído; depois continua acessível manualmente.

**Home** ou o botão **Centralizar mapa** dentro de Teclas restaura a câmera e o zoom inicial. Painéis modais congelam a simulação enquanto permanecem abertos, mas preservam a velocidade escolhida.
