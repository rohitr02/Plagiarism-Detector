#include "compare.h"

int main(int argc, char* argv[]){
    /* Input Validity */
    if(argc < 2) return EXIT_FAILURE;                   // No arguments entered

    bool errorInProgram = false;                        // Flag for the overall program. If for any reason a non-vital error is thrown set this to true and return EXIT_FAILURE at the end

    // Optional Parameters
    // int numOfDirectoryThreads = 1;
    // int numOfFileThreads = 1;
    // int numOfAnalysisThreads = 1;
    // char* fileNameSuffix = ".txt";
    
    
    // Read the inputs
    // for(int i = 1; i < argc; i++){
        
    // }


    if(errorInProgram == true)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}