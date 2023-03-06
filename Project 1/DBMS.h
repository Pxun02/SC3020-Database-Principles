#include <iostream>
#include <map>
#include <list>
#include <algorithm>
#include <set>
#include "DiskSimulator.h"
#include "BPlusTree.h"
#include "structures.h"
#include <string>
#include <fstream>

using namespace std;

class DBMS
{
    public:
    // Define global data available to whole DBMS
    int DISK_SIZE; // calculated in MB
    int BLOCK_SIZE; // calculated in B
    int MAX_RECORDS; // maximum number of movieRecords for a block
    int numRecords; // total number of records
    int numBlocks;

    list<void*> freeBlocks; // Allows for tracking of blocks that can still accomodate additional records
    BPlusTree* bPlusTree;
    DiskSimulator* disk; 
    void* initialBlockPtr;

    //Initialisation functions
    DBMS(unsigned int diskSize, unsigned int blockSize);
    ~DBMS();

    void importData(std::string tsv_file);
    void findRecords(unsigned int numVotesStart, unsigned int numVotesEnd, ofstream &output);
    void findRecordsBF(unsigned int numVotesStart, unsigned int numVotesEnd, ofstream &output);
    movieRecord* retrieveRecord(pointerBlockPair recordToRetrieve, set<void*> &accessedBlocks);
    movieRecord* retrieveRecordBF(list<void*> &accessedBlocks);
    void insertRecord(movieRecord toInsert);
    void deleteRecord(unsigned int numVotes, ofstream &output);
    void deleteRecordBF(unsigned int numVotes, ofstream &output);
    void deleteRecordFunc(pointerBlockPair recordToDelete);

    //Functions for Experiments/Visualization
    void printDataBlock(void* block, ofstream &output);
};