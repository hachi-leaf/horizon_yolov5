import os
import shutil

file_srt = r"E:\deve\yolo\yolov5-2.0\voc\Dominoes\images\train_fe"
file_drt = r"E:\deve\yolo\yolov5-2.0\voc\Dominoes\images\train"

images = os.listdir(file_srt)

for image in images:
    shutil.copyfile(file_srt+"\\"+image , file_drt+"\\"+image[:-4]+".jpg")
    print(images,"to jpg finally!")