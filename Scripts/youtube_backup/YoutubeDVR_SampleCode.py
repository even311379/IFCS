from vidgear.gears import CamGear
import cv2

'''
a few key points to make it work....

(1) the url must use this manner... the 11 code format

(2) Somehow, must add "key =xxx if key ==q break..." in code, which is very strange
otherwise it won't write the video

'''

stream = CamGear(source='https://youtu.be/zc8XYQnPj-4',stream_mode = True,logging=True).start() 
w = int(stream.stream.get(cv2.CAP_PROP_FRAME_WIDTH))
h = int(stream.stream.get(cv2.CAP_PROP_FRAME_HEIGHT))
fps = stream.stream.get(cv2.CAP_PROP_FPS)
print("Test write 30 minutes of video...")
print(w, h, fps)

# writer = cv2.VideoWriter("Youtube_Record_30Min.mp4", cv2.VideoWriter_fourcc(*'mp4v'), fps, (w, h))
# frames_written = 0
while True:
    frame = stream.read()
    if frame is None:
        break

    print("show something!!!!")
    cv2.imshow("Output", frame)
    # writer.write(frame)
    key = cv2.waitKey(1) & 0xFF
    if key == ord("q"):
        break
    # if frames_written > 30 * 60 * 30:
    #     print("Reading 30s of video is fine!")
    #     break
    # if frames_written % (60 * 30) == 0:
    #     print("1 minute passed")
    
    # frames_written += 1

# writer.release()
cv2.destroyAllWindows()
stream.stop()
