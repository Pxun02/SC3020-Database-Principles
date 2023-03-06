#include "DiskSimulator.h"

DiskSimulator::DiskSimulator(int size, int sizeOfBlock)
{
    blockSize = sizeOfBlock;
    disk = malloc(size*1000000);

    // Initialise mapTable - i.e. split disk into blocks
    numOfBlocks = (size*1000000) / sizeOfBlock;
    int i;
    for (i=0; i<numOfBlocks; i++)
    {
        // Add offset to disk's base address to get this block's address
        void* blockAddress = reinterpret_cast<void*>(reinterpret_cast<char*>(disk) + i*sizeOfBlock);
        mapTable[blockAddress] = true;
        emptyBlocks.push_back(blockAddress);
    }
}

//Toggles whether block is in use or not 
void DiskSimulator::updateMapTable(void* blockAddr)
{
    mapTable[blockAddr] = !mapTable[blockAddr];
    if (mapTable[blockAddr]) { // block is now unused
        emptyBlocks.push_back(blockAddr);
    }
}

//Returns the address of the first free block in the list of free blocks remaining and pop it
void* DiskSimulator::getUnusedBlock()
{
    void* blockAddr = emptyBlocks.front();
    emptyBlocks.pop_front();
    return blockAddr;
}