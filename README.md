# NeatXP
**Author:** [@EKKOING](https://github.com/EKKOING)
**Last Update:** 20220428

A look into applying the NEAT algorithm to the space combat simulator XPilot.

## Project Proposal
I will attempt to compare and contrast the use of GAs and smart GAs (NEAT) to evolve an ANN to control an XPilot agent. The former will be heavily based upon the work completed for Program #4, though new training will need to take place due to the error that occurred in the training for the competition. From my earlier findings, training will likely take close to 800 generations to reach near maturity for this pure GA, thus evaluation of NEAT’s performance will be done over the same generation period. If it is determined that the project would benefit from additional training time, and/or additional trials of the same length, they will be performed. The NEAT package that will be used in this project can be found here. Metrics will be logged across each generation as well as copies of the best performing agent at each generation for later analysis.  

## About Xpilot
![XPilot Screenshot](https://upload.wikimedia.org/wikipedia/commons/thumb/a/a7/Xpilotscreen.jpg/1024px-Xpilotscreen.jpg)  
*A Screenshot of an XPilot Game*  
>XPilot is an open-source 2-dimensional space combat simulator which is playable over the internet. Multiple players can connect to a central XPilot server and compete in many varieties of game play, such as free-for-all combat, capture-the-flag, or team combat. Each player controls a space-ship that can turn, thrust, and shoot. There is often a variety of weapons and ship upgrades available on the particular map in which they play. The game uses synchronized client/server networking to allow for solid network play.  

\- [The XPilot-AI Homepage](https://xpilot-ai.com)  
More information on the history of the game's development can be found [here](https://en.wikipedia.org/wiki/XPilot).  

## About Genetic Algorithms
TBC...

## About NEAT
TBC...

## Experimental Design
For this study the GA and NEAT will be tasked with solving the same task, controlling the Shellbot XPilot Agent in a 1 on 1 battle against a hand coded opponent written by me. Both algorithms will share the same inputs, actions, and fitness functions to try and keep the environments as similar as possible. Additionally, both algorithms will have the same population size and limits on the weights of each node. 

The inputs provided to the respective networks are as follows:

| **Input**            | **Description**                                                                       |
|:--------------------:|:-------------------------------------------------------------------------------------:|
| Speed                | The speed of the agent                                                                |
| Track Wall           | The distance to the tracking angle wall                                               |
| Closest Wall         | The distance to the closest wall                                                      |
| Front Wall           | The distance to the front wall                                                        |
| Left Wall            | The distance to the left wall                                                         |
| Right Wall           | The distance to the right wall                                                        |
| Wall 30 R            | The distance to the wall 30 degrees right of the agent                                |
| Wall 30 L            | The distance to the wall 30 degrees left of the agent                                 |
| Enemy Distance       | The distance to the enemy                                                             |
| Enemy Speed          | The speed of the enemy                                                                |
| Enemy Presence       | Whether or not the enemy is present on screen (1 if present, 0 if not)                |
| Delta Angle to Enemy | The angle between the current heading of the agent and the enemy                      |
| Enemy CPA dist       | The distance at the closest point of approach between the agent and the enemy         |
| Can Shoot            | 1 if the agent has a valid aim direction to the enemy, 0 if not                       |
| Delta Angle to Shoot | The angle between the current heading of the agent and the aim direction to the enemy |
| Shot Present         | 1 if there is a shot on screen, 0 if not                                              |
| Shot Distance        | The distance to the shot                                                              |
| Angle to the Shot    | The angle between the current heading of the agent and the shot                       |
| Shot Alert           | Uses the XPilot AI shot alert value                                                   |
| TT Track             | Frames until collision with the tracking angle wall                                   |
| TT Retro             | Frames until it is too late to retrograde                                             |
| Last Action          | The last action taken by the agent                                                    |

The actions available to the agent are as follows:

| **Action**  | **Description**                                                                          |
|:-----------:|:----------------------------------------------------------------------------------------:|
| Avoid Walls | Retrogrades away from the tracking angle or faces away from the nearest wall and thrusts |
| Dodge Shot  | Attempts to dodge the shot, does nothing otherwise                                       |
| Slow Down   | Retrogrades to slow down the agent                                                       |
| Kill Enemy  | Attempts to kill the enemy, does nothing otherwise                                       |
| Center      | Centers the agent on the map as far away as possible from walls                          |
| Thrust      | Thrusts the agent forward                                                                |
| Turn Left   | Turns the agent left 15 degrees                                                          |
| Turn Right  | Turns the agent right 15 degrees                                                         |

## Results
TBD

## Future Work
TBD

## Credits
[Neat-Python](https://github.com/CodeReclaimers/neat-python)
[XPilot-AI](http://www.xpilot-ai.org/)
