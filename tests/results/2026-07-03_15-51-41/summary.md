# Benchmark Results - 2026-07-03_15-51-41

## Allocator Comparison

| Workload | Strategy | Utilization% | ExtFrag% | IntFrag% | FreeBlocks | Allocs | Success | Failed |
|----------|----------|-------------|----------|----------|------------|--------|---------|--------|
| workload_sequential_uniform | first_fit | 25.1953 | 18.799 | N/A | 10 | 23 | 23 | 0 |
| workload_sequential_uniform | best_fit | 25.1953 | 18.799 | N/A | 10 | 23 | 23 | 0 |
| workload_sequential_uniform | worst_fit | 25.1953 | 20.8877 | N/A | 11 | 23 | 23 | 0 |
| workload_sequential_uniform | buddy | 26.5625 | 31.9149 | 5.14706 | 12 | 23 | 23 | 0 |
| workload_random_mixed | first_fit | 66.5894 | 61.7099 | N/A | 11 | 45 | 44 | 1 |
| workload_random_mixed | best_fit | 66.5894 | 47.3877 | N/A | 13 | 45 | 44 | 1 |
| workload_random_mixed | worst_fit | 54.2358 | 55.6682 | N/A | 12 | 45 | 42 | 3 |
| workload_random_mixed | buddy | 64.8438 | 64.4444 | 13.7801 | 9 | 45 | 43 | 2 |
| workload_buddy_vs_standard | first_fit | 0 | 0 | N/A | 1 | 15 | 15 | 0 |
| workload_buddy_vs_standard | best_fit | 0 | 0 | N/A | 1 | 15 | 15 | 0 |
| workload_buddy_vs_standard | worst_fit | 0 | 0 | N/A | 1 | 15 | 14 | 1 |
| workload_buddy_vs_standard | buddy | 0 | 100 | 0 | 0 | 15 | 14 | 1 |

## Cache/VM Locality Results

| Cache | Hits | Misses | Hit Rate% |
|-------|------|--------|-----------|
| L1 | 45 | 3 | 93.75 |
| L2 | 0 | 3 | 0 |
| L1 | 48 | 64 | 42.8571 |
| L2 | 0 | 64 | 0 |

### Virtual Memory (final)
- Page hits: 108
- Page faults: 4
- Page evictions: 0

