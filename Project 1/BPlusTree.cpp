#include "BPlusTree.h"

BPlusTree::BPlusTree(unsigned int nodeSize) {
    numNodes = 0;
    numOverflowNodes = 0;
    numIndexAccessed = 0;
    numOverflowNodesAccessed = 0;
    numNodesDeleted = 0;
    numOverflowNodesDeleted = 0;
    height = 0;

    // maxKeys = (size of a block - size of node's header - right most pointer) / (size of ptr-key pairs)
    sizeOfNode = nodeSize;
    const int sizeOfKeyPtrPair = (sizeof(pointerBlockPair) + sizeof(unsigned int)); 
    maxKeys = (nodeSize - sizeof(NodeHeader) - sizeof(pointerBlockPair)) / sizeOfKeyPtrPair;  
    root = getNewNode(true, false);
}

// Gets a new node from memory to be used as a node in the B+ Tree
// isOverflow used to determine whether to increment numOverflowNodes or numNodes
// isLeaf is also assigned for the node based on the input
void* BPlusTree::getNewNode(bool isLeaf, bool isOverflow) {
    void* addr = malloc(sizeOfNode);
    
    // Initialise header of the node
    NodeHeader* header;
    header = (NodeHeader*) addr;
    header->numKeys = 0; // First 4 bytes (size of int) is numOfRecords = 0 
    header->isLeaf = isLeaf;
    
    // Initialise the pointer to parent
    pointerBlockPair ptr;
    ptr.blockAddress = nullptr;
    ptr.recordID = -1;
    header->pointerToParent = ptr;
    
    // Initialise last pointer to null
    // Required for leaf nodes in case it is the last leaf node
    pointerBlockPair* ptrArr = (pointerBlockPair*) (((NodeHeader*) addr ) + 1 );
    unsigned int* numVotesArr = (unsigned int*) (ptrArr + maxKeys + 1);
    ptrArr[maxKeys] = {nullptr, -1};

    // Incrementing number of nodes created for the B+ Tree
    isOverflow ? numOverflowNodes++ : numNodes++;
     
    return addr;
}


// Print the contents of a specific index block in the B+ Tree
// Used for experiments
int BPlusTree::printIndexBlock(void* node, ofstream &output) {
    int numKeys = *(unsigned int*)node;
    pointerBlockPair* ptrArr = (pointerBlockPair*) (((NodeHeader*) node ) + 1 );
    unsigned int* numVotesArr = (unsigned int*) (ptrArr + maxKeys + 1);
    
    cout << " | ";
    if (output.is_open())
        output << " | ";
    char toPrint[24];
    for (int i=0; i<maxKeys; i++) {
        if (i < numKeys) {
            snprintf(toPrint, 24, "%6u | ", numVotesArr[i]);
        } else {
            snprintf(toPrint, 24, "%6s | ", "   ");
        }
        cout << toPrint;
        if (output.is_open())
            output << toPrint;
    }
    cout << endl;
    return numKeys;
}


