---
title: |
  Evolutionary Computation\
  Assignment 5: Move evaluations from previous iterations in local search
author: |
  Michał Żarnowski 160277\
  Piotr Szymiec 160322
geometry: margin=2.3cm
fontsize: 11pt
---

# Problem description

We are given a set of nodes, each with x and y coordinates and a cost. The task is to select exactly half of the nodes (rounded up if the number of nodes is odd) and to form a Hamiltonian cycle through the selected nodes. The goal is to minimize the length of the cycle plus the total cost of the selected nodes. Distances are Euclidean, rounded mathematically to integers, computed in the same way as in the solution checker: `(int)(sqrt(dx*dx + dy*dy) + 0.5)`.

Both instances, TSPA and TSPB, have 200 nodes, so every solution contains 100 nodes. The distance matrix is computed right after reading an instance and the coordinates are then discarded, so the optimization methods only see the distance matrix and the node costs.

# Implemented methods

In Assignment 3 the best neighborhood was the exchange of two edges (2-opt) as the intra-route move together with the exchange of a selected and an unselected node as the inter-route move. In this assignment the steepest local search with this neighborhood, started from random solutions, is sped up with a list of improving moves (LM). The baseline is the steepest local search from Assignment 3 with the same neighborhood and the same random starting solutions, which evaluates the whole neighborhood in every iteration. The candidate moves from Assignment 4 are not used together with the list.

In the following, `d` is the distance matrix and `c` is the vector of costs. For a 2-opt move that removes the edges `a->b` and `c->d` and adds `a-c` and `b-d` the change of the objective is `d[a][c] + d[b][d] - d[a][b] - d[c][d]`. For replacing a selected node `v` with neighbors `p` and `s` by an unselected node `u` it is `d[p][u] + d[u][s] - d[p][v] - d[v][s] + c[u] - c[v]`.

## Representation of moves

A move in the list is stored by the nodes it touches, not by their positions, because positions change after every applied move. A 2-opt move is stored as four nodes `(a, b, c, d)` and means "remove edges `a-b` and `c-d`, add edges `a-c` and `b-d`". The same move can be written as `(c, d, a, b)`, `(b, a, d, c)` or `(d, c, b, a)`, and only the smallest of these four tuples is kept, so a move is not added to the list twice. A node exchange is stored as `(p, v, s, u)`: the removed node `v`, its two neighbors and the inserted node `u`.

The list is a `std::set` ordered by delta, so the best move is always first. A move that is already in the list is not inserted again.

## Checking a move from the list

When a 2-opt move `(a, b, c, d)` is taken from the list, there are three cases:

1. At least one of the edges `a-b`, `c-d` does not exist in the current cycle (in either direction). The move is removed from the list.
2. Both edges exist, but in different relative directions, for example `a->b` and `d->c`. Removing them and adding `a-c` and `b-d` would split the cycle into two, so the move cannot be applied now. It stays in the list, because it can become applicable after some later reversal of a segment.
3. Both edges exist in the same relative direction, `a->b` and `c->d` or both reversed, `b->a` and `d->c`. The move is applied and removed from the list. In the second case the segment between the edges is the same, only seen from the other side, so the new edges and the delta are the same.

For a node exchange the direction does not matter. The move is applicable if `u` is still unselected and the edges `p-v` and `v-s` still exist, otherwise it is removed. A move that became invalid because `u` was selected is not lost: when `u` is removed from the cycle again, all moves inserting `u` are evaluated anew.

## Evaluating new moves

For every pair of edges `a->b` and `c->d` of the current cycle two moves are evaluated: `(a, b, c, d)`, which can be applied now, and `(a, b, d, c)` with the second edge inverted, which removes the same edges but adds `a-d` and `b-c`. The second one is applicable only after the direction of one of the edges changes, but it has to be evaluated when the edges appear, because later the pair will not be looked at again. Both are added to the list if their delta is negative. This is done for the initial solution and for every new edge.

After a 2-opt move, the two new edges are evaluated against all edges of the cycle, in both relative directions. The edges inside the reversed segment are the same, only their direction changed, and the moves with both directions were already in the list. Node exchanges are evaluated again for the four nodes whose neighbors changed.

After a node exchange that replaced `v` by `u` between `p` and `s`, the new edges `p-u` and `u-s` are evaluated against all edges, the exchanges of `p`, `u` and `s` with every unselected node are evaluated, and so are the exchanges of every selected node with `v`, which is now unselected.

