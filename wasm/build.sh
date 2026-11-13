#!/bin/sh
# Builds the NodeCraft backend to WebAssembly.
# Run from the NodeCraft/ folder after activating Emscripten (source emsdk_env.sh).
set -e
cd "$(dirname "$0")/.."
mkdir -p web
em++ -std=c++17 -O2 -lembind -s MODULARIZE=1 -s EXPORT_NAME=createNodeCraft \
  -s ALLOW_MEMORY_GROWTH=1 ds/*.cpp wasm/bindings.cpp -o web/nodecraft.js
echo "Built web/nodecraft.js and web/nodecraft.wasm"
