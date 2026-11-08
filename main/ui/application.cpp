#include "application.hpp"
#include "audio/audio.hpp"
#include "bsp/esp-bsp.h"
#include "core/city.hpp"
#include "core/clock.hpp"
#include "display/blitter.hpp"
#include "input/keyboard.hpp"
LV_FONT_DECLARE(mc_font_ui);
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "network/receiver.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include "storage/assets.hpp"
#include "storage/save.hpp"
#include "storage/settings.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <sys/stat.h>
using namespace mc;
namespace {
constexpr int W = 1280, H = 720, MapH = 526;
constexpr const char *Root = "/sdcard/microcity";
enum class Screen { Menu, NewCity, LoadCity, Game, Settings, Assets, Info };
enum class Panel { None, Tools, Overlays, Help, Tutorial, Budget };
constexpr const char *ToolNames[] = {
    "Mover câmera [V]", "Consultar [I]", "Estrada [1/R]", "Residencial [2]", "Comercial [3]",
    "Industrial [4]",   "Usina [5/E]",   "Água [6/U]",    "Bombeiros [7/F]", "Polícia [8/P]",
    "Saúde [9/H]",      "Parque [0/K]",  "Demolir [X]", "Escola primária [Q]",
    "Escola secundária [L]", "Universidade [Y]", "Parque municipal [G]", "Avenida [N]",
    "Porto [T]"};
constexpr const char *ToolTitles[] = {
    "Mover câmera", "Consultar", "Estrada", "Residencial", "Comercial", "Industrial", "Usina",
    "Água",         "Bombeiros", "Polícia", "Saúde",       "Parque",    "Demolir",
    "Escola primária", "Escola secundária", "Universidade", "Parque municipal", "Avenida",
    "Porto"};
constexpr const char *ToolKeys[] = {"V",     "I",     "1 / R", "2",     "3",     "4",      "5 / E",
                                    "6 / U", "7 / F", "8 / P", "9 / H", "0 / K", "X / Del",
                                    "Q", "L", "Y", "G", "N", "T"};
constexpr const char *OverlayNames[] = {"Normal",  "Energia", "Água",    "Bombeiros",
                                        "Polícia", "Saúde",   "Parques", "Poluição"};
struct Event {
    int action, a = 0, b = 0, c = 0, d = 0;
    int64_t queuedAt = 0;
};
struct DrawItem {
    int id, x, y;
};
struct App {
    std::unique_ptr<City> city;
    Assets assets;
    Receiver receiver;
    Clock simulationClock;
    Audio audio;
    Keyboard keyboard;
    Keys keys;
    Preferences preferences;
    Panel openPanel = Panel::None;
    int tutorialPage = 0;
    bool tutorialSeen = false, tutorialPending = false;
    lv_obj_t *palette = nullptr;
    int category = 0;
    std::array<lv_obj_t *, 19> toolButtons{};
    std::array<lv_obj_t *, 8> overlayButtons{};
    int64_t receiveStart = 0;
    Camera camera{640, -1785, 1}, renderCamera{640, -1785, 1}, displayedCamera{640, -1785, 1};
    bool sceneReady = false;
    Screen screen = Screen::Menu;
    lv_obj_t *status = nullptr, *notice = nullptr, *canvas = nullptr, *nightShade = nullptr,
             *toolbox = nullptr,
             *overlaybox = nullptr;
    uint16_t *framebuffer = nullptr, *backbuffer = nullptr;
    Blitter blitter;
    std::array<bool, 256> needed{};
    bool rendering = false;
    int assetCursor = 0, paintCursor = 0;
    size_t clearedPixels = 0;
    int64_t frameStarted = 0, scenePublishedAt = 0, maxSlice = 0, maxCommandWait = 0;
    std::string lastStatus, lastNotice, routeFeedback;
    lv_obj_t *volumeLabel = nullptr, *muteButton = nullptr;
    std::array<lv_obj_t *, 3> taxLabels{};
    int64_t preferencesChanged = 0;
    uint32_t pendingSeed = 1;
    int selectedSlot = 1;
    bool overwritePending = false;
    std::array<Event, 32> events{};
    size_t read = 0, write = 0;
    // Only visible cells and decorations enter a frame. Keeping this fixed avoids
    // quadrupling internal RAM when the simulation map grows to 128×128.
    std::array<DrawItem, 4096> items{};
    int itemCount = 0;
    int speed = 1, tool = 0, overlay = 0;
    int warnedPower = -1, warnedWater = -1;
    bool redraw = true, sd = false, dragging = false, route = false, gestureCancelled = false,
         constructionView = true;
    lv_point_t start{}, last{};
    int startX = 0, startY = 0, endX = 0, endY = 0, routeCost = 0;
    int64_t previous = 0, activeTime = 0, lastProfile = 0, maxDraw = 0, maxTick = 0, maxSave = 0;
    unsigned frames = 0;
    int lastNightOpacity = -1;
    std::string feedback = "Energia e água seguem as estradas. Construa usina e estação de água.";
    void push(Event e) {
        e.queuedAt = esp_timer_get_time();
        auto n = (write + 1) % events.size();
        if (n != read) {
            events[write] = e;
            write = n;
        }
    }
    void requestRedraw(bool supersede = false) {
        redraw = true;
        if (supersede)
            rendering = false;
    }
    int renderProgress() const {
        if (sceneReady)
            return 100;
        if (!rendering)
            return 0;
        if (clearedPixels < size_t(W * MapH))
            return int(clearedPixels * 20 / size_t(W * MapH));
        if (assetCursor < 256)
            return 20 + assetCursor * 30 / 256;
        return 50 + (itemCount ? paintCursor * 49 / itemCount : 49);
    }
    void capturePreferences() {
        preferences.citySeed = city ? city->seed : preferences.citySeed;
        preferences.cameraX = camera.x;
        preferences.cameraY = camera.y;
        preferences.zoom = camera.zoom;
        preferences.volume = audio.volume.load();
        preferences.mute = audio.mute.load();
        preferences.tutorialSeen = tutorialSeen;
        preferences.tool = tool;
        preferences.overlay = overlay;
        preferences.speed = speed;
        preferences.selectedSlot = selectedSlot;
        preferences.constructionView = constructionView;
    }
    bool persistPreferences() {
        if (!sd)
            return false;
        capturePreferences();
        bool ok = savePreferences(std::string(Root) + "/settings", preferences);
        if (ok)
            preferencesChanged = 0;
        return ok;
    }
    void schedulePreferences() {
        preferencesChanged = esp_timer_get_time();
    }
    std::string slotRoot(int slot) const {
        return std::string(Root) + "/saves/slot-" + std::to_string(slot);
    }
    bool slotSummary(int slot, SaveSummary &summary) const {
        if (inspectCity(slotRoot(slot), summary))
            return true;
        return slot == 1 && inspectCity(std::string(Root) + "/saves", summary);
    }
    lv_obj_t *text(lv_obj_t *parent, const char *s, int x, int y) {
        auto l = lv_label_create(parent);
        lv_label_set_text(l, s);
        lv_obj_set_style_text_font(l, &mc_font_ui, 0);
        lv_obj_set_pos(l, x, y);
        lv_obj_update_layout(parent);
        lv_obj_set_width(l, std::max(80, int(lv_obj_get_content_width(parent)) - x - 16));
        return l;
    }
    lv_obj_t *card(lv_obj_t *parent, int x, int y, int width, int height, uint32_t color) {
        auto c = lv_obj_create(parent);
        lv_obj_set_pos(c, x, y);
        lv_obj_set_size(c, width, height);
        lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(c, lv_color_hex(color), 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(c, lv_color_hex(0x597985), 0);
        lv_obj_set_style_border_width(c, 1, 0);
        lv_obj_set_style_radius(c, 10, 0);
        lv_obj_set_style_pad_all(c, 12, 0);
        return c;
    }
    lv_obj_t *button(const char *s, int x, int y, int width, int action, int a = 0,
                     lv_obj_t *parent = nullptr, int height = 54) {
        auto b = lv_button_create(parent ? parent : lv_screen_active());
        lv_obj_remove_flag(b, lv_obj_flag_t(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN_HOR |
                                            LV_OBJ_FLAG_SCROLL_CHAIN_VER));
        lv_obj_set_ext_click_area(b, 3);
        lv_obj_set_style_anim_duration(b, 0, 0);
        // A null LV_STYLE_TRANSITION is not a valid way to disable it: LVGL
        // dereferences the descriptor when the button enters its pressed state.
        lv_obj_set_style_text_font(b, &mc_font_ui, 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x31505c), 0);
        lv_obj_set_style_text_color(b, lv_color_hex(0xeee7d5), 0);
        lv_obj_set_style_shadow_width(b, 0, 0);
        lv_obj_set_style_pad_all(b, 6, 0);
        lv_obj_set_pos(b, x, y);
        lv_obj_set_size(b, width, height);
        auto l = lv_label_create(b);
        lv_obj_set_style_text_font(l, &mc_font_ui, 0);
        lv_obj_set_width(l, LV_PCT(100));
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(l, s);
        lv_obj_center(l);
        lv_obj_set_user_data(b, this);
        lv_obj_add_event_cb(
            b,
            [](lv_event_t *e) {
                auto obj = lv_event_get_target_obj(e);
                auto self = (App *)lv_obj_get_user_data(obj);
                auto encoded = (intptr_t)lv_event_get_user_data(e);
                self->push({int(encoded & 255), int(encoded >> 8)});
            },
            LV_EVENT_CLICKED, (void *)(intptr_t)(action | (a << 8)));
        return b;
    }
    void layout(Screen next) {
        screen = next;
        rendering = false;
        sceneReady = false;
        scenePublishedAt = 0;
        volumeLabel = muteButton = nullptr;
        taxLabels.fill(nullptr);
        lastStatus.clear();
        lastNotice.clear();
        cancelGesture();
        lv_obj_clean(lv_screen_active());
        lv_obj_remove_flag(lv_screen_active(),
                           lv_obj_flag_t(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN_HOR |
                                         LV_OBJ_FLAG_SCROLL_CHAIN_VER));
        status = notice = canvas = nightShade = toolbox = overlaybox = palette = nullptr;
        openPanel = Panel::None;
        toolButtons.fill(nullptr);
        overlayButtons.fill(nullptr);
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x182e38), 0);
        lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xeee7d5), 0);
        lv_obj_set_style_text_font(lv_screen_active(), &mc_font_ui, 0);
        if (next == Screen::Game) {
            std::fill(framebuffer, framebuffer + W * MapH, uint16_t(0x1947));
            displayedCamera = camera;
            canvas = lv_canvas_create(lv_screen_active());
            lv_canvas_set_buffer(canvas, framebuffer, W, MapH, LV_COLOR_FORMAT_RGB565);
            lv_obj_set_pos(canvas, 0, 70);
            lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_user_data(canvas, this);
            lv_obj_add_event_cb(
                canvas,
                [](lv_event_t *e) {
                    auto self = (App *)lv_obj_get_user_data(lv_event_get_target_obj(e));
                    self->touch(e);
                },
                LV_EVENT_ALL, nullptr);
            nightShade = lv_obj_create(lv_screen_active());
            lv_obj_set_pos(nightShade, 0, 70);
            lv_obj_set_size(nightShade, W, MapH);
            lv_obj_remove_flag(nightShade, lv_obj_flag_t(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
            lv_obj_set_style_bg_color(nightShade, lv_color_hex(0x10264f), 0);
            lv_obj_set_style_bg_opa(nightShade, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(nightShade, 0, 0);
            lv_obj_set_style_pad_all(nightShade, 0, 0);
            status = lv_label_create(lv_screen_active());
            lv_obj_set_pos(status, 16, 4);
            toolbox = button(ToolNames[tool], 12, 607, 220, 22);
            button(constructionView ? "Obras: visível [B]" : "Obras: normal [B]", 240, 607, 160,
                   39);
            char overlayTitle[64];
            snprintf(overlayTitle, sizeof(overlayTitle), "Mapa: %s", OverlayNames[overlay]);
            overlaybox = button(overlayTitle, 408, 607, 200, 24);
            button("Pausa", 616, 607, 88, 20);
            button("1×", 712, 607, 55, 3, 1);
            button("2×", 775, 607, 55, 3, 2);
            button("4×", 838, 607, 55, 3, 4);
            button("−", 901, 607, 54, 4, 255);
            button("+", 963, 607, 54, 4, 1);
            button("Salvar", 1025, 607, 110, 5);
            button("Menu", 1143, 607, 124, 6);
            button("Guia", 840, 8, 80, 27);
            button("Finanças", 928, 8, 125, 36);
            button("Teclas", 1061, 8, 90, 23);
            button("Ajustes", 1159, 8, 111, 7);
            lv_obj_set_width(status, 820);
            notice = lv_label_create(lv_screen_active());
            lv_obj_set_pos(notice, 14, 676);
            lv_obj_set_width(notice, 1250);
            lv_label_set_text(notice, feedback.c_str());
        } else {
            text(lv_screen_active(), "MICROCITY  /  PALUDO", 60, 45);
            if (next == Screen::Menu) {
                text(lv_screen_active(), "Construa uma cidade. Cuide de quem vive nela.", 60, 100);
                button("Nova cidade", 60, 180, 330, 1);
                button("Continuar / carregar", 60, 245, 330, 2);
                button("Configurações", 60, 310, 330, 7);
                button("Gerenciar assets", 60, 375, 330, 8);
                button("Informações", 60, 440, 330, 9);
                text(lv_screen_active(), "Mapa 128 × 128 • simulação nativa • recursos no microSD",
                     460, 190);
                text(lv_screen_active(),
                     "Comece com estradas, usina e água conectadas.\nZoneie residências e "
                     "indústrias junto às vias.\nServiços e parques melhoram a cidade.\nSelecione "
                     "Mover câmera para arrastar o mapa.",
                     460, 250);
                text(lv_screen_active(), feedback.c_str(), 60, 550);
            }
            if (next == Screen::NewCity) {
                SaveSummary occupied;
                bool exists = slotSummary(selectedSlot, occupied);
                char seedText[300];
                snprintf(seedText, sizeof(seedText),
                         "NOVA CIDADE — SLOT %d\nSEED %lu  /  0x%08lX\n\nA mesma seed gera o mesmo "
                         "terreno.%s",
                         selectedSlot, (unsigned long)pendingSeed, (unsigned long)pendingSeed,
                         exists ? "\nEste slot já contém uma cidade." : "\nSlot vazio.");
                text(lv_screen_active(), seedText, 60, 150);
                for (int i = 1; i <= 3; i++) {
                    char label[30];
                    snprintf(label, sizeof(label), "Slot %d%s", i,
                             i == selectedSlot ? " [ativo]" : "");
                    button(label, 60 + (i - 1) * 160, 310, 145, 33, i);
                }
                button("Outra seed", 60, 390, 220, 31);
                if (overwritePending) {
                    text(lv_screen_active(), "Confirmar substituição do slot?", 320, 380);
                    button("Confirmar", 650, 370, 190, 34);
                } else
                    button(exists ? "Substituir cidade" : "Criar cidade", 320, 390, 280, 32);
            }
            if (next == Screen::LoadCity) {
                text(lv_screen_active(), "Escolha uma cidade salva", 60, 120);
                for (int i = 1; i <= 3; i++) {
                    SaveSummary summary;
                    char label[220];
                    if (slotSummary(i, summary))
                        snprintf(label, sizeof(label),
                                 "Slot %d · Hab. %d · Fel. %d%% · $%lld\nAno %u · E−%d A−%d · "
                                 "seed %lu",
                                 i, summary.population, summary.happiness, (long long)summary.money,
                                 unsigned(summary.days / 360 + 1), summary.unpowered,
                                 summary.unwatered, (unsigned long)summary.seed);
                    else
                        snprintf(label, sizeof(label), "Slot %d · vazio", i);
                    button(label, 60, 190 + (i - 1) * 105, 600, 35, i, nullptr, 88);
                }
            }
            if (next == Screen::Info) {
                text(lv_screen_active(),
                     "Firmware 0.11.0 / engine 1\nArte original gerada por ferramentas Python. "
                     "Licença MIT.\nAssets, cidades, preferências e sessão ficam no "
                     "microSD.\nDesempenho e "
                     "periféricos ainda exigem validação física.",
                     60, 160);
                char b[420];
                snprintf(b, sizeof(b),
                         "City: %u bytes | Cell: %u bytes\nHeap interno: %u | PSRAM: %u\nCache: %u "
                         "bytes | hits %u | misses %u\nTeclado Tab5: %s | USB: %s | última tecla "
                         "HID: %u\nBarramento do teclado: %s | dispositivo já respondeu: %s",
                         unsigned(sizeof(City)), unsigned(sizeof(Cell)),
                         unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
                         unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
                         unsigned(assets.bytes), unsigned(assets.hits), unsigned(assets.misses),
                         keyboard.tab5Connected ? "detectado" : "ausente",
                         keyboard.usbConnected ? "detectado" : "ausente",
                         unsigned(keyboard.lastUsage.load()),
                         keyboard.tab5BusReady ? "pronto" : "falhou",
                         keyboard.tab5ProbeSeen ? "sim" : "não");
                text(lv_screen_active(), b, 60, 310);
            }
            if (next == Screen::Assets) {
                button("Abrir recepção Wi-Fi", 650, 160, 400, 13);
                button("Reabrir cartão SD", 650, 550, 400, 16);
                notice = lv_label_create(lv_screen_active());
                lv_obj_set_pos(notice, 650, 240);
                lv_obj_set_width(notice, 580);
                text(lv_screen_active(),
                     "RECUPERAÇÃO PELO LAUNCHER\n\nEnvie primeiro slot-a.mcap e, somente depois, "
                     "slot-a.ready para /microcity no microSD. O arquivo .ready por último torna "
                     "a troca atômica. Também é possível instalar o diretório dist/launcher com "
                     "tools/install_sd.py.\n\nUse Reabrir cartão SD para validar novamente. A cidade "
                     "salva não é apagada quando os assets faltam. Nenhum sprite fica embutido no firmware.",
                     60, 160);
            }
            if (next == Screen::Settings) {
                text(lv_screen_active(), "AJUSTES DA CIDADE", 60, 72);
                auto taxes = card(lv_screen_active(), 50, 125, 560, 430, 0x203e49);
                auto soundCard = card(lv_screen_active(), 640, 125, 580, 430, 0x203e49);
                char volumeText[60];
                snprintf(volumeText, sizeof(volumeText), "Volume: %d%%", audio.volume.load());
                text(taxes, "IMPOSTOS", 12, 10);
                text(taxes,
                     "Alíquota segura de 1 a 20%. Impostos altos reduzem a demanda de cada zona.",
                     12, 52);
                text(soundCard, "SOM E ARMAZENAMENTO", 12, 10);
                volumeLabel = text(soundCard, volumeText, 12, 70);
                muteButton =
                    button(audio.mute ? "Som: desligado" : "Som: ligado", 12, 112, 260, 14, 0,
                           soundCard, 58);
                button("Volume −", 12, 190, 150, 15, 0, soundCard, 58);
                button("Volume +", 174, 190, 150, 15, 1, soundCard, 58);
                button("Reabrir cartão SD", 12, 292, 300, 16, 0, soundCard, 62);
                text(soundCard,
                     "Preferências, cidades e assets permanecem no microSD. Reabra após reinserir o cartão.",
                     12, 365);
                for (int i = 0; i < 3; i++) {
                    char b[64];
                    snprintf(b, sizeof(b), "%s: %u%%",
                             i == 0   ? "Residencial"
                             : i == 1 ? "Comercial"
                                      : "Industrial",
                             city ? city->tax[i] : 9);
                    taxLabels[i] = text(taxes, b, 12, 125 + i * 82);
                    button("−", 330, 108 + i * 82, 70, 10, i * 2, taxes, 58);
                    button("+", 414, 108 + i * 82, 70, 10, i * 2 + 1, taxes, 58);
                }
            }
            if (next != Screen::Menu)
                button(city ? "Voltar à cidade" : "Voltar ao menu", 60, 620, 300, 0);
        }
        previous = esp_timer_get_time();
        simulationClock.reset();
        redraw = true;
    }
    void highlight(lv_obj_t *button, bool selected) {
        if (!button)
            return;
        lv_obj_set_style_bg_color(button, lv_color_hex(selected ? 0xe4c688 : 0x31505c), 0);
        lv_obj_set_style_text_color(button, lv_color_hex(selected ? 0x182e38 : 0xeee7d5), 0);
        lv_obj_set_style_border_width(button, selected ? 3 : 1, 0);
        lv_obj_set_style_border_color(button, lv_color_hex(selected ? 0xffe8ad : 0x53717b), 0);
    }
    void closePanel() {
        if (palette)
            lv_obj_delete(palette);
        palette = nullptr;
        openPanel = Panel::None;
        toolButtons.fill(nullptr);
        overlayButtons.fill(nullptr);
    }
    void cancelGesture() {
        route = dragging = false;
        gestureCancelled = true;
    }
    void rememberTutorial() {
        tutorialSeen = true;
        tutorialPending = false;
        persistPreferences();
    }
    void showPanel(Panel kind) {
        closePanel();
        openPanel = kind;
        cancelGesture();
        palette = lv_obj_create(lv_screen_active());
        bool wide = kind == Panel::Help || kind == Panel::Tutorial || kind == Panel::Budget;
        lv_obj_set_pos(palette, wide ? 180 : 900, wide ? 82 : 95);
        lv_obj_set_size(palette, wide ? 920 : 368, wide ? 550 : 490);
        lv_obj_remove_flag(palette, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(palette, lv_color_hex(0x203e49), 0);
        lv_obj_set_style_text_font(palette, &mc_font_ui, 0);
        lv_obj_set_style_pad_all(palette, 0, 0);
        lv_obj_set_style_border_color(palette, lv_color_hex(0xe4c688), 0);
        text(palette,
             kind == Panel::Tools      ? "Construções"
             : kind == Panel::Overlays ? "Sobreposições"
             : kind == Panel::Tutorial ? "Guia rápido"
             : kind == Panel::Budget   ? "Finanças e infraestrutura"
                                       : "Teclado e controles",
             14, 14);
        button("Fechar  [Esc]", wide ? 742 : 276, 8, wide ? 160 : 78, 25, 0, palette, 48);
        if (kind == Panel::Tools) {
            const char *names[] = {"Infra", "Zonas", "Serviços", "Educação"};
            for (int i = 0; i < 4; i++) {
                auto b = button(names[i], 8 + i * 89, 60, 84, 26, i, palette);
                highlight(b, i == category);
            }
            constexpr int groups[4][6] = {{0, 1, 2, 17, 6, 7},
                                          {3, 4, 5, 12, -1, -1},
                                          {8, 9, 10, 11, 16, 18},
                                          {13, 14, 15, -1, -1, -1}};
            int n = 0;
            for (int id : groups[category])
                if (id >= 0) {
                    char label[96];
                    int building = id >= 6 && id <= 11 ? id - 4
                                   : id == 13 ? int(Building::PrimarySchool)
                                   : id == 14 ? int(Building::SecondarySchool)
                                   : id == 15 ? int(Building::University)
                                   : id == 16 ? int(Building::LargePark)
                                   : id == 17 ? int(Building::Avenue)
                                   : id == 18 ? int(Building::Port)
                                              : 0;
                    int cost = building              ? Definitions[building].cost
                               : id >= 2 && id <= 5 ? 10
                               : id == 12           ? 5
                                                    : 0;
                    snprintf(label, sizeof(label), "%s\n%s%s%d · %s", ToolTitles[id],
                             cost ? "$" : "", cost ? "" : "", cost, ToolKeys[id]);
                    if (!cost)
                        snprintf(label, sizeof(label), "%s\n[%s]", ToolTitles[id], ToolKeys[id]);
                    toolButtons[id] = button(label, 10 + (n % 2) * 176, 124 + (n / 2) * 106, 168,
                                             17, id, palette, 94);
                    highlight(toolButtons[id], id == tool);
                    n++;
                }
            text(palette, "Botões grandes; B alterna o modo Obras; Tab fecha.", 12, 452);
        } else if (kind == Panel::Overlays) {
            for (int i = 0; i < 8; i++) {
                char label[80];
                snprintf(label, sizeof(label), "%s\nAlt+%d", OverlayNames[i], i);
                overlayButtons[i] =
                    button(label, 10 + (i % 2) * 176, 68 + (i / 2) * 90, 168, 18, i, palette, 80);
                highlight(overlayButtons[i], i == overlay);
            }
        } else if (kind == Panel::Tutorial) {
            static const char *pages[] = {
                "1/3  INFRAESTRUTURA\n\nConstrua uma estrada. Ao lado dela, coloque uma usina e "
                "uma estação de água. Energia e água percorrem apenas trechos conectados da rede "
                "viária. A barra superior mostra consumo/capacidade.",
                "2/3  CRESCIMENTO\n\nZoneie Residencial, Comercial e Industrial junto às vias. "
                "Toque e arraste para marcar um retângulo. As construções aparecem com acesso, "
                "energia, água e demanda. Use Consultar para ver o bloqueio de cada célula.",
                "3/3  QUALIDADE E RISCOS\n\nBombeiros, polícia e saúde alcançam bairros pelas "
                "estradas. Educação e parques melhoram a cidade; indústrias poluem. Incêndios, "
                "tempestades e tremores podem atingir prédios, mas nunca destroem vias. Um ciclo "
                "completo de dia e noite ocorre a cada mês."};
            auto content = card(palette, 24, 78, 872, 370, 0x294b58);
            text(content, pages[tutorialPage], 16, 18);
            if (tutorialPage)
                button("← Anterior", 24, 472, 190, 29, 0, palette, 58);
            button(tutorialPage == 2 ? "Começar" : "Próximo →", 706, 472, 190,
                   tutorialPage == 2 ? 30 : 28, 0, palette, 58);
        } else if (kind == Panel::Budget && city) {
            int counts[18]{};
            for (const auto &cell : city->cells)
                counts[int(cell.building)]++;
            const auto &m = city->metrics;
            char economy[260], demand[260], infrastructure[420];
            snprintf(economy, sizeof(economy),
                     "ECONOMIA\n\nCaixa     $%lld\nReceita   +$%d / mês\nDespesas  −$%d / mês\nResultado %s$%d",
                     (long long)city->money, m.revenue, m.expenses,
                     m.revenue - m.expenses >= 0 ? "+" : "", m.revenue - m.expenses);
            snprintf(demand, sizeof(demand),
                     "IMPOSTOS E DEMANDA\n\nResidencial  %u%%  ·  %d\nComercial    %u%%  ·  %d\nIndustrial   %u%%  ·  %d\n\nDesastres ativos: %d",
                     unsigned(city->tax[0]), m.demand[0], unsigned(city->tax[1]), m.demand[1],
                     unsigned(city->tax[2]), m.demand[2], m.fires + m.storms + m.quakes);
            snprintf(infrastructure, sizeof(infrastructure),
                     "INFRAESTRUTURA\n\nEnergia  %d / %d  ·  %d sem atendimento\nÁgua     %d / %d  ·  %d sem atendimento\n\nVias %d  ·  Usinas %d  ·  Bombas %d  ·  Avenidas %d\nBombeiros %d  ·  Polícia %d  ·  Saúde %d  ·  Parques %d\nEducação primária/secundária/superior: %d / %d / %d",
                     m.powerNeed, m.powerCapacity,
                     m.unpowered, m.waterNeed, m.waterCapacity, m.unwatered,
                     counts[int(Building::Road)] + counts[int(Building::Avenue)],
                     counts[int(Building::Power)],
                     counts[int(Building::Water)], counts[int(Building::Avenue)], counts[int(Building::Fire)],
                     counts[int(Building::Police)], counts[int(Building::Health)],
                     counts[int(Building::Park)] + counts[int(Building::LargePark)],
                     counts[int(Building::PrimarySchool)] / 4,
                     counts[int(Building::SecondarySchool)] / 4,
                     counts[int(Building::University)] / 9);
            auto a = card(palette, 24, 76, 276, 430, 0x294b58);
            auto b = card(palette, 312, 76, 276, 430, 0x294b58);
            auto c = card(palette, 600, 76, 296, 430, 0x294b58);
            text(a, economy, 8, 8); text(b, demand, 8, 8); text(c, infrastructure, 8, 8);
        } else {
            auto navigation = card(palette, 24, 78, 420, 350, 0x294b58);
            auto building = card(palette, 456, 78, 440, 350, 0x294b58);
            text(navigation,
                 "NAVEGAÇÃO\n\nSetas / WASD   mover câmera\nShift           movimento rápido\nV               mover\nI               consultar\nB               modo Obras\nC               centralizar\nO / Alt+0–7     overlays\n+/−             zoom\nEspaço          pausa\nEsc             fechar/cancelar", 8, 8);
            text(building,
                 "CONSTRUÇÃO\n\n1–4     via e zonas R/C/I\n5–9, 0  energia, água e serviços\nR/E/U   via, energia e água\nF/P/H/K bombeiros, polícia, saúde, parque\nQ/L/Y   escolas e universidade\nG / N   parque municipal / avenida\nT       porto\nX/Del   demolir\nCtrl+S  salvar\nTab     abrir construções", 8, 8);
            button("Centralizar mapa  [C]", 330, 458, 260, 37, 0, palette, 58);
        }
        requestRedraw(true);
    }
    void selectTool(int value) {
        if (value < 0 || value > 18)
            return;
        tool = value;
        cancelGesture();
        closePanel();
        if (toolbox)
            lv_label_set_text(lv_obj_get_child(toolbox, 0), ToolNames[tool]);
        feedback = std::string("Selecionado: ") + ToolNames[tool];
        schedulePreferences();
        requestRedraw(true);
    }
    void selectOverlay(int value) {
        if (value < 0 || value > 7)
            return;
        overlay = value;
        cancelGesture();
        closePanel();
        if (overlaybox) {
            char title[64];
            snprintf(title, sizeof(title), "Mapa: %s", OverlayNames[overlay]);
            lv_label_set_text(lv_obj_get_child(overlaybox, 0), title);
        }
        feedback = std::string("Sobreposição: ") + OverlayNames[overlay];
        schedulePreferences();
        requestRedraw(true);
    }
    bool ready() {
        if (!framebuffer || !backbuffer) {
            feedback = "PSRAM insuficiente para abrir cidade.";
            return false;
        }
        std::string p;
        if (!sd) {
            feedback = "microSD indisponível. Desligue e confira o cartão FAT32.";
            return false;
        }
        if (!selectAssets(Root, p) || !assets.open(p)) {
            feedback = "Pacote visual ausente ou inválido. Abra Gerenciar assets.";
            return false;
        }
        return true;
    }
    bool openSavedCity(int slot) {
        if (!ready())
            return false;
        city.reset(new City);
        bool loaded = loadCity(slotRoot(slot), *city);
        bool legacy = false;
        if (!loaded && slot == 1) {
            loaded = loadCity(std::string(Root) + "/saves", *city);
            legacy = loaded;
        }
        if (!loaded) {
            city.reset();
            feedback = "Slot vazio ou cidade inválida; outros slots foram preservados.";
            return false;
        }
        selectedSlot = slot;
        if (legacy)
            city->dirty = true; // migrate to slot-1 on the next safe save
        schedulePreferences();
        activeTime = 0;
        if (preferences.citySeed == city->seed) {
            camera = {preferences.cameraX, preferences.cameraY, preferences.zoom};
            tool = preferences.tool;
            overlay = preferences.overlay;
            speed = preferences.speed;
            constructionView = preferences.constructionView != 0;
        } else {
            camera = {640, -1785, 1};
            tool = overlay = 0;
            speed = 1;
        }
        layout(Screen::Game);
        return true;
    }
    void touch(lv_event_t *e) {
        auto code = lv_event_get_code(e);
        if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING && code != LV_EVENT_RELEASED &&
            code != LV_EVENT_PRESS_LOST)
            return;
        if (!canvas || !city || !sceneReady)
            return;
        auto indev = lv_indev_active();
        if (!indev)
            return;
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        p.y -= 70;
        if (code == LV_EVENT_PRESSED) {
            gestureCancelled = false;
            start = last = p;
            dragging = false;
            auto a = displayedCamera.inverse(p.x, p.y);
            startX = endX = int(floorf(a.x + .5f));
            startY = endY = int(floorf(a.y + .5f));
            route = tool == 2 || (tool >= 3 && tool <= 5) || tool == 17;
            // Only roads have a visual preview on press. A simple tap is
            // rendered after its command, avoiding two full map frames.
            if (route)
                requestRedraw(true);
        }
        if (code == LV_EVENT_PRESSING) {
            if (gestureCancelled)
                return;
            if (touchIsDrag(p.x - start.x, p.y - start.y, tool == 0))
                dragging = true;
            if (tool == 0 && dragging) {
                int dx = p.x - last.x, dy = p.y - last.y;
                camera.x = std::clamp(camera.x + float(dx), -6000.f, 7000.f);
                camera.y = std::clamp(camera.y + float(dy), -8500.f, 550.f);
                if (dx || dy)
                    requestRedraw(true);
                if (dx || dy)
                    schedulePreferences();
            }
            if (route) {
                auto a = displayedCamera.inverse(p.x, p.y);
                int x = int(floorf(a.x + .5f)), y = int(floorf(a.y + .5f));
                if (x != endX || y != endY) {
                    endX = x;
                    endY = y;
                    requestRedraw(true);
                }
            }
            last = p;
        }
        if (code == LV_EVENT_RELEASED) {
            if (gestureCancelled) {
                gestureCancelled = false;
                route = false;
                return;
            }
            if (route)
                push({tool >= 3 && tool <= 5 ? 38 : 12, startX, startY, endX, endY});
            else if (!dragging && tool > 0)
                push({11, startX, startY});
            if (route)
                requestRedraw(true);
            route = false;
        }
        if (code == LV_EVENT_PRESS_LOST) {
            if (route)
                requestRedraw(true);
            route = false;
            dragging = false;
        }
    }
    void blit(int id, int cx, int cy) {
        blitter.draw(backbuffer, W, MapH, assets.raster(id), cx, cy);
    }
    bool save() {
        if (city && !city->dirty && city->saveSequence)
            return true;
        int64_t start = esp_timer_get_time();
        bool ok = saveCity(slotRoot(selectedSlot), *city);
        maxSave = std::max(maxSave, esp_timer_get_time() - start);
        return ok;
    }
    void beginDraw() {
        if (!canvas || !city)
            return;
        rendering = true;
        renderCamera = camera;
        redraw = false;
        frameStarted = esp_timer_get_time();
        assetCursor = paintCursor = 0;
        clearedPixels = 0;
        blitter.setScale(int(lroundf(camera.zoom * 4)));
        itemCount = 0;
        needed.fill(false);
        auto add = [&](int id, int x, int y) {
            if (itemCount < int(items.size())) {
                items[itemCount++] = {id, x, y};
                needed[id] = true;
            }
        };
        // Inverse-project the viewport and visit only the visible isometric
        // diagonals. A 128x128 city must not scan all 16,384 cells per frame.
        int minCol = Side - 1, maxCol = 0, minRow = Side - 1, maxRow = 0;
        for (auto corner : std::array<lv_point_t, 4>{{{-180, -180}, {W + 180, -180},
                                                       {-180, MapH + 180}, {W + 180, MapH + 180}}}) {
            auto q = renderCamera.inverse(corner.x, corner.y);
            minCol = std::min(minCol, int(std::floor(q.x)) - 4);
            maxCol = std::max(maxCol, int(std::ceil(q.x)) + 4);
            minRow = std::min(minRow, int(std::floor(q.y)) - 4);
            maxRow = std::max(maxRow, int(std::ceil(q.y)) + 4);
        }
        minCol = std::clamp(minCol, 0, Side - 1);
        maxCol = std::clamp(maxCol, 0, Side - 1);
        minRow = std::clamp(minRow, 0, Side - 1);
        maxRow = std::clamp(maxRow, 0, Side - 1);
        std::array<lv_point_t, 8> ports{};
        int portCount = 0;
        for (int i = 0; i < Count && portCount < int(ports.size()); i++)
            if (city->cells[i].building == Building::Port && city->cells[i].stage == 1)
                ports[portCount++] = {int16_t(i % Side), int16_t(i / Side)};
        int ships = 0;
        for (int diag = minCol + minRow; diag <= maxCol + maxRow; diag++)
            for (int row = std::max(minRow, diag - maxCol);
                 row <= std::min(maxRow, diag - minCol); row++) {
                int col = diag - row, i = row * Side + col;
                auto p = renderCamera.project(col, row);
                if (p.x < -100 || p.x > W + 100 || p.y < -40 || p.y > MapH + 140)
                    continue;
                auto &c = city->cells[i];
                int id = i % 3;
                int footprintWidth, footprintHeight;
                City::footprint(c.building, footprintWidth, footprintHeight);
                bool coveredByAnchor = footprintWidth > 1 && c.stage > 1 && !constructionView;
                if (c.building == Building::Bridge)
                    id = 156 + c.roadMask;
                else if (c.terrain == Terrain::Water) {
                    int mask = 0;
                    const int dx[] = {0, 1, 0, -1}, dy[] = {-1, 0, 1, 0};
                    for (int d = 0; d < 4; d++) {
                        int x = col + dx[d], y = row + dy[d];
                        if (City::inside(x, y) &&
                            city->cells[y * Side + x].terrain != Terrain::Water)
                            mask |= 1 << d;
                    }
                    id = 10 + mask;
                } else if (City::isRoad(c.building))
                    id = (c.building == Building::Avenue ? 140 : 40) + c.roadMask;
                else if (constructionView && int(c.building) >= 2)
                    id = c.building == Building::Residence ? 180 + std::clamp(int(c.stage), 1, 3) - 1
                         : c.building == Building::Shop ? 183 + std::clamp(int(c.stage), 1, 3) - 1
                         : c.building == Building::Factory ? 186 + std::clamp(int(c.stage), 1, 3) - 1
                                                           : 117;
                else if (int(c.building) >= int(Building::Residence) &&
                         int(c.building) <= int(Building::Factory))
                    id = (c.building == Building::Residence ? 70
                          : c.building == Building::Shop    ? 80
                                                            : 90) +
                         (c.stage - 1) * 2 + i % 2;
                else if (int(c.building) >= 2 && int(c.building) <= 7)
                    id = 100 + int(c.building) - 2;
                else if (int(c.building) >= int(Building::PrimarySchool) &&
                         int(c.building) <= int(Building::LargePark) && c.stage == 1)
                    id = 132 + int(c.building) - int(Building::PrimarySchool);
                else if (c.building == Building::Port && c.stage == 1)
                    id = 172;
                else if (c.zone != Zone::None)
                    id = 60 + int(c.zone) - 1;
                else if (c.terrain == Terrain::Trees)
                    id = 30 + i % 3;
                // The anchor sprite already contains the complete lot. Later
                // reserved cells must not repaint grass over its campus wings.
                if (!coveredByAnchor)
                    add(id, int(p.x), int(p.y));
                if (footprintWidth > 1)
                    add(189, int(p.x), int(p.y));
                // Ships are deterministic, sparse and hard-capped. They never
                // enter simulation state or increase save size. Road vehicles
                // are intentionally disabled after physical performance tests.
                uint32_t motion = uint32_t(i) * 2654435761u + uint32_t(city->days / 2) * 97u;
                if (c.terrain == Terrain::Water && c.building != Building::Bridge && ships < 4 &&
                    motion % 47u == 0) {
                    bool nearPort = false;
                    for (int n = 0; n < portCount; n++)
                        nearPort |= std::max(abs(col - ports[n].x), abs(row - ports[n].y)) <= 12;
                    if (nearPort) {
                        add(177 + (motion >> 9) % 3, int(p.x), int(p.y));
                        ships++;
                    }
                }
                if (c.fire)
                    add(c.fire <= 3 ? 113 : c.fire <= 6 ? 190 : 191, int(p.x), int(p.y));
                if (c.distress > 15)
                    add(112, int(p.x), int(p.y));
                if (overlay) {
                    int flag = overlay == 1 ? Powered : overlay == 2 ? Watered : 1 << overlay;
                    bool valid = overlay == 7 ? c.pollution < 40 : (c.flags & flag);
                    add(valid ? 110 : 111, int(p.x), int(p.y));
                }
            }
        if (route) {
            int cost = 0;
            bool zoning = tool >= 3 && tool <= 5;
            auto r = zoning ? city->zoneRect(startX, startY, endX, endY, Zone(tool - 2), false, cost)
                            : city->roadRoute(startX, startY, endX, endY, false, cost,
                                              tool == 17 ? Building::Avenue : Building::Road);
            routeCost = cost;
            if (zoning) {
                for (int y = std::min(startY, endY); y <= std::max(startY, endY); y++)
                    for (int x = std::min(startX, endX); x <= std::max(startX, endX); x++)
                        if (City::inside(x, y)) {
                            auto p = camera.project(x, y);
                            add(r == Result::Ok ? 110 : 111, p.x, p.y);
                        }
            } else {
                int x = startX, y = startY;
                for (int n = 0; n < Side * 2 && City::inside(x, y); n++) {
                    auto p = camera.project(x, y);
                    add(r == Result::Ok ? 110 : 111, p.x, p.y);
                    if (x == endX && y == endY)
                        break;
                    if (x != endX)
                        x += x < endX ? 1 : -1;
                    else
                        y += y < endY ? 1 : -1;
                }
            }
            char b[100];
            snprintf(b, sizeof(b), "Prévia %s: $%d · %s",
                     zoning ? "do zoneamento" : (tool == 17 ? "da avenida" : "da estrada"), cost,
                     r == Result::Ok ? "solte para construir" : message(r));
            routeFeedback = b;
        }
    }
    void drawStep() {
        if (!rendering || !canvas)
            return;
        int64_t started = esp_timer_get_time();
        int painted = 0;
        // Yield to input between small chunks. A partial frame is never published.
        do {
            if (clearedPixels < size_t(W * MapH)) {
                size_t end = std::min(clearedPixels + W * 16, size_t(W * MapH));
                std::fill(backbuffer + clearedPixels, backbuffer + end, uint16_t(0x1947));
                clearedPixels = end;
                continue;
            }
            if (assetCursor < 256) {
                int id = assetCursor++;
                if (needed[id]) {
                    bool cold = !assets.cached(id);
                    if (!assets.prefetch(id)) {
                        feedback = "Falha ao ler assets do SD; cidade preservada em RAM.";
                        speed = 0;
                        rendering = false;
                        redraw = false;
                        break;
                    }
                    if (cold)
                        break;
                }
                continue;
            }
            if (paintCursor < itemCount) {
                const auto &item = items[paintCursor++];
                blit(item.id, item.x, item.y);
                if (++painted >= 48)
                    break;
                continue;
            }
            std::swap(framebuffer, backbuffer);
            displayedCamera = renderCamera;
            sceneReady = true;
            scenePublishedAt = esp_timer_get_time();
            lv_canvas_set_buffer(canvas, framebuffer, W, MapH, LV_COLOR_FORMAT_RGB565);
            lv_obj_invalidate(canvas);
            rendering = false;
            frames++;
            maxDraw = std::max(maxDraw, esp_timer_get_time() - frameStarted);
            break;
        } while (esp_timer_get_time() - started < 3000);
        maxSlice = std::max(maxSlice, esp_timer_get_time() - started);
    }
    void updateHud() {
        if (screen != Screen::Game || !city || !status || !notice)
            return;
        char b[280];
        auto &m = city->metrics;
        snprintf(b, sizeof(b),
                 "Caixa $%lld%s · Hab. %d · Emp. %d · Fel. %d%% · %02u/%02u/%u  "
                 "  %d× · %s\nR/C/I %d/%d/%d · Energia %d/%d · Água %d/%d · $%d − $%d/mês",
                 (long long)city->money, city->money < 0 ? " [DÍVIDA]" : "", m.population, m.jobs,
                 m.happiness, unsigned(city->days % 30 + 1), unsigned((city->days / 30) % 12 + 1),
                 unsigned(city->days / 360 + 1), speed,
                 std::abs(int(city->days % 30) - 15) >= 11 ? "Noite" :
                 std::abs(int(city->days % 30) - 15) >= 6 ? "Crepúsculo" : "Dia",
                 m.demand[0], m.demand[1], m.demand[2],
                 m.powerNeed, m.powerCapacity, m.waterNeed, m.waterCapacity, m.revenue, m.expenses);
        if (lastStatus != b) {
            lastStatus = b;
            lv_label_set_text(status, b);
        }
        std::string noticeText;
        if (!sceneReady)
            noticeText = "Carregando mapa: " + std::to_string(renderProgress()) + "%";
        else
            noticeText = route && !routeFeedback.empty() ? routeFeedback : feedback;
        if (lastNotice != noticeText) {
            lastNotice = noticeText;
            lv_label_set_text(notice, noticeText.c_str());
        }
        if (nightShade) {
            int distance = std::abs(int(city->days % 30) - 15);
            int opacity = std::clamp((distance - 4) * 7, 0, 72);
            if (opacity != lastNightOpacity) {
                lastNightOpacity = opacity;
                lv_obj_set_style_bg_opa(nightShade, opacity, 0);
                lv_obj_invalidate(nightShade);
            }
        }
    }
    void updateSettings() {
        if (screen != Screen::Settings)
            return;
        char b[80];
        if (volumeLabel) {
            snprintf(b, sizeof(b), "Volume: %d%%", audio.volume.load());
            lv_label_set_text(volumeLabel, b);
        }
        if (muteButton)
            lv_label_set_text(lv_obj_get_child(muteButton, 0),
                              audio.mute ? "Som: desligado" : "Som: ligado");
        for (int i = 0; i < 3; i++)
            if (taxLabels[i]) {
                snprintf(b, sizeof(b), "%s: %u%%",
                         i == 0   ? "Residencial"
                         : i == 1 ? "Comercial"
                                  : "Industrial",
                         city ? city->tax[i] : 9);
                lv_label_set_text(taxLabels[i], b);
            }
    }
    void persistAudio() {
        persistPreferences();
    }
    void sound(int id) {
        if (!audio.mute && assets.prefetch(id)) {
            auto s = assets.sprite(id);
            if (s)
                audio.play(assets.cached(id), s->length);
        }
    }
    void process(Event e) {
        if (e.queuedAt)
            maxCommandWait = std::max(maxCommandWait, esp_timer_get_time() - e.queuedAt);
        bool mapChanged = false;
        switch (e.action) {
        case 17:
            if (screen == Screen::Game)
                selectTool(e.a);
            break;
        case 18:
            if (screen == Screen::Game)
                selectOverlay(e.a);
            break;
        case 19:
            if (screen == Screen::Game)
                selectOverlay((overlay + 1) % 8);
            break;
        case 20:
            if (screen == Screen::Game) {
                speed = speed ? 0 : 1;
                if (!speed)
                    audio.stop();
                schedulePreferences();
            }
            break;
        case 21:
            if (screen == Screen::Game) {
                if (openPanel != Panel::None) {
                    if (openPanel == Panel::Tutorial)
                        rememberTutorial();
                    closePanel();
                } else
                    selectTool(0);
                cancelGesture();
            }
            break;
        case 22:
            if (screen == Screen::Game) {
                if (openPanel == Panel::Tools)
                    closePanel();
                else
                    showPanel(Panel::Tools);
            }
            break;
        case 23:
            if (screen == Screen::Game)
                showPanel(Panel::Help);
            break;
        case 24:
            if (screen == Screen::Game) {
                if (openPanel == Panel::Overlays)
                    closePanel();
                else
                    showPanel(Panel::Overlays);
            }
            break;
        case 25:
            if (openPanel == Panel::Tutorial)
                rememberTutorial();
            closePanel();
            break;
        case 26:
            if (screen == Screen::Game && e.a >= 0 && e.a < 4) {
                category = e.a;
                showPanel(Panel::Tools);
            }
            break;
        case 27:
            if (screen == Screen::Game) {
                tutorialPage = 0;
                showPanel(Panel::Tutorial);
            }
            break;
        case 28:
            if (screen == Screen::Game && openPanel == Panel::Tutorial) {
                tutorialPage = std::min(2, tutorialPage + 1);
                showPanel(Panel::Tutorial);
            }
            break;
        case 29:
            if (screen == Screen::Game && openPanel == Panel::Tutorial) {
                tutorialPage = std::max(0, tutorialPage - 1);
                showPanel(Panel::Tutorial);
            }
            break;
        case 30:
            if (openPanel == Panel::Tutorial) {
                rememberTutorial();
                closePanel();
            }
            break;
        case 36:
            if (screen == Screen::Game) {
                if (openPanel == Panel::Budget)
                    closePanel();
                else
                    showPanel(Panel::Budget);
            }
            break;
        case 37:
            if (screen == Screen::Game) {
                camera = {640, -1785, 1};
                cancelGesture();
                closePanel();
                schedulePreferences();
                requestRedraw(true);
                feedback = "Câmera centralizada e zoom restaurado.";
            }
            break;
        case 0:
            receiver.stop();
            if (city && assets.path.empty()) {
                feedback = "Reabra o cartão antes de voltar à cidade.";
                break;
            }
            layout(city ? Screen::Game : Screen::Menu);
            break;
        case 1:
            pendingSeed = esp_random();
            selectedSlot = 1;
            overwritePending = false;
            layout(Screen::NewCity);
            break;
        case 2:
            layout(Screen::LoadCity);
            break;
        case 31:
            if (screen == Screen::NewCity) {
                pendingSeed = esp_random();
                overwritePending = false;
                layout(Screen::NewCity);
            }
            break;
        case 32:
            if (screen == Screen::NewCity) {
                SaveSummary existing;
                if (slotSummary(selectedSlot, existing)) {
                    overwritePending = true;
                    layout(Screen::NewCity);
                } else
                    push({34});
            }
            break;
        case 33:
            if (screen == Screen::NewCity && e.a >= 1 && e.a <= 3) {
                selectedSlot = e.a;
                overwritePending = false;
                layout(Screen::NewCity);
            }
            break;
        case 34:
            if (screen == Screen::NewCity && ready()) {
                city.reset(new City);
                city->generate(pendingSeed);
                activeTime = 0;
                speed = 1;
                tool = overlay = 0;
                camera = {640, -1785, 1};
                tutorialPending = !tutorialSeen;
                schedulePreferences();
                if (save())
                    layout(Screen::Game);
                else {
                    city.reset();
                    feedback = "Não foi possível criar o slot no microSD.";
                    layout(Screen::Menu);
                }
            } else if (screen == Screen::NewCity)
                layout(Screen::Assets);
            break;
        case 35:
            if (screen == Screen::LoadCity && e.a >= 1 && e.a <= 3) {
                if (!openSavedCity(e.a))
                    layout(assets.path.empty() ? Screen::Assets : Screen::LoadCity);
            }
            break;
        case 3:
            speed = e.a;
            if (!speed)
                audio.stop();
            schedulePreferences();
            break;
        case 4: {
            auto p = camera.inverse(W / 2, MapH / 2);
            float z = std::clamp(camera.zoom + (e.a == 1 ? .25f : -.25f), .5f, 1.5f);
            mapChanged = z != camera.zoom;
            camera.zoom = z;
            auto q = camera.project(p.x, p.y);
            camera.x += W / 2 - q.x;
            camera.y += MapH / 2 - q.y;
            schedulePreferences();
            break;
        }
        case 5:
            feedback = save() && persistPreferences()
                           ? "Cidade e sessão salvas no microSD"
                           : "Falha ao salvar. Confira o SD e espaço livre.";
            break;
        case 6:
            if (city->dirty && !save()) {
                feedback = "Não foi possível salvar. A cidade continua aberta.";
                break;
            }
            audio.stop();
            persistPreferences();
            city.reset();
            assets.close();
            feedback = "Cidade salva. Até a próxima!";
            layout(Screen::Menu);
            break;
        case 7:
            layout(Screen::Settings);
            break;
        case 8:
            layout(Screen::Assets);
            break;
        case 9:
            layout(Screen::Info);
            break;
        case 10:
            if (city) {
                int i = e.a / 2;
                city->setTax(i, std::clamp(int(city->tax[i]) + (e.a % 2 ? 1 : -1), 1, 20));
            }
            updateSettings();
            break;
        case 11:
            if (city && City::inside(e.a, e.b)) {
                int i = e.b * Side + e.a;
                if (tool == 1) {
                    feedback = std::string("Célula ") + std::to_string(e.a) + "," +
                               std::to_string(e.b) + ": " + city->describe(i);
                } else {
                    Command c{e.a, e.b};
                    if (tool >= 3 && tool <= 5)
                        c.zone = Zone(tool - 2);
                    else if (tool >= 6 && tool <= 11)
                        c.building = Building(tool - 4);
                    else if (tool == 12)
                        c.demolish = true;
                    else if (tool == 13)
                        c.building = Building::PrimarySchool;
                    else if (tool == 14)
                        c.building = Building::SecondarySchool;
                    else if (tool == 15)
                        c.building = Building::University;
                    else if (tool == 16)
                        c.building = Building::LargePark;
                    else if (tool == 18)
                        c.building = Building::Port;
                    auto result = city->apply(c);
                    feedback = message(result);
                    sound(result == Result::Ok ? 200 : 201);
                    if (result == Result::Ok)
                        city->rebuild();
                    mapChanged = result == Result::Ok;
                }
            }
            break;
        case 14:
            audio.mute = !audio.mute.load();
            schedulePreferences();
            updateSettings();
            break;
        case 15:
            audio.volume = std::clamp(audio.volume.load() + (e.a ? 10 : -10), 0, 100);
            schedulePreferences();
            updateSettings();
            break;
        case 16:
            receiver.stop();
            receiveStart = 0;
            audio.stop();
            assets.close();
            bsp_sdcard_unmount();
            sd = bsp_sdcard_mount() == ESP_OK;
            if (sd) {
                mkdir(Root, 0775);
                Preferences loaded;
                if (loadPreferences(std::string(Root) + "/settings", loaded)) {
                    preferences = loaded;
                    audio.volume = preferences.volume;
                    audio.mute = preferences.mute != 0;
                    tutorialSeen = preferences.tutorialSeen != 0;
                }
                std::string path;
                bool ok = selectAssets(Root, path) && assets.open(path);
                feedback = ok ? "SD e pacote reabertos. Cidade em RAM preservada."
                              : "SD montado; instale um pacote válido.";
            } else
                feedback = "Falha ao montar SD. Cidade em RAM preservada.";
            if (notice)
                lv_label_set_text(notice, feedback.c_str());
            break;
        case 13:
            if (sd && !city) {
                assets.close();
                feedback =
                    receiver.start(Root) ? "Recepção aberta" : "Falha ao iniciar rede/servidor";
                receiveStart = esp_timer_get_time();
                lv_label_set_text(notice, feedback.c_str());
            }
            break;
        case 12:
            if (city) {
                int cost;
                auto result = city->roadRoute(e.a, e.b, e.c, e.d, true, cost,
                                              tool == 17 ? Building::Avenue : Building::Road);
                feedback = message(result);
                sound(result == Result::Ok ? 200 : 201);
                if (result == Result::Ok)
                    city->rebuild();
                mapChanged = result == Result::Ok;
            }
            break;
        case 38:
            if (city && tool >= 3 && tool <= 5) {
                int cost;
                auto result = city->zoneRect(e.a, e.b, e.c, e.d, Zone(tool - 2), true, cost);
                feedback = message(result);
                sound(result == Result::Ok ? 200 : 201);
                if (result == Result::Ok)
                    city->rebuild();
                mapChanged = result == Result::Ok;
            }
            break;
        case 39:
            if (screen == Screen::Game) {
                constructionView = !constructionView;
                feedback = constructionView
                               ? "Modo Obras ativo: prédios viraram marcadores baixos."
                               : "Modo Obras desativado: volumes dos prédios restaurados.";
                schedulePreferences();
                layout(Screen::Game);
            }
            break;
        }
        if (mapChanged)
            requestRedraw(true);
    }
    void update() {
        keyboard.poll(keys);
        if (screen == Screen::Game) {
            for (int key = 1; key < 256; key++)
                if (keys.pressed(key)) {
                    auto command = shortcut(key, keys.pressControl(key), keys.pressAlt(key));
                    if (command.action)
                        push({command.action, command.argument});
                }
        }
        keys.finish();
        while (read != write) {
            auto e = events[read];
            read = (read + 1) % events.size();
            process(e);
        }
        int64_t now = esp_timer_get_time();
        if (preferencesChanged && now - preferencesChanged >= 750000) {
            persistAudio();
        }
        if (screen == Screen::Assets && notice && receiveStart) {
            if (now - receiveStart > 600000000) {
                receiver.stop();
                receiveStart = 0;
                lv_label_set_text(notice, "Recepção encerrada após 10 minutos.");
            } else if (!receiver.accessToken().empty()) {
                char b[500];
                snprintf(b, sizeof(b),
                         "Wi-Fi: MicroCity-Tab5\nSenha: %s\nURL: http://192.168.4.1/assets\nToken: "
                         "%s\nProgresso: %d%% | %s\nVersão instalada: %lu\n\nVoltar "
                         "encerra a recepção.",
                         receiver.apPassword().c_str(), receiver.accessToken().c_str(),
                         receiver.progress.load(), receiver.statusText(),
                         (unsigned long)receiver.installed.load());
                lv_label_set_text(notice, b);
            }
        }
        if (now - lastProfile >= 10000000) {
            ESP_LOGI("profile",
                     "frames/10s=%u frame/slice/queue/tick/save us=%lld/%lld/%lld/%lld/%lld heap "
                     "internal=%u min=%u "
                     "largest=%u psram=%u min=%u largest=%u ui-stack=%u cache=%u hit/miss=%u/%u",
                     frames, (long long)maxDraw, (long long)maxSlice, (long long)maxCommandWait,
                     (long long)maxTick, (long long)maxSave,
                     unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
                     unsigned(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)),
                     unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)),
                     unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
                     unsigned(heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM)),
                     unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)),
                     unsigned(uxTaskGetStackHighWaterMark(nullptr)), unsigned(assets.bytes),
                     unsigned(assets.hits), unsigned(assets.misses));
            frames = 0;
            lastProfile = now;
        }
        int64_t previousDelta = previous;
        int64_t delta = std::clamp<int64_t>(now - previous, 0, 250000);
        previous = now;
        if (screen == Screen::Game && city) {
            if (!keys.control() && !keys.alt() && openPanel == Panel::None) {
                float amount = float(std::clamp<int64_t>(now - previousDelta, 0, 100000)) /
                               1000000.f * (keys.shift() ? 850 : 420);
                if (keys.panX() || keys.panY()) {
                    cancelGesture();
                    camera.x = std::clamp(camera.x + keys.panX() * amount, -6000.f, 7000.f);
                    camera.y = std::clamp(camera.y + keys.panY() * amount, -8500.f, 550.f);
                    schedulePreferences();
                    requestRedraw(true);
                }
            }
            // Modal panels freeze simulation without changing the selected speed.
            // `previous` is still updated above, so closing a panel never catches up old ticks.
            if (speed && openPanel == Panel::None) {
                activeTime += delta;
                int steps = simulationClock.advance(now - previousDelta, speed);
                for (int step = 0; step < steps; step++) {
                    int64_t tickStart = esp_timer_get_time();
                    int previousFires = city->metrics.fires;
                    int previousStorms = city->metrics.storms;
                    int previousQuakes = city->metrics.quakes;
                    auto previousDay = city->days;
                    city->tick();
                    if (city->metrics.fires > previousFires) {
                        feedback = "Incêndio! Bombeiros conectados ajudam a proteger o bairro.";
                        sound(202);
                    } else if (city->metrics.storms > previousStorms) {
                        feedback = "Tempestade! Serviços e construções podem sofrer danos.";
                        sound(202);
                    } else if (city->metrics.quakes > previousQuakes) {
                        feedback = "Tremor! A malha viária foi preservada; verifique os prédios.";
                        sound(202);
                    }
                    maxTick = std::max(maxTick, esp_timer_get_time() - tickStart);
                    if (previousDay != city->days) {
                        int missingPower = city->metrics.unpowered;
                        int missingWater = city->metrics.unwatered;
                        if ((missingPower != warnedPower || missingWater != warnedWater) &&
                            city->metrics.fires <= previousFires) {
                            char warning[180];
                            if (missingPower || missingWater)
                                snprintf(warning, sizeof(warning),
                                         "Déficit: %d locais sem energia e %d sem água. Confira "
                                         "conexões e capacidade.",
                                         missingPower, missingWater);
                            else if (warnedPower > 0 || warnedWater > 0)
                                snprintf(warning, sizeof(warning),
                                         "Energia e água normalizadas em todos os locais.");
                            else
                                warning[0] = 0;
                            if (warning[0])
                                feedback = warning;
                            warnedPower = missingPower;
                            warnedWater = missingWater;
                        }
                        // Never starve a slow first/full frame with periodic simulation updates.
                        // Publish it, then render the newer state on the following pass.
                        requestRedraw(false);
                    }
                }
                if (activeTime >= 120000000) {
                    activeTime = 0;
                    if (city->dirty) {
                        feedback = save() ? "Salvamento automático concluído"
                                          : "Autosave falhou; cidade preservada em RAM";
                        if (!city->dirty)
                            persistPreferences();
                        redraw = true;
                    }
                }
            }
            if (openPanel == Panel::None) {
                if (!rendering && redraw)
                    beginDraw();
                if (rendering)
                    drawStep();
                if (sceneReady && tutorialPending && !rendering &&
                    now - scenePublishedAt >= 500000) {
                    tutorialPage = 0;
                    showPanel(Panel::Tutorial);
                }
            }
            updateHud();
        }
    }
};
App *app;
} // namespace

