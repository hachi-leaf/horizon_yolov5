# ----------------------------------------------------------------------
#        This script is a sample program for Yolov5 acceleration.
#                    
#                                Made by Leaf.
# ----------------------------------------------------------------------
from hobot_dnn import pyeasy_dnn as dnn
from hobot_vio import libsrcampy as srcampy
import time
import cv2
import numpy as np
import yolov5_post
import multiprocessing
import copy as cp
import os


def bgr2nv12_opencv(image):
    height, width = image.shape[0], image.shape[1]
    area = height * width
    yuv420p = cv2.cvtColor(image, cv2.COLOR_BGR2YUV_I420).reshape((area * 3 // 2,))
    y = yuv420p[:area]
    uv_planar = yuv420p[area:].reshape((2, area // 4))
    uv_packed = uv_planar.transpose((1, 0)).reshape((area // 2,))

    nv12 = np.zeros_like(yuv420p)
    nv12[:height * width] = y
    nv12[height * width:] = uv_packed
    return nv12

def print_properties(pro):
    print("tensor type:", pro.tensor_type)
    print("data type:", pro.dtype)
    print("layout:", pro.layout)
    print("shape:", pro.shape)

def compute_box(qu,out,size,cls_num,img_w,img_h): 
    qu.put(yolov5_post.postprocess(out, size, cls_num, img_w, img_h))


if __name__ == '__main__':
    # Overclocking
    os.system("sudo bash -c 'echo performance > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor'")
    os.system("sudo bash -c 'echo 1 > /sys/devices/system/cpu/cpufreq/boost'")

    # Loading model
    models = dnn.load('./dominoes_new_yolov5.bin')

    # Print model's information
    for output in models[0].outputs:
        print_properties(output.properties)
        
    # Read image and translate to 640x640 nv12 data
    img_file = cv2.imread('./dominoes.jpg')
    resized_data = cv2.resize(img_file, (640,640), interpolation=cv2.INTER_AREA)
    nv12_data = bgr2nv12_opencv(resized_data)

    # Some calculation parameters
    img_w, img_h, _ = img_file.shape
    model_size = 640
    class_num = 6

    # Make a data queue
    q = multiprocessing.Queue()

    # Begin time
    t1 = time.time()

    # 1.First infer
    outputs = models[0].forward(nv12_data)

    for i in range(100):
        # 2.Define and start the process of post
        thread_cpu = multiprocessing.Process(target=compute_box,args=(q, outputs, model_size, class_num, img_w, img_h))
        thread_cpu.start()

        # 3.When the post-processing is running in another process, a 
        # new inference computation begins
        outputs = models[0].forward(nv12_data)

        # 4.Wait the post-processing finish
        thread_cpu.join()
        
        # 5.Obtain post-processing results
        box = q.get()

    
    print(box)
    print(time.time()-t1)
    