#include "DBMS.h"
#include "data_loader.h"
#include <chrono>

DBMS::DBMS(unsigned int diskSize, unsigned int blockSize)
{
    DISK_SIZE = diskSize; // calculated in MB
    BLOCK_SIZE = blockSize; // calculated in B
    MAX_RECORDS = (BLOCK_SIZE - sizeof(unsigned int))/(sizeof(movieRecord) + sizeof(indexMapping)); // maximum number of movieRecords for a block

    freeBlocks = {}; // Allows for tracking of blocks that can still accomodate additional records
    disk = new DiskSimulator(DISK_SIZE, BLOCK_SIZE);
    bPlusTree = new BPlusTree(BLOCK_SIZE);
    numBlocks = 0;
}

DBMS::~DBMS() {
    free(disk);
    free(bPlusTree);
}

// Imports record data from the tsv file
void DBMS::importData(std::string tsv_file)
{
    std::vector<movieRecord> data = {};
    DataLoader data_loader = DataLoader();

    cout << "Reading in TSV file, please wait..." << endl;
    data = data_loader.loadTSV(tsv_file);
    cout << "Size: " << data.size() << endl;

    this->numRecords = 0;

    // Loop over the data and insert all the movie records
    for (auto movie_record_address = data.begin();
         movie_record_address != data.end();
         ++movie_record_address)
    {
        insertRecord(*movie_record_address);  
        this->numRecords++;
        if (this->numRecords % 10000 == 0){ // Update user for each 10,000 records entered
            cout << "Number of records inserted thus far: " << this->numRecords << endl;
        }
    }
    cout << "\n============\n"
        "Total number of records inserted: " << this->numRecords << endl;
}


// Inserts a movieRecord and updates the B+ Tree
void DBMS::insertRecord(movieRecord toInsert)
{
    // note that checking if record is already inserted should be done in the B+ tree implementation
    void* blockAddress;
    void* blockToInsert;
    unsigned int* numRecords;
    indexMapping* indexMappingTable;

    //Retrieve a block for insertion of record, get new block from disk if all blocks are fully filled
    if (freeBlocks.size() == 0) {
        // no free blocks, get a new one and initialize header information
        blockAddress = disk->getUnusedBlock();
        disk->updateMapTable(blockAddress);
        numBlocks++;
        freeBlocks.push_front(blockAddress);
        blockToInsert = blockAddress;
        numRecords = (unsigned int*)blockToInsert;
        *numRecords = 0; // initialize first 4 bytes to be 0
    }
    else {
        blockAddress = freeBlocks.front();        
        blockToInsert = blockAddress;
        numRecords = (unsigned int*)blockToInsert;
    }

    if(initialBlockPtr==nullptr)    initialBlockPtr = blockToInsert;
    
    indexMappingTable = (indexMapping*)(numRecords + 1); // pointer to start of indexMapping table, starts directly after numRecords
    movieRecord* tail = (movieRecord*)((char*)blockToInsert + BLOCK_SIZE - sizeof(movieRecord)); // pointer to record slot at bottom of the block

    // Setting insertRecordPointer to point to the space for insertion of record
    // Records are inserted starting from the back of the block
    movieRecord* insertRecordPointer = tail - *numRecords;
    indexMapping* insertindexMappingPointer = indexMappingTable + *numRecords;

    // Search for gravestones which indicate free spaces within the blocks for record insertion
    int numGravestones = 0;
    int index = *numRecords;
    for(unsigned int i = 0; i<*numRecords; i++)
    {
        if ((indexMappingTable + i)->indexOfRecord == -1)
        {
            // Replace last gravestone
            insertRecordPointer = tail - i;
            insertindexMappingPointer = indexMappingTable + i;
            index = i;
            numGravestones++;
        }
    }

    if (numGravestones > 0) 
    {
        numGravestones--; // revive a single gravestone
        (*numRecords)--; // to keep numRecords unchanged when it is incremented later
    }
    
    // Insert record to disk
    *insertRecordPointer = toInsert; // insert record data
    *insertindexMappingPointer = {toInsert.recordID, index}; // insert new indexMapping table entry
    (*numRecords)++;

    // Update B+ Tree with new record inserted
    bPlusTree->insertRecord(toInsert.numVotes, {blockAddress, (int) toInsert.recordID});

    // Remove block from list of freeblocks if updated block cannot hold any more records
    if (*numRecords == MAX_RECORDS && numGravestones == 0)
    {
        freeBlocks.pop_front();
    }

    return;
}