void mc::startApplication(bool mounted) {
    app = new App;
    app->audio.init();
    app->keyboard.init();
    nvs_handle_t nvs;
    if (nvs_open("microcity", NVS_READONLY, &nvs) == ESP_OK) {
        uint8_t v = 40, m = 1, tutorial = 0;
        nvs_get_u8(nvs, "volume", &v);
        nvs_get_u8(nvs, "mute", &m);
        nvs_get_u8(nvs, "tutorial", &tutorial);
        app->audio.volume = std::min(100, int(v));
        app->audio.mute = m != 0;
        app->tutorialSeen = tutorial != 0;
        nvs_close(nvs);
    }
    app->framebuffer =
        (uint16_t *)heap_caps_malloc(W * MapH * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    app->backbuffer =
        (uint16_t *)heap_caps_malloc(W * MapH * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (app->framebuffer)
        std::fill(app->framebuffer, app->framebuffer + W * MapH, uint16_t(0x1947));
    app->sd = mounted;
    if (!mounted)
        bsp_sdcard_unmount();
    if (app->sd)
        mkdir(Root, 0775);
    app->preferences.volume = app->audio.volume.load();
    app->preferences.mute = app->audio.mute.load();
    app->preferences.tutorialSeen = app->tutorialSeen;
    if (app->sd) {
        Preferences loaded;
        if (loadPreferences(std::string(Root) + "/settings", loaded))
            app->preferences = loaded;
        else
            savePreferences(std::string(Root) + "/settings", app->preferences);
        app->audio.volume = app->preferences.volume;
        app->audio.mute = app->preferences.mute != 0;
        app->tutorialSeen = app->preferences.tutorialSeen != 0;
        app->selectedSlot = app->preferences.selectedSlot;
    }
    ESP_LOGI("microcity", "baseline internal=%u PSRAM=%u City=%u",
             unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
             unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)), unsigned(sizeof(City)));
    bsp_display_lock(0);
    if (!app->framebuffer || !app->backbuffer) {
        app->feedback = "PSRAM insuficiente para o mapa.";
    }
    app->layout(Screen::Menu);
    {
        auto timer = lv_timer_create([](lv_timer_t *) { app->update(); }, 10, nullptr);
        (void)timer;
    }
    bsp_display_unlock();
}
