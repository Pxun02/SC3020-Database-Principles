#ifndef STRUCTURES_H
#define STRUCTURES_H


// The Data Structure for storing a movie record
// Fields:
// recordID: the id of a movie record
// tconst: alphanumeric unique identifier of the title
// averageRating: weighted average of all the individual users' ratings
// numVotes: number of votes this movie has received

// Example: {123, "tt12345678", 9.6, 1234}
struct movieRecord
{
    unsigned int recordID;  // 4 bytes
    char tconst[11];        // 11 bytes with null terminator
    float averageRating;    // 4 bytes
    unsigned int numVotes;  // 4 bytes
};

// Used within a block to map the recordID to its position in the block
// Position is determined from the end
// For example, a record with ID 42, located as the last record in the block
// would be {42, 0}
struct indexMapping
{
    unsigned int recordID; 
    int indexOfRecord; // -1 represents a deleted record in block
};

// Used as our pointer structure in B+ tree
// For leaf nodes, blockAddress means address of the data block it points to
// For non-leaf nodes, blockAddress means address of the index block it points to
struct pointerBlockPair // 8 bytes
{
    void* blockAddress;
    int recordID; // -1 indicates an overflow, any positive indicates the a duplicated record
};


// Used to store relevant header information for a node in the B+ tree
struct NodeHeader // 14->16 (padded) bytes, multiple of 4
{
    unsigned int numKeys;
    pointerBlockPair pointerToParent;
    bool isLeaf;
};

#endif