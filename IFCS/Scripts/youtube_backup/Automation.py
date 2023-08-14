import subprocess
import sys
import datetime
import time
import os
import yaml



if __name__ ==  "__main__":
    with open("Config.yaml", 'r', encoding='utf8') as f:
        CONFIG = yaml.safe_load(f.read())  
    if not CONFIG:
        print("Error... Config file missing...")
    clip_length = CONFIG["ClipLength"]
    target_folder = CONFIG["TargetFolder"]
    cameras = CONFIG["Cameras"]
    if not os.path.exists(target_folder):
        os.mkdir(target_folder)
    for cam in cameras:
        if not os.path.exists(target_folder + "/" + cam["Name"]):
            os.mkdir(target_folder + "/" + cam["Name"])


    print("*"*100)
    print("Press Ctrl + C to stop".center(100, "*"))
    print("*"*100)
    clips_recording = 0
    while True:
        if clips_recording == 0:
            for cam in cameras:
                target_path = target_folder + "/" + cam["Name"]
                subprocess.Popen(f"{sys.executable} YoutubeDVR_Task.py --clip_length {clip_length} --video_id {cam['StreamID']} --path {target_path} --start_now", shell=True)    
                print("***One process is opened...***")
            clips_recording += 1
        else:
            now = datetime.datetime.now()
            if now.second == 0:            
                if (clip_length == 15 and now.minute%15 == 14) or (clip_length == 30 and now.minute%30 == 29) or (clip_length == 60 and now.minute == 59) or \
                    (clip_length == 120 and now.hour%2 == 1 and now.minute == 59):
                    for cam in cameras:
                        target_path = target_folder + "/" + cam["Name"]
                        subprocess.Popen(f"{sys.executable} YoutubeDVR_Task.py --clip_length {clip_length} --video_id {cam['StreamID']} --path {target_path}", shell=True)                
                    clips_recording += 1
        time.sleep(1)
        

