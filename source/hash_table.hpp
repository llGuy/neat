#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

// Simple hash table implementation
template <
    typename T,
    uint32_t Bucket_Count,
    uint32_t Bucket_Size,
    uint32_t Bucket_Search_Count> struct hash_table_t {
    enum : uint64_t { UNINITIALISED_HASH = 0xFFFFFFFFFFFFFFFF };
    enum { ITEM_POUR_LIMIT    = Bucket_Search_Count };

    struct item_t {
        uint64_t hash = UNINITIALISED_HASH;
        T value = T();
    };

    struct bucket_t {
        uint32_t bucket_usage_count = 0;
        item_t items[Bucket_Size] = {};
    };

    bucket_t buckets[Bucket_Count] = {};

    void init() {
        for (uint32_t bucket_index = 0; bucket_index < Bucket_Count; ++bucket_index) {
            for (uint32_t item_index = 0; item_index < Bucket_Size; ++item_index) {
                buckets[bucket_index].items[item_index].hash = UNINITIALISED_HASH;
            }
        }
    }

    void clear() {
        for (uint32_t i = 0; i < Bucket_Count; ++i) {
            buckets[i].bucket_usage_count = 0;

            for (uint32_t item = 0; item < Bucket_Size; ++item) {
                buckets[i].items[item].hash = UNINITIALISED_HASH;
            }
        }
    }
    
    void clean_up() {
        for (uint32_t i = 0; i < Bucket_Count; ++i) {
            buckets[i].bucket_usage_count = 0;
        }
    }

    void insert(
        uint64_t hash,
        T value) {
        bucket_t *bucket = &buckets[hash % (uint64_t)Bucket_Count];

        for (uint32_t bucket_item = 0; bucket_item < Bucket_Size; ++bucket_item) {
            item_t *item = &bucket->items[bucket_item];
            if (item->hash == UNINITIALISED_HASH) {
                /* found a slot for the object */
                item->hash = hash;
                item->value = value;
                return;
            }
        }
        
        printf("Fatal error in hash table insert()\n");
        assert(0);
    }
    
    void remove(
        uint64_t hash) {
        bucket_t *bucket = &buckets[hash % (uint64_t)Bucket_Count];

        for (uint32_t bucket_item = 0; bucket_item < Bucket_Size; ++bucket_item) {

            item_t *item = &bucket->items[bucket_item];

            if (item->hash == hash && item->hash != UNINITIALISED_HASH) {
                item->hash = UNINITIALISED_HASH;
                item->value = T();
                return;
            }
        }

        printf("Error in hash table remove()\n");
    }
    
    T *get(
        uint64_t hash) {
        static int32_t invalid = -1;
        
        bucket_t *bucket = &buckets[hash % (uint64_t)Bucket_Count];
        
        uint32_t bucket_item = 0;

        uint32_t filled_items = 0;

        for (; bucket_item < Bucket_Size; ++bucket_item) {
            item_t *item = &bucket->items[bucket_item];
            if (item->hash != UNINITIALISED_HASH) {
                ++filled_items;
                if (hash == item->hash) {
                    return(&item->value);
                }
            }
        }

        if (filled_items == Bucket_Size) {
            printf("Error in hash table get()\n");
        }

        return NULL;
    }
};
