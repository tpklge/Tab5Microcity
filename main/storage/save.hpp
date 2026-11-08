#pragma once
#include "core/city.hpp"
#include <string>
#include <vector>
namespace mc {
struct SaveSummary {
    uint32_t seed = 0, days = 0, sequence = 0;
    int64_t money = 0;
    int population = 0, jobs = 0, happiness = 0, unpowered = 0, unwatered = 0;
};
std::vector<uint8_t> encode(const City &);
bool decode(const std::vector<uint8_t> &, City &);
bool saveCity(const std::string &root, City &);
bool loadCity(const std::string &root, City &);
bool inspectCity(const std::string &root, SaveSummary &);
uint32_t crc32(const uint8_t *, size_t);
} // namespace mc
