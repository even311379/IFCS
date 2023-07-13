import subprocess
import sys
import datetime
import time

video_id = "6tj5ZdIJK_o"
clip_length = 15
target_dir = "L:/Free_Dev/YoutubeBackup"


if __name__ ==  "__main__":
    print("*"*100)
    print("Press Ctrl + C to stop".center(100, "*"))
    now = datetime.datetime.now()
    # need to reserve some seconds for task to start...
    reserve_time = 45 # reserve 45 second for task to start
    print('waiting to start...')
    if now.second < 60 - reserve_time:
        time.sleep(60 - reserve_time - now.second)
    else:
        time.sleep(60 - now.second + 60 - reserve_time)
    subprocess.Popen(f"{sys.executable} YoutubeDVR_Task.py --clip_length {clip_length} --video_id {video_id} --path {target_dir}")    
    print('task starts...')
    time.sleep(60 * (clip_length - now.minute % clip_length) - 5)
    while True:
        subprocess.Popen(f"{sys.executable} YoutubeDVR_Task.py --clip_length {clip_length} --video_id {video_id} --path {target_dir}")    
        print('task starts...')
        time.sleep(60 * clip_length)
        

