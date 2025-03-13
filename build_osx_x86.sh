git submodule update --init --recursive

pip install --user protobuf --break-system-packages

brew update
brew install --force streetpea/streetpea/chiaki-ng-qt@6 \
    ffmpeg \
    pkgconfig \
    opus \
    openssl \
    cmake \
    ninja \
    nasm \
    sdl2 \
    protobuf \
    speexdsp \
    libplacebo \
    wget \
    python-setuptools \
    json-c miniupnpc || true

cmake -S . -B build -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCHIAKI_ENABLE_PYBIND=ON \
    -DCHIAKI_ENABLE_CLI=OFF \
    -DCHIAKI_ENABLE_GUI=OFF \
    -DCHIAKI_ENABLE_STEAMDECK_NATIVE=OFF \
    -DCHIAKI_ENABLE_STEAMDECK_NATIVE=OFF \
    -DCHIAKI_ENABLE_STEAM_SHORTCUT=OFF \
    -DCMAKE_PREFIX_PATH="$(brew --prefix)/opt/@openssl@3;$(brew --prefix)/opt/chiaki-ng-qt@6"

export CPATH=$(brew --prefix)/opt/ffmpeg/include
cmake --build build --config Debug --clean-first --target chiaki_py