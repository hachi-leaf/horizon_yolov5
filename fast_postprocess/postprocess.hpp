// ----------------------------------------
//      相关函数声明
//                  Made by Leaf.
// ----------------------------------------
#ifndef POSTPROCESS_HPP
#define POSRPRCOESS_HPP

#include <cmath>

float sigmoid(float x)
{
    return 0.5 * (1 + tanh(0.5 * x));
} // 快速sigmoid函数

int cpp_postprocess(
    const float *output1,
    const float *output2,
    const float *output3,
    const int model_size,
    const int cls_num,
    const int img_w,
    const int img_h,
    const float score_threshold, // 0.4
    const float nms_threshold,   // 0.45

    float *nms_bboxes); // 后处理主代码

int postprocess_boxes(
    const float *pred_bbox,
    const int img_w,
    const int img_h,
    const int model_size,
    const int cls_num,
    const float score_threshold,

    float *bboxes);

void yolov5_decoder(
    const float *conv_output,
    const int size,
    const int cls_num,
    const int anchors[3][2],
    const int stride,

    float *decode_output); // yolov5解码

void decoder_son(
    const int j, const int j1, const int j2,
    const int data_size, const int stride, const int cls_num,
    const int anchors[3][2],
    const float *conv_output,
    float *decode_output); // decoder子函数，用于塞入多线程

int nms(
    const float *bboxes,
    const int bboxes_len,
    const int cls_num,
    const float iou_threshold,

    float *best_bboxes); // 非极大值抑制

void bboxes_iou(
    const float *boxes1,
    const float *boxes2,
    const int r,

    float *ious); // 计算iou

void pred_bbox_concatenate(
    const float *pred_bbox0,
    const float *pred_bbox1,
    const float *pred_bbox2,
    const int size,
    const int stride[3],
    const int cls_num,

    float *pred_bbox); // 仿写python-numpy concatenate函数

#endif
