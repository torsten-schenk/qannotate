#!/bin/sh

rm -Rf build

mkdir -p build/specgen.dir
mkdir -p build/globalgen.dir
mkdir -p build/qannotate.dir

cd build/specgen.dir
cmake ../../util/specgen
make
make install

cd ../globalgen.dir
cmake ../../util/globalgen
make
make install

cd ../qannotate.dir
cmake ../../qannotate
make -j10

