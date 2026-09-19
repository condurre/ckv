#include "kv_store.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct kv_entry {
    char *key;
    char *value;
    struct kv_entry *next;
};

struct kv_store {
    size_t bucket_count;
    struct kv_entry **buckets;
    pthread_mutex_t mutex;
};

static unsigned long hash_key(const char *key)
{
    unsigned long hash = 5381;
    unsigned char character;

    while ((character = (unsigned char)*key++) != '\0') {
        hash = ((hash << 5) + hash) ^ character;
    }

    return hash;
}

static struct kv_entry *find_entry(const kv_store *store, const char *key)
{
    size_t bucket = hash_key(key) % store->bucket_count;
    struct kv_entry *entry = store->buckets[bucket];

    while (entry != NULL) {
        int comparison = strcmp(entry->key, key);

        if (comparison == 0) {
            return entry;
        }
        entry = entry->next;
    }

    return NULL;
}

kv_store *kv_store_create(size_t bucket_count)
{
    kv_store *store;
    int mutex_result;

    if (bucket_count == 0) {
        return NULL;
    }

    store = calloc(1, sizeof(*store));
    if (store == NULL) {
        return NULL;
    }

    store->buckets = calloc(bucket_count, sizeof(*store->buckets));
    if (store->buckets == NULL) {
        free(store);
        return NULL;
    }

    mutex_result = pthread_mutex_init(&store->mutex, NULL);
    if (mutex_result != 0) {
        free(store->buckets);
        free(store);
        return NULL;
    }

    store->bucket_count = bucket_count;
    return store;
}

void kv_store_destroy(kv_store *store)
{
    size_t bucket;

    if (store == NULL) {
        return;
    }

    for (bucket = 0; bucket < store->bucket_count; bucket++) {
        struct kv_entry *entry = store->buckets[bucket];

        while (entry != NULL) {
            struct kv_entry *next = entry->next;

            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
    }

    pthread_mutex_destroy(&store->mutex);
    free(store->buckets);
    free(store);
}

int kv_store_set(kv_store *store, const char *key, const char *value)
{
    struct kv_entry *entry;
    char *new_value;
    int lock_result;

    if (store == NULL || key == NULL || value == NULL || *key == '\0') {
        return KV_STORE_INVALID_ARGUMENT;
    }

    new_value = strdup(value);
    if (new_value == NULL) {
        return KV_STORE_OUT_OF_MEMORY;
    }

    lock_result = pthread_mutex_lock(&store->mutex);
    if (lock_result != 0) {
        free(new_value);
        return KV_STORE_OUT_OF_MEMORY;
    }

    entry = find_entry(store, key);
    if (entry != NULL) {
        free(entry->value);
        entry->value = new_value;
        pthread_mutex_unlock(&store->mutex);
        return KV_STORE_OK;
    }

    entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        pthread_mutex_unlock(&store->mutex);
        free(new_value);
        return KV_STORE_OUT_OF_MEMORY;
    }

    entry->key = strdup(key);
    if (entry->key == NULL) {
        pthread_mutex_unlock(&store->mutex);
        free(new_value);
        free(entry);
        return KV_STORE_OUT_OF_MEMORY;
    }

    entry->value = new_value;
    entry->next = store->buckets[hash_key(key) % store->bucket_count];
    store->buckets[hash_key(key) % store->bucket_count] = entry;
    pthread_mutex_unlock(&store->mutex);

    return KV_STORE_OK;
}

int kv_store_get(kv_store *store, const char *key, char **value)
{
    struct kv_entry *entry;
    int lock_result;

    if (store == NULL || key == NULL || value == NULL || *key == '\0') {
        return KV_STORE_INVALID_ARGUMENT;
    }

    *value = NULL;
    lock_result = pthread_mutex_lock(&store->mutex);
    if (lock_result != 0) {
        return KV_STORE_OUT_OF_MEMORY;
    }

    entry = find_entry(store, key);
    if (entry == NULL) {
        pthread_mutex_unlock(&store->mutex);
        return KV_STORE_NOT_FOUND;
    }

    *value = strdup(entry->value);
    pthread_mutex_unlock(&store->mutex);

    if (*value == NULL) {
        return KV_STORE_OUT_OF_MEMORY;
    }

    return KV_STORE_OK;
}

int kv_store_delete(kv_store *store, const char *key)
{
    size_t bucket;
    struct kv_entry **entry;
    int lock_result;

    if (store == NULL || key == NULL || *key == '\0') {
        return KV_STORE_INVALID_ARGUMENT;
    }

    lock_result = pthread_mutex_lock(&store->mutex);
    if (lock_result != 0) {
        return KV_STORE_OUT_OF_MEMORY;
    }

    bucket = hash_key(key) % store->bucket_count;
    entry = &store->buckets[bucket];
    while (*entry != NULL && strcmp((*entry)->key, key) != 0) {
        entry = &(*entry)->next;
    }

    if (*entry == NULL) {
        pthread_mutex_unlock(&store->mutex);
        return KV_STORE_NOT_FOUND;
    }

    {
        struct kv_entry *removed = *entry;

        *entry = removed->next;
        free(removed->key);
        free(removed->value);
        free(removed);
    }

    pthread_mutex_unlock(&store->mutex);
    return KV_STORE_OK;
}

void kv_store_free_value(char *value)
{
    free(value);
}
