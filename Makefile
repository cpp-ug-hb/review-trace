all:
	mkdir -p build
	cd build && cmake ..
	+${MAKE} -C build
