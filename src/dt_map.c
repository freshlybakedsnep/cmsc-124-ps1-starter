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

struct dt_map {
    // we have 5 fields

    // both dt_map_entry arrays points to the same dt_map_entry struct...
    // what is inside of those arrays are different
    // contains the n elements of slots and we will have 10 slots
    dt_map_entry  **buckets;
    size_t num_buckets; // the number of slots
    size_t num_entries; // the number of keys in the entire map
     
    
    // an array of pointers that points to the same dt_map_entry...
    // inside of the array are the dt_map_entry structs from the first one inserted...
    // until the last one that was inserted
    dt_map_entry **order;
    size_t order_cap; //capacity of order,

};

// this is a linked list
struct dt_map_entry {
    char *key;   
    dt_value value;
    dt_map_entry *next;
};

//prototype functions
static unsigned long long hash_key(const char *key);
static dt_map_entry *find_entry(const dt_map *m, const char *key);
/*static dt_map_entry *find_entry_order(const dt_map *m, const char *key);*/

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

    new_map->order_cap  = 10; // number of keys we can create, can be increased or not
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
    (void)m;

    //clear the slots first
    for (size_t i = 0; i < m->num_buckets; i++) {
        free(m->order[i]->key);
        free(m->order[i]);

    };
    free(m->order);
    free(m->buckets);
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
    
    
    dt_map_entry *current;
    dt_map_entry *new_entry;
    dt_map_entry **new_order;

    

    
    current = find_entry(m, key);

    // edit the current entry
    if (current != NULL) {
        current->value = v;
        return DT_OK;
    }

    
    // allocate first the new entry
    //check if allocation failed
    new_entry = malloc(sizeof(dt_map_entry));
    if (new_entry == NULL) {
        return DT_ERR_CAPACITY;
    };

    // allocate new key
    size_t len = strlen(key) + 1;
    new_entry->key = malloc(len);
    if (new_entry->key == NULL) {
        free(new_entry);
        return DT_ERR_CAPACITY;
    }

    memcpy(new_entry->key, key, len);
    new_entry->value = v;

    // grow the order array if it is full (temp pointer so the old array isn't lost on failure)
    if (m->num_entries == m->order_cap) {
        size_t new_cap = m->order_cap * 2;
        new_order = realloc(m->order, new_cap * sizeof(dt_map_entry *));
        if (new_order == NULL) {
            free(new_entry->key);
            free(new_entry);
            return DT_ERR_CAPACITY;
        }
        m->order = new_order;
        m->order_cap = new_cap;
    }

    // every allocation succeeded, so now edit the map
    // link the new entry at the head of its bucket chain
    size_t index = hash_key(key) % m->num_buckets;
    new_entry->next = m->buckets[index];
    m->buckets[index] = new_entry;
    
    // add it to the end of the insertion order
    m->order[m->num_entries] = new_entry;

    // increment the number of unique keys in the map
    m->num_entries += 1;

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
    (void)m;
    (void)key;
    (void)out;
    return DT_ERR_KEY;
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
    (void)m;
    (void)key;
    return DT_ERR_KEY;
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
    (void)m;
    (void)index;
    (void)out;
    return DT_ERR_RANGE;
}




// HELPER FUNCTIONS
static dt_map_entry *find_entry(const dt_map *m, const char *key) {
    //init
    dt_map_entry *current;
    
    // bucket = hash_key(key) % m->num_buckets
    size_t index = hash_key(key) % m->num_buckets;

    // walk the chain with dt_map_entry->next, compare with strcmp
    current = m->buckets[index];
    while (current != NULL && (strcmp(current->key, key) != 0)) {
        //traverse through the linked list
        current = current->next;
        
    };
    
    // return the entry or NULL
    if (strcmp(current->key, key) == 0) {
        return current;
    };

    // current is NULL
    return NULL;
}
/*
static dt_map_entry *find_entry_order(dt_map *m, const char *key) {
    
    
    //init
    dt_map_entry *current;
   
    // walk the chain with dt_map_entry->next, compare with strcmp
    current = m->order[0];
    while (current != NULL && (strcmp(current->key, key) != 0)) {
        //traverse through the linked list
        current = current->next;
        
    };
    
    // return the entry or NULL
    if (strcmp(current->key, key) == 0) {
        return current;
    };

    // current is NULL
    return NULL;
    
    return 0;
}
*/

static unsigned long long hash_key(const char *key)
{
    unsigned long long h = 14695981039346656037ULL;
    for (const unsigned char *p = (const unsigned char *)key; *p; p++) {
        // XOR
        h ^= *p;

        // multiply h to 1099511628211ULL
        h *= 1099511628211ULL;
    }
    return h; 
}