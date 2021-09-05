#!python3
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys

if __name__ == "__main__":
    df = pd.read_csv(sys.argv[1], index_col=0)

    min_rate = 4400
    min_tesuu = 50
    df = df[(df.black_rate >= min_rate) & (df.white_rate >= min_rate)]
    df = df[df.move_num >= min_tesuu]
    print(df.info())

    df[["black_rate", "white_rate"]].min(axis=1).plot(kind='hist')
    plt.figure()
    df["move_num"].plot(kind='hist')

    df.head()
    df.to_csv("output_filterd.csv")
    plt.show()
