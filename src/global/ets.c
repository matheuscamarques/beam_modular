#include "ets_internal.h"
#include <string.h>

static uint32_t hash_eterm(Eterm key) {
    uint32_t x = (uint32_t)key;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

beam_ets_table_t* beam_ets_table_create_typed(const char* name, beam_ets_type_t type, beam_ets_protection_t protection, const beam_allocator_i* alloc) {
    if (!alloc || !alloc->alloc || !alloc->free) return NULL;

    beam_ets_table_t* table = (beam_ets_table_t*)alloc->alloc(alloc->ctx, sizeof(beam_ets_table_t));
    if (!table) return NULL;

    memset(table, 0, sizeof(beam_ets_table_t));
    if (name) {
        strncpy(table->name, name, sizeof(table->name) - 1);
    }
    table->type = type;
    table->protection = protection;
    table->alloc = *alloc;
    table->bucket_count = ETS_DEFAULT_BUCKETS;

    table->buckets = (ets_entry_t**)alloc->alloc(alloc->ctx, sizeof(ets_entry_t*) * table->bucket_count);
    if (!table->buckets) {
        alloc->free(alloc->ctx, table);
        return NULL;
    }
    memset(table->buckets, 0, sizeof(ets_entry_t*) * table->bucket_count);

    return table;
}

beam_ets_table_t* beam_ets_table_create(const char* name, const beam_allocator_i* alloc) {
    return beam_ets_table_create_typed(name, BEAM_ETS_SET, BEAM_ETS_PUBLIC, alloc);
}

void beam_ets_table_destroy(beam_ets_table_t* table) {
    if (!table) return;

    beam_allocator_i alloc = table->alloc;

    for (size_t i = 0; i < table->bucket_count; i++) {
        ets_entry_t* entry = table->buckets[i];
        while (entry) {
            ets_entry_t* next = entry->next;
            alloc.free(alloc.ctx, entry);
            entry = next;
        }
    }

    if (table->buckets) {
        alloc.free(alloc.ctx, table->buckets);
    }
    alloc.free(alloc.ctx, table);
}

beam_result_t beam_ets_insert(beam_ets_table_t* table, Eterm key, Eterm value) {
    if (!table) return BEAM_ERR_INVALID_ARG;

    uint32_t hash = hash_eterm(key);
    size_t idx = hash % table->bucket_count;

    if (table->type == BEAM_ETS_SET || table->type == BEAM_ETS_ORDERED_SET) {
        /* Check if key already exists, update value */
        ets_entry_t* entry = table->buckets[idx];
        while (entry) {
            if (entry->key == key) {
                entry->value = value;
                return BEAM_OK;
            }
            entry = entry->next;
        }
    } else if (table->type == BEAM_ETS_BAG) {
        /* BAG: ignore exact duplicate (key, value) pairs */
        ets_entry_t* entry = table->buckets[idx];
        while (entry) {
            if (entry->key == key && entry->value == value) {
                return BEAM_OK;
            }
            entry = entry->next;
        }
    }
    /* DUPLICATE_BAG: insert unconditionally */

    /* Insert new entry */
    ets_entry_t* entry = (ets_entry_t*)table->alloc.alloc(table->alloc.ctx, sizeof(ets_entry_t));
    if (!entry) return BEAM_ERR_NO_MEMORY;

    entry->key = key;
    entry->value = value;
    entry->next = table->buckets[idx];
    table->buckets[idx] = entry;
    table->item_count++;

    return BEAM_OK;
}

beam_result_t beam_ets_lookup(const beam_ets_table_t* table, Eterm key, Eterm* out_value) {
    if (!table || !out_value) return BEAM_ERR_INVALID_ARG;

    uint32_t hash = hash_eterm(key);
    size_t idx = hash % table->bucket_count;

    ets_entry_t* entry = table->buckets[idx];
    while (entry) {
        if (entry->key == key) {
            *out_value = entry->value;
            return BEAM_OK;
        }
        entry = entry->next;
    }

    return BEAM_ERR_NOT_FOUND;
}

beam_result_t beam_ets_delete(beam_ets_table_t* table, Eterm key) {
    if (!table) return BEAM_ERR_INVALID_ARG;

    uint32_t hash = hash_eterm(key);
    size_t idx = hash % table->bucket_count;

    ets_entry_t** curr = &table->buckets[idx];
    while (*curr) {
        if ((*curr)->key == key) {
            ets_entry_t* to_free = *curr;
            *curr = to_free->next;
            table->alloc.free(table->alloc.ctx, to_free);
            table->item_count--;
            return BEAM_OK;
        }
        curr = &(*curr)->next;
    }

    return BEAM_ERR_NOT_FOUND;
}

size_t beam_ets_count(const beam_ets_table_t* table) {
    return table ? table->item_count : 0;
}

beam_result_t beam_ets_update_counter(beam_ets_table_t* table, Eterm key, intptr_t increment, Eterm* out_new_value) {
    if (!table || !out_new_value) return BEAM_ERR_INVALID_ARG;

    uint32_t hash = hash_eterm(key);
    size_t idx = hash % table->bucket_count;

    ets_entry_t* entry = table->buckets[idx];
    while (entry) {
        if (entry->key == key) {
            intptr_t curr_val = eterm_to_small_int(entry->value);
            intptr_t new_val = curr_val + increment;
            entry->value = make_small_int(new_val);
            *out_new_value = entry->value;
            return BEAM_OK;
        }
        entry = entry->next;
    }

    return BEAM_ERR_NOT_FOUND;
}
