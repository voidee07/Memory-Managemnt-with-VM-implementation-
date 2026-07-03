#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include <vector>
#include <list>
#include <unordered_map>
#include <cstddef>



class BuddyAllocator {
public:
    BuddyAllocator();

    //initialize with total memory size (must be power of two)
    bool init(std::size_t total_size);
    //allocate memory (rounded to nearest power of two)
    int malloc(std::size_t size);
    //free previously allocated block
    bool free_block(int id);
    //dump free lists and allocated blocks
    void dump() const;
    //statistics
    void stats() const;

private:
    //helper utilities
    bool is_power_of_two(std::size_t x) const;
    std::size_t next_power_of_two(std::size_t x) const;
    int size_to_order(std::size_t size) const;
    std::size_t order_to_size(int order) const;

    //allocation helpers
    int allocate_block(int order);
    void split_block(int from_order, int to_order);
    void try_coalesce(std::size_t addr, int order);

private:
    //memory configuration
    std::size_t total_size_;
    int max_order_;
    //info about an allocated block
    struct AllocInfo {
        std::size_t addr;
        int order;
        std::size_t requested_size;  // original size before power-of-two rounding
    };

    //free lists: free_lists[k] = list of free blocks of size 2^k
    std::vector<std::list<std::size_t>> free_lists_;
    //allocated blocks: id -> AllocInfo
    std::unordered_map<int, AllocInfo> allocated_;
    //bookkeeping
    int next_id_;
    //statistics
    std::size_t used_memory_;
    std::size_t requested_memory_;  // sum of original request sizes (pre-rounding)
    std::size_t total_alloc_requests_;
    std::size_t successful_allocs_;
    std::size_t failed_allocs_;
};

#endif