FROM ubuntu:20.04
RUN apt update
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt-get -y install tzdata
RUN apt install -y cmake clang python3-pip scons git
RUN git clone --recursive https://github.com/daos-stack/daos/
WORKDIR daos
RUN git checkout v2.0.1
RUN ./utils/scripts/install-ubuntu20.sh
RUN scons --version
RUN scons install -j$(nproc --ignore=2) --build-deps=yes --config=force
WORKDIR app
RUN apt install -y uuid-dev
COPY CMakeLists.txt .
COPY src/ src/
COPY lib/ lib/
RUN cmake -DDAOS_DIR=/daos/install -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -B build . 
RUN cmake --build build/ 
RUN cmake --install build

