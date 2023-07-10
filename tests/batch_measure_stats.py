"""
This generates random tilesets then runs test-tileset-suggestion on each of them
"""

from os.path import dirname, join, isfile
from math import sqrt
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
    all_reports = getAllReports()
    all_stats = extractStats(all_reports)
    consolidateStats(all_stats)

#--------------------------------------------
# Steps

def getAllReports():
    all_reports = []
    for i in range(tileset_count):
        tileset_name = f"tileset.random{i:04d}"
        for macro_name in macrosurfaces:
            exp_name = f"{macro_name}_{tileset_name}"
            report_path = join(report_dir, f"test-tile-suggestion_{exp_name}.json")
            all_reports.append((exp_name, report_path))
    return all_reports

def extractStats(all_reports):
    all_stats = []
    for exp_name, path in all_reports:
        with open(path, 'r') as f:
            data = json.load(f)
        entry = {}
        best_stats = None
        for exp in data["experiments"].values():
            if "trivial" in exp["report"] and exp["report"]["trivial"]:
                # Solvable config, no suggestion was needed
                break
            success_rate = exp["report"]["successCount"] / exp["config"]["iterations"]
            mean_count = exp["report"]["meanGeneratedTiles"]
            stddev_count = exp["report"]["stddevGeneratedTiles"]
            stats = Stats(success_rate, mean_count, stddev_count)
            strategy = exp["config"]["strategy"]
            entry[strategy] = stats
            if best_stats is None or stats.isBetterThan(best_stats):
                best_stats = stats
        best_count = 0
        for stats in entry.values():
            stats.is_best = stats.equals(best_stats)
            if stats.is_best:
                best_count += 1
        all_stats.append((exp_name, entry, best_count))
    return all_stats

def consolidateStats(all_stats):
    all_strategies = [
        "Voting",
        "GuidedRandom",
        "Random",
    ]
    for strategy in all_strategies:
        is_best = []
        is_exclusive_best = []
        success_rates = []
        mean_counts = []
        mean_counts_success = []
        stddev_counts = []
        for exp_name, entry, best_count in all_stats:
            stats = entry.get(strategy)
            if stats is None:
                continue
            is_best.append(stats.is_best)
            is_exclusive_best.append(stats.is_best and best_count == 1)
            success_rates.append(stats.success_rate)
            if stats.success_rate > 0:
                mean_counts.append(stats.mean_count)
                mean_counts_success.append(stats.mean_count)
                stddev_counts.append(stats.stddev_count)
            else:
                mean_counts.append(10)
        print(f"Strategy '{strategy}' is best in {sum(is_best) / len(is_best) * 100:.1f}% of cases")
        print(f"Strategy '{strategy}' is exclusively best in {sum(is_exclusive_best) / len(is_exclusive_best) * 100:.1f}% of cases")

        avg_success_rates = sum(success_rates) / len(success_rates)
        std_success_rates = sqrt(sum([x**2 for x in success_rates]) / len(success_rates) - avg_success_rates**2)
        print(f"Strategy '{strategy}' has an average success rate of {avg_success_rates * 100:.1f}% +- {std_success_rates * 100:.1f}%")

        avg_mean_counts = sum(mean_counts) / len(mean_counts)
        avg_stddev_counts = sum(stddev_counts) / len(stddev_counts)
        print(f"Strategy '{strategy}' has an average tile need of {avg_mean_counts:.1f} +- {avg_stddev_counts:.1f} (when clamping to 10)")

        avg_mean_counts = sum(mean_counts_success) / len(mean_counts_success)
        avg_stddev_counts = sum(stddev_counts) / len(stddev_counts)
        print(f"Strategy '{strategy}' has an average tile need of {avg_mean_counts:.1f} +- {avg_stddev_counts:.1f} among successful runs")

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
        return (
            prefix + f"{self.success_rate * 100:.0f}\\%" + sufix + " & " +
            prefix + f"{self.mean_count:.1f} \\pm {self.stddev_count:.1f}" + sufix
        )

#--------------------------------------------

if __name__ == "__main__":
    main()
