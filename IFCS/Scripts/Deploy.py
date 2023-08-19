import os, sys, subprocess, re, datetime, time, atexit, random, shutil
import yaml
import logging
import requests
from IFCS_Deploy_old import notify_server_stopping_scheduled_task
from twilio.rest import Client
from twilio.base.exceptions import TwilioException
import cv2
from termcolor import colored
import pandas as pd
import individual_tracker as tracker

# import logging, requests, re, yaml, cv2, os, sys, atexit, random, subprocess

# import numpy as np
# import pandas as pd
# from statistics import mean
# from datetime import datetime, timedelta, time
# from twilio.rest import Client

# global vars
CONFIG = {}
CONFIG_PROBLEMS = {}

FAILURE_LIMIT = 5
DB_TIEM_FORMAT = "%Y-%M-%dT%H:%M:S+08:00"
CLIP_TIME_FORMAT = "%Y%m%d%H%M"
PASS_COUNT_FPS = 30
WEB_APP_SESSION = requests.Session()
SMS_SERVER = Client("Fake", "FakeSID")


def init_config():
    global CONFIG
    global CONFIG_PROBLEMS
    global SMS_SERVER

    # dir
    if not os.path.exists(f"{CONFIG['TaskOutputDir']}/Detections"):
        os.mkdir(f"{CONFIG['TaskOutputDir']}/Detections")
        os.mkdir(f"{CONFIG['TaskOutputDir']}/PassCounts")
        os.mkdir(f"{CONFIG['TaskOutputDir']}/Highlights")
        # TODO: let this be part of new model in my server
        analysis_event = dict(cam=[], model=[], event_date=[], analysis_type=[], analysis_time=[])
        fishway_utility_summary = dict(cam=[], fish=[], date=[], hour=[], count=[])
        pass_count_summary = dict(cam=[], fish=[], date=[], hour=[], count=[])
        pd.DataFrame(analysis_event).to_csv(f"{CONFIG['TaskInputDir']}/events_summary.csv")
        pd.DataFrame(fishway_utility_summary).to_csv(f"{CONFIG['TaskInputDir']}/fishway_utility_summary.csv")
        pd.DataFrame(pass_count_summary).to_csv(f"{CONFIG['TaskInputDir']}/fishway_pass_summary.csv")

    # check server status
    try:
        r = WEB_APP_SESSION.get(CONFIG["Server"])
    except Exception as e:
        CONFIG_PROBLEMS["Server"] = ["Server connection error!", str(e)]
        return False
    WEB_APP_SESSION.auth = (CONFIG["ServerAccount"], CONFIG["ServerPassword"])
    WEB_APP_SESSION.headers.update({"Content-Tyoe": "application/json"})

    # TODO: how to check my session is authenticated...

    # check if cam name is registered in server
    r = WEB_APP_SESSION.get(CONFIG["Server"] + "/api/camera_info")
    registered_cam_names = [cam["name"] for cam in r.json()["results"]]
    r = WEB_APP_SESSION.get(CONFIG["Server"] + "/api/target_fish_species")
    registered_fish_names = [fish["name"] for fish in r.json()["results"]]
    r = WEB_APP_SESSION.get(CONFIG["Server"] + "/api/detection_model_info")
    registered_model_names = [model["name"] for model in r.json()["results"]]

    for cam in CONFIG["Cameras"]:
        if cam["CameraName"] in registered_cam_names:
            if not os.path.exists(f"{CONFIG['TaskOutputDir']}/Detections/{cam['CameraName']}"):
                os.mkdir(f"{CONFIG['TaskOutputDir']}/Detections/{cam['CameraName']}")
                os.mkdir(f"{CONFIG['TaskOutputDir']}/PassCounts/{cam['CameraName']}")
                os.mkdir(f"{CONFIG['TaskOutputDir']}/Highlights/{cam['CameraName']}")

        else:
            CONFIG_PROBLEMS["Sever"] = [f"Camera name ({cam['CameraName']}) does not exist in server!"]
            return False
        if cam["ModelName"] not in registered_model_names:
            CONFIG_PROBLEMS["Sever"] = [f"Model name ({cam['ModelName']}) does not exist in server!"]
            return False

    # get category data and append it to config
    sys.path.append(CONFIG["YoloV7Path"])
    from models.experimental import attempt_load

    all_species = []
    for cam in CONFIG["Cameras"]:
        model = attempt_load(cam["ModelFilePath"])
        cam["Categories"] = model.module.names if hasattr(model, "module") else model.names
        # check if category name registered...
        all_species += cam["Categories"]
    for spe in set(all_species):
        if spe not in registered_fish_names:
            CONFIG_PROBLEMS["Server"] = [f"species name ({spe}) does not exist in server"]
            return False

    # test twilio info? how?
    SMS_SERVER = Client(CONFIG["TwilioSID"], CONFIG["TwilioAuthToken"])
    try:
        SMS_SERVER.messages.list()
        print("twilio auth success!")
    except TwilioException:
        print("twilio auth failedd...")
        CONFIG_PROBLEMS["SMS_PROBLEMS"] = ["Authentication failure! your SID or AuthToken is wrong!"]

    return False


