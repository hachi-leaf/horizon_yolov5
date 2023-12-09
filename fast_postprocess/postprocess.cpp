//----------------------------------------
//      主要函数实现
//                  Made by Leaf.
// ----------------------------------------

#include <iostream>
#include <limits>
#include "postprocess.hpp"
#include <chrono>
#include <thread>


using namespace std;


int cpp_postprocess(
    const float *model_output0, const float *model_output1, const float *model_output2,
    const int model_size, const int cls_num, const int img_w, const int img_h,
    const float score_threshold, const float nms_threshold, // 0.4 , 0.45
    float *nms_bboxes)
{

    const int
        anchors[3][3][2] = {10, 13, 16, 30, 33, 23, 30, 61, 62, 45, 59, 119, 116, 90, 156, 198, 373, 326},
        stride[3] = {8, 16, 32};

    float // 内存过大，只能用动态内存分配声明
        *pred_sbbox = new float[1 * (model_size / stride[0]) * (model_size / stride[0]) * (cls_num + 5) * 3],
        *pred_mbbox = new float[1 * (model_size / stride[1]) * (model_size / stride[1]) * (cls_num + 5) * 3],
        *pred_lbbox = new float[1 * (model_size / stride[2]) * (model_size / stride[2]) * (cls_num + 5) * 3];

    thread decoder_sbbox(yolov5_decoder, &model_output0[0], model_size, cls_num, anchors[0], stride[0], &pred_sbbox[0]);
    thread decoder_mbbox(yolov5_decoder, &model_output1[0], model_size, cls_num, anchors[1], stride[1], &pred_mbbox[0]);
    thread decoder_lbbox(yolov5_decoder, &model_output2[0], model_size, cls_num, anchors[2], stride[2], &pred_lbbox[0]); // 多线程，解码

    float *pred_bbox = new float[1 * (model_size / stride[0]) * (model_size / stride[0]) * (cls_num + 5) * 3 +
                                 1 * (model_size / stride[1]) * (model_size / stride[1]) * (cls_num + 5) * 3 +
                                 1 * (model_size / stride[2]) * (model_size / stride[2]) * (cls_num + 5) * 3]; // 动态内存分配

    decoder_sbbox.join();
    decoder_mbbox.join();
    decoder_lbbox.join(); // 等待3个线程结束

    pred_bbox_concatenate(&pred_sbbox[0], &pred_mbbox[0], &pred_lbbox[0],
                          model_size, stride, cls_num,
                          &pred_bbox[0]); // 将3个box合成

    delete[] pred_sbbox;
    delete[] pred_mbbox;
    delete[] pred_lbbox; // 释放内存

    float *bboxes = new float[6 * int(pow(model_size / stride[0], 2) +
                                      pow(model_size / stride[1], 2) +
                                      pow(model_size / stride[2], 2))]; // 由于bboxes_len长度不确定，为了防止内存分配过多，这里分配的空间是经验值

    int bboxes_len = postprocess_boxes(&pred_bbox[0], img_w, img_h, model_size, cls_num, score_threshold, &bboxes[0]);

    delete[] pred_bbox; // 释放内存

    int target_n = nms(&bboxes[0], bboxes_len, cls_num, nms_threshold, &nms_bboxes[0]); // 非极大值抑制，返回目标个数

    return target_n;
}

