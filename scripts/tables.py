from common import ROOT, assignment_arg, label, load_best, load_summary


def bold_if(text, cond):
    return f"**{text}**" if cond else text


def table(df, value):
    instances = list(dict.fromkeys(df.instance))
    lines = ["| Method | " + " | ".join(instances) + " |",
             "|---|" + "---|" * len(instances)]
    for method in dict.fromkeys(df.method):
        cells = [value(df[(df.instance == i) & (df.method == method)].iloc[0])
                 for i in instances]
        lines.append(f"| {label(method)} | " + " | ".join(cells) + " |")
    return "\n".join(lines)


def main():
    a = assignment_arg()
    df = load_summary(a)
    best_avg = df.groupby("instance")["avg"].min()
    best_min = df.groupby("instance")["min"].min()
    fastest = df.groupby("instance")["avg_ms"].min()
    out = [
        "Objective value: average (min - max)",
        "",
        # best average and best single solution in each instance are in bold
        table(df, lambda r: f"{bold_if(f'{r.avg:.1f}', r.avg == best_avg[r.instance])} "
                            f"({bold_if(r['min'], r['min'] == best_min[r.instance])} - {r['max']})"),
        "",
        "Average time of one run [ms]",
        "",
        table(df, lambda r: bold_if(f"{r.avg_ms:.3f}",
                                    f"{r.avg_ms:.3f}" == f"{fastest[r.instance]:.3f}")),
        "",
    ]
    for r in df.itertuples():
        sol = load_best(a, r.instance, r.method)
        out += [f"{label(r.method)}, {r.instance} ({r.min}):", "",
                "```", ", ".join(map(str, sol)), "```", ""]

    path = ROOT / "reports" / f"assignment{a}" / "tables.md"
    path.write_text("\n".join(out))
    print(path.relative_to(ROOT))
    print("\n".join(out[:15]))


if __name__ == "__main__":
    main()
