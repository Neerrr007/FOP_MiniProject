# Path-Finding-Visualizer
A C-based pathfinding visualizer using SDL2

![Path Finding Visualizer](https://user-images.githubusercontent.com/60056206/184668376-b0b07c64-645d-42b7-9a2b-1297f8829dd7.png)


### Path Finding Using the A* Algorithm
The A* algorithm in the grid below begins at the start point (green node) and considers all adjacent nodes. After populating the list of neighboring nodes, it filters out those that are inaccessible (walls obstacles, out of bounds). The node with the lowest cost is then chosen. This process is repeated until the shortest Manhattan distance to the end point (red node) is found.

![Path Finding](https://user-images.githubusercontent.com/60056206/184073773-11c20884-62fc-4cad-a04c-d486e3bb8b04.gif)

### Maze Generation Using the Depth-first Search Algorithm
A randomized variation of the depth-first search algorithm is used to generate a random maze. The grid's nodes are first all initialized as wall nodes. The algorithm begins with a random node and selects a random nearby node that has not yet been visited. The algorithm then removes the wall node between the two nodes, adds the new node to the stack, and marks it as visited. This procedure is repeated by the algorithm until it doesn't find a node with no unvisited neighbors, at which point it backtracks through the path until it finds a new node with an unvisited neighbor. This operation is repeated until all nodes have been visited, at which point the algorithm returns to the initial random node it chose.

![Maze Generator](https://user-images.githubusercontent.com/60056206/184073771-1509b47a-7cc0-4ae4-b839-0318de01edd0.gif)