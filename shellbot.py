''' 
Shellbot is a python3 script that allows for manipulation of an XPilot agent from another thread.
Author: Nicholas Lorentzen
Date: 20220426
'''

import math
import pickle
import threading
from typing import List, Tuple, Union
from neat import nn

import libpyAI as ai
import numpy as np
from neat import nn

class ShellBot(threading.Thread):

    ## Initialization Settings
    username: str = "Kerbal"
    headless: bool = False

    ## Neat Info
    nn = None
    
    ## Configuration Settings
    max_speed: int = 10
    desired_speed: int = 6
    safety_margin: float = 1.1
    turnspeed: int = 20
    power_level: float = 28.0
    scan_distance: int = 1000
    cpa_check_distance = 500
    thrust_heading_tolerance: int = 10
    e_thrust_heading_tolerance: int = 15
    wall_threshold: int = 100
    firing_threshold: int = 5
    max_shot_distance: int = 800

    ## Flags For Production System
    turn: bool = False
    thrust: bool = False
    shoot: bool = False
    desired_heading: float = 0
    current_action: int = 5
    last_action_performed: int = 5

    ## Needed for episode reset to work
    reset_now = False
    close_now = False
    frame: int = 0
    started: bool = False
    died_but_no_reset = False

    ## Score of the bot
    score: float = 0
    starting_score: float = 0
    enemy_score: float = 0

    ## State of the bot
    alive: float = 0.0
    last_alive: float = 0.0

    ## Last known radar positions
    last_enemy_radar_x: int = 0
    last_enemy_radar_y: int = 0
    last_radar_x: int = 0
    last_radar_y: int = 0

    ## Properties For Processing
    ## Ship Info
    heading: int = 90
    tracking: int = 90
    heading_x: float = 0
    heading_y: float = 0
    x_vel: float = 0
    y_vel: float = 0
    wall_front: float = 1000
    wall_left: float = 1000
    wall_right: float = 1000
    wall_back: float = 1000
    track_wall: float = 1000
    closest_wall: float = 0
    closest_wall_heading: int = 0
    tt_retro: float = 140.0
    tt_tracking: float = 140.0
    tt_retro_point: float = 140.0

    ## Enemy Info
    enemy_on_screen: float = 0.0
    enemy_delta_x: float = 0
    enemy_delta_y: float = 0
    enemy_x_vel: float = 0
    enemy_y_vel: float = 0
    enemy_cpa_time: float = 1000
    enemy_cpa_dist: float = 1000
    valid_aim: float = 0.0

    ## Shot Info
    shot_on_screen: float = 0.0
    shot_delta_x: float = 0
    shot_delta_y: float = 0
    shot_x_vel: float = 0
    shot_y_vel: float = 0
    lowest_alert: float = 0

    ## Reward Calculation
    last_action_failed = 0.0

    ## Data Logging
    ##save_every: int = 140
    oberservations = []
    actions = []

    def __init__(self, username:str="InitNoName") -> None:
        super(ShellBot, self).__init__()
        self.username = username
        self.max_turntime = math.ceil(180 / self.turnspeed)

    def get_score(self,) -> float:
        return (self.score + 100) ** 1.1
    
    ## For Interfacing with the Environment
    def run(self,) -> None:
        if not self.started:
            self.started = True
            ai.start(self.run_loop, ["-name", self.username, "-join", "localhost"])
        else:
            print("Bot already started")
    
    def reset(self,) -> None:
        self.reset_now = True

    def close_bot(self,) -> None:
        self.close_now = True

    def get_observations(self,) -> List[float]:
        ## Normalize the values
        wall_track = 1.0 - (float(self.track_wall) / float(self.scan_distance))
        closest_wall = 1.0 - (float(self.closest_wall) / float(self.scan_distance))
        wall_front = 1.0 - (float(self.wall_front) / float(self.scan_distance))
        wall_left = 1.0 - (float(self.wall_left) / float(self.scan_distance))
        wall_right = 1.0 - (float(self.wall_right) / float(self.scan_distance))
        wall_back = 1.0 - (float(self.wall_back) / float(self.scan_distance))

        ship_x_vel = self.x_vel / 20.0
        ship_y_vel = self.y_vel / 20.0
        ship_speed = self.speed / 20.0

        enemy_x_vel = self.enemy_x_vel / 20.0
        enemy_y_vel = self.enemy_y_vel / 20.0
        enemy_delta_x = 1.0 - (self.enemy_delta_x / 1500.0)
        enemy_delta_y = 1.0 - (self.enemy_delta_y / 1500.0)
        enemy_dist = 1.0 - (self.get_distance(self.enemy_delta_x, self.enemy_delta_y, 0, 0) / 700.0)

        enemy_cpa_dist = 1.0 - (self.enemy_cpa_dist / 1500.0)
        enemy_cpa_time = 1.0 - (self.enemy_cpa_time / 140.0)

        shot_x_vel = self.shot_x_vel / 40.0
        shot_y_vel = self.shot_y_vel / 40.0
        shot_delta_x = 1.0 - (self.shot_delta_x / 1500.0)
        shot_delta_y = 1.0 - (self.shot_delta_y / 1500.0)
        lowest_alert = 1.0 - (self.lowest_alert / 500.0)
        shot_dist = 1.0 - (self.get_distance(self.shot_delta_x, self.shot_delta_y, self.x, self.y) / 700.0)

        delta_angle_aim = self.angle_diff(self.heading, self.desired_heading) / 180.0


        tt_tracking = 1.0 - (self.tt_tracking / 140.0)
        tt_retro_point = 1.0 - (self.tt_retro_point / 70.0)

        last_action = float(self.last_action_performed) / 8.0

        ## Organize the values
        oberservations = [ship_speed, wall_track, closest_wall, wall_front, wall_back, wall_left, wall_right, enemy_dist, self.enemy_speed, self.enemy_on_screen, self.valid_aim, delta_angle_aim, self.shot_on_screen, shot_dist, lowest_alert, tt_tracking, tt_retro_point, ]

        ## Check Normalization
        for idx in range(0, len(oberservations)):
            num = oberservations[idx]
            if math.isnan(num):
                num = 1.0
            num = min(num, 1.0)
            num = max(num, -1.0)
            oberservations[idx] = num

        return oberservations

    def sigmoid_activation(self, x: np.ndarray) -> np.ndarray:
        return (1/(1+np.exp(-x)))
    
    def forward_propogation(self, inputs: np.ndarray, weights: np.ndarray, biases: np.ndarray) -> np.ndarray:
        activations = self.sigmoid_activation(np.dot(inputs, weights) + biases)
        return activations

    def set_action(self,) -> None:
        ## Get the observations
        observations = self.get_observations()
        ## Forward Propogation
        inputs = np.array(observations)
        outputs = self.nn.activate(inputs)
        action = np.argmax(outputs)
        ## Set the action
        self.current_action = int(action)
    
    ## Running the bot
    def run_loop(self,) -> None:
        ##print(f"Bot starting frame {self.frame}")
        try:
            if self.reset_now:
                self.reset_now = False
                ai.talk("/reset all")
                self.frame = 0
                self.score = 0
                self.enemy_score = 0
                self.alive = False
                self.last_alive = False
                self.current_action = 5
                self.died_but_no_reset = False
                return
            ai.setTurnSpeedDeg(self.turnspeed)
            
            self.reset_flags()
            self.collect_info()
            self.set_action()
            self.perform_action()
            self.production_system()
        except AttributeError:
            pass
        except Exception as e:
            print("Error: " + str(e))
            pass

        self.frame += 1
        if self.last_alive != self.alive:
            if self.alive == 0.0:
                self.died_but_no_reset = True
            self.frame = 0
            ai.thrust(1)
        if self.frame % 56 == 0:
            ai.thrust(1)
        self.last_alive = self.alive

    def production_system(self,) -> None:
        '''production_system Uses the three flags to execute the desired actions for the bot
        '''
        if self.turn and self.thrust and self.shoot:
            self.turn_to_degree(self.desired_heading)
            ai.thrust(1)
            ai.fireShot()
        elif self.turn and self.thrust:
            self.turn_to_degree(self.desired_heading)
            ai.thrust(1)
        elif self.turn and self.shoot:
            self.turn_to_degree(self.desired_heading)
            ai.thrust(0)
            ai.fireShot()
        elif self.thrust and self.shoot:
            ai.thrust(1)
            ai.fireShot()
        elif self.turn:
            self.turn_to_degree(self.desired_heading)
            ai.thrust(0)
        elif self.thrust:
            ai.thrust(1)
        elif self.shoot:
            ai.fireShot()
        else:
            ai.thrust(0)
    
    def reset_flags(self,) -> None:
        '''reset_flags Sets all flags to false and resets control states
        '''
        self.turn, self.thrust, self.shoot = False, False, False
        ai.turnLeft(0)
        ai.turnRight(0)
        ai.thrust(0)
        self.last_action_failed = 0.0
    
    def turn_to_degree(self, degree: float) -> None:
        '''turn_to_degree Turns the bot to the desired heading

        Args:
            degree (float): Heading to turn to
        '''
        delta = self.angle_diff(self.heading, degree)
        if abs(delta) > 20:
            if delta < 0:
                ai.turnRight(1)
            else:
                ai.turnLeft(1)
        else:
            ai.turnToDeg(int(degree))

    def collect_info(self,) -> None:
        ## Basics
        self.last_alive = self.alive
        self.alive = float(ai.selfAlive())

        self.last_score = self.score
        self.score = ai.selfScore()

        enemy_score = ai.enemyScore(0)
        if enemy_score != -1:
            self.enemy_score = enemy_score

        self.x = ai.selfX()
        self.y = ai.selfY()

        self.heading = int(ai.selfHeadingDeg())
        self.heading_x, self.heading_y = self.get_components(self.heading, 1)

        self.speed = ai.selfSpeed()
        self.tracking = int(ai.selfTrackingDeg())
        self.x_vel = ai.selfVelX()
        self.y_vel = ai.selfVelY()

        ## Walls
        self.track_wall = ai.wallFeeler(self.scan_distance, self.tracking)
        self.wall_front = float(ai.wallFeeler(self.scan_distance, int(self.heading)))
        self.wall_back = float(ai.wallFeeler(self.scan_distance, int(self.angle_add(self.heading, 180))))
        self.wall_left = float(ai.wallFeeler(self.scan_distance, int(self.angle_add(self.heading, 90))))
        self.wall_right = float(ai.wallFeeler(self.scan_distance, int(self.angle_add(self.heading, -90))))
        self.wall_45_left = float(ai.wallFeeler(self.scan_distance, int(self.angle_add(self.heading, 45))))
        self.wall_45_right = float(ai.wallFeeler(self.scan_distance, int(self.angle_add(self.heading, -45))))

        ## Timings
        self.tt_tracking = math.ceil(float(self.track_wall) / (self.speed + 0.0000001))
        self.tt_retro = math.ceil(self.speed / float(self.power_level))

        ## Timings to timing pts
        self.tt_retro_point = min(self.tt_tracking - ((self.max_turntime + self.tt_retro) * self.safety_margin + 1), 70.0)

        ## Closest Enemy
        self.valid_aim = 0.0
        if ai.screenEnemyXId(0) == -1 and ai.closestRadarX() == -1:
            self.enemy_on_screen = 0.0
            self.enemy_delta_x = 1000.0
            self.enemy_delta_y = 1000.0
            self.enemy_x_vel = 0.0
            self.enemy_y_vel = 0.0
            self.enemy_cpa_dist = 1000.0
            self.enemy_cpa_time = 1000.0
        elif ai.screenEnemyXId(0) == -1.0 and ai.closestRadarX() != -1:
            self.enemy_on_screen = 0.0
            enemy_radar_x = ai.closestRadarX()
            enemy_radar_y = ai.closestRadarY()

            ship_x = ai.selfRadarX()
            ship_y = ai.selfRadarY()

            self.enemy_delta_x = (enemy_radar_x - ship_x) * 4.37
            self.enemy_delta_y = (enemy_radar_y - ship_y) * 4.37

            self.enemy_x_vel = (enemy_radar_x - self.last_enemy_radar_x) * 4.37
            self.enemy_y_vel = (enemy_radar_y - self.last_enemy_radar_y) * 4.37

            self.enemy_speed = pow(pow(self.enemy_x_vel, 2) + pow(self.enemy_y_vel, 2), 0.5) * 4.37
            enemy_heading = self.get_angle_from_to(self.last_enemy_radar_x, self.last_enemy_radar_y, enemy_radar_x, enemy_radar_y)

            self.enemy_cpa_time, self.enemy_cpa_dist, self.enemy_cpa_x, self.enemy_cpa_y = self.get_cpa_self(enemy_radar_x * 4.37, enemy_radar_y * 4.37, enemy_heading, self.enemy_speed)

        else:
            self.enemy_on_screen = 1.0
            enemy_y = ai.screenEnemyYId(0)
            enemy_x = ai.screenEnemyXId(0)
            self.enemy_x_vel, self.enemy_y_vel = self.get_components(ai.enemyTrackingDegId(0), ai.enemySpeedId(0))
            self.enemy_delta_x = float(self.x - enemy_x)
            self.enemy_delta_y = float(self.y - enemy_y)
            self.enemy_cpa_time, self.enemy_cpa_dist, self.enemy_cpa_x, self.enemy_cpa_y = self.get_cpa_self(enemy_x, enemy_y, ai.enemyHeadingDegId(0), ai.enemySpeedId(0))
            if self.aim_dir_enemy() != -1:
                self.valid_aim = 1.0
        
        self.last_enemy_radar_x = ai.closestRadarX()
        self.last_enemy_radar_y = ai.closestRadarY()

        shot_x = ai.shotX(0)
        if shot_x == -1:
            self.shot_on_screen = 0.0
            self.shot_delta_x = 1000.0
            self.shot_delta_y = 1000.0
            self.shot_x_vel = 0.0
            self.shot_y_vel = 0.0
            self.lowest_alert = 1000.0
            self.lowest_idx = -1
        else:
            self.shot_on_screen = 1.0
            idx = 0
            self.lowest_idx = -1
            self.lowest_alert = 1000.0
            next_shot_alert = ai.shotAlert(idx)
            while next_shot_alert != -1:
                if next_shot_alert < self.lowest_alert:
                    self.lowest_alert = next_shot_alert
                    self.lowest_idx = idx
                idx += 1
                next_shot_alert = ai.shotAlert(idx)
            if self.lowest_alert == -1:
                self.lowest_alert = 1000.0
            shot_y = ai.shotY(self.lowest_idx)
            shot_x = ai.shotX(self.lowest_idx)
            self.shot_x_vel, self.shot_y_vel = self.get_components(ai.shotVelDir(self.lowest_idx), ai.shotVel(self.lowest_idx))
            self.shot_delta_x = float(self.x - shot_x)
            self.shot_delta_y = float(self.y - shot_y)
    
    def perform_action(self,) -> None:
        self.last_action_performed = self.current_action
        if self.current_action == 0:
            self.avoid_walls()
        elif self.current_action == 1:
            self.avoid_shot()
        elif self.current_action == 2:
            self.slow_down()
        elif self.current_action == 3:
            self.kill()
        elif self.current_action == 4:
            self.check_position()
        elif self.current_action == 5:
            self.thrust = True
        elif self.current_action == 6:
            self.desired_heading = self.angle_add(self.heading, 15.0)
            self.turn = True
        elif self.current_action == 7:
            self.desired_heading = self.angle_add(self.heading, -15.0)
            self.turn = True
        self.check_luck()

    ## Action Functions
    def avoid_walls(self, ) -> None:
        '''check_walls Checks for possible wall collisions and sets flags accordingly if necessary
        '''
        self.update_closest_wall()
        self.last_action_failed = 0.0
        self.turn = True
        if self.closest_wall < self.wall_threshold:
            self.desired_heading = self.angle_add(
                self.closest_wall_heading, 180)
            if self.check_heading_tolerance(self.e_thrust_heading_tolerance) or self.tt_tracking < self.tt_retro + 1:
                self.thrust = True
            self.last_action_failed = -2.0
        else:
            if self.tt_retro_point < 0:
                self.last_action_failed = -1.0
            self.desired_heading = self.angle_add(self.tracking, 180)
            if self.check_heading_tolerance():
                self.thrust = True
        ##print(f'speed: {self.speed} tth: {self.tt_tracking} dist: {self.track_wall} ttr: {self.tt_retro}')

    def avoid_shot(self, ) -> None:
        '''check_shots Checks for possible bullet collisions using shot alert values and sets flags accordingly if necessary
        '''
        if self.lowest_idx == -1:
            self.last_action_failed = 1.0
            return
        self.last_action_failed = -1.0
        if self.lowest_alert < 50:
            self.last_action_failed = -2.0
        shot_speed = ai.shotVel(self.lowest_idx)
        shot_heading = ai.shotVelDir(self.lowest_idx)
        shot_x = ai.shotX(self.lowest_idx)
        shot_y = ai.shotY(self.lowest_idx)
        self.turn = True
        tt_cpa, dt_cpa, cpa_x, cpa_y = self.get_cpa_self(shot_x, shot_y, shot_heading, shot_speed, t_step=0.2)
        cpa_x += self.x_vel * 3
        cpa_y += self.y_vel * 3
        angle_to_cpa = self.get_angle_to(cpa_x, cpa_y)
        angle_to_shot = self.get_angle_to(shot_x, shot_y)
        closer_shot_tangent = self.get_closer_angle(self.angle_add(shot_heading, 90), self.angle_add(shot_heading, -90))
        ## Heading Directly Towards Shot Or AWay From Shot
        if abs(self.angle_diff(angle_to_shot, self.tracking)) < 10:
            ##print(f'Dodging Head On Shot')
            self.desired_heading = closer_shot_tangent
            self.turn = True
            if self.check_heading_tolerance(10):
                self.thrust = True
            return
        elif abs(self.angle_diff(angle_to_shot, self.angle_add(self.tracking, 180))) < 10:
            ##print(f'Dodging Chasing Shot')
            self.desired_heading = closer_shot_tangent
            self.turn = True
            if self.check_heading_tolerance(15):
                self.thrust = True
            return
        self.desired_heading = self.angle_add(angle_to_cpa, 180)
        if self.check_heading_tolerance() or (tt_cpa > 2 and tt_cpa < self.tt_retro + 2) or self.speed < 1:
            self.thrust = True
        

    def slow_down(self,) -> None:
        '''check_speed Checks if the ship is going faster than the set maximum speed and sets flags accordingly if necessary
        '''
        self.last_action_failed = 0.0
        if self.speed > self.max_speed:
            self.last_action_failed = -1.0
        elif self.speed < self.desired_speed:
            self.last_action_failed = 0.2
        self.desired_heading = self.angle_add(self.tracking, 180)
        self.turn = True
        if self.check_heading_tolerance():
            self.thrust = True
        self.check_luck()

    def kill(self,) -> None:
        '''check_kills Check for an enemy on the screen that can be shot and sets flags accordingly if the enemy can be dealt with
        '''
        if self.aim_dir_enemy(0) != -1:
            self.last_action_failed = -1.0
            self.turn = True
            self.desired_heading = self.aim_dir_enemy(0)
            if self.desired_heading == -1.0:
                self.turn = False
                if self.frame % 133 == 0:
                    self.thrust = True
            if self.check_heading_tolerance(5):
                self.shoot = True
                self.last_action_failed = -2.0
                ##print(f'{self.username}: Shooting')
        else:
            self.last_action_failed = 1.0

    def check_position(self,) -> None:
        '''check_position Brings the ship closer to the center of the screen if it is too far away
        '''
        self.update_closest_wall()
        self.last_action_failed = 0.0

        if self.enemy_on_screen == 0.0 and self.shot_on_screen == 0.0:
            self.last_action_failed = -0.5
        else:
            self.last_action_failed = 0.2

        if self.closest_wall < 400:
            self.turn = True
            self.desired_heading = self.angle_add(
                self.closest_wall_heading, 180)
            delta = abs(self.angle_diff(
                self.tracking, self.closest_wall_heading))
            delta_desired = abs(self.angle_diff(
                self.heading, self.desired_heading))
            if delta_desired < 30 and (delta < 140 or self.speed < self.desired_speed):
                self.thrust = True
        else:
            self.turn = True
            self.desired_heading = self.angle_add(self.tracking, 180)
            if self.check_heading_tolerance(5):
                self.thrust = True
            self.shoot = True

    def check_luck(self,) -> bool:
        '''check_luck Hmmm i wonder if there happens to be an enemy all lined up

        Returns:
            bool: True if someone is about to have a very unfortunate time, otherwise False
        '''        
        nearest_aim_dir = self.aim_dir_enemy(0)
        if nearest_aim_dir != -1:
            delta = abs(self.angle_diff(self.heading, nearest_aim_dir))
            if delta < 10:
                self.shoot = True
                return True
        return False

    ##Utility Functions
    def update_closest_wall(self,) -> None:
        '''update_closest_wall Updates the closest wall distance and heading
        '''
        self.closest_wall = self.scan_distance
        self.closest_wall_heading = -1
        for degree in range(0, 360, 30):
            wall = ai.wallFeeler(self.scan_distance, degree)
            if wall < self.closest_wall:
                self.closest_wall = wall
                self.closest_wall_heading = degree

    def get_closer_angle(self, a1: float, a2: float) -> float:
        '''get_closer_angle Returns whichever heading is closer to the current heading of the ship

        Args:
            a1 (float): First heading
            a2 (float): Second heading

        Returns:
            float: The heading that is closer to the current heading of the ship
        '''        
        if abs(self.angle_diff(self.heading, a1)) < abs(self.angle_diff(self.heading, a2)):
            return a1
        return a2

    def get_cpa_self(self, x, y, heading, speed, t_step: float = 1) -> Tuple[float, float, float, float]:
        '''get_cpa_self Ever wondered how long it will be until some object either hits or passes the ship? No... Well this function calculates that for you.

        Args:
            x (int): X coordinate of the object
            y (int): Y coordinate of the object
            heading (float): heading of the object
            speed (float): speed of the object

        Returns:
            Tuple[float, float, float, float]: The time to CPA in frames and the distance to CPA in pixels and the coordinates of the CPA
        '''
        return self.get_cpa(self.x, self.y, self.tracking, self.speed, x, y, heading, speed, t_step=t_step)

    def get_cpa(self, x1: int, y1: int, h1: int, s1: float, x2: int, y2: int, h2: int, s2: float, max_t: int = 9999, t_step: float = 1) -> Tuple[float, float, float, float]:
        '''get_cpa Ever wondered how long it will be until some object either hits or passes some other object? No... Well this function calculates that for you.

        Args:
            x1 (int): X coordinate of the first object
            y1 (int): Y coordinate of the first object
            h1 (int): heading of the first object
            s1 (float): speed of the first object
            x2 (int): X coordinate of the second object
            y2 (int): Y coordinate of the second object
            h2 (int): heading of the second object
            s2 (float): speed of the second object
            max_t (int, optional): max number of frames to calculate to. Defaults to 9999.
            t_step (float, optional): how much to increment for each step. Defaults to 1.

        Returns:
            Tuple[float, float, float, float]: The time to CPA in frames and the distance to CPA in pixels and the coordinates of the CPA
        '''        
        y_vel_1, x_vel_1 = self.get_components(h1, s1)
        y_vel_2, x_vel_2 = self.get_components(h2, s2)
        found_min = False
        last_dist: float = self.get_distance(x1, x1, x2, y2)
        t: float = 1
        while not found_min and t < max_t:
            current_dist = self.get_distance(
                x1 + x_vel_1 * t, y1 + y_vel_1 * t, x2 + x_vel_2 * t, y2 + y_vel_2 * t)
            if current_dist < last_dist:
                t += t_step
                last_dist = current_dist
            else:
                found_min = True
        if t >= max_t or t == 1:
            return -1, -1, -1, -1
        return t, last_dist, x1 + x_vel_1 * t, y1 + y_vel_1 * t
    
    def aim_dir(self, x2: int, y2: int, h2: int, s2: float, x1: int = -1, y1: int = -1, h1: int = -1, s1: float = -1) -> float:
        '''aim_dir Finds the direction to aim at a target

        Args:
            x2 (int): X coordinate of the target
            y2 (int): Y coordinate of the target
            h2 (int): heading of the target
            s2 (float): speed of the target
            x1 (int, optional): X coordinate of the attacker. Defaults to ship X pos.
            y1 (int, optional): Y coordinate of the attacker. Defaults to ship Y pos.
            h1 (int, optional): heading of the attacker. Defaults to ship tracking.
            s1 (float, optional): speed of the attacker. Defaults to ship speed.

        Returns:
            float: The direction to aim at the target in degrees
        '''        
        ## Default to self if no arguments are passed for attacker
        if x1 == -1 or y1 == -1 or h1 == -1 or s1 == -1:
            x1 = self.x
            y1 = self.y
            h1 = self.tracking
            s1 = self.speed + 0.000000001

        if s2 <= 0.5 or s1 <= 0.1:
            return int(self.get_angle_to(x2, y2))
        
        shot_speed = 21
        
        x_vel_1, y_vel_1 = self.get_components(h1, s1)
        x_vel_2, y_vel_2 = self.get_components(h2, s2)
        
        ## Now we pre aim for the next shot
        modifier = ai.selfReload()
        
        x1 += x_vel_1 * modifier
        y1 += y_vel_1 * modifier
        x2 += x_vel_2 * modifier
        y2 += y_vel_2 * modifier

        ## Everything is relative ya know
        x_diff = x1 - x2
        y_diff = y1 - y2
        x_vel_diff = x_vel_1 - x_vel_2
        y_vel_diff = y_vel_1 - y_vel_2
        relative_speed = pow(pow(x_diff, 2) + pow(y_diff, 2), 0.5)

        if relative_speed <= 0.5:
            return int(self.get_angle_to(x2, y2))

        ## Below is based on the formula found here: 
        ## https://www.gamedeveloper.com/programming/shooting-a-moving-target
        delta_pos = np.array([x_diff, y_diff])
        delta_vel = np.array([x_vel_diff, y_vel_diff])

        a = np.dot(delta_vel, delta_vel) - shot_speed * shot_speed
        b = 2.0 * np.dot(delta_pos, delta_vel)
        c = np.dot(delta_pos, delta_pos)
        discriminant = b * b - 4.0 * a * c

        if discriminant < 0:
            return -1

        t = 2.0 * c / (np.sqrt(discriminant) - b)

        intercept_x = x2 + x_vel_2 * t
        intercept_y = y2 + y_vel_2 * t

        shot_dir = self.get_angle_to(intercept_x, intercept_y)

        ##print(f'Intercept: ({int(intercept_x)},{int(intercept_y)}) {int(shot_dir)} in {t} frames from ({x1},{y1})')
        try:
            return int(shot_dir)
        except Exception:
            return -1
    
    def aim_dir_enemy(self, idx: int = 0) -> float:
        '''aim_dir_enemy Aim at the enemy with the given index

        Args:
            idx (int, optional): Index of enemy. Defaults to 0.

        Returns:
            float: The direction to aim at the enemy in degrees
        '''        
        enemy_x = ai.screenEnemyX(idx)
        if enemy_x == -1:
            return -1
        
        ## We can just use the built in function if it works
        built_aim_dir = ai.aimdir(idx)
        if built_aim_dir > 0:
            return built_aim_dir

        ## Otherwise we have to do our own thing
        enemy_y = ai.screenEnemyY(idx)
        enemy_speed = ai.enemySpeed(idx)
        enemy_tracking = ai.enemyTrackingDeg(idx)
        aim_heading = self.aim_dir(enemy_x, enemy_y, enemy_tracking, enemy_speed,)
        ##print(f'Aiming at {aim_heading} not {built_aim_dir}')
        return aim_heading

    def get_components(self, heading: float, speed: float) -> Tuple[float, float]:
        '''get_components Returns the x and y components of a given heading and speed

        Args:
            heading (float): heading of the object
            speed (float): speed of the object

        Returns:
            Tuple[float, float]: x and y components of the given heading and speed
        '''

        heading_rad: float = math.radians(heading)
        x: float = speed * math.cos(heading_rad)
        y: float = speed * math.sin(heading_rad)

        return x, y

    def get_distance(self, x1: float, y1: float, x2: float, y2: float) -> float:
        '''get_distance Returns the distance between two points

        Args:
            x1 (float): x coordinate of the first point
            y1 (float): y coordinate of the first point
            x2 (float): x coordinate of the second point
            y2 (float): y coordinate of the second point

        Returns:
            float: Distance between the two points
        '''        
        return math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2)

    def get_angle_to(self, x: float, y: float) -> float:
        '''get_angle_to Finds the heading the ship would need to face to get to a point

        Args:
            x (float): x coordinate of the point
            y (float): y coordinate of the point

        Returns:
            float: angle to the point
        '''        
        x_diff = x - self.x
        y_diff = y - self.y

        angle_to = math.degrees(
            math.atan(abs(y_diff) / abs(x_diff + 0.0000001)))

        ## Too lazy for a better way to do this
        ## See libpyAI.c line 2042 for more information
        if x_diff <= 0 and y_diff > 0:
            angle_to = 180 - angle_to
        elif x_diff <= 0 and y_diff <= 0:
            angle_to = 180 + angle_to
        elif x_diff > 0 and y_diff <= 0:
            angle_to = 360 - angle_to
        return angle_to

    def get_angle_from_to(self, x1: float, y1: float, x2: float, y2: float) -> float:
        '''get_angle_to Finds the heading from 1 point to another

        Args:
            x1 (float): x coordinate of the first point
            y1 (float): y coordinate of the first point
            x2 (float): x coordinate of the second point
            y2 (float): y coordinate of the second point

        Returns:
            float: angle to the second point from the first point
        '''        
        x_diff = x2 - x1
        y_diff = y2 - x1

        angle_to = math.degrees(
            math.atan(abs(y_diff) / abs(x_diff + 0.0000001)))

        ## Too lazy for a better way to do this
        ## See libpyAI.c line 2042 for more information
        if x_diff <= 0 and y_diff > 0:
            angle_to = 180 - angle_to
        elif x_diff <= 0 and y_diff <= 0:
            angle_to = 180 + angle_to
        elif x_diff > 0 and y_diff <= 0:
            angle_to = 360 - angle_to
        return angle_to

    def add_vectors(self, h1: int, s1: float, h2: int, s2: float) -> Tuple[float, float]:
        x_vel1, y_vel1 = self.get_components(h1, s1)
        x_vel2, y_vel2 = self.get_components(h2, s2)
        x_vel = x_vel1 + x_vel2
        y_vel = y_vel1 + y_vel2
        return self.get_angle_to(x_vel, y_vel), pow(pow(x_vel, 2) + pow(y_vel, 2), 0.5)

    def check_heading_tolerance(self, tolerance: Union[int, None] = None) -> bool:
        '''check_heading_tolerance Checks if the ship is within a certain tolerance of its desired heading

        Args:
            tolerance (int, optional): number of degrees of error allowed. Defaults to None.

        Returns:
            bool: True if the ship is within the tolerance, otherwise False
        '''        
        if tolerance is None:
            tolerance = self.thrust_heading_tolerance
        if abs(self.angle_diff(self.heading, self.desired_heading)) < tolerance:
            return True
        return False

    def angle_add(self, a1: float, a2: float) -> float:
        '''angle_add Adds two angles together

        Args:
            a1 (float): angle 1
            a2 (float): angle 2

        Returns:
            float: result of adding the two angles
        '''       
        while a1 <= 0:
            a1 += 360
        while a2 <= 0:
            a2 += 360
        return (a1+a2+360) % 360

    def angle_diff(self, a1: float, a2: float) -> float:
        '''angle_diff Finds the difference between two angles

        Args:
            a1 (float): angle 1
            a2 (float): angle 2

        Returns:
            float: result of the difference between the two angles
        '''        
        diff = a2 - a1
        comp_diff = a2 + 360 - a1
        if abs(diff) < abs(comp_diff):
            return diff
        return comp_diff
    
    def get_tt_feeler(self, angle: float, distance: float, x_vel: float, y_vel: float) -> float:
        x_dist, y_dist = self.get_components(angle, distance)
        tt_x = x_dist / (x_vel + 0.0000001)
        tt_y = y_dist / (y_vel + 0.0000001)
        tt_feel = min(tt_x, tt_y)
        return tt_feel

if __name__ == "__main__":
    test = ShellBot('Test')
    test.start()

