import time, logging, requests, re, yaml, cv2, os, sys, atexit
import numpy as np
from datetime import datetime, timedelta
from twilio.rest import Client


'''
send the passwords, sid, phone number to my client with a encrypted pdf file...

'''

# global vars
config_data = {}

FAILURE_LIMIT = 5 


def SendMessage(twilio_account_sid, twilio_auth_token, twilio_phone_number, to_phone_number, message):
    client = Client(twilio_account_sid, twilio_auth_token)
    message = client.messages.create(
         body=message,
         from_ =  twilio_phone_number,
         to = to_phone_number
    )
    message.sid # message 

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



def AddDetectionAnalysisToServer(target_date, camera):
    pass

def AddInfeasibleDetectionToServer(target_datetime, camera):
    '''
    make sure "infeasible" fish species is registered!!
    '''
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
    DETECTION_FREQ = config_data["DetectionFrequency"]
    IN_DIR = config_data["TaskInputDir"]
    OUT_DIR = config_data["TaskOutputDir"]

    for camera_info in config_data["Cameras"]:        
        # prepare clips
        one_day_clips = sorted([file for file in os.listdir(config_data["TaskInputDir"]) if camera_info['DVRName'] in file and target_date.strftime("%Y%m%d") in file and file.endswith('.video')])
        if config_data["IsDaytimeOnly"]:
            temp = one_day_clips.copy()
            one_day_clips = []
            start_time = target_date + timedelta(hours=config_data["DetectionStartTime"])
            end_time = target_date + timedelta(hours=config_data["DetectionEndTime"]+12)
            sid ,eid = 0
            for i in range(len(temp) - 1):
                if datetime.strptime(re.findall('-(\d+-\d+).video', temp[i])[0], "%Y%m%d-%H%M%S") < start_time < datetime.strptime(re.findall('-(\d+-\d+).video', temp[i+1])[0], "%Y%m%d-%H%M%S"):
                    sid = i
                if datetime.strptime(re.findall('-(\d+-\d+).video', temp[i])[0], "%Y%m%d-%H%M%S") > end_time:
                    eid = i
                    break
            for i in range(sid, eid+1):
                one_day_clips.append(temp[i])

        # get the basic video parameters
        cap = cv2.VideoCapture(f"{IN_DIR}/{one_day_clips[0]}")
        fps = cap.get(cv2.CAP_PROP_FPS)
        w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        cap.release()

        video_writer = cv2.VideoWriter(f"{OUT_DIR}/Temp/{camera_info['CameraName']}_{target_date.strftime('%Y-%m-%d')}_Detection.mp4",
                                        cv2.VideoWriter_fourcc(*'mp4v'), 30, (w, h)) # fps doesn't matter here.. just set it 30... 
        frames_info_file = open(f"{OUT_DIR}/Temp/{camera_info['CameraName']}_{target_date.strftime('%Y-%m-%d')}_frames_info.txt", "w")

        # handle the first video...
        start_time = datetime.strptime(re.findall('-(\d+-\d+).video', one_day_clips[0])[0], "%Y%m%d-%H%M%S")
        frames_to_move = (datetime(start_time.year, start_time.month, start_time.day, start_time.hour + 1) - start_time).seconds * fps
        cap = cv2.VideoCapture(f"{IN_DIR}/{one_day_clips[0]}")
        t = 0
        is_frame_pos_valid = 1
        while is_frame_pos_valid:
            cap.set(cv2.CAP_PROP_POS_FRAMES, f + DETECTION_FREQ*t)
            is_frame_pos_valid, frame = GetFrameSafe(cap)
            if is_frame_pos_valid:
                video_writer.write(frame)            
            t += 1
        

        # handle the rest
        for i in range(1, len(one_day_clips) -1):
            pass

        #handle the end frame


