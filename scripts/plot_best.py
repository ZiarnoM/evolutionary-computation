import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap, Normalize
from mpl_toolkits.axes_grid1 import make_axes_locatable

from common import ROOT, assignment_arg, label, load_best, load_nodes, load_summary

# single blue hue, light = cheap, dark = expensive
CMAP = LinearSegmentedColormap.from_list(
    "cost", ["#86b6ef", "#3987e5", "#1c5cab", "#0d366b"])


def plot(a, instance, method, obj, start, out_dir):
    nodes = load_nodes(instance)
    sol = load_best(a, instance, method)
    norm = Normalize(nodes.cost.min(), nodes.cost.max())
    sel = nodes.iloc[sol]
    # same size scale for all nodes, so skipped expensive nodes are visible too
    size = 10 + 90 * norm(nodes.cost)

    fig, ax = plt.subplots(figsize=(9, 5.6))
    ax.scatter(nodes.x, nodes.y, s=size, color="#d6d6d6", zorder=1, label="not selected")
    cyc = sol + sol[:1]
    ax.plot(nodes.x.iloc[cyc], nodes.y.iloc[cyc], color="#555555", lw=1, zorder=2)
    pts = ax.scatter(sel.x, sel.y, s=size[sol], c=sel.cost, cmap=CMAP, norm=norm,
                     edgecolors="white", linewidths=0.6, zorder=3, label="selected")
    # for random the start index is only the seed
    if method != "random":
        ax.scatter(nodes.x[start], nodes.y[start], s=160, facecolors="none",
                   edgecolors="#c0392b", linewidths=1.5, zorder=4, label=f"start ({start})")

    ax.set_aspect("equal")
    cax = make_axes_locatable(ax).append_axes("right", size="3%", pad=0.15)
    fig.colorbar(pts, cax=cax, label="node cost")
    ax.set_title(f"{label(method)}, {instance}, objective = {obj}")
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    leg = ax.legend(loc="upper center", bbox_to_anchor=(0.5, -0.13), ncol=3, fontsize=8,
                    frameon=False)
    for h in leg.legend_handles[:2]:
        h.set_sizes([40])
    leg.legend_handles[1].set_facecolor("#3987e5")
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    fig.tight_layout()
    path = out_dir / f"{instance}_{method}.png"
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    return path


def main():
    a = assignment_arg()
    out_dir = ROOT / "reports" / f"assignment{a}"
    out_dir.mkdir(parents=True, exist_ok=True)
    for row in load_summary(a).itertuples():
        print(plot(a, row.instance, row.method, row.min, row.best_start, out_dir).relative_to(ROOT))


if __name__ == "__main__":
    main()
