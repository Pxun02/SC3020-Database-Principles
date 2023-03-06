#include <utility>
#include <unordered_map>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <list>

using namespace std;

class DiskSimulator
{
    public:
    void* disk;     // holds disk's base address
    int blockSize;  
    int numOfBlocks;
    list<void*> emptyBlocks;
    unordered_map<void*, bool> mapTable;    // mapTable keeps track for each block, if it is not in use
    
    // Constructs a new disk of {size}MB and splits the disk into multiple blocks of {sizeOfBlock}B each
    DiskSimulator(int size, int sizeOfBlock);
    
    // function to set a block as empty or non-empty
    void updateMapTable(void* blockAddr);

    // function to return a block's address
    void* fetchBlockAddress(int blockId);

    // function to get an unused block
    void* getUnusedBlock();
};