import time, logging, requests, re, yaml, cv2, os, sys, atexit
import numpy as np
from datetime import datetime, timedelta, time
from twilio.rest import Client


'''
send the passwords, sid, phone number to my client with a encrypted pdf file...


TODO: Domain name is missing in config_data... (ServerUrl)

'''

# global vars
CONFIG = {}

FAILURE_LIMIT = 5 
TimeFormat = "%Y-%M-%dT%H:%M:S+08:00"


def SendMessage(twilio_account_sid, twilio_auth_token, twilio_phone_number, to_phone_number, message):
    client = Client(twilio_account_sid, twilio_auth_token)
    message = client.messages.create(
         body=message,
         from_ =  twilio_phone_number,
         to = to_phone_number
    )
    message.sid # message 


def GetDatetimeFromClipFile(clip_file_name):
    return datetime.strptime(re.findall('-(\d+-\d+).video', clip_file_name)[0], "%Y%m%d-%H%M%S")

def GetFrameSafe(in_cap):
    read_times = 0
    while True:
        ret, frame = in_cap.read()
        if ret:
            return read_times, frame
        else:
            read_times += 1
            if read_times > FAILURE_LIMIT:
                return 0, []



def AddDetectionAnalysisToServer(target_date):
    '''
    Add 1 for each camera
    '''
    s = requests.Session()
    s.auth = (CONFIG["ServerAccount"], CONFIG["ServerPassword"])
    s.headers.update({"Content-Tyoe":"application/json"})
    data = []
    analysis_time = datetime.now().strftime(TimeFormat)
    for C in CONFIG["Cameras"]:
        d = {}
        d["camera"] = C["CameraName"]
        if CONFIG["IsDaytimeOnly"]:
            d["event_time"] = (target_date + timedelta(hours=CONFIG["DetectionStartTime"])).strftime(TimeFormat)
            d["event_period"] = time(CONFIG["DetectionEndTime"] + 12 - CONFIG["DetectionStartTime"]).strftime("%H:%M:%S")
        else:
            d["event_time"] = target_date.strftime(TimeFormat)
            d["event_period"] = "23:59:59"
        d["analysis_type"] = "DE"
        d["analysis_time"] = analysis_time 

        '''
        TODO: consider this twice... should this ALWAYS be true? 
        each detection result from this analysis has independt infeasible test result...
        how does that relate to the whole analysis?
        '''
        d["can_analysis"] = "true" 

        data.append(d)
    s.post(f'{CONFIG["ServerUrl"]}/api/fish_analysis', json=data)
    

def AddInfeasibleDetectionToServer(analysis_start_time, detection_time, camera):
    '''
    TODO: need test...
    make sure "infeasible" fish species is registered!!
    '''
    s = requests.Session()
    s.auth = (CONFIG["ServerAccount"], CONFIG["ServerPassword"])
    s.headers.update({"Content-Tyoe":"application/json"})
    d = {}
    d["analysis"] = f'{camera["CameraName"]},{camera["ModelName"]},{analysis_start_time.strftime("%Y-%m-%d %H:%M:%S")}'
    d["fish"] = "無法辨識"
    d["count"] = 0
    d["detect_time"] = detection_time.strftime("%H:%M:%S")
    d["can_detect"] = "false"
    s.post(f'{CONFIG["ServerUrl"]}/api/fish_detection', json=[d])

def SendDetectionResult():
    pass

def SendZeroDetection():
    pass

def GetRGBFromU32(u32_color):
    b = u32_color & 255
    g = (u32_color >> 8) & 255
    r = (u32_color >> 16) & 255
    return (r,g,b)
    