// Queries a record/range of keys that is selected by the user
// This may return multiple pointerBlockPaires due to the possibility of multiple records having same key value of numVotes
// For querying of single value, set numVotesStart and numVotesEnd to both be the value
// Calls findNode() to locate the appropiate leaf node
list<pointerBlockPair> BPlusTree::findRecord(unsigned int numVotesStart, unsigned int numVotesEnd, ofstream &output) {
    
    numIndexAccessed = 0; 
    numOverflowNodesAccessed = 0;
    
    list<pointerBlockPair> results;
    void* currNode = findNode(numVotesStart, root, 0, output, false);
    
    unsigned int numKeys = *(unsigned int *)currNode;
    pointerBlockPair* ptrArr = (pointerBlockPair*) (((NodeHeader*) currNode ) + 1 );
    unsigned int* numVotesArr = (unsigned int*) (ptrArr + maxKeys + 1);

    int i = 0;

    // Continue iterating when key is smaller than search key and the current non-full node has not reached the end
    while ( i < numKeys && numVotesArr[i] <= numVotesEnd) { 
        if (numVotesArr[i] >= numVotesStart){   // Check if key is greater than starting key
            if (ptrArr[i].recordID == -1){ // If duplicates exist, need to traverse overflowNodes
                void* currOverflowNode = ptrArr[i].blockAddress;
                unsigned int numKeysOverflow;
                pointerBlockPair* ptrArrOverflow;

                // Traverse all overflowNodes and add their records into results
                while (currOverflowNode != nullptr){
                    numOverflowNodesAccessed++;
                    numKeysOverflow = *(unsigned int *)currOverflowNode;
                    ptrArrOverflow = (pointerBlockPair*) (((NodeHeader*) currOverflowNode ) + 1 );
                    for (int j = 0; j < numKeysOverflow; j++){
                        results.push_back(ptrArrOverflow[j]);
                    }                   
                    currOverflowNode = ptrArrOverflow[maxKeys].blockAddress;
                }
            } else { // Duplicate keys do not exist
                results.push_back(ptrArr[i]);
            }
        }

        //End of the current leaf node has been reached, need to traverse to next leaf node
        if (i == numKeys-1) {
            currNode = ptrArr[maxKeys].blockAddress; //Traverse to the next leaf node

            // Reset the search to start of the next leaf node
            ptrArr = (pointerBlockPair*) (((NodeHeader*)  currNode ) + 1 );
            numVotesArr = (unsigned int*) (ptrArr + maxKeys + 1);
            numKeys = *(unsigned int *)currNode;
            i = 0;            
            continue;
        }
        i++;
    }

    if (output.is_open()) {
        output << "\nTotal number of index nodes accessed: " << numIndexAccessed << "\n";
        output << "Total number of overflow index nodes accessed: " << numOverflowNodesAccessed << "\n";
        output << "Total number of index + overflow nodes accessed: " << (numOverflowNodesAccessed + numIndexAccessed) << "\n";
        cout << "\nTotal number of index nodes accessed: " << numIndexAccessed << "\n";
        cout << "Total number of overflow nodes accessed: " << numOverflowNodesAccessed << "\n";
        cout << "Total number of index + overflow nodes accessed: " << (numOverflowNodesAccessed + numIndexAccessed) << "\n";
    }
    return results;
}


// Finds the appropiate node to be used for retrieval/insertion/deletion
// Starts from the root node and recursively calls findNode() every time it goes down a level
// Terminating condition occurs when a leaf node is reached 
// This DOES NOT mean that the key is definitely present in the node, iteration through the node still needs to be done
void* BPlusTree::findNode(unsigned int numVotes, void* node, unsigned int currHeight, ofstream &output, bool willPrint) {

    numIndexAccessed++;

    // Print index blocks to screen and file
    if(willPrint){
        if (output.is_open()) {
            printIndexBlock(node, output);
            cout << endl;
            output << endl;
        }
    }

    // return terminal node
    if (currHeight == height) {
        return node;
    }
    
    pointerBlockPair* ptrArr = (pointerBlockPair*) (((NodeHeader*) node ) + 1 );
    unsigned int* numVotesArr = (unsigned int*) (ptrArr + maxKeys + 1);
    unsigned int numKeys = *((unsigned int*) node);

    for (int i = 0; i <= numKeys - 1; i++) {
        if (numVotes < numVotesArr[i]) { 
            return findNode(numVotes, ptrArr[i].blockAddress, ++currHeight, output, willPrint); // Search into pointer left of current index
        } else {
            if (i != numKeys - 1) {
                continue; // compare with next numVotes if the last numVotes has not been reached
            } else {
                return findNode(numVotes, ptrArr[i+1].blockAddress, ++currHeight, output, willPrint); // Search into pointer right of last numVotes
            }
        }
    }

    return nullptr;
}


