/*
 * dt_map.c: Associative arrays for Unit 5, Section E.
 *
 * An array does not store its indices. This map stores its keys.
 * An array calculates a position with one subtraction.
 * The map calculates a hash and then compares keys in one bucket.
 *
 * Hashing turns the key into a bucket number. Compare all keys in that bucket
 * because two keys can select it. Use a linked list for each bucket. Start the
 * unsigned accumulator at 14695981039346656037ULL. For each unsigned byte,
 * exclusive-or the byte into it and multiply by 1099511628211ULL.
 *
 * A separate list stores insertion order for stable output. dt_map_key_at
 * reads this list. Updating a key preserves its position. Removing and
 * reinserting a key moves it to the end.
 */

#include "dt.h"

#include <stdlib.h>
#include <string.h>

typedef struct dt_map_entry dt_map_entry;
static dt_map_entry *find_entry(const dt_map *m, const char *key);
static size_t dt_hash(const char *key);

struct dt_map {
    /* TODO: Add the buckets and insertion-order data. */
    // the buckets for the hash table
    struct dt_map_entry **buckets;      // pointer to an array of pointers to dt_map_entry
    size_t num_buckets;                 // the number of buckets in the hash table
    size_t num_entries;                 // the current number of entries in the map
    size_t capacity;                    // the current capacity of the map
    struct dt_map_entry **order;        // the list of keys in insertion order
};

// entry for each key-value pair in the map
struct dt_map_entry {
    char *key;                      // copied key string
    dt_value value;                 // value associated with the key
    struct dt_map_entry *next;      // pointer to the next entry in the same bucket (linked list)
};

// helper function to find an entry in the map by key
struct dt_map_entry *find_entry(const dt_map *m, const char *key)
{
    size_t bucket = dt_hash(key) % m->num_buckets;      // calculate the bucket index for the key
    struct dt_map_entry *entry = m->buckets[bucket];    // get the head of the linked list for that bucket
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) return entry;
        entry = entry->next;
    }
    return NULL;
}

// helper hash method
static size_t dt_hash(const char *key){
    unsigned long long hash = 14695981039346656037ULL;
    size_t key_length = strlen(key);
    for (size_t i = 0; i < key_length; i++) {
        // bitwise operations to compute the hash value using FNV-1a algorithm
        hash ^= (unsigned char)key[i];  // XOR the byte into the hash
        hash *= 1099511628211ULL;       // multiply by the FNV prime
    }
    return hash;
}

/*
 * dt_map_new builds an empty map. It returns NULL after an allocation failure.
 */
dt_map *dt_map_new(void)
{
    /* TODO: Return an allocated empty map. Return NULL after an allocation failure.
       dt_map_new()  -> a map whose dt_map_len is 0
       cases/normal/map_basics.case */
    

    // allocate the new map and each of the slots
    dt_map  *new_map = malloc(sizeof(dt_map));
    if ( new_map == NULL){
        return NULL;
    };
    new_map->buckets = malloc (10 * sizeof(dt_map_entry *));
    if ( new_map->buckets == NULL){
        free (new_map);
        return NULL;
    };

    new_map->order = malloc (10 * sizeof(dt_map_entry *));
    if ( new_map->order == NULL){
        free (new_map);
        return NULL;
    };


    // initialize the contents inside the bucket array in the map to null values
    for (size_t i = 0; i < 10; i++) new_map->buckets[i] = NULL;
    for (size_t i = 0; i < 10; i++) new_map->order[i] = NULL;

    //initialize the other fields
    new_map->num_buckets = 10; // number of slots 
    new_map->num_entries = 0; // number of keys

    new_map->capacity  = 10; // number of keys we can create, can be increased or not
    return new_map;
}

/*
 * dt_map_free releases each entry, copied key, order array, and map.
 * It accepts NULL. The environment owns the values.
 */
void dt_map_free(dt_map *m)
{
    /* TODO: Release each entry, copied key, order array, and map.
       Preserve the values. The environment owns them.
       a map holding a string value  -> the nodes and keys go, the string stays
       dt_map_free(NULL)             -> returns, having done nothing
       cases/cleanup/map_churn.case */
    
    // skip if m is NULL, 
    if (m == NULL) return;

    // otherwise free each entry, copied key, order array, and map
    for (size_t i=0; i < m->num_entries; i++) {
        struct dt_map_entry *entry = m->order[i];
        free(entry->key);
        free(entry);
    }
    free(m->buckets);
    free(m->order);
    free(m);
}

/*
 * dt_map_len returns the number of keys in constant time.
 */
size_t dt_map_len(const dt_map *m)
{
    /* TODO: Return the current key count.
       Replacing a value does not change this count.
       after put alpha, beta, gamma:  dt_map_len(m) -> 3
       after put beta again:          dt_map_len(m) -> 3, still
       after del alpha:               dt_map_len(m) -> 2
       cases/normal/map_basics.case */
    return m->num_entries;
}

/*
 * dt_map_put binds v to key.
 * An existing key keeps its insertion position. A new key becomes the last key.
 * Copy each new key because the caller owns the source buffer.
 * Return DT_ERR_CAPACITY after an allocation failure.
 */
