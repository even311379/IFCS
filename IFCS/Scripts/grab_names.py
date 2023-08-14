import argparse
from models.experimental import attempt_load

"""
put this file in yolov7 folder and use it!

"""
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--weights", nargs="+", type=str, default="yolov7.pt", help="model.pt path(s)"
    )
    parser.add_argument(
        "--out",
        type=str,
        default="grabed_label_names.txt",
        help="save labeled names to this file",
    )
    weights = parser.parse_args().weights
    out = parser.parse_args().out
    model = attempt_load(weights)
    names = model.module.names if hasattr(model, "module") else model.names
    for name in names:
        print(name)
    with open(out, "w+") as f:
        for name in names:
            f.write(name + "\n")