def get_clips_from_date(clips_folder: str, query_date: datetime.date) -> list[str]:
    # TODO: should I sort?
    return [
        clips_folder + "/" + f
        for f in os.listdir(clips_folder)
        if datetime.datetime.strptime(f.split("__")[0], CLIP_TIME_FORMAT).date() == query_date
    ]


def perform_detection__fishway_utility(target_date, cam):
    global logger

    logger.log("   prepare raw clips...")
    # prepare clip
    detection_frequency = CONFIG["DetectionFrequency"]
    clips_to_detect = get_clips_from_date(f"{CONFIG['TaskInputDir']}/{cam['CameraName']}", target_date)
    work_folder = f"{CONFIG['TaskOutputDir']}/Detections/{cam['CameraName']}/{target_date.strftime(CLIP_TIME_FORMAT)}"
    writer = cv2.VideoWriter(work_folder + "/frames.mp4", cv2.VideoWriter_fourcc(*"mp4v"), 30)
    frames_info = open(work_folder + "/frames_info.txt", "w")
    for clip in clips_to_detect:
        clip_start_time = datetime.datetime.strptime(re.findall(r"/(.*)__", clip)[0], CLIP_TIME_FORMAT)
        cap = cv2.VideCapture(clip)
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
        fps = cap.get(cv2.CAP_PROP_FPS)
        frame_count = 0
        frame_count_written = 0
        while cap.isOpened():
            ret, frame = cap.read()
            if ret and frame_count % fps * detection_frequency == 0:
                writer.write(frame)
                time_info = clip_start_time + datetime.timedelta(seconds=frame_count_written * detection_frequency)
                frame_count_written += 1
                frames_info.write(time_info.strftime(CLIP_TIME_FORMAT + "%S") + "/n")
            frame_count += 1
    writer.release()
    frames_info.close()

    logger.log("   running yolov7 detection...")
    # run yolov7 detection
    ## TODO:test if subprocess may work?
    task = subprocess.Popen(
        [
            "python",
            "detect.py",
            "--weights",
            cam["ModelFilePath"],
            "--conf-thres",
            cam["Confidence"],
            "--iou-thres",
            cam["IOU"],
            "--img-size",
            cam["ImageSize"],
            "--project",
            work_folder,
            "--name",
            "detail",
            "--source",
            work_folder + "/frames.mp4",
            "--save-txt",
            "--save-conf",
            "--nosave",
        ],
        stdout=subprocess.DEVNULL,
    )
    task.wait()
    logger.log("   yolov7 complete")


