'''
Test if the DVR remote backup file format is the same as I expected?
'''
from datetime import datetime, timedelta
import random, re, os, cv2

target_date =  datetime(year=2023, month=5, day=14)
target_folder = "???"
dvr_name = "???"

# get file list
one_day_clips = sorted([target_folder + "/" + file for file in os.listdir(target_folder) 
    if dvr_name in file and target_date.strftime("%Y%m%d") in file and file.endswith('.video')])


# random pick 2 consecutive one
idx = random.randint(0, len(one_day_clips) - 1)

print(f'Test file is: {one_day_clips[idx]}')
print(f'Next file is: {one_day_clips[idx+1]}')

time1 = datetime.strptime(re.findall('-(\d+-\d+).video', one_day_clips[idx])[0], "%Y%m%d-%H%M%S")
time2 = datetime.strptime(re.findall('-(\d+-\d+).video', one_day_clips[idx + 1])[0], "%Y%m%d-%H%M%S")
delta_sec = (time2 - time1).seconds
print(f"gap seconds: {delta_sec}")

# check if frame size matched!
cap = cv2.VideoCapture(one_day_clips[idx])
fps = cap.get(cv2.CAP_PROP_FPS)

valid_frames = 0
failed_frames = 0
consecutive_failed_frames = 0
while True:
    ret, frame = cap.read()
    if ret:
        valid_frames += 1
        consecutive_failed_frames = 0 
    else:
        failed_frames += 1
        consecutive_failed_frames += 1
    if consecutive_failed_frames > 5:
        break

print(f"Total frames = {valid_frames}(valid) + {failed_frames - consecutive_failed_frames}(failed) ")
print(f"Estimated total frames: {delta_sec * fps}(fps * time gap in second) VS {valid_frames + failed_frames}(actual frames, valid + failed)")


'''

Is fps in every clip the same?
NEED TESTING!

'''
print("************************")
print("Test if fps in every clip the same...")
for clip in one_day_clips:
    cap = cv2.VideoCapture(clip)
    fps = cap.get(cv2.CAP_RPOP_FPS)
    print(f"{clip.ljust(50)} ... fps: {fps}")
    cap.release()
    