// Inserts a key into the B+ Tree if it exists
// Accounts for duplicate keys and creates overflow nodes to hold duplicate keys if required
// Leaf nodes will only hold unique key values, which may have pointers to overflow nodes if mutliple records have the same index 
// Calls splitLeafNode() if number of keys exceeds the maximum number of keys the leaf node can hold
void BPlusTree::insertRecord(unsigned int numVotes, pointerBlockPair record) {

    ofstream dummy;
    void* nodeToInsertAt = findNode(numVotes, root, 0, dummy, true);
    int numKeys = *(unsigned int*)nodeToInsertAt;
    
    pointerBlockPair* ptrArr = (pointerBlockPair*) (((NodeHeader*) nodeToInsertAt ) + 1 );
    unsigned int* numVotesArr = (unsigned int*) (ptrArr + maxKeys + 1);

    // Check if there will be duplicate keys after the new record is inserted

    // Case 1: Duplicate key detected in the B+ tree
    for (int i = 0; i <= numKeys-1; i++) { 
        if (numVotes == numVotesArr[i]){
            // first check if the key alr has a overflow node
            void* overflowNode;
            pointerBlockPair* ptrArrO;
            unsigned int* numVotesArrO;
            unsigned int* numKeysO;
            
            if (ptrArr[i].recordID == -1) { //An overflowNode already exists
                overflowNode = ptrArr[i].blockAddress;
                ptrArrO = (pointerBlockPair*) (((NodeHeader*) overflowNode ) + 1 );

                // Traverse to the last overflow block by following the last pointer
                while (ptrArrO[maxKeys].blockAddress != nullptr) { 
                    overflowNode = ptrArrO[maxKeys].blockAddress;
                    ptrArrO = (pointerBlockPair*) (((NodeHeader*) overflowNode ) + 1 );
                }

                // Check if the last overflowNode has capacity for another duplicate record
                numKeysO = (unsigned int*) overflowNode;
                int posToInsert = 0;

                if (*numKeysO == maxKeys) { // last overflowNode is full, a new overflowNode needs to be created
                    void* newOverflowNode = getNewNode(true, true);                    
                    ptrArrO[maxKeys].blockAddress = newOverflowNode; // Link previous overflowNode to newOverflowNode 

                    // Reset pointer to newOverflowNode for insertion of key later
                    ptrArrO = (pointerBlockPair*) (((NodeHeader*) newOverflowNode ) + 1 ); 
                    numKeysO = (unsigned int*) newOverflowNode; 
                    (*numKeysO) = 1; // newOverflowNode will consist of 1 key 
                    ptrArrO[maxKeys] = {nullptr, -1}; // Set newOverflowNode to point to nullptr to indicate that it is the last overflowNode       

                } else { // last overflowNode has enough space; insert key into this overflowNode
                    posToInsert = *numKeysO;
                    (*numKeysO)++; // increment number of keys in overflowNode
                }         

                // Perform insertion of key into the correct overflowNode
                numVotesArrO = (unsigned int*) (ptrArrO + maxKeys + 1);
                ptrArrO[posToInsert] = record;
                numVotesArrO[posToInsert] = numVotes;   

            // Key currently added is the first duplicate, to create an overflow node and link it to the leaf node
            } else { 
                overflowNode = getNewNode(true, true);
                ptrArrO = (pointerBlockPair*) (((NodeHeader*) overflowNode ) + 1 );
                numVotesArrO = (unsigned int*) (ptrArrO + maxKeys + 1);
                numKeysO = (unsigned int*) overflowNode;

                // set first key-ptr pair of overflow block to point to the existing key and its record
                ptrArrO[0] = ptrArr[i]; 
                numVotesArrO[0] = numVotesArr[i];

                // set second key-ptr pair of overflow block to point to new (duplicate) key
                ptrArrO[1] = record; 
                numVotesArrO[1] = numVotes;

                ptrArr[i].blockAddress = overflowNode;
                ptrArr[i].recordID = -1;
                ptrArrO[maxKeys].blockAddress = nullptr;
                ptrArrO[maxKeys].recordID = -1;
                (*numKeysO) = 2; // Number of keys in overflowNode = 2 (1 for existing key, 1 for duplicate key)
            }
            return;
        } 
    }
    
    // Case 2: Unique key, but number of keys after insertion to node exceeds max number of keys allowed
    if (numKeys == maxKeys){
        splitLeafNode(numVotes, record, nodeToInsertAt, ptrArr, numVotesArr);
        return;
    }
    
    // Case 3: Unique key, and node has sufficient space to hold new key
    int i;
    for (i = 0; i <= numKeys-1; i++) { //Find position within node to insert key
        if (numVotes < numVotesArr[i]){
            for (int j = numKeys; j > i; j--) { // Shift current keys back to accomondate new key
                numVotesArr[j] = numVotesArr[j-1];
                ptrArr[j] = ptrArr[j-1];                
            }
            break;
        }
    }
    numVotesArr[i] = numVotes;
    ptrArr[i] = record;
    (*(unsigned int*)nodeToInsertAt)++; //Increment number of records in leaf node
}


