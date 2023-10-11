import argparse
import yaml

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument('--bad_file', type=str)
	arg = parser.parse_args()

	with open(arg.bad_file, 'r') as file:
		good = yaml.safe_load(file)

	with open("Fixed_" + arg.bad_file, 'w') as file:
	    yaml.dump(good, file)