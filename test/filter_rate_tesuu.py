#!python3
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys
from argparse import ArgumentParser

if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--cut_rate", type=int, default=4000)
    parser.add_argument("--cut_tesuu", type=int, default=50)
    parser.add_argument("file", type=str)

    args = parser.parse_args()
    min_rate = args.cut_rate
    min_tesuu = args.cut_tesuu
    df = pd.read_csv(args.file, index_col=0)
    df = df[(df.black_rate >= min_rate) & (df.white_rate >= min_rate)]
    df = df[df.move_num >= min_tesuu]
    print(df.info())

    df[["black_rate", "white_rate"]].min(axis=1).plot(kind='hist')
    plt.figure()
    df["move_num"].plot(kind='hist')

    df.to_csv("output_filterd.csv")