// Deletes a key from the B+ Tree if it exists
// Accounts for deletion of key from both leaf and non-leaf nodes
// Initial deleting of a key always starts from a leaf node
// If merging was performed, deleteKey() may be called again in mergeNodes() for the parent node 
void BPlusTree::deleteKey(unsigned int numVotes, void* nodeToDeleteFrom) {

    unsigned int* numKeys = (unsigned int*)nodeToDeleteFrom;
    NodeHeader header = *(NodeHeader*) nodeToDeleteFrom;
    pointerBlockPair* ptrArr = (pointerBlockPair*) (((NodeHeader*) nodeToDeleteFrom ) + 1 );
    unsigned int* numVotesArr = (unsigned int*) (ptrArr + maxKeys + 1);
    
    // Search for node for deletion of key
    bool keyExists = false;
    int i;
    for (i=0; i<*numKeys; i++) {
        if (numVotesArr[i] == numVotes) {
            keyExists = true;
            break;
        }
    }

    // If record does not exist, deletion cannot be done
    if (!keyExists) {
        printf("Record with numVotes = %u doesn't exist!\n", numVotes);
        return;
    }
    
    // Declaration of minimum number of keys allowed depending on leaf or non-leaf node
    unsigned int minKeys = 0;
    if (header.isLeaf) {
        minKeys = (maxKeys+1)/2;
    } else {
        minKeys = maxKeys/2;
    }

    // Perform deletion of any overflow nodes first, if they exist
    if (ptrArr[i].recordID == -1) { // RecordID of -1 indicates that there is an overflow node
        void* overflowNode = ptrArr[i].blockAddress;
        pointerBlockPair* ptrArr;
        void* nextOverflow;
        while (overflowNode != nullptr) {
            numOverflowNodesDeleted++;
            ptrArr = (pointerBlockPair*) (((NodeHeader*) overflowNode ) + 1 );
            nextOverflow = ptrArr[maxKeys].blockAddress; // hold pointer nextOverflow before we free current overflow block
            free(overflowNode);
            numOverflowNodes--;
            overflowNode = nextOverflow; //Proceed to delete and free next overflowNode
        }
    }
    // Perform deletion of key from node
    (*numKeys)--;
    if (i != maxKeys-1) { //If element to remove is not the last element
        shiftElementsForward(numVotesArr, ptrArr, i, header.isLeaf);
    }

    void* parentNode = ((NodeHeader*)nodeToDeleteFrom)->pointerToParent.blockAddress;
    int numKeysInParent = *(unsigned int*) parentNode;
    pointerBlockPair* ptrArrParent = (pointerBlockPair*) (((NodeHeader*) parentNode ) + 1 );
    unsigned int* numVotesArrParent = (unsigned int*) (ptrArrParent + maxKeys + 1);
    
    // Check if the key to be deleted appears in any of its ancestors and find the node it is in
    // This will only occur if the key we are deleting is the smallest key in its index node
    if (i == 0) {
        void* recursiveParent = parentNode;
        bool foundFlag = false;
        while (recursiveParent != nullptr){
            int numKeysInRParent = *(unsigned int*) recursiveParent;
            pointerBlockPair* ptrArrRParent = (pointerBlockPair*) (((NodeHeader*) recursiveParent ) + 1 );
            unsigned int* numVotesArrRParent = (unsigned int*) (ptrArrRParent + maxKeys + 1);
            for (int k = 0; k < numKeysInParent; k++){
                if (numVotesArrRParent[k] == numVotes) {
                    numVotesArrRParent[k] = numVotesArr[0];
                    foundFlag = true;
                    break;
                }
            }
            if (foundFlag) break;
            else recursiveParent = ((NodeHeader*)recursiveParent)->pointerToParent.blockAddress;
        } 
    }
    
    // If the number of keys remaining is less than minimum keys allowed, borrowing/merging needs to be performed
    if (*numKeys < minKeys){  
        for (int ourPosInParent = 0; ourPosInParent <= numKeysInParent; ourPosInParent++){
            //find number of keys in sibling node
            //check if number- 1 is less than minkeys
            if(ptrArrParent[ourPosInParent].blockAddress == nodeToDeleteFrom){ // find our position in parentNode so we can identify our siblings
                void* sibling = nullptr;
                unsigned int* siblingNumKeys;
                bool borrowFromLeft;
                NodeHeader siblingHeader;

                // If left sibling exists, check if a key can be borrowed from left sibling
                if (ourPosInParent != 0) {                
                    sibling = ptrArrParent[ourPosInParent-1].blockAddress;
                    siblingNumKeys = (unsigned int*) sibling;
                    siblingHeader = *(NodeHeader*) sibling;
                    if (*siblingNumKeys - 1 < minKeys){ // Not possible to borrow from left sibling
                        sibling = nullptr;
                    } else { // borrow from left sibling, dont bother checking with right sibling
                        borrowFromLeft = true;
                    }
                }

                // If key cannot be borrowed from left sibiling/left sibling does not exist, check right sibling
                if (sibling == nullptr && ourPosInParent != numKeysInParent) { 
                    sibling = ptrArrParent[ourPosInParent+1].blockAddress;
                    siblingNumKeys = (unsigned int*) sibling;
                    siblingHeader = *(NodeHeader*) sibling;
                    if (*siblingNumKeys - 1 < minKeys){ // Not possible to borrow from right sibling
                        sibling = nullptr;
                    } else { // borrow from right sibling
                        borrowFromLeft = false;
                    }
                }
                
                // Perform borrowing if there exists a sibling that allows for borrowing
                if (sibling != nullptr){
                    pointerBlockPair* ptrArrSibling = (pointerBlockPair*) (((NodeHeader*) sibling ) + 1 );
                    unsigned int* numVotesArrSibling = (unsigned int*) (ptrArrSibling + maxKeys + 1);
                    if (borrowFromLeft){

                        // Borrow last key from left sibling
                        // Shift all elements in nodeToDeleteFrom to the right to make space for the new key
                        shiftElementsBack(numVotesArr, ptrArr, 0, siblingHeader.isLeaf);
                        numVotesArr[0] = numVotesArrSibling[(*siblingNumKeys)-1]; // Borrowing of key
                        ptrArr[0] = ptrArrSibling[(*siblingNumKeys)-1];
                        (*siblingNumKeys)--;
                        (*numKeys)++;

                        //Update the index in parent node that leads to this node
                        numVotesArrParent[ourPosInParent-1] = numVotesArr[0]; 
                    } else {

                        // Borrow first key from right sibling
                        numVotesArr[*numKeys] = numVotesArrSibling[0]; // Borrowing of key
                        ptrArr[*numKeys] = ptrArrSibling[0];
                        (*numKeys)++;

                        // Shift all elements in right sibling to fill up space due to key borrowed
                        shiftElementsForward(numVotesArrSibling, ptrArrSibling, 0, siblingHeader.isLeaf);

                        (*siblingNumKeys)--;

                        //update the index in parent node that leads to right sibling
                        numVotesArrParent[ourPosInParent] = numVotesArrSibling[0]; 
                    }

                // Borrowing from sibling, cannot be performed, to merge with sibling
                } else { 
                    if (ourPosInParent != 0){ // if not leftmost node, merge with left sibling
                        mergeNodes(ptrArrParent[ourPosInParent-1].blockAddress, nodeToDeleteFrom);
                    } else { // if leftmost node, merge with right sibling
                        mergeNodes(nodeToDeleteFrom, ptrArrParent[1].blockAddress);
                    }                    
                }

                break;
            }
        }
    } 

    // If deleting root node, the current node becomes the new root node
    if (nodeToDeleteFrom == root && *numKeys == 1){
        free(root);
        numNodes--;
        numNodesDeleted++;
        root = ptrArr[0].blockAddress;
    }
}


