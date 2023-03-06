#include "data_loader.h"

// Input:
// 	* tsv_file: the name of tsv_file in string format

// Output:
// 	* data: a vector consists of custom data structure movieRecord

// A simple example of using this DataLoader Class:
// DataLoader data_loader = DataLoader();
// data = data_loader.Loadtsv(tsv_file);
std::vector<movieRecord> DataLoader::loadTSV(std::string tsv_file)
{
    // Creation of ifstream class object to read the file
    std::ifstream fin;

    std::string line;
    fin.open(tsv_file);

    // Create the vector to store all the movie records from the file
    std::vector<movieRecord> data = {};

    int recordID = 1;
    bool isFirstLine = true;

    // Execute a loop until EOF(end of file)
    while (!fin.eof()){

        // Read a line from File
        getline(fin, line);

        //Skip the header of the tsv file
        if (isFirstLine){
            isFirstLine = false;
            continue;
        }
        
        //Prevents adding of empty lines
        if (line.empty()){
            continue;
        }

        // create a movieRecord to store the fields data later
        movieRecord record = {};

        // initialize the recordID
        record.recordID = recordID;

        // use separator to read parts of the line 
        // fields contained in one line are: tconst, averageRating and numVotes
        std::istringstream read_fields;
        read_fields.str(line);

        // field_index to indicate current accessing field
        // 0 is tconst, 1 is averageRating, 2 is numVotes
        int field_index = 0;

        for (std::string line; std::getline(read_fields, line,'\t'); ){
            if (field_index % 3 == 0){
                // copy the contents of the string to char array
                std::strcpy(record.tconst, line.c_str());
                field_index = field_index + 1;

            }
            else if(field_index % 3 == 1){
                // convert the string to float value
                float averageRating = std::atof(line.c_str());

                record.averageRating = averageRating;
                field_index = field_index + 1;
            }
            else if(field_index % 3 == 2){
                unsigned int numVotes = std::stoul(line.c_str());

                record.numVotes = numVotes;
                field_index = field_index + 1;
            }
            else{
                std::cout << "Error when increasing the field_index." << "\n";
            }

        }

        data.push_back(record);

        // increase the recordID by 1
        recordID = recordID + 1;

        // for debugging
        // std::cout << "Record ID is: " << recordID << '\n';
        // std::cout << "Record tconst is: " << record.tconst << "\n";
        // std::cout << "Record averageRating is: " << record.averageRating << '\n';
        // std::cout << "Record numVotes is: " << record.numVotes << '\n';

    }

    // Close the file
    fin.close();

    return data;
}