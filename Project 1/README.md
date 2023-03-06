# CZ4031-DBMS-project-1

## Running the program
To run the program, you can simply run the executable file DBMS, which should open up in command prompt. If not, please run either of the following commands in command prompt, with the working directory set to this directory: 
- <code>DBMS</code>
- <code>./DBMS</code>

Alternatively, you can compile the program yourself, by running the following command in command prompt:
- <code>g++ *.cpp -o DBMS -std=c++17</code>
- <code>./DBMS </code>

# Note on data.tsv
data.tsv must be placed in this directory for the program to read in the data records successfully.

# Experiment results
Experiment results are located under the folder <b>results</b>

# Note on C++ data sizes
The maximum number of records and keys that can fit in a data block and index node respectively varies based on the version of C++ used to compile the program. For example, compiling the program with a 32-bit C++ compiler will set the maximum number of keys in a B+ tree index node to be 6. The original DBMS.exe provided in this folder was compiled with a 32-bit C++ compiler; if you re-compiled the program, you may get different experiment results.
