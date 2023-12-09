# Horizon_YOLOV5
本库开源与地平线相关的yolov5项目。  
This library is open source for the horizon-related yolov5 project.

## yolov5_v2.0_leaf
yolov5的修改版本，修改了大部分bug，删除了不相关的部分文件，网络结构与`yolov5 tag v2.0`相同，这是为了便于导出为`onnx`模型，使用地平线天工开物工具链量化为`bin`模型，部署在板端。  

该框架使用说明博客 已在地平线开发者社区发布：[【yolov5模型训练】面向小白！从配置环境，制作训练集，到训练模型——手把手教你使用yolov5框架训练自己的模型](https://developer.horizon.ai/forumDetail/185446371330059463)

后续还会发布量化为`bin`文件并部署上板推理相关代码和博客，敬请期待

## fast_postprocess
为了加速X3中`yolov5`推理计算，使用C++结合多线程重写了后处理代码，并用多进程使`主要占用BPU资源的推理计算`和`主要占用CPU资源的后处理计算`分离，并行运算，以及对系统进行超频，大大提高计算帧率。  
  
demo中：
- 模型尺寸：    
640x640  
- 模型类别数：  
6
- 推理帧数：    
约18.7 FPS