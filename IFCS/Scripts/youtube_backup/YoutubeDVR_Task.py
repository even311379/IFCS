from vidgear.gears import CamGear
import cv2
import argparse
import datetime
import time

def main():
    parser = argparse.ArgumentParser(description="Automate backup youtube streams to your local disk")
    parser.add_argument('--video_id', type=str, help='the video id for your target youtube video. /n \
        Take a random youtube video clip as example: https://www.youtube.com/watch?v=lro-8GUX9Yk the ID is lro-8GUX9Yk')
    parser.add_argument('--clip_length', type=int, help='the length of each clips you want to store, should be 15, 30, or 60',
                        choices=[15, 30, 60], default=60)
    parser.add_argument('--path', type=str, help='the absolute path to where you want to store the clips')
    args = parser.parse_args()
    if not args.video_id or not args.path:
        print('arguments are missing!')
        return
    print("start recording...")
    video_id = args.video_id
    clip_length = args.clip_length
    path = args.path
    # stream = CamGear(source='https://youtu.be/' + video_id,stream_mode = True,logging=True, backend=cv2.CAP_GSTREAMER).start() 
    stream = CamGear(source='https://youtu.be/' + video_id,stream_mode = True,logging=True).start() 
    w = int(stream.stream.get(cv2.CAP_PROP_FRAME_WIDTH))
    h = int(stream.stream.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = stream.stream.get(cv2.CAP_PROP_FPS)
    print(f"video info: w {w}, h {h}, fps {fps}")
    now = datetime.datetime.now()    
    now = now + datetime.timedelta(seconds=60) # make correct minute in response for reserve time
    if clip_length == 15:
        if now.minute < 15:
            now_string = now.strftime("%Y%m%d-%H%M-%H15")
        elif now.minute < 30:
            now_string = now.strftime("%Y%m%d-%H%M-%H30")
        elif now.minute < 45:    
            now_string = now.strftime("%Y%m%d-%H%M-%H45")
        else:
            now_string = now.strftime("%Y%m%d-%H%M") + f"-{str(now.hour+1).zfill(2)}00"
    elif clip_length == 30:
        if now.minute < 30:
            now_string = now.strftime("%Y%m%d-%H%M-%H30")
        else:
            now_string = now.strftime("%Y%m%d-%H%M") + f"-{str(now.hour+1).zfill(2)}00"
    else:
        now_string = now.strftime("%Y%m%d-%H%M") + f"-{str(now.hour+1).zfill(2)}00"

    # print("recording streams...")
    writer = cv2.VideoWriter(f"{path}/{now_string}.mp4", cv2.VideoWriter_fourcc(*'mp4v'), fps, (w, h))
    frames_written = 0
    minus_minute = now.minute % clip_length
    start = datetime.datetime.now().second == 0
    while True:
        if not start:
            time.sleep(1)
            start = datetime.datetime.now().second == 0
        if start:
            frame = stream.read()
            if frame is None:
                break
            writer.write(frame)
            key = cv2.waitKey(1) & 0xFF
            if key == ord("q"):
                break
            if frames_written > fps * 60 * (clip_length - minus_minute):
                print("one clip is finished".center(50, "*"))
                break
            if frames_written % (60 * fps) == 0:
                print("... Recording ... 1 minute passed")
            frames_written += 1

    writer.release()

if __name__ == "__main__":
    main()