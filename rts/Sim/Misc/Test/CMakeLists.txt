# Place executables and shared libs under "build-dir/",
# instead of under "build-dir/rts/"
# This way, we have the build-dir structure more like the install-dir one,
# which makes testing spring in the builddir easier, eg. like this:
# cd build-dir
# SPRING_DATADIR=$(pwd) ./spring
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../../../..")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

ADD_DEFINITIONS(-DUNITTEST -DBITMAP_NO_OPENGL -DNOT_USING_CREG -DHEADLESS)

INCLUDE_DIRECTORIES(../../../lib/lua/include)
INCLUDE_DIRECTORIES(../../../lib/7zip)
INCLUDE_DIRECTORIES(../../..)
INCLUDE_DIRECTORIES(../../../System)

set(test_files
	LosHandlerMockObjects.cpp
	../LosHandler.cpp
	../LosMap.cpp
	)

### Build the executable
ADD_EXECUTABLE(test-sim-misc ${test_files})
TARGET_LINK_LIBRARIES(test-sim-misc)
