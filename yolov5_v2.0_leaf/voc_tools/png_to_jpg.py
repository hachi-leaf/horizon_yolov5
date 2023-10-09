import os
import shutil

file_src = r"C:\\Users\\zxy_y\\Desktop\\Dominoes" # 源图片位置
file_dst = r"\\wsl.localhost\\Ubuntu-22.04\\home\\leaf\\deve\\yolo\\voc\\Dominoes\\images\\train" # 输出jpg图片位置

images = os.listdir(file_src)

for image in images:
    shutil.copyfile(file_src+"\\"+image , file_dst+"\\"+image[:-4]+".jpg")
    print(image,"to jpg finally!")