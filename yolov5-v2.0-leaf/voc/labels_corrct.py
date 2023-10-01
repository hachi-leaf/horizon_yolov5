import os

path = r'E:\deve\yolo\yolov5-2.0\voc\dmn\labels\train' #标注文件目录，不同系统需要改斜杠

file = os.listdir(path)

for txtfile in file:
    f = open(path+'\\'+txtfile)
    lines = f.readlines()
    f.close()
    data = []
    open(path+'\\'+txtfile, 'w').close()
    with open(path+'\\'+txtfile,'a+',encoding='utf-8') as test:
        for line in lines:
            data_line = line[0:-1].split()
            x , y = float(data_line[1]) , float(data_line[2])
            w , h = float(data_line[3]) , float(data_line[4])
            xmin = x - w/2
            ymin = y - h/2
            xmax = x + w/2
            ymax = y + h/2
            if xmin<0:
                xmin=0
            if ymin<0:
                ymin=0
            if xmax>1:
                xmax=1
            if ymax>1:
                ymax=1
            newline = data_line[0]+' '+str((xmin+xmax)/2)+' '+str((ymin+ymax)/2)+' '+str(xmax-xmin)+' '+str(ymax-ymin)+'\n'
            test.write(newline)
