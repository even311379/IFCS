import time, logging, requests, re, yaml, cv2, os, sys, atexit, random, subprocess
import numpy as np
import pandas as pd
from statistics import mean
from datetime import datetime, timedelta, time
from twilio.rest import Client


'''
send the passwords, sid, phone number to my client with a encrypted pdf file...


TODO: Domain name is missing in config_data... (ServerUrl)
it will be: Server: http://127.0.0.1:8000  ?

TODO: Fish Cat ID -> Fish Name is not provided in config data
it will be in each camera info: (just grab model info and output it to config)
Categories: ["fish0", "fish1", ...]

TODO: Add new function for config data <-> server check... check if the neccessary info in server is correct and matched

BackupMinimumTime

'''

# global vars
CONFIG = {}

FAILURE_LIMIT = 5 
DB_TimeFormat = "%Y-%M-%dT%H:%M:S+08:00"
Session = requests.Session()


def SendMessage(twilio_account_sid, twilio_auth_token, twilio_phone_number, to_phone_number, message):
    client = Client(twilio_account_sid, twilio_auth_token)
    message = client.messages.create(
         body=message,
         from_ =  twilio_phone_number,
         to = to_phone_number
    )
    message.sid # message 


def get_datetime_from_clip_file(clip_file_name):
    return datetime.strptime(re.findall('-(\d+-\d+).video', clip_file_name)[0], "%Y%m%d-%H%M%S")

def get_frame_safe(in_cap):
    read_times = 0
    while True:
        ret, frame = in_cap.read()
        if ret:
            return read_times, frame
        else:
            read_times += 1
            if read_times > FAILURE_LIMIT:
                return 0, []

def label_file_to_df(file_path):    
    out = dict(CatID=[], X=[], Y=[], W=[], H=[], Conf=[])

    with open(file_path, "r") as f:
        content = f.read()
    for l in content.split("\n")[:-1]:
        d = l.split(" ")
        out["CatID"].append(int(d[0]))
        out["X"].append(float(d[1]))
        out["Y"].append(float(d[2]))
        out["W"].append(float(d[3]))
        out["H"].append(float(d[4]))
        out["Conf"].append(float(d[5]))
    return pd.DataFrame(out)



'''
to make things easier... 
just rewrite my c++ version code to python..
'''

class LabelData:

    def __init__(self, cat_id: int, x: float, y: float, w: float, h: float, conf: float):
        self.cat_id = cat_id
        self.x = x
        self.y = y
        self.w = w
        self.h = h
        self.conf = conf

    def get_distance(self, other, width, height) -> float:
        '''
        the real width, height size in pixel is actaully of no meaning...
        since you can adjust resolution as you want.
        ..
        however, the w-h ratio means a lot
        '''
        return (((self.x - other.x) * width)**2 + ((self.y - other.y)*height)**2)**0.5

    def get_approx_body_size(self, width, height) -> float:
        '''
        return body length and height!

        *** the bigger one is body length!! ***
        '''
        bl = max(self.w * width, self.y * height)
        bh = min(self.w * width, self.y * height)
        return bl, bh


class Individual:

    def __init__(self, frame_num: int = -1, label_data:LabelData = -1):
        self.info = {} #dict[int, LabelData]
        self.is_completed = False
        self.has_picked = False
        if frame_num != -1 and label_data != -1: # trick to make costructor overload?
            self.info[frame_num] = label_data

    def get_enter_frame(self):
        return sorted(self.info)[0]

    def get_leave_frame(self):
        return sorted(self.info)[-1]

    def get_approx_body_size(self, width, height):
        bl = []
        bh = []
        for data in self.info.values():
            l,h = data.get_approx_body_size(width, height)
            bl.append(l)
            bh.append(h)
        return mean(bl), mean(bh)

    def get_approx_speed(self, width, height):
        if len(self.info) == 1:
            return 0
        S = []
        i = 0
        last_frame = 0
        last_label = 0
        for frame_num, label in self.info.items():
            if i != 0:
                S.append(label.get_distasnce(last_label, width, height) / (frame_num - last_frame))
            last_frame = frame_num
            last_label = label
            i += 1
        return mean(S)

    def get_name(self, camera_info):
        cat_confs = {} # dict[str, list[float]]
        s = 0
        for frame_num, label in self.info.items():
            if label.cat_id + 1 > len(camera_info["Categories"]):
                return "Error"
            if label.conf < camera_info["SpeciesDetermineThreshold"]:
                continue
            category_name = camera_info["Categories"][label.cat_id]
            if category_name not in cat_confs:
                cat_confs[category_name] = []
            cat_confs[category_name].append(label.conf)
            s += 1
        if s == 0:
            return "Uncertain"
        
        best_fit_name = ""
        max_pass = 999
        have_same_best_fit = False
        equal_cats = []
        for cat, confs in cat_confs.items():
            if len(confs) == max_pass:
                equal_cats.append(cat)
                have_same_best_fit = True
            elif len(confs) < max_pass:
                max_pass = len(confs)
                best_fit_name = cat
                have_same_best_fit = False
                equal_cats = []
        if not have_same_best_fit:
            return best_fit_name
        max_mean_conf = 0
        for cat in equal_cats:
            mean_conf = mean(cat_confs[cat])
            if mean_conf > max_mean_conf:
                max_mean_conf = mean_conf
                best_fit_name = cat

        return best_fit_name

    def add_info(self, new_frame_num: int, new_label_data: LabelData):
        self.info[new_frame_num] = new_label_data


