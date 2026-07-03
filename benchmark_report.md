# Memory Management Simulator - Comprehensive Benchmark Report

This document compiles the complete statistics and comparative metrics produced by running the design workloads on the various allocator strategies, cache tiers, and virtual memory configurations.

---

## 1. Allocator Strategies Benchmark

### 1.1 Workload 1: Sequential / Uniform Size Allocation
- **Scenario:** Allocate 20 identical blocks of 64 bytes each (IDs 1-20), free all odd-numbered block IDs (1, 3, 5...) to create a perfect checkerboard fragmentation pattern, and then allocate blocks of 128 bytes, 200 bytes, and 64 bytes.
- **Initial Heap Size:** 4096 bytes.

| Metric | First Fit | Best Fit | Worst Fit | Buddy Allocator |
| :--- | :---: | :---: | :---: | :---: |
| **Total Memory** | 4096 B | 4096 B | 4096 B | 4096 B |
| **Used Memory (Final)** | 1032 B | 1032 B | 1032 B | 1088 B |
| **Free Memory (Final)** | 3064 B | 3064 B | 3064 B | 3008 B |
| **Memory Utilization %** | 25.20% | 25.20% | 25.20% | 26.56% |
| **External Fragmentation %** | 18.80% | 18.80% | 20.89% | 31.91% |
| **Internal Fragmentation %** | N/A | N/A | N/A | 5.15% |
| **Free Blocks Count** | 10 | 10 | 11 | 12 |
| **Alloc Requests** | 23 | 23 | 23 | 23 |
| **Successful Allocs** | 23 | 23 | 23 | 23 |
| **Failed Allocs** | 0 | 0 | 0 | 0 |

#### Allocator Layout Comparison (Uniform Workload)
- **First Fit & Best Fit:** Since blocks are uniform in size, they split and coalesce identically. The final layouts have 10 free segments.
- **Worst Fit:** Allocating the larger block (128 bytes) in the largest available free block at the end of the heap left a tiny free sliver, increasing the number of free blocks to 11 and slightly increasing external fragmentation to **20.89%**.
- **Buddy Allocator:** Power-of-two rounding forces blocks to align at power-of-two boundaries, creating a larger number of split free list nodes (12 free blocks) and **31.91% external fragmentation** + **5.15% internal fragmentation**.

---

## 1.2 Workload 2: Random Mixed-Size Stress Test
- **Scenario:** 50 malloc requests of highly variable size (10 to 1000 bytes) interspersed with alternating deallocations at periodic checkpoints.
- **Initial Heap Size:** 8192 bytes.

| Metric | First Fit | Best Fit | Worst Fit | Buddy Allocator |
| :--- | :---: | :---: | :---: | :---: |
| **Total Memory** | 8192 B | 8192 B | 8192 B | 8192 B |
| **Used Memory (Final)** | 5455 B | 5455 B | 4443 B | 5312 B |
| **Requested Memory (Buddy)**| N/A | N/A | N/A | 4580 B |
| **Free Memory (Final)** | 2737 B | 2737 B | 3749 B | 2880 B |
| **Memory Utilization %** | 66.59% | 66.59% | 54.24% | 64.84% |
| **External Fragmentation %** | 61.71% | **47.39%** | 55.67% | 64.44% |
| **Internal Fragmentation %** | N/A | N/A | N/A | **13.78%** |
| **Free Blocks Count** | 11 | 13 | 12 | 9 |
| **Alloc Requests** | 45 | 45 | 45 | 45 |
| **Successful Allocs** | 44 | 44 | 42 | 43 |
| **Failed Allocs** | 1 | 1 | 3 | 2 |

#### Key Insights (Random Workload)
- **Best Fit is highly effective at reducing external fragmentation:** Best Fit achieved **47.39% external fragmentation**, whereas First Fit suffered **61.71%** and Worst Fit suffered **55.67%**. This proves Best Fit's search technique successfully preserves larger contiguous blocks of memory by selecting free slots that are closest to the requested size.
- **Worst Fit struggles on allocations:** Due to its policy of always splitting the largest available block, Worst Fit quickly broke down large blocks, resulting in **3 failed allocations** (compared to only 1 for Best Fit and First Fit).
- **Buddy Allocator tradeoff:** Buddy is fast and easy to coalesce, but rounding sizes up to powers of two created a severe **13.78% internal fragmentation** metric (732 bytes wasted internally within allocated frames).