// Merges two nodes if number of keys is insufficient from the B+ Tree
// Merging occurs by keeping the left node, and deleting the right node
// Is called by deleteKey() if deletion results in nodes with insufficient keys
void BPlusTree::mergeNodes(void* leftNode, void* rightNode) {

    pointerBlockPair* ptrArrL = (pointerBlockPair*) (((NodeHeader*) leftNode ) + 1 );
    unsigned int* numVotesArrL = (unsigned int*) (ptrArrL + maxKeys + 1);

    pointerBlockPair* ptrArrR = (pointerBlockPair*) (((NodeHeader*) rightNode ) + 1 );
    unsigned int* numVotesArrR = (unsigned int*) (ptrArrR + maxKeys + 1);

    unsigned int smallestRight = numVotesArrR[0];
    unsigned int* numKeysL = (unsigned int*)leftNode;
    unsigned int* numKeysR = (unsigned int*)rightNode;

    // For each item in the right node, append to the left node
    for (int i=0; i<*numKeysR; i++) {
        numVotesArrL[*numKeysL+i] = numVotesArrR[i];
        ptrArrL[*numKeysL+i] = ptrArrR[i];
    }
    *numKeysL += *numKeysR;
    
    // For leaf nodes, the original left node should now point to the node pointed to by the original right node
    NodeHeader header = *(NodeHeader*) leftNode;
    if (header.isLeaf) {
        ptrArrL[maxKeys] = ptrArrR[maxKeys];
    }

    // Retrieve parent node for deletion of key
    void* parentNode = ((NodeHeader*)leftNode)->pointerToParent.blockAddress;

    free(rightNode);
    numNodes--;
    numNodesDeleted++;

    //Parent node that points to the original left and right node will have one less key
    //Key to be removed from parent node is always first key of original right node
    //Smallest right is passed in with the parent node
    deleteKey(smallestRight, parentNode);
}


