import time
from datetime import datetime, timedelta
import logging
from twilio.rest import Client
import requests
import re
import yaml
import cv2
import os

def SendMessage(twilio_account_sid, twilio_auth_token, twilio_phone_number, to_phone_number, message):
    client = Client(twilio_account_sid, twilio_auth_token)
    message = client.messages.create(
         body=message,
         from_ =  twilio_phone_number,
         to = to_phone_number
    )
    message.sid # message 

def CombineVideo(source_dir, monitor_name, target_date):
    '''
    get the one day video info from separate clips    
    '''
    one_day_clips = [c for c in os.listdir(source_dir) if monitor_name in c and target_date in c]
    ## Possible Error here need to send SMS notification: video missing!!!
        
    

def EstimateAnalysisFeasibility():
    pass

def RunDetection():
    pass

def RunCounting():
    pass

def PreserveVideo():
    pass

def RemoveVideo():
    '''
    maybe remove the clips after 1 weeks which all the anlysis is done perfectly?
    '''
    pass

def SendServer():
    pass

def LoadConfig():
    pass

if __name__ == "__main__":
    while 1:
        time.sleep(60)
        if time.strftime("%H:%M") == "15:26":
            print("It's the time!")        
            
    SendMessage()


    # use arg parse to get the input
    # create a .bash file from UI