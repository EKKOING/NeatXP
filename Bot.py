''' 
Sparring Partner Bot For GA Evaluations
Author: Nicholas Lorentzen
Date: 20220501
'''

import faulthandler
import random
import math
from typing import Tuple, Union

import libpyAI as ai

faulthandler.enable(all_threads=True)


class bot(object):
    '''bot AI written for xpilot-ai by Nicholas Lorentzen last updated: 20220501
    '''
    username: str = "EKKO"
    
    max_speed: int = 3
    desired_speed: int = 3
    safety_margin: int = 12
    turnspeed: int = 20
    power_level: float = 28.0
    scan_distance: int = 1000
    cpa_check_distance = 500
    thrust_heading_tolerance: int = 10
    e_thrust_heading_tolerance: int = 15
    wall_threshold: int = 50
    firing_threshold: int = 5
    max_shot_distance: int = 800

    desired_heading: float = 0
    desired_thrust: int = 0
    last_radar_heading: float = 0
    current_frame: int = 0

    def __init__(self, name='ekko') -> None:
        '''__init__ Initializes the bot

        Args:
            name (str, optional): Username for xpilot server. Defaults to 'ekko'.
        '''
        self.username = name
        self.max_turntime = math.ceil(180 / self.turnspeed)

    def run_loop(self, ) -> None:
        '''run_loop Runs on every frame to control the bot
        '''
        if ai.selfAlive() == 0:
            self.current_frame = 0
        ai.setTurnSpeedDeg(self.turnspeed)
        self.current_frame += 1
        self.reset_flags()
        self.set_flags()
        self.production_system()

        if self.current_frame < 5:
            #Gotta get those free spawn kills
            ai.fireShot()
            if self.current_frame < 2:
                ai.thrust(1)

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

    def reset_flags(self,) -> None:
        '''reset_flags Sets all flags to false and resets control states
        '''
        self.turn, self.thrust, self.shoot = False, False, False
        ai.turnLeft(0)
        ai.turnRight(0)
        ai.thrust(0)

    def set_flags(self, ) -> None:
        '''set_flags Progressively examines the bot's environment until a flag is set
        '''
        if self.check_walls():
            ##print(f'{self.username}: Avoiding wall')
            pass
        elif self.check_shots():
            ##print(f'{self.username}: Avoiding bullet')
            pass
        elif self.check_kills():
            ##print(f'{self.username}: Aggressive')
            pass
        elif self.check_ships():
            ##print(f'{self.username}: Avoiding ship')
            pass
        elif self.check_speed():
            ##print(f'{self.username}: Slowing down')
            pass
        elif False and self.check_position():
            ##print(f'{self.username}: Moving to center')
            pass
        elif self.check_radar():
            ##print(f'{self.username}: Enemy Detected On Radar')
            pass
        else:
            self.desired_heading = self.heading + random.randint(0, 20)
            self.turn = True
            self.shoot = False

    def check_walls(self, ) -> bool:
        '''check_walls Checks for possible wall collisions and sets flags accordingly if necessary

        Returns:
            bool: True if wall avoidance is necessary, otherwise False
        '''
        self.x = ai.selfX()
        self.y = ai.selfY()
        self.heading = int(ai.selfHeadingDeg())
        self.tracking = int(ai.selfTrackingDeg())
        self.speed: float = ai.selfSpeed()
        self.x_vel = ai.selfVelX()
        self.y_vel = ai.selfVelY()
        self.heading = int(ai.selfHeadingDeg())
        self.track_wall = ai.wallFeeler(self.scan_distance, self.tracking)
        self.tt_tracking = math.ceil(
            self.track_wall / (self.speed + 0.0000001))
        self.tt_retro = math.ceil(self.speed / self.power_level)

        self.update_closest_wall()

        if self.closest_wall < self.wall_threshold:
            self.turn = True
            self.desired_heading = self.angle_add(
                self.closest_wall_heading, 180)
            if self.check_heading_tolerance(self.e_thrust_heading_tolerance) or self.tt_tracking < self.tt_retro + 1:
                self.thrust = True
            self.check_luck()
            return True

        ##print(f'speed: {self.speed} tth: {self.tt_tracking} dist: {self.track_wall} ttr: {self.tt_retro}')

        if self.tt_tracking < self.max_turntime + self.tt_retro + self.safety_margin:
            self.turn = True
            self.desired_heading = self.angle_add(self.tracking, 180)
            if self.check_heading_tolerance():
                self.thrust = True
            self.check_luck()
            return True

        return False

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

    def check_ships(self, ) -> bool:
        '''check_ships Checks for possible ship collisions using CPA prediction and sets flags accordingly if necessary

        Returns:
            bool: True if ship avoidance is necessary, otherwise False
        '''        
        idx = 0
        next_ship_dist = ai.enemyDistance(idx)
        while next_ship_dist != -1 and next_ship_dist < self.cpa_check_distance:
            enemy_x = ai.screenEnemyX(idx)
            enemy_y = ai.screenEnemyY(idx)
            enemy_speed = ai.enemySpeed(idx)
            enemy_tracking = ai.enemyTrackingDeg(idx)
            enemy_cpa_time, enemy_cpa_distance = self.get_cpa_self(
                enemy_x, enemy_y, enemy_speed, enemy_tracking)
            if enemy_speed < 1:
                break
            ##print(f'{ai.enemyName(idx)} cpa time: {enemy_cpa_time} - dist: {enemy_cpa_distance}')
            if enemy_cpa_distance < 50:
                if enemy_cpa_time < self.max_turntime:
                    self.turn = True
                    left_option = self.angle_add(enemy_tracking, 90)
                    right_option = self.angle_add(enemy_tracking, -90)
                    self.desired_heading = self.get_closer_angle(
                        left_option, right_option)
                    ##print(f'{ai.enemyName(idx)} is close, turning to {self.desired_heading}')
                    self.thrust = True
                    return True
                if enemy_cpa_time < self.tt_retro + self.safety_margin + self.max_turntime:
                    self.turn = True
                    left_option = self.angle_add(enemy_tracking, 90)
                    right_option = self.angle_add(enemy_tracking, -90)
                    self.desired_heading = self.get_closer_angle(
                        left_option, right_option)
                    if self.check_heading_tolerance():
                        self.thrust = True
                    ##(f'{ai.enemyName(idx)} is close, turning to {self.desired_heading}')
                    self.check_luck()
                    return True
            idx += 1
            next_ship_dist = ai.enemyDistance(idx)
        return False

    def check_shots(self, ) -> bool:
        '''check_shots Checks for possible bullet collisions using shot alert values and sets flags accordingly if necessary

        Returns:
            bool: True if bullet avoidance is necessary, otherwise False
        '''        
        idx = 0
        lowest_idx = -1
        lowest_alert = 3000
        next_shot_alert = ai.shotAlert(idx)
        while next_shot_alert != -1:
            if next_shot_alert < lowest_alert:
                lowest_alert = next_shot_alert
            idx += 1
            next_shot_alert = ai.shotAlert(idx)
        ##print(f'lowest alert: {lowest_alert} idx: {idx}')
        if lowest_alert < 50:
            shot_vel_dir = self.tracking
            self.turn = True
            left_option = self.angle_add(shot_vel_dir, 75)
            right_option = self.angle_add(shot_vel_dir, -75)
            self.desired_heading = self.get_closer_angle(
                left_option, right_option)
            if self.check_heading_tolerance(self.e_thrust_heading_tolerance) or lowest_alert < 50:
                self.thrust = True
            ##print(f'Dodging shot, turning to {self.desired_heading}')
            return True
        return False

    def check_speed(self,) -> bool:
        '''check_speed Checks if the ship is going faster than the set maximum speed and sets flags accordingly if necessary

        Returns:
            bool: True if retrograde thrust is necessary, otherwise False
        '''        
        if self.speed > self.max_speed:
            self.desired_heading = self.angle_add(self.tracking, 180)
            self.turn = True
            if self.check_heading_tolerance():
                self.thrust = True
            return True
        return False

    def check_kills(self,) -> bool:
        '''check_kills Check for an enemy on the screen that can be shot and sets flags accordingly if the enemy can be dealt with

        Returns:
            bool: True if the bot is about to kill someone, otherwise False
        '''        
        if ai.aimdir(0) != -1:
            self.turn = True
            self.desired_heading = ai.aimdir(0)
            if self.check_heading_tolerance(5):
                self.shoot = True
                ##print(f'{self.username}: Shooting')
            if self.current_frame % 133 == 0:
                self.thrust = True
            return True
        return False

    def check_radar(self,) -> bool:
        '''check_radar Checks for enemies on the radar and sets flags accordingly if necessary

        Returns:
            bool: True if enemy located on radar, otherwise False
        '''         
        x_diff = ai.closestRadarX()
        if x_diff != -1:
            y_diff = ai.closestRadarY()
            self.x_diff = ai.selfRadarX()
            self.y_diff = ai.selfRadarY()

            x_diff = x_diff - self.x_diff
            y_diff = y_diff - self.y_diff

            radar_angle = math.degrees(
                math.atan(abs(y_diff) / abs(x_diff + 0.0000001)))

            if x_diff < 0 and y_diff > 0:
                radar_angle = 180 - radar_angle
            elif x_diff < 0 and y_diff < 0:
                radar_angle = 180 + radar_angle
            elif x_diff > 0 and y_diff < 0:
                radar_angle = 360 - radar_angle
            #temp = self.last_radar_heading
            #self.last_radar_heading = radar_angle
            #radar_angle += self.angle_diff(temp, radar_angle)
            self.desired_heading = radar_angle
            self.turn = True
            if self.check_heading_tolerance(5) and abs(x_diff) > 8 and abs(y_diff) > 8 and self.current_frame % 48 == 0:
                self.shoot = True
            ##print(f'Radar@ {x_diff},{y_diff} - {radar_angle}')
            return True
        self.last_radar_heading = self.desired_heading
        return False

    def check_position(self,) -> bool:
        '''check_position Brings the ship closer to the center of the screen if it is too far away

        Returns:
            bool: True if the ship is too far away, otherwise False
        '''        
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
            return True
        elif self.speed > 1:
            self.turn = True
            self.desired_heading = self.angle_add(self.tracking, 180)
            if self.check_heading_tolerance(5):
                self.thrust = True
            return True
        return False

    def check_luck(self,) -> bool:
        '''check_luck Hmmm i wonder if there happens to be an enemy all lined up

        Returns:
            bool: True if someone is about to have a very unfortunate time, otherwise False
        '''        
        nearest_aim_dir = ai.aimdir(0)
        if nearest_aim_dir != -1:
            delta = abs(self.angle_diff(self.heading, nearest_aim_dir))
            if delta < 10:
                self.shoot = True
                return True
        return False

    def get_closer_angle(self, a1: float, a2: float) -> float:
        '''get_closer_angle Returns whichever heading is closer to the current heading of the ship

        Args:
            a1 (float): First heading
            a2 (float): Second heading

        Returns:
            float: The heading that is closer to the current heading of the ship
        '''        
        if self.angle_diff(self.heading, a1) < self.angle_diff(self.heading, a2):
            return a1
        return a2

    def get_cpa_self(self, x, y, heading, speed) -> Tuple[float, float]:
        '''get_cpa_self Ever wondered how long it will be until some object either hits or passes the ship? No... Well this function calculates that for you.

        Args:
            x (int): X coordinate of the object
            y (int): Y coordinate of the object
            heading (float): heading of the object
            speed (float): speed of the object

        Returns:
            Tuple[float, float]: The time to CPA in frames and the distance to CPA in pixels
        '''        
        enemy_x_vel, enemy_y_vel = self.get_components(speed, heading)
        found_min = False
        last_dist: float = self.get_distance(self.x, self.y, x, y)
        t: int = 1
        while not found_min:
            current_dist = self.get_distance(
                self.x + self.x_vel * t, self.y + self.y_vel * t, x + enemy_x_vel * t, y + enemy_y_vel * t)
            if current_dist < last_dist:
                t += 1
                last_dist = current_dist
            else:
                found_min = True
        return t, last_dist

    def get_components(self, heading: float, speed: float) -> Tuple[float, float]:
        '''get_components Returns the x and y components of a given heading and speed

        Args:
            heading (float): heading of the object
            speed (float): speed of the object

        Returns:
            Tuple[float, float]: x and y components of the given heading and speed
        '''        
        heading_rad = math.radians(heading)
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

        if x_diff < 0 and y_diff > 0:
            angle_to = 180 - angle_to
        elif x_diff < 0 and y_diff < 0:
            angle_to = 180 + angle_to
        elif x_diff > 0 and y_diff < 0:
            angle_to = 360 - angle_to
        return angle_to

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


if __name__ == "__main__":
    test = bot("EKKO")
    ai.headlessMode()
    ai.start(test.run_loop, ["-name", test.username, "-join", "localhost"])
    print("Bot stopped")
