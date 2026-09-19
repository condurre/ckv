#include "kv_store.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    char *value = NULL;
    kv_store *store = kv_store_create(8);

    assert(store != NULL);
    assert(kv_store_set(store, "name", "ckv") == KV_STORE_OK);
    assert(kv_store_get(store, "name", &value) == KV_STORE_OK);
    assert(strcmp(value, "ckv") == 0);
    kv_store_free_value(value);

    assert(kv_store_set(store, "name", "updated") == KV_STORE_OK);
    assert(kv_store_get(store, "name", &value) == KV_STORE_OK);
    assert(strcmp(value, "updated") == 0);
    kv_store_free_value(value);

    assert(kv_store_delete(store, "name") == KV_STORE_OK);
    assert(kv_store_get(store, "name", &value) == KV_STORE_NOT_FOUND);
    assert(kv_store_delete(store, "name") == KV_STORE_NOT_FOUND);
    assert(kv_store_set(store, "", "invalid") == KV_STORE_INVALID_ARGUMENT);
    kv_store_destroy(store);
    return 0;
}
