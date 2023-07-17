import subprocess
import sys
import datetime
import time
import os

video_id = "zc8XYQnPj-4"
clip_length = 15
target_dir = "/home/even/文件/DEV/"


if __name__ ==  "__main__":
    print("*"*100)
    print("Press Ctrl + C to stop".center(100, "*"))
    print("*"*100)
    clips_recording = 0
    while True:
        if clips_recording == 0:
            subprocess.Popen(f"{sys.executable} YoutubeDVR_Task.py --clip_length {clip_length} --video_id {video_id} --path {target_dir} --start_now", shell=True)    
            clips_recording += 1
        else:
            now = datetime.datetime.now()
            if now.second == 0:            
                if (clip_length == 15 and now.minute%15 == 14) or (clip_length == 30 and now.minute%30 == 29) or (clip_length == 60 and now.minute == 59) or \
                    (clip_length == 120 and now.hour%2 == 1 and now.minute == 59):
                    subprocess.Popen(f"{sys.executable} YoutubeDVR_Task.py --clip_length {clip_length} --video_id {video_id} --path {target_dir}", shell=True)    
                    clips_recording += 1
        time.sleep(1)
        

