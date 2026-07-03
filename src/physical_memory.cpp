#include "physical_memory.h"
#include <iostream>
#include <algorithm>

PhysicalMemory::PhysicalMemory()
    : used_memory_(0),
      total_alloc_requests_(0),
      successful_allocs_(0),
      failed_allocs_(0),
      total_size_(0),
      blocks_(),
      next_id_(1),
      allocator_(AllocatorType::FIRST_FIT) {}


void PhysicalMemory::init(std::size_t total_size) {
    total_size_ = total_size;
    blocks_.clear();

    used_memory_ = 0;
    total_alloc_requests_ = 0;
    successful_allocs_ = 0;
    failed_allocs_ = 0;
    next_id_ = 1;
    allocator_ = AllocatorType::FIRST_FIT;

    blocks_.emplace_back(0, total_size, true, -1);
}

void PhysicalMemory::dump() const {
    for (const auto &blk : blocks_) {
        std::size_t end = blk.start + blk.size - 1;
        if (blk.free) {
            std::cout << "[" << blk.start << " - " << end << "] FREE\n";
        } else {
            std::cout << "[" << blk.start << " - " << end
                      << "] USED (id=" << blk.id << ")\n";
        }
    }
}

int PhysicalMemory::malloc(std::size_t size) {
    total_alloc_requests_++;

    int id = -1;
    switch (allocator_) {
        case AllocatorType::FIRST_FIT:
            id = malloc_first_fit(size);
            break;
        case AllocatorType::BEST_FIT:
            id = malloc_best_fit(size);
            break;
        case AllocatorType::WORST_FIT:
            id = malloc_worst_fit(size);
            break;
    }

    if (id == -1)
        failed_allocs_++;
    else
        successful_allocs_++;

    return id;
}

void PhysicalMemory::set_allocator(AllocatorType type) {
    allocator_ = type;
}

int PhysicalMemory::malloc_first_fit(std::size_t size) {
    for (auto it = blocks_.begin(); it != blocks_.end(); ++it) {
        if (it->free && it->size >= size) {
            return allocate_from_block(it, size);
        }
    }
    return -1;
}

int PhysicalMemory::malloc_best_fit(std::size_t size) {
    auto best = blocks_.end();

    for (auto it = blocks_.begin(); it != blocks_.end(); ++it) {
        if (it->free && it->size >= size) {
            if (best == blocks_.end() || it->size < best->size)
                best = it;
        }
    }

    return (best != blocks_.end()) ? allocate_from_block(best, size) : -1;
}

int PhysicalMemory::malloc_worst_fit(std::size_t size) {
    auto worst = blocks_.end();

    for (auto it = blocks_.begin(); it != blocks_.end(); ++it) {
        if (it->free && it->size >= size) {
            if (worst == blocks_.end() || it->size > worst->size)
                worst = it;
        }
    }

    return (worst != blocks_.end()) ? allocate_from_block(worst, size) : -1;
}

int PhysicalMemory::allocate_from_block(std::list<Block>::iterator it, std::size_t size) {
    int id = next_id_++;

    // exact fit
    if (it->size == size) {
        it->free = false;
        it->id = id;
        used_memory_ += size; 
        return id;
    }

    std::size_t remaining_size = it->size - size;
    std::size_t new_start = it->start + size;

    Block allocated(it->start, size, false, id);
    Block remaining(new_start, remaining_size, true, -1);

    auto next_it = blocks_.erase(it);
    blocks_.insert(next_it, allocated);
    blocks_.insert(next_it, remaining);

    used_memory_ += size;
    return id;
}

bool PhysicalMemory::free_block(int id) {
    for (auto it = blocks_.begin(); it != blocks_.end(); ++it) {
        if (!it->free && it->id == id) {
            it->free = true;
            it->id = -1;
            used_memory_ -= it->size;

            if (it != blocks_.begin()) {
                auto prev = std::prev(it);
                if (prev->free) {
                    prev->size += it->size;
                    it = blocks_.erase(it);
                    it = prev;
                }
            }

            auto next = std::next(it);
            if (next != blocks_.end() && next->free) {
                it->size += next->size;
                blocks_.erase(next);
            }
            return true;
        }
    }
    return false;
}

void PhysicalMemory::stats() const {
    if (total_size_ == 0) {
        std::cout << "Memory not initialized\n";
        return;
    }

    std::size_t free_memory = total_size_ - used_memory_;
    std::size_t largest_free = 0;
    std::size_t free_block_count = 0;

    for (const auto &blk : blocks_) {
        if (blk.free) {
            largest_free = std::max(largest_free, blk.size);
            free_block_count++;
        }
    }

    double external_frag = 0.0;
    if (free_memory > 0 && largest_free < free_memory) {
        external_frag = 1.0 - (double)largest_free / free_memory;
    }

    double utilization = (double)used_memory_ / total_size_;

    std::cout << "Total memory: " << total_size_ << "\n";
    std::cout << "Used memory: " << used_memory_ << "\n";
    std::cout << "Free memory: " << free_memory << "\n";
    std::cout << "Free blocks: " << free_block_count << "\n";
    std::cout << "Memory utilization: " << utilization * 100 << "%\n";
    std::cout << "External fragmentation: " << external_frag * 100 << "%\n";
    std::cout << "Alloc requests: " << total_alloc_requests_ << "\n";
    std::cout << "Successful allocs: " << successful_allocs_ << "\n";
    std::cout << "Failed allocs: " << failed_allocs_ << "\n";
}