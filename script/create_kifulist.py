import logging
import argparse
import os
import glob
import csv


def parseCSA(file):
    with open(file) as f:
        black_rate = -1
        white_rate = -1
        for line in f.readlines():
            if line[:len("'black_rate:")] == "'black_rate:":
                black_rate = line[line.rfind(':') + 1:].rstrip()
            elif line[:len("'white_rate:")] == "'white_rate:":
                white_rate = line[line.rfind(':') + 1:].rstrip()
                break
    return [file, black_rate, white_rate]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("files", nargs="*")
    parser.add_argument("--max_filenum", type=int)
    parser.add_argument("--output_file", default="filelist.csv")

    args = parser.parse_args()

    logging.basicConfig(format='%(levelname)s:%(message)s',
                        level=logging.DEBUG)
    logger = logging.getLogger("logger")
    logger.setLevel(logging.INFO)

    # ファイルリスト
    filelist = []
    for file in args.files:
        if os.path.isfile(file):
            filelist.append(file)
        elif os.path.isdir(file):
            filelist.extend(glob.glob(f"{file}/*.csa"))
    if args.max_filenum:
        filelist = filelist[:args.max_filenum]

    logger.info("file list num %d", len(filelist))

    datas = [parseCSA(f) for f in filelist]

    labels = ["filename", "black_rate", "white_rate"]
    with open(args.output_file, "w") as f:
        writer = csv.writer(f)
        writer.writerow(labels)
        writer.writerows(datas)