def label_file_to_list(file_path):
    out = []
    with open(file_path, "r") as f:
        content = f.read()
    for l in content.split("\n")[:-1]:
        d = l.split(" ")
        out.append(LabelData(int(d[0]), float(d[1]), float(d[2]), float(d[3]), float(d[4]), float(d[5])))
    return out


def track_individual(labels_folder, camera_info, clip_width, clip_height) -> list[Individual]:

    # local functions...
    def is_size_similar(label1, label2, frame_diff):
        if not CONFIG["ShouldEnableBodySizeThreshold"]:
            return True
        bl1, bh1 = label1.get_approx_body_size(clip_width, clip_height)
        s1 = bl1 * bh1
        bl2, bh2 = label2.get_approx_body_size(clip_width, clip_height)
        s2 = bl2 * bh2
        th = camera_info["BodySizeThreshold"]
        return s1 < s2*(1+th)**(frame_diff+1) and s1 > s2*(1-th)**(frame_diff+1) or \
            s2 < s1*(1+th)**(frame_diff+1) and s2 > s1*(1-th)**(frame_diff+1)

    def is_distance_acceptable(label1, label2, frame_diff):
        if not camera_info["ShouldEnableSpeedThreshold"]:
            return True
        distance = label1.get_distance(label2, clip_width, clip_height)
        return distance / frame_diff / (clip_width**2 + clip_height**2)**0.5 < camera_info["SpeedThreshold"]

    individual_data=[]
    min_fishway_pos = min(CONFIG["FishwayStartEnd"])
    max_fishway_pos = max(CONFIG["FishwayStartEnd"])
    fishway_start, fishway_end = CONFIG["FishwayStartEnd"]
    loaded_labels = {}
    for file in labels_folder:
        num = re.findall(file, "-(\d+).txt")
        loaded_labels[num] = label_file_to_list(file)

    loaded_labels = dict(sorted(loaded_labels.items())) # sort dict...
    temp_indivial_data = [] #list[Individual] # list of individual
    for frame_num, labels in loaded_labels.items():
        for label in labels:
            is_in_fishway = False
            is_in_leaving_area = False
            if camera_info["IsPassVertical"]:
                is_in_fishway = (label.y >= min_fishway_pos) and (label.y <= max_fishway_pos)
                is_in_leaving_area = fishway_end if fishway_start > fishway_end else label.y > fishway_end
            else:
                is_in_fishway = (label.x >= min_fishway_pos) and (label.x <= max_fishway_pos)
                is_in_leaving_area = label.x < fishway_end if fishway_start > fishway_end else label.x > fishway_end
            
            if is_in_fishway:
                if not temp_indivial_data:
                    temp_indivial_data.append(Individual(frame_num, label))
                    temp_indivial_data[0].has_picked = True
                else:
                    closest_idx = -1
                    closest_distance = 999999
                    i = 0
                    for data in temp_indivial_data:
                        if data.has_picked: 
                            continue
                        last_frame_num = sorted(data.info)[-1]
                        last_label = data.info[last_frame_num]
                        distance = last_label.get_distance()
                        if is_size_similar(last_label, label, frame_num - last_frame_num) and \
                                is_distance_acceptable(last_label, label, frame_num - last_frame_num) and \
                                distance <= closest_distance:
                            closest_idx = i
                            closest_distance = distance                            
                        i += 1
                    if closest_idx == -1:
                        temp_indivial_data.append(Individual(frame_num, label))
                        temp_indivial_data[0].has_picked = True
                    else:
                        temp_indivial_data[closest_idx].add_info(frame_num, label)
                        temp_indivial_data[closest_idx].has_picked = True
            elif is_in_leaving_area:
                closest_idx = -1
                closest_distance = 999999
                i = 0
                for data in temp_indivial_data:
                    last_frame_num = sorted(data.info)[-1]
                    last_label = data.info[last_frame_num]
                    distance = last_label.get_distance()
                    if is_size_similar(last_label, label, frame_num - last_frame_num) and is_distance_acceptable(last_label, label, frame_num - last_frame_num) \
                        and distance <= closest_distance:
                        closest_idx = i
                        closest_distance = distance
                    i += 1
                if closest_idx != -1:
                    temp_indivial_data[closest_idx].has_picked = True
                    temp_indivial_data[closest_idx].is_completed = True

        for data in temp_indivial_data:
            data.has_picked = False
            if len(data.info) == 1:
                continue
            last_frame_num = sorted(data.info)[-1]
            if last_frame_num + camera_info["FrameBufferSize"] < frame_num or data.is_completed:
                individual_data.append(data)
        temp_indivial_data = [data for data in temp_indivial_data 
                              if not data.is_completed and sorted(data.info)[-1] + CONFIG["FrameBufferSize"] > frame_num]

    # make df would be better
    df = dict(
        category=[],
        approx_speed=[],
        approx_body_length=[],
        approx_body_height=[],
        enter_frame=[],
        leave_frame=[]
    )    
    for data in individual_data:
        if not data.is_completed: 
            continue
        df["category"].append(data.get_name())
        df["approx_speed"].append(data.get_approx_speed())
        bl, bh = data.get_approx_body_size()
        df["approx_body_length"].append(bl)
        df["approx_body_height"].append(bh)
        df["enter_frame"].append(data.get_enter_frame())
        df["leave_frame"].append(data.get_leave_frame())
    return pd.DataFrame(df)
        

