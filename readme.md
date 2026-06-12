# Design and Implementation of a Memory Management System

## Overview

This project is a user-space simulation of an operating system's memory management subsystem.

It demonstrates how physical memory allocation, caching, and virtual memory interact in a modern operating system. The simulator is designed for conceptual correctness and clarity rather than cycle-accurate hardware modeling.

All components are implemented in C++ and are controlled through a command-line interface (CLI).

## Features Implemented

### 1. Physical Memory Simulation

- Models a contiguous block of physical memory
- Memory is divided into variable-sized blocks
- Supports block splitting on allocation
- Supports block coalescing on deallocation

### 2. Memory Allocation Strategies

Supported allocation policies:

- First Fit
- Best Fit
- Worst Fit

The allocator strategy can be switched dynamically at runtime through the CLI.

### 3. Allocation Interface

Supported commands:

| Command | Description |
|----------|-------------|
| `malloc <size>` | Allocate memory |
| `free <block_id>` | Deallocate memory |
| `dump` | Display memory layout |
| `stats` | Display allocation statistics |

### 4. Metrics & Statistics

The simulator tracks:

- Total memory
- Used memory
- Free memory
- Memory utilization
- External fragmentation
- Allocation requests
- Successful allocations
- Failed allocations

> Note: Internal fragmentation is not explicitly tracked, as clarified to be optional.

### 5. Buddy Allocation System (Optional Extension)

Features:

- Power-of-two memory allocation
- Allocation requests rounded up to the nearest power of two
- Recursive block splitting
- Buddy coalescing on deallocation
- Implemented as a separate allocator for comparison

### 6. Multilevel Cache Simulation

The simulator implements two cache levels:

- L1 Cache
- L2 Cache

Each cache is:

- Set-associative
- Configurable in size
- Configurable in block size
- Configurable in associativity

Additional features:

- FIFO replacement policy
- Tag / Index / Offset based addressing
- Real cache behavior
- Cache hit and miss tracking
- Explicit miss propagation across cache hierarchy

The cache models lookup, replacement, and miss propagation behavior but does not simulate cycle-accurate access latency.

### 7. Virtual Memory Simulation (Optional Extension)

Features:

- Paging-based virtual memory system
- Single-process address space
- FIFO page replacement
- Page fault handling and page eviction
- Disk access simulated representationally
- Integrated with cache and physical memory pipeline

### 8. Integration Between Components

Memory access follows the pipeline:

```text
Virtual Address
      ↓
Page Table
      ↓
Physical Address
      ↓
L1 Cache
      ↓
L2 Cache
      ↓
Main Memory
```

## Build Instructions

### Requirements

- C++17 compatible compiler (`g++` or `clang++`)
- make

### Build and Run

```bash
make clean
make
./memsim
```

## CLI Usage Examples

### Physical Memory

```bash
init memory 1024
set allocator first_fit

malloc 100
malloc 200

free 1

dump
stats
```

### Buddy Allocator

```bash
init memory 512
set allocator buddy

malloc 60

free 1

dump
```

### Cache Simulation

```bash
cache init L1 64 8 2
cache init L2 128 8 2

access 26
access 27

cache dump
cache stats
```

### Virtual Memory

```bash
vm init 16 8

vaccess 32
vaccess 35

vm stats
```

## Assumptions and Simplifications

The following design choices were made intentionally:

- The simulator runs entirely in user space
- Physical memory contents are not stored, only metadata
- Disk access is representational (no real I/O)
- Cache write policies are not simulated
- No TLB simulation
- No multi-process virtual address spaces
- No cycle-accurate timing
- Internal fragmentation is not explicitly computed

These simplifications align with the project scope and official clarifications.

## Project Structure

```text
Design-and-Implementation-of-a-Memory-Management-System/

README.md
design.md
include/
src/
Makefile
```

## Documentation

- **README.md** — Overview, features, build instructions, and usage
- **design.md** — Detailed design decisions, assumptions, and architecture

## Test Artifacts

The simulator includes manual CLI test workloads demonstrating:

- Allocation patterns
- Memory fragmentation behavior
- Buddy allocation operations
- Cache hit and miss scenarios
- Virtual memory accesses
- Page fault handling

Expected behaviors are described in the documentation and demonstrated during project testing.