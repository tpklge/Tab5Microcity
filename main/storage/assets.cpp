#include "assets.hpp"
#include "save.hpp"
#ifdef MICROCITY_HOST
#include <openssl/sha.h>
using mbedtls_sha256_context = SHA256_CTX;
static void mbedtls_sha256_init(SHA256_CTX *) {}
static void mbedtls_sha256_free(SHA256_CTX *) {}
static int mbedtls_sha256_starts(SHA256_CTX *c, int) {
    return SHA256_Init(c);
}
static int mbedtls_sha256_update(SHA256_CTX *c, const unsigned char *p, size_t n) {
    return SHA256_Update(c, p, n);
}
static int mbedtls_sha256_finish(SHA256_CTX *c, unsigned char *p) {
    return SHA256_Final(p, c);
}
#else
#include <mbedtls/sha256.h>
#endif
#include <algorithm>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
namespace mc {
static uint16_t u16(const uint8_t *p) {
    return p[0] | uint16_t(p[1]) << 8;
}
static uint32_t u32(const uint8_t *p) {
    return u16(p) | uint32_t(u16(p + 2)) << 16;
}
static bool header(FILE *f, uint8_t *h) {
    return fread(h, 1, 60, f) == 60 && std::string((char *)h, 4) == "MCAP" && u32(h + 4) == 1 &&
           u32(h + 8) <= 1 && u32(h + 12) >= 1 && u32(h + 20) > 0 && u32(h + 20) <= 256 &&
           u32(h + 24) <= 32 * 1024 * 1024 - 60;
}
static Sprite entry(const uint8_t *p) {
    return {u16(p),     u16(p + 2),  u16(p + 4),  u16(p + 6),
            u16(p + 8), u16(p + 10), u32(p + 12), u32(p + 16)};
}
bool Assets::validate(const std::string &p, uint32_t &ver, std::string &err) {
    FILE *f = fopen(p.c_str(), "rb");
    if (!f) {
        err = "Não foi possível abrir pacote";
        return false;
    }
    bool ok = false;
    do {
        uint8_t h[60];
        if (!header(f, h)) {
            err = "Formato ou engine incompatível";
            break;
        }
        uint32_t count = u32(h + 20), len = u32(h + 24);
        if (len < count * 20) {
            err = "Catálogo incompleto";
            break;
        }
        std::array<bool, 256> seen{};
        bool bad = false;
        uint32_t next = count * 20;
        for (unsigned i = 0; i < count; i++) {
            uint8_t e[20];
            if (fread(e, 1, 20, f) != 20) {
                bad = true;
                break;
            }
            auto s = entry(e);
            bool image = s.format == 1 && s.w >= 1 && s.w <= 256 && s.h >= 1 && s.h <= 320 &&
                         s.ax < s.w && s.ay < s.h && s.length == uint32_t(s.w) * s.h * 3;
            bool sound = s.format == 2 && s.w == 2400 && s.h == 1 && s.ax == 0 && s.ay == 0 &&
                         s.length == 4800 && s.id >= 200 && s.id <= 202;
            if (s.id >= 256 || seen[s.id] || (!image && !sound) || s.offset != next ||
                s.offset > len || s.length > len - s.offset) {
                bad = true;
                break;
            }
            seen[s.id] = true;
            next += s.length;
        }
        const int required[] = {0,  1,   2,   30,  31,  32,  60,  61,  62,  70,  71, 72, 73,
                                74, 75,  80,  81,  82,  83,  84,  85,  90,  91,  92, 93, 94,
                                95,  100, 101, 102, 103, 104, 105, 110, 111, 112, 113, 114,
                                115, 116, 117, 132, 133, 134, 135};
        for (int id : required)
            if (!seen[id])
                bad = true;
        for (int i = 10; i < 26; i++)
            if (!seen[i])
                bad = true;
        for (int i = 40; i < 56; i++)
            if (!seen[i])
                bad = true;
        for (int i = 140; i < 156; i++)
            if (!seen[i])
                bad = true;
        for (int i = 156; i < 172; i++)
            if (!seen[i])
                bad = true;
        // 189+ are optional visual refinements. Keeping them optional lets a
        // new firmware roll back to the previous complete art slot safely.
        for (int i = 172; i < 189; i++)
            if (!seen[i])
                bad = true;
        if (bad || next != len) {
            err = "Sprite ou catálogo inválido";
            break;
        }
        fseek(f, 60, SEEK_SET);
        mbedtls_sha256_context sha;
        mbedtls_sha256_init(&sha);
        mbedtls_sha256_starts(&sha, 0);
        uint8_t buf[4096], digest[32];
        uint32_t remaining = len;
        while (remaining) {
            size_t n = fread(buf, 1, std::min<uint32_t>(remaining, sizeof(buf)), f);
            if (!n)
                break;
            mbedtls_sha256_update(&sha, buf, n);
            remaining -= n;
        }
        mbedtls_sha256_finish(&sha, digest);
        mbedtls_sha256_free(&sha);
        if (remaining || fgetc(f) != EOF || !std::equal(digest, digest + 32, h + 28)) {
            err = "Tamanho ou SHA-256 divergente";
            break;
        }
        ver = u32(h + 16);
        ok = true;
    } while (false);
    fclose(f);
    return ok;
}
void Assets::close() {
    if (file)
        fclose(file);
    file = nullptr;
    for (auto &c : cache) {
        free(c.data);
        free(c.colors);
        c = Cached{};
    }
    bytes = 0;
    count = 0;
    catalogIndex.fill(-1);
    cacheIndex.fill(-1);
    path.clear();
}
bool Assets::open(const std::string &p) {
    close();
    if (!validate(p, version, error))
        return false;
    file = fopen(p.c_str(), "rb");
    if (!file)
        return false;
    uint8_t h[60];
    if (!header(file, h)) {
        close();
        return false;
    }
    count = u32(h + 20);
    for (unsigned i = 0; i < count; i++) {
        uint8_t e[20];
        if (fread(e, 1, 20, file) != 20) {
            close();
            return false;
        }
        catalog[i] = entry(e);
        catalogIndex[catalog[i].id] = i;
    }
    path = p;
    return true;
}
const Sprite *Assets::sprite(uint16_t id) const {
    if (id >= 256 || catalogIndex[id] < 0)
        return nullptr;
    return &catalog[catalogIndex[id]];
}
const uint8_t *Assets::cached(uint16_t id) const {
    if (id >= 256 || cacheIndex[id] < 0)
        return nullptr;
    return cache[cacheIndex[id]].data;
}
RasterView Assets::raster(uint16_t id) const {
    auto s = sprite(id);
    if (!s || s->format != 1 || cacheIndex[id] < 0)
        return {};
    const auto &c = cache[cacheIndex[id]];
    return {c.data, c.colors, c.rows, s->w, s->h, s->ax, s->ay};
}
bool Assets::prefetch(uint16_t id) {
    clock++;
    if (id < 256 && cacheIndex[id] >= 0) {
        cache[cacheIndex[id]].used = clock;
        hits++;
        return true;
    }
    misses++;
    auto s = sprite(id);
    if (!file || !s)
        return false;
    size_t colorBytes = s->format == 1 ? size_t(s->w) * s->h * 2 : 0,
           rowBytes = s->format == 1 ? size_t(s->h) * sizeof(RowBounds) : 0,
           needed = s->length + colorBytes + rowBytes;
    auto discard = [&](Cached &c) {
        bytes -= c.size;
        if (c.id < 256)
            cacheIndex[c.id] = -1;
        free(c.data);
        free(c.colors);
        c = Cached{};
    };
    auto it = std::min_element(cache.begin(), cache.end(), [](const Cached &a, const Cached &b) {
        return (!a.data ? 0 : a.used) < (!b.data ? 0 : b.used);
    });
    while (bytes - it->size + needed > 4 * 1024 * 1024) {
        auto oldest = cache.end();
        for (auto candidate = cache.begin(); candidate != cache.end(); ++candidate)
            if (candidate != it && candidate->data &&
                (oldest == cache.end() || candidate->used < oldest->used))
                oldest = candidate;
        if (oldest == cache.end())
            return false;
        discard(*oldest);
    }
    discard(*it);
    it->data = (uint8_t *)malloc(s->length);
    if (colorBytes) {
        it->colors = (uint16_t *)malloc(colorBytes + rowBytes);
        if (it->colors)
            it->rows = (RowBounds *)((uint8_t *)it->colors + colorBytes);
    }
    if (!it->data || (colorBytes && !it->colors) || (rowBytes && !it->rows)) {
        discard(*it);
        return false;
    }
    if (fseek(file, 60 + s->offset, SEEK_SET) || fread(it->data, 1, s->length, file) != s->length) {
        discard(*it);
        error = "Falha de leitura no SD";
        return false;
    }
    if (s->format == 1)
        prepareRaster(it->data, it->colors, it->rows, s->w, s->h);
    it->id = id;
    it->size = needed;
    it->used = clock;
    bytes += needed;
    cacheIndex[id] = int(it - cache.begin());
    return true;
}
uint32_t assetSequence(const std::string &root, int slot) {
    std::string path = root + (slot ? "/slot-b.ready" : "/slot-a.ready");
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return 0;
    uint8_t b[12];
    bool ok = fread(b, 1, 12, f) == 12 && fgetc(f) == EOF;
    fclose(f);
    if (!ok || u32(b) != 0x59444152 || u32(b + 8) != crc32(b, 8))
        return 0;
    return u32(b + 4);
}
bool publishAssets(const std::string &root, int slot, uint32_t seq) {
    uint8_t b[12];
    uint32_t v[] = {0x59444152, seq, 0};
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 4; i++)
            b[j * 4 + i] = (v[j] >> (8 * i)) & 255;
    v[2] = crc32(b, 8);
    for (int i = 0; i < 4; i++)
        b[8 + i] = (v[2] >> (8 * i)) & 255;
    std::string path = root + (slot ? "/slot-b.ready" : "/slot-a.ready");
    FILE *f = fopen(path.c_str(), "wb");
    if (!f)
        return false;
    bool ok = fwrite(b, 1, 12, f) == 12;
    if (fflush(f) || fsync(fileno(f)))
        ok = false;
    if (fclose(f))
        ok = false;
    return ok;
}
bool selectAssets(const std::string &root, std::string &path) {
    uint32_t a = assetSequence(root, 0), b = assetSequence(root, 1), v;
    std::string err;
    for (int attempt = 0; attempt < 2; attempt++) {
        int slot = (b > a) ? 1 : 0;
        uint32_t seq = slot ? b : a;
        if (seq) {
            auto p = root + (slot ? "/slot-b.mcap" : "/slot-a.mcap");
            if (Assets::validate(p, v, err)) {
                path = p;
                return true;
            }
        }
        if (slot)
            b = 0;
        else
            a = 0;
    }
    return false;
}
} // namespace mc
