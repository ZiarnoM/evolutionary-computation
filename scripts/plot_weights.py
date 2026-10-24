import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

from common import ROOT, label

COLORS = {"nn_any": "#2a78d6", "greedy_cycle": "#eb6834"}


def main():
    df = pd.read_csv(ROOT / "results" / "assignment2" / "weights.csv")
    fig, axes = plt.subplots(1, 2, figsize=(10, 4))
    for ax, (instance, part) in zip(axes, df.groupby("instance")):
        for base, g in part.groupby("base", sort=False):
            c = COLORS[base]
            ax.fill_between(g.w, g["min"], g["max"], color=c, alpha=0.12, lw=0)
            ax.plot(g.w, g.avg, color=c, lw=2, marker="o", ms=4, label=label(base))
        ax.axvline(0.5, color="#999999", lw=1, ls="--", zorder=0)
        ax.set_title(instance)
        ax.set_xlabel("weight of regret w")
        ax.set_xticks([i / 10 for i in range(11)])
        ax.grid(axis="y", color="#e6e6e6", lw=0.8)
        ax.set_axisbelow(True)
        for s in ("top", "right"):
            ax.spines[s].set_visible(False)
    axes[0].set_ylabel("objective")
    axes[0].legend(frameon=False, fontsize=8, loc="upper left")
    fig.tight_layout()
    path = ROOT / "reports" / "assignment2" / "weights.png"
    fig.savefig(path, dpi=150, bbox_inches="tight")
    print(path.relative_to(ROOT))


if __name__ == "__main__":
    main()
