#pragma once

#include "hashtable.h"

namespace kola {

template <typename Key,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<Key>>
class unordered_set {
public:
  using key_type = hashtable::key_type;
  using value_type = hashtable::value_type;
  using size_type = hashtable::size_type;
  using difference_type = hashtable::difference_type;

private:
  template <typename T>
  struct identity {
    const T& operator()(const T& x) const { return x; }
  };

  using hashtable = kola::hashtable<Key, Key, Hash, identity<Key>, KeyEqual, Allocator>;

  hashtable ht_;
};

}
