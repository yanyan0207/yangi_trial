import numpy as np
from sklearn.model_selection import train_test_split
import tensorflow as tf
from tensorflow.keras.datasets import mnist
from tensorflow import keras
from tensorflow.keras.layers import Dense, Activation, Conv2D, Flatten
from tensorflow.keras.models import Sequential
from tensorflow.keras.utils import to_categorical
import sys
import pandas as pd


def makeHHandleLabel(df):
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


if __name__ == "__main__":
    csv = sys.argv[1]

    df = pd.read_csv(csv, index_col=0)
    df = df[df["Tesuu"] % 2 == 1]
    df = df[:10000]

    # (駒8 なり駒6 持ち駒38( 歩18 + 香4 + 桂4 + 銀4 + 金4 + 角2 + 飛2)) x 2
    data = np.zeros((len(df), 104, 81))
    board = df.loc[:, [c for c in df.columns if c[:2] == "FP"]].values

    col = 0
    for teban in range(2):
        # 盤上の駒
        for i in range(14):
            data[:, col, :][board == (i + 1) * (1 if teban == 0 else -1)] = 1

        # 持ち駒
        koma_list = ["FU", "KY", "KE", "GI", "KA", "HI"]
        koma_num_list = [18, 4, 4, 4, 2, 2]

        for koma, koma_num in zip(koma_list, koma_num_list):
            for i in range(koma_num):
                mochi = np.zeros(data.shape[0])
                mochi[df[("FB" if teban == 0 else "FW") + koma] > i] = 1
                data[:, col] = np.expand_dims(mochi, axis=-1)
        col += 1

    X = data.reshape(data.shape[0], 104, 9, 9)
    makeHHandleLabel(df)

    y = to_categorical(df["HHandLabel"], num_classes=81*27)
    x_train, x_test, y_train, y_test = train_test_split(X, y, test_size=0.2)

    # Sequentialクラスを使ってモデルを準備する
    model = Sequential()

    # 先に作成したmodelへレイヤーを追加
    model.add(Conv2D(32, kernel_size=(3, 3),
                     activation='relu',
                     input_shape=(104, 9, 9)))
    # model.add(Dropout(0.25))
    model.add(Flatten())
    model.add(Dense(81 * 27 * 2, activation='relu'))
    # 出力層を追加
    model.add(Dense(81 * 27, activation='softmax'))

    # TensorFlowのモデルを構築
    model.compile(optimizer=tf.optimizers.Adam(0.01),
                  loss='categorical_crossentropy', metrics=['accuracy'])

    print(model.summary())
    # Early stoppingを適用してフィッティング
    log = model.fit(X, y, epochs=1, batch_size=512, validation_split=0.2, verbose=True,
                    callbacks=[keras.callbacks.EarlyStopping(monitor='val_loss',
                                                             min_delta=0, patience=10,
                                                             verbose=1)],
                    )
