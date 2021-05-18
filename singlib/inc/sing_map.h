#pragma once

#include <vector>
#include <stdint.h>

namespace sing {

int32_t hash_from_bytes(const uint8_t *buffer, int count);

template<class K, class V>
class map_record 
{
public:
    map_record(K keyval, V valval, int32_t nextval, int32_t ashval) {
        key = keyval;
        value = valval;
        next = nextval;
        hash = ashval;
    }

    bool operator==(const map_record<K, V> &right) const
    {
        return(key == right.key && value == right.value);
    }

    K       key;
    V       value;
    int32_t next;
    int32_t hash;
};

template<class K>
class hash_functor
{
public:
    int32_t hash_(const K &key) const
    {
        return(hash_from_bytes((const uint8_t *)&key, sizeof(K)));
    }  
};

template<>
class hash_functor<std::string>
{
public:
    int32_t hash_(const std::string &key) const
    {
        return(hash_from_bytes((const uint8_t*)key.data(), key.length()));
    }  
};

template<class K, class V>
class map {
public:
    // constructor 
    map() {
        hash_mask_ = 0;
        last_searched_ = -1;
    }

    map(const std::initializer_list<std::pair<K,V>> &list) {
        hash_mask_ = 0;
        last_searched_ = -1;
        reserve(list.size());
        const std::pair<K,V> *src = list.begin();
        for (size_t ii = 0; ii < list.size(); ++ii) {
            insert(src->first, src->second);
            src++;
        }
    }

    map(const map<K, V> &src) {
        *this = src;
    }

    // operators
    void operator=(const std::initializer_list<std::pair<K,V>> &list) {
        clear();
        reserve(list.size());
        const std::pair<K,V> *src = list.begin();
        for (size_t ii = 0; ii < list.size(); ++ii) {
            insert(src->first, src->second);
            src++;
        }
    }

    map<K, V> &operator=(const map<K, V> &src) {
        hashvect_ = src.hashvect_;         
        pairs_ = src.pairs_;
        hash_mask_ = src.hash_mask_;
        last_searched_ = -1;                 
    }

    // defective: returns false if order of items differ !!
    bool operator==(const map<K, V> &right) const
    {
        return(pairs_ == right.pairs_);
    }

    // modification
    void insert(const K &key, const V &value)
    {
        // enlarge if needed
        reash((pairs_.size() + 1) << 1);

        hash_functor<K> hasher;
        int32_t hash = hasher.hash_(key);
        int slot = hash & hash_mask_;        

        // already there ?
        int pos = hashvect_[slot];
        while (pos >= 0) {
            map_record<K, V> *rec = &pairs_[pos];
            if (rec->key == key) {
                rec->value = value;
                return;
            }
            pos = rec->next;
        }

        // no: insert !
        pairs_.emplace_back(key, value, hashvect_[slot], hash);
        hashvect_[slot] = pairs_.size() - 1;
    }

    void erase(const K &key)
    {
        if (hash_mask_ == 0) return;
        hash_functor<K> hasher;
        int slot = hasher.hash_(key) & hash_mask_;        

        // look for the item to replace
        int pos = hashvect_[slot];
        map_record<K, V> *prev = nullptr;
        while (pos >= 0) {
            map_record<K, V> *rec = &pairs_[pos];
            if (rec->key == key) {

                // found : unlink
                if (prev == nullptr) {
                    hashvect_[slot] = rec->next;
                } else {
                    prev->next = rec->next;
                }

                int idx_of_last = pairs_.size() - 1;

                // if is not the last item must overwrite with the last
                if (pos != idx_of_last) {

                    // find and fix the link to the last item in pairs_
                    int slot_of_last = pairs_.back().hash & hash_mask_;
                    int pos_of_last = hashvect_[slot_of_last];
                    if (pos_of_last == idx_of_last) {
                        hashvect_[slot_of_last] = pos;
                    } else {
                        while (pos_of_last >= 0) {
                            map_record<K, V> *rec = &pairs_[pos_of_last];
                            if (rec->next == idx_of_last) {
                                rec->next = pos;
                                break;
                            }
                        }
                    }
                    pairs_[pos] = pairs_.back();
                }
                pairs_.pop_back();
                return;
            }
            prev = rec;
            pos = rec->next;
        }
    }

    void clear(void)
    {
        pairs_.clear();
        hashvect_.clear();
        hash_mask_ = 0;
        last_searched_ = -1;
    }

    void reserve(const size_t count) {        
        pairs_.reserve(count);
        reash(count << 1);
    }

    void shrink_to_fit(void) {
        pairs_.shrink_to_fit();
    }

    // queries
    const V &get(const K &key) const
    {
        if (last_searched_ >= 0 && last_searched_ < pairs_.size()) {
            const map_record<K, V> *rec = &pairs_[last_searched_];
            if (rec->key == key) return(rec->value);
        }
        last_searched_ = -1;
        V *vp = find(key);
        if (vp == nullptr) {
            throw(std::out_of_range("map key"));
        }
        return(*vp);
    }

    const V &get_safe(const K &key, const V &value) const
    {
        V *vp = find(key);
        if (vp == nullptr) {
            return(value);
        }
        return(*vp);
    }

    bool has(const K &key) const
    {
        return(find(key) != nullptr);
    }

    int32_t size(void) const
    {
        return((int32_t)pairs_.size());
    }

    size_t size64(void) const
    {
        return(pairs_.size());
    }

    const map_record<K, V> *begin(void) const
    {
        return(pairs_.begin());
    }

    const map_record<K, V> *end(void) const
    {
        return(pairs_.end());
    }

    int32_t capacity(void) const {
        return(pairs_.capacity());
    }

    bool isempty(void) const {
        return(pairs_.size() == 0);
    }

    const K &key_at(size_t idx) const
    {
        return(pairs_.at(idx).key);
    }

    const V &value_at(size_t idx) const
    {
        return(pairs_.at(idx).value);
    }

private:
    V *find(const K &key) const
    {
        if (hash_mask_ != 0) { 
            hash_functor<K> hasher;
            int pos = hashvect_[hasher.hash_(key) & hash_mask_];
            while (pos >= 0) {
                const map_record<K, V> *rec = &pairs_[pos];
                if (rec->key == key) {
                    last_searched_ = pos;
                    return((V*)&rec->value);
                }
                pos = rec->next;
            }
        }
        return(nullptr);
    }

    // input parm is the minimum required hash table size 
    void reash(int count)
    {
        count = std::max(count, 16);
        int ash_size = hash_mask_ + 1;
        if (ash_size >= count) {
            return;
        }
        while (ash_size < count) ash_size <<= 1;
        hash_mask_ = ash_size - 1;
        hashvect_.clear();
        hashvect_.reserve(ash_size);
        for (int ii = 0; ii < ash_size; ++ii) hashvect_.push_back(-1);

        for (int ii = 0; ii < pairs_.size(); ++ii) {
            map_record<K, V> *rec = &pairs_[ii];
            int slot = rec->hash & hash_mask_;
            rec->next = hashvect_[slot];
            hashvect_[slot] = ii;
        }
        last_searched_ = -1;
    }

    std::vector<int32_t>            hashvect_;         
    std::vector<map_record<K, V>>   pairs_;
    int32_t                         hash_mask_;     
    mutable int32_t                 last_searched_;                 
};

} // namespace
