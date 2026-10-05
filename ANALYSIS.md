Q1. 
Pick 3 of the 10 categories. For each, pick a language that gives it to you for free and say what that language pays for it. "Python has dictionaries" isn't an answer. What does Python's dictionary cost in memory or in speed compared to what you built, and where would you notice?

`dt_array`'s custom indexing: Ada. 
    Physical memory computation for non 0 or 1 based indexing. Additional overhead for subtraction is done to determine the location of the memory.

`dt_tuple`'s duck typing: Python. 
    Determined only during runtime and requires extra memory to store the object's actual size per tuple, in comparison to compiled languages that allocates memory upon compilation and treats each tuple combination as different types (thus, dont require additional memory to store the object's size).

Q2. 
You wrote the tag check in `dt_value_as_int` by hand. Some languages don't let you. They make the tagged union a language construct, so the compiler writes the check for you, refuses to compile a read that skips it, and refuses to compile a set of cases that misses one. Rust's `enum` and `match` work this way, and so do ML's datatypes and Swift's enumerations with associated values. What does the C version let you do that a compiler enforcing the check wouldn't, and is any of it worth wanting?

Q3. 
Your `dt_map` keeps insertion order separately from the hash buckets, which is memory spent on something no lookup uses. Argue the other side: describe a design that drops it, say what breaks, and say whether you'd ship it.

Lets say we dont have an array of insertion order in the hash map object. Basically we can still add, delete, and edit entries using the bucket array. `dt_map_put` would no longer need to grow the array. `dt_map_remove` would no longer do the n = n+1 shifting of the entries in the order array. It only allocates 1 array and the map itself. The trade off in this is you wont have an the order of the insertion of values

This would be the structure of the data type 
struct dt_map {
    struct dt_map_entry **buckets;
    size_t num_buckets;
    size_t num_entries;
};

However this time we cant use the `dt_map_key_at` function since we dont know how the keys were inserted in the first place as this was the purpose of the order array and since the index it inserts is random (using the hash key) when adding map entries in the bucket. `dt_map_key_at` can no longer return keys by insertion position. Keys would only be reachable in bucket array, which depends on the hash and bucket count, not on when they were inserted. 

Would I ship it? If the hashmap did not need to have its insertion be ordered, then i would ship it since the map would be simpler and use less memory than having the insertion order. There is also no need to allocate more memory if it reaches the current maximum capacity.

Q4. 
Compare access after release with an allocation that remains unreleased at the driver's final check. What damage can each cause in a long-running server? How does that answer change for a command-line tool that exits in a second?

When accessing the same address you just freed, the memory may have been reused by something else. Reading it returns garbage or another object's data, and writing to it might cause some data being changed unintentionally. When this occurs in a long running server, then it might corrupt data being stored, it might change the behavior of programs, etc.

An allocation that is unreleased makes it so that the memory that was created cant be reached anymore by pointers, so it just sits there taking up space. Now for a server that is running for a long time, this unreleased data may slowly take up more and more space until the server runs out of memory to use.

When in a command line tool that exits after a second, the OS reclaims all its memory that has no pointers to it for an allocation that remains unreleased. While an access after release still has the same problems when it is on a long-running server.