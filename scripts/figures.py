#!/usr/bin/env python3
"""Turn a sweep's output into the figures a release publishes.

    scripts/figures.py table <sweep-out>
        The accumulated figures, one row per stage, as Markdown.

    scripts/figures.py amend <sweep-out> <ref> [--update-ref <refname>]
        Rewrite the figures block in each stage commit's message from the sweep.
        Messages only: every tree is kept, so the sweep's figures stay valid for
        the rewritten commits. Prints the new tip; moves no ref unless asked.
"""
import argparse
import csv
import os
import pathlib
import re
import subprocess
import sys

# Flash is the read-only image plus the initialisers copied out of it; RAM is
# .data plus .bss, which already holds the heap, the mbedTLS pool and the stacks.
DERIVED = {
    "Flash": ("flash_text", "flash_data"),
    "RAM": ("flash_data", "static_bss"),
    "mbedTLS peak": ("mbedtls_peak",),
    "Log stack": ("stack_log",),
    "Service": ("stack_service",),
}
TABLE_COLUMNS = ["Flash", "RAM", "mbedTLS peak", "Log stack", "Service"]

BLOCK_LINE = re.compile(r"^  ([A-Za-z][A-Za-z .]*?)\s+([+-]?[\d,]+) B(?:\s+\((.*)\))?\s*$")
PROSE_FIGURE = re.compile(r"\d[\d,]*\s*(?:bytes|B\b|KiB|KB)")


