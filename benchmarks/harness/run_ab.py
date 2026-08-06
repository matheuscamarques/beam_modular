#!/usr/bin/env python3
"""
run_ab.py - A/B harness for BEAM comparative benchmarks.

Side A: original Erlang/OTP VM (reference, built from otp_src).
Side B: the modular rewrite (C drivers in benchmarks/drivers/b, via CMake
        ENABLE_BENCH).

Driver output protocol (both sides):
  RESULT <line>       (repeated; canonical sequence = parity)
  FINGERPRINT <hex64>  (FNV-1a 64 over lines + "\n")
  TIME_US <int>        (driver-internal time, secondary)
  OPS <int>            (semantic operations)
  METRIC k=v           (extra metrics, do not participate in parity)

Parity = hard gate: the RESULT sequence of ALL runs and sides must be
byte-for-byte identical. Performance uses median wall-time per side
(median + MAD, alternating ABBA order, warmup, taskset pinning).
"""

import argparse
import json
import os
import statistics
import subprocess
import sys
import time
from datetime import datetime, timezone

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CATALOG = os.path.join(REPO, "benchmarks", "workloads.json")
DRIVERS_A = os.path.join(REPO, "benchmarks", "drivers", "a")
SAMPLES = os.path.join(REPO, "benchmarks", "samples")
REPORTS = os.path.join(REPO, "benchmarks", "reports")
SCRATCH = os.path.join(REPO, "benchmarks", "work", "scratch")
BENCH_DIR = os.path.join(REPO, "benchmarks", "work", "bench")
ERL_DEFAULT = os.path.join(REPO, "otp_src", "bin", "erl")
ERLC_DEFAULT = os.path.join(REPO, "otp_src", "bin", "erlc")


def log(msg, quiet=False):
    if not quiet:
        print(msg, flush=True)


def cmd_ok(path, hint):
    if not os.path.isfile(path):
        print(f"ERROR: {path} not found. {hint}", file=sys.stderr)
        sys.exit(2)
    return path


def cpu_model():
    try:
        with open("/proc/cpuinfo") as f:
            for line in f:
                if line.startswith("model name"):
                    return line.split(":", 1)[1].strip()
    except OSError:
        pass
    return "unknown"


def otp_release(erl):
    try:
        out = subprocess.run(
            [erl, "-noshell", "-eval",
             'io:format("~s", [erlang:system_info(otp_release)]), halt(0).'],
            capture_output=True, timeout=60)
        return out.stdout.decode().strip()
    except Exception:
        return "?"


def git_head():
    try:
        out = subprocess.run(["git", "-C", REPO, "rev-parse", "--short", "HEAD"],
                             capture_output=True, timeout=10)
        return out.stdout.decode().strip()
    except Exception:
        return "?"


def compile_driver(erlc, src, outdir, quiet):
    os.makedirs(outdir, exist_ok=True)
    beam = os.path.join(outdir, os.path.splitext(os.path.basename(src))[0] + ".beam")
    if os.path.exists(beam) and os.path.getmtime(beam) >= os.path.getmtime(src):
        return
    log(f"  erlc: {os.path.basename(src)}", quiet)
    r = subprocess.run([erlc, "-o", outdir, src], capture_output=True, timeout=120)
    if r.returncode != 0:
        print(f"ERROR compiling {src}:\n{r.stderr.decode()}", file=sys.stderr)
        sys.exit(2)


def parse_stdout(out_bytes):
    lines = out_bytes.decode(errors="replace").splitlines()
    result = [l[len("RESULT "):] for l in lines if l.startswith("RESULT ")]
    fp = next((l[len("FINGERPRINT "):] for l in lines if l.startswith("FINGERPRINT ")), "")
    time_us = next((l[len("TIME_US "):] for l in lines if l.startswith("TIME_US ")), "")
    ops = next((l[len("OPS "):] for l in lines if l.startswith("OPS ")), "")
    metrics = {}
    for l in lines:
        if l.startswith("METRIC "):
            k, _, v = l[len("METRIC "):].partition("=")
            metrics[k] = v
    return result, fp, time_us, ops, metrics


def resolve_args(wl, scratch):
    if "args" in wl:
        return [os.path.join(scratch, a) for a in wl["args"]]
    return [str(wl["n"])]


