FROM zauberzeug/l4t-opencv:4.5.2-on-nano-32.6.1 as builder

RUN DEBIAN_FRONTEND=noninteractive apt update && apt-get install -y git wget libeigen3-dev

ARG MAKEFLAGS

#install new cmake version
RUN apt remove -y cmake
RUN pip3 install --no-cache-dir --verbose --upgrade pip
RUN pip3 install --no-cache-dir --verbose --upgrade cmake

RUN cmake --version

# Source https://github.com/dusty-nv/jetson-containers/blob/master/Dockerfile.ros.foxy
# compile yaml-cpp-0.6, which is needed (but is not in the 18.04 apt repo)
RUN git clone --branch yaml-cpp-0.6.0 https://github.com/jbeder/yaml-cpp yaml-cpp-0.6 && \
    cd yaml-cpp-0.6 && \
    mkdir build && \
    cd build && \
    cmake -DBUILD_SHARED_LIBS=ON .. && \
    make $MAKEFLAGS && \
    make install

WORKDIR /

RUN git clone -b tensorrt8 https://github.com/ceccocats/tkDNN.git

WORKDIR /tkDNN

# applying patch to provide python support (adapted from https://github.com/ceccocats/tkDNN/pull/117)
COPY python_support.patch ./
RUN git apply python_support.patch

WORKDIR /tkDNN/build
RUN cmake .. -D CMAKE_INSTALL_PREFIX=/usr/local/tkDNN
RUN make $MAKEFLAGS
RUN make install

FROM zauberzeug/l4t-opencv:4.5.2-on-nano-32.6.1

COPY --from=builder /usr/local/tkDNN /usr/local
COPY --from=builder /tkDNN/build/libdarknetRT.so /usr/local/lib/libdarknetRT.so
COPY --from=builder /usr/local/libyaml-cpp /usr/local
COPY --from=builder /tkDNN/build/test_yolo4tiny /usr/local/bin

CMD bash
