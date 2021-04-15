from ctypes import *
import cv2
import numpy as np
import argparse
import os
from threading import Thread
import time

class IMAGE(Structure):
    _fields_ = [("w", c_int),
                ("h", c_int),
                ("c", c_int),
                ("data", POINTER(c_float))]

class BOX(Structure):
    _fields_ = [("x", c_float),
                ("y", c_float),
                ("w", c_float),
                ("h", c_float)]

class DETECTION(Structure):
    _fields_ = [("cl", c_int),
                ("bbox", BOX),
                ("prob", c_float),
                ("name", c_char*20),
                ]

lib = CDLL("./build/libdarknetRT.so", RTLD_GLOBAL) 


load_network = lib.load_network
load_network.argtypes = [c_char_p, c_int, c_int]
load_network.restype = c_void_p

copy_image_from_bytes = lib.copy_image_from_bytes
copy_image_from_bytes.argtypes = [IMAGE,c_char_p]

make_image = lib.make_image
make_image.argtypes = [c_int, c_int, c_int]
make_image.restype = IMAGE

do_inference = lib.do_inference
do_inference.argtypes = [c_void_p, IMAGE]

get_network_boxes = lib.get_network_boxes
get_network_boxes.argtypes = [c_void_p, c_float, POINTER(c_int)]
get_network_boxes.restype = POINTER(DETECTION)



def detect_image(net, darknet_image, thresh=.2):
    num = c_int(0)

    pnum = pointer(num)
    do_inference(net, darknet_image)
    dets = get_network_boxes(net, thresh, pnum)
    res = []
    for i in range(pnum[0]):
        b = dets[i].bbox
        res.append((dets[i].name.decode("ascii"), dets[i].prob, (b.x, b.y, b.w, b.h)))

    return res


def loop_detect(detect_m, file_path, width, height):
    start = time.time()
    cnt = 0
    if not file_path.endswith('.mp4'):
        image = cv2.imread(file_path)
        img_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

        for _ in range(10):
            detections = detect_m.detect(img_rgb)
            cnt += 1
            for det in detections:
                    print(det)
            print()
    else:
        stream = cv2.VideoCapture(file_path)
        while stream.isOpened():
            ret, image = stream.read()
            if ret is False:
                break
            image = cv2.resize(image,
                           (width, height),
                           interpolation=cv2.INTER_LINEAR)
            
            detections = detect_m.detect(image)
            cnt += 1
            for det in detections:
                print(det)

        stream.release()

    end = time.time()
    print(f"frame:{cnt},time:{end-start},FPS:{cnt/(end-start)}")


class YOLO4RT(object):
    def __init__(self,
                 image_width,
                 image_height,
                 weight_file,
                 conf_thres=0.2,
                 device='cuda'):
        self.image_width = image_width
        self.image_height = image_height 
        self.model = load_network(weight_file.encode("ascii"), 9, 1)
        self.darknet_image = make_image(image_width, image_height, 3)
        self.thresh = conf_thres

    def detect(self, image):
        try:
            frame_data = image.ctypes.data_as(c_char_p)
            copy_image_from_bytes(self.darknet_image, frame_data)

            detections = detect_image(self.model, self.darknet_image, thresh=self.thresh)

            return detections
        except Exception as e:
            print(e)

def parse_args():
    parser = argparse.ArgumentParser(description='tkDNN detect')
    parser.add_argument('weight', help='rt weightfile path')
    parser.add_argument('width', nargs='?', help='width of frame', default=1600)
    parser.add_argument('height', nargs='?', help='height of frame', default=1200)
    parser.add_argument('--file',  type=str, help='file path')
    args = parser.parse_args()

    return args



if __name__ == '__main__':
    args = parse_args()
    
    detect_m = YOLO4RT(image_width=int(args.width), image_height=int(args.height), weight_file=args.weight)

    t = Thread(target=loop_detect, args=(detect_m, args.file, int(args.width), int(args.height)), daemon=True)

    t.start()
    t.join()