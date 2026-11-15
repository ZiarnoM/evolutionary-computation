
import matplotlib
matplotlib.use("Agg")

import matplotlib.pyplot as plt
import pandas as pd

from common import ROOT, label

COLORS = {
    "steepest_edges_random_candidate": "#2a78d6",
}

DEFAULT_COLOR = "#777777"


def main():
    df = pd.read_csv(
        ROOT / "results" / "assignment4" / "candidates.csv"
    )

    instances = list(df["instance"].unique())

    fig, axes = plt.subplots(
        len(instances),
        2,
        figsize=(11, 4 * len(instances)),
        squeeze=False,
    )

    for row, instance in enumerate(instances):
        part = df[df["instance"] == instance]

        ax_obj = axes[row, 0]
        ax_time = axes[row, 1]

        for base, g in part.groupby("base", sort=False):
            g = g.sort_values("candidate_count")

            color = COLORS.get(base, DEFAULT_COLOR)
            name = label(base)

            # Objective value
            ax_obj.fill_between(
                g["candidate_count"],
                g["min"],
                g["max"],
                color=color,
                alpha=0.12,
                linewidth=0,
            )
            ax_obj.plot(
                g["candidate_count"],
                g["avg"],
                color=color,
                linewidth=2,
                marker="o",
                markersize=4,
                label=name,
            )

            # Runtime
            ax_time.plot(
                g["candidate_count"],
                g["avg_ms"],
                color=color,
                linewidth=2,
                marker="o",
                markersize=4,
                label=name,
            )

        ax_obj.set_title(f"{instance} — solution quality")
        ax_obj.set_xlabel("candidate moves size")
        ax_obj.set_ylabel("objective")
        ax_obj.set_xticks(sorted(part["candidate_count"].unique()))
        ax_obj.grid(axis="y", color="#e6e6e6", linewidth=0.8)
        ax_obj.set_axisbelow(True)

        ax_time.set_title(f"{instance} — average runtime")
        ax_time.set_xlabel("candidate moves size")
        ax_time.set_ylabel("runtime (ms)")
        ax_time.set_xticks(sorted(part["candidate_count"].unique()))
        ax_time.grid(axis="y", color="#e6e6e6", linewidth=0.8)
        ax_time.set_axisbelow(True)

        for ax in (ax_obj, ax_time):
            for spine in ("top", "right"):
                ax.spines[spine].set_visible(False)

        ax_obj.legend(frameon=False, fontsize=8, loc="best")
        ax_time.legend(frameon=False, fontsize=8, loc="best")

    fig.tight_layout()

    path = ROOT / "reports" / "assignment4" / "candidates.png"
    path.parent.mkdir(parents=True, exist_ok=True)

    fig.savefig(path, dpi=150, bbox_inches="tight")
    print(path.relative_to(ROOT))


if __name__ == "__main__":
    main()
