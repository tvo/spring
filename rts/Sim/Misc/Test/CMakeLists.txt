# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(root "../../../..")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${root}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

ADD_DEFINITIONS(-DUNITTEST -DBITMAP_NO_OPENGL -DNOT_USING_CREG -DHEADLESS)

INCLUDE_DIRECTORIES(${root}/rts/lib/lua/include)
INCLUDE_DIRECTORIES(${root}/rts/lib/7zip)
INCLUDE_DIRECTORIES(${root}/rts)
INCLUDE_DIRECTORIES(${root}/rts/System)

set(test_files
	LosHandlerMockObjects.cpp
	LosHandlerTest.cpp
	../LosMap.cpp
	${root}/rts/System/MemPool.cpp
	)

### Build the executable
ADD_EXECUTABLE(test-sim-misc ${test_files})
TARGET_LINK_LIBRARIES(test-sim-misc streflop)
