#include <cstdlib>
#include <list>
#include "structures.h"
#include <iostream>
#include <math.h>
#include <fstream>

using namespace std;

class BPlusTree
{
    public:
    void *root;
    unsigned int height;
    unsigned int maxKeys;
    unsigned int sizeOfNode;

    //For Experiments
    unsigned int numNodes;
    unsigned int numOverflowNodes;
    int numIndexAccessed;
    int numNodesDeleted;
    int numOverflowNodesAccessed;
    int numOverflowNodesDeleted;

    //Initialisation and setting functions
    BPlusTree(unsigned int sizeOfNode);
    void* getNewNode(bool isLeaf, bool isOverflow);

    //Retrieval functions
    list<pointerBlockPair> findRecord(unsigned int numVotesStart, unsigned int numVotesEnd, ofstream &output);
    void* findNode(unsigned int numVotes, void* node, unsigned int currentHeight, ofstream &output, bool willPrint);

    //Functions for inserting a record
    void insertRecord(unsigned int numVotes, pointerBlockPair record);
    void splitLeafNode(unsigned int numVotes, pointerBlockPair record, void* nodeToSplit, pointerBlockPair* ptrArr, unsigned int* numVotesArr);
    void splitNonLeafNode(unsigned int numVotes, pointerBlockPair record, void* nodeToSplit, pointerBlockPair* ptrArr, unsigned int* numVotesArr);
    void updateParentNodeAfterSplit(void* parentNode, void* rightNode, unsigned int newParentKey);

    //Functions for deleting a record
    void deleteKey(unsigned int numVotes, void* nodeToDeleteFrom);
    void mergeNodes(void* leftNode, void* rightNode);
    void shiftElementsForward(unsigned int* numVotesArr, pointerBlockPair* ptrArr, int start, bool isLeaf);
    void shiftElementsBack(unsigned int* numVotesArr, pointerBlockPair* ptrArr, int end, bool isLeaf);

    //Functions for Experiments/Visualization
    int printIndexBlock(void* node, ofstream &output);
    void printRoot(ofstream &output);
    void printTree(ofstream &output);
};