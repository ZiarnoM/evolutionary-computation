import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap, Normalize
from matplotlib.lines import Line2D
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
    ax.scatter(nodes.x, nodes.y, s=size, color="#d6d6d6", zorder=1)
    # NN methods build a path and close it at the end, so that edge is drawn dashed
    nn = method.startswith("nn")
    edges = sol if nn else sol + sol[:1]
    ax.plot(nodes.x.iloc[edges], nodes.y.iloc[edges], color="#555555", lw=1, zorder=2)
    if nn:
        close = [sol[-1], sol[0]]
        ax.plot(nodes.x.iloc[close], nodes.y.iloc[close], color="#555555", lw=1.2, ls="--",
                zorder=2)
    pts = ax.scatter(sel.x, sel.y, s=size[sol], c=sel.cost, cmap=CMAP, norm=norm,
                     edgecolors="white", linewidths=0.6, zorder=3)
    # legend markers are drawn separately, the scatter ones take size and color from the data
    handles = [
        Line2D([], [], ls="", marker="o", ms=7, color="#d6d6d6", label="not selected"),
        Line2D([], [], ls="", marker="o", ms=7, color="#3987e5", mec="white",
               label="selected"),
    ]
    if nn:
        handles.append(Line2D([], [], color="#555555", lw=1.2, ls="--", label="closing edge"))
    # for random the start index is only the seed
    if method != "random":
        ax.scatter(nodes.x[start], nodes.y[start], s=160, facecolors="none",
                   edgecolors="#c0392b", linewidths=1.5, zorder=4)
        handles.append(Line2D([], [], ls="", marker="o", ms=11, mfc="none", mec="#c0392b",
                              mew=1.5, label=f"start ({start})"))

    ax.set_aspect("equal")
    cax = make_axes_locatable(ax).append_axes("right", size="3%", pad=0.15)
    fig.colorbar(pts, cax=cax, label="node cost")
    ax.set_title(f"{label(method)}, {instance}, objective = {obj}")
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.legend(handles=handles, loc="upper center", bbox_to_anchor=(0.5, -0.13), ncol=4,
              fontsize=8, frameon=False)
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