def run_side(side, wl, scratch, erl, bench_dir, cpu, quiet):
    if side == "A":
        mod = wl["module"]
        base = [erl, "-noshell", "-pa", scratch, "-eval", f"{mod}:main(), halt(0).", "-extra"]
        args = base + resolve_args(wl, scratch)
    else:
        args = [os.path.join(bench_dir, wl["bin"])] + resolve_args(wl, scratch)

    run_args = args
    if cpu:
        run_args = ["taskset", "-c", cpu] + args

    t0 = time.perf_counter()
    try:
        r = subprocess.run(run_args, capture_output=True, timeout=600)
    except FileNotFoundError:
        run_args = args
        r = subprocess.run(run_args, capture_output=True, timeout=600)
    wall_s = time.perf_counter() - t0

    if r.returncode == 127 and cpu:  # taskset unavailable
        run_args = args
        t0 = time.perf_counter()
        r = subprocess.run(run_args, capture_output=True, timeout=600)
        wall_s = time.perf_counter() - t0

    result, fp, time_us, ops, metrics = parse_stdout(r.stdout)
    if r.returncode != 0:
        raise RuntimeError(
            f"side {side} ({os.path.basename(args[0])}) exit={r.returncode}: "
            f"{r.stderr.decode(errors='replace')[-500:]}")

    return {
        "side": side,
        "wall_s": wall_s,
        "time_us": int(time_us) if time_us.isdigit() else None,
        "ops": int(ops) if ops.isdigit() else None,
        "fingerprint": fp,
        "result": result,
        "metrics": metrics,
    }


def run_workload(name, wl, erl, bench_dir, scratch, cpu, runs, warmup, quiet):
    log(f"== Workload: {name} ({wl.get('desc', '')})", quiet)
    if not os.path.isfile(os.path.join(bench_dir, wl["bin"])):
        raise RuntimeError(
            f"driver B '{wl['bin']}' nao encontrado em {bench_dir}. "
            f"Execute: make -C benchmarks/drivers/b")
    if warmup > 0:
        log("  warmup:", quiet)
        for s in ("A", "B"):
            try:
                run_side(s, wl, scratch, erl, bench_dir, cpu, quiet)
                log(f"    {s} ok", quiet)
            except RuntimeError as e:
                log(f"    {s} FAILED in warmup: {e}", quiet)

    data = []
    for r in range(runs):
        pair = ("A", "B", "B", "A") if r % 2 == 0 else ("B", "A", "A", "B")
        for s in pair:
            rec = run_side(s, wl, scratch, erl, bench_dir, cpu, quiet)
            rec["run"] = r
            data.append(rec)
            log(f"  run {r} {s}: wall={rec['wall_s']:.3f}s fp={rec['fingerprint'][:8]}", quiet)

    parity = len({tuple(d["result"]) for d in data}) == 1
    status = "PASS" if parity else "FAIL"

    stats = {}
    for s in ("A", "B"):
        walls = [d["wall_s"] for d in data if d["side"] == s]
        med = statistics.median(walls)
        stats[s] = {
            "median_wall_s": med,
            "min_wall_s": min(walls),
            "max_wall_s": max(walls),
            "mad_wall_s": statistics.median(abs(x - med) for x in walls),
            "n": len(walls),
        }
    delta = ((stats["B"]["median_wall_s"] - stats["A"]["median_wall_s"])
             / stats["A"]["median_wall_s"] * 100.0) if stats["A"]["median_wall_s"] else None

    log(f"  PARITY: {status}", quiet)
    log(f"  A median: {stats['A']['median_wall_s']*1000:.1f}ms   "
        f"B median: {stats['B']['median_wall_s']*1000:.1f}ms   delta(B/A): "
        f"{delta:+.1f}%" if delta is not None else "", quiet)

    return {"parity": parity, "runs": data, "stats": stats, "delta_pct": delta,
            "status": status}


