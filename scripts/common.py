import sys
from pathlib import Path

import pandas as pd

ROOT = Path(__file__).resolve().parent.parent

LABELS = {
    "random": "Random solution",
    "nn_end": "Nearest neighbor (end only)",
    "nn_any": "Nearest neighbor (any position)",
    "greedy_cycle": "Greedy cycle",
    "nn_any_regret": "NN (any position), 2-regret",
    "nn_any_weighted": "NN (any position), weighted",
    "greedy_cycle_regret": "Greedy cycle, 2-regret",
    "greedy_cycle_weighted": "Greedy cycle, weighted",
}


def assignment_arg():
    return int(sys.argv[1]) if len(sys.argv) > 1 else 1


def label(method):
    return LABELS.get(method, method)


def load_nodes(instance):
    return pd.read_csv(ROOT / "data" / f"{instance}.csv", sep=";", header=None,
                       names=["x", "y", "cost"])


def load_summary(a):
    return pd.read_csv(ROOT / "results" / f"assignment{a}" / "summary.csv")


def load_best(a, instance, method):
    path = ROOT / "results" / f"assignment{a}" / "best" / f"{instance}_{method}.txt"
    return [int(v) for v in path.read_text().split()]
