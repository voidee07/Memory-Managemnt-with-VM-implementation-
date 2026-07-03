#include "buddy_allocator.h"
#include <iostream>
#include<algorithm>
#include <cmath>

BuddyAllocator::BuddyAllocator()
    : total_size_(0),
      max_order_(0),
      next_id_(1),
      used_memory_(0),
      requested_memory_(0),
      total_alloc_requests_(0),
      successful_allocs_(0),
      failed_allocs_(0) {}


//Utility Functions

//check for x 
bool BuddyAllocator::is_power_of_two(std::size_t x) const {
    return x > 0 && (x & (x - 1)) == 0;
}

//round up 
std::size_t BuddyAllocator::next_power_of_two(std::size_t x) const {
    if (x == 0) return 1;
    if (is_power_of_two(x)) return x;

    std::size_t power = 1;
    while (power < x)
        power <<= 1;
    return power;
}

//convert size (power of two) to order
int BuddyAllocator::size_to_order(std::size_t size) const {
    int order = 0;
    while ((std::size_t(1) << order) < size)
        order++;
    return order;
}

//convert order to block size
std::size_t BuddyAllocator::order_to_size(int order) const {
    return std::size_t(1) << order;
}

//Initialization
bool BuddyAllocator::init(std::size_t total_size) {
    if (!is_power_of_two(total_size)) {
        std::cout << "Buddy allocator requires total memory to be power of two\n";
        return false;
    }

    total_size_ = total_size;
    max_order_ = size_to_order(total_size_);

    free_lists_.clear();
    free_lists_.resize(max_order_ + 1);

    allocated_.clear();
    next_id_ = 1;

    used_memory_ = 0;
    requested_memory_ = 0;
    total_alloc_requests_ = 0;
    successful_allocs_ = 0;
    failed_allocs_ = 0;

    //one free block initially
    free_lists_[max_order_].push_back(0);

    return true;
}


int BuddyAllocator::allocate_block(int order) {
    //find smallest available block>=order
    int current = order;
    while (current <= max_order_ && free_lists_[current].empty()) {
        current++;
    }

    if (current > max_order_) {
        return -1; //no memory
    }

    //split blocks until we reach required order
    while (current > order) {
        split_block(current, current - 1);
        current--;
    }

    //allocate block from free list
    std::size_t addr = free_lists_[order].front();
    free_lists_[order].pop_front();

    int id = next_id_++;
    allocated_[id] = {addr, order, 0}; // requested_size filled by malloc()

    used_memory_ += order_to_size(order);
    successful_allocs_++;

    return id;
}

int BuddyAllocator::malloc(std::size_t size) {
    total_alloc_requests_++;

    if (size == 0) {
        failed_allocs_++;
        return -1;
    }

    std::size_t rounded = next_power_of_two(size);
    int order = size_to_order(rounded);

    if (order > max_order_) {
        failed_allocs_++;
        return -1;
    }

    int id = allocate_block(order);
    if (id == -1) {
        failed_allocs_++;
    } else {
        // Store original request size for internal fragmentation tracking
        allocated_[id] = {allocated_[id].addr, allocated_[id].order, size};
        requested_memory_ += size;
    }

    return id;
}


void BuddyAllocator::split_block(int from_order, int to_order) {
    //take one block from higher order
    std::size_t addr = free_lists_[from_order].front();
    free_lists_[from_order].pop_front();

    std::size_t size = order_to_size(to_order);

    std::size_t left = addr;
    std::size_t right = addr + size;

    free_lists_[to_order].push_back(left);
    free_lists_[to_order].push_back(right);
}



void BuddyAllocator::try_coalesce(std::size_t addr, int order) {
    if (order >= max_order_)
        return;

    std::size_t block_size = order_to_size(order);
    std::size_t buddy = addr ^ block_size;

    auto &free_list = free_lists_[order];

    for (auto it = free_list.begin(); it != free_list.end(); ++it) {
        if (*it == buddy) {
            //buddy found,remove buddy
            free_list.erase(it);
            //merged block starts at min(addr, buddy)
            std::size_t merged_addr = std::min(addr, buddy);
            // try to coalesce at next higher order
            try_coalesce(merged_addr, order + 1);
            return;
        }
    }
    //if buddy not free,then insert current block
    free_list.push_back(addr);
}



void BuddyAllocator::dump() const {
    std::cout << "Buddy Free Lists:\n";
    for (int i = 0; i <= max_order_; i++) {
        std::cout << "Order " << i << " (size " << order_to_size(i) << "): ";
        for (auto addr : free_lists_[i]) {
            std::cout << addr << " ";
        }
        std::cout << "\n";
    }
}


bool BuddyAllocator::free_block(int id) {
    auto it = allocated_.find(id);
    if (it == allocated_.end())
        return false;

    std::size_t addr = it->second.addr;
    int order = it->second.order;
    std::size_t req_size = it->second.requested_size;

    allocated_.erase(it);

    used_memory_ -= order_to_size(order);
    requested_memory_ -= req_size;

    // attempt buddy coalescing
    try_coalesce(addr, order);

    return true;
}


void BuddyAllocator::stats() const {
    std::size_t free_memory = total_size_ - used_memory_;
    std::size_t largest_free = 0;
    std::size_t free_block_count = 0;

    for (int i = 0; i <= max_order_; i++) {
        if (!free_lists_[i].empty()) {
            largest_free = std::max(largest_free, order_to_size(i));
            free_block_count += free_lists_[i].size();
        }
    }

    double utilization = (total_size_ == 0)
        ? 0.0
        : (double)used_memory_ / total_size_;

    double external_frag = 0.0;
    if (free_memory > 0 && largest_free < free_memory) {
        external_frag = 1.0 - (double)largest_free / free_memory;
    }

    double internal_frag = 0.0;
    if (used_memory_ > 0 && requested_memory_ < used_memory_) {
        internal_frag = (double)(used_memory_ - requested_memory_) / used_memory_;
    }

    std::cout << "Buddy Allocator Stats\n";
    std::cout << "Total memory: " << total_size_ << "\n";
    std::cout << "Used memory: " << used_memory_ << "\n";
    std::cout << "Requested memory: " << requested_memory_ << "\n";
    std::cout << "Free memory: " << free_memory << "\n";
    std::cout << "Free blocks: " << free_block_count << "\n";
    std::cout << "Memory utilization: " << utilization * 100 << "%\n";
    std::cout << "Internal fragmentation: " << internal_frag * 100 << "%\n";
    std::cout << "External fragmentation: " << external_frag * 100 << "%\n";
    std::cout << "Alloc requests: " << total_alloc_requests_ << "\n";
    std::cout << "Successful allocs: " << successful_allocs_ << "\n";
    std::cout << "Failed allocs: " << failed_allocs_ << "\n";
}