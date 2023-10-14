import argparse, sys, subprocess, shutil

try:
	import wget
except:
	python = sys.executable
	print("installing wget...")
	subprocess.check_call([python, '-m', 'pip', 'install', 'wget'], stdout=subprocess.DEVNULL)
	import wget


if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('--model', type=str, default='yolov7', help='the name of model, type one of yolov7,\
		yolov7-d6, yolov7-e6, yolov7-e6e, yolov7-tiny, yolov7-w6, yolov7x')
	parser.add_argument('--yolov7_path', type=str, default="", help="the location of yolo7 project")
	arg = parser.parse_args()
	if (arg.model not in ['yolov7', 'yolov7-d6', 'yolov7-e6', 'yolov7-e6e', 'yolov7-w6', 'yolov7x']):
		print("the model should be one of: 'yolov7', 'yolov7-d6', 'yolov7-e6', 'yolov7-e6e', 'yolov7-w6', 'yolov7x'!")
	else:
		url = f"https://github.com/WongKinYiu/yolov7/releases/download/v0.1/{arg.model}_training.pt"
		print(f"Downloading {arg.model}_training.pt:")
		wget.download(url)
		if arg.yolov7_path:
			shutil.move(f"{arg.model}_training.pt", f"{arg.yolov7_path}/{arg.model}_training.pt")
		print("\nDone!")


