"""
This generates random tilesets then runs test-tileset-suggestion on each of them
"""

from os.path import dirname, join, isfile
from dataclasses import dataclass
import json
import subprocess
import random
random.seed(0)

#--------------------------------------------
# Parameters

iterations = 100
max_suggestions = 10
max_attempts = 10
tex_report_path = join(dirname(__file__), "data", "results", "batch.tex")

test_bin = join(dirname(__file__), "..", "build", "tests", "Release", "TestTileSuggestion.exe")
if not isfile(test_bin):
    print(f"ERROR: Binary file not found at {test_bin}")
    exit(1)

override_existing_reports = False

macrosurfaces = [
    "tshirt",
    "irgrid",
    "basket",
    "dress",
    "shell-instant",
    "torus",
    "irtorus",
]
macrosurface_dir = join(dirname(__file__), "data")

tileset_count = 20
tileset_dir = join(dirname(__file__), "data", "random-tilesets")

report_dir = join(dirname(__file__), "data", "results", "batch")

label_colors = [
    (0.9764705882352941, 0.2549019607843137, 0.26666666666666666),
    (0.9725490196078431, 0.5882352941176471, 0.11764705882352941),
    (0.9764705882352941, 0.7803921568627451, 0.30980392156862746),
    (0.5647058823529412, 0.7450980392156863, 0.42745098039215684),
    (0.2627450980392157, 0.6666666666666666, 0.5450980392156862),
    (0.3411764705882353, 0.4588235294117647, 0.5647058823529412),
    (0.4666666666666667, 0.3254901960784314, 0.6352941176470588),
]
max_label_count = len(label_colors)
max_tile_count = 5

#--------------------------------------------
# Main

def main():
    generateAllTilesets()
    runAllExperiments()
    generateTexSummary()

#--------------------------------------------
# Steps

def generateAllTilesets():
    for i in range(tileset_count):
        filename = join(tileset_dir, f"tileset.random{i:04d}.json")
        if isfile(filename):
            continue
        tileset = generateRandomTileset()
        saveTileset(tileset, filename)

def runAllExperiments():
    for i in range(tileset_count):
        for macro in macrosurfaces:
            runExperiment(macro, i)

def runExperiment(macro_name, tileset_idx):
    tileset_name = f"tileset.random{tileset_idx:04d}"
    tileset_path = join(tileset_dir, tileset_name + ".json")
    macro_path = join(macrosurface_dir, macro_name + ".obj")
    report_path = join(report_dir, f"test-tile-suggestion_{macro_name}_{tileset_name}.json")
    stdout_path = join(report_dir, f"stdout_{macro_name}_{tileset_name}.txt")
    stderr_path = join(report_dir, f"stderr_{macro_name}_{tileset_name}.txt")
    if isfile(report_path) and not override_existing_reports:
        print(f"Skipping experiment {macro_name}_{tileset_name} (report already exists).")
        return
    print(f"Running experiment {macro_name}_{tileset_name}...")
    proc = subprocess.run([
        test_bin,
        '-t', tileset_path,
        '-m', macro_path,
        '-r', report_path,
        '--iterations', str(iterations),
        '--max-suggestions', str(max_suggestions),
        '--max-attempts', str(max_attempts),
    ], capture_output=True)
    with open(stdout_path, 'wb') as f:
        f.write(proc.stdout)
    with open(stderr_path, 'wb') as f:
        f.write(proc.stderr)

