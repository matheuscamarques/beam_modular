#!/usr/bin/env python3
import os
import json
import re
import subprocess

def get_git_commit():
    try:
        commit = subprocess.check_output(["git", "rev-parse", "HEAD"]).decode("utf-8").strip()
        return commit
    except Exception:
        return "unknown"

def parse_opcodes(header_path):
    opcodes = []
    if not os.path.exists(header_path):
        return opcodes
    with open(header_path, "r", encoding="utf-8") as f:
        content = f.read()
    
    match = re.search(r'typedef enum\s*\{(.*?)\}\s*beam_opcode_t;', content, re.DOTALL)
    if match:
        enum_body = match.group(1)
        for line in enum_body.split('\n'):
            line = line.strip()
            if line and not line.startswith("//") and not line.startswith("/*"):
                name = line.split('=')[0].split(',')[0].strip()
                if name.startswith("BEAM_OP_"):
                    opcodes.append(name)
    return opcodes

def scan_includes(src_dir):
    dependencies = []
    for root, _, files in os.walk(src_dir):
        for file in files:
            if file.endswith(".c") or file.endswith(".h"):
                file_path = os.path.join(root, file)
                rel_path = os.path.relpath(file_path, src_dir)
                with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                    for line in f:
                        m = re.match(r'#include\s+["<](.*?)[">]', line.strip())
                        if m:
                            inc = m.group(1)
                            dependencies.append({
                                "source": rel_path,
                                "target": inc
                            })
    return dependencies

def generate_knowledge_graph():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    src_dir = os.path.join(repo_root, "src")
    header_path = os.path.join(src_dir, "emulator", "beam_emu_internal.h")
    
    commit = get_git_commit()
    opcodes = parse_opcodes(header_path)
    dependencies = scan_includes(src_dir)
    
    modules = ["emulator", "scheduler", "memory", "messaging", "global", "io", "utils"]
    
    graph_data = {
        "commit": commit,
        "language": "C23",
        "standard": "ISO C23 (-std=c23)",
        "modules": modules,
        "supported_opcodes_count": len(opcodes),
        "supported_opcodes": opcodes,
        "dependencies": dependencies,
        "architecture_notes": {
            "memory_model": "32-bit tagged Eterm immediate & heap pointer encoding",
            "state_persistence": "beam_emulator_frame_t embedded directly in beam_process_t PCB for preemptive scheduler safety",
            "loader_fix": "AtU8 chunk num_atoms parsed as signed 32-bit int (-NumAtoms) to prevent 34GB OOM"
        }
    }
    
    docs_dir = os.path.join(repo_root, "docs")
    os.makedirs(docs_dir, exist_ok=True)
    
    json_path = os.path.join(docs_dir, "knowledge_graph.json")
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(graph_data, f, indent=2)
    print(f"[KNOWLEDGE GRAPH] Successfully generated JSON graph at {json_path}")
    
    mermaid_path = os.path.join(docs_dir, "architecture_graph.md")
    with open(mermaid_path, "w", encoding="utf-8") as f:
        f.write("# Architecture & Knowledge Graph\n\n")
        f.write(f"**Last Commit**: `{commit}`  \n")
        f.write(f"**Supported Opcodes**: {len(opcodes)}  \n\n")
        f.write("## Component Dependency Graph\n\n")
        f.write("```mermaid\n")
        f.write("graph TD\n")
        f.write("    EMU[Emulator Module] --> |Decodes & Executes| OP[Opcodes]\n")
        f.write("    EMU --> |BIF Dispatch| BIF[BIF System]\n")
        f.write("    SCHED[Scheduler Module] --> |Preempts & Manages| PCB[Process Control Block]\n")
        f.write("    PCB --> |Embeds| FRAME[Emulator Frame]\n")
        f.write("    LOAD[BEAM Loader] --> |Parses Chunks| BEAM[BEAM Files]\n")
        f.write("    MEM[Memory System] --> |Allocates| HEAP[Process Heap & Arenas]\n")
        f.write("```\n\n")
        f.write("## Supported Opcodes List\n\n")
        for op in opcodes:
            f.write(f"- `{op}`\n")
            
    print(f"[KNOWLEDGE GRAPH] Successfully generated Mermaid graph at {mermaid_path}")

if __name__ == "__main__":
    generate_knowledge_graph()
