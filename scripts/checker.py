import shutil
import subprocess
import tempfile
from pathlib import Path

from common import ROOT, assignment_arg, load_best, load_summary

# Pastes every best solution into column F of a copy of the solution checker, lets Numbers
# (macOS) recalculate it and compares K2 with the objective from summary.csv.
SCRIPT = """
on run argv
  set f to POSIX file (item 1 of argv)
  set sheetName to item 2 of argv
  set vals to {}
  repeat with p in paragraphs of (item 3 of argv)
    set end of vals to (p as integer)
  end repeat
  tell application "Numbers"
    set d to open f
    delay 2
    tell table 1 of sheet sheetName of d
      repeat with i from 1 to count of vals
        set value of cell ("F" & (i + 2)) to (item i of vals)
      end repeat
      set res to value of cell "K2"
    end tell
    close d saving no
  end tell
  return res as integer
end run
"""


def main():
    a = assignment_arg()
    tmp = Path(tempfile.mkdtemp())
    copy = tmp / "checker.xlsx"
    shutil.copy(ROOT / "docs" / "Solution checker.xlsx", copy)
    bad = 0
    for r in load_summary(a).itertuples():
        sol = load_best(a, r.instance, r.method)
        out = subprocess.run(["osascript", "-e", SCRIPT, str(copy), r.instance,
                              "\n".join(map(str, sol))],
                             capture_output=True, text=True)
        if out.returncode:
            raise SystemExit(out.stderr)
        got = int(out.stdout.strip())
        ok = got == r.min
        bad += not ok
        print(f"{r.instance} {r.method:22} checker {got:7} ours {r.min:7} {'ok' if ok else 'DIFF'}")
    shutil.rmtree(tmp)
    print("all match" if bad == 0 else f"{bad} mismatch(es)")


if __name__ == "__main__":
    main()
