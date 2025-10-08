#pragma once

#include <cstddef>
#include <forward_list>
#include <utility>
#include <vector>

template <typename K, typename V> class HashMap {
public:
  // Entry stores a const key and a mutable value
  struct Entry {
    K key_;
    V value_;
    Entry(K key, V value) : key_(key), value_(value) {}
  };

  using Bucket = std::forward_list<Entry>;
  using Table = std::vector<Bucket>;

  // Construct with a number of buckets (must be >= 1)
  HashMap(std::size_t nbuckets = 1024) { buckets_ = Table(nbuckets); }

  // Return pointer to value associated with key, or nullptr if not found.
  // Only iterate the appropriate bucket.
  V *get(const K &key) {
    std::size_t index = std::hash<K>{}(key) % buckets_.size();
    for (Entry &entry : buckets_[index]) {
      if (entry.key_ == key) {
        return &(entry.value_);
      }
    }
    return nullptr;
  }

  // Insert or update (key,value).
  // Returns true if an existing entry was updated, false if a new entry was
  // inserted.
  bool put(const K &key, const V &value) {
    if (count_ >= buckets_.size() * 2) {
      resize(buckets_.size() * 2);
    }
    std::size_t index = std::hash<K>{}(key) % buckets_.size();
    for (Entry &entry : buckets_[index]) {
      if (entry.key_ == key) {
        entry.value_ = value;
        return true;
      }
    }
    buckets_[index].push_front(Entry(key, value));
    count_ += 1;
    return false;
  }

  // Current number of stored entries
  std::size_t size() const { return count_; }

  // Convert table contents to a vector of key/value pairs.
  std::vector<std::pair<K, V>> toKeyValuePairs() const {
    std::vector<std::pair<K, V>> acc;
    for (const Bucket &b : buckets_) {
      for (const Entry &e : b) {
        acc.push_back({e.key_, e.value_});
      }
    }
    return acc;
  }

  // Optional: number of buckets
  // std::size_t bucket_count() const;

private:
  Table buckets_;
  std::size_t count_ = 0;

  void resize(size_t n) {
    Table new_buckets(n);
    for (const Bucket &bucket : buckets_) {
      for (const Entry &entry : bucket) {
        std::size_t new_index = std::hash<K>{}(entry.key_) % n;
        new_buckets[new_index].push_front(entry);
      }
    }
    buckets_ = std::move(new_buckets);
  }
};
