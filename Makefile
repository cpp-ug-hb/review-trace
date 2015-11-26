all:
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release ..
	+${MAKE} -C build

format:
		clang-format -i */*.{cc,h,hpp}
