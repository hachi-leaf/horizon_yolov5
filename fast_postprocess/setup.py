# ----------------------------------------
#       编译成库文件
#                   Made by Leaf.
#----------------------------------------
from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
import numpy as np

try:
    numpy_include = np.get_include()
except AttributeError:
    numpy_include = np.get_numpy_include()

source_files = ["yolov5_post.pyx"]
ext_modules = [Extension("yolov5_post", source_files, language="c++",include_dirs = [numpy_include])]

setup(name="yolov5_post", ext_modules=cythonize(ext_modules))
print("Set up done! ")