// Splits leaf node into two nodes when a record is inserted
// New node will be to the right of the original node
// Original node is now the left node
// Calls updateParentNodeAfterSplit() to update keys in parent node
void BPlusTree::splitLeafNode(unsigned int numVotes, pointerBlockPair record, void* nodeToSplit, pointerBlockPair* ptrArr, unsigned int* numVotesArr) {

    void* leftNode = nodeToSplit;
    void* rightNode = getNewNode(true, false); // Create new right node

    list<pointerBlockPair> tempPtrList;
    list<unsigned int> tempNumVotesList;
    unsigned int numLeftKeys = ceil((maxKeys+1)/2.0);
    unsigned int numRightKeys = floor((maxKeys+1)/2.0);
    void* parentNode = ((NodeHeader*)nodeToSplit)->pointerToParent.blockAddress;

    // Copy existing keys into a temp list, and add in new key in correct position
    bool newKeyInserted = false;
    
    for (int i = 0; i < maxKeys; i++) {
        if (!newKeyInserted && numVotes < numVotesArr[i]){
            tempNumVotesList.push_back(numVotes);
            tempPtrList.push_back(record);
            newKeyInserted = true;
        }
        tempNumVotesList.push_back(numVotesArr[i]);
        tempPtrList.push_back(ptrArr[i]);
    }
    if (numVotes > numVotesArr[maxKeys-1]){ // Runs when new numNodes is bigger than all keys
        tempNumVotesList.push_back(numVotes);
        tempPtrList.push_back(record);
    }

    pointerBlockPair* ptrArrR = (pointerBlockPair*) (((NodeHeader*) rightNode ) + 1 );
    unsigned int* numVotesArrR = (unsigned int*) (ptrArrR + maxKeys + 1);
    
    // Filling in keys for new left node
    for (int i = 0; i < numLeftKeys; i++) {
        numVotesArr[i] = tempNumVotesList.front();
        ptrArr[i] = tempPtrList.front();
        tempNumVotesList.pop_front();
        tempPtrList.pop_front();
    }
    *((unsigned int*) leftNode) = numLeftKeys;
    
    // Filling in keys for new right node
    for (int i = 0; i < numRightKeys; i ++) {
        numVotesArrR[i] = tempNumVotesList.front();
        ptrArrR[i] = tempPtrList.front();
        tempNumVotesList.pop_front();
        tempPtrList.pop_front();
    }
    *((unsigned int*) rightNode) = numRightKeys;

    // Linking of leaf nodes
    // Original right node should now point to the node pointed to by the original left node
    // Left node should now point to the newly created right node
    ptrArrR[maxKeys].blockAddress = ptrArr[maxKeys].blockAddress;
    ptrArr[maxKeys].blockAddress = rightNode; 

    updateParentNodeAfterSplit(parentNode, rightNode, numVotesArrR[0]);
    
    return;
}


