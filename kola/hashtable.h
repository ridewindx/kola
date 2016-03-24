#pragma once

#include <vector>
#include <functional>

namespace kola {

namespace detail {

std::size_t next_prime(std::size_t n) const noexcept;

std::size_t max_size() const noexcept;

}

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc = std::allocator>
class hashtable {
public:
  using key_type = Key;
  using value_type = Value;
  using allocator_type = Alloc;
  using key_equal = Equal;
  using hash = Hash;
  using extract_key = ExtractKey;

  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;

  struct iterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = hashtable::value_type;
    using size_type = hashtable::size_type;
    using difference_type = hashtable::difference_type;
    using reference = hashtable::reference;
    using pointer = hashtable::pointer;

    iterator(node* cur, hashtable* ht) : cur_(cur), ht_(ht) {}

    reference operator*() const { return cur_->val; }
    pointer operator->() const { return &(operator*()); }

    iterator& operator++();
    iterator operator++(int);

    bool operator==(const iterator& it) const { return cur_ == it.cur_; }
    bool operator!=(const iterator& it) const { return cur_ != it.cur_; }

  private:
    node* cur_;
    hashtable* ht_;
  };

  iterator find(const key_type& key);

  size_type count(const key_type& key) const;

  size_type size() const noexcept { return num_elements_; }

  size_type bucket_count() const noexcept { return buckets_.size(); }

  size_type max_bucket_count() const noexcept { return detail::max_size(); }

  void resize(size_type num_elements_hint);

  std::pair<iterator, bool> insert_unique(const value_type& obj) {
    resize(num_elements_ + 1);
    return insert_unique_noresize(obj);
  };

  std::pair<iterator, bool> insert_unique_noresize(const value_type& obj);

  iterator insert_equal(const value_type& obj) {
    resize(num_elements_ + 1);
    return insert_equal_noresize(obj);
  }

  iterator insert_equal_noresize(const value_type& obj);

  void clear();

  void copy_from(const hashtable& ht);

private:
  size_type next_size(size_type n) const noexcept { return detail::next_prime(n); }

  void initialize_buckets(size_type n) {
    const size_type n_buckets = next_size(n);
    buckets_.reserve(n_buckets);
    buckets_.insert(buckets_.end(), n_buckets, nullptr);
    num_elements_ = 0;
  }

  size_type bkt_num(const value_type& obj, size_type n) const noexcept {
    return bkt_num_key(extract_key(obj), n);
  }

  size_type bkt_num(const value_type& obj) const noexcept {
    return bkt_num_key(extract_key(obj));
  }

  size_type bkt_num_key(const key_type& key) const noexcept {
    return bkt_num_key(key, buckets_.size());
  }

  size_type bkt_num_key(const key_type& key, size_type n) const noexcept {
    return hash(key) % n;
  }

  node* new_node(const value_type& obj) {
    node* n = allocator_type<node>()::allocate(1);
    n->next = nullptr;
    try {
      allocator_type<node>()::construct(&n->val, obj);
      return n;
    } catch (...) {
      allocator_type<node>()::deallocate(n, 1);
      throw;
    }
  }

  void delete_node(node* n) {
    allocator_type<node>()::destroy(&n->val);
    allocator_type<node>()::deallocate(n, 1);
  }

  struct node {
    node* next;
    value_type val;
  };

  std::vector<node*, Alloc> buckets_;
  size_type num_elements_;
};

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc>
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::iterator
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::find(const key_type &key) {
  size_type bucket = bkt_num_key(key);
  node* cur = buckets_[bucket];
  while (cur && !key_equal(extract_key(cur->val), key))
    cur = cur->next;
  return iterator(cur, this);
};

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc>
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::size_type
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::count(const key_type& key) const {
  size_type bucket = bkt_num_key(key);
  size_type num = 0;
  for (node* cur = buckets_[bucket]; cur; cur = cur->next) {
    if (key_equal(extract_key(cur->val), key))
      ++num;
  }
  return num;
};

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc>
void hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::resize(size_type num_elements_hint) {
  const size_type old_num = buckets_.size();
  if (num_elements_hint <= old_num)
    return;

  const size_type n = next_size(num_elements_hint);
  if (n <= old_num)
    return;

  std::vector<node*, allocator_type> new_buckets(n, nullptr);

  for (size_type bucket = 0; bucket < old_num; ++bucket) {
    node* first = buckets_[bucket];
    while (first) {
      size_type new_bucket = bkt_num(first->val, n);
      buckets_[buckets_] = first->next;
      first->next = new_buckets[new_bucket];
      new_buckets[new_bucket] = first;
      first = buckets_[bucket];
    }
  }

  buckets_.swap(new_buckets);
};

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc>
std::pair<hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::iterator, bool>
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::insert_unique_noresize(const value_type& obj) {
  const size_type bucket = bkt_num(obj);
  node* first = buckets_[bucket];

  for (node* cur = first; cur; cur = cur->next) {
    if (key_equal(extract_key(cur->val), extract_key(obj)))
      return std::pair<iterator, bool>(iterator(cur, this), false);
  }

  node* tmp = new_node(obj);
  tmp->next = first;
  buckets_[bucket] = tmp;
  ++num_elements_;
  return std::pair<iterator, bool>(iterator(tmp, this), true);
};

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc>
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::iterator
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::insert_equal_noresize(const value_type& obj) {
  const size_type bucket = bkt_num(obj);
  node* first = buckets_[bucket];

  for (node* cur = first; cur; cur = cur->next) {
    if (key_equal(extract_key(cur->val), extract_key(obj))) {
      node* tmp = new_node(obj);
      tmp->next = cur->next;
      cur->next = tmp;
      ++num_elements_;
      return iterator(tmp, this);
    }
  }

  node* tmp = new_node(obj);
  tmp->next = first;
  buckets_[bucket] = tmp;
  ++num_elements_;
  return iterator(tmp, this);
};

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc>
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::iterator&
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::iterator::operator++() {
  const node* old = cur_;
  cur_ = cur_->next;
  if (!cur_) {
    size_type bucket = ht_->bkt_num(old->val);
    while (!cur_ && ++bucket < ht_->buckets_.size())
      cur_ = ht_->buckets_[bucket];
  }
  return *this;
}

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc>
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::iterator
hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::iterator::operator++(int) {
  iterator old_it = *this;
  ++*this;
  return old_it;
}

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc>
void hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::clear() {
  for (size_type i = 0; i < buckets_.size(); ++i) {
    node* cur = buckets_[i];
    while (cur) {
      node* next = cur->next;
      delete_node(cur);
      cur = next;
    }
    buckets_[i] = nullptr;
  }

  num_elements_ = 0;
};

template<typename Key, typename Value, typename Hash, typename ExtractKey, typename Equal, typename Alloc>
void hashtable<Key, Value, Hash, ExtractKey, Equal, Alloc>::copy_from(const hashtable& ht) {
  buckets_.clear();
  buckets_.reserve(ht.buckets_.size());
  buckets_.insert(buckets_.end(), ht.buckets_.size(), nullptr);
  try {
    for (size_type i = 0; i < ht.buckets_.size(); ++i) {
      if (node* cur = ht.buckets_[i]) {
        node* copy = new_node(cur->val);
        buckets_[i] = copy;

        for (node* next = cur->next; next; cur = next, next = cur->next) {
          copy->next = new_node(next->val);
          copy = copy->next;
        }
      }
    }

    num_elements_ = ht.num_elements_;
  }
  catch (...) {
    clear();
    throw;
  }
}

}