int postprocess_boxes(
    const float *pred_bbox,
    const int img_w, const int img_h,
    const int model_size, const int cls_num,
    const float score_threshold,
    float *bboxes)
{
    int boxes_len = 0;

    float inf = std::numeric_limits<float>::infinity();

    int len =
        1 * 3 * (model_size / 8) * (model_size / 8) +
        1 * 3 * (model_size / 16) * (model_size / 16) +
        1 * 3 * (model_size / 32) * (model_size / 32);

    float
        pred_xywh[4],
        pred_coor[4],
        bboxes_scale,
        pred_conf,
        *pred_prob = new float[cls_num],
        scores;

    int
        classes;

    float
        w_ratio = 1.0 * model_size / img_w,
        h_ratio = 1.0 * model_size / img_h,
        dw = (model_size - w_ratio * img_w) * 0.5,
        dh = (model_size - h_ratio * img_h) * 0.5;

    int max_index;
    float max_data;

    for (int i = 0; i < len; i++)
    {
        int index = i * (cls_num + 5);
        pred_xywh[0] = pred_bbox[index + 0];
        pred_xywh[1] = pred_bbox[index + 1];
        pred_xywh[2] = pred_bbox[index + 2];
        pred_xywh[3] = pred_bbox[index + 3];

        //(x, y, w, h) --> (xmin, ymin, xmax, ymax)
        pred_coor[0] = pred_xywh[0] - pred_xywh[2] * 0.5;
        pred_coor[1] = pred_xywh[1] - pred_xywh[3] * 0.5;
        pred_coor[2] = pred_xywh[0] + pred_xywh[2] * 0.5;
        pred_coor[3] = pred_xywh[1] + pred_xywh[3] * 0.5;

        pred_coor[0] = (pred_coor[0] - dw) / w_ratio;
        pred_coor[2] = (pred_coor[2] - dw) / w_ratio;
        pred_coor[1] = (pred_coor[1] - dh) / h_ratio;
        pred_coor[3] = (pred_coor[3] - dh) / h_ratio;

        // 修正坐标
        if (pred_coor[0] < 0.0)
            pred_coor[0] = 0.0;
        if (pred_coor[1] < 0.0)
            pred_coor[1] = 0.0;
        if (pred_coor[2] > img_w - 1)
            pred_coor[2] = img_w - 1;
        if (pred_coor[3] > img_h - 1)
            pred_coor[3] = img_h - 1;

        if (pred_coor[0] > pred_coor[2] ||
            pred_coor[1] > pred_coor[3])
            for (int j = 0; j < 4; j++)
                pred_coor[j] = 0;

        // 计算面积
        bboxes_scale = pow((pred_coor[2] - pred_coor[0]) * (pred_coor[3] - pred_coor[1]), 0.5);

        pred_conf = pred_bbox[i * (cls_num+5) + 4];

        // 寻找最大可能类
        max_index = 0;
        max_data = pred_bbox[i * (cls_num+5) + 5];
        for (int j = 0; j < cls_num; j++)
        {
            pred_prob[j] = pred_bbox[i * (cls_num+5) + j + 5];

            if (pred_prob[j] > max_data)
            {
                max_data = pred_prob[j];
                max_index = j;
            }
        }
        classes = max_index;

        scores = pred_conf * pred_prob[classes];

        if (bboxes_scale > 0 &&
            bboxes_scale < inf &&
            scores > score_threshold)
        {

            bboxes[boxes_len * 6 + 0] = pred_coor[0];
            bboxes[boxes_len * 6 + 1] = pred_coor[1];
            bboxes[boxes_len * 6 + 2] = pred_coor[2];
            bboxes[boxes_len * 6 + 3] = pred_coor[3];
            // cout << "test1" << endl;

            bboxes[boxes_len * 6 + 4] = scores;
            bboxes[boxes_len * 6 + 5] = classes;
            boxes_len++;
        }
    }
    return boxes_len;
}

void pred_bbox_concatenate(
    const float *pred_bbox0, const float *pred_bbox1, const float *pred_bbox2,
    const int size, const int stride[3], const int cls_num,
    float *pred_bbox)
{
    int
        len1 = 1 * 3 * (size >> 3) * (size >> 3) * (cls_num + 5),
        len2 = 1 * 3 * (size >> 4) * (size >> 4) * (cls_num + 5),
        len3 = 1 * 3 * (size >> 5) * (size >> 5) * (cls_num + 5);

    for (int i = 0; i < len1; i++)
        pred_bbox[i] = pred_bbox0[i];
    for (int i = 0; i < len2; i++)
        pred_bbox[i + len1] = pred_bbox1[i];
    for (int i = 0; i < len3; i++)
        pred_bbox[i + len1 + len2] = pred_bbox2[i];
}

int nms(
    const float *bboxes,
    const int bboxes_len,
    const int cls_num,
    const float iou_threshold,
    float *best_bboxes)
{

    float best_bbox[6];

    int best_bboxes_len = 0;

    int len_cls_bboxes;

    int *cls_list = new int[cls_num];
    for (int i = 0; i < cls_num; i++)
    {
        cls_list[i] = 0; // 获取类型列表
    }
    for (int i = 0; i < bboxes_len; i++)
    {
        cls_list[(int)bboxes[i * 6 + 5]]++; // 获取类型列表
    }

    for (int i = 0; i < cls_num; i++)
    { // 遍历图中类型
        if (cls_list[i] == 0)
            continue;

        float *cls_bboxes = new float[cls_list[i] * 6]; // cls_list[i]是该类型的数量
        int j = 0, j_ = 0;
        for (j = 0; j < bboxes_len; j++)
        {
            // cout << j << endl;
            if ((int)bboxes[j * 6 + 5] == i)
            {
                for (int k = 0; k < 6; k++)
                {
                    cls_bboxes[j_ * 6 + k] = bboxes[(j) * 6 + k]; // 只保留同一类的数据
                }
                j_++; // 长度
            }
        }

        len_cls_bboxes = j_;

        while (len_cls_bboxes > 0)
        {
            int max_index = 0;
            float max_prob = cls_bboxes[0 * 6 + 4];
            for (j = 0; j < len_cls_bboxes; j++)
            {
                if (max_prob < cls_bboxes[j * 6 + 4])
                {
                    max_index = j;
                    max_prob = cls_bboxes[j * 6 + 4];
                }
            }
            for (j = 0; j < 6; j++)
            {
                best_bbox[j] = cls_bboxes[max_index * 6 + j]; // 置信度最高的数据

                // 先加入 best_bboxes 中
                best_bboxes[(best_bboxes_len) * 6 + j] = best_bbox[j];
            }
            (best_bboxes_len)++;

            // 从cls_bboxes中删除这一行
            for (j = max_index; j < len_cls_bboxes; j++)
            {
                for (int k = 0; k < 6; k++)
                {
                    cls_bboxes[j * 6 + k] = cls_bboxes[(j + 1) * 6 + k];
                }
            }
            len_cls_bboxes--;

            // 计算iou
            float *iou = new float[len_cls_bboxes];

            bboxes_iou(&best_bbox[0], &cls_bboxes[0 * 6 + 0], len_cls_bboxes, &iou[0]);

            // 过滤cls_bboxes
            int len = 0;
            for (j = 0; j < len_cls_bboxes; j++)
            {
                int jndex = j * 6;
                int lendex = len * 6;
                if (cls_bboxes[j * 6 + 4] > 0 && iou[j] < iou_threshold)
                {
                    cls_bboxes[lendex + 0] = cls_bboxes[jndex + 0];
                    cls_bboxes[lendex + 1] = cls_bboxes[jndex + 1];
                    cls_bboxes[lendex + 2] = cls_bboxes[jndex + 2];
                    cls_bboxes[lendex + 3] = cls_bboxes[jndex + 3];
                    cls_bboxes[lendex + 4] = cls_bboxes[jndex + 4];
                    cls_bboxes[lendex + 5] = cls_bboxes[jndex + 5];
                    len++;
                }
            }
            len_cls_bboxes = len;
        }
    }
    return best_bboxes_len;
}