// Splits non-leaf node into two nodes when a record is inserted
// New node will be to the right of the original node
// Original node is now the left node
// Calls updateParentNodeAfterSplit() to update keys in parent node
void BPlusTree::splitNonLeafNode(unsigned int numVotes, pointerBlockPair record, void* nodeToSplit, pointerBlockPair* ptrArr, unsigned int* numVotesArr) {

    void* leftNode = nodeToSplit;
    void* rightNode = getNewNode(false, false); // Create new right node

    list<pointerBlockPair> tempPtrList;
    list<unsigned int> tempNumVotesList;
    unsigned int numLeftKeys = ceil(maxKeys/2.0); 
    unsigned int numRightKeys = floor(maxKeys/2.0);
    void* parentNode = ((NodeHeader*)nodeToSplit)->pointerToParent.blockAddress;

    // Copy existing keys into a temp list
    for (int i = 0; i < maxKeys; i++) {
        tempNumVotesList.push_back(numVotesArr[i]);
        tempPtrList.push_back(ptrArr[i]);
    }
    tempPtrList.push_back(ptrArr[maxKeys]);

    // Add in new key into the temp list in the correct position
    list<pointerBlockPair>::iterator ptrItr = tempPtrList.begin();
    list<unsigned int>::iterator numVotesItr = tempNumVotesList.begin();
    while (true) {
        if (numVotes < *numVotesItr) { 
            tempNumVotesList.insert(numVotesItr, numVotes); // insert adds new element to front of current iteration
            ptrItr++; 
            tempPtrList.insert(ptrItr, record);
            break;
        }
        if (numVotesItr == tempNumVotesList.end()) { // If index should be last element in node, just append to the back
            tempNumVotesList.push_back(numVotes);
            tempPtrList.push_back(record);
            break;
        }
        ptrItr++;
        numVotesItr++;
    }

    pointerBlockPair* ptrArrR = (pointerBlockPair*) (((NodeHeader*) rightNode ) + 1 );
    unsigned int* numVotesArrR = (unsigned int*) (ptrArrR + maxKeys + 1);
    
    // Filling in keys for new left node
    int i;
    for (i = 0; i < numLeftKeys; i++) {
        numVotesArr[i] = tempNumVotesList.front();
        ptrArr[i] = tempPtrList.front();
        ((NodeHeader*) ptrArr[i].blockAddress)->pointerToParent.blockAddress = leftNode; // update all children to point to leftNode as new parent
        tempNumVotesList.pop_front();
        tempPtrList.pop_front();
    }
    ptrArr[i] = tempPtrList.front(); // node needs 1 more ptr than key
    tempPtrList.pop_front(); //Pop the pointer after assigning it
    ((NodeHeader*) ptrArr[numLeftKeys].blockAddress)->pointerToParent.blockAddress = leftNode; // update last children to point to leftNode as new parent
    *((unsigned int*) leftNode) = numLeftKeys; //Update the number of keys for this left node


    // Filling in keys for new right node
    // The key currently at front of the list will be the parent for both left and right nodes
    // Thus the key is popped out and passed into updateParentNodeAfterSplit later for promotion
    unsigned int newParentKey = tempNumVotesList.front(); 
    tempNumVotesList.pop_front();

    for (i = 0; i < numRightKeys; i++) {
        numVotesArrR[i] = tempNumVotesList.front();
        ptrArrR[i] = tempPtrList.front();
        ((NodeHeader*) ptrArrR[i].blockAddress)->pointerToParent.blockAddress = rightNode; // update all children of right node to point to itself as new parent
        tempNumVotesList.pop_front();
        tempPtrList.pop_front();
    }    
    ptrArrR[numRightKeys] = tempPtrList.front(); // For non-leaf nodes, n keys requires (n+1) pointers, pop one more pointer
    tempPtrList.pop_front();
    ((NodeHeader*) ptrArrR[numRightKeys].blockAddress)->pointerToParent.blockAddress = rightNode; // update last children to point to itself as new parent
    *((unsigned int*) rightNode) = numRightKeys; // Update the number of keys for this right node

    updateParentNodeAfterSplit(parentNode, rightNode, newParentKey);

    return;
}


