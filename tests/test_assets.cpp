#include "storage/assets.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace mc;
namespace fs = std::filesystem;
static void corrupt(const fs::path &path, size_t offset) {
    std::fstream f(path, std::ios::binary | std::ios::in | std::ios::out);
    f.seekg(offset);
    char b;
    f.read(&b, 1);
    b ^= 127;
    f.seekp(offset);
    f.write(&b, 1);
}
int main() {
    auto root = fs::temp_directory_path() / "microcity-asset-tests";
    fs::remove_all(root);
    fs::create_directories(root);
    auto package = fs::path("assets/package/microcity.mcap");
    auto a = root / "slot-a.mcap", b = root / "slot-b.mcap";
    uint32_t v;
    std::string error, path;
    assert(Assets::validate(package.string(), v, error));
    assert(v == 10);
    fs::copy_file(package, a);
    assert(publishAssets(root.string(), 0, 1));
    assert(selectAssets(root.string(), path) && path == a.string());
    // Every persisted phase keeps the previous complete slot bootable.
    for (int step = 0; step < 5; step++) {
        fs::remove(root / "slot-b.ready");
        fs::copy_file(package, b, fs::copy_options::overwrite_existing);
        if (step == 0)
            fs::resize_file(b, 100); // interrupted stream / missing card
        if (step == 1)
            corrupt(b, 10000); // divergent hash
        if (step == 2)
            corrupt(b, 60); // invalid catalog
        if (step == 3)
            corrupt(b, 8); // incompatible engine
        if (step < 4)
            assert(!Assets::validate(b.string(), v, error));
        assert(selectAssets(root.string(), path) && path == a.string()); // no ready record
        assert(publishAssets(root.string(), 1, 2));
        if (step < 4)
            assert(selectAssets(root.string(), path) && path == a.string());
    }
    assert(selectAssets(root.string(), path) && path == b.string());
    corrupt(root / "slot-b.ready", 8);
    assert(selectAssets(root.string(), path) && path == a.string());
    assert(publishAssets(root.string(), 1, 3));
    fs::resize_file(b, 60);
    assert(selectAssets(root.string(), path) && path == a.string());
    Assets cache;
    assert(cache.open(a.string()));
    for (int id = 0; id < 256; id++)
        if (cache.sprite(id))
            assert(cache.prefetch(id));
    assert(cache.bytes <= 4 * 1024 * 1024);
    assert(cache.cached(70));
    auto view = cache.raster(70);
    assert(view.colors && view.rows);
    for (int i = 0; i < view.width * view.height; i++)
        assert(view.colors[i] ==
               (uint16_t(view.rgba565[i * 3]) | uint16_t(view.rgba565[i * 3 + 1]) << 8));
    cache.close();
    assert(cache.bytes == 0);
    fs::remove_all(root);
    std::cout << "Firmware asset validation, interrupted slots, hash, catalog, compatibility, "
                 "activation rollback and cache bound: PASS\n";
}
