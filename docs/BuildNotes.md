CONFIGURE

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

BUILD (the .clap lands at build/StruckSynth.clap; no files outside the build tree are touched)

cmake --build build -j

CLEAN 

cmake --build build -j --clean-first


Config Dump build:

    cd ~/dev/dev-clap/StruckSynth
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DSTRUCKSYNTH_PRESET_DUMP=ON
cmake --build build -j"$(nproc)"