dt_status dt_map_put(dt_map *m, const char *key, dt_value v)
{
    /* TODO: Replace the value for an existing key.
       Add a new entry for a new key. Copy each new key.
       Hash the key. Select its bucket. Search the bucket chain.
       Add a new entry to the chain and insertion list.
       put "beta" -> 2 on an empty map    -> DT_OK, "beta" is last in order
       put "beta" -> 22 on that map       -> DT_OK, same position, new value
       an allocation failure              -> DT_ERR_CAPACITY, map unchanged
       cases/normal/map_basics.case */
    
    // find if an entry already exists
    struct dt_map_entry *entry = find_entry(m, key);
    // key exists, replace the value
    if (entry != NULL) {
        entry->value = v;
        return DT_OK;
    }

    // key does not exist, create a new entry
    // check if we need to expand the order array
    if (m->num_entries == m->capacity) {
        if (m->capacity >= SIZE_MAX / 2) return DT_ERR_CAPACITY;  // prevent overflow
        size_t new_capacity = m->capacity * 2;  // double the capacity

        struct dt_map_entry **new_order = realloc(m->order, new_capacity * sizeof(**new_order));
        if (new_order == NULL) return DT_ERR_CAPACITY;  // allocation failure
        m->order = new_order;
        m->capacity = new_capacity;
    }
    
    // generate a new entry for the key-value pair
    entry = malloc(sizeof(struct dt_map_entry));
    if (entry == NULL) return DT_ERR_CAPACITY;  // allocation failure
    entry->key = malloc(strlen(key) + 1);  // allocate memory for the key copy
    if (entry->key == NULL) {  // allocation failure
        free(entry);
        return DT_ERR_CAPACITY;
    }

    strcpy(entry->key, key);  // copy the key
    entry->value = v;

    size_t b = dt_hash(key) % m->num_buckets;   // calculate the bucket index
    entry->next = m->buckets[b];                // insert at the head of the bucket chain
    m->buckets[b] = entry;                      // insert the new entry at the head of the bucket chain
    m->order[m->num_entries++] = entry;  // add the new entry to the insertion order list
    return DT_OK;
}

/*
 * dt_map_get writes the value for key to *out.
 * It returns DT_ERR_KEY and does not change *out when the key is absent.
 * An absent key differs from a nil value.
 */
dt_status dt_map_get(const dt_map *m, const char *key, dt_value *out)
{
    /* TODO: Return DT_ERR_KEY when the key is absent.
       Preserve *out after this error. A nil value can be present.
       after put "beta" -> 22:
         dt_map_get(m, "beta", &out)   -> DT_OK, *out is the integer 22
         dt_map_get(m, "ghost", &out)  -> DT_ERR_KEY, *out untouched
       cases/normal/map_basics.case, cases/boundary/map_missing_key.case */
    
    struct dt_map_entry *entry = find_entry(m, key);
    if (entry == NULL) return DT_ERR_KEY;  // key is absent, return error
    *out = entry->value;  // write the value to *out
    return DT_OK;
}

/*
 * dt_map_remove removes key from its bucket and insertion position.
 * It releases the copied key. It returns DT_ERR_KEY when the key is absent.
 */
dt_status dt_map_remove(dt_map *m, const char *key)
{
    /* TODO: Remove the entry from its bucket and insertion position.
       Release the copied key. Return DT_ERR_KEY when the key is absent.
       a map holding alpha, beta, gamma:
         dt_map_remove(m, "alpha")  -> DT_OK, order is now beta, gamma
         dt_map_remove(m, "ghost")  -> DT_ERR_KEY, nothing changes
       reinserting "alpha" appends it after "gamma"
       cases/normal/map_basics.case, cases/boundary/map_remove_missing_key.case */
    
    size_t b = dt_hash(key) % m->num_buckets;  // calculate the bucket index

    // **link - pointer to the pointer to the current entry in the bucket chain
    // &m->buckets[b] - gets the address of the pointer to the head of the bucket chain
    struct dt_map_entry **link = &m->buckets[b];    
    
    //// traverse the bucket chain to find the entry with the matching key
    // link - stores the address of the pointer to the current entry 
    // *link - reads the pointer, therefore *link is the current entry
    // *link->key - key of the current entry
    // &(*link)->next - gets the address of the next entry pointer, which is the next link in the chain
    while (*link != NULL && strcmp((*link)->key, key) != 0) {
        link = &(*link)->next;  
    }

    struct dt_map_entry *entry = *link;     // store the pointer to the entry to be removed
    if (entry == NULL) return DT_ERR_KEY;   // key is absent, return error
    *link = entry->next;                    // remove the entry from the bucket chain

    size_t index = 0;
    while (m->order[index] != entry) index++;   // find the index of the entry in the insertion order list

    // shift the remaining entries in the order array to fill the gap
    for (size_t i = index; i < m->num_entries - 1; i++) {
        m->order[i] = m->order[i + 1];
    }
    m->num_entries--;  // decrease the number of entries
    free(entry->key);  // release the copied key
    free(entry);       // release the entry
    return DT_OK;
}

/*
 * dt_map_key_at writes the key at insertion position index to *out.
 * It returns DT_ERR_RANGE and does not change *out for an invalid index.
 */
dt_status dt_map_key_at(const dt_map *m, size_t index, const char **out)
{
    /* TODO: Write the key at the specified insertion position to *out.
       Return DT_ERR_RANGE for an invalid position. Preserve *out after this error.
       The printer uses this order.
       a map holding alpha, beta, gamma:
         dt_map_key_at(m, 0, &out)  -> DT_OK, *out = "alpha"
         dt_map_key_at(m, 3, &out)  -> DT_ERR_RANGE, *out untouched
       cases/normal/map_basics.case */
    if (index >= m->num_entries) return DT_ERR_RANGE;  // invalid index, return error
    *out = m->order[index]->key;  // write the key at the specified index to *out
    return DT_OK;
}