def add_detection_analysis_to_server(target_date):
    '''
    Add 1 for each camera
    '''
    data = []
    analysis_time = datetime.now().strftime(DB_TimeFormat)
    for C in CONFIG["Cameras"]:
        d = {}
        d["camera"] = C["CameraName"]
        if CONFIG["IsDaytimeOnly"]:
            d["event_time"] = (target_date + timedelta(hours=CONFIG["DetectionStartTime"])).strftime(DB_TimeFormat)
            d["event_period"] = time(CONFIG["DetectionEndTime"] + 12 - CONFIG["DetectionStartTime"]).strftime("%H:%M:%S")
        else:
            d["event_time"] = target_date.strftime(DB_TimeFormat)
            d["event_period"] = "23:59:59"
        d["analysis_type"] = "DE"
        d["analysis_time"] = analysis_time 

        '''
        TODO: consider this twice... should this ALWAYS be true? 
        each detection result from this analysis has independt infeasible test result...
        how does that relate to the whole analysis?
        '''
        d["can_analysis"] = "true"
        d["analysis_log"] = "created by automation task"

        data.append(d)
    Session.post(f'{CONFIG["Server"]}/api/fish_analysis', json=data)
    

def add_infeasible_detection_to_server(analysis_event_time, detection_time, camera):
    '''
    TODO: need test...
    make sure "infeasible" fish species is registered!!
    '''
    d = dict(
        analysis = f'{camera["CameraName"]},{camera["ModelName"]},{analysis_event_time.strftime("%Y-%m-%d %H:%M:%S")}',
        fish = "無法辨識",
        count = 0,
        detect_time = detection_time.strftime("%H:%M:%S"),
        can_detect = "false"
    )
    Session.post(f'{CONFIG["Server"]}/api/fish_detection', json=[d])

# def SendDetectionResult():
#     pass

# def SendZeroDetection():
#     pass

def get_rgb_from_u32(u32_color):
    b = u32_color & 255
    g = (u32_color >> 8) & 255
    r = (u32_color >> 16) & 255
    return (r,g,b)
    

def is_feasibility_test_passed(frame_data, camera_info):
    height, width, _ = frame_data.shape    
    for zone in camera_info["FeasibleZones"]:
        x,y,w,h = zone["XYWH"]
        ROI = frame_data[ int((x-0.5*w)*width): int((x+0.5*w)*width), int((y-0.5*h)*height):int((y+0.5*h)*height),:]
        ## TODO: check BGR RGB issue... 
        AvgColor = np.mean(np.mean(ROI,axis=0),axis=0) # this is BGR!!
        low_color = get_rgb_from_u32(zone["ColorLower"]) # RGB
        upper_color = get_rgb_from_u32(zone["ColorUpper"]) #RGB
        if low_color[0] <= AvgColor[2] and upper_color[0] >= AvgColor[2] and low_color[1] <= AvgColor[1] and upper_color[1] >= AvgColor[1] \
            and low_color[2] <= AvgColor[0] and upper_color[2] >= AvgColor[0]:
                return True        
    return False

