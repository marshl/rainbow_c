# Rainbow_c — build and render recipes.
#
# `make` (no arguments) prints a list of what's available.
# `make build` compiles the project.
# `make trans` (or any other render target) rebuilds if needed, then renders.
#
# The build lives in `cmake-build-release/` — same directory CLion uses,
# so building here and clicking play in CLion produce the same artifacts.

BUILD_DIR := cmake-build-release
BIN := $(BUILD_DIR)/rainbow_c

# What runs when someone just types `make`. Prints usage instead of
# doing something unexpected.
.DEFAULT_GOAL := help

# Any target that isn't the name of a real file needs to be listed here,
# or `make` will get confused if a file with that name ever appears.
.PHONY: help build clean trans trans-boundary rainbow lesbian germany


# ─── Build ────────────────────────────────────────────────────────────────

# The CMake configure step — creates the build directory and generates
# the inner Makefile that cmake will invoke. It only runs when
# CMakeCache.txt is missing (first build, or after `make clean`), so
# repeated `make build` calls don't reconfigure.
$(BUILD_DIR)/CMakeCache.txt:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

# Compiles the project. `cmake --build` is a portable wrapper — it
# doesn't care whether the underlying generator is Make, Ninja, MSVC,
# etc. Fast when nothing has changed (just checks timestamps).
build: $(BUILD_DIR)/CMakeCache.txt
	cmake --build $(BUILD_DIR)

# Nuke the build dir. Next `make build` will reconfigure from scratch.
clean:
	rm -rf $(BUILD_DIR)


# ─── Render recipes ───────────────────────────────────────────────────────
#
# Each depends on `build` so the binary is up to date before running.
# Trans/rainbow/lesbian are the flags we've been iterating on; add more
# by copying one of these blocks.

trans: build
	$(BIN) -w 1500 -h 1000 \
	    -C 5BCEFA -C F5A9B4 -C FFFFFF -C F5A9B4 -C 5BCEFA \
	    -P 100,300,500,700,900 \
	    -o R -d colour -F 10

# Same trans flag but with -B for hard boundaries between stripes.
trans-boundary: build
	$(BIN) -w 1500 -h 1000 \
	    -C 5BCEFA -C F5A9B4 -C FFFFFF -C F5A9B4 -C 5BCEFA \
	    -P 100,300,500,700,900 \
	    -B -o R -d colour -F 10

rainbow: build
	$(BIN) -w 1800 -h 1200 \
	    -C E40303 -C FF8C00 -C FFED00 -C 008026 -C 004DFF -C 750787 \
	    -P 100,300,500,700,900,1100 \
	    -o R -d colour -F 10

lesbian: build
	$(BIN) -w 1750 -h 1050 \
	    -C D52D00 -C EF7627 -C FF9A56 -C FFFFFF -C D162A4 -C B55690 -C A30262 \
	    -P 75,225,375,525,675,825,975 \
	    -o R -d colour -F 10

# German flag: 5:3 ratio, three equal horizontal bands (black, red, gold),
# hard boundaries between stripes.
germany: build
	$(BIN) -w 1500 -h 900 \
	    -C 000000 -C DD0000 -C FFCE00 \
	    -P 150,450,750 \
	    -B -o R -d colour -F 10


# ─── Help ─────────────────────────────────────────────────────────────────

# The `@` prefix on each line tells make not to echo the command itself
# before running it — otherwise `make help` would print `echo "..."`
# followed by the output.
help:
	@echo "Rainbow_c targets:"
	@echo ""
	@echo "  Build:"
	@echo "    build           Configure (if needed) and compile the project."
	@echo "    clean           Remove the build directory."
	@echo ""
	@echo "  Render (each auto-builds first):"
	@echo "    trans           Trans pride flag (1500x1000)."
	@echo "    trans-boundary  Same, with -B for razor-sharp stripe edges."
	@echo "    rainbow         Classic 6-stripe rainbow (1800x1200)."
	@echo "    lesbian         7-stripe lesbian pride flag (1750x1050)."
	@echo "    germany         German national flag, hard bands (1500x900)."
	@echo ""
	@echo "  Usage:  make <target>"
