# OSM2Landscape

## Build process

the build process require CMake 3.12 or above and the Conan C/C++ package manager to automatically resolve the dependencies :

    mkdir build && cd build
	conan install ..
	cmake .. -DCMAKE_BUILD_TYPE=Release
	cmake --build .


### Configure Conan for GCC >= 5.1

    conan profile update settings.compiler.libcxx=libstdc++11 default


## Dependencies

nlohmann_json 3.9.1 (https://github.com/nlohmann/json)
poco 1.10.1 (https://github.com/pocoproject/poco)
boost 1.75.0 (https://www.boost.org/)

libosmium