def load(out):
    out = pathlib.Path(out)
    with open(out / "stages.csv", newline="", encoding="utf-8") as f:
        stages = list(csv.DictReader(f))
    figures = {}
    with open(out / "figures.csv", newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            figures.setdefault(int(row["index"]), {})[row["key"]] = (
                int(row["current"]),
                int(row["above_baseline"]),
            )
    return stages, figures


def value(figures, label, field):
    keys = DERIVED.get(label)
    if keys is None:
        raise SystemExit(f"figures: no figure is known for the label {label!r}")
    return sum(figures[k][field] for k in keys)


# ---- table ---------------------------------------------------------------------


def table(args):
    stages, figures = load(args.out)
    print("| # | Stage | " + " | ".join(TABLE_COLUMNS) + " | Run |")
    print("|---|---|" + "---:|" * len(TABLE_COLUMNS) + "---|")
    for s in stages:
        k = int(s["index"])
        f = figures.get(k)
        cells = []
        for label in TABLE_COLUMNS:
            if not f:
                cells.append("")
                continue
            field = 0 if k == 0 else 1
            v = value(f, label, field)
            cell = f"{v:,}" if k == 0 else f"{v:+,}"
            prev = figures.get(k - 1)
            if k > 1 and prev:
                d = v - value(prev, label, 1)
                if d:
                    cell += f" ({d:+,})"
            cells.append(cell)
        print(f"| {k} | {s['slug']} | " + " | ".join(cells) + f" | {s['verdict']} |")
    print()
    print("Bytes. The Baseline row is absolute; every other row is above the Baseline,")
    print("with the change on the previous stage in brackets.")


# ---- amend ---------------------------------------------------------------------


def git(*args, env=None, stdin=None):
    # Bytes both ways, and UTF-8 whatever the user's config says: text mode on Windows
    # would write a message's newlines as CRLF, and a legacy i18n encoding would mislabel
    # or mis-decode the message.
    out = subprocess.run(
        ["git", "-c", "i18n.commitEncoding=UTF-8", "-c", "i18n.logOutputEncoding=UTF-8", *args],
        check=True, capture_output=True, env=env,
        input=None if stdin is None else stdin.encode("utf-8"),
    ).stdout
    return out.decode("utf-8").replace("\r\n", "\n")


def rewrite_block(message, k, figures):
    lines = message.split("\n")
    start = next((i for i, line in enumerate(lines) if BLOCK_LINE.match(line)), None)
    if start is None:
        return message, False
    end = start
    while end < len(lines) and BLOCK_LINE.match(lines[end]):
        end += 1

    cur = figures[k]
    prev = figures.get(k - 1)
    entries = []
    for pos, line in enumerate(lines[start:end]):
        label, _, paren = BLOCK_LINE.match(line).groups()
        if k == 0:
            text = f"{value(cur, label, 0):,} B"
        else:
            text = f"{value(cur, label, 1):+,} B"
        note = None
        if paren is not None and "absolute" in paren:
            note = f"({value(cur, label, 0):,} absolute)"
        elif paren is not None and prev is not None:
            d = value(cur, label, 1) - value(prev, label, 1)
            if d == 0:
                note = "(unchanged)"
            else:
                note = f"({d:+,}{' on the previous stage' if pos == 0 else ''})"
        entries.append((label, text, note))

    wl = max(len(e[0]) for e in entries)
    wv = max(len(e[1]) for e in entries)
    block = [
        f"  {label.ljust(wl)}  {text.rjust(wv)}" + (f"  {note}" if note else "")
        for label, text, note in entries
    ]
    for line in lines[:start] + lines[end:]:
        if PROSE_FIGURE.search(line):
            print(f"  check prose: {line.strip()}", file=sys.stderr)
    return "\n".join(lines[:start] + block + lines[end:]), True


def amend(args):
    stages, figures = load(args.out)
    mapped = {s["commit"]: int(s["index"]) for s in stages}
    tip = git("rev-parse", "--verify", f"{args.ref}^{{commit}}").strip()
    chain = git("rev-list", "--first-parent", "--reverse", tip).split()
    # Rebuilding keeps one parent per commit, so a merge would lose the others.
    merges = git("rev-list", "--merges", tip).split()
    if merges:
        raise SystemExit(f"figures: {args.ref} is not linear; {merges[0][:7]} is a merge")
    missing = [c for c in mapped if c not in chain]
    if missing:
        raise SystemExit(f"figures: {len(missing)} swept commit(s) are not on {args.ref}, e.g. {missing[0]}")

    parent = None
    rebuilding = False
    for c in chain:
        message = git("log", "-1", "--format=%B", c).rstrip("\n") + "\n"
        new_message = message
        if c in mapped and mapped[c] in figures:
            print(f"{c[:7]} {git('log', '-1', '--format=%s', c).strip()}", file=sys.stderr)
            new_message, found = rewrite_block(message, mapped[c], figures)
            if not found and mapped[c] > 0:
                print("  no figures block", file=sys.stderr)
        if not rebuilding and new_message == message:
            parent = c
            continue
        rebuilding = True
        fmt = "%an%x00%ae%x00%ad%x00%cn%x00%ce%x00%cd"
        an, ae, ad, cn, ce, cd = git("log", "-1", f"--format={fmt}", "--date=raw", c).split("\x00")
        env = dict(
            os.environ,
            GIT_AUTHOR_NAME=an, GIT_AUTHOR_EMAIL=ae, GIT_AUTHOR_DATE=ad,
            GIT_COMMITTER_NAME=cn, GIT_COMMITTER_EMAIL=ce, GIT_COMMITTER_DATE=cd.strip(),
        )
        tree = git("rev-parse", f"{c}^{{tree}}").strip()
        parents = ["-p", parent] if parent else []
        parent = git("commit-tree", tree, *parents, env=env, stdin=new_message).strip()

    if not rebuilding:
        print("no message changed", file=sys.stderr)
    elif args.update_ref:
        git("update-ref", args.update_ref, parent, tip)
    print(parent)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="command", required=True)
    t = sub.add_parser("table")
    t.add_argument("out")
    t.set_defaults(run=table)
    a = sub.add_parser("amend")
    a.add_argument("out")
    a.add_argument("ref")
    a.add_argument("--update-ref")
    a.set_defaults(run=amend)
    args = p.parse_args()
    args.run(args)


if __name__ == "__main__":
    main()
