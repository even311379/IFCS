# IFCS - Integrated Fish Counting System

A tool set that include all necessary steps to establish an automatic fish counting system in fish way. It aims to make ecological researchers to easily establish their own object detection pipeline for their target species. It is designed for fish way, but should work perfectly in any similar use case. The core algorithm for object detection in this app is [yolo v7](https://github.com/WongKinYiu/yolov7). 

### Major features:
* Annotate on both image and directly on video clip
* Asset management system specialized for yolo v7 to help you create the best model.
* Count fish pass

Hope this will make your life easier. 
___
Although trying to minimize the required knowledge, you still need some basic understanding about **machine learning**.


### How to build in windows:

(1) Download premake and set its path properly (ex: in env path), then run gen_win_project.bat to generate project files.
(2) Download [OpenCV](https://opencv.org/releases/) (4.6), and put its prebuild files (xxx.exe, xxx.dll, xxx.pdb) to your env path. Those files are not included in the repository, but required in this project.
