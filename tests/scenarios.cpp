#include "core/city.hpp"
#include "storage/save.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace mc;
static void road(City &c, int x0, int y0, int x1, int y1) {
    int cost;
    assert(c.roadRoute(x0, y0, x1, y1, true, cost) == Result::Ok);
}
static void place(City &c, int x, int y, Building b) {
    assert(c.apply({x, y, b}) == Result::Ok);
}
static void zone(City &c, int x, int y, Zone z) {
    assert(c.apply({x, y, Building::None, z}) == Result::Ok);
}
static void exportCity(const char *name, City &c) {
    c.rebuild();
    auto data = encode(c);
    std::ofstream f(std::string("tests/fixtures/") + name + ".sav", std::ios::binary);
    f.write((const char *)data.data(), data.size());
    std::cout << name << ": population=" << c.metrics.population << " jobs=" << c.metrics.jobs
              << " revenue=" << c.metrics.revenue << " expenses=" << c.metrics.expenses
              << " money=" << c.money << "\n";
}
int main() {
    std::filesystem::create_directories("tests/fixtures");
    City c;
    c.generate(51);
    for (auto &cell : c.cells)
        cell.terrain = Terrain::Land;
    road(c, 20, 30, 40, 30);
    place(c, 20, 29, Building::Power);
    place(c, 21, 29, Building::Water);
    place(c, 22, 29, Building::Fire);
    place(c, 23, 29, Building::Police);
    place(c, 24, 29, Building::Health);
    place(c, 25, 29, Building::Park);
    for (int x = 26; x < 34; x++)
        zone(c, x, 29, Zone::Residential);
    for (int x = 34; x < 40; x++)
        zone(c, x, 29, Zone::Industrial);
    for (int x = 24; x < 30; x++)
        zone(c, x, 31, Zone::Commercial);
    for (int i = 0; i < 8 * 120; i++)
        c.tick();
    assert(c.metrics.population > 0 && c.metrics.jobs > 0 &&
           c.metrics.revenue > c.metrics.expenses);
    exportCity("pequena-equilibrada", c);
    road(c, 20, 30, 20, 42);
    road(c, 20, 42, 40, 42);
    for (int x = 21; x < 40; x++) {
        zone(c, x, 41, x < 34 ? Zone::Residential : Zone::Commercial);
        zone(c, x, 43, Zone::Industrial);
    }
    for (int i = 0; i < 8 * 90; i++)
        c.tick();
    exportCity("media-bairros", c);
    assert(c.apply({21, 29, Building::None, Zone::None, true}) == Result::Ok);
    c.money = -1500;
    for (int i = 0; i < 8 * 35; i++)
        c.tick();
    assert(c.metrics.population < 8 * 3 * 21);
    exportCity("deficit-recuperavel", c);
}