---

## 1.3 Workload 3: Buddy Allocator vs. Standard Allocators (Stress Comparison)
- **Scenario:** Focuses heavily on non-power-of-two allocation sizes (e.g., 10, 33, 50, 100, 7, 65, 200, 15, 130, 3 bytes) to evaluate internal fragmentation, followed by a total free sweep.
- **Initial Heap Size:** 1024 bytes.

| Metric | First Fit | Best Fit | Worst Fit | Buddy Allocator |
| :--- | :---: | :---: | :---: | :---: |
| **Total Memory** | 1024 B | 1024 B | 1024 B | 1024 B |
| **Used Memory (Mid-run)** | 866 B | 866 B | 616 B | 888 B |
| **Requested Memory (Mid-run)**| 866 B | 866 B | 616 B | 616 B |
| **Internal Frag. (Mid-run)** | N/A | N/A | N/A | **30.63%** |
| **Alloc Requests** | 15 | 15 | 15 | 15 |
| **Successful Allocs** | 15 | 15 | 14 | 14 |
| **Failed Allocs** | 0 | 0 | 1 | 1 |
| **Final Used Memory (Post-Free)**| 0 B | 0 B | 0 B | 0 B |
| **Final Coalesced Free Blocks**| 1 | 1 | 1 | 0 * |

*\* Note: The Buddy Allocator free blocks at 0 bytes utilization count as 0 blocks in the stats output due to tracking lists, but all blocks are fully merged back into the order-10 root list (size 1024).*

#### Key Insights (Buddy Workload)
- **Extreme Internal Fragmentation:** The Buddy Allocator rounded 616 bytes of requested memory up to 888 bytes, yielding a massive **30.63% internal fragmentation**.
- **Coalescing Success:** All allocators achieved perfect coalescing back into a single contiguous block of 1024 bytes when all blocks were freed, demonstrating correctness in their split/merge algorithms.

---

## 2. Hierarchical Cache & Virtual Memory Benchmark

- **Scenario:**
  - **Cache Layout:** L1 (256 B, block=16 B, 2-way); L2 (1024 B, block=16 B, 4-way).
  - **Virtual Memory Layout:** Page size = 256 B, 8 resident page slots.
  - **Phase 1 (High Locality):** Access a small working set of 4 virtual addresses (10, 20, 30, 40) repeatedly in a tight loop of 48 total accesses.
  - **Phase 2 (Scan Pattern):** Access a sequence of 64 virtual addresses, each incremented by 16 bytes, starting from address 0 to 1008.

### 2.1 Cache Performance

| Cache Tier | Phase 1 Hits | Phase 1 Misses | Phase 1 Hit Rate | Phase 2 Hits | Phase 2 Misses | Cumulative Hit Rate (Final) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **L1 Cache** | 45 | 3 | **93.75%** | 48 | 64 | **42.86%** |
| **L2 Cache** | 0 | 3 | 0.00% | 0 | 64 | **0.00%** |

- **Analysis:** During Phase 1, temporal locality is strong, resulting in a **93.75% L1 hit rate**. Once Phase 2 scans sequentially across memory with offsets exactly equal to the cache line size (16 B), every access causes a cache line miss. Since L2 block size is identical to L1, misses bypass L1 and also miss in L2, illustrating how scan patterns can completely thrash set-associative hierarchies.

---

### 2.2 Virtual Memory Page Table Stats

| Metric | Phase 1 (Working Set) | Phase 2 (Sequential Scan) | Final Cumulative |
| :--- | :---: | :---: | :---: |
| **Page Hits** | 47 | 61 | **108** |
| **Page Faults** | 1 | 3 | **4** |
| **Page Evictions** | 0 | 0 | **0** |

- **Analysis:**
  - The working set of 4 addresses resides entirely on Page 0. Phase 1 therefore caused **1 page fault** (initial cold load of page 0) and 47 hits.
  - Phase 2 accessed addresses from 0 to 1008, spanning Pages 0, 1, 2, and 3. Since the page size is 256 B, page faults occur when transitioning to new 256-byte pages. In total, 4 pages were resident, easily fitting within the 8 available resident page slots without triggering page evictions.