def config_checking():
    '''
    Check config data is perfectly setup!
    Check if the data in server is perfectly setup!

    required fish species: 無法辨識 空白

    '''

    return False


def combine_video(target_date):
    '''
    combine frames which need to detect into one video based on the settings...

    infeasible detection is cleared here, and relavant info is stored to further matching
    
    multiple per frame clips per hour

    *** CORRUPTED VIDEO ***
    there are missing frames in each clip, basic video parameters (total frame count) are not reachable by opencv. 
    Some weird logic is applied to handle it. 

    By this ASSUMPTION:
        each clip is actually concat to each other

    '''
    #the max allowed frame size for failure to load frame, maybe its the end of the clip? since I can't use normal way to check if video is finished...
    DETECTION_FREQ = CONFIG["DetectionFrequency"]
    IN_DIR = CONFIG["TaskInputDir"]
    OUT_DIR = CONFIG["TaskOutputDir"]

    CAP = cv2.VideoCapture()
    VID_WRITER = cv2.VideoWriter()

    event_start_time = target_date
    if CONFIG["IsDaytimeOnly"]:
        event_start_time += timedelta(hours=CONFIG["DetectionStartTime"])


    for camera_info in CONFIG["Cameras"]:        
        # prepare clips
        one_day_clips = sorted([file for file in os.listdir(CONFIG["TaskInputDir"]) if camera_info['DVRName'] in file and target_date.strftime("%Y%m%d") in file and file.endswith('.video')])
        if CONFIG["IsDaytimeOnly"]:
            temp = one_day_clips.copy()
            one_day_clips = []
            event_end_time = target_date + timedelta(hours=CONFIG["DetectionEndTime"]+12)
            sid ,eid = 0
            for i in range(len(temp) - 1):
                if get_datetime_from_clip_file(temp[i]) < event_start_time < get_datetime_from_clip_file(temp[i+1]):
                    sid = i
                if get_datetime_from_clip_file(temp[i]) > event_end_time:
                    eid = i
                    break
            for i in range(sid, eid+1):
                one_day_clips.append(temp[i])

        # get the basic video parameters
        CAP.open(f"{IN_DIR}/{one_day_clips[0]}")
        FPS = CAP.get(cv2.CAP_PROP_FPS)
        WIDTH = int(CAP.get(cv2.CAP_PROP_FRAME_WIDTH))
        HEIGHT = int(CAP.get(cv2.CAP_PROP_FRAME_HEIGHT))
        CAP.release()

        VID_WRITER.open(f"{OUT_DIR}/Temp/{camera_info['CameraName']}_{target_date.strftime('%Y-%m-%d')}_Detection.mp4",
                                        cv2.VideoWriter_fourcc(*'mp4v'), 30, (WIDTH, HEIGHT)) # fps doesn't matter here.. just set it 30... 
        frames_info_file = open(f"{OUT_DIR}/Temp/{camera_info['CameraName']}_{target_date.strftime('%Y-%m-%d')}_frames_info.txt", "w")

        if CONFIG["IsDaytimeOnly"]:
            # handle the first video...
            video_start_time = get_datetime_from_clip_file(one_day_clips[0])
            start_frame_pos = (datetime(video_start_time.year, video_start_time.month, video_start_time.day, video_start_time.hour + 1) - video_start_time).seconds * FPS
            CAP.open(f"{IN_DIR}/{one_day_clips[0]}")
            saved_frame_count = 0
            is_frame_pos_valid = 1
            while is_frame_pos_valid:
                frame_pos = start_frame_pos + DETECTION_FREQ * saved_frame_count
                CAP.set(cv2.CAP_PROP_POS_FRAMES, frame_pos)
                is_frame_pos_valid, frame = get_frame_safe(CAP)
                if is_frame_pos_valid:
                    if is_feasibility_test_passed(frame):
                        VID_WRITER.write(frame)
                        time_str = (video_start_time + timedelta(seconds=frame_pos/FPS)).strftime("%H:%M:%S")           
                        frames_info_file.write(time_str+'\n')
                    else:
                        add_infeasible_detection_to_server(event_start_time, video_start_time + timedelta(seconds=frame_pos/FPS), camera_info["CameraName"])
                    saved_frame_count += 1
                else:
                    pass
                    # print("end of video is reached!")
            CAP.release()

            # handle the rest
            for i in range(1, len(one_day_clips) -1):
                CAP.open(f"{IN_DIR}/{one_day_clips[i]}")
                video_start_time = get_datetime_from_clip_file(one_day_clips[i])
                saved_frame_count = 0
                is_frame_pos_valid = 1
                while is_frame_pos_valid:
                    frame_pos = DETECTION_FREQ * saved_frame_count
                    CAP.set(cv2.CAP_PROP_POS_FRAMES, frame_pos)
                    is_frame_pos_valid, frame = get_frame_safe(CAP)
                    if is_frame_pos_valid:
                        if is_feasibility_test_passed(frame):
                            VID_WRITER.write(frame)
                            time_str = (video_start_time + timedelta(seconds=frame_pos/FPS)).strftime("%H:%M:%S")           
                            frames_info_file.write(time_str+'\n')
                        else:
                            add_infeasible_detection_to_server(event_start_time, video_start_time + timedelta(seconds=frame_pos/FPS), camera_info["CameraName"])
                        saved_frame_count += 1
                CAP.release()

            #handle the end frame            
            CAP.open(f"{IN_DIR}/{one_day_clips[-1]}")
            video_start_time = get_datetime_from_clip_file(one_day_clips[-1])
            saved_frame_count = 0
            is_frame_pos_valid = 1
            current_frame_time = video_start_time
            while is_frame_pos_valid and current_frame_time <= target_date + timedelta(hours=CONFIG["DetectionEndTime"]+12):
                frame_pos = DETECTION_FREQ * saved_frame_count
                current_frame_time = target_date + timedelta(seconds=frame_pos/FPS)
                CAP.set(cv2.CAP_PROP_POS_FRAMES, frame_pos)
                is_frame_pos_valid, frame = get_frame_safe(CAP)
                if is_frame_pos_valid:
                    if is_feasibility_test_passed(frame):
                        VID_WRITER.write(frame)
                        time_str = (video_start_time + timedelta(seconds=frame_pos/FPS)).strftime("%H:%M:%S")           
                        frames_info_file.write(time_str+'\n')
                    else:
                        add_infeasible_detection_to_server(event_start_time, video_start_time + timedelta(seconds=frame_pos/FPS), camera_info["CameraName"])
                    saved_frame_count += 1
            CAP.release()

        else: # not day time only...

            # just handle every clip with same logic
            for i in range(len(one_day_clips)):
                CAP.open(f"{IN_DIR}/{one_day_clips[i]}")
                video_start_time = get_datetime_from_clip_file(one_day_clips[i])
                saved_frame_count = 0
                is_frame_pos_valid = 1
                while is_frame_pos_valid:
                    frame_pos = DETECTION_FREQ * saved_frame_count
                    CAP.set(cv2.CAP_PROP_POS_FRAMES, frame_pos)
                    is_frame_pos_valid, frame = get_frame_safe(CAP)
                    if is_frame_pos_valid:
                        if is_feasibility_test_passed(frame):
                            VID_WRITER.write(frame)
                            time_str = (video_start_time + timedelta(seconds=frame_pos/FPS)).strftime("%H%M%S")           
                            frames_info_file.write(time_str+'\n')
                        else:
                            add_infeasible_detection_to_server(event_start_time, video_start_time + timedelta(seconds=frame_pos/FPS), camera_info["CameraName"])
                        saved_frame_count += 1
                CAP.release()
        
        VID_WRITER.release()    
        frames_info_file.close()
        
        # handling pass count
        start_hour = 0
        end_hour = 23
        if CONFIG["IsDaytimeOnly"]:
            start_hour = CONFIG["DetectionStartTime"]
            end_hour = CONFIG["DetectionEndTime"] + 12
        
        ## assuming no overlapping between clips...
        '''
        find clip
        if one clip
        and if two clips are required...        

        TODO: should I just ignore invalid frame as if it doesn't exist at all?

        '''
        for target_hour in range(start_hour, end_hour + 1):
            is_duration_in_one_clip = False
            target_clip = ""
            for i in range(len(one_day_clips) - 1):
                # if the time
                if target_date + timedelta(hours=target_hour) <= get_datetime_from_clip_file(one_day_clips[i]) <= target_date + timedelta(hours=target_hour, minutes=CONFIG["PassCountDuration"]):
                    target_clip = (one_day_clips[i-1], one_day_clips[i])
                    break

                if get_datetime_from_clip_file(one_day_clips[i]) <= target_date + timedelta(hours=target_hour) and \
                    get_datetime_from_clip_file(one_day_clips[i+1]) >= target_date + timedelta(hours=target_hour, minutes=CONFIG["PassCountDuration"]):
                    is_duration_in_one_clip = True
                    target_clip = one_day_clips[i]
                    break
            VID_WRITER.open(f"{OUT_DIR}/Temp/{camera_info['CameraName']}_{target_date.strftime('%Y-%m-%d')}-{str(target_hour).zfill(2)}_Count.mp4",
                                    cv2.VideoWriter_fourcc(*'mp4v'), 30, (WIDTH, HEIGHT)) # fps doesn't matter here.. just set it 30... 
            if is_duration_in_one_clip:
                start_frame_pos = ((target_date + timedelta(hours=target_hour)) - get_datetime_from_clip_file(target_clip)).seconds * FPS
                end_frame_pos = start_frame_pos + FPS * CONFIG["PassCountDuration"] * 60
                CAP.open(target_clip)
                CAP.set(cv2.CAP_PROP_POS_FRAMES, start_frame_pos)
                f = start_frame_pos
                while f < end_frame_pos:
                    ret, frame = CAP.read()
                    if ret:
                        VID_WRITER.write(frame)
                    f += 1
                CAP.release()
                VID_WRITER.release()
            else:
                start_frame_pos = ((target_date + timedelta(hours=target_hour)) - get_datetime_from_clip_file(target_clip[0])).seconds * FPS
                end_frame_pos = ((target_date + timedelta(hours=target_hour, minutes=CONFIG["PassCountDuration"])) - get_datetime_from_clip_file(target_clip[1])).seconds * FPS
                CAP.open(target_clip[0])
                CAP.set(cv2.CAP_PROP_POS_FRAMES)
                failed_count = 0
                while failed_count < FAILURE_LIMIT:
                    ret, frame = CAP.read()
                    if ret:
                        VID_WRITER.write(frame)
                        failed_count = 0
                    else:
                        failed_count += 1
                CAP.release()
                CAP.open(target_clip[1])
                CAP.release()
                f = 0
                while f < end_frame_pos:
                    ret, frame = CAP.read()
                    if ret:
                        VID_WRITER.write(frame)
                    f += 1
                CAP.release()
                VID_WRITER.release()

    logging.log("Video combining is completed...")
                    
        
