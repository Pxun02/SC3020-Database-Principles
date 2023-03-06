#include "DBMS.h"
#include <fstream>
#include <cstring>

void displayOptions() {
  cout << "1) Run Experiment 1\n"
          "2) Run Experiment 2\n"
          "3) Run Experiment 3\n"
          "4) Run Experiment 4\n"
          "5) Run Experiment 5\n"
          "6) Exit program\n";
}

int main()
{
    int choice;
    char choice_5;
    DBMS* dbms;

    ofstream exp1Output;
    ofstream exp2Output;
    ofstream exp3Output;
    ofstream exp4Output;
    ofstream exp5Output;

    const unsigned int blockSize = 200;
    // Using disk capacity of 100MB
    unsigned int diskSize = 100;
    string resultsDir = "results/";;
    dbms = new DBMS(diskSize, blockSize);
    
    do {
        // Display the options list
        displayOptions();

        // Get user input to run experience
        while (true) {
            cout << "Enter your choice: ";
            if (cin >> choice) {
                // input is an integer
                cout << endl;
                break;
            } else {
                // input is not an integer, clear the input stream and ignore the input
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter an integer." << endl;
            }
        }

        // Perform actions based on the user's choice
        switch (choice) {
            case 1:
                // Exp1: store the data (which is about IMDb movives and described in Part 4) on the disk (as specified in Part 1) and report the following statistics
                // The number of records
                // The size of a record
                // The number of records stored in a block
                // The number of blocks for storing the data
                cout << "-----Running Experiment 1-----" <<endl;  
                dbms->importData("data.tsv");
                exp1Output.open(resultsDir + "experiment_1.txt");

                // Write output to file
                exp1Output << "Number of records: " << dbms->numRecords << endl;
                exp1Output << "Size of a record: " << (sizeof(movieRecord)) << "-Byte" << endl;
                exp1Output << "Number of records stored in a block: " << ceil((blockSize - sizeof(unsigned int))/ (sizeof(movieRecord) + sizeof(indexMapping))) << endl; 
                exp1Output << "Number of blocks for storing the data: " << dbms->numBlocks << endl;

                // print to screen
                // cout << "Number of records: " <<  dbms->numRecords << endl; // Already printed when import data
                cout << "Size of a record: " << (sizeof(movieRecord)) << "-Byte" << endl;
                cout << "Number of records stored in a block: " << ceil((blockSize - sizeof(unsigned int))/ (sizeof(movieRecord) + sizeof(indexMapping))) << endl;
                cout << "Number of blocks for storing the data: " << dbms->numBlocks << endl;
                exp1Output.close();
                break;
            case 2:
                // Exp2: build a B+ tree on the attribute "numVotes" by inserting the records sequentially and report the following statistics:
                // The parameter n of the B+ tree
                // The number of nodes of the B+ tree
                // The number of levels of the B+ tree
                cout << "-----Running Experiment 2-----" <<endl;  
                exp2Output.open(resultsDir + "experiment_2.txt");

                // Write output to file
                exp2Output << "Parameter n of the B+ Tree: " << dbms->bPlusTree->maxKeys << "\n";
                exp2Output << "Number of nodes (excluding overflow): " << dbms->bPlusTree->numNodes << "\n";
                exp2Output << "Number of overflow nodes: " << dbms->bPlusTree->numOverflowNodes << "\n";
                exp2Output << "Total Number of nodes (including overflow): " << (dbms->bPlusTree->numNodes + dbms->bPlusTree->numOverflowNodes) << "\n";
                exp2Output << "Number of levels of the B+ tree: " << dbms->bPlusTree->height+1 << "\n"; // DBMS starts height at 0

                // print to screen     
                cout << "Parameter n of the B+ Tree: " << dbms->bPlusTree->maxKeys << "\n";
                cout << "Number of nodes (excluding overflow): " << dbms->bPlusTree->numNodes << "\n";
                cout << "Number of overflow nodes: " << dbms->bPlusTree->numOverflowNodes << "\n";
                cout << "Total Number of nodes (including overflow): " << (dbms->bPlusTree->numNodes + dbms->bPlusTree->numOverflowNodes) << "\n";
                cout << "Height of B+ Tree: " << dbms->bPlusTree->height+1 << "\n"; // DBMS starts height at 0
                cout << "\n=====Content of root=====" << endl;
                cout << "Root: \n";
                exp2Output << "Root: \n";
                dbms->bPlusTree->printIndexBlock(dbms->bPlusTree->root, exp2Output);
                exp2Output.close();
                break;  
            case 3:
                // Exp 3: retrieve those movies with the “numVotes” equal to 500 and report the following statistics:
                // The number of index nodes the process accesses
                // The number of data blocks the process accesses
                // The average of “averageRating’s” of the records that are returned
                // The running time of the retrieval process (measured by chrono::system_clock)
                // The number of data blocks that would be accessed by a brute-force linear scan method and its running time (for comparison)
                cout << "-----Running Experiment 3-----" <<endl;  
                exp3Output.open(resultsDir + "experiment_3.txt");

                dbms->findRecords(500, 500, exp3Output);
                dbms->findRecordsBF(500, 500, exp3Output);
                exp3Output.close();
                break;
            case 4:
                // Exp 4: retrieve those movies with the attribute “numVotes” from 30,000 to 40,000, both inclusively and report the following statistics:
                // The number of index nodes the process accesses
                // The number of data blocks the process accesses
                // The average of “averageRating’s” of the records that are returned
                // The running time of the retrieval process
                // The number of data blocks that would be accessed by a brute-force linear scan method and its running time (for comparison)
                cout << "-----Running Experiment 4-----" <<endl;  
                exp4Output.open(resultsDir + "experiment_4.txt");

                dbms->findRecords(30000, 40000, exp4Output);
                dbms->findRecordsBF(30000, 40000, exp4Output);
                exp4Output.close();
                break;
            case 5:
                // Exp 5: delete those movies with the attribute “numVotes” equal to 1,000, update the B+ tree accordingly, and report the following statistics:
                // The number nodes of the updated B+ tree
                // The number of levels of the updated B+ tree
                // The content of the root node of the updated B+ tree(only the keys); running time of the process;
                // The number of data blocks that would be accessed by a brute-force linear scan method and its running time (for comparison)
                cout << "-----Running Experiment 5-----" <<endl;  
                exp5Output.open(resultsDir + "experiment_5.txt");
                 do{
                    cout << "Delete by.." << endl;
                    cout << "a) B+ Tree" << endl;
                    cout << "b) Brute-force" << endl;
                    cout << "Enter your choice: ";
                    cin >> choice_5;
                    switch(choice_5){
                        case('a'):
                            dbms->deleteRecord(1000, exp5Output);
                            // write optput to file          
                            exp5Output << "Number of times an index node (excluding overflow) is deleted: " << dbms->bPlusTree->numNodesDeleted << endl;
                            exp5Output << "Number of times an overflow node is deleted: " << dbms->bPlusTree->numOverflowNodesDeleted << endl;
                            exp5Output << "Number of nodes (excluding overflow): " << dbms->bPlusTree->numNodes << "\n";
                            exp5Output << "Number of overflow nodes: " << dbms->bPlusTree->numOverflowNodes << "\n";
                            exp5Output << "Height of B+ Tree: " << dbms->bPlusTree->height+1 << "\n";
                            exp5Output << "\n=====Content of root and first child=====" << endl;

                            // print to screen
                            cout << "Number of times an index node (excluding overflow) is deleted: " << dbms->bPlusTree->numNodesDeleted << endl;
                            cout << "Number of times an overflow node is deleted: " << dbms->bPlusTree->numOverflowNodesDeleted << endl;
                            cout << "Number of nodes (excluding overflow): " << dbms->bPlusTree->numNodes << "\n";
                            cout << "Number of overflow nodes: " << dbms->bPlusTree->numOverflowNodes << "\n";
                            cout << "Height of B+ Tree: " << dbms->bPlusTree->height+1 << "\n";
                            cout << "\n=====Content of root and first child=====" << endl;

                            // print to screen and write to file
                            dbms->bPlusTree->printIndexBlock(dbms->bPlusTree->root, exp5Output);
                            break;
                        
                        case('b'):
                            // brute-force
                            dbms->deleteRecordBF(1000, exp5Output);
                            break;

                        default:
                            cout << "Invalid option selected\n";
                            break;
                    }
                }while(choice_5!='a' && choice_5!='b');
                exp5Output.close();
                break;
            case 6:
                cout << "Exiting...";
                break;
            default:
                cout << "Invalid option selected\n";
        }

        // Add a line break for readability
        cout << endl;

    } while(choice != 6);

    return 0;
}