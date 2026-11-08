#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
namespace mc {
constexpr int Side = 128, Count = Side * Side;
enum class Terrain : uint8_t { Land, Water, Trees };
enum class Zone : uint8_t { None, Residential, Commercial, Industrial };
enum class Building : uint8_t {
    None,
    Road,
    Power,
    Water,
    Fire,
    Police,
    Health,
    Park,
    Residence,
    Shop,
    Factory,
    PrimarySchool,
    SecondarySchool,
    University,
    LargePark,
    Avenue,
    Bridge,
    Port
};
enum Flag : uint8_t {
    Access = 1,
    Powered = 2,
    Watered = 4,
    FireCover = 8,
    PoliceCover = 16,
    HealthCover = 32,
    ParkCover = 64
};
struct Cell {
    Terrain terrain{};
    Zone zone{};
    Building building{};
    uint8_t stage = 0, distress = 0, flags = 0, roadMask = 0, pollution = 0, fire = 0;
};
static_assert(sizeof(Cell) == 9);
struct Definition {
    int cost, maintenance, capacity;
};
extern const std::array<Definition, 18> Definitions;
struct Command {
    int x, y;
    Building building = Building::None;
    Zone zone = Zone::None;
    bool demolish = false;
};
enum class Result { Ok, Bounds, Terrain, Occupied, Funds, Empty };
const char *message(Result);
struct Metrics {
    int population = 0, jobs = 0, happiness = 50, pollution = 0, revenue = 0, expenses = 0,
        powerNeed = 0, waterNeed = 0, powerCapacity = 0, waterCapacity = 0, fires = 0,
        storms = 0, quakes = 0;
    int unpowered = 0, unwatered = 0;
    int primaryCapacity = 0, secondaryCapacity = 0, higherCapacity = 0;
    std::array<int, 3> demand{{60, 0, 40}};
};
class City {
  public:
    std::array<Cell, Count> cells{};
    uint32_t seed = 1, rng = 1, ticks = 0, days = 0, saveSequence = 0;
    std::array<char, 32> name{{'N', 'o', 'v', 'a', ' ', 'P', 'a', 'l', 'u', 'd', 'o', 0}};
    int64_t money = 30000;
    std::array<uint8_t, 3> tax{{9, 9, 9}};
    Metrics metrics{};
    bool dirty = false;
    void generate(uint32_t value);
    Result apply(const Command &);
    Result roadRoute(int x0, int y0, int x1, int y1, bool commit, int &cost,
                     Building kind = Building::Road);
    Result zoneRect(int x0, int y0, int x1, int y1, Zone zone, bool commit, int &cost);
    void tick();
    bool setTax(int index, int percent);
    bool startDisaster(int index, int kind);
    void rebuild();
    uint32_t random();
    const char *blocked(int index) const;
    std::string describe(int index) const;
    static bool inside(int x, int y) {
        return x >= 0 && y >= 0 && x < Side && y < Side;
    }
    static bool isRoad(Building building) {
        return building == Building::Road || building == Building::Avenue ||
               building == Building::Bridge;
    }
    static void footprint(Building building, int &width, int &height);

  private:
    std::array<int16_t, Count> component{};
    std::array<int16_t, Count> queue{};
    std::array<int, Count> power{}, water{};
    std::array<int16_t, Count> distance{};
    bool topology = true;
    void summarize();
};
struct Point {
    float x, y;
};
struct Camera {
    float x = 0, y = 0, zoom = 1;
    Point project(float col, float row) const {
        return {(col - row) * 32 * zoom + x, (col + row) * 16 * zoom + y};
    }
    Point inverse(float sx, float sy) const {
        float a = (sx - x) / (32 * zoom), b = (sy - y) / (16 * zoom);
        return {(a + b) * .5f, (b - a) * .5f};
    }
};
} // namespace mc
