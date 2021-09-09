import numpy as np
import pandas as pd

import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
from tensorflow.keras.layers import Dense, Activation, Conv2D, Flatten
from tensorflow.python.keras.layers.core import Permute

from pydlshogi.common import *
from pydlshogi.features import *
from pydlshogi.read_kifu import *

import argparse
import random
import pickle
import os
import time
from pathlib import Path

import logging

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
parser.add_argument('--min_rate', type=float, default=3500)
parser.add_argument('--min_rate_train', type=float)
parser.add_argument('--min_rate_test', type=float)
args = parser.parse_args()

logging.basicConfig(format='%(asctime)s\t%(levelname)s\t%(message)s',
                    datefmt='%Y/%m/%d %H:%M:%S', filename=args.log, level=logging.DEBUG)

# Init/Resume
if args.initmodel:
    logging.info('Load model from {}'.format(args.initmodel))
    # TODO initmodelオプション対応
    # serializers.load_npz(args.initmodel, model)
if args.resume:
    logging.info('Load optimizer state from {}'.format(args.resume))
    # TODO resumeオプション対応
    # serializers.load_npz(args.resume, optimizer)

min_rate_train = args.min_rate_train if args.min_rate_train else args.min_rate
min_rate_test = args.min_rate_test if args.min_rate_test else args.min_rate

logging.info('read kifu start')
# 保存済みのpickleファイルがある場合、pickleファイルを読み込む
# train date
train_pickle_filename = f"{Path(args.kifulist_train).stem}_minrate{min_rate_train}.pickle"
if os.path.exists(train_pickle_filename):
    with open(train_pickle_filename, 'rb') as f:
        positions_train = pickle.load(f)
    logging.info('load train pickle')
else:
    df_kifu_train = pd.read_csv(args.kifulist_train, index_col=0)
    df_kifu_train = df_kifu_train[df_kifu_train.min(axis=1) >= min_rate_train]
    logging.info("kifulist train %d", len(df_kifu_train))
    positions_train = read_kifu(df_kifu_train.index)

# test data
test_pickle_filename = f"{Path(args.kifulist_test).stem}_minrate{min_rate_test}.pickle"
if os.path.exists(test_pickle_filename):
    with open(test_pickle_filename, 'rb') as f:
        positions_test = pickle.load(f)
    logging.info('load test pickle')
else:
    df_kifu_test = pd.read_csv(args.kifulist_test, index_col=0)
    df_kifu_test = df_kifu_test[df_kifu_test.min(axis=1) >= min_rate_test]
    logging.info("kifulist test %d", len(df_kifu_test))
    positions_test = read_kifu(df_kifu_test.index)

# 保存済みのpickleがない場合、pickleファイルを保存する
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

# mini batch


def mini_batch(positions, i, batchsize):
    mini_batch_data = []
    mini_batch_move = []
    for b in range(batchsize):
        features, move, win = make_features(positions[i + b])
        mini_batch_data.append(features)
        mini_batch_move.append(move)

    return (np.array(mini_batch_data, dtype=np.float32).transpose(0, 2, 3, 1),
            np.array(mini_batch_move, dtype=np.int32))


def mini_batch_for_test(positions, batchsize):
    mini_batch_data = []
    mini_batch_move = []
    for b in range(batchsize):
        features, move, win = make_features(random.choice(positions))
        mini_batch_data.append(features)
        mini_batch_move.append(move)

    return (np.array(mini_batch_data, dtype=np.float32).transpose(0, 2, 3, 1),
            np.array(mini_batch_move, dtype=np.int32))


inputs = keras.Input(shape=(9, 9, 104), name="digits")
x = Conv2D(32, kernel_size=(3, 3),
           activation='relu', padding='same')(inputs)
x = Conv2D(27, kernel_size=(1, 1),
           activation='relu')(x)
x = Permute((2, 3, 1))(x)
x = Flatten()(x)
outputs = x
model = keras.Model(inputs=inputs, outputs=outputs)
print(model.summary())
# Instantiate an optimizer.
optimizer = keras.optimizers.SGD(learning_rate=1e-3)
# Instantiate a loss function.
loss_fn = keras.losses.SparseCategoricalCrossentropy(from_logits=True)

# Prepare the metrics.
train_acc_metric = keras.metrics.SparseCategoricalAccuracy()
val_acc_metric = keras.metrics.SparseCategoricalAccuracy()

# train
logging.info('start training')
itr = 0
sum_loss = 0
for e in range(args.epoch):
    logging.info("\nStart of epoch %d" % (e,))
    start_time = time.time()

    positions_train_shuffled = random.sample(
        positions_train, len(positions_train))

    itr_epoch = 0
    sum_loss_epoch = 0
    for step in range(0, len(positions_train_shuffled) - args.batchsize, args.batchsize):
        x_batch_train, y_batch_train = mini_batch(
            positions_train_shuffled, step, args.batchsize)
        # Open a GradientTape to record the operations run
        # during the forward pass, which enables auto-differentiation.
        with tf.GradientTape() as tape:

            # Run the forward pass of the layer.
            # The operations that the layer applies
            # to its inputs are going to be recorded
            # on the GradientTape.
            # Logits for this minibatch
            logits = model(x_batch_train, training=True)

            # Compute the loss value for this minibatch.
            loss_value = loss_fn(y_batch_train, logits)

        # Use the gradient tape to automatically retrieve
        # the gradients of the trainable variables with respect to the loss.
        grads = tape.gradient(loss_value, model.trainable_weights)

        # Run one step of gradient descent by updating
        # the value of the variables to minimize the loss.
        optimizer.apply_gradients(zip(grads, model.trainable_weights))

        # Update training metric.
        train_acc_metric.update_state(y_batch_train, logits)

        # Log every 200 batches.
        if step % 200 == 0:
            print(
                "Training loss (for one batch) at step %d: %.4f"
                % (step, float(loss_value))
            )
            print("Seen so far: %s samples" % ((step + 1) * args.batchsize))

    # Display metrics at the end of each epoch.
    train_acc = train_acc_metric.result()
    print("Training acc over epoch: %.4f" % (float(train_acc),))

    # Reset training metrics at the end of each epoch
    train_acc_metric.reset_states()

    # Run a validation loop at the end of each epoch.
    for step in range(0, len(positions_test) - args.batchsize, args.batchsize):
        x_batch_val, y_batch_val = mini_batch(
            positions_test, step, args.batchsize)
        val_logits = model(x_batch_val, training=False)
        # Update val metrics
        val_acc_metric.update_state(y_batch_val, val_logits)
    val_acc = val_acc_metric.result()
    val_acc_metric.reset_states()
    print("Validation acc: %.4f" % (float(val_acc),))
    print("Time taken: %.2fs" % (time.time() - start_time))
logging.info('save the model')
logging.info('save the optimizer')
