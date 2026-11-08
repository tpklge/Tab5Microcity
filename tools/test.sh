#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .host-build
compiler="${CXX:-c++}"
common=(-std=c++17 -Wall -Wextra -Werror -g -Imain)
if [[ "$(uname -s)" == Darwin ]]; then
    sdk="${SDKROOT:-/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk}"
    [[ -d "$sdk" ]] || sdk="$(xcrun --show-sdk-path)"
    common+=(-isysroot "$sdk")
fi
if [[ -d "${OPENSSL_ROOT:-/opt/homebrew/opt/openssl@3}/include/openssl" ]]; then
    sslroot="${OPENSSL_ROOT:-/opt/homebrew/opt/openssl@3}"
    ssl=(-I"$sslroot/include" -L"$sslroot/lib" -lcrypto)
else
    read -ra ssl <<< "$(pkg-config --cflags --libs openssl)"
fi
"$compiler" "${common[@]}" -fsanitize=address,undefined tests/test_city.cpp main/core/city.cpp main/storage/save.cpp -o .host-build/test_city
.host-build/test_city
"$compiler" "${common[@]}" -Wno-deprecated-declarations -fsanitize=address,undefined -DMICROCITY_HOST tests/test_assets.cpp main/display/blitter.cpp main/storage/assets.cpp main/storage/save.cpp main/core/city.cpp "${ssl[@]}" -o .host-build/test_assets
.host-build/test_assets
"$compiler" "${common[@]}" -fsanitize=address,undefined tests/test_controls.cpp main/input/controls.cpp -o .host-build/test_controls
.host-build/test_controls
"$compiler" "${common[@]}" -fsanitize=address,undefined tests/test_blitter.cpp main/display/blitter.cpp -o .host-build/test_blitter
.host-build/test_blitter
"$compiler" "${common[@]}" -fsanitize=address,undefined tests/test_settings.cpp main/storage/settings.cpp main/storage/save.cpp main/core/city.cpp -o .host-build/test_settings
.host-build/test_settings
python3 -m unittest discover -s tests -p 'test_*.py' -v
"$compiler" "${common[@]}" tests/scenarios.cpp main/core/city.cpp main/storage/save.cpp -o .host-build/scenarios
.host-build/scenarios
