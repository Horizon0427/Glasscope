.PHONY: all configure hyprpm-build clean

all: configure
	cmake --build build -j

configure:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

hyprpm-build:
	bash scripts/build-hyprpm.sh

clean:
	cmake -E remove_directory build
	cmake -E remove_directory .hyprpm-build
	cmake -E remove_directory dist