'''
above steps are for day time only

otherwise just loop til the end is good!
'''



        # # handling detection
        # for idx in range(len(one_day_clips)):
        #     clip = one_day_clips[idx]
        #     cap = cv2.VideoCapture(f"{config_data['TaskInputDir']}/{clip}")
        #     # total_frames = cap.get(cv2.CAP_PROP_FRAME_COUNT) 
        #     video_start_time = datetime.strptime(re.findall('-(\d+-\d+).video', clip)[0], "%Y%m%d-%H%M%S")
        #     # ASSUMING the test is passed            
        #     video_end_time = datetime.strptime(re.findall('-(\d+-\d+).video', one_day_clips[idx + 1])[0], "%Y%m%d-%H%M%S")
        #     if not is_first_clip_found:
        #         if config_data['IsDaytimeOnly']:
        #             detection_start_time = target_date + timedelta(hours=config_data["DetectionStartTime"])
        #             if video_start_time < detection_start_time and video_end_time > detection_start_time:
        #                 frames_to_read = []
        #                 f = 0
        #                 while f < total_frames:
        #                     if video_start_time + timedelta(seconds=f/fps) > detection_start_time:
        #                         frames_to_read.append(f)
        #                     f += config_data["DetectionFrequency"]
        #                 for f in frames_to_read:
        #                     cap.set(cv2.CAP_PROP_POS_FRAMES, f)
        #                     ret, frame = cap.read()
        #                     # do the feasibility test!
        #                     if IsFeasibilityTestPassed(frame, camera_info):
        #                         video_writer.write(frame)
        #                         time_str = (video_start_time + timedelta(seconds=f/fps)).strftime("%H%M%S%f")
        #                         frames_info_file.write(f"{time_str}\n")
        #                     else:
        #                         AddInfeasibleDetectionToServer(video_start_time + timedelta(seconds=f/fps), camera_info["CameraName"])
                                
        #                 is_first_clip_found = True
        #                 progress_time = video_end_time
                            
        #             else:
        #                 cap.release()
        #         else:
        #             frames_to_read = []
        #             f = 0
        #             while f < total_frames:
        #                 frames_to_read.append(f)
        #                 f += config_data["DetectionFrequency"]
        #             for f in frames_to_read:
        #                 cap.set(cv2.CAP_PROP_POS_FRAMES, f)
        #                 _, frame = cap.read()
        #                 # do the feasibility test!
        #                 if IsFeasibilityTestPassed(frame, camera_info):
        #                     video_writer.write(frame)
        #                     time_str = (video_start_time + timedelta(seconds=f/fps)).strftime("%H%M%S%f")
        #                     frames_info_file.write(f"{time_str}\n")
        #                 else:
        #                     AddInfeasibleDetectionToServer(video_start_time + timedelta(seconds=f/fps), camera_info["CameraName"])
        #             is_first_clip_found = True
        #             progress_time = video_end_time
        #             cap.release()
        #     else: # handling the rest clip ... it starts in between...
        #         # check if process time is in between video start and end
        #         if progress_time > video_start_time and progress_time < video_end_time:
        #             frames_to_read = []
        #             f = 0
        #             while f < total_frames:
        #                 if video_start_time + timedelta(seconds=f/fps) > progress_time:
        #                     frames_to_read.append(f)                            
        #                 f += config_data["DetectionFrequency"]
        #                 # stop it's night time or stop it's the second day...
        #                 if (config_data["IsDaytimeOnly"] and video_start_time + timedelta(seconds=f/fps) > target_date + timedelta(hours=config_data["DetectionEndTime"] + 12)) or \
        #                     (video_start_time + timedelta(seconds=f/fps) > target_date + timedelta(days=1)):
        #                         is_task_done = True
        #                         break
        #             for f in frames_to_read:
        #                 cap.set(cv2.CAP_PROP_POS_FRAMES, f)
        #                 _, frame = cap.read()
        #                 if IsFeasibilityTestPassed(frame, camera_info):
        #                     video_writer.write(frame)
        #                     time_str = (video_start_time + timedelta(seconds=f/fps)).strftime("%H%M%S%f")
        #                     frames_info_file.write(f"{time_str}\n")
        #                 else:
        #                     AddInfeasibleDetectionToServer(video_start_time + timedelta(seconds=f/fps), camera_info["CameraName"])
        #         cap.release()
        #         if is_task_done:
        #             break
        video_writer.release()    
        frames_info_file.close()
        
        # handling pass count
        hour_to_write = 0
        end_hour = 23
        if config_data["IsDaytimeOnly"]:
            hour_to_write = config_data["DetectionStartTime"]
            end_hour = config_data["DetectionEndTime"] + 12
        
        ## assuming the clips are overlapping a lot... I don't need to concat in any case and I will always find the one
        while hour_to_write <= end_hour:
            found_clip = ""
            # early_clips = [] #TODO: clear early clips before start???...
            for clip in one_day_clips:
                # remove clip to make this loop faster?
                cap = cv2.VideoCapture(f"{config_data['TaskInputDir']}/{clip}")
                total_frames = cap.get(cv2.CAP_PROP_FRAME_COUNT)
                video_start_time = datetime.strptime(re.findall('-(\d+-\d+).video', clip), "%Y%m%d-%H%M%S")
                # video_end_time = video_start_time + timedelta(seconds=total_frames/fps)
                if video_start_time > target_date + timedelta(hours=hour_to_write) and video_end_time < target_date:
                    frame_to_jump = (target_date + timedelta(hours=hour_to_write) - video_start_time).seconds * fps
                    cap.set(cv2.CAP_PROP_POS_FRAMES, frame_to_jump)
                    f = 0
                    video_writer = cv2.VideoWriter(f"{config_data['TaskOutputDir']}/Temp/{camera_info['CameraName']}_{target_date.strftime('%Y-%m-%d')}_{hour_to_write}_Pass.mp4", cv2.VideoWriter_fourcc(*'mp4v'), fps, (w, h))
                    while f < config_data["PassCountDuration"] * 60 * fps:
                        _, frame = cap.read()
                        video_writer.write(frame)                        
                        f += 1
                    video_writer.release()
                    found_clip = clip
                    break                
                cap.release()
            if found_clip:
                one_day_clips.remove(found_clip)
                hour_to_write += 1 
            else:
                msg = f"Can not find a clip includes the required time period {target_date.strftime('%Y%m%d')} {hour_to_write}:00 ~ {hour_to_write}:{str(config_data['PassCountDuration']).zfill(2)}"
                # if this case ever happened... I must write someother tedious code to concat clips...
                logging.error(msg)
                raise LookupError(msg)
            
        
