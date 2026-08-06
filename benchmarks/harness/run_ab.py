#!/usr/bin/env python3
"""
run_ab.py - Harness A/B dos testes comparativos BEAM.

Lado A: VM Erlang/OTP original (referencia, buildado de otp_src).
Lado B: reescrita modular (drivers C em benchmarks/drivers/b, via CMake ENABLE_BENCH).

Protocolo de saida dos drivers (ambos os lados):
  RESULT <linha>       (repetido; sequencia canonica = paridade)
  FINGERPRINT <hex64>  (FNV-1a 64 sobre linhas + "\\n")
  TIME_US <int>        (tempo interno do driver, secundario)
  OPS <int>            (operacoes semanticas)
  METRIC k=v           (metricas extras, nao participam da paridade)

Paridade = gate duro: a sequencia RESULT de TODOS os runs e lados deve
ser identica byte-a-byte. Performance usa wall-time mediano por lado
(mediana + MAD, ordem ABBA alternada, warmup, pinning via taskset).
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
SCRATCH = os.path.join(REPO, "build", "ab")
BENCH_DIR = os.path.join(REPO, "build", "benchmarks")
ERL_DEFAULT = os.path.join(REPO, "build_otp30", "bin", "erl")
ERLC_DEFAULT = os.path.join(REPO, "build_otp30", "bin", "erlc")


def log(msg, quiet=False):
    if not quiet:
        print(msg, flush=True)


def cmd_ok(path, hint):
    if not os.path.isfile(path):
        print(f"ERRO: {path} nao encontrado. {hint}", file=sys.stderr)
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
    return "desconhecido"


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
        print(f"ERRO compilando {src}:\n{r.stderr.decode()}", file=sys.stderr)
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

    if r.returncode == 127 and cpu:  # taskset indisponivel
        run_args = args
        t0 = time.perf_counter()
        r = subprocess.run(run_args, capture_output=True, timeout=600)
        wall_s = time.perf_counter() - t0

    result, fp, time_us, ops, metrics = parse_stdout(r.stdout)
    if r.returncode != 0:
        raise RuntimeError(
            f"lado {side} ({os.path.basename(args[0])}) exit={r.returncode}: "
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
    if warmup > 0:
        log("  warmup:", quiet)
        for s in ("A", "B"):
            try:
                run_side(s, wl, scratch, erl, bench_dir, cpu, quiet)
                log(f"    {s} ok", quiet)
            except RuntimeError as e:
                log(f"    {s} FALHOU no warmup: {e}", quiet)

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

    log(f"  PARIDADE: {status}", quiet)
    log(f"  A mediana: {stats['A']['median_wall_s']*1000:.1f}ms   "
        f"B mediana: {stats['B']['median_wall_s']*1000:.1f}ms   delta(B/A): "
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

    md = [f"# Relatorio A/B - {ts}",
          "",
          f"- Referencia A: OTP {meta['otp_release']} ({meta['erl']})",
          f"- Reescrita B: beam_modular @ {meta['git_head']}",
          f"- Maquina: {meta['cpu']}",
          f"- CPU pin: {meta['cpu_pin'] or 'nenhum'} | runs: {meta['runs']} (+{meta['warmup']} warmup)",
          "",
          "| workload | paridade | A med (ms) | B med (ms) | delta B/A | MAD A | MAD B |",
          "|---|---|---|---|---|---|---|"]
    for name, w in workloads.items():
        md.append(
            f"| {name} | {w['status']} | {w['stats']['A']['median_wall_s']*1000:.1f} "
            f"| {w['stats']['B']['median_wall_s']*1000:.1f} "
            f"| {w['delta_pct']:+.1f}% " if w['delta_pct'] is not None else "| - "
            f"| {w['stats']['A']['mad_wall_s']*1000:.1f} "
            f"| {w['stats']['B']['mad_wall_s']*1000:.1f} |")
    with open(base + ".md", "w") as f:
        f.write("\n".join(md) + "\n")

    log(f"Relatorio: {base}.md", quiet)
    return base


def main():
    ap = argparse.ArgumentParser(description="Harness A/B BEAM original vs reescrita")
    ap.add_argument("--workload", help="nome do workload no catalogo")
    ap.add_argument("--all", action="store_true", help="rodar todos os workloads")
    ap.add_argument("--runs", type=int, default=7, help="numero de runs (default 7)")
    ap.add_argument("--warmup", type=int, default=1, help="runs de aquecimento (default 1)")
    ap.add_argument("--cpu", default="3", help="pinning taskset -c (default '3'; '' desliga)")
    ap.add_argument("--out", default=REPORTS, help="diretorio de relatorios")
    ap.add_argument("--erl", default=ERL_DEFAULT, help="caminho do erl (lado A)")
    ap.add_argument("--erlc", default=ERLC_DEFAULT, help="caminho do erlc")
    ap.add_argument("--bench-dir", default=BENCH_DIR, help="diretorio dos drivers B")
    ap.add_argument("--quiet", action="store_true", help="menos saida")
    args = ap.parse_args()

    erl = cmd_ok(args.erl, "Execute make -j4 em build_otp30 (ou passe --erl).")
    erlc = cmd_ok(args.erlc, "Execute make -j4 em build_otp30 (ou passe --erlc).")
    cmd_ok(os.path.join(args.bench_dir, "bench_loader"),
           "Execute: cmake -B build -DENABLE_BENCH=ON && cmake --build build")

    with open(CATALOG) as f:
        catalog = json.load(f)
    if args.all:
        names = [k for k in catalog if not k.startswith("_")]
    elif args.workload:
        names = [args.workload]
    else:
        ap.error("informe --workload <nome> ou --all")
    for n in names:
        if n not in catalog:
            print(f"ERRO: workload '{n}' desconhecido. Disponiveis: "
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
    log(f"OTP referencia: {meta['otp_release']} | commit reescrita: {meta['git_head']}")

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
            print(f"  ERRO no workload {name}: {e}", file=sys.stderr)
            res = {"parity": False, "status": "ERROR", "runs": [], "stats": {},
                   "delta_pct": None, "error": str(e)}
        workloads[name] = res
        if not res["parity"]:
            all_ok = False

    base = write_report(args.out, workloads, meta, args.quiet)
    if args.quiet:
        print(f"Relatorio: {base}.md")
    sys.exit(0 if all_ok else 1)


if __name__ == "__main__":
    main()