def run_detection(target_date):
    add_detection_analysis_to_server(target_date)
    cwd = os.getcwd()
    os.chdir(f"cd {CONFIG['YoloV7Path']}")
    event_start_time = target_date
    if CONFIG["IsDaytimeOnly"]:
        event_start_time += timedelta(hours=CONFIG["DetectionStartTime"])
    for C in CONFIG["Cameras"]:
        p = subprocess.Popen(["python","detect.py","--weights", C["ModelFilePath"], "--conf-thres", C['Confidence'], "--iou-thres", C['IOU'],
                 "--img-size", C['ImageSize'], "--project", f"{CONFIG['TaskOutputDir']}/Detections", "--name", f"{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}",
                 "--source", f"{CONFIG['TaskOutputDir']}/Temp/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}_Detection.mp4",  "--save-txt", "--save-conf", "--nosave"],
                stdout=subprocess.DEVNULL
                )
        p.wait()
        # os.system(f"{sys.executable} detect.py --weights {C['ModelFilePath']} --conf-thres {C['Confidence']} --iou-thres {C['IOU']}\
        #      --img-size {C['ImageSize']} --project {CONFIG['TaskOutputDir']}/Detections --name {C['CameraName']}_{target_date.strftime('%Y-%m-%d')}\
        #      --source {CONFIG['TaskOutputDir']}/Temp/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}_Detection.mp4 --save-txt --save-conf --nosave")
        
        label_dir = f"{CONFIG['TaskOutputDir']}/Detections/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}/labels"
        label_file_prefix = re.findall(os.listdir(label_dir)[0], '/(.*)_\d+.txt')[0]
        detected_idx = [int(re.findall(s, '_(\d+).txt')[0]) for s in os.listdir(label_dir)]

        frames_info_file = f"{CONFIG['TaskOutputDir']}/Temp/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}_frames_info.txt"
        with open(frames_info_file, 'r') as f:
            # format is %H:%M:%S
            timestamps = f.read().split("\n")[:-1]

        data = []
        zero_data = []
        analysis_str = f'{C["CameraName"]},{C["ModelName"]},{event_start_time.strftime("%Y-%m-%d %H:%M:%S")}'
        for i in range(len(timestamps)):
            if i in detected_idx:
                size_df = label_file_to_df(label_dir + "/" +label_file_prefix + f"_{i}.txt").groupby(["CatID"]).size()
                for j in range(len(size_df)):
                    d = dict(
                          analysis=analysis_str,
                          fish=C["Categories"][size_df.index[j]],
                          count=size_df[j],
                          detect_time=timestamps[i],
                          can_detect="true"
                          )
                    data.append(d)
            else:
                d = dict(
                        analysis=analysis_str,
                        fish="空白",
                        count=0,
                        detect_time=timestamps[i],
                        can_detect="true"
                        )
                zero_data.append(d)
        if data:
            Session.post(f'{CONFIG["Server"]}/api/fish_detection', json=data)
        if zero_data:
            Session.post(f'{CONFIG["Server"]}/api/fish_detection', json=zero_data)
    os.chdir(cwd)


