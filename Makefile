# NodeCraft build
#   make test   native g++ build of ds/ + tests/, then run the tests
#   make wasm   Emscripten build -> web/nodecraft.js + web/nodecraft.wasm
#   make streamlit  single-file build (wasm inside the JS) -> web/nodecraft.single.js
#   make clean  remove native build output (the built web files are kept)

CXX      ?= g++
EMXX     ?= em++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -g -O1

DS_SRC   := $(wildcard ds/*.cpp)
TEST_SRC := $(wildcard tests/*.cpp)
HEADERS  := $(wildcard ds/*.h) tests/check.h
TEST_BIN := build/nodecraft_tests

# ---- Sanitizers (TR-3) -------------------------------------------------------
# Use AddressSanitizer + UBSan when the compiler can link them (Linux, macOS).
# Some Windows toolchains (MinGW/w64devkit) ship no sanitizer runtime; there we
# fall back to UBSan in "trap" mode, which needs no runtime library.
# The tests also count every new/delete themselves, so leaks fail the run either way.
SAN_PROBE = echo 'int main(){return 0;}' | $(CXX) -x c++ $(1) - -o build/.probe >/dev/null 2>&1 && echo yes
FULL_SAN  := -fsanitize=address,undefined -fno-omit-frame-pointer -DNODECRAFT_SANITIZE
TRAP_SAN  := -fsanitize=undefined -fsanitize-undefined-trap-on-error

$(shell mkdir -p build web)
ifeq ($(shell $(call SAN_PROBE,$(FULL_SAN))),yes)
  SANFLAGS := $(FULL_SAN)
  SANNOTE  := AddressSanitizer + UndefinedBehaviorSanitizer
else ifeq ($(shell $(call SAN_PROBE,$(TRAP_SAN))),yes)
  SANFLAGS := $(TRAP_SAN)
  SANNOTE  := UBSan (trap mode) + built-in allocation counter (ASan not available here)
else
  SANFLAGS :=
  SANNOTE  := built-in allocation counter only (no sanitizers available here)
endif

.PHONY: test wasm streamlit clean

test: $(TEST_BIN)
	@echo "Sanitizers: $(SANNOTE)"
	./$(TEST_BIN)

$(TEST_BIN): $(DS_SRC) $(TEST_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SANFLAGS) $(DS_SRC) $(TEST_SRC) -o $@

# ---- WebAssembly ---------------------------------------------------------------
wasm:
	@command -v $(EMXX) >/dev/null 2>&1 || { echo "em++ not found. Install the Emscripten SDK and run 'source emsdk_env.sh' first."; exit 1; }
	$(EMXX) -std=c++17 -O2 -lembind -s MODULARIZE=1 -s EXPORT_NAME=createNodeCraft \
	  -s ALLOW_MEMORY_GROWTH=1 $(DS_SRC) wasm/bindings.cpp -o web/nodecraft.js

# Same engine with the .wasm embedded inside the JS (base64), for hosts that can
# only serve one HTML page, such as Streamlit (see streamlit_app.py).
streamlit:
	@command -v $(EMXX) >/dev/null 2>&1 || { echo "em++ not found. Install the Emscripten SDK and run 'source emsdk_env.sh' first."; exit 1; }
	$(EMXX) -std=c++17 -O2 -lembind -s MODULARIZE=1 -s EXPORT_NAME=createNodeCraft \
	  -s ALLOW_MEMORY_GROWTH=1 -s SINGLE_FILE=1 $(DS_SRC) wasm/bindings.cpp -o web/nodecraft.single.js

clean:
	rm -rf build
