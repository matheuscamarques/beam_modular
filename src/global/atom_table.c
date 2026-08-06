#include "beam_global.h"
#include <string.h>

#define BEAM_ATOM_DEFAULT_CAPACITY 256

typedef struct beam_atom_entry {
    char* name;
    size_t len;
    uint32_t hash;
    uint32_t index;
    struct beam_atom_entry* next;
} beam_atom_entry_t;

struct beam_atom_table {
    beam_allocator_i alloc;
    beam_atom_entry_t** buckets;
    beam_atom_entry_t** index_map;
    size_t bucket_count;
    size_t count;
    size_t index_map_capacity;
};

static uint32_t hash_atom_name(const char* name, size_t len) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)name[i];
        hash *= 16777619u;
    }
    return hash;
}

beam_atom_table_t* beam_atom_table_create(const beam_allocator_i* alloc, size_t initial_capacity) {
    if (!alloc || !alloc->alloc || !alloc->free) {
        return NULL;
    }

    size_t capacity = (initial_capacity > 0) ? initial_capacity : BEAM_ATOM_DEFAULT_CAPACITY;

    beam_atom_table_t* table = (beam_atom_table_t*)alloc->alloc(alloc->ctx, sizeof(beam_atom_table_t));
    if (!table) return NULL;

    memset(table, 0, sizeof(beam_atom_table_t));
    table->alloc = *alloc;
    table->bucket_count = capacity;
    table->index_map_capacity = capacity;

    table->buckets = (beam_atom_entry_t**)alloc->alloc(alloc->ctx, sizeof(beam_atom_entry_t*) * table->bucket_count);
    if (!table->buckets) {
        alloc->free(alloc->ctx, table);
        return NULL;
    }
    memset(table->buckets, 0, sizeof(beam_atom_entry_t*) * table->bucket_count);

    table->index_map = (beam_atom_entry_t**)alloc->alloc(alloc->ctx, sizeof(beam_atom_entry_t*) * table->index_map_capacity);
    if (!table->index_map) {
        alloc->free(alloc->ctx, table->buckets);
        alloc->free(alloc->ctx, table);
        return NULL;
    }
    memset(table->index_map, 0, sizeof(beam_atom_entry_t*) * table->index_map_capacity);

    return table;
}

void beam_atom_table_destroy(beam_atom_table_t* table) {
    if (!table) return;

    beam_allocator_i alloc = table->alloc;

    for (size_t i = 0; i < table->bucket_count; i++) {
        beam_atom_entry_t* entry = table->buckets[i];
        while (entry) {
            beam_atom_entry_t* next = entry->next;
            if (entry->name) {
                alloc.free(alloc.ctx, entry->name);
            }
            alloc.free(alloc.ctx, entry);
            entry = next;
        }
    }

    if (table->buckets) alloc.free(alloc.ctx, table->buckets);
    if (table->index_map) alloc.free(alloc.ctx, table->index_map);
    alloc.free(alloc.ctx, table);
}

beam_result_t beam_atom_find(const beam_atom_table_t* table, const char* name, size_t len, uint32_t* out_index) {
    if (!table || !name || !out_index) return BEAM_ERR_INVALID_ARG;

    uint32_t hash = hash_atom_name(name, len);
    size_t bucket_idx = hash % table->bucket_count;

    beam_atom_entry_t* entry = table->buckets[bucket_idx];
    while (entry) {
        if (entry->hash == hash && entry->len == len && memcmp(entry->name, name, len) == 0) {
            *out_index = entry->index;
            return BEAM_OK;
        }
        entry = entry->next;
    }

    return BEAM_ERR_NOT_FOUND;
}

beam_result_t beam_atom_put(beam_atom_table_t* table, const char* name, size_t len, uint32_t* out_index) {
    if (!table || !name || !out_index) return BEAM_ERR_INVALID_ARG;

    beam_result_t res = beam_atom_find(table, name, len, out_index);
    if (res == BEAM_OK) return BEAM_OK;

    if (table->count >= table->index_map_capacity) {
        size_t new_cap = table->index_map_capacity * 2;
        beam_atom_entry_t** new_map = (beam_atom_entry_t**)table->alloc.realloc(
            table->alloc.ctx, table->index_map, sizeof(beam_atom_entry_t*) * new_cap);
        if (!new_map) return BEAM_ERR_NO_MEMORY;
        table->index_map = new_map;
        table->index_map_capacity = new_cap;
    }

    uint32_t hash = hash_atom_name(name, len);
    size_t bucket_idx = hash % table->bucket_count;

    beam_atom_entry_t* entry = (beam_atom_entry_t*)table->alloc.alloc(table->alloc.ctx, sizeof(beam_atom_entry_t));
    if (!entry) return BEAM_ERR_NO_MEMORY;

    entry->name = (char*)table->alloc.alloc(table->alloc.ctx, len + 1);
    if (!entry->name) {
        table->alloc.free(table->alloc.ctx, entry);
        return BEAM_ERR_NO_MEMORY;
    }

    memcpy(entry->name, name, len);
    entry->name[len] = '\0';
    entry->len = len;
    entry->hash = hash;
    entry->index = (uint32_t)table->count;

    entry->next = table->buckets[bucket_idx];
    table->buckets[bucket_idx] = entry;

    table->index_map[table->count] = entry;
    *out_index = entry->index;
    table->count++;

    return BEAM_OK;
}

const char* beam_atom_get_name(const beam_atom_table_t* table, uint32_t index, size_t* out_len) {
    if (!table || index >= table->count) return NULL;

    beam_atom_entry_t* entry = table->index_map[index];
    if (!entry) return NULL;

    if (out_len) *out_len = entry->len;
    return entry->name;
}

size_t beam_atom_table_count(const beam_atom_table_t* table) {
    return table ? table->count : 0;
}
