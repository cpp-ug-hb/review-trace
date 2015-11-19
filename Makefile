all:
	mkdir -p build
	cd build && cmake ..
	+${MAKE} -C build

format:
		clang-format -i */*.{cc,h,hpp}
