import lightgbm as lgb
import numpy as np
from sklearn.metrics import accuracy_score
from tensorflow.keras.datasets import fashion_mnist
import pandas as pd
import argparse
import sys
from sklearn.model_selection import train_test_split

if __name__ == "__main__":
    csv = sys.argv[1]

    df = pd.read_csv(csv, index_col=0)
    for c in [c for c in df.columns if c[:2] == "FP"]:
        df[c] = df[c].astype('category')

    df = df[df["Tesuu"] % 2 == 1]
    df["HFromRow"] = df["HFrom"] // 9
    df["HFromCol"] = df["HFrom"] % 9
    df["HToRow"] = df["HTo"] // 9
    df["HToCol"] = df["HTo"] % 9
    df["HMoveRow"] = df["HToRow"] - df["HFromRow"]
    df["HMoveCol"] = df["HToCol"] - df["HFromCol"]
    df["HMoveLabel"] = -1
    df.loc[(df["HMoveRow"] == 0) & (df["HMoveCol"] > 0), "HMoveLabel"] = 0
    df.loc[(df["HMoveRow"] == 0) & (df["HMoveCol"] < 0), "HMoveLabel"] = 7
    df.loc[(df["HMoveCol"] == 0) & (df["HMoveRow"] > 0), "HMoveLabel"] = 2
    df.loc[(df["HMoveCol"] == 0) & (df["HMoveRow"] < 0), "HMoveLabel"] = 5
    df.loc[(df["HMoveRow"] == df["HMoveCol"]) & (
        df["HMoveRow"] > 0), "HMoveLabel"] = 3
    df.loc[(df["HMoveRow"] == df["HMoveCol"]) & (
        df["HMoveRow"] < 0), "HMoveLabel"] = 4
    df.loc[(df["HMoveRow"] == df["HMoveCol"] * -1) &
           (df["HMoveRow"] > 0), "HMoveLabel"] = 1
    df.loc[(df["HMoveRow"] == df["HMoveCol"] * -1) &
           (df["HMoveRow"] < 0), "HMoveLabel"] = 6
    df.loc[(df["HMoveRow"] == -2) & (df["HMoveCol"] == 1), "HMoveLabel"] = 8
    df.loc[(df["HMoveRow"] == -2) & (df["HMoveCol"] == -1), "HMoveLabel"] = 9
    df["HMoveLabel"] += 10 * df["HNari"]
    df.loc[df["HFrom"] == 81, "HMoveLabel"] = 20 + \
        df[df["HFrom"] == 81]["HKoma"] - 1
    df["HHandLabel"] = df["HTo"] * 27 + df["HMoveLabel"]

    if np.any(df["HMoveLabel"] == -1):
        print("HMoveLabel Error")
        df[df["HMoveLabel"] == -1].to_csv("df_error.csv")
        sys.exit(1)
    if np.any(df["HHandLabel"] == -1):
        print("HHandLabel Error")
        df[df["HHandLabel"] == -1].to_csv("df_error.csv")
        sys.exit(1)

    df.to_csv("df.csv")
    train, test = train_test_split(df, test_size=0.2)
    fcolumns = [c for c in train.columns if c[0] == "F"]
    x_train = train.loc[:, fcolumns]
    y_train = train["HHandLabel"]
    x_test = test.loc[:, fcolumns]
    y_test = test["HHandLabel"]

    print(x_train.head())
    print(y_train.head())

    lgb_train = lgb.Dataset(x_train, y_train)
    lgb_eval = lgb.Dataset(x_test, y_test, reference=lgb_train)

    params = {
        'objective': 'multiclass',
        'num_class': 81 * 27,
        'num_boost_round': 300,
        'early_stopping_round': 10,
        'metric':
            {'multi_logloss'},
    }

    gbm = lgb.train(params, lgb_train, valid_sets=lgb_eval)

    print("train:", len(y_train))
    y_pred = gbm.predict(x_train, num_iteration=gbm.best_iteration)
    y_prex_max = np.argmax(y_pred, axis=1)
    print(accuracy_score(y_prex_max, y_train))

    print("test:", len(y_test))
    y_pred = gbm.predict(x_test, num_iteration=gbm.best_iteration)
    y_prex_max = np.argmax(y_pred, axis=1)
    print(accuracy_score(y_prex_max, y_test))

    # 0.8981
