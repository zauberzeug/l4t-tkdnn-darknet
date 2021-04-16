# l4t-tkdnn

tkDNN and darknet for Jetson Nano (Linux for Tegra, l4t) with Docker. At this stage, the Container is tested with a YOLOv4-Tiny net with three output layers.

Code: https://github.com/zauberzeug/l4t-tkdnn-darknet

Image: tbd.

# Usage

## Dockerfile

Most of the time you will use this image as base for your own Dockerfile:

```
FROM zauberzeug/l4t-tkdnn-darknet:nano-r32.4.4

...
```

## Docker Run

`docker run --rm -it --runtime=nvidia -e NVIDIA_VISIBLE_DEVICES=all zauberzeug/l4t-tkdnn-darknet:nano-r32.4.4 bash`

## Docker Compose

```
version: "3.3"

services:
  tkdnn:
    image: "zauberzeug/l4t-tkdnn-darknet:nano-r32.4.4"
    environment:
      - NVIDIA_VISIBLE_DEVICES=all
    build:
      args:
        MAKEFLAGS: "-j6"
    command: "/bin/bash"
    volumes:
      - ./test_data:/model
      - ./tkdnn_python/darknet_rt.py:/tkDNN/darknet_rt.py
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              capabilities: [gpu, utility]
```

# Build

We use drone to automatically build this image. If you want to do it by hand, execute

`docker build --build-arg MAKEFLAGS=-j6 -t l4t-tkdnn-darknet:latest .`

# Demo

## Prequesites

This Repo uses the [tkDNN Repository](https://github.com/ceccocats/tkDNN). Although tkDNN and darknet are already compiled, the weights have to be exported manually.
Therefore, `./darknet export <path-to-cfg-file> <path-to-weights> layers` has to be executed inside `/tkDNN/darknet`.

If data like a custom `.cfg`file or test images should be available on startup, you can place those files inside the `test_data` directory. For the demo to work with custom data, the needed files are a `cfg` file called `training.cfg`, a `weightfile` called `some_weightfile` and a `txt` file called `names.txt` including the classnames. If you want to use different filenames for the yolo4tiny demo, you have to adapt the `/tkdnn/tests/yolo4tiny.cpp` file and recompile tkdnn.

After the weights are exported, the `.rt` file has to be created by calling `./test_yolo4tiny`inside the `build` directory. Of course this step can be done with any other net available inside that directory. 
Precision is set beforehand by `export TKDNN_MODE=FP16` or `FP32`. `INT8` is not supported by Jetson Nano.

## Run Demo

The demo can be run like explained in the tkDNN Repo: https://github.com/ceccocats/tkDNN#run-the-demo

This demo additionally includes a Python Wrapper like suggested here: https://github.com/ceccocats/tkDNN/pull/44

This wrapper is based on this pull request and can be run by calling `python3 darknet_rt.py ./darknet_fp16.rt --file=example.jpg`

In general, the file takes 4 arguments:

`python3 darknet_rt.py <network-rt-file> <width> <height> <path-to-file>`

where: 
* `<network-rt-file>` : The generated .rt file
* `<width>`: width of the provided file. If not provided, it defaults to 1600.
* `<height>`: height of the provided file. If not provided, it defaults to 1200.
* `<path-to-file>`: Path to the used image or video file. Has to be provided by using `--file=*`







