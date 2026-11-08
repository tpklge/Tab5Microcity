#include "save.hpp"
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
namespace mc {
uint32_t crc32(const uint8_t *p, size_t n) {
    uint32_t c = ~0U;
    while (n--) {
        c ^= *p++;
        for (int b = 0; b < 8; b++)
            c = (c >> 1) ^ ((0U - (c & 1)) & 0xedb88320U);
    }
    return ~c;
}
static void put(std::vector<uint8_t> &b, uint64_t v, int n = 4) {
    while (n--) {
        b.push_back(v & 255);
        v >>= 8;
    }
}
static uint64_t get(const std::vector<uint8_t> &b, size_t &p, int n = 4) {
    uint64_t v = 0;
    for (int i = 0; i < n; i++)
        v |= uint64_t(b[p++]) << (8 * i);
    return v;
}
std::vector<uint8_t> encode(const City &c) {
    std::vector<uint8_t> b;
    b.reserve(75 + Count * 6 + 4);
    put(b, 0x5653434d);
    put(b, 5);
    put(b, Side);
    put(b, c.seed);
    put(b, c.rng);
    put(b, c.ticks);
    put(b, c.days);
    put(b, uint64_t(c.money), 8);
    for (auto t : c.tax)
        put(b, t, 1);
    put(b, c.saveSequence);
    for (auto ch : c.name)
        put(b, uint8_t(ch), 1);
    for (auto &a : c.cells) {
        put(b, unsigned(a.terrain), 1);
        put(b, unsigned(a.zone), 1);
        put(b, unsigned(a.building), 1);
        put(b, a.stage, 1);
        put(b, a.distress, 1);
        put(b, a.fire, 1);
    }
    put(b, crc32(b.data(), b.size()));
    return b;
}
bool decode(const std::vector<uint8_t> &b, City &c) {
    if (b.size() < 79)
        return false;
    size_t headerAt = 0;
    if (get(b, headerAt) != 0x5653434d)
        return false;
    auto version = get(b, headerAt), dimension = get(b, headerAt);
    if ((version < 2 || version > 5) || (dimension != 64 && dimension != Side))
        return false;
    size_t N = 75 + size_t(dimension) * dimension * 6 + 4;
    if (b.size() != N)
        return false;
    size_t end = N - 4;
    auto stored = get(b, end);
    if (stored != crc32(b.data(), N - 4))
        return false;
    size_t p = 0;
    get(b, p);
    get(b, p);
    get(b, p);
    // Validate the complete payload before changing the live city.
    for (size_t i = 75; i < N - 4; i += 6) {
        int t = b[i], z = b[i + 1], v = b[i + 2], s = b[i + 3];
        int maxBuilding = version == 2 ? 10 : version == 3 ? 15 : 17;
        int maxStage = v >= int(Building::PrimarySchool) && v <= int(Building::LargePark)
                           ? 9
                           : v == int(Building::Port) ? 6 : 3;
        if (t > 2 || z > 3 || v > maxBuilding || s > maxStage || b[i + 4] > 30 ||
            b[i + 5] > (version >= 5 ? 9 : 3))
            return false;
        if (t == 1 && (z || (v && v != int(Building::Bridge))))
            return false;
        if (v >= int(Building::Residence) && v <= int(Building::Factory) &&
            (z != v - 7 || !s))
            return false;
        if (z && v > 0 && v < 8)
            return false;
        if (t == 2 && (z || v))
            return false;
        if (!s && b[i + 5] &&
            (version < 5 || v == 0 || v == int(Building::Road) || v == int(Building::Avenue) ||
             v == int(Building::Bridge)))
            return false;
        bool footprintPart = (v >= int(Building::PrimarySchool) && v <= int(Building::LargePark)) ||
                             v == int(Building::Port);
        if (v < 8 && s)
            return false;
        if (footprintPart && !s)
            return false;
        if ((v == int(Building::Avenue) || v == int(Building::Bridge)) && s)
            return false;
    }
    for (int j = 0; j < 3; j++)
        if (b[36 + j] < 1 || b[36 + j] > 20)
            return false;
    if (b[74] != 0)
        return false;
    size_t cashOffset = 28;
    auto cash = int64_t(get(b, cashOffset, 8));
    if (cash < -1000000000LL || cash > 1000000000LL)
        return false;
    auto seed = uint32_t(get(b, p)), rng = uint32_t(get(b, p)), ticks = uint32_t(get(b, p)),
         days = uint32_t(get(b, p));
    auto money = int64_t(get(b, p, 8));
    if (dimension != Side)
        c.generate(seed);
    else
        c.cells.fill(Cell{});
    c.seed = seed;
    c.rng = rng;
    c.ticks = ticks;
    c.days = days;
    c.money = money;
    for (auto &t : c.tax)
        t = get(b, p, 1);
    c.saveSequence = get(b, p);
    for (auto &ch : c.name)
        ch = char(get(b, p, 1));
    for (int y = 0; y < int(dimension); y++)
        for (int x = 0; x < int(dimension); x++) {
            auto &a = c.cells[y * Side + x];
            a = {};
            a.terrain = Terrain(get(b, p, 1));
            a.zone = Zone(get(b, p, 1));
            a.building = Building(get(b, p, 1));
            a.stage = get(b, p, 1);
            a.distress = get(b, p, 1);
            a.fire = get(b, p, 1);
        }
    c.rebuild();
    c.dirty = false;
    return true;
}
static bool read(const std::string &path, std::vector<uint8_t> &b) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return false;
    if (fseek(f, 0, SEEK_END)) {
        fclose(f);
        return false;
    }
    auto length = ftell(f);
    size_t oldSize = 75 + 64 * 64 * 6 + 4, currentSize = 75 + Count * 6 + 4;
    if (length != long(oldSize) && length != long(currentSize)) {
        fclose(f);
        return false;
    }
    rewind(f);
    b.resize(length);
    bool ok = fread(b.data(), 1, b.size(), f) == b.size();
    fclose(f);
    return ok;
}
bool saveCity(const std::string &root, City &c) {
    mkdir(root.c_str(), 0775);
    std::vector<uint8_t> a, b;
    City *scratch = new City;
    bool av = read(root + "/city-a.sav", a) && decode(a, *scratch),
         bv = read(root + "/city-b.sav", b) && decode(b, *scratch);
    uint32_t at = 0, bt = 0;
    if (av) {
        size_t p = 39;
        at = get(a, p);
    }
    if (bv) {
        size_t p = 39;
        bt = get(b, p);
    }
    std::string target = root + (!av || (bv && at <= bt) ? "/city-a.sav" : "/city-b.sav");
    delete scratch;
    uint32_t previousSequence = c.saveSequence;
    c.saveSequence = std::max(c.saveSequence, std::max(at, bt));
    if (c.saveSequence == UINT32_MAX)
        return false;
    c.saveSequence++;
    auto data = encode(c);
    FILE *f = fopen(target.c_str(), "wb");
    if (!f) {
        c.saveSequence = previousSequence;
        return false;
    }
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    if (fflush(f))
        ok = false;
    if (fsync(fileno(f)))
        ok = false;
    if (fclose(f))
        ok = false;
    if (ok) {
        std::vector<uint8_t> verified;
        ok = read(target, verified) && verified == data;
    }
    if (ok)
        c.dirty = false;
    else
        c.saveSequence = previousSequence;
    return ok;
}
bool loadCity(const std::string &root, City &c) {
    std::vector<uint8_t> a, b;
    bool av = read(root + "/city-a.sav", a), bv = read(root + "/city-b.sav", b);
    size_t p = 39, q = 39;
    uint32_t at = av ? get(a, p) : 0, bt = bv ? get(b, q) : 0;
    if (bv && (!av || bt > at)) {
        if (decode(b, c))
            return true;
        return av && decode(a, c);
    }
    if (av && decode(a, c))
        return true;
    return bv && decode(b, c);
}
bool inspectCity(const std::string &root, SaveSummary &summary) {
    City *city = new City;
    if (!loadCity(root, *city)) {
        delete city;
        return false;
    }
    summary.seed = city->seed;
    summary.days = city->days;
    summary.sequence = city->saveSequence;
    summary.money = city->money;
    summary.population = city->metrics.population;
    summary.jobs = city->metrics.jobs;
    summary.happiness = city->metrics.happiness;
    summary.unpowered = city->metrics.unpowered;
    summary.unwatered = city->metrics.unwatered;
    delete city;
    return true;
}
} // namespace mc