# TODO: maybe I should edit the web app models now?
# there are so many property looks useless now...
def extract_fishway_utility_results(target_date, cam):
    # handle ROI?
    # collect data as csv format, save it locally
    work_folder = f"{CONFIG['TaskOutputDir']}/Detections/{cam['CameraName']}/{target_date.strftime(CLIP_TIME_FORMAT)}"
    out = dict(fish=[], count=[], detect_time=[])
    with open(work_folder + "/frames_info.txt", "r") as f:
        time_info = f.readlines()
    time_info = [datetime.datetime.strptime(t, CLIP_TIME_FORMAT + "%S\n") for t in time_info]

    def is_xy_in_roi(x, y):
        if cam["ShouldApplyDetectionROI"]:
            rx1, ry1, rx2, ry2 = cam["DetectionROI"]
            min_x = min(rx1, rx2)
            max_x = max(rx1, rx2)
            min_y = min(ry1, ry2)
            max_y = max(ry1, ry2)
            return min_x <= x <= max_x and min_y <= y <= max_y
        return True

    for f in os.listdir(work_folder + "/detail/labels"):
        detected_frame_id = re.findall(r"_(\d*)\.txt", f)[0]
        with open(work_folder + "/detail/labels" + f, "r") as label_file:
            labels = label_file.readlines()
            cats = []
            for label in labels:
                cat_id, x, y, _, _, _ = label[:-1].split(" ")
                if is_xy_in_roi(x, y):
                    cats.append(cam["Categories"][cat_id])
            for cat in set(cats):
                out["fish"].append(cat)
                out["count"].append(cats.count(cat))
            out["detect_time"] += [time_info[detected_frame_id]] * len(set(cats))
    df = pd.DataFrame(out)
    df.to_csv(work_folder + "/results.csv", index=False)
    df["hour"] = [t.hour for t in df["detect_time"]]
    df.drop("detect_time", axis=1, inplace=True)
    gdf = df.groupby(["fish", "hour"], as_index=False).mean()
    gdf["cam"] = [cam["CameraName"]] * len(gdf)
    gdf["date"] = [target_date] * len(gdf)
    gdf.to_csv(f"{CONFIG['TaskOutputDir']}/fishway_utility_summary.csv", mode="a", header=False)
    aout = dict(
        cam=[cam["CameraName"]],
        model=[cam["ModelName"]],
        event_date=[target_date],
        analysis_type=["FishwayUtility"],
        analysis_time=[datetime.datetime.now()],
    )
    pd.DataFrame(aout).to_csv(f"{CONFIG['TaskOutputDir']}/events_summary.csv", mode="a", header=False)