## Pseudocode

```
x = random solution
LM = empty set of moves ordered by delta
add all improving moves of x to LM, 2-opt moves in both relative directions
repeat:
    m = none
    for move in LM from the best:
        if the removed edges of move do not exist (or the inserted node is selected):
            remove move from LM
        else if the removed edges have different relative directions:
            skip move, keep it in LM
        else:
            m = move, remove m from LM, stop browsing
    if m is not none:
        apply m to x, objective += delta(m)
        add improving moves using the new edges or the changed nodes to LM
    else:
        add all improving moves of x to LM
        if no move from LM is applicable: stop
return x
```

The last full evaluation follows the alternative algorithm from the assignment. It is only a safety check, because every new edge and every changed node is already evaluated after a move. In our runs it never found a new improving move: it ran once at the end of every run and the search stopped right after it.

## Implementation notes

The cycle is kept in an array together with the position of every node (-1 for unselected nodes), so the predecessor and successor of a node and the check whether an edge exists take constant time. A 2-opt move reverses the shorter of the two parts of the cycle between the removed edges.

To check that the list gives the same search as the baseline, we ran a separate test that, before every move taken from the list, computed the best move in the whole neighborhood by brute force. In all 400 runs (53461 moves, about 134 per run) the delta of the applied move was equal to the best delta in the neighborhood, so the method is still a steepest local search and only the way of finding the best move changed.

All methods have the same interface: they take the instance and the starting node and return the solution together with the objective value, which they update incrementally from the deltas. After every run the solution is validated: it must contain exactly `k` nodes, all indices must be in range, there can be no duplicates, and the objective recomputed from scratch must be equal to the value returned by the method. All solutions passed these checks. The program was compiled with Apple clang 17 with `-O2` and run on an Apple M2 Pro. All methods in the tables were run again on this machine, so the times can be compared with each other but not with the times in the earlier reports.

# Results

Each method was run 200 times on each instance. The local search methods start from the same 200 random solutions (the random generator is seeded with the run number), the greedy methods start from every node. The first four rows are the construction methods from Assignment 1, and the candidate moves from Assignment 4 are added for comparison.

Table: Objective value, average (min - max) over 200 runs. The best average and the best solution on each instance are in bold.

| Method | TSPA | TSPB |
|------------------------|---------------:|---------------:|
| Random solution | 261657.6 (233510 - 292900) | 206274.9 (184873 - 230807) |
| Nearest neighbor (end only) | 87476.6 (84362 - 90075) | 59397.3 (56007 - 63166) |
| Nearest neighbor (any position) | 74347.0 (**72328** - 77022) | 59475.5 (52915 - 66164) |
| Greedy cycle | **73793.4** (72590 - 75281) | 59649.9 (55655 - 63734) |
| Steepest edges, random start | 76192.8 (73859 - 80012) | 52653.3 (50429 - 55421) |
| Steepest edges, 10 candidates | 80252.6 (76034 - 87470) | 52719.2 (**50204** - 55618) |
| Steepest edges, list of moves | 76173.3 (73859 - 80012) | **52626.3** (50429 - 55421) |

Table: Time of one run in milliseconds, average (min - max).

| Method | TSPA | TSPB |
|------------------------|---------------:|---------------:|
| Random solution | 0.004 | 0.004 |
| Nearest neighbor (end only) | 0.031 | 0.019 |
| Nearest neighbor (any position) | 0.727 | 0.683 |
| Greedy cycle | 0.613 | 0.651 |
| Steepest edges, random start | 14.94 (13.37 - 19.03) | 14.92 (13.16 - 17.09) |
| Steepest edges, 10 candidates | 10.83 (9.53 - 12.68) | 13.68 (11.99 - 16.29) |
| Steepest edges, list of moves | **2.73** (2.13 - 6.79) | **2.53** (2.18 - 3.01) |

The list of moves and the baseline give the same objective from the same starting solution in 179 of 200 runs on TSPA and in 166 of 200 runs on TSPB. In the other runs there were several moves with the same best delta at some point, the baseline takes the first one in the order of its loops and the list takes the first one in the order of the set, so the searches go different ways after that. On average this difference is in favour of the list, by 19.4 on TSPA and 27.1 on TSPB, but it changes sign between runs (from -1154 to +817 on TSPA and from -2409 to +724 on TSPB), so it is a matter of chance. The minimum and maximum are the same for both methods.

