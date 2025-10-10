#!/usr/bin/env python

from sb3_contrib import MaskablePPO
from sb3_contrib.common.wrappers import ActionMasker
from stable_baselines3.common.logger import Logger, CSVOutputFormat, HumanOutputFormat

import gymnasium as gym

from minigrid.wrappers import RGBImgObsWrapper, ImgObsWrapper

import os

from utils import MiniGridShieldHandler, ShieldingConfig, MiniWrapper, shield_needed, plot_sb3_metric
from sb3utils import MiniGridSbShieldingWrapper, InfoCallback

import os, sys


GRID_TO_PRISM_BINARY=os.getenv("M2P_BINARY")

def mask_fn(env: gym.Env):
    return env.create_action_mask()

def nomask_fn(env: gym.Env):
    return [1.0] * 7

def main():
    env = "MiniGrid-WindyCity2-v0"
    formula = "Pmax=? [G ! AgentIsOnLava]"
    shield_comparison =  "absolute"

    value_for_training = 1.0
    shielding = ShieldingConfig.Disabled

    experiment_name = "probabilistic_unshielded"
    log_directory = "/opt/logresults"
    log_file = f"{log_directory}/{experiment_name}.csv"

    logger = Logger(log_directory, output_formats=[HumanOutputFormat(sys.stdout), CSVOutputFormat(log_file)])

    env = gym.make(env, render_mode="rgb_array")
    env = RGBImgObsWrapper(env, 8)
    env = ImgObsWrapper(env)
    env = MiniWrapper(env)


    env.reset()

    shield_handlers = dict()
    if shield_needed(shielding):
        for value in [value_for_training]:
            shield_handler = MiniGridShieldHandler(GRID_TO_PRISM_BINARY, "grid.txt", "grid.prism", formula, shield_value=value, shield_comparison=shield_comparison, nocleanup=True, prism_file=None)
            env = MiniGridSbShieldingWrapper(env, shield_handler=shield_handler, create_shield_at_reset=False)
            shield_handlers[value] = shield_handler

    if shielding == ShieldingConfig.Training:
        env = MiniGridSbShieldingWrapper(env, shield_handler=shield_handlers[value_for_training], create_shield_at_reset=False)
        env = ActionMasker(env, mask_fn)
    elif shielding == ShieldingConfig.Disabled:
        env = ActionMasker(env, nomask_fn)
    else:
        assert(False)
    model = MaskablePPO("CnnPolicy", env, verbose=1, device="auto")
    model.set_logger(logger)
    steps = 400000

    model.learn(steps,callback=[InfoCallback()])

    plot_sb3_metric(log_file, "info/sum_crashed_into_building", f"{log_directory}/{experiment_name}_violations.png", "Probabilistic Unshielded Violations")
    plot_sb3_metric(log_file, "rollout/ep_rew_mean",            f"{log_directory}/{experiment_name}_rewards.png", "Probabilistic Unshielded Rewards")


if __name__ == '__main__':
    print("Starting the training")
    main()