def perform_detection__fishway_pass(target_date, cam):
    """
    no need to consider special cases... just let it reduce to 720p and 30 fps
    # reduce to than 30 fps or the lowest fps across the clips
    # reduce to 720p or the lowest resolution across the clips...

    aggreagate to one single file
    randomly select X minutes contiguous block per hour

    """
    ## TODO: maybe to boost the performance... I can cut down the fps in this stage to 15 or 12?
    ## anyways, I'll just stick on 30fps for now... also need to consider the frame buffer size parameter...

    pass_count_duration = CONFIG["PassCountDuration"]
    clips = get_clips_from_date(f"{CONFIG['TaskInputDir']}/{cam['CameraName']}", target_date)
    work_folder = f"{CONFIG['TaskOutputDir']}/PassCounts/{cam['CameraName']}/{target_date.strftime(CLIP_TIME_FORMAT)}"
    block_info = open(work_folder + "/block_info.txt", "w")
    writer = cv2.VideoWriter(work_folder + "/blocks.mp4", cv2.VideoWriter_fourcc(*"mp4v"), PASS_COUNT_FPS)
    # get the per hour clip
    pass_count_clips = []
    for i in range(24):
        candidate_clips = []
        time_limit_lower = target_date + datetime.timedelta(hours=i)
        time_limit_upper = target_date + datetime.timedelta(hours=i + 1)
        short_clips = []  # grab clips that are just too short...
        for c in clips:
            clip_start_time = datetime.datetime.strptime(re.findall(r"/(.*)__", c)[0], CLIP_TIME_FORMAT)
            clip_end_time = datetime.datetime.strptime(re.findall(r"__(.*)\.mp4", c)[0], CLIP_TIME_FORMAT)
            if clip_end_time - clip_start_time < datetime.timedelta(minutes=pass_count_duration + 5):
                short_clips.append(c)
                continue
            if (
                clip_start_time >= time_limit_lower
                and clip_start_time + datetime.timedelta(minutes=pass_count_duration) < time_limit_upper
            ):
                candidate_clips.append(c)
        pass_count_clips.append(random.choice(candidate_clips))
        for c in short_clips:
            clips.remove(c)
        for c in candidate_clips:
            clips.remove(c)

    # get the real clip that will get cut into block!
    for clip in pass_count_clips:
        clip_start_time = datetime.datetime.strptime(re.findall(r"/(.*)__", clip)[0], CLIP_TIME_FORMAT)
        cap = cv2.VideoCapture(clip)
        fps = cap.get(cv2.CAP_PROP_FPS)
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
        total_frames = cap.get(cv2.CAP_PROP_FRAME_COUNT)
        pos = random.randrange(0, total_frames - fps * (pass_count_duration + 1))
        sample_time = clip_start_time + datetime.timedelta(seconds=pos // fps)
        cap.set(cv2.CAP_RPOP_POS_FRAMES, pos)
        # TODO: the fps will only be 60 or 30, this is ... really great!
        frame_count = 0
        frame_count_written = 0
        while cap.isOpened():
            ret, frame = cap.read()
            if ret and frame_count % fps // PASS_COUNT_FPS == 0:
                writer.write(frame)
                frame_count_written += 1
            if frame_count_written >= PASS_COUNT_FPS * pass_count_duration:
                break
            frame_count += 1
        block_info.write(sample_time.strftime(CLIP_TIME_FORMAT + "%S") + "\n")
    writer.release()
    block_info.close()

    # run yolov7 detection
    ## TODO:test if subprocess may work?
    task = subprocess.Popen(
        [
            "python",
            "detect.py",
            "--weights",
            cam["ModelFilePath"],
            "--conf-thres",
            cam["Confidence"],
            "--iou-thres",
            cam["IOU"],
            "--img-size",
            cam["ImageSize"],
            "--project",
            work_folder,
            "--name",
            "detail",
            "--source",
            work_folder + "/blocks.mp4",
            "--save-txt",
            "--save-conf",
            "--nosave",
        ],
        stdout=subprocess.DEVNULL,
    )
    task.wait()


def extract_fishway_pass_result(target_date, cam):
    work_folder = f"{CONFIG['TaskOutputDir']}/PassCounts/{cam['CameraName']}/{target_date.strftime(CLIP_TIME_FORMAT)}"
    block_info = [
        datetime.datetime.strptime(b, CLIP_TIME_FORMAT + "%S\n")
        for b in open(work_folder + "/block_info.txt", "r").readlines()
    ]
    out = dict(fish=[], enter_time=[], leave_time=[])

    for h in range(24):
        # load labels files in that hour
        label_frame_offset = CONFIG["PassCountDuration"] * PASS_COUNT_FPS * h
        loaded_lables = {}
        for file in os.listdir(work_folder + "/detail/labels"):
            frame_num = int(re.findall(r"_(\d*)\.txt", file)[0])
            if (
                frame_num < label_frame_offset
                or frame_num >= label_frame_offset + CONFIG["PassCountDuration"] * PASS_COUNT_FPS
            ):
                continue
            loaded_lables[frame_num - label_frame_offset] = tracker.label_file_to_list(
                work_folder + "/detail/labels/" + file
            )

        # cconvert to individual
        individuals = tracker.track_individual(cam, loaded_lables)
        for ind in individuals:
            if ind.is_completed:
                out["fish"].append(ind.get_name(cam))
                out["enter_time"].append(
                    block_info[h] + datetime.timedelta(seconds=ind.get_enter_frame_num() / PASS_COUNT_FPS)
                )
                out["leave_time"].append(
                    block_info[h] + datetime.timedelta(seconds=ind.get_enter_leave_num() / PASS_COUNT_FPS)
                )
    df = pd.DataFrame(out)
    df.to_csv(work_folder + "/results.csv", index=False)
    df["hour"] = [t.hour for t in df["leave_time"]]
    gdf = df.groupby(["fish", "hour"], as_index=False).count()
    gdf["cam"] = [cam["CameraName"]] * len(gdf)
    gdf["date"] = [target_date] * len(gdf)
    gdf.to_csv(f"{CONFIG['TaskOutputDir']}/fishway_pass_summary.csv", mode="a", header=False)
    aout = dict(
        cam=[cam["CameraName"]],
        model=[cam["ModelName"]],
        event_date=[target_date],
        analysis_type=["FishwayPass"],
        analysis_time=[datetime.datetime.now()],
    )
    pd.DataFrame(aout).to_csv(f"{CONFIG['TaskOutputDir']}/events_summary.csv", mode="a", header=False)


def send_results_to_server(target_date, cam):
    pass


# collect 1 region with highest fishway utility, 3 minutes long!
def collect_highlight(target_date, cam):
    global logger
    utility_result = (
        f"{CONFIG['TaskOutputDir']}/Detections/{cam['CameraName']}/{target_date.strftime(CLIP_TIME_FORMAT)}/results.csv"
    )
    us = pd.DataFrame(utility_result).groupby(["detect_time"], as_index=False).sum()
    if max(us.count) == 0:
        logger.log(f"    skip highlight since there is no any detection! {cam} - {target_date}")
        return
    max_time = us[us["count"] == max(us["count"])].detect_time.iloc[0]
    clips_folder = CONFIG["TaskInputDir"]+cam["CameraName"]
    hightlight_name = f"{CONFIG['TaskOutputDir']}/Highlights/{cam['CameraName']}_{target_date.strptime(CLIP_TIME_FORMAT)}.mp4}
    for clip in os.listdir(clips_folder):
        clip_start_time = datetime.datetime.strptime(re.findall(r"/(.*)__", clip)[0], CLIP_TIME_FORMAT)
        clip_end_time = datetime.datetime.strptime(re.findall(r"__(.*)\.mp4", clip)[0], CLIP_TIME_FORMAT)
        if clip_start_time <= max_time <=clip_end_time:
            if clip_end_time - clip_start_time <= datetime.timedelta(minutes=3):
                shutil.copy(clips_folder+"/"+clip, hightlight_name)
            else:
                cap = cv2.VideCapture(clips_folder+"/"+clip)
                fps = cap.get(cv2.CAP_PROP_FPS)
                writer = cv2.VideoWriter(hightlight_name, cv2.VideoWriter_fourcc(*"mp4v"), fps)
                if clip_end_time - max_time > datetime.timedelta(seconds=90):                    
                    start_pos = ((max_time - clip_end_time).seconds - 90) * fps 
                else:
                    start_pos = ((clip_end_time - datetime.timedelta(seconds=180)) - clip_start_time).seconds * fps
                cap.set(cv2.CAP_PROP_FRAME_COUNT)
                frames_written = 0
                while cap.isOpend():
                    ret, frame = cap.read()
                    writer.write(frame)
                    frames_written += 1
                    if frames_written > 300:
                        break
                writer.release()
            break

def SMS_notify():
    pass


def run_tasks(target_date):
    global logger
    for cam in CONFIG["Cameras"]:
        os.mkdir(CONFIG["TaskOutputDir"] + "/Detections/" + target_date.strftime(CLIP_TIME_FORMAT))
        os.mkdir(CONFIG["TaskOutputDir"] + "/PassCounts/" + target_date.strftime(CLIP_TIME_FORMAT))
        logger.log(f"Start fishway utility analysis {cam['CameraName']} - {target_date}")
        perform_detection__fishway_utility(target_date, cam)
        extract_fishway_utility_results(target_date, cam)
        logger.log(f"Analysis complete {cam['CameraName']} - {target_date}")
        logger.log(f"Start fishway pass analysis {cam['CameraName']} - {target_date}")
        perform_detection__fishway_pass(target_date, cam)
        extract_fishway_pass_result(target_date, cam)
        logger.log(f"Analysis complete {cam['CameraName']} - {target_date}")
        send_results_to_server(target_date, cam)
        logger.log(f"Send date to server complete {cam['CameraName']} - {target_date} - fishway utility")
        collect_highlight(target_date, cam)
        logger.log(f"Collect highlight complete {cam['CameraName']} - {target_date}")


def one_time_task():
    # TODO: need to check if the date has already processed...
    # if so... maybe double check with the client and provide some options to handle
    # such as ... remove the date data in server completely? and ???
    global logger
    dates_to_run = []
    if CONFIG["IsRunSpecifiedDates"]:
        for d in CONFIG["RunDates"]:
            dates_to_run.append(datetime.datetime(year=d["Year"] + 1900, month=d["Month"] + 1, day=d["Day"]))
    else:
        sd = CONFIG["StartDate"]
        ed = CONFIG["EndDate"]
        start_date = datetime.date(year=sd["Year"] + 1900, month=sd["Month"] + 1, day=sd["Day"])
        end_date = datetime.date(year=ed["Year"] + 1900, month=ed["Month"] + 1, day=ed["Day"])
        d = start_date
        while d <= end_date:
            dates_to_run.append(d)
            d += datetime.timedelta(days=1)
    for d in dates_to_run:
        for cam in CONFIG["Cameras"]:
            cam_name = cam["CameraName"]
            if os.path.exists(CONFIG["TaskOutputDir"] + "/Detections/" + cam_name + d.strftime(CLIP_TIME_FORMAT)):
                msg = f"detection analysis was performed on {cam_name} {d}, you can not just rerun it!"
                logger.error(msg)
                print(colored("Error, " + msg, "red"))
                return
            if os.path.exists(CONFIG["TaskOutputDir"] + "/PassCounts/" + cam_name + d.strftime(CLIP_TIME_FORMAT)):
                msg = f"pass count analysis was performed on {cam_name} {d}, you can not just rerun it!"
                logger.error(msg)
                print(colored("Error, " + msg, "red"))
                return
    for d in dates_to_run:
        run_tasks(d)


def schedule_task():
    global logger

    # notify server task is started...

    def notify_server_stopping_scheduled_task():
        pass

    atexit.register(notify_server_stopping_scheduled_task)

    ## TODO: send startup SMS?

    while True:
        now = datetime.datetime.now()
        run_hour, run_minute = CONFIG["ScheduledRuntime"]
        sms_hour, sms_minute = CONFIG["SMS_SendTime"]
        if now.hour == run_hour and now.minute == run_minute:
            target_date = now.date() - datetime.timedelta(days=1)
            run_tasks(target_date)

        if now.hour == sms_hour and now.minute == sms_minute:
            SMS_notify()
        time.sleep(60)


if __name__ == "__main__":
    with open("IFCS_DeployConfig.yaml", "r", encoding="utf8") as f:
        CONFIG = yaml.safe_load(f.read())
    logging.basicConfig(
        filename=CONFIG["TaskOutputDir"] + "/Deploy.log",
        encoding="utf-8",
        level=logging.INFO,
        format="%(asctime)s %(message)s",
        datefmt="%Y/%m/%d %H:%M:%S",
    )
    # exclude logger from other modules... so annoying...
    # loggers from yolov7? still exists....
    for name, logger in logging.root.manager.loggerDict.items():
        logger.disabled = True

    logger = logging.getLogger(__name__)
    is_init_sucess = init_config()
    if is_init_sucess:
        # TODO: init per day output dir?
        if CONFIG["IsTaskStartNow"]:
            one_time_task()
        else:
            schedule_task()
    else:
        for k in CONFIG_PROBLEMS:
            print("*" * 100)
            print(colored((k + " ERROR").center(25, " ").center(100, "*"), "red"))
            for v in CONFIG_PROBLEMS[k]:
                print("    " + v)
                logger.error(k + " ERROR: " + v)
            print("*" * 100)
