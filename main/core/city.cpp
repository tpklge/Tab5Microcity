#include "city.hpp"
#include <algorithm>
#include <cstdlib>
namespace mc {
const std::array<Definition, 18> Definitions{{{0, 0, 0},
                                              {10, 1, 0},
                                              {1200, 60, 800},
                                              {800, 40, 800},
                                              {600, 25, 12},
                                              {600, 25, 12},
                                              {800, 35, 14},
                                              {100, 3, 5},
                                              {0, 0, 8},
                                              {0, 0, 8},
                                              {0, 0, 12},
                                              {2200, 80, 400},
                                              {4200, 150, 800},
                                              {9000, 340, 1600},
                                              {650, 18, 18},
                                              {35, 2, 0},
                                              {80, 3, 0},
                                              {12000, 420, 900}}};
const char *message(Result r) {
    static const char *s[] = {"Construído",    "Fora do mapa",       "Terreno incompatível",
                              "Local ocupado", "Saldo insuficiente", "Nada para demolir"};
    return s[int(r)];
}
uint32_t City::random() {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}
void City::footprint(Building building, int &width, int &height) {
    width = height = 1;
    if (building == Building::PrimarySchool || building == Building::SecondarySchool ||
        building == Building::LargePark)
        width = height = 2;
    if (building == Building::University)
        width = height = 3;
    if (building == Building::Port) {
        width = 3;
        height = 2;
    }
}
void City::generate(uint32_t v) {
    cells.fill(Cell{});
    ticks = days = saveSequence = 0;
    money = 30000;
    tax = {9, 9, 9};
    seed = rng = v ? v : 1;
    for (int y = 0; y < Side; y++)
        for (int x = 0; x < Side; x++) {
            auto &c = cells[y * Side + x];
            auto n = random();
            c.terrain = (x > Side * 3 / 4 + (y % 9) && y > Side / 6)
                            ? Terrain::Water
                            : (n % 7 == 0 ? Terrain::Trees : Terrain::Land);
            if (x > Side / 3 && x < Side * 2 / 3 && y > Side / 3 && y < Side * 2 / 3)
                c.terrain = Terrain::Land;
        }
    dirty = true;
    rebuild();
}
Result City::apply(const Command &a) {
    if (!inside(a.x, a.y))
        return Result::Bounds;
    auto &c = cells[a.y * Side + a.x];
    int price = 0;
    if (a.demolish) {
        if (c.building == Building::None && c.zone == Zone::None && c.terrain != Terrain::Trees)
            return Result::Empty;
        price = 5;
    } else {
        if (a.building == Building::None && a.zone == Zone::None)
            return Result::Empty;
        if (unsigned(a.building) >= Definitions.size() || unsigned(a.zone) > 3 ||
            (a.zone != Zone::None && a.building != Building::None) ||
            (unsigned(a.building) >= unsigned(Building::Residence) &&
             unsigned(a.building) <= unsigned(Building::Factory)))
            return Result::Occupied;
        int width, height;
        footprint(a.building, width, height);
        if (!inside(a.x + width - 1, a.y + height - 1))
            return Result::Bounds;
        price = a.zone != Zone::None ? 10 : Definitions[int(a.building)].cost;
        for (int yy = 0; yy < height; yy++)
            for (int xx = 0; xx < width; xx++) {
                const auto &part = cells[(a.y + yy) * Side + a.x + xx];
                if (part.terrain == Terrain::Water)
                    return Result::Terrain;
                if (part.building != Building::None || part.zone != Zone::None)
                    return Result::Occupied;
                if (part.terrain == Terrain::Trees)
                    price += 5;
            }
        if (a.building == Building::Port) {
            bool waterfront = false;
            for (int yy = -1; yy <= height; yy++)
                for (int xx = -1; xx <= width; xx++)
                    if (inside(a.x + xx, a.y + yy) &&
                        cells[(a.y + yy) * Side + a.x + xx].terrain == Terrain::Water)
                        waterfront = true;
            if (!waterfront)
                return Result::Terrain;
        }
    }
    if (money < price)
        return Result::Funds;
    money -= price;
    if (a.demolish) {
        int width, height;
        footprint(c.building, width, height);
        int anchorX = a.x, anchorY = a.y;
        if (width > 1 && c.stage) {
            int part = c.stage - 1;
            anchorX -= part % width;
            anchorY -= part / width;
        }
        for (int yy = 0; yy < height; yy++)
            for (int xx = 0; xx < width; xx++) {
                auto &part = cells[(anchorY + yy) * Side + anchorX + xx];
                part.building = Building::None;
                part.zone = Zone::None;
                part.stage = part.distress = part.fire = 0;
                if (a.building != Building::Port)
                    part.terrain = Terrain::Land;
            }
    } else {
        int width, height;
        footprint(a.building, width, height);
        for (int yy = 0; yy < height; yy++)
            for (int xx = 0; xx < width; xx++) {
                auto &part = cells[(a.y + yy) * Side + a.x + xx];
                part.terrain = Terrain::Land;
                part.building = a.building;
                part.zone = a.zone;
                part.stage = width > 1 ? yy * width + xx + 1 : 0;
            }
    }
    topology = dirty = true;
    return Result::Ok;
}
Result City::roadRoute(int x0, int y0, int x1, int y1, bool commit, int &cost, Building kind) {
    cost = 0;
    if (kind != Building::Road && kind != Building::Avenue)
        return Result::Occupied;
    if (!inside(x0, y0) || !inside(x1, y1))
        return Result::Bounds;
    int x = x0, y = y0;
    for (;;) {
        auto &c = cells[y * Side + x];
        if (!isRoad(c.building)) {
            if (c.building != Building::None || c.zone != Zone::None)
                return Result::Occupied;
            auto placed = c.terrain == Terrain::Water ? Building::Bridge : kind;
            cost += Definitions[int(placed)].cost + (c.terrain == Terrain::Trees ? 5 : 0);
        }
        if (x == x1 && y == y1)
            break;
        if (x != x1)
            x += x < x1 ? 1 : -1;
        else
            y += y < y1 ? 1 : -1;
    }
    if (money < cost)
        return Result::Funds;
    if (commit) {
        money -= cost;
        x = x0;
        y = y0;
        for (;;) {
            auto &c = cells[y * Side + x];
            if (!isRoad(c.building)) {
                c.building = c.terrain == Terrain::Water ? Building::Bridge : kind;
                if (c.building != Building::Bridge)
                    c.terrain = Terrain::Land;
            }
            if (x == x1 && y == y1)
                break;
            if (x != x1)
                x += x < x1 ? 1 : -1;
            else
                y += y < y1 ? 1 : -1;
        }
        topology = dirty = true;
    }
    return Result::Ok;
}
Result City::zoneRect(int x0, int y0, int x1, int y1, Zone zone, bool commit, int &cost) {
    cost = 0;
    if (zone == Zone::None || !inside(x0, y0) || !inside(x1, y1))
        return Result::Bounds;
    int left = std::min(x0, x1), right = std::max(x0, x1);
    int top = std::min(y0, y1), bottom = std::max(y0, y1);
    for (int y = top; y <= bottom; y++)
        for (int x = left; x <= right; x++) {
            const auto &c = cells[y * Side + x];
            if (c.terrain == Terrain::Water)
                return Result::Terrain;
            if (c.building != Building::None || c.zone != Zone::None)
                return Result::Occupied;
            cost += 10 + (c.terrain == Terrain::Trees ? 5 : 0);
        }
    if (money < cost)
        return Result::Funds;
    if (commit) {
        money -= cost;
        for (int y = top; y <= bottom; y++)
            for (int x = left; x <= right; x++) {
                auto &c = cells[y * Side + x];
                c.terrain = Terrain::Land;
                c.zone = zone;
            }
        topology = dirty = true;
    }
    return Result::Ok;
}
static constexpr int dx[] = {0, 1, 0, -1}, dy[] = {-1, 0, 1, 0};
void City::rebuild() {
    component.fill(-1);
    for (auto &c : cells) {
        c.flags = c.roadMask = c.pollution = 0;
    }
    int networks = 0;
    for (int i = 0; i < Count; i++)
        if (isRoad(cells[i].building) && component[i] < 0) {
            int head = 0, tail = 0;
            queue[tail++] = i;
            component[i] = networks;
            while (head < tail) {
                int p = queue[head++], x = p % Side, y = p / Side;
                for (int d = 0; d < 4; d++)
                    if (inside(x + dx[d], y + dy[d])) {
                        int n = p + dy[d] * Side + dx[d];
                        if (isRoad(cells[n].building)) {
                            cells[p].roadMask |= 1 << d;
                            if (component[n] < 0) {
                                component[n] = networks;
                                queue[tail++] = n;
                            }
                        }
                    }
            }
            networks++;
        }
    // Lots are served up to two cells from a road. Chebyshev distance includes
    // 90-degree corners and the complete frontage of multi-cell buildings.
    for (int i = 0; i < Count; i++)
        if (!isRoad(cells[i].building)) {
            int x = i % Side, y = i / Side;
            for (int oy = -2; oy <= 2; oy++)
                for (int ox = -2; ox <= 2; ox++)
                    if ((ox || oy) && inside(x + ox, y + oy)) {
                        int n = (y + oy) * Side + x + ox;
                        if (isRoad(cells[n].building) &&
                            (component[i] < 0 || component[n] < component[i]))
                            component[i] = component[n];
                    }
        }
    // Every tile of a multi-cell service shares the best road connection of its footprint.
    for (int i = 0; i < Count; i++) {
        auto b = cells[i].building;
        int width, height;
        footprint(b, width, height);
        if (width == 1 || cells[i].stage != 1)
            continue;
        int best = -1, ax = i % Side, ay = i / Side;
        for (int yy = 0; yy < height; yy++)
            for (int xx = 0; xx < width; xx++) {
                int net = component[(ay + yy) * Side + ax + xx];
                if (net >= 0 && (best < 0 || net < best))
                    best = net;
            }
        for (int yy = 0; yy < height; yy++)
            for (int xx = 0; xx < width; xx++)
                component[(ay + yy) * Side + ax + xx] = best;
    }
    power.fill(0);
    water.fill(0);
    for (int i = 0; i < Count; i++)
        if (component[i] >= 0) {
            auto b = cells[i].building;
            if (b == Building::Power)
                power[component[i]] += 800;
            if (b == Building::Water)
                water[component[i]] += 800;
        }
    for (int i = 0; i < Count; i++) {
        auto &c = cells[i];
        int net = component[i];
        if (net < 0)
            continue;
        c.flags |= Access;
        int width, height;
        footprint(c.building, width, height);
        bool anchor = width == 1 || c.stage == 1;
        int need = (c.zone != Zone::None || (int(c.building) >= 2 && anchor))
                       ? (c.zone != Zone::None ? std::max(1, int(c.stage)) : 1) * 4
                       : 0;
        if (power[net] > 0 && power[net] >= need) {
            c.flags |= Powered;
            power[net] -= need;
        }
        if (water[net] > 0 && water[net] >= need) {
            c.flags |= Watered;
            water[net] -= need;
        }
    }
    // A multi-cell building is one consumer: every reserved tile displays the
    // access/utilities state decided for its anchor.
    for (int i = 0; i < Count; i++) {
        int width, height;
        footprint(cells[i].building, width, height);
        if (width == 1 || cells[i].stage != 1)
            continue;
        int shared = cells[i].flags & (Access | Powered | Watered);
        int ax = i % Side, ay = i / Side;
        for (int yy = 0; yy < height; yy++)
            for (int xx = 0; xx < width; xx++)
                cells[(ay + yy) * Side + ax + xx].flags |= shared;
    }
    // Multi-source bounded road BFS for each service; parks use Manhattan radius.
    for (int kind = 4; kind <= 6; kind++) {
        auto &dist = distance;
        dist.fill(-1);
        int h = 0, t = 0;
        for (int i = 0; i < Count; i++)
            if (int(cells[i].building) == kind && (cells[i].flags & 7) == 7) {
                for (int d = 0; d < 4; d++) {
                    int x = i % Side + dx[d], y = i / Side + dy[d];
                    if (inside(x, y)) {
                        int n = y * Side + x;
                if (isRoad(cells[n].building) && dist[n] < 0) {
                            dist[n] = 0;
                            queue[t++] = n;
                        }
                    }
                }
            }
        while (h < t) {
            int p = queue[h++];
            cells[p].flags |= 1 << (kind - 1);
            for (int d = 0; d < 4; d++) {
                int x = p % Side + dx[d], y = p / Side + dy[d];
                if (!inside(x, y))
                    continue;
                int n = y * Side + x;
                cells[n].flags |= 1 << (kind - 1);
                if (isRoad(cells[n].building) && dist[n] < 0 && dist[p] < 12) {
                    dist[n] = dist[p] + 1;
                    queue[t++] = n;
                }
            }
        }
    }
    for (int i = 0; i < Count; i++)
        if (cells[i].building == Building::Park || cells[i].building == Building::LargePark ||
            cells[i].building == Building::Factory) {
            bool park = cells[i].building == Building::Park || cells[i].building == Building::LargePark;
            if (cells[i].building == Building::LargePark && cells[i].stage != 1)
                continue;
            int radius = cells[i].building == Building::LargePark ? 9 : 5;
            for (int y = std::max(0, i / Side - radius); y <= std::min(Side - 1, i / Side + radius); y++)
                for (int x = std::max(0, i % Side - radius); x <= std::min(Side - 1, i % Side + radius);
                     x++) {
                    int d = abs(x - i % Side) + abs(y - i / Side);
                    if (d <= radius) {
                        if (park)
                            cells[y * Side + x].flags |= ParkCover;
                        else
                            cells[y * Side + x].pollution =
                                std::min(100, int(cells[y * Side + x].pollution) + (6 - d) * 8);
                    }
                }
        }
    topology = false;
    summarize();
}
void City::summarize() {
    metrics = Metrics{};
    int happy = 0, developed = 0, commercialJobs = 0, industrialJobs = 0, portRevenue = 0;
    for (auto &c : cells) {
        if (c.fire && c.fire <= 3)
            metrics.fires++;
        else if (c.fire && c.fire <= 6)
            metrics.storms++;
        else if (c.fire)
            metrics.quakes++;
        int stage = c.stage;
        if (c.building == Building::Residence) {
            metrics.population += stage * 8;
            happy += 30 + ((c.flags & 7) == 7 ? 30 : 0) + ((c.flags & FireCover) ? 8 : 0) +
                     ((c.flags & PoliceCover) ? 8 : 0) + ((c.flags & HealthCover) ? 8 : 0) +
                     ((c.flags & ParkCover) ? 12 : 0) - c.pollution / 3;
            developed++;
        }
        if (c.building == Building::Shop) {
            metrics.jobs += stage * 8;
            commercialJobs += stage * 8;
        }
        if (c.building == Building::Factory) {
            metrics.jobs += stage * 12;
            industrialJobs += stage * 12;
        }
        metrics.pollution += c.pollution;
        int width, height;
        footprint(c.building, width, height);
        bool anchor = width == 1 || stage == 1;
        if (anchor && c.building == Building::Port && (c.flags & 7) == 7) {
            metrics.jobs += 120;
            commercialJobs += 40;
            industrialJobs += 80;
            portRevenue += 600;
        }
        if (anchor)
            metrics.expenses += Definitions[int(c.building)].maintenance;
        int need = (c.zone != Zone::None || (int(c.building) >= 2 && anchor))
                       ? (c.zone != Zone::None ? std::max(1, stage) : 1) * 4
                       : 0;
        metrics.powerNeed += need;
        metrics.waterNeed += need;
        if (need && !(c.flags & Powered))
            metrics.unpowered++;
        if (need && !(c.flags & Watered))
            metrics.unwatered++;
        if (c.building == Building::Power && component[&c - cells.data()] >= 0)
            metrics.powerCapacity += Definitions[int(Building::Power)].capacity;
        if (c.building == Building::Water && component[&c - cells.data()] >= 0)
            metrics.waterCapacity += Definitions[int(Building::Water)].capacity;
        if (anchor && component[&c - cells.data()] >= 0 && (c.flags & 7) == 7) {
            if (c.building == Building::PrimarySchool)
                metrics.primaryCapacity += Definitions[int(c.building)].capacity;
            if (c.building == Building::SecondarySchool)
                metrics.secondaryCapacity += Definitions[int(c.building)].capacity;
            if (c.building == Building::University)
                metrics.higherCapacity += Definitions[int(c.building)].capacity;
        }
    }
    int educationBonus = (metrics.primaryCapacity > 0 ? 4 : 0) +
                         (metrics.secondaryCapacity > 0 ? 4 : 0) +
                         (metrics.higherCapacity > 0 ? 4 : 0);
    metrics.happiness = developed ? std::clamp(happy / developed + educationBonus, 0, 100) : 60;
    metrics.pollution /= Count;
    metrics.demand[0] = std::clamp(50 + std::clamp((metrics.jobs - metrics.population / 2) / 4,
                                                   -35, 35) -
                                           (int(tax[0]) - 9) * 4,
                                       5, 95);
    metrics.demand[1] = std::clamp(25 + std::min(50, metrics.population / 8) - commercialJobs / 8 -
                                           (int(tax[1]) - 9) * 4,
                                       5, 95);
    metrics.demand[2] = std::clamp(45 + std::min(35, metrics.population / 12) - industrialJobs / 10 -
                                           (int(tax[2]) - 9) * 4,
                                       5, 95);
    metrics.revenue = metrics.population * tax[0] / 2 + portRevenue;
    for (auto &c : cells) {
        if (c.building == Building::Shop)
            metrics.revenue += c.stage * 8 * tax[1] / 2;
        if (c.building == Building::Factory)
            metrics.revenue += c.stage * 12 * tax[2] / 2;
    }
}
const char *City::blocked(int i) const {
    const auto &c = cells[i];
    if (!(c.flags & Access))
        return "Sem acesso viário";
    if (!(c.flags & Powered))
        return "Sem energia";
    if (!(c.flags & Watered))
        return "Sem água";
    if (c.zone != Zone::None && metrics.demand[int(c.zone) - 1] < 20)
        return "Baixa demanda";
    if (c.fire)
        return c.fire <= 3 ? "Incêndio em andamento"
             : c.fire <= 6 ? "Tempestade em andamento"
                           : "Tremor em andamento";
    if (c.pollution > 60)
        return "Poluição elevada";
    return "Condições adequadas";
}
std::string City::describe(int i) const {
    if (i < 0 || i >= Count)
        return "Fora do mapa";
    const auto &c = cells[i];
    static const char *terrain[] = {"terra", "água", "vegetação"};
    static const char *zones[] = {"sem zona", "residencial", "comercial", "industrial"};
    static const char *buildings[] = {"vazio",      "estrada",  "usina",    "estação de água",
                                      "bombeiros",  "polícia",  "saúde",    "parque",
                                      "residência", "comércio", "indústria", "escola primária",
                                      "escola secundária", "universidade", "parque municipal",
                                      "avenida", "ponte", "porto"};
    std::string out = buildings[int(c.building)];
    out += " · ";
    out += zones[int(c.zone)];
    if (c.stage) {
        out += " · nível ";
        out += char('0' + c.stage);
    }
    out += " · ";
    out += blocked(i);
    out += "\nTerreno: ";
    out += terrain[int(c.terrain)];
    out += " · poluição ";
    out += std::to_string(c.pollution);
    out += " · serviços ";
    out += (c.flags & FireCover) ? "B" : "-";
    out += (c.flags & PoliceCover) ? "P" : "-";
    out += (c.flags & HealthCover) ? "S" : "-";
    out += (c.flags & ParkCover) ? "V" : "-";
    return out;
}
bool City::setTax(int index, int percent) {
    if (index < 0 || index >= 3 || percent < 1 || percent > 20)
        return false;
    tax[index] = percent;
    dirty = true;
    summarize();
    return true;
}
bool City::startDisaster(int index, int kind) {
    if (index < 0 || index >= Count || kind < 1 || kind > 3)
        return false;
    auto &c = cells[index];
    if (isRoad(c.building) || c.building == Building::None || c.fire)
        return false;
    int width, height;
    footprint(c.building, width, height);
    if (width > 1 && c.stage != 1)
        return false;
    c.fire = kind * 3;
    dirty = true;
    summarize();
    return true;
}
void City::tick() {
    if (topology)
        rebuild();
    ticks++;
    dirty = true;
    if (ticks % 8)
        return;
    days++;
    int ignition = random() % Count;
    auto &target = cells[ignition];
    if (int(target.building) >= int(Building::Residence) &&
        int(target.building) <= int(Building::Factory) && target.stage &&
        !(target.flags & FireCover) &&
        random() % 1000 < unsigned(2 + target.pollution / 4))
        target.fire = 3;
    if (days % 30 == 1 && random() % 100 < 35) {
        int kind = 1 + random() % 3, start = random() % Count;
        for (int n = 0; n < Count; n++)
            if (startDisaster((start + n) % Count, kind))
                break;
    }
    for (int i = 0; i < Count; i++) {
        auto &c = cells[i];
        if (!c.fire)
            continue;
        int kind = (c.fire - 1) / 3 + 1;
        int terminal = (kind - 1) * 3 + 1;
        if (kind == 1 && (c.flags & FireCover)) {
            c.fire = 0;
            continue;
        }
        if (c.fire > terminal) {
            c.fire--;
            continue;
        }
        int width, height;
        footprint(c.building, width, height);
        bool service = int(c.building) < int(Building::Residence) ||
                       int(c.building) > int(Building::Factory);
        bool destroy = kind == 3 || service;
        if (!destroy && c.stage > 1) {
            c.stage--;
            c.fire = 0;
        } else if (width > 1 && c.stage == 1) {
            int ax = i % Side, ay = i / Side;
            for (int yy = 0; yy < height; yy++)
                for (int xx = 0; xx < width; xx++) {
                    auto &part = cells[(ay + yy) * Side + ax + xx];
                    part.building = Building::None;
                    part.zone = Zone::None;
                    part.stage = part.distress = part.fire = 0;
                }
        } else {
            c.building = Building::None;
            c.stage = c.distress = c.fire = 0;
        }
        topology = true;
    }
    for (int i = 0; i < Count; i++) {
        auto &c = cells[i];
        if (c.zone == Zone::None)
            continue;
        bool valid = (c.flags & 7) == 7 && metrics.demand[int(c.zone) - 1] >= 20 &&
                     c.pollution < 80 && !c.fire &&
                     (c.zone != Zone::Residential || metrics.happiness >= 30);
        if (valid) {
            c.distress = 0;
            if (days % 5 == 0 && c.stage < 3) {
                c.stage++;
                c.building = c.zone == Zone::Residential  ? Building::Residence
                             : c.zone == Zone::Commercial ? Building::Shop
                                                          : Building::Factory;
                topology = true;
            }
        } else if (c.stage && ++c.distress >= 30) {
            c.stage--;
            c.distress = 0;
            if (!c.stage)
                c.building = Building::None;
            topology = true;
        }
    }
    if (topology)
        rebuild();
    else
        summarize();
    if (days % 30 == 0)
        money = std::clamp<int64_t>(money + metrics.revenue - metrics.expenses, -1000000000LL,
                                    1000000000LL);
}
} // namespace mc