// Finds records, used for both range queries and single value queries
// For single value query, numVotesStart and numVotesEnd to be set as the same
void DBMS::findRecords(unsigned int numVotesStart, unsigned int numVotesEnd, ofstream &output) {
    printf("Retrieving records from disk..\n");

    // clock starts
    chrono::system_clock::time_point start, end;
    start = chrono::system_clock::now();

    list<pointerBlockPair> results = bPlusTree->findRecord(numVotesStart, numVotesEnd, output);

    set<void*> accessedBlocks; // to avoid counting and printing blocks more than once

    float sumOfAverageRating = 0;

    for (pointerBlockPair recordLocation : results) {
        movieRecord* record = retrieveRecord(recordLocation, accessedBlocks);
        sumOfAverageRating += record->averageRating;
    }
    // clock ends
    end = chrono::system_clock::now();
    double elapsed = chrono::duration_cast<chrono::microseconds>(end-start).count();

    float average = sumOfAverageRating / results.size();

    // print to file
    output << "\nTotal number of records retrieved: " << results.size() << "\n";
    output << "Total number of data blocks the process accessed: " << accessedBlocks.size() << "\n";
    output << "The average of 'averageRating' of the records: " << average << "\n";
    output << "The running time of the retrieval process (measured by chrono::system_clock): " << elapsed / 1000 <<  " ms" << "\n";
    
    // print to screen
    cout << "Total number of records retrieved: " << results.size() << "\n";
    cout << "Total number of data blocks the process accessed: " << accessedBlocks.size() << "\n";
    cout << "The average of 'averageRating' of the records: " << average << "\n";
    cout << "The running time of the retrieval process (measured by chrono::system_clock) =  " << elapsed / 1000 <<  " ms" << "\n";
}

// Brute-force linear scan method
// Only print the number of data blocks accessed
// For Experiment 3, 4, 5
void DBMS::findRecordsBF(unsigned int numVotesStart, unsigned int numVotesEnd, ofstream &output){
    cout << "\nScan through a brute-force linear method..." << "\n\n";

    int numOfBlockAccessed = 0;
    int numOfNumVotes = 0;
    set<void*> accessedBlocks; // to avoid counting and printing blocks more than once
    double sumOfAverageRating = 0;

    // clock starts
    chrono::system_clock::time_point start, end;
    start = chrono::system_clock::now();

    for(int blockID=0;blockID<numBlocks;blockID++){
        numOfBlockAccessed++;
        void* blockPtr = (void*)((char*)this->initialBlockPtr + blockID * BLOCK_SIZE);

        for(int recID=1;recID<=MAX_RECORDS;recID++){
            pointerBlockPair recordLocation = {blockPtr, recID + blockID * MAX_RECORDS};
            movieRecord* record = retrieveRecord(recordLocation, accessedBlocks);
            if(record==nullptr) break;
            if(record->numVotes >= numVotesStart && record->numVotes <= numVotesEnd){
                numOfNumVotes++;
                sumOfAverageRating += record->averageRating;
            }
        }
    }

    // clock ends
    end = chrono::system_clock::now();
    double elapsed = chrono::duration_cast<chrono::microseconds>(end-start).count();


    output << "The number of data blocks accessed if a brute-force linear scan used: " << numOfBlockAccessed << "\n";
    output << "The running time of the retrieval process (measured by chrono::system_clock): " << elapsed / 1000 <<  " ms" << "\n";
    cout << "The number of data blocks accessed if a brute-force linear scan is used: " << numOfBlockAccessed << "\n";
    cout << "The running time of the retrieval process (measured by chrono::system_clock) =  " << elapsed / 1000 <<  " ms" << "\n";
    cout << "***Number of records retrieved: " << numOfNumVotes << endl;
    cout << "***Average Rating: " << sumOfAverageRating / numOfNumVotes << endl;
}

//Retrieves record, used in findRecords() to get the records required from disk
movieRecord* DBMS::retrieveRecord(pointerBlockPair recordToRetrieve, set<void*> &accessedBlocks){

    void* blockToRetrieve = recordToRetrieve.blockAddress;

    accessedBlocks.insert(blockToRetrieve);

    unsigned int* numOfRecords = (unsigned int*)blockToRetrieve;
    indexMapping* indexMappingTable = (indexMapping*)(numOfRecords + 1); // Pointer to start of indexMapping table, starts directly after numRecords
    movieRecord* tail = (movieRecord*)((char*)blockToRetrieve + BLOCK_SIZE - sizeof(movieRecord)); // Pointer to start of record slot at bottom of the block
    
    for (int i = 0; i <= *numOfRecords; i++){
        if (indexMappingTable->recordID == recordToRetrieve.recordID && indexMappingTable->indexOfRecord != -1){
            movieRecord* recordPointer = tail - indexMappingTable->indexOfRecord;
            return recordPointer;
        } else {
            indexMappingTable++;
        }
    }

    return nullptr;
} 

//Prints tconst values of records within data block, used for reporting statistics for experiments
void DBMS::printDataBlock(void* block, ofstream &output) {

    int numRecords = *((unsigned int*) block);

    indexMapping* indexMappingTable = (indexMapping*)(((unsigned int*)block) + 1); // Pointer to start of indexMapping table, starts directly after numRecords
    movieRecord* tail = (movieRecord*)((char*) block + BLOCK_SIZE - sizeof(movieRecord)); // Pointer to start of record slot at bottom of the block

    cout << " | ";
    char toPrint[24];
    for (int i=0; i<MAX_RECORDS; i++) {
        if (i < numRecords) {
            snprintf(toPrint, 24, "%12s | ", (tail-i)->tconst);
        } else {
            snprintf(toPrint, 24, "%12s | ", "            ");
        }
        output << toPrint;
        cout << toPrint;
    }
    cout << "\n";
    output << "\n";
}