def IsFeasibilityTestPassed(frame_data, camera_info):
    height, width, _ = frame_data.shape    
    for zone in camera_info["FeasibleZones"]:
        x,y,w,h = zone["XYWH"]
        ROI = frame_data[ int((x-0.5*w)*width): int((x+0.5*w)*width), int((y-0.5*h)*height):int((y+0.5*h)*height),:]
        ## TODO: check BGR RGB issue... 
        AvgColor = np.mean(np.mean(ROI,axis=0),axis=0) # this is BGR!!
        low_color = GetRGBFromU32(zone["ColorLower"]) # RGB
        upper_color = GetRGBFromU32(zone["ColorUpper"]) #RGB
        if low_color[0] <= AvgColor[2] and upper_color[0] >= AvgColor[2] and low_color[1] <= AvgColor[1] and upper_color[1] >= AvgColor[1] \
            and low_color[2] <= AvgColor[0] and upper_color[2] >= AvgColor[0]:
                return True        
    return False

def CombineVideo(target_date):
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
                if GetDatetimeFromClipFile(temp[i]) < event_start_time < GetDatetimeFromClipFile(temp[i+1]):
                    sid = i
                if GetDatetimeFromClipFile(temp[i]) > event_end_time:
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
            video_start_time = GetDatetimeFromClipFile(one_day_clips[0])
            start_frame_pos = (datetime(video_start_time.year, video_start_time.month, video_start_time.day, video_start_time.hour + 1) - video_start_time).seconds * FPS
            CAP.open(f"{IN_DIR}/{one_day_clips[0]}")
            saved_frame_count = 0
            is_frame_pos_valid = 1
            while is_frame_pos_valid:
                frame_pos = start_frame_pos + DETECTION_FREQ * saved_frame_count
                CAP.set(cv2.CAP_PROP_POS_FRAMES, frame_pos)
                is_frame_pos_valid, frame = GetFrameSafe(CAP)
                if is_frame_pos_valid:
                    if IsFeasibilityTestPassed(frame):
                        VID_WRITER.write(frame)
                        time_str = (video_start_time + timedelta(seconds=frame_pos/FPS)).strftime("%H:%M:%S")           
                        frames_info_file.write(time_str+'\n')
                    else:
                        AddInfeasibleDetectionToServer(event_start_time, video_start_time + timedelta(seconds=frame_pos/FPS), camera_info["CameraName"])
                    saved_frame_count += 1
                else:
                    pass
                    # print("end of video is reached!")
            CAP.release()

            # handle the rest
            for i in range(1, len(one_day_clips) -1):
                CAP.open(f"{IN_DIR}/{one_day_clips[i]}")
                video_start_time = GetDatetimeFromClipFile(one_day_clips[i])
                saved_frame_count = 0
                is_frame_pos_valid = 1
                while is_frame_pos_valid:
                    frame_pos = DETECTION_FREQ * saved_frame_count
                    CAP.set(cv2.CAP_PROP_POS_FRAMES, frame_pos)
                    is_frame_pos_valid, frame = GetFrameSafe(CAP)
                    if is_frame_pos_valid:
                        if IsFeasibilityTestPassed(frame):
                            VID_WRITER.write(frame)
                            time_str = (video_start_time + timedelta(seconds=frame_pos/FPS)).strftime("%H:%M:%S")           
                            frames_info_file.write(time_str+'\n')
                        else:
                            AddInfeasibleDetectionToServer(event_start_time, video_start_time + timedelta(seconds=frame_pos/FPS), camera_info["CameraName"])
                        saved_frame_count += 1
                CAP.release()

            #handle the end frame            
            CAP.open(f"{IN_DIR}/{one_day_clips[-1]}")
            video_start_time = GetDatetimeFromClipFile(one_day_clips[-1])
            saved_frame_count = 0
            is_frame_pos_valid = 1
            current_frame_time = video_start_time
            while is_frame_pos_valid and current_frame_time <= target_date + timedelta(hours=CONFIG["DetectionEndTime"]+12):
                frame_pos = DETECTION_FREQ * saved_frame_count
                current_frame_time = target_date + timedelta(seconds=frame_pos/FPS)
                CAP.set(cv2.CAP_PROP_POS_FRAMES, frame_pos)
                is_frame_pos_valid, frame = GetFrameSafe(CAP)
                if is_frame_pos_valid:
                    if IsFeasibilityTestPassed(frame):
                        VID_WRITER.write(frame)
                        time_str = (video_start_time + timedelta(seconds=frame_pos/FPS)).strftime("%H:%M:%S")           
                        frames_info_file.write(time_str+'\n')
                    else:
                        AddInfeasibleDetectionToServer(event_start_time, video_start_time + timedelta(seconds=frame_pos/FPS), camera_info["CameraName"])
                    saved_frame_count += 1
            CAP.release()

        else: # not day time only...

            # just handle every clip with same logic
            for i in range(len(one_day_clips)):
                CAP.open(f"{IN_DIR}/{one_day_clips[i]}")
                video_start_time = GetDatetimeFromClipFile(one_day_clips[i])
                saved_frame_count = 0
                is_frame_pos_valid = 1
                while is_frame_pos_valid:
                    frame_pos = DETECTION_FREQ * saved_frame_count
                    CAP.set(cv2.CAP_PROP_POS_FRAMES, frame_pos)
                    is_frame_pos_valid, frame = GetFrameSafe(CAP)
                    if is_frame_pos_valid:
                        if IsFeasibilityTestPassed(frame):
                            VID_WRITER.write(frame)
                            time_str = (video_start_time + timedelta(seconds=frame_pos/FPS)).strftime("%H%M%S%f")           
                            frames_info_file.write(time_str+'\n')
                        else:
                            AddInfeasibleDetectionToServer(event_start_time, video_start_time + timedelta(seconds=frame_pos/FPS), camera_info["CameraName"])
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
                if target_date + timedelta(hours=target_hour) <= GetDatetimeFromClipFile(one_day_clips[i]) <= target_date + timedelta(hours=target_hour, minutes=CONFIG["PassCountDuration"]):
                    target_clip = (one_day_clips[i-1], one_day_clips[i])
                    break

                if GetDatetimeFromClipFile(one_day_clips[i]) <= target_date + timedelta(hours=target_hour) and \
                    GetDatetimeFromClipFile(one_day_clips[i+1]) >= target_date + timedelta(hours=target_hour, minutes=CONFIG["PassCountDuration"]):
                    is_duration_in_one_clip = True
                    target_clip = one_day_clips[i]
                    break
            VID_WRITER.open(f"{OUT_DIR}/Temp/{camera_info['CameraName']}_{target_date.strftime('%Y-%m-%d')}-{str(target_hour).zfill(2)}_Count.mp4",
                                    cv2.VideoWriter_fourcc(*'mp4v'), 30, (WIDTH, HEIGHT)) # fps doesn't matter here.. just set it 30... 
            if is_duration_in_one_clip:
                start_frame_pos = ((target_date + timedelta(hours=target_hour)) - GetDatetimeFromClipFile(target_clip)).seconds * FPS
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
                start_frame_pos = ((target_date + timedelta(hours=target_hour)) - GetDatetimeFromClipFile(target_clip[0])).seconds * FPS
                end_frame_pos = ((target_date + timedelta(hours=target_hour, minutes=CONFIG["PassCountDuration"])) - GetDatetimeFromClipFile(target_clip[1])).seconds * FPS
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
                    
        
def RunDetection(target_date):
    AddDetectionAnalysisToServer(target_date)
    cwd = os.getcwd()
    os.chdir(f"cd {CONFIG['YoloV7Path']}")
    for C in CONFIG["Cameras"]:
        os.system(f"{sys.executable} detect.py --weights {C['ModelFilePath']} --conf-thres {C['Confidence']} --iou-thres {C['IOU']}\
             --img-size {C['ImageSize']} --project {CONFIG['TaskOutputDir']}/Detections --name {C['CameraName']}_{target_date.strftime('%Y-%m-%d')}\
             --source {CONFIG['TaskOutputDir']}/Temp/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}_Detection.mp4 --save-txt --save-conf --nosave")
        
        
        label_dir = f"{CONFIG['TaskOutputDir']}/Detections/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}"
        detected_idx = [int(re.findall(s, '_(\d+).txt')[0]) for s in os.listdir(label_dir)]

        frames_info_file = f"{CONFIG['TaskOutputDir']}/Temp/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}_frames_info.txt"
        with open(frames_info_file, 'r') as f:
            timestamps = f.read().split("\n")[:-1]
        timestamps = [datetime.strptime(s, "%H%M%S%f").time() for s in timestamps] # format is datetime.time

        for i in range(len(timestamps)):
            if i in detected_idx:
                SendDetectionResult()
                # TODO: send normal data to server!
                pass
            else:
                # TODO: send 0 to server!
                SendZeroDetection()
                pass
    os.chdir(cwd)