def run_counting(target_date):
    '''
    for each camera:
    for each counting clip:
        feasibility test
        add analysis to server
        run yolov7
        parse pass count by my algorithm...
        send detail and simple to server
    '''
    cwd = os.getcwd()
    os.chdir(f"cd {CONFIG['YoloV7Path']}")               
    CAP = cv2.VideoCapture()
    num_frames_test = round(1/CONFIG["PassCountDuration"])
    temp_dir = f'{CONFIG["TaskOutputDir"]}/Temp/'    
    analysis_period = f'00:{CONFIG["PassCountDuration"].zfill(2)}:00'
    analysis_time = datetime.now().strftime(DB_TimeFormat)
    for C in CONFIG["Cameras"]:
        clips = [temp_dir + f for f in os.listdir(temp_dir) if f"{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}" in f and "Count" in f]
        for clip in clips:
            # feasiblity test and add analysis data to server
            hour = int(re.findall(clip, "-(\d+)_Count.mp4")[0])
            CAP.open(clip)
            num_frames = CAP.get(cv2.CAP_PROP_FRAME_COUNT)
            frames_to_test = sorted(random.sample(range(num_frames), num_frames_test))
            is_feasiblity_passed = True
            for f in frames_to_test:
                CAP.set(cv2.CAP_PROP_POS_FRAMES, f)
                _, frame = CAP.read()
                if not is_feasibility_test_passed(frame, C):
                    is_feasiblity_passed = False
                    break
            CAP.release()
            if is_feasiblity_passed:
                event_time = target_date + timedelta(hours=hour)
                d = dict(
                    camera=C["CameraName"],
                    event_time=event_time.strftime(DB_TimeFormat),
                    event_period=analysis_period,
                    analysis_type="CO",
                    analysis_time=analysis_time,
                    can_analyze="true",
                    analysis_log="created by automation task"
                )
                Session.post(f'{CONFIG["Server"]}/api/fish_analysis', json=[d])
                analysis_str = f'{C["CameraName"]},{C["ModelName"]},{event_time.strftime("%Y-%m-%d %H:%M:%S")}'
                p = subprocess.Popen(["python","detect.py","--weights", C["ModelFilePath"], "--conf-thres", C['Confidence'], "--iou-thres", C['IOU'],
                    "--img-size", C['ImageSize'], "--project", f"{CONFIG['TaskOutputDir']}/PassCounts", "--name", f"{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}-{str(hour).zfill(2)}",
                    "--source", clip,  "--save-txt", "--save-conf", "--nosave"],
                    stdout=subprocess.DEVNULL
                    )
                p.wait()
                pass_df = track_individual(f"{CONFIG['TaskOutputDir']}/PassCounts/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}-{str(hour).zfill(2)}/labels", C)
                # pass_data to simple data for upload...
                size_df = pass_df.groupby(["category"]).size()
                data = []
                for j in range(len(size_df)):
                    d = dict(
                        analysis=analysis_str,
                        fish=size_df.index[j],
                        count=size_df[j]
                    )
                    data.append(d)
                Session.post(f'{CONFIG["Server"]}/api/fish_count', json=data)
                
                # pass_data to individual detail for upload...
                data = []
                for index, row in pass_df.iterrows():
                    d = dict(
                        analysis=analysis_str,
                        fish=row['category'],
                        approximate_speed=row['approx_speed'],
                        approximate_body_length=row['approx_body_length'],
                        approximate_body_height=row['approx_body_height'],
                        enter_frame=row['enter_frame'],
                        leave_frame=row['leave_frame']
                    )
                    data.append(d)
                Session.post(f'{CONFIG["Server"]}/api/fish_count_detail', json=data)

            else:
                d = dict(
                    camera=C["CameraName"],
                    event_time=(target_date + timedelta(hours=hour)).strftime(DB_TimeFormat),
                    event_period=analysis_period,
                    analysis_type="CO",
                    analysis_time=analysis_time,
                    can_analyze="false",
                    analysis_log="created by automation task"
                )
                Session.post(f'{CONFIG["Server"]}/api/fish_analysis', json=[d])
    os.chdir(cwd)
            
    
