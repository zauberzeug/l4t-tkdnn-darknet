FROM zauberzeug/l4t-opencv:4.5.2-on-nano-r32.5.0 as builder

RUN DEBIAN_FRONTEND=noninteractive apt update && apt-get install -y git wget libeigen3-dev

ARG MAKEFLAGS

#install new cmake version
RUN apt remove -y cmake
RUN wget https://cmake.org/files/v3.15/cmake-3.15.0.tar.gz && tar -zxvf cmake-3.15.0.tar.gz
RUN cd cmake-3.15.0 && ./bootstrap && make install

RUN cmake --version

# Source https://github.com/dusty-nv/jetson-containers/blob/master/Dockerfile.ros.foxy
# compile yaml-cpp-0.6, which is needed (but is not in the 18.04 apt repo)
RUN git clone --branch yaml-cpp-0.6.0 https://github.com/jbeder/yaml-cpp yaml-cpp-0.6 && \
    cd yaml-cpp-0.6 && \
    mkdir build && \
    cd build && \
    cmake -DBUILD_SHARED_LIBS=ON -D CMAKE_INSTALL_PREFIX=/usr/local/libyaml-cpp .. && \
    make $MAKEFLAGS && \
    make install && \
    cp libyaml-cpp.so.0.6.0 /usr/lib/aarch64-linux-gnu/ && \
    ln -s /usr/lib/aarch64-linux-gnu/libyaml-cpp.so.0.6.0 /usr/lib/aarch64-linux-gnu/libyaml-cpp.so.0.6

WORKDIR /
RUN ls
RUN git clone https://github.com/ceccocats/tkDNN.git

# changes in CMakeLists.txt neccesary 
RUN rm -rf tkDNN
RUN git clone https://github.com/ceccocats/tkDNN.git tkDNN
COPY CMakeLists.txt /tkDNN/CMakeLists.txt

# adapt scripts to work with python
COPY ./tkdnn_python/DetectionNN.h /tkDNN/include/tkDNN/DetectionNN.h
COPY ./tkdnn_python/utils.h /tkDNN/include/tkDNN/utils.h
COPY ./tkdnn_python/utils.cpp /tkDNN/src/utils.cpp
COPY ./tkdnn_python/darknetRT.cpp /tkDNN/demo/demo/darknetRT.cpp
COPY ./tkdnn_python/darknetRT.h /tkDNN/demo/demo/darknetRT.h

RUN mkdir /tkDNN/build
COPY yolo4tiny.cpp /tkDNN/tests/darknet/yolo4tiny.cpp
RUN cd tkDNN && cd build && cmake .. -D CMAKE_INSTALL_PREFIX=/usr/local/tkDNN && make $MAKEFLAGS && make install

FROM zauberzeug/l4t-opencv:4.5.2-on-nano-r32.5.0

COPY --from=builder /usr/local/tkDNN /usr/local
COPY --from=builder /tkDNN/build/libdarknetRT.so /usr/local/lib/libdarknetRT.so
COPY --from=builder /usr/local/libyaml-cpp /usr/local
COPY --from=builder /tkDNN/build/test_yolo4tiny /usr/local/bin

CMD bash