def RunCounting(target_date):

    
    '''
    rewrite my c++ script to python!!!!
    '''
    pass


# TODO: padding time not include in IFCS... thats a required var to set? 
def PreserveImportantRegions():
    pass


# very roughly estimates... it will take 4 hours to preserve 24 hours video!
# it just runs on one core... no problem at all!
def PreserveCombinedClips():
    pass

def RemoveRawClips():
    '''
    maybe remove the clips after 1 weeks which all the anlysis is done perfectly?
    '''
    pass

def NotifySeverStoppingScheduledTask():
    logging.log("Scheduled task is stopped!")
    print("hey... this will be the perfect code to do the stuff...")

def OneTimeTask():
    logging.log("Start one time task...")
    task_start_time = time.time()
    dates_to_run = []
    '''
    year in tm() starts from 1900, month starts from 0
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
        CombineVideo(date)
        RunDetection(date)
        RunCounting(date)
        if CONFIG["ShouldBackupImportantRegions"]:
            PreserveImportantRegions(date)
        if CONFIG["ShouldBackupCombinedClips"]:
            PreserveCombinedClips(date)
        if CONFIG["ShouldDeleteRawClips"]:
            RemoveRawClips(date)      
    time_elapse = str(timedelta(seconds=round(time.time() - task_start_time)))
    logging.log(f"One time task is finished! (time elapse: {time_elapse})")
    
    
def ScheduledTask():
    atexit.register(NotifySeverStoppingScheduledTask)
    while True:
        if datetime.now().hour == CONFIG["ScheduledRuntime"][0] and datetime.now().minute == CONFIG["ScheduledRuntime"][1]:
            logging.log("Start scheduled task...")
            task_start_time = time.time()
            yesterday = datetime.today() - timedelta(days=1)
            CombineVideo(yesterday)
            RunDetection(yesterday)
            RunCounting(yesterday)
            if CONFIG["ShouldBackupImportantRegions"]:
                PreserveImportantRegions(yesterday)
            if CONFIG["ShouldBackupCombinedClips"]:
                PreserveCombinedClips(yesterday)
            if CONFIG["ShouldDeleteRawClips"]:
                RemoveRawClips(yesterday)                                
            time_elapse = str(timedelta(seconds=round(time.time() - task_start_time)))
            logging.log(f"Scheduled task is finished! (time elapse: {time_elapse})")
        time.sleep(60)
        
        
if __name__ == "__main__":
    with open("IFCS_DeployConfig.yaml", 'r') as f:
        CONFIG = yaml.safe_load(f.read())      
    if not os.path.exists(f"{CONFIG['TaskOutputDir']}/Temp"):
        os.mkdir(f"{CONFIG['TaskOutputDir']}/Temp")    
        os.mkdir(f"{CONFIG['TaskOutputDir']}/Detections")    
        os.mkdir(f"{CONFIG['TaskOutputDir']}/PassCount")    
        os.mkdir(f"{CONFIG['TaskOutputDir']}/Backup")    
    logging.basicConfig(filename=CONFIG['TaskOutputDir']+"/Deploy.log", encoding='utf-8', level=logging.INFO,
        format='%(asctime)s %(message)s', datefmt='%Y/%m/%d %H:%M:%S')
    
    print("Press Ctrl + C to stop!")
    print(f"Log message is saved at {CONFIG['TaskOutputDir']}/Deploy.log")    
    if CONFIG['IsTaskStartNow']:
        OneTimeTask()
    else:
        ScheduledTask()