# TODO: padding time not include in IFCS... thats a required var to set? 
def preserve_important_regions(target_date):
    
    pass


# very roughly estimates... it will take 4 hours to preserve 24 hours video!
# it just runs on one core... no problem at all!
def preserve_combined_clips(target_date):
    pass

def remove_raw_clips(target_date):
    '''
    maybe remove the clips after 1 weeks which all the anlysis is done perfectly?
    '''
    pass

def notify_server_stopping_scheduled_task():
    logging.log("Scheduled task is stopped!")
    print("hey... this will be the perfect code to do the stuff...")

def one_time_task():
    logging.log("Start one time task...")
    task_start_time = time.time()
    dates_to_run = []
    '''
    year in c++ time format ***tm*** starts from 1900, month starts from 0
    '''
    if CONFIG["IsRunSpecifiedDates"]:
        for d in CONFIG["RunDates"]:
            dates_to_run.append(datetime(year=d["Year"] + 1900, month=d["Month"] + 1, day=d["Day"]))
    else:
        sd = CONFIG["StartDate"]
        ed = CONFIG["EndDate"]
        start_date = datetime(year=sd["Year"] + 1900, month = sd["Month"] + 1, day=sd["Day"])
        end_date = datetime(year=ed["Year"] + 1900, month = ed["Month"] + 1, day=ed["Day"])
        d = start_date
        while d <= end_date:
            dates_to_run.append(d)
            d += timedelta(days=1)
    for date in dates_to_run:
        logging.log(f"processing {date.strftime('%Y/%m/%d')}...")
        combine_video(date)
        run_detection(date)
        run_counting(date)
        if CONFIG["ShouldBackupImportantRegions"]:
            preserve_important_regions(date)
        if CONFIG["ShouldBackupCombinedClips"]:
            preserve_combined_clips(date)
        if CONFIG["ShouldDeleteRawClips"]:
            remove_raw_clips(date)      
    time_elapse = str(timedelta(seconds=round(time.time() - task_start_time)))
    logging.log(f"One time task is finished! (time elapse: {time_elapse})")
    '''
    No SMS notification in on_time_task!! Not necessary at all!
    '''
    
    
