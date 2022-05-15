from unittest import result
import numpy as np
from typing import List

class GANet(object):
    weights: np.ndarray
    biases: np.ndarray
    
    def __init__(self, weights: np.ndarray, biases: np.ndarray):
        self.weights = weights
        self.biases = biases

    def activate(self, inputs: List[float]) -> List[float]:
        nparray = np.array(inputs)
        resultants = self.forward_propogation(nparray, self.weights, self.biases).clip(min=0, max=1)
        return resultants.flatten().tolist()
    
    def sigmoid_activation(self, x: np.ndarray) -> np.ndarray:
        return (1/(1+np.exp(-x)))

    def forward_propogation(self, inputs: np.ndarray, weights: np.ndarray, biases: np.ndarray) -> np.ndarray:
        activations = self.sigmoid_activation(np.dot(inputs, weights) + biases)
        return activations

if __name__ == '__main__':
    weights = np.random.rand(22, 8) - 0.5 * 2
    biases = np.random.rand(1, 8) - 0.5 * 2
    inputs = np.random.rand(1, 22)
    net = GANet(weights, biases)
    print(net.activate(inputs.astype(float).tolist()))