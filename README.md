# Evolutionary Computation

Code for the Evolutionary Computation lab assignments. All assignments use the same
problem (select half of the nodes and build a cycle minimizing length plus node costs),
so the code is shared: `src/core` has the instance, distance matrix and objective,
`src/methods` has one file per method.

## Build

Needs CMake and a C++17 compiler.

```
cmake --preset release
cmake --build --preset release
./build/tests
```

The preset builds into `build/` with `-O2`. `compile_commands.json` is generated there.

## Run

```
./build/run --assignment 1
```

Without arguments it runs assignment 1. Data is read from `data/` and results go to
`results/assignmentN/`, independent of the working directory.

## Plots and tables

Python is only used for plots and tables:

```
python3 -m venv venv
venv/bin/pip install -r requirements.txt
venv/bin/python scripts/plot_best.py 1
venv/bin/python scripts/tables.py 1
```

The argument is the assignment number (default 1). Both scripts read
`results/assignmentN/` and write to `reports/assignmentN/`.

## Assignment 1: greedy heuristics

Methods: random solution, nearest neighbor adding only at the end of the path, nearest
neighbor adding at any position, greedy cycle. Each greedy method is started from every
node (200 runs), plus 200 random solutions.

Output in `results/assignment1/`:

- `summary.csv` - min, max, avg objective, start node of the best solution and time
  for each instance and method
- `runs.csv` - objective of every run
- `best/<instance>_<method>.txt` - best solution, one node index per line (can be
  pasted into column F of the solution checker)

`reports/assignment1/` has a plot of the best solution for each instance and method
and `tables.md` with the result tables.
