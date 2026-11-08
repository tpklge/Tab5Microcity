#include "core/city.hpp"
#include "core/clock.hpp"
#include "storage/save.hpp"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
using namespace mc;
static void plain(City &c) {
    c.generate(51);
    for (auto &a : c.cells)
        a.terrain = Terrain::Land;
    c.rebuild();
}
static void build(City &c, int x, int y, Building b) {
    assert(c.apply({x, y, b}) == Result::Ok);
}
int main() {
    for (int speed : {1, 2, 4}) {
        Clock clock;
        int ticks = 0;
        for (int frame = 0; frame < 40 / speed; frame++)
            ticks += clock.advance(50000, speed);
        assert(ticks == 8);
        assert(clock.advance(60000000, 0) == 0);
        assert(clock.advance(60000000, 4) <= 4);
    }
    City a, b;
    plain(a);
    plain(b);
    assert(encode(a) == encode(b));
    for (float z : {.5f, 1.f, 1.5f}) {
        Camera cam{-137, 58, z};
        for (int y = 0; y < Side; y++)
            for (int x = 0; x < Side; x++) {
                auto p = cam.project(x, y);
                auto q = cam.inverse(p.x, p.y);
                assert(std::abs(q.x - x) < .001f && std::abs(q.y - y) < .001f);
                for (float v : {-.49f, .49f}) {
                    p = cam.project(x + v, y + v);
                    q = cam.inverse(p.x, p.y);
                    assert(int(std::floor(q.x + .5f)) == x && int(std::floor(q.y + .5f)) == y);
                }
            }
    }
    auto unchanged = encode(a);
    assert(!a.setTax(3, 9) && !a.setTax(0, 21) && !a.setTax(0, 0));
    assert(encode(a) == unchanged);
    assert(a.setTax(0, 12));
    assert(a.tax[0] == 12);
    assert(a.setTax(0, 9));
    auto snapshot = encode(a);
    assert(a.apply({-1, 0, Building::Road}) == Result::Bounds);
    assert(encode(a) == snapshot);
    a.money = 0;
    snapshot = encode(a);
    assert(a.apply({20, 20, Building::Power}) == Result::Funds);
    assert(encode(a) == snapshot);
    a.money = 30000;
    int cost;
    build(a, 23, 20, Building::Power);
    snapshot = encode(a);
    assert(a.roadRoute(20, 20, 25, 20, true, cost) == Result::Occupied);
    assert(encode(a) == snapshot);
    plain(a);
    assert(a.roadRoute(20, 20, 30, 20, true, cost) == Result::Ok && cost == 110);
    assert(a.roadRoute(20, 20, 30, 20, true, cost) == Result::Ok && cost == 0);
    build(a, 20, 19, Building::Power);
    build(a, 21, 19, Building::Water);
    assert(a.apply({29, 19, Building::None, Zone::Residential}) == Result::Ok);
    a.rebuild();
    assert((a.cells[19 * Side + 29].flags & 7) == 7);
    assert(a.metrics.powerCapacity == 800 && a.metrics.waterCapacity == 800);
    assert(a.metrics.powerNeed == 12 && a.metrics.waterNeed == 12);
    assert(a.metrics.unpowered == 0 && a.metrics.unwatered == 0);
    plain(b);
    assert(b.zoneRect(10, 10, 12, 11, Zone::Residential, false, cost) == Result::Ok && cost == 60);
    assert(b.zoneRect(10, 10, 12, 11, Zone::Residential, true, cost) == Result::Ok);
    for (int y = 10; y <= 11; y++)
        for (int x = 10; x <= 12; x++)
            assert(b.cells[y * Side + x].zone == Zone::Residential);
    auto zoned = encode(b);
    assert(b.zoneRect(9, 9, 10, 10, Zone::Commercial, true, cost) == Result::Occupied);
    assert(encode(b) == zoned);
    plain(b);
    assert(b.apply({20, 20, Building::PrimarySchool}) == Result::Ok);
    for (int y = 20; y < 22; y++)
        for (int x = 20; x < 22; x++)
            assert(b.cells[y * Side + x].building == Building::PrimarySchool);
    assert(b.apply({21, 21, Building::None, Zone::None, true}) == Result::Ok);
    for (int y = 20; y < 22; y++)
        for (int x = 20; x < 22; x++)
            assert(b.cells[y * Side + x].building == Building::None);
    assert(b.apply({Side - 2, Side - 2, Building::University}) == Result::Bounds);
    assert(b.roadRoute(5, 5, 8, 5, true, cost, Building::Avenue) == Result::Ok && cost == 140);
    assert(b.cells[5 * Side + 6].building == Building::Avenue);
    // Routes automatically become bridges on water and retain road connectivity.
    b.cells[70 * Side + 70].terrain = Terrain::Water;
    assert(b.roadRoute(69, 70, 71, 70, true, cost) == Result::Ok);
    b.rebuild();
    assert(b.cells[70 * Side + 70].building == Building::Bridge);
    assert(b.cells[70 * Side + 70].terrain == Terrain::Water);
    assert(b.cells[70 * Side + 70].roadMask == 10);
    // Radius two includes diagonal corners; the complete 3x3 footprint shares service.
    plain(b);
    assert(b.roadRoute(40, 48, 48, 48, true, cost) == Result::Ok);
    build(b, 40, 47, Building::Power);
    build(b, 41, 47, Building::Water);
    build(b, 50, 50, Building::University);
    b.rebuild();
    for (int y = 50; y < 53; y++)
        for (int x = 50; x < 53; x++)
            assert((b.cells[y * Side + x].flags & 7) == 7);
    // A port occupies 3x2 land cells, requires a waterfront and contributes only when served.
    plain(b);
    b.cells[60 * Side + 63].terrain = Terrain::Water;
    assert(b.apply({60, 60, Building::Port}) == Result::Ok);
    assert(b.cells[60 * Side + 60].stage == 1 && b.cells[61 * Side + 62].stage == 6);
    assert(b.apply({80, 80, Building::Port}) == Result::Terrain);
    assert(!(b.metrics.demand[0] == b.metrics.demand[1] &&
             b.metrics.demand[1] == b.metrics.demand[2]));
    // Disasters target buildings and services, never the road network.
    plain(b);
    build(b, 20, 20, Building::Road);
    build(b, 20, 19, Building::Power);
    assert(!b.startDisaster(20 * Side + 20, 3));
    assert(b.startDisaster(19 * Side + 20, 3));
    for (int i = 0; i < 24; i++)
        b.tick();
    assert(b.cells[20 * Side + 20].building == Building::Road);
    assert(b.cells[19 * Side + 20].building == Building::None);
    assert(b.metrics.quakes == 0);
    auto modern = encode(b);
    auto modernCopy = new City;
    assert(modern[4] == 5 && decode(modern, *modernCopy));
    assert(encode(*modernCopy) == modern);
    delete modernCopy;
    auto description = a.describe(19 * Side + 29);
    assert(description.find("residencial") != std::string::npos);
    assert(description.find("Condições adequadas") != std::string::npos);
    assert(a.describe(-1) == "Fora do mapa");
    assert(a.cells[20 * Side + 25].roadMask == 10);
    assert(a.apply({25, 20, Building::None, Zone::None, true}) == Result::Ok);
    a.rebuild();
    assert(!(a.cells[19 * Side + 29].flags & Powered));
    assert(!(a.cells[19 * Side + 29].flags & Watered));
    assert(a.metrics.unpowered > 0 && a.metrics.unwatered > 0);
    assert(a.roadRoute(25, 20, 25, 20, true, cost) == Result::Ok);
    a.rebuild();
    for (int i = 0; i < 40; i++)
        a.tick();
    assert(a.metrics.population > 0);
    auto bytes = encode(a);
    assert(decode(bytes, b));
    assert(encode(a) == encode(b));
    for (int i = 0; i < 500; i++) {
        a.tick();
        b.tick();
    }
    assert(encode(a) == encode(b));
    bytes = encode(a);
    auto intact = encode(b);
    bytes[100] ^= 1;
    assert(!decode(bytes, b));
    assert(encode(b) == intact);
    bytes.pop_back();
    assert(!decode(bytes, b));
    plain(a);
    build(a, 20, 20, Building::Road);
    int64_t initial = a.money;
    for (int i = 0; i < 239; i++)
        a.tick();
    assert(a.money == initial);
    a.tick();
    assert(a.money == initial - 1);
    for (int i = 0; i < 8; i++)
        a.tick();
    assert(a.money == initial - 1);
    auto temp = std::filesystem::temp_directory_path() / "microcity-tests";
    std::filesystem::remove_all(temp);
    std::filesystem::create_directories(temp);
    assert(saveCity(temp.string(), a));
    SaveSummary summary;
    assert(inspectCity(temp.string(), summary));
    assert(summary.seed == a.seed && summary.money == a.money &&
           summary.population == a.metrics.population && summary.happiness == a.metrics.happiness &&
           summary.unpowered == a.metrics.unpowered && summary.unwatered == a.metrics.unwatered &&
           summary.sequence == a.saveSequence);
    a.tick();
    assert(saveCity(temp.string(), a));
    assert(loadCity(temp.string(), b));
    assert(encode(a) == encode(b));
    plain(a);
    a.money = 12345;
    assert(saveCity(temp.string(), a));
    assert(loadCity(temp.string(), b));
    assert(b.money == 12345);
    assert(encode(a) == encode(b));
    std::filesystem::remove_all(temp);
    std::cout << "City rules, rollback, networks, growth, economy, determinism, save corruption "
                 "and isometric picking: PASS\nsizeof(Cell)="
              << sizeof(Cell) << " sizeof(City)=" << sizeof(City) << " save=" << encode(a).size()
              << " bytes\n";
}
