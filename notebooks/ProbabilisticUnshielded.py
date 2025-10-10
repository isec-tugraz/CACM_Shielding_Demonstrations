#!/usr/bin/env python
# coding: utf-8

# # Probabilistic Shielding

# In[1]:


from sb3_contrib import MaskablePPO
from sb3_contrib.common.wrappers import ActionMasker
from stable_baselines3.common.logger import Logger, CSVOutputFormat, TensorBoardOutputFormat, HumanOutputFormat

import gymnasium as gym

from minigrid.core.actions import Actions
from minigrid.core.constants import TILE_PIXELS
from minigrid.wrappers import RGBImgObsWrapper, ImgObsWrapper

import tempfile, datetime, shutil

import time
import os

from utils import MiniGridShieldHandler, create_log_dir, ShieldingConfig, MiniWrapper, expname, shield_needed, shielded_evaluation, create_shield_overlay_image
from sb3utils import MiniGridSbShieldingWrapper, parse_sb3_arguments, InfoCallback

import os, sys
from copy import deepcopy

from PIL import Image


# In[2]:


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

    logger = Logger("/opt/logresults", output_formats=[HumanOutputFormat(sys.stdout), CSVOutputFormat("/opt/logresults/probabilistic_unshielded.csv")])

    env = gym.make(env, render_mode="rgb_array")
    #image_env = RGBImgObsWrapper(env, TILE_PIXELS)
    env = RGBImgObsWrapper(env, 8)
    env = ImgObsWrapper(env)
    env = MiniWrapper(env)


    env.reset()
    #Image.fromarray(env.render()).show()

    shield_handlers = dict()
    if shield_needed(shielding):
        for value in [value_for_training]:
            shield_handler = MiniGridShieldHandler(GRID_TO_PRISM_BINARY, "grid.txt", "grid.prism", formula, shield_value=value, shield_comparison=shield_comparison, nocleanup=True, prism_file=None)
            env = MiniGridSbShieldingWrapper(env, shield_handler=shield_handler, create_shield_at_reset=False)


            shield_handlers[value] = shield_handler
    #if shield_needed(shielding):
    #    for value in [0.95, 0.97, 1.0]:
    #        create_shield_overlay_image(image_env, shield_handlers[value].create_shield())
    #        print(f"The shield for shield_value = {value}")

    if shielding == ShieldingConfig.Training:
        env = MiniGridSbShieldingWrapper(env, shield_handler=shield_handlers[value_for_training], create_shield_at_reset=False)
        env = ActionMasker(env, mask_fn)
        #print("Training with shield:")
        #create_shield_overlay_image(image_env, shield_handlers[value_for_training].create_shield())
    elif shielding == ShieldingConfig.Disabled:
        env = ActionMasker(env, nomask_fn)
    else:
        assert(False)
    model = MaskablePPO("CnnPolicy", env, verbose=1, device="auto")
    model.set_logger(logger)
    steps = 400000

    model.learn(steps,callback=[InfoCallback()])



if __name__ == '__main__':
    print("Starting the training")
    main()


# In[ ]:




