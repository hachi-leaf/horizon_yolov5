# ----------------------------------------
#       函数相关API的cython接口
#                   Made by Leaf.
# ----------------------------------------

cimport numpy as np
import numpy as pynp
#from libc.string cimport memcpy
#from libc.stdlib cimport malloc, free

# 导入C++函数定义
cdef extern from "./postprocess.cpp":
    int cpp_postprocess(float* ,float* ,float* ,int ,int ,int ,int ,float ,float ,float*)

# 适配C++函数 类型转换
def _postprocess(
    np.ndarray[np.float32_t, ndim=4] py_conv_output0,
    np.ndarray[np.float32_t, ndim=4] py_conv_output1,
    np.ndarray[np.float32_t, ndim=4] py_conv_output2,

    int model_size, int cls_num, int img_w, int img_h,
    float score_threshold=0.4, float nms_threshold=0.45,
    int max_num = 50):

    output = pynp.zeros((int(max_num),6), dtype=pynp.float32)

    # print(float(index(<float*>np.PyArray_DATA(py_conv_output), n)))
    cdef int targets_num = cpp_postprocess(
        <float*>np.PyArray_DATA(py_conv_output0.reshape(-1)),
        <float*>np.PyArray_DATA(py_conv_output1.reshape(-1)),
        <float*>np.PyArray_DATA(py_conv_output2.reshape(-1)),

        <int>model_size, <int>cls_num, <int>img_w, <int>img_h,
        <float>score_threshold, <float>nms_threshold,
        <float*>np.PyArray_DATA(output))

    return output[0:targets_num];

# 简化接口
def postprocess(
    infer_output,

    int model_size, int cls_num, int img_w, int img_h,
    float score_threshold=0.4, float nms_threshold=0.45,
    int max_num = 50):

    return _postprocess( 
        infer_output[0].buffer,
        infer_output[1].buffer,
        infer_output[2].buffer,

        model_size, cls_num, img_w, img_h,
        score_threshold, nms_threshold, max_num)