def generateTexSummary():
    all_reports = []
    for i in range(tileset_count):
        tileset_name = f"tileset.random{i:04d}"
        for macro_name in macrosurfaces:
            exp_name = f"{macro_name}_{tileset_name}"
            report_path = join(report_dir, f"test-tile-suggestion_{exp_name}.json")
            all_reports.append((exp_name, report_path))

    strategy_column = {
        "Voting": 0,
        "GuidedRandom": 1,
        "Random": 2,
    }

    table_data = []
    for exp_name, path in all_reports:
        with open(path, 'r') as f:
            data = json.load(f)
        row = [None, None, None]
        best_col = None
        best_stats = None
        for exp in data["experiments"].values():
            if "trivial" in exp["report"] and exp["report"]["trivial"]:
                # Solvable config, no suggestion was needed
                break
            success_rate = exp["report"]["successCount"] / exp["config"]["iterations"]
            mean_count = exp["report"]["meanGeneratedTiles"]
            stddev_count = exp["report"]["stddevGeneratedTiles"]
            stats = Stats(success_rate, mean_count, stddev_count)
            col = strategy_column[exp["config"]["strategy"]]
            row[col] = stats
            if best_stats is None or stats.isBetterThan(best_stats):
                best_stats = stats
                best_col = col
        for stats in row:
            if stats is not None:
                stats.is_best = stats.equals(best_stats)
        table_data.append((exp_name, row, best_col))

    with open(tex_report_path, 'w') as f:
        f.write(r"""\begin{tabular}{ccccccc}
    \toprule
    Ex. & \multicolumn{2}{c}{Voting (ours)} & \multicolumn{2}{c}{Guided Random} & \multicolumn{2}{c}{Fully Random} \\
        & $R$ & $N$ & $R$ & $N$ & $R$ & $N$ \\
    \midrule
""")
        for (exp_name, row, best_col) in table_data:
            exp_name = exp_name.replace('_', '\\_')
            if best_col is None:
                f.write(exp_name + r" & \multicolumn{6}{c}{\textit{irrelevant (already solvable)}} \\" + "\n")
            else:
                entries = [
                    stats.tex()
                    if success_rate > 0
                    else f"$0\\%$ & N/A"
                    for col, stats in enumerate(row)
                ]
                f.write(exp_name + " & " + "& ".join(entries) + r" \\" + "\n")
        f.write(r"""\bottomrule
\end{tabular}
""")

#--------------------------------------------
# Utils

@dataclass
class Stats:
    success_rate: float
    mean_count: float
    stddev_count: float
    is_best: bool = False

    def isBetterThan(self, other):
        if self.success_rate > other.success_rate: return True
        if self.success_rate < other.success_rate: return False
        if self.mean_count < other.mean_count: return True
        if self.mean_count > other.mean_count: return False
        if self.stddev_count < other.stddev_count: return True
        if self.stddev_count > other.stddev_count: return False
        return False

    def equals(self, other):
        return (
            self.success_rate == other.success_rate
            and self.mean_count == other.mean_count
            and self.stddev_count == other.stddev_count
        )

    def tex(self):
        if self.is_best:
            prefix = r"\textcolor{violet}{$\mathbf{"
            sufix = r"}$}"
        else:
            prefix = r"$"
            sufix = r"$"

        if self.mean_count < 0:
            count_tex = "N/A"
        else:
            count_tex = prefix + f"{self.mean_count:.1f} \\pm {self.stddev_count:.1f}" + sufix

        rate_tex = prefix + f"{self.success_rate * 100:.0f}\\%" + sufix + " & "

        return rate_tex + count_tex

def random_bool():
    return random.randint(0, 1) == 1

def generateRandomTileset():
    label_count = random.randint(1, max_label_count)
    edgeTypes = []
    for i in range(label_count):
        borderEdge = random_bool()
        borderOnly = borderEdge and random_bool()
        edgeTypes.append({
            "shapes": [],
            "isExplicit": True,
            "borderEdge": borderEdge,
            "borderOnly": borderOnly,
            "color": label_colors[i]
        })

    tile_count = random.randint(1, max_tile_count)
    tileTypes = []
    for i in range(tile_count):
        tileTypes.append({
            "innerPaths": [],
            "edges": [
                {
                    "transform": {
                        "flipped": random_bool()
                    },
                    "tileEdgeIndex": random.randint(0, label_count - 1)
                }
                for _ in range(4)
            ],
            "transformPermission": {
                "rotation": random_bool(),
                "flipX": random_bool(),
                "flipY": random_bool()
            }
        })

    return {
        "tileTypes": tileTypes,
        "edgeTypes": edgeTypes,
        "defaultTileTransformPermission": {
            "rotation": True,
            "flipX": True,
            "flipY": True
        },
    }

def saveTileset(tileset, filename):
    with open(filename, 'w') as f:
        json.dump(tileset, f, indent=2)

#--------------------------------------------

if __name__ == "__main__":
    main()
