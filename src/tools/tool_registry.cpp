// src/tools/tool_registry.cpp — YOU IMPLEMENT (spec §4.3).
//
// A hand-written hash table. No std::unordered_map / std::map / std::set.
// Every function below is a stub so the starter compiles. Replace each body.

#include "tools/tool_registry.h"
#include <utility>

ToolRegistry::ToolRegistry() = default;    // TODO: create your initial buckets
ToolRegistry::~ToolRegistry() = default;   // unique_ptrs in your buckets free the tools

std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    
    // TODO: FNV-1a (spec Appendix A) or another real string hash
    std::uint64_t h= 14695981039346656037UL; //FNV-1A offset

    for (unsigned char c:key){
        h ^= c;
        h *= 1099511628211ULL;//FNC Prime
    }
    return static_cast<std::size_t>(h);
}

bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    if (tool == nullptr){// check for null 
        return false;
    }
    const std::string name = tool->name();// cpy tools name
    
    if (find(name)!= nullptr){ //check for duplicates
        return false;
    }

    if (static_cast<double>(count_+1)> kMaxLoadFactor * static_cast<double>(buckets_.size())){//check load factor and resize accordingly
        rehash(buckets_.size() * 2);
    }

    const std::size_t b = index_for(name);
    buckets_[b].push_back(std::move(tool));//hand over ownership of the tool
    ++count_;

    return true;   // TODO
}

Tool* ToolRegistry::find(std::string_view /*name*/) const {
    return nullptr;   // TODO
}

std::size_t ToolRegistry::size() const noexcept {
    return count_;
}

void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    // TODO
    for (const std::vector<std::unique_ptr<Tool>>& chain : buckets_){
        for (const std::unique_ptr<Tool>& entry : chain) {
          fn(*entry);  
        }
    }
}

std::size_t ToolRegistry::bucket_count() const noexcept {
    return buckets_.size();   // TOO
}

double ToolRegistry::load_factor() const noexcept {
    return static_cast<double>(count_) / static_cast<double>(buckets_.size());
}

std::size_t ToolRegistry::max_probe_length() const {
    std::size_t longest = 0;

    for (const std::vector<std::unique_ptr<Tool>>& chain : buckets_) {
        if (chain.size()>longest){
            longest = chain.size();
        }
    } 
    return longest;   // TODO
}

std::size_t ToolRegistry::index_for(std::string_view key) const noexcept {
    // TODO
    return hash(key) & (buckets_.size()-1);
}

void ToolRegistry::rehash(std::size_t new_bucket_count) {

    std::vector<std::vector<std::unique_ptr<Tool>>> new_buckets(new_bucket_count);// make new empty buckets

    // Walk every bucket of the OLD table, every tool is a entry.
    for (std::vector<std::unique_ptr<Tool>>& chain : buckets_) {// Move the unique_ptr into its new bucket. This copies only the address
        for (std::unique_ptr<Tool>& entry : chain) {
        const std::size_t i = hash(entry->name()) &(new_bucket_count- 1);
 
            
        new_buckets[i].push_back(std::move(entry));
        }

    }
}

