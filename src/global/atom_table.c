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

static Eterm atom_eterm(uint32_t index) {
    return (Eterm)(((Eterm)index << 4) | TAG_IMMED1_ATOM);
}

static uint32_t atom_index(Eterm term) {
    return (uint32_t)(term >> 4);
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

static beam_atom_entry_t* atom_table_find_entry(const beam_atom_table_t* table, const char* name, size_t len) {
    if (!table || !name) return NULL;

    uint32_t hash = hash_atom_name(name, len);
    size_t bucket_idx = hash % table->bucket_count;

    beam_atom_entry_t* entry = table->buckets[bucket_idx];
    while (entry) {
        if (entry->hash == hash && entry->len == len && memcmp(entry->name, name, len) == 0) {
            return entry;
        }
        entry = entry->next;
    }

    return NULL;
}

static Eterm atom_table_insert(beam_atom_table_t* table, const char* name, size_t len) {
    if (!table || !name) return 0;

    if (table->count >= table->index_map_capacity) {
        size_t new_cap = table->index_map_capacity * 2;
        beam_atom_entry_t** new_map = (beam_atom_entry_t**)table->alloc.realloc(
            table->alloc.ctx, table->index_map, sizeof(beam_atom_entry_t*) * new_cap);
        if (!new_map) return 0;
        table->index_map = new_map;
        table->index_map_capacity = new_cap;
    }

    uint32_t hash = hash_atom_name(name, len);
    size_t bucket_idx = hash % table->bucket_count;

    beam_atom_entry_t* entry = (beam_atom_entry_t*)table->alloc.alloc(table->alloc.ctx, sizeof(beam_atom_entry_t));
    if (!entry) return 0;

    entry->name = (char*)table->alloc.alloc(table->alloc.ctx, len + 1);
    if (!entry->name) {
        table->alloc.free(table->alloc.ctx, entry);
        return 0;
    }

    memcpy(entry->name, name, len);
    entry->name[len] = '\0';
    entry->len = len;
    entry->hash = hash;
    entry->index = (uint32_t)table->count;

    entry->next = table->buckets[bucket_idx];
    table->buckets[bucket_idx] = entry;

    table->index_map[table->count] = entry;
    table->count++;

    return atom_eterm(entry->index);
}

Eterm beam_atom_intern(beam_atom_table_t* table, const char* name) {
    if (!table || !name) return 0;

    size_t len = strlen(name);
    beam_atom_entry_t* existing = atom_table_find_entry(table, name, len);
    if (existing) {
        return atom_eterm(existing->index);
    }

    return atom_table_insert(table, name, len);
}

Eterm beam_atom_intern_length(beam_atom_table_t* table, const char* name, size_t len) {
    if (!table || !name) return 0;

    beam_atom_entry_t* existing = atom_table_find_entry(table, name, len);
    if (existing) {
        return atom_eterm(existing->index);
    }

    return atom_table_insert(table, name, len);
}

const char* beam_atom_lookup(const beam_atom_table_t* table, Eterm atom_term) {
    if (!table) return NULL;
    if ((atom_term & 0x0F) != TAG_IMMED1_ATOM) return NULL;

    uint32_t index = atom_index(atom_term);
    if (index >= table->count) return NULL;

    beam_atom_entry_t* entry = table->index_map[index];
    if (!entry) return NULL;

    return entry->name;
}

size_t beam_atom_table_count(const beam_atom_table_t* table) {
    return table ? table->count : 0;
}