def RunDetection(target_date):
    cwd = os.getcwd()
    os.chdir(f"cd {config_data['YoloV7Path']}")
    for C in config_data["Cameras"]:
        os.system(f"{sys.executable} detect.py --weights {C['ModelFilePath']} --conf-thres {C['Confidence']} --iou-thres {C['IOU']}\
             --img-size {C['ImageSize']} --project {config_data['TaskOutputDir']}/Detections --name {C['CameraName']}_{target_date.strftime('%Y-%m-%d')}\
             --source {config_data['TaskOutputDir']}/Temp/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}_Detection.mp4 --save-txt --save-conf --nosave")
        
        
        label_dir = f"{config_data['TaskOutputDir']}/Detections/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}"
        detected_idx = [int(re.findall(s, '_(\d+).txt')[0]) for s in os.listdir(label_dir)]

        frames_info_file = f"{config_data['TaskOutputDir']}/Temp/{C['CameraName']}_{target_date.strftime('%Y-%m-%d')}_frames_info.txt"
        with open(frames_info_file, 'r') as f:
            timestamps = f.read().split("\n")[:-1]
        timestamps = [datetime.strptime(s, "%H%M%S%f").time() for s in timestamps] # format is datetime.time

        for i in range(len(timestamps)):
            if i in detected_idx:
                # TODO: send normal data to server!
                pass
            else:
                # TODO: send 0 to server!
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
    if config_data["IsRunSpecifiedDates"]:
        for d in config_data["RunDates"]:
            dates_to_run.append(datetime(year=d["Year"] + 1900, month=d["Month"] + 1, day=d["Day"]))
    else:
        sd = config_data["StartDate"]
        ed = config_data["EndDate"]
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
        if config_data["ShouldBackupImportantRegions"]:
            PreserveImportantRegions(date)
        if config_data["ShouldBackupCombinedClips"]:
            PreserveCombinedClips(date)
        if config_data["ShouldDeleteRawClips"]:
            RemoveRawClips(date)      
    time_elapse = str(timedelta(seconds=round(time.time() - task_start_time)))
    logging.log(f"One time task is finished! (time elapse: {time_elapse})")
    
    
def ScheduledTask():
    atexit.register(NotifySeverStoppingScheduledTask)
    while True:
        if datetime.now().hour == config_data["ScheduledRuntime"][0] and datetime.now().minute == config_data["ScheduledRuntime"][1]:
            logging.log("Start scheduled task...")
            task_start_time = time.time()
            yesterday = datetime.today() - timedelta(days=1)
            CombineVideo(yesterday)
            RunDetection(yesterday)
            RunCounting(yesterday)
            if config_data["ShouldBackupImportantRegions"]:
                PreserveImportantRegions(yesterday)
            if config_data["ShouldBackupCombinedClips"]:
                PreserveCombinedClips(yesterday)
            if config_data["ShouldDeleteRawClips"]:
                RemoveRawClips(yesterday)                                
            time_elapse = str(timedelta(seconds=round(time.time() - task_start_time)))
            logging.log(f"Scheduled task is finished! (time elapse: {time_elapse})")
        time.sleep(60)
        
        
if __name__ == "__main__":
    with open("IFCS_DeployConfig.yaml", 'r') as f:
        config_data = yaml.safe_load(f.read())      
    if not os.path.exists(f"{config_data['TaskOutputDir']}/Temp"):
        os.mkdir(f"{config_data['TaskOutputDir']}/Temp")    
        os.mkdir(f"{config_data['TaskOutputDir']}/Detections")    
        os.mkdir(f"{config_data['TaskOutputDir']}/PassCount")    
        os.mkdir(f"{config_data['TaskOutputDir']}/Backup")    
    logging.basicConfig(filename=config_data['TaskOutputDir']+"/Deploy.log", encoding='utf-8', level=logging.INFO,
        format='%(asctime)s %(message)s', datefmt='%Y/%m/%d %H:%M:%S')
    
    print("Press Ctrl + C to stop!")
    print(f"Log message is saved at {config_data['TaskOutputDir']}/Deploy.log")    
    if config_data['IsTaskStartNow']:
        OneTimeTask()
    else:
        ScheduledTask()
