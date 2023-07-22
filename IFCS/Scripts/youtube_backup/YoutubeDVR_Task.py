from vidgear.gears import CamGear
import cv2
import argparse
import datetime
import time

def main():
    parser = argparse.ArgumentParser(description="Automate backup youtube streams to your local disk")
    parser.add_argument('--video_id', type=str, help='the video id for your target youtube video. /n \
        Take a random youtube video clip as example: https://www.youtube.com/watch?v=lro-8GUX9Yk the ID is lro-8GUX9Yk')
    parser.add_argument('--clip_length', type=int, help='the length of each clips you want to store, should be 15, 30, 60, or 120',
                        choices=[15, 30, 60, 120], default=60)
    parser.add_argument('--path', type=str, help='the absolute path to where you want to store the clips')
    parser.add_argument('--start_now', dest='start_now', action='store_true', help='start this task without wait')
    parser.set_defaults(start_now=False)
    args = parser.parse_args()
    if not args.video_id or not args.path:
        print('arguments are missing!')
        return
    video_id = args.video_id
    clip_length = args.clip_length
    path = args.path
    start_now = args.start_now
    # stream = CamGear(source='https://youtu.be/' + video_id,stream_mode = True,logging=True, backend=cv2.CAP_GSTREAMER).start() 
    stream = CamGear(source='https://youtu.be/' + video_id, stream_mode = True,logging=False, time_delay=1).start() 
    w = int(stream.stream.get(cv2.CAP_PROP_FRAME_WIDTH))
    h = int(stream.stream.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = stream.stream.get(cv2.CAP_PROP_FPS)
    print(f"stream ready! info: w {w}, h {h}, fps {fps}")

    while fps > 62:
        print("Something wrong in FPS... wait 10 seconds to retry...")
        time.sleep(10)
        stream = CamGear(source='https://youtu.be/' + video_id, stream_mode = True,logging=False, time_delay=1).start()
        w = int(stream.stream.get(cv2.CAP_PROP_FRAME_WIDTH))
        h = int(stream.stream.get(cv2.CAP_PROP_FRAME_HEIGHT))
        fps = stream.stream.get(cv2.CAP_PROP_FPS)
        print(f"stream ready! info: w {w}, h {h}, fps {fps}")

    start_time = datetime.datetime.now()    
    start_time = start_time.replace(second=0)
    if start_time.hour == 23:
        new_hour = 0
    else:
        new_hour = start_time.hour + 1
    # setup proper start + end time
    if clip_length == 15:
        if start_time.minute < 15:
            start_time = start_time.replace(minute=15)
        elif start_time.minute < 30:
            start_time = start_time.replace(minute=30)
        elif start_time.minute < 45:
            start_time = start_time.replace(minute=45)
        else:
            start_time = start_time.replace(minute=0, hour=new_hour)
            if new_hour == 0:
                start_time = start_time + datetime.timedelta(days=1)
    elif clip_length == 30:
        if start_time.minute < 30:
            start_time = start_time.replace(minute=30)
        else:
            start_time = start_time.replace(minute=0, hour=new_hour)
            if new_hour == 0:
                start_time = start_time + datetime.timedelta(days=1)
    elif clip_length == 60:
        start_time = start_time.replace(minute=0, hour=new_hour)
        if new_hour == 0:
            start_time = start_time + datetime.timedelta(days=1)
    elif clip_length == 120:
        start_time = start_time.replace(minute=0)
        if start_time.hour >= 22:
            start_time = start_time.replace(day=start_time.day+1, hour=0)
        else:
            if start_time.hour % 2 == 0:
                new_hour = start_time.hour + 2
            else:
                new_hour = start_time.hour + 1
            start_time = start_time.replace(hour=new_hour)    
    if start_now:
        end_time = start_time
        start_time = datetime.datetime.now()
    else:
        end_time = start_time + datetime.timedelta(minutes=clip_length)
        

    time_info = start_time.strftime("%Y%m%d%H%M") + "__" + end_time.strftime("%Y%m%d%H%M")
    writer = cv2.VideoWriter(f"{path}/{time_info}.mp4", cv2.VideoWriter_fourcc(*'mp4v'), fps, (w, h))
    frames_written = 0
    minutes_recorded = 0
    if start_now:
        print(f'Start recording... {start_time.strftime("%Y%m%d%H%M")} to {end_time.strftime("%Y%m%d%H%M")}')
    frames_to_write = fps * (end_time - start_time).seconds
    while True:
        frame = stream.read()

        if frame is None:
            break
        if start_now:
            writer.write(frame)
            if frames_written > frames_to_write:
                print("one clip is finished".center(50, "*"))
                break
            if frames_written % (60 * fps) == 0:
                print(f"    Recording ... {minutes_recorded}/{(end_time - start_time).seconds//60} minutes")
                minutes_recorded += 1
            frames_written += 1
        else:
            if datetime.datetime.now() >= start_time:
                start_now = True
                print(f'Start recording... {start_time.strftime("%Y%m%d%H%M")} to {end_time.strftime("%Y%m%d%H%M")}')
            # else:
            #     time.sleep(1)
    print("Recording completed!")
    writer.release()

if __name__ == "__main__":
    main()