# Fast postprocess
本项目是为了加速地平线`RDK X3`开发板中`Yolov5`模型运算速度而制作。  

在地平线开发板中，`Yolov5`模型直接部署于BPU运算会由于输出层算子不支持造成计算缓慢，需要去除输出解码层，将这部分运算移至后处理阶段。  

demo中的后处理使用python计算，速度不理想。  

本项目实现了C++后处理并使用Cython处理API，并利用多线程和多进程加速整个连续推理过程  。


## 文件简介
- postprocess.hpp   
  后处理算法的C++声明头文件  
  
- postprocess.cpp  
  后处理算法的C++功能实现代码
  
- yolov5_post.pyx  
  C和python之间的接口定义  
  
- setup.py  
  编译需要python代码  
  
- yolov5_test.py  
  加速推理使用demo  
  
- dominoes_new_yolov5.bin  
  去除输出解码层并使用地平线天工开物工具链量化的模型，用于检测有特殊几何体图片的骨牌目标  
  
- dominoes.jpg   
出现了骨牌的图片  

## 使用方法
一定要先安装Cython和各文件内相关库  

在本目录下键入`sudo python3 setup.py build_ext --inplace`以编译Cython，看见`Set up done`即完成（有冒出warning警告不要慌，问题不大）  

键入`sudo python3 yolov5_test.py`运行demo程序，程序会自动开启系统超频，然后使用目录下的模型，并使用加速的后处理代码，结合多进程，对目录下的照片进行推理计算100次，随后输出推理结果和耗时。  

demo不附带绘制框功能，有这方面需求的dever自行开发