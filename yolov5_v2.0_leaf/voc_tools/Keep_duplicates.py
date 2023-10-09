import os

img_file = r'E:\deve\\yolo\\voc\dmn_new\dmn'
label_file = r'E:\deve\\yolo\\voc\dmn_new\\labels'

imgs = os.listdir(img_file)
labels = os.listdir(label_file)

print(imgs)
print(labels)

for img in imgs:
    if img[:-4] + '.txt' not in labels:
        os.remove(img_file+'\\'+img)
        print("rm:",img)

for label in labels:
    if label[:-4] + '.jpg' not in imgs:
        os.remove(label_file+'\\'+label)
        print("rm:",label)