// Updates parent node after a split has been occured
// Is called by either splitLeafNode() or splitNonLeafNode()
// Can also call splitNonLeafNode() if the parent node ends up having insufficient number of keys
void BPlusTree::updateParentNodeAfterSplit(void* parentNode, void* rightNode, unsigned int newKey) { 

    //If root node is the node being split, we need to create a new root 
    if (parentNode == nullptr) {
        void* newRootNode = getNewNode(false, false); // create a parent node (root)

        pointerBlockPair* ptrArrNew = (pointerBlockPair*) (((NodeHeader*) newRootNode ) + 1 );
        unsigned int* numVotesArrNew = (unsigned int*) (ptrArrNew + maxKeys + 1);
        
        ptrArrNew[0].blockAddress = root; // old root node became the left node
        ptrArrNew[1].blockAddress = rightNode; 
        numVotesArrNew[0] = newKey; // only key in new root node is the smallest key of the right subtree
        (*((unsigned int*) newRootNode))++;

        // update parent of the new child nodes
        ((NodeHeader*) root)->pointerToParent.blockAddress = newRootNode; // this is the left node
        ((NodeHeader*) rightNode)->pointerToParent.blockAddress = newRootNode;

        if (((NodeHeader*) root)->isLeaf) { // left node (old root) needs to link to (new) right node
            pointerBlockPair* ptrArrRoot = (pointerBlockPair*) (((NodeHeader*) root) + 1 );
            ptrArrRoot[maxKeys].blockAddress = rightNode; // link leaf nodes together
        } 

        root = newRootNode; //Reinitialise new root
        height++; //Increment the variable storing the height of B++ tree
        
    } else { //there exists a parent node already
        int numKeys = *(unsigned int*) parentNode;
        
        //Initialise ptrArr and numVotesArr to access pointer and key arrays
        pointerBlockPair* ptrArr = (pointerBlockPair*) (((NodeHeader*) parentNode ) + 1 );
        unsigned int* numVotesArr = (unsigned int*) (ptrArr + maxKeys + 1);
        
        //parent node need to be split
        if (numKeys == maxKeys) {      
            pointerBlockPair addrToRightNode = {rightNode, -1};
            splitNonLeafNode(newKey, addrToRightNode, parentNode, ptrArr, numVotesArr);
        } else { // parent node don't need to split
            int i;
            for (i = 0; i <= numKeys-1; i++) { //Find position within node to insert key
                if (newKey < numVotesArr[i]){ // replaced smallestKey with newKey
                    ptrArr[numKeys+1] = ptrArr[numKeys]; //replace the last pointer first
                    for (int j = numKeys; j > i; j--) { // shift keys back to accomodate new key
                        numVotesArr[j] = numVotesArr[j-1];
                        ptrArr[j] = ptrArr[j-1];
                    }
                    break;
                }
            }
            numVotesArr[i] = newKey; //Insert the index value at specified location // replaced smallestKey with newKey
            ptrArr[i+1].blockAddress = rightNode; //Insert the pointer to the record in the disk at specified location
            (*(unsigned int*)parentNode)++; //Increment numRecords
            ((NodeHeader*) rightNode)->pointerToParent.blockAddress = parentNode; // right node's parent is the same as left node
        } 
    }
    
} 


// Shift all keys forward by one space
// Called by deleteKey() when borrowing elements
// Also called by deleteKey() when deleting the first key from the node
void BPlusTree::shiftElementsForward(unsigned int* numVotesArr, pointerBlockPair* ptrArr, int start, bool isLeaf) {

    if (isLeaf) {
        for (int j = start; j < maxKeys-1; j++) { // stop shifting at i=maxKeys-2 since numVotesArr[maxKeys-1] is the last key
            numVotesArr[j] = numVotesArr[j+1]; 
            ptrArr[j] = ptrArr[j+1]; 
        } 
    } else {
        for (int j = start; j < maxKeys-1; j++) { 
            numVotesArr[j] = numVotesArr[j+1];
            ptrArr[j+1] = ptrArr[j+2]; //For non-leaf node, the j-th key correspond to the (j+1)th pointer
        } 
    }
}


// Shift all keys backward by one space
// Called by deleteKey() when borrowing elements
void BPlusTree::shiftElementsBack(unsigned int* numVotesArr, pointerBlockPair* ptrArr, int end, bool isLeaf) {

    if (isLeaf) {
        for (int j = maxKeys-1; j > end; j--) {
            numVotesArr[j] = numVotesArr[j-1];
            ptrArr[j] = ptrArr[j-1];
        }
    } else {
        for (int j = maxKeys-1; j > end; j--) {
            numVotesArr[j] = numVotesArr[j-1];
            ptrArr[j+1] = ptrArr[j];
        }
    }
}


// Prints the current B+ Tree level by level
// Shows the keys currently in each node
void BPlusTree::printTree(ofstream &output) {

    list<void*> queue;
    int nodesInCurLevel = 1;
    int nodesInNextLevel = 0;
    int nodesPrinted = 0;
    queue.push_back(root);
    
    while (queue.size() != 0) {
        void* currNode = queue.front();
        queue.pop_front();

        // print currNode
        nodesInNextLevel += printIndexBlock(currNode, output) + 1;
        nodesPrinted++;
        if(nodesPrinted == nodesInCurLevel){
            nodesInCurLevel = nodesInNextLevel;
            nodesInNextLevel = 0;
            nodesPrinted = 0;
            cout<<"\n+++++++++++++++++++++++\n";
        } else {
            cout<<"--------";
        }
        
        NodeHeader* header = (NodeHeader*) currNode;
        unsigned int numKeys = header->numKeys;
        pointerBlockPair* ptrArr = (pointerBlockPair*) (((NodeHeader*) currNode ) + 1 );
        // add child nodes
        if (!(header->isLeaf)) {            
            for (unsigned int i=0; i<numKeys+1; i++) {
                queue.push_back(ptrArr[i].blockAddress);
            }
        }

    }

    cout << "\n===================\n";
}