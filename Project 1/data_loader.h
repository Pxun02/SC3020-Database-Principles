// fstream header file for ifstream, ofstream, fstream classes
#include <fstream> 
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

// convert string to an array
#include <cstring>
#include "structures.h"

class DataLoader{
	public:

	// Loads tsv file into a vector consists of custom data structure MovieRecord
	// Example:
	//	DataLoader data_loader = DataLoader();
	//	std::vector data = DataLoader.Loadtsv(name_of_tsv_file);
	std::vector<movieRecord> loadTSV(std::string tsv_file);
};