void bboxes_iou(
    const float *boxes1, const float *boxes2,
    const int r, float *ious)
{

    float boxes1_area = (boxes1[2] - boxes1[0]) * (boxes1[3] - boxes1[1]);
    float boxes2_area,
        left_up[2],
        right_down[2],
        inter_area,
        union_area;

    for (int i = 0; i < r; i++)
    {
        boxes2_area = (boxes2[i * 6 + 2] - boxes2[i * 6 + 0]) * (boxes2[i * 6 + 3] - boxes2[i * 6 + 1]);

        left_up[0] = max(boxes1[0], boxes2[i * 6 + 0]);
        left_up[1] = max(boxes1[1], boxes2[i * 6 + 1]);

        right_down[0] = min(boxes1[2], boxes2[i * 6 + 2]);
        right_down[1] = min(boxes1[3], boxes2[i * 6 + 3]);

        inter_area = max(right_down[0] - left_up[0], float(0.0)) * max(right_down[1] - left_up[1], float(0.0));

        union_area = boxes1_area + boxes2_area - inter_area;

        ious[i] = max(inter_area / union_area, numeric_limits<float>::epsilon());
        // cout << ious[i][0] <<endl;
    }
}

void yolov5_decoder(
    const float *conv_output,
    const int size, const int cls_num,
    const int anchors[3][2], const int stride,
    float *decode_output)
{

    int data_size = size / stride,
        j1, j2;

    for (int i = 0; i < 1; i++)
    {
        int j;
        j = 0;
        j1 = j * (cls_num + 5);
        j2 = j * data_size * data_size * (cls_num + 5);
        thread decoder0(decoder_son, j, j1, j2, data_size, stride, cls_num, anchors, &conv_output[0],
                        &decode_output[0]);

        j = 1;
        j1 = j * (cls_num + 5);
        j2 = j * data_size * data_size * (cls_num + 5);
        thread decoder1(decoder_son, j, j1, j2, data_size, stride, cls_num, anchors, &conv_output[0],
                        &decode_output[0]);

        j = 2;
        j1 = j * (cls_num + 5);
        j2 = j * data_size * data_size * (cls_num + 5);
        thread decoder2(decoder_son, j, j1, j2, data_size, stride, cls_num, anchors, &conv_output[0],
                        &decode_output[0]);

        decoder0.join();
        decoder1.join();
        decoder2.join();
    }
}

void decoder_son(
    const int j, const int j1, const int j2,
    const int data_size, const int stride, const int cls_num,
    const int anchors[3][2],
    const float *conv_output,
    float *decode_output)
{
    for (int k = 0; k < data_size; k++)
    {
        int k1 = k * data_size * 3 * (cls_num + 5) + j1;
        int k2 = k * data_size * (cls_num + 5) + j2;

        for (int l = 0; l < data_size; l++)
        {
            int l1 = l * 3 * (cls_num + 5) + k1;
            int l2 = l * (cls_num + 5) + k2;

            decode_output[l2 + 0] = (sigmoid(conv_output[l1 + 0]) * 2.0 - 0.5 + l) * stride;
            decode_output[l2 + 1] = (sigmoid(conv_output[l1 + 1]) * 2.0 - 0.5 + k) * stride;
            decode_output[l2 + 2] = pow(sigmoid(conv_output[l1 + 2]) * 2.0, 2) * anchors[j][0];
            decode_output[l2 + 3] = pow(sigmoid(conv_output[l1 + 3]) * 2.0, 2) * anchors[j][1];
            decode_output[l2 + 4] = sigmoid(conv_output[l1 + 4]);
        
            for (int m = 5; m < cls_num + 5; m++)
            {
            decode_output[l2 + m] = sigmoid(conv_output[l1 + m]);
            }
        }
    }
}
