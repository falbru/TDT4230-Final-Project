SOURCES := $(shell find src/ -type f | grep -E '\.(h|c)(pp)?$$')
MAKE_OPTS := -j4
GDB_OPTS := -ex "set style enabled on"

.PHONY: help
help:
	@echo -e "Try one of these make targets:\n"
	@grep "^\.PHONY: " Makefile | cut -d" " -f2- | tr -s " " | sed -e "s/ /\n/g" | grep -v "^_" | sed -e "s/^/make /"

.PHONY: run run-with-music run-debug
run: build
	cd build && ./tdt4230
run-with-music: build
	cd build && ./tdt4230 --enable-music
run-debug: build-debug | has-gdb
	cd build-debug && gdb -batch $(GDB_OPTS) -ex "run" -ex "backtrace" ./tdt4230

.PHONY: build
build: build/tdt4230
build/tdt4230: ${SOURCES} | build/Makefile has-make
	make -C build $(MAKE_OPTS)
build/Makefile: | build/ _submodules has-cmake
	cd build && cmake ..

.PHONY: build-debug
build-debug: build-debug/tdt4230
build-debug/tdt4230: ${SOURCES} | build-debug/Makefile has-make
	make -C build-debug $(MAKE_OPTS)
build-debug/Makefile: | build-debug/ _submodules has-cmake
	cd build-debug && cmake -DCMAKE_BUILD_TYPE=Debug ..

.PHONY: _submodules
_submodules: | has-git
	@git submodule update --init

.PHONY: clean
clean:
	@# TODO: submodules
	@test -d build-debug && rm -rfv build-debug/ || true
	@rm -rfv build/*


# === helpers: ===

# make folders, use as order-only prerequisite
%/:
	mkdir -p $*

# check if program is available in PATH, use as order-only prerequisite
has-%:
	@command -v $* >/dev/null || ( \
		echo "ERROR: Command '$*' not found! Make sure it is installed and available in PATH"; \
		false; \
	) >&2
