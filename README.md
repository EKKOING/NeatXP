# NeatXP
**Author:** [@EKKOING](https://github.com/EKKOING)  
**Last Update:** 20220518

Final project for [COM407](https://oak.conncoll.edu/parker/com407/projects.html) at Connecticut College as taught by [Prof. Gary Parker](https://oak.conncoll.edu/parker).

A look into applying the NEAT algorithm to the space combat simulator XPilot.  

## Table of Contents
  - [Project Proposal](#project-proposal)
  - [About Xpilot](#about-xpilot)
  - [About Genetic Algorithms](#about-genetic-algorithms)
  - [About NEAT](#about-neat)
  - [Experimental Design](#experimental-design)
    - [Observations](#observations)
    - [Action Space](#action-space)
    - [The Fitness Function](#the-fitness-function)
  - [Results](#results)
    - [Fitness Plots](#fitness-plots)
    - [Score Plots](#score-plots)
    - [Bonus Plots](#bonus-plots)
    - [Animated Joint Plot](#animated-joint-plot)
    - [Summary](#summary)
  - [Future Work](#future-work)
  - [Credits](#credits)


## Project Proposal
I will attempt to compare and contrast the use of GAs and smart GAs (NEAT) to evolve an ANN to control an XPilot agent. The former will be heavily based upon the work completed for Program #4, though new training will need to take place due to the error that occurred in the training for the competition. From my earlier findings, training will likely take close to 800 generations to reach near maturity for this pure GA, thus evaluation of NEAT’s performance will be done over the same generation period. If it is determined that the project would benefit from additional training time, and/or additional trials of the same length, they will be performed. The NEAT package that will be used in this project can be found here. Metrics will be logged across each generation as well as copies of the best performing agent at each generation for later analysis.  

## About Xpilot
![XPilot Screenshot](https://upload.wikimedia.org/wikipedia/commons/thumb/a/a7/Xpilotscreen.jpg/1024px-Xpilotscreen.jpg)  
*A Screenshot of an XPilot Game*  
>XPilot is an open-source 2-dimensional space combat simulator which is playable over the internet. Multiple players can connect to a central XPilot server and compete in many varieties of game play, such as free-for-all combat, capture-the-flag, or team combat. Each player controls a space-ship that can turn, thrust, and shoot. There is often a variety of weapons and ship upgrades available on the particular map in which they play. The game uses synchronized client/server networking to allow for solid network play.  

\- [The XPilot-AI Homepage](https://xpilot-ai.com)  
More information on the history of the game's development can be found [here](https://en.wikipedia.org/wiki/XPilot).  

## About Genetic Algorithms
Genetic Algorithms are a form of computational intelligence that through mimicing the natural world's evolution can perform complex optimizations. This optimization is not performed through error correction, but rather through semi-random adjustments. 
The basic structure of a GA is as follows:
Create a population of individuals.
Evaluate the fitness of each individual in the population.
Select a subset of the population to undergo genetic operations.
Perform genetic operations on the selected subset.
Repeat steps 2 and 3 until a satisfactory solution is found.

## About NEAT
NEAT is a genetic algorithm that is specifically designed to solve the problem of evolving neural networks. It is a form of artificial intelligence that is used to solve problems that are difficult to solve using conventional genetic algorithms or backpropagation.
NEAT modifies the general GA structure by introducing a number of innovations to the GA. These innovations include:
 - The ability to modify the structure of the NN as well as the weights and biases of the NN.
 - Adding speciation, which allows individuals to only compete amongst those similar to them until the strategy can develop.

The former is the greatest strength of NEAT as it allows for the network to increase in complexity to meet whatever challenge is presented to the algorithm. The latter is a great help to ensuring that the algorithm does not suffer from the drawbacks of a standard GA being applied to a NN.

## Experimental Design
For this study the GA and NEAT will be tasked with solving the same task, controlling the Shellbot XPilot Agent in a 1 on 1 battle against a hand coded opponent written by me. Both algorithms will share the same inputs, actions, and fitness functions to try and keep the environments as similar as possible. Additionally, both algorithms will have the same population size and limits on the weights of each node. 

### Observations
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

### Action Space
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

### The Fitness Function
The fitness function of the agents is made up of two sub functions:

The first function (Score) is the score of the agent at the end of the evaluation period.  
The second function (Bonus) is assigned as follows:
 - 0.01 points for each frame the agent is alive
 - 0.03 points for each frame the agent is moving faster than 0.5 units

The final fitness of the agent is the sum of the two sub functions.

## Results
### Fitness Plots
![Fitness Plots](https://github.com/EKKOING/NeatXP/blob/main/graphs/fitness.png?raw=true)
### Score Plots
![Score Plots](https://github.com/EKKOING/NeatXP/blob/main/graphs/score.png?raw=true)
### Bonus Plots
![Bonus Plots](https://github.com/EKKOING/NeatXP/blob/main/graphs/bonus.png?raw=true)

### Animated Joint Plot
![Joint Plot](https://github.com/EKKOING/NeatXP/blob/main/graphs/animjointplot_inf.gif?raw=true)

### GA Net Visualization
![GA Net Animation](https://github.com/EKKOING/NeatXP/blob/main/graphs/ga_net.gif?raw=true)

### Summary
Both algorithms proved to be successful in improving performance over time. The fitness plots show that the general trend of the median and mean was upwards for the GA and NEAT, though in the case of NEAT it seemed to make the majority of its progress in the first 100 generations and then stagnate (more on this later). On the other hand, the GA showed an almost linear trend in the fitnesses of all 4 metrics from start to finish.
 - **NEAT**:
Some of the more interesting charecteristics of the NEAT learning graphs are the almost instantaneous jump in the bonus function and inverse in the score function between generation 70 and 85. This represents the emergence of a new strategy that the algorithm quickly determines to be dominant. Unfortunately, this was the only evidence of any benefit speciation brought to NEAT as there was a misconfiguration to the config file that caused the threshold for speciation to be far too high in comparison to the learning rates hence little to no speciation occurred after the extinctions of the original species. Additionally the algorithm likely would have benefitted from a far more agressive learning rate as NEAT benefits from greater diversity far more than the GA.
 - **GA**:
The GA graphs are about as promising as one could ever want. Both subfunctions of the fitness function are increasing in value over time almost linearly. While the GA technically finished the 250 generations behind in score to NEAT, this could partially be attributed to a lesser starting population than its counterpart as when compared by difference from start to finish, the GA made very similar progress. When only comparing the score metric, the GA was even able to compete on par with NEAT despite this handicap. The animated jointplot shows how much more consistent the fitnesses were within generations than NEAT. 

While both algorithms were able to show substantial progress from start to finish, neither was capable of beating the hand coded opponent consistently enough to have a positive score. That said it was also very clear that both algorithms would benefit from far more generations to truly test the limits of their ability to improve. This would likely prove NEAT's ability to outperform its standard GA counterpart as the network continued to evolve in complexity.

## Future Work
To improve upon the results of this study the most essential item would simply be more time for trials, as well as repeated trials with tweaks to hyperparameters. This would allow for NEAT's thresholds to be better adjusted to the given scenario as the GA's had been done by a previous project. It would also be interesting to see what effect the more complex map had on the development rate of the algorithms as this is likely why neither algorithm could beat the hand coded opponent. The bonus function could also use some tweaking to reward even more adventerous strategies to increase the rate of improvement.

## Credits
[Neat-Python](https://github.com/CodeReclaimers/neat-python): NEAT library used for learning.  

[XPilot-AI](http://www.xpilot-ai.org/): The interface for the Xpilot game for python (and many other languages).  
