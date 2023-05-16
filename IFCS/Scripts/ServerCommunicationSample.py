import requests
from datetime import datetime, timedelta

s = requests.Session()
s.auth = ('YOUR_SERVER_ACCOUNT', 'YOUR_SERVER_PASSWORD')
s.headers.update({"Content-Type": "application/json"})

data = {}
data["camera"] = "樹下"
data["detection_model"] = "好模型"

# must use this format ?
data["event_time"] = "2023-05-16T18:00:00+08:00"
data["event_period"] = "00:05:00"
data["analysis_type"] = "CO"
data["analysis_time"] = "2023-05-16T18:00:00+08:00"
data["can_analysis"] = "true"
data["analysis_log"] = "test log from code... 32146"
# data[""] = "香魚"
s.post('http://127.0.0.1:8000/api/fish_analysis', json=[data])
# payload = {
#     'inUserName': 'even',
#     'inUserPass': 'Freelancer2023AALife'
# }

# # Use 'with' to ensure the session context is closed after use.
# with requests.Session() as s:
#     p = s.post('http://127.0.0.1:8000/admin/login/', data=payload)
#     # print the html returned or something more intelligent to see if it's a successful login page.
#     # print p.text

#     # An authorised request.
#     print(r.text)
#         # etc...