def write_report(reports, workloads, meta, quiet):
    os.makedirs(reports, exist_ok=True)
    ts = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    base = os.path.join(reports, f"report_{ts}")
    payload = {"meta": meta, "workloads": workloads}

    with open(base + ".json", "w") as f:
        json.dump(payload, f, indent=2, default=str)

    md = [f"# A/B report - {ts}",
          "",
          f"- Reference A: OTP {meta['otp_release']} ({meta['erl']})",
          f"- Rewrite B: beam_modular @ {meta['git_head']}",
          f"- Machine: {meta['cpu']}",
          f"- CPU pin: {meta['cpu_pin'] or 'none'} | runs: {meta['runs']} (+{meta['warmup']} warmup)",
          "",
          "| workload | parity | A med (ms) | B med (ms) | delta B/A | MAD A | MAD B |",
          "|---|---|---|---|---|---|---|"]
    for name, w in workloads.items():
        st = w.get("stats") or {}
        a, b = st.get("A"), st.get("B")
        if a is None or b is None:
            md.append(f"| {name} | {w['status']} | - | - | - | - | - |")
            continue
        delta = w["delta_pct"]
        md.append(
            f"| {name} | {w['status']} | {a['median_wall_s']*1000:.1f} "
            f"| {b['median_wall_s']*1000:.1f} "
            f"| {delta:+.1f}% " if delta is not None else "| - "
            f"| {a['mad_wall_s']*1000:.1f} "
            f"| {b['mad_wall_s']*1000:.1f} |")
    with open(base + ".md", "w") as f:
        f.write("\n".join(md) + "\n")

    log(f"Report: {base}.md", quiet)
    return base


def main():
    ap = argparse.ArgumentParser(description="A/B harness: original BEAM vs rewrite")
    ap.add_argument("--workload", help="workload name from the catalog")
    ap.add_argument("--all", action="store_true", help="run all workloads")
    ap.add_argument("--runs", type=int, default=7, help="number of runs (default 7)")
    ap.add_argument("--warmup", type=int, default=1, help="warmup runs (default 1)")
    ap.add_argument("--cpu", default="3", help="taskset -c pinning (default '3'; '' disables)")
    ap.add_argument("--out", default=REPORTS, help="reports directory")
    ap.add_argument("--erl", default=ERL_DEFAULT, help="path to erl (side A)")
    ap.add_argument("--erlc", default=ERLC_DEFAULT, help="path to erlc")
    ap.add_argument("--bench-dir", default=BENCH_DIR, help="B drivers directory")
    ap.add_argument("--quiet", action="store_true", help="less output")
    args = ap.parse_args()

    erl = cmd_ok(args.erl, "Run make -j4 in otp_src (in-source OTP 30 build).")
    erlc = cmd_ok(args.erlc, "Run make -j4 in otp_src (in-source OTP 30 build).")

    with open(CATALOG) as f:
        catalog = json.load(f)
    if args.all:
        names = [k for k in catalog if not k.startswith("_")]
    elif args.workload:
        names = [args.workload]
    else:
        ap.error("provide --workload <name> or --all")
    for n in names:
        if n not in catalog:
            print(f"ERROR: unknown workload '{n}'. Available: "
                  f"{[k for k in catalog if not k.startswith('_')]}", file=sys.stderr)
            sys.exit(2)

    meta = {
        "otp_release": otp_release(erl),
        "erl": erl,
        "git_head": git_head(),
        "cpu": cpu_model(),
        "cpu_pin": args.cpu,
        "runs": args.runs,
        "warmup": args.warmup,
        "timestamp": datetime.now(timezone.utc).isoformat(),
    }
    log(f"Reference OTP: {meta['otp_release']} | rewrite commit: {meta['git_head']}")

    os.makedirs(SCRATCH, exist_ok=True)
    compile_driver(erlc, os.path.join(DRIVERS_A, "ab.erl"), SCRATCH, args.quiet)
    compile_driver(erlc, os.path.join(SAMPLES, "sample_module.erl"), SCRATCH, args.quiet)

    workloads = {}
    all_ok = True
    for name in names:
        wl = catalog[name]
        compile_driver(erlc, os.path.join(DRIVERS_A, wl["module"] + ".erl"), SCRATCH, args.quiet)
        try:
            res = run_workload(name, wl, erl, args.bench_dir, SCRATCH, args.cpu,
                               args.runs, args.warmup, args.quiet)
        except RuntimeError as e:
            print(f"  ERROR in workload {name}: {e}", file=sys.stderr)
            res = {"parity": False, "status": "ERROR", "runs": [], "stats": {},
                   "delta_pct": None, "error": str(e)}
        workloads[name] = res
        if not res["parity"]:
            all_ok = False

    base = write_report(args.out, workloads, meta, args.quiet)
    if args.quiet:
        print(f"Report: {base}.md")
    sys.exit(0 if all_ok else 1)


if __name__ == "__main__":
    main()