#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <list>
#include <unordered_map>
#include <utility>
#include <cstddef>
#include <optional>

// A small, header-only, generic LRU cache.
// - Not thread-safe.
// - Provides get/put/clear/size/capacity.
// - Keys must be hashable and equality comparable.
// - Values are copied in/out.
template<typename Key, typename Value, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
class LruCache {
public:
    explicit LruCache(std::size_t capacity)
        : capacity_(capacity) {}

    void clear() {
        order_.clear();
        map_.clear();
    }

    std::size_t size() const { return map_.size(); }
    std::size_t capacity() const { return capacity_; }
    void set_capacity(std::size_t c) {
        capacity_ = c;
        evict_if_needed();
    }

    bool get(const Key& key, Value& out_value) {
        auto it = map_.find(key);
        if (it == map_.end()) return false;
        order_.splice(order_.begin(), order_, it->second);
        out_value = it->second->second;
        return true;
    }

    void put(const Key& key, const Value& value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = value;
            order_.splice(order_.begin(), order_, it->second);
            return;
        }
        order_.emplace_front(key, value);
        map_[key] = order_.begin();
        evict_if_needed();
    }

    bool contains(const Key& key) const {
        return map_.find(key) != map_.end();
    }

private:
    void evict_if_needed() {
        while (capacity_ > 0 && map_.size() > capacity_) {
            auto last_it = std::prev(order_.end());
            map_.erase(last_it->first);
            order_.pop_back();
        }
        if (capacity_ == 0) {
            clear();
        }
    }

    std::size_t capacity_;
    std::list<std::pair<Key, Value>> order_;
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator, Hasher, KeyEq> map_;
};

#endif // LRU_CACHE_H


