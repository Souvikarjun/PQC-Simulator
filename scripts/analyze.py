"""Minimal CSV analysis helper; simulation remains independent of Python."""

import csv
import statistics
import sys


def main(path: str) -> None:
    with open(path, newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise SystemExit("CSV contains no result rows")
    delivery = [float(row["delivery_ratio"]) for row in rows]
    authentication = [float(row["auth_ratio"]) for row in rows]
    print(f"runs: {len(rows)}")
    print(f"delivery ratio mean: {statistics.mean(delivery):.4f}")
    print(f"authentication ratio mean: {statistics.mean(authentication):.4f}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: python scripts/analyze.py result.csv")
    main(sys.argv[1])