def scheduled_task():
    atexit.register(notify_server_stopping_scheduled_task)
    while True:
        if datetime.now().hour == CONFIG["ScheduledRuntime"][0] and datetime.now().minute == CONFIG["ScheduledRuntime"][1]:
            logging.log("Start scheduled task...")
            task_start_time = time.time()
            yesterday = datetime.today() - timedelta(days=1)
            combine_video(yesterday)
            run_detection(yesterday)
            run_counting(yesterday)
            if CONFIG["ShouldBackupImportantRegions"]:
                preserve_important_regions(yesterday)
            if CONFIG["ShouldBackupCombinedClips"]:
                preserve_combined_clips(yesterday)
            if CONFIG["ShouldDeleteRawClips"]:
                remove_raw_clips(yesterday)                                
            time_elapse = str(timedelta(seconds=round(time.time() - task_start_time)))
            logging.log(f"Scheduled task is finished! (time elapse: {time_elapse})")
            send_SMS()
        time.sleep(60)


def send_SMS():
    pass

def send_test_SMS():
    pass

        
if __name__ == "__main__":
    with open("IFCS_DeployConfig.yaml", 'r') as f:
        CONFIG = yaml.safe_load(f.read())      
    if not os.path.exists(f"{CONFIG['TaskOutputDir']}/Temp"):
        os.mkdir(f"{CONFIG['TaskOutputDir']}/Temp")    
        os.mkdir(f"{CONFIG['TaskOutputDir']}/Detections")    
        os.mkdir(f"{CONFIG['TaskOutputDir']}/PassCounts")    
        os.mkdir(f"{CONFIG['TaskOutputDir']}/Backup")    
    logging.basicConfig(filename=CONFIG['TaskOutputDir']+"/Deploy.log", encoding='utf-8', level=logging.INFO,
        format='%(asctime)s %(message)s', datefmt='%Y/%m/%d %H:%M:%S')
    Session.auth = (CONFIG["ServerAccount"], CONFIG["ServerPassword"])
    Session.headers.update({"Content-Tyoe":"application/json"})
    if config_checking():
        print("Press Ctrl + C to stop!")
        print(f"Log message is saved at {CONFIG['TaskOutputDir']}/Deploy.log")    
        send_test_SMS()
        if CONFIG['IsTaskStartNow']:
            one_time_task()
        else:
            scheduled_task()
    else:
        logging.error("Configuration is not completed...")
        print("*** Configuration is not completed ***")
        print("Please double check you configuration file and server database")
        print()
        # TODO: how to grab the configuration problem here, so that the user would understand how to handle?
        print()
        input("PRESS ANY KEY TO QUIT")