# Visualization of the best solutions

The plots show the best solutions of the baseline and of the list of moves. The size of every node shows its cost, so larger points are more expensive. Grey points are nodes that were not selected, and selected nodes are also coloured by cost (darker means more expensive). The size and colour scales are the same for all methods on one instance. For random starting solutions the red circle marks the number of the run that gave the best solution, which is only the seed of the generator. The best runs of both methods started from the same solution and ended in the same cycle, only written from a different first node.

![](TSPA_steepest_edges_random.png){width=95%}

![](TSPA_steepest_edges_lm.png){width=95%}

![](TSPB_steepest_edges_random.png){width=95%}

![](TSPB_steepest_edges_lm.png){width=95%}

# Best solutions

Node indices start from 0. The cycle closes from the last node back to the first one.

Steepest edges, list of moves, TSPA, objective 73859:

157, 91, 38, 80, 48, 31, 60, 108, 152, 49, 11, 63, 149, 116, 119, 168, 127, 97, 160, 83, 123, 90, 132, 76, 180, 28, 141, 187, 30, 161, 171, 2, 189, 87, 45, 73, 135, 114, 181, 192, 158, 58, 84, 134, 72, 176, 174, 150, 106, 113, 89, 42, 151, 57, 107, 153, 193, 78, 170, 146, 159, 130, 93, 50, 3, 1, 47, 147, 101, 55, 29, 178, 184, 136, 186, 164, 142, 8, 54, 9, 33, 143, 177, 36, 131, 99, 41, 140, 53, 179, 52, 144, 67, 185, 35, 16, 51, 139, 112, 71

Steepest edges, list of moves, TSPB, objective 50429:

38, 21, 183, 30, 130, 121, 198, 96, 120, 133, 136, 172, 69, 160, 8, 117, 72, 46, 86, 89, 57, 104, 190, 70, 81, 94, 105, 196, 114, 10, 90, 129, 127, 80, 182, 153, 118, 68, 110, 93, 2, 14, 20, 74, 171, 39, 99, 88, 179, 87, 82, 51, 85, 59, 73, 167, 181, 34, 24, 67, 192, 54, 162, 76, 135, 27, 3, 184, 151, 64, 33, 40, 141, 155, 71, 60, 45, 134, 175, 36, 197, 132, 29, 6, 180, 115, 170, 125, 177, 178, 65, 145, 28, 149, 158, 26, 97, 154, 140, 47

# Solution checker

Every solution was validated in the program as described above. The best solutions of all methods in the table on both instances were also pasted into column F of a copy of the solution checker (by a script, `scripts/checker.py`, which opens the copy in Numbers and reads the total from cell K2). For all 14 solutions the checker gives the same objective values as in the table.

# Source code

<https://github.com/ZiarnoM/evolutionary-computation/tree/assignment-5>

The new method is in `src/methods/move_list.cpp`. The baseline is in `src/methods/local_search.cpp` and the candidate moves are in `src/methods/candidate_moves_local_search.cpp`. The common code is in `src/core` and the experiment is in `src/main.cpp`. Plots and tables are made by the Python scripts in `scripts`.

# Conclusions

The list of improving moves made the steepest local search about 5.5 times faster on TSPA (14.94 ms to 2.73 ms per run) and 5.9 times faster on TSPB (14.92 ms to 2.53 ms), without changing the quality of the solutions. The brute force test showed that every applied move is the best one in the whole neighborhood, so the method is the same steepest search, and the averages differ only by 19 and 27 because of different tie-breaking. It is also four to five times faster than the candidate moves from Assignment 4, which on TSPA lose about 4000 on the average objective.

The speedup comes from the fact that after a move only a small part of the neighborhood changes. The baseline evaluates about 4850 2-opt moves and 10000 node exchanges in every one of about 134 iterations, while with the list after a 2-opt move only two new edges are compared with all edges and four nodes with all unselected nodes, which is a few hundred deltas. The cost is browsing the list, where moves that are no longer valid are removed only when they are reached and moves with edges in a different relative direction are skipped every time.

Storing the moves with the inverted edge is needed for this to work. After a 2-opt move the edges inside the reversed segment are not evaluated again, so a move that uses one of them with the opposite direction can only be found if it was put in the list earlier. Since the final full evaluation never found any new move, the list was complete in all runs.
