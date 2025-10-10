import gymnasium as gym
import numpy as np
import random

from utils import MiniGridShieldHandler, common_parser
from stable_baselines3.common.callbacks import BaseCallback, CheckpointCallback
from stable_baselines3.common.logger import Image

class MiniGridSbShieldingWrapper(gym.core.Wrapper):
    def __init__(self,
                 env,
                 shield_handler : MiniGridShieldHandler,
                 create_shield_at_reset = False,
                 ):
        super().__init__(env)
        self.shield_handler = shield_handler
        self.create_shield_at_reset = create_shield_at_reset

        shield = self.shield_handler.create_shield(env=self.env)
        self.shield = shield

    def create_action_mask(self):
        try:
            return self.shield[self.env.get_symbolic_state()]
        except:
            return [0.0] * 3 + [0.0] * 4

    def reset(self, *, seed=None, options=None):
        obs, infos = self.env.reset(seed=seed, options=options)

        if self.create_shield_at_reset:
            shield = self.shield_handler.create_shield(env=self.env)
            self.shield = shield
        return obs, infos

    def step(self, action):
        obs, rew, done, truncated, info = self.env.step(action)
        info["no_shield_action"] = not self.shield.__contains__(self.env.get_symbolic_state())
        return obs, rew, done, truncated, info

def parse_sb3_arguments():
    parser = common_parser()
    args = parser.parse_args()

    return args


class InfoCallback(BaseCallback):
    """
    Custom callback for plotting additional values in tensorboard.
    """

    def __init__(self, verbose=0):
        super().__init__(verbose)
        self.sum_goal = 0
        self.sum_lava = 0
        self.sum_collisions = 0
        self.sum_opened_door = 0
        self.sum_picked_up = 0
        self.no_shield_action = 0

    def _on_step(self) -> bool:
        infos = self.locals["infos"][0]
        if infos["reached_goal"]:
            self.sum_goal += 1
        if infos["ran_into_lava"]:
            self.sum_lava += 1
        self.logger.record("info/sum_reached_goal", self.sum_goal)
        self.logger.record("info/sum_crashed_into_building", self.sum_lava)
        if "collision" in infos:
            if infos["collision"]:
                self.sum_collisions += 1
            self.logger.record("info/sum_collision", self.sum_collisions)
        #if "opened_door" in infos:
        #    if infos["opened_door"]:
        #        self.sum_opened_door += 1
        #    self.logger.record("info/sum_opened_door", self.sum_opened_door)
        #if "picked_up" in infos:
        #    if infos["picked_up"]:
        #        self.sum_picked_up += 1
        #    self.logger.record("info/sum_picked_up", self.sum_picked_up)
        #if "no_shield_action" in infos:
        #    if infos["no_shield_action"]:
        #        self.no_shield_action += 1
        #    self.logger.record("info/no_shield_action", self.no_shield_action)
        return True
