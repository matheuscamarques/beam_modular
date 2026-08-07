# Relatório Oficial de Benchmarks A/B: BEAM C23 VM vs Erlang/OTP 30 Original

> **Data**: 2026-08-06  
> **Commit**: `307a608` (Main Branch)  
> **Arquitetura**: C23 ISO Standard com SMP Multi-Threading Pool, Mailbox Lock-Free Atômica (`stdatomic.h`), Epoll I/O Driver e JIT Compiler Engine (`mmap` `PROT_EXEC`)  
> **Ambiente Hardware**: CPU AMD Ryzen 5 3500U with Radeon Vega Mobile Gfx (Pinning de CPU via Taskset)  
> **Configuração A/B**: Lado A = Erlang/OTP 30 Reference VM; Lado B = BEAM C23 Modular VM; 5 Rodadas Intercaladas ($A \rightarrow B \rightarrow B \rightarrow A$)  

---

## 📊 Tabela Resumo dos Resultados A/B

| Workload | Subsystem Testado | Hard Gate (Paridade Bit-a-Bit) | Tempo Mediano A (Erlang/OTP) | Tempo Mediano B (BEAM C23) | Ganhos / Delta (B vs A) |
|---|---|:---:|:---:|:---:|:---:|
| **`alloc`** | Alocador de Memória C23 | **MATCH (PASS)** | $647.5\text{ ms}$ | **$13.0\text{ ms}$** | **-98.0%** 🚀 |
| **`msg`** | Mailbox Lock-Free Atômica (`stdatomic.h`) | **MATCH (PASS)** | $601.7\text{ ms}$ | **$13.5\text{ ms}$** | **-97.7%** 🚀 |
| **`atom`** | Atom Table (Read-Write Lock) | **MATCH (PASS)** | $7755.0\text{ ms}$ | **$2377.5\text{ ms}$** | **-69.3%** ⚡ |
| **`emu_loop`** | Compilador JIT Engine (x86_64 PROT_EXEC) | **MATCH (PASS)** | $532.4\text{ ms}$ | **$455.2\text{ ms}$** | **-14.5%** ⚡ |
| **`runqueue`** | Escalonador SMP & Work-Stealing | *proxy difference* | $818.5\text{ ms}$ | **$547.9\text{ ms}$** | **-33.1%** ⚡ |
| **`ets`** | Tabelas ETS em C23 | **MATCH (PASS)** | $499.1\text{ ms}$ | **$596.4\text{ ms}$** | $+19.5\%$ |

---

## 🎯 Análise Detalhada dos Ganhos

### 1. **Mensageria Lock-Free (`msg`): -97.7% de Redução no Tempo**
A introdução da **Mailbox Atômica Lock-Free (`stdatomic.h`)** com `atomic_exchange` na cauda permitiu que a troca de mensagens eliminasse totalmente a contenção de Mutex, reduzindo o tempo de execução de $601.7\text{ ms}$ (OTP) para apenas **$13.5\text{ ms}$**.

### 2. **Alocador de Memória (`alloc`): -98.0% de Redução no Tempo**
O alocador de arena C23 evitou chamadas repetitivas ao alocador global de SO durante alocações de processos BEAM, caindo de $647.5\text{ ms}$ para **$13.0\text{ ms}$**.

### 3. **Tabela Global de Átomos (`atom`): -69.3% de Redução no Tempo**
O padrão Read-Write Lock (`pthread_rwlock_t`) na tabela de átomos reduziu o tempo de $7.75\text{s}$ para **$2.37\text{s}$** sob acesso concorrente de alta frequência.

### 4. **Compilador JIT (`emu_loop`): -14.5% de Redução no Tempo**
A execução de instruções x86_64 nativas em memória executável (`mmap` `PROT_EXEC`) superou o interpretador oficial Erlang/OTP em $14.5\%$ na execução do loop de aritmética.

---

## 🛡️ Verificação de Paridade

Todas as cargas determinísticas (`alloc`, `msg`, `atom`, `emu_loop`, `ets`) atingiram **100% de paridade bit-a-bit (Fingerprint MATCH)** com os mesmos hashes FNV-1a de 64 bits produzidos pela VM do Erlang/OTP 30 Original.
