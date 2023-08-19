import os
from vidgear.gears import CamGear
import cv2
import argparse
import datetime
import re
import time
from termcolor import colored
import logging
import sys
import subprocess


def abs_add_hour(in_time: datetime.datetime):
    out_time = in_time.replace(minute=0)
    return out_time + datetime.timedelta(hours=1)


def main():
    parser = argparse.ArgumentParser(
        description="Automate backup youtube streams to your local disk"
    )
    parser.add_argument(
        "--video_id",
        type=str,
        help="the video id for your target youtube video. /n \
        Take a random youtube video clip as example: \
        https://www.youtube.com/watch?v=lro-8GUX9Yk the ID is lro-8GUX9Yk",
    )
    parser.add_argument(
        "--clip_length",
        type=int,
        help="the length of each clips you want to store, should be 15, 30, 60, or 120",
        choices=[15, 30, 60, 120],
        default=60,
    )
    parser.add_argument(
        "--path",
        type=str,
        help="the absolute path to where you want to store the clips",
    )
    parser.add_argument(
        "--start_now",
        dest="start_now",
        action="store_true",
        help="start this task without wait",
    )
    parser.set_defaults(start_now=False)
    args = parser.parse_args()
    if not args.video_id or not args.path:
        print(colored("arguments are missing!", "red"))
        return
    logging.basicConfig(
        filename="youtube_backup_tasks.log",
        encoding="utf-8",
        level=logging.INFO,
        format="%(asctime)s %(message)s",
        datefmt="%Y/%m/%d %H:%M:%S",
    )
    video_id = args.video_id
    clip_length = args.clip_length
    path = args.path
    start_now = args.start_now
    options = {
        "STREAM_RESOLUTION": "720p",
    }
    stream = CamGear(
        source="https://youtu.be/" + video_id,
        stream_mode=True,
        logging=False,
        backend=cv2.CAP_GSTREAMER,
        **options,
    ).start()
    w = int(stream.stream.get(cv2.CAP_PROP_FRAME_WIDTH))
    h = int(stream.stream.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = stream.stream.get(cv2.CAP_PROP_FPS)
    print(colored(f"stream ready! info: w {w}, h {h}, fps {fps}", "green"))

    while fps > 62:
        print(colored("Something wrong in FPS... wait 10 seconds to retry...", "red"))
        time.sleep(10)
        stream = CamGear(
            source="https://youtu.be/" + video_id,
            stream_mode=True,
            logging=False,
            time_delay=1,
        ).start()
        w = int(stream.stream.get(cv2.CAP_PROP_FRAME_WIDTH))
        h = int(stream.stream.get(cv2.CAP_PROP_FRAME_HEIGHT))
        fps = stream.stream.get(cv2.CAP_PROP_FPS)
        print(colored(f"stream ready! info: w {w}, h {h}, fps {fps}", "green"))

    start_time = datetime.datetime.now()
    start_time = start_time.replace(second=0)
    # setup proper start + end time
    if clip_length == 15:
        if start_time.minute < 15:
            start_time = start_time.replace(minute=15)
        elif start_time.minute < 30:
            start_time = start_time.replace(minute=30)
        elif start_time.minute < 45:
            start_time = start_time.replace(minute=45)
        else:
            start_time = abs_add_hour(start_time)
    elif clip_length == 30:
        if start_time.minute < 30:
            start_time = start_time.replace(minute=30)
        else:
            start_time = abs_add_hour(start_time)
    elif clip_length == 60:
        start_time = abs_add_hour(start_time)
    elif clip_length == 120:
        start_time = start_time.replace(minute=0)
        if start_time.hour % 2 == 0:
            hour_mod = 2
        else:
            hour_mod = 1
        start_time = start_time + datetime.timedelta(hours=hour_mod)
    if start_now:
        end_time = start_time
        # make start time one minute later?
        start_time = datetime.datetime.now()
        start_time = start_time.replace(second=0)
        start_time = start_time + datetime.timedelta(minutes=1)
    else:
        end_time = start_time + datetime.timedelta(minutes=clip_length)

    time_info = (
        start_time.strftime("%Y%m%d%H%M") + "__" +
        end_time.strftime("%Y%m%d%H%M")
    )
    new_clip = f"{path}/{time_info}.mp4"
    writer = cv2.VideoWriter(
        new_clip, cv2.VideoWriter_fourcc(*"mp4v"), fps, (w, h))
    sleep_time = (start_time - datetime.datetime.now()).seconds
    print(f"    wait {sleep_time} sec to start (in order to match time?)")
    time.sleep(sleep_time)
    print(
        colored(
            "Start recording..."
            + f"{start_time.strftime('%Y%m%d%H%M')}"
            + f" to {end_time.strftime('%Y%m%d%H%M')}",
            "green",
        )
    )
    frames_written = 0
    minutes_recorded = 0
    corrupted_frames = 0
    frames_to_write = fps * (end_time - start_time).seconds
    while True:
        frame = stream.read()
        if frame is None:
            print(colored("ERROR".center(100, "*"), "red"))
            corrupted_frames += 1
            msg = f"EMPTY FRAME DETECTED ({corrupted_frames}) -- {path.split('/')[-1]}"
            msg = msg.center(50, " ").center(100, "*")
            print(colored(msg, "red"))
            if corrupted_frames > 5:
                logging.error("stream is crashed...")
                msg = "stream is crashed... open another recording task..."
                print(colored(msg, "red"))
                writer.release()
                end_time = datetime.datetime.now().strftime("%Y%m%d%H%M")
                mod_clip = new_clip.replace(
                    re.findall(r"__(.*)\.", new_clip)[0], end_time
                )
                os.rename(new_clip, mod_clip)
                print(
                    colored(f"current file is renames as {mod_clip}", "yellow"))
                logging.warning(f"rename current recording as {mod_clip}")
                # target_path = new_clip.replace(new_clip.split("/")[-1], "")
                task_command = f"{sys.executable} YoutubeDVR_Task.py \
                    --clip_length {clip_length} --video_id "
                subprocess.Popen(
                    task_command + f"{video_id} --path {path} --start_now",
                    shell=True,
                )
                return
            continue
        writer.write(frame)
        if frames_written > frames_to_write:
            print(
                colored(
                    "one clip is finished".center(
                        50, " ").center(100, "*"), "yellow"
                )
            )
            break
        if frames_written % (60 * fps) == 0:
            print(
                "    Recording ..."
                + f"{minutes_recorded}/{(end_time - start_time).seconds//60} minutes"
                + f" ({path.split('/')[-1]})"
            )
            minutes_recorded += 1
        frames_written += 1
    if corrupted_frames > 0:
        print(
            colored(
                f"{corrupted_frames} corrupted frames is found inside"
                + f"{path}/{time_info}.mp4...",
                "red",
            )
        )
        logging.error(
            f"{corrupted_frames} empty frame is inside this video, {path}/{time_info}.mp4"
        )
    else:
        logging.info(f"{new_clip} is completed without any issue...")
    print("Recording completed!")
    writer.release()


if __name__ == "__main__":
    main()