// Deletes records with a certain number of votes and updates the B+ Tree
void DBMS::deleteRecord(unsigned int numVotes, ofstream &output)
{
    printf("Deleting record from disk...\n");

    ofstream dummy;

    // clock starts
    chrono::system_clock::time_point start, end;
    start = chrono::system_clock::now();

    list<pointerBlockPair> recordsToDelete = bPlusTree->findRecord(numVotes, numVotes, dummy);
    int numOfBlockAccessed = 0;
    
    //Start deleting records from disk
    for (pointerBlockPair recordToDelete : recordsToDelete) {
        numOfBlockAccessed++;
        deleteRecordFunc(recordToDelete);       
    }

    // clock ends
    end = chrono::system_clock::now();
    double elapsed = chrono::duration_cast<chrono::microseconds>(end-start).count();

    printf("Record(s) successfully deleted from disk!\n");

    //Updating B+ Tree after deletion
    printf("Updating B+ Tree Index...\n");
    void* nodeToDeleteFrom = bPlusTree->findNode(numVotes, bPlusTree->root, 0, dummy, true);
    bPlusTree->deleteKey(numVotes, nodeToDeleteFrom);
    
    printf("B+ Tree Index successfully updated!\n");

    output << "The number of data blocks accessed if B+ Tree is used: " << numOfBlockAccessed << "\n";
    output << "The running time of the retrieval process (measured by chrono::system_clock) =  " << elapsed / 1000 <<  " ms" << "\n";
    cout << "The number of data blocks accessed if B+ Tree is used: " << numOfBlockAccessed << "\n";
    cout << "The running time of the retrieval process (measured by chrono::system_clock) =  " << elapsed / 1000 <<  " ms" << "\n";
}

void DBMS::deleteRecordBF(unsigned int numVotes, ofstream &output){
    printf("Deleting record from disk in a brute-force linear scan...\n");

    int numOfBlockAccessed = 0;
    set<void*> accessedBlocks; 
    list<pointerBlockPair> recordsToDelete;

    // clock starts
    chrono::system_clock::time_point start, end;
    start = chrono::system_clock::now();

    // make a list of pointers of delete records
    for(int blockID=0;blockID<numBlocks;blockID++){
        numOfBlockAccessed++;
        void* blockPtr = (void*)((char*)this->initialBlockPtr + blockID * BLOCK_SIZE);

        for(int recID=1;recID<=MAX_RECORDS;recID++){
            pointerBlockPair recordLocation = {blockPtr, recID + blockID * MAX_RECORDS};
            movieRecord* record = retrieveRecord(recordLocation, accessedBlocks);
            if(record==nullptr) break;
            if(record->numVotes==numVotes)
                recordsToDelete.push_back(recordLocation);
        }
    }
    for (pointerBlockPair recordToDelete: recordsToDelete)
        deleteRecordFunc(recordToDelete);

    // clock ends
    end = chrono::system_clock::now();
    double elapsed = chrono::duration_cast<chrono::microseconds>(end-start).count();

    printf("Record(s) successfully deleted from disk!\n");

    output << "The number of data blocks accessed if a brute-force linear scan is used: " << numOfBlockAccessed << "\n";
    output << "The running time of the retrieval process (measured by chrono::system_clock) =  " << elapsed / 1000 <<  " ms" << "\n";
    cout << "The number of data blocks accessed if a brute-force linear scan is used: " << numOfBlockAccessed << "\n";
    cout << "The running time of the retrieval process (measured by chrono::system_clock) =  " << elapsed / 1000 <<  " ms" << "\n";
}

void DBMS::deleteRecordFunc(pointerBlockPair recordToDelete){
    void* blockToRetrieve = recordToDelete.blockAddress;
    unsigned int* numOfRecords = (unsigned int*)blockToRetrieve;
    indexMapping* indexMappingTable = (indexMapping*)(numOfRecords + 1);
    for (int i = 0; i <= *numOfRecords; i++){
        if (indexMappingTable->recordID == recordToDelete.recordID){
            //Set gravestone and decrement number of records in block
            indexMappingTable->indexOfRecord = -1;
            indexMappingTable->recordID = 0;
            (*numOfRecords)--;
            
            //Iterate through list of free blocks, if not previously inside, then add it in
            list<void*>::iterator iter = find(freeBlocks.begin(), freeBlocks.end(), recordToDelete.blockAddress);
            if (*iter != recordToDelete.blockAddress){ //Previously full block, now can accommodate record, add to freeBlocks
                freeBlocks.push_front(recordToDelete.blockAddress);
            } 
            // Update map table to indicate block is free if there are no records inside anymore
            else if (*numOfRecords == 0) { 
                disk->updateMapTable(blockToRetrieve);
            }
            break;
        } else {
            indexMappingTable++;
        }
    }    
}
