#ifndef HE_DATATYPES_HEADER
#define HE_DATATYPES_HEADER

#include <stdlib.h>
#include "common.h"

// ------------------- VECTOR -------------------

typedef struct vector {
    void** items;
    size_t size;
    size_t capacity;
} vector;

/**
 * @brief Constructor method creates empty vector with an initial
 *      capacity.
 * 
 * @param init_capacity Initial pointer storage size of vector
 * @return vector
 */
vector vector_new(size_t init_capacity);

/**
 * @brief Resizes vector and changes the memory allocated to store
 *      pointers.
 * 
 * @param v Reference to vector
 * @param new_capacity New capacity of vector
 */
void vector_resize(vector* v, size_t new_capacity);

/**
 * @brief Appends a pointer to the end of the vector and allocates
 *      more memory if needed.
 * 
 * @param v Reference to vector
 * @param item Item to append
 */
void vector_push(vector* v, void* item);

/**
 * @brief Removes and returns the final item of the vector and resizes
 *      the vector to free up memory if possible.
 * 
 * @param v Reference to vector
 * @return Pointer to removed item
 */
void* vector_pop(vector* v);

/**
 * @brief Returns the top/final pointer of the vector.
 * 
 * @param v Reference to vector
 * @return Pointer to last item
 */
void* vector_top(vector* v);

/**
 * @brief Returns an element in the vector by index.
 * 
 * @param v Reference to vector 
 * @param index Index of item
 * @return Requested item
 */
void* vector_get(vector* v, size_t index);

/**
 * @brief Overwrites a pointer in the vector at a specified
 *      position index, and returns the old occupant.
 * 
 * @param v Reference to vector
 * @param index Position of item
 * @param item New item to set
 * @return Old item
 */
void* vector_set(vector* v, size_t index, void* item) ;

/**
 * @brief Inserts an item into the vector at a specified
 *      index.
 * 
 * @param v Reference to vector
 * @param index Position the item will occupy
 * @param item Item to insert
 */
void vector_insert(vector* v, size_t index, void* item);

/**
 * @brief Removes and returns the pointer to an object at
 *      a position in the vector.
 * 
 * @param v Reference to vector
 * @param index Index of item to removes
 * @return Pointer to item
 */
void* vector_rm(vector* v, size_t index);

/**
 * @brief Deletes the vector object and deallocates the space 
 *      allocated to store it's items. Map must be heap allocated 
 *      or else segmentation fault will occur.
 * 
 * @param v Reference to vector
 */
void vector_delete(vector* v);

// ----------------- STRING MAP -----------------

typedef struct map {
    const char** keys;
    void** values;
    size_t size;
    size_t capacity;
} map;

/**
 * @brief Map constructor allocates space for an empty 
 *      string map.
 * 
 * @param init_capacity Initial size of map
 * @return Map object
 */
map map_new(size_t init_capacity);

/**
 * @brief Fetches a map value by key, returns NULL if key is
 *      not present in the map.
 * 
 * @param m Reference to map
 * @param key Key
 * @return Value
 */
void* map_get(map* m, const char* key);

/**
 * @brief Inserts a new key-value entry to the end of the string
 *       map and resizes the map if necessary.
 * 
 * @param m Reference to map
 * @param key Key
 * @param value Value
 */
void map_put(map* m, const char* key, void* value);

/**
 * @brief Removes and returns an entry from the map by key and 
 *      shifts remaining entries to fill empty space. Returns NULL if
 *      key is not present in map.
 * 
 * @param m Reference to map
 * @param key Key of entry to remove
 * @return Value
 */
void* map_rm(map* m, const char* key);

/**
 * @brief Returns true if a specified key exists in the map, returns
 *      false otherwise.
 * 
 * @param m Reference to map
 * @param key Key to search
 * @return True if map contains key, false otherwise 
 */
boolean map_has(map* m, const char* key);

/**
 * @brief Resizes map to contain a specified number of entries.
 * 
 * @param m Reference to map
 * @param new_capacity New max size of map
 */
void map_resize(map* m, size_t new_capacity);

/**
 * @brief Deletes the map object and deallocates the space 
 *      allocated to store it's entries. Map must be heap
 *      allocated or else segmentation fault will occur.
 * 
 * @param m Reference to map
 */
void map_delete(map* m);

#endif