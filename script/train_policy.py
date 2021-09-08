import numpy as np
import pandas as pd

from pydlshogi.common import *
from pydlshogi.features import *
from pydlshogi.read_kifu import *

import tensorflow as tf
from tensorflow.keras.datasets import mnist
from tensorflow import keras
from tensorflow.keras.layers import Dense, Activation, Conv2D, Flatten
from tensorflow.keras.models import Sequential
from tensorflow.keras.utils import to_categorical

import argparse
import random
import pickle
import os
import re

import logging


def mini_batch(positions, i, batchsize):
    mini_batch_data = []
    mini_batch_move = []
    for b in range(batchsize):
        features, move, win = make_features(positions[i + b])
        mini_batch_data.append(features)
        mini_batch_move.append(move)

    return (np.array(mini_batch_data, dtype=np.float32),
            np.array(mini_batch_move, dtype=np.int32))


def mini_batch_for_test(positions, batchsize):
    mini_batch_data = []
    mini_batch_move = []
    for b in range(batchsize):
        features, move, win = make_features(random.choice(positions))
        mini_batch_data.append(features)
        mini_batch_move.append(move)

    return (np.array(mini_batch_data, dtype=np.float32),
            np.array(mini_batch_move, dtype=np.int32))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('kifulist_train', type=str, help='train kifu list')
    parser.add_argument('kifulist_test', type=str, help='test kifu list')
    parser.add_argument('--batchsize', '-b', type=int, default=32,
                        help='Number of positions in each mini-batch')
    parser.add_argument('--test_batchsize', type=int, default=512,
                        help='Number of positions in each test mini-batch')
    parser.add_argument('--epoch', '-e', type=int, default=1,
                        help='Number of epoch times')
    parser.add_argument('--model', type=str,
                        default='model/model_policy', help='model file name')
    parser.add_argument('--state', type=str,
                        default='model/state_policy', help='state file name')
    parser.add_argument('--initmodel', '-m', default='',
                        help='Initialize the model from given file')
    parser.add_argument('--resume', '-r', default='',
                        help='Resume the optimization from snapshot')
    parser.add_argument('--log', default=None, help='log file path')
    parser.add_argument('--lr', type=float, default=0.01, help='learning rate')
    parser.add_argument('--eval_interval', '-i', type=int,
                        default=1000, help='eval interval')
    parser.add_argument('--min_rate', type=float, default=-1)
    args = parser.parse_args()

    logging.basicConfig(format='%(asctime)s\t%(levelname)s\t%(message)s',
                        datefmt='%Y/%m/%d %H:%M:%S', filename=args.log, level=logging.DEBUG)

    # Init/Resume
    if args.initmodel:
        logging.info('Load model from {}'.format(args.initmodel))
    if args.resume:
        logging.info('Load optimizer state from {}'.format(args.resume))

    logging.info('read kifu start')
    # 保存済みのpickleファイルがある場合、pickleファイルを読み込む
    # train date
    train_pickle_filename = re.sub(
        r'\..*?$', '', args.kifulist_train) + '.pickle'
    if os.path.exists(train_pickle_filename):
        with open(train_pickle_filename, 'rb') as f:
            positions_train = pickle.load(f)
        logging.info('load train pickle')
    else:
        df_train = pd.read_csv(args.kifulist_test, index_col=0)
        df_train = df_train[df_train.min(axis=1) > args.min_rate]
        logging.info("df_train %s", str(df_train.shape))
        positions_train = read_kifu(df_train.index)

    # test data
    test_pickle_filename = re.sub(
        r'\..*?$', '', args.kifulist_test) + '.pickle'
    if os.path.exists(test_pickle_filename):
        with open(test_pickle_filename, 'rb') as f:
            positions_test = pickle.load(f)
        logging.info('load test pickle')
    else:
        df_test = pd.read_csv(args.kifulist_test, index_col=0)
        df_test = df_test[df_test.min(axis=1) > args.min_rate]
        logging.info("df_test %s", str(df_test.shape))
        positions_test = read_kifu(df_test.index)

    # 保存済みのpickleがない場合、pickleファイルを保存する
    logging.debug("train_pickle_filename %s", train_pickle_filename)
    logging.debug("test_pickle_filename %s", test_pickle_filename)
    if not os.path.exists(train_pickle_filename):
        with open(train_pickle_filename, 'wb') as f:
            pickle.dump(positions_train, f, pickle.HIGHEST_PROTOCOL)
        logging.info('save train pickle')
    if not os.path.exists(test_pickle_filename):
        with open(test_pickle_filename, 'wb') as f:
            pickle.dump(positions_test, f, pickle.HIGHEST_PROTOCOL)
        logging.info('save test pickle')
    logging.info('read kifu end')

    logging.info('train position num = {}'.format(len(positions_train)))
    logging.info('test position num = {}'.format(len(positions_test)))

    # train
    x_train, y_train = mini_batch(positions_train, 0, len(positions_train))
    logging.info('start training')

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
                  loss='sparse_categorical_crossentropy', metrics=['accuracy'])

    print(model.summary())
    # Early stoppingを適用してフィッティング
    log = model.fit(x_train, y_train, epochs=1, batch_size=512, validation_split=0.2, verbose=True,
                    callbacks=[keras.callbacks.EarlyStopping(monitor='val_loss',
                                                             min_delta=0, patience=10,
                                                             verbose=1)],
                    )
