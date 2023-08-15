import subprocess
import sys
import datetime
import time
import os
import yaml


def main():
    with open("Config.yaml", "r", encoding="utf8") as f:
        CONFIG = yaml.safe_load(f.read())
    if not CONFIG:
        print("Error... Config file missing...")
        return 0
    clip_length = CONFIG["ClipLength"]
    target_folder = CONFIG["TargetFolder"]
    cameras = CONFIG["Cameras"]
    if not os.path.exists(target_folder):
        os.mkdir(target_folder)
    for cam in cameras:
        if not os.path.exists(target_folder + "/" + cam["Name"]):
            os.mkdir(target_folder + "/" + cam["Name"])

    print("*" * 100)
    print("Press Ctrl + C to stop".center(30, " ").center(100, "*"))
    print("*" * 100)
    task_command = (
        f"{sys.executable} YoutubeDVR_Task.py --clip_length {clip_length} --video_id "
    )
    print("Dispatching first backup task...")
    for cam in cameras:
        target_path = target_folder + "/" + cam["Name"]
        subprocess.Popen(
            task_command +
            f"{cam['StreamID']} --path {target_path} --start_now",
            shell=True,
        )
    while True:
        now = datetime.datetime.now()
        if now.second == 0:
            should_start = False
            if clip_length == 15:
                should_start = now.minute % 15 == 14
            if clip_length == 30:
                should_start = now.minute % 30 == 29
            if clip_length == 60:
                should_start = now.minute == 59
            if clip_length == 120:
                should_start = now.hour % 2 == 1 and now.minute == 59
            if should_start:
                print("Dispatching backup tasks...")
                for cam in cameras:
                    target_path = target_folder + "/" + cam["Name"]
                    subprocess.Popen(
                        task_command +
                        f"{cam['StreamID']} --path {target_path}",
                        shell=True,
                    )
        time.sleep(1)


if __name__ == "__main__":
    main()
