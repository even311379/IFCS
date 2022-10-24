K:/Anaconda/Scripts/activate.bat &^
conda activate yolov7 &^
cd "K:\YOLO\yolov7" &^
python train.py --workers 1 --device 0 --batch-size 32 --data data/fish.yaml  --img 640 640 --cfg cfg/training/yolov7.yaml --weights yolov7.pt  --name fish7 --hyp data/hyp.scratch.p5.yaml --epochs 1
