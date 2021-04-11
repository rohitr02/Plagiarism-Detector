#include "compare.h"

int main(int argc, char* argv[]){
    /* Input Validity */
    if(argc < 2) return EXIT_FAILURE;                   // No arguments entered
    bool errorInProgram = false;                        // Flag for the overall program. If for any reason a non-vital error is thrown set this to true and return EXIT_FAILURE at the end


    // Optional Parameters
    int numOfDirThreads = 1;
    int numOfFileThreads = 1;
    int numOfAnalysisThreads = 1;
    char* fileNameSuffix = malloc(5 * sizeof(char));
    if(fileNameSuffix == NULL){                         // Malloc Failure
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    strcpy(fileNameSuffix, ".txt");


    // Initializing the file and directory queues
    Queue* fileQueue = initQueue(NULL);
    if(fileQueue == NULL){                              // Malloc Failure
        free(fileNameSuffix);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    Queue* dirQueue = initQueue(NULL);
    if(dirQueue == NULL){                               // Malloc Failure
        fileQueue = destroyQueue(fileQueue);
        free(fileNameSuffix);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    

    // Read the inputs
    for(int i = 1; i < argc; i++){
        if(isRegFile(argv[i]) == true){             // Check if it is a regular file and enqueue directly to fileQueue
            enqueue(fileQueue, argv[i]);
        }else if(isDir(argv[i]) == true){           // Check if it is a directory and enqueue directly to dirQueue
            enqueue(dirQueue, argv[i]);
        }else if(argv[i][0] == '-'){                // Check if it is a optional argument and update the optional argument if it is. Otherwise return failure if incomplete or invalid option.
            if(setOptionalParameter(argv[i], &numOfDirThreads, &numOfFileThreads, &numOfAnalysisThreads, &fileNameSuffix) == false){
                free(fileNameSuffix);
                fileQueue = destroyQueue(fileQueue);
                dirQueue = destroyQueue(dirQueue);
                return EXIT_FAILURE;
            }
        }else{
            fprintf(stderr, "%s %s\n", argv[i], "is an invalid argument.");
        }
    }
    

    // Checking if everything is correct
    if(DEBUG == true){
        printf("%d %d %d %s\n", numOfDirThreads, numOfFileThreads, numOfAnalysisThreads, fileNameSuffix);
        printQueue(fileQueue);
        printQueue(dirQueue);
    }


    // Create variables to store thread IDs and the arguments to pass to each thread
    pthread_t* dirThreadIDs = malloc(numOfDirThreads * sizeof(pthread_t));
    if(dirThreadIDs == NULL){
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    thread_args* args = malloc(numOfDirThreads * sizeof(thread_args));
    if(args == NULL){
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        free(dirThreadIDs);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    
    // Loop through the number of threads to create and make them
    for(int i = 0; i < numOfDirThreads; i++){
        args[i].dirQueue = dirQueue;
        args[i].fileQueue = fileQueue;
        args[i].id = i;
        args[i].exitCode = EXIT_SUCCESS;
        args[i].fileSuffix = fileNameSuffix;
        if(pthread_create(&dirThreadIDs[i], NULL, readDirectory, &args[i]) != 0){
            perror("pthread_create failure");
            // not sure how to handle killing all of the threads here
        }
    }

    sleep(5);
    stopQueue(dirQueue);

    // Join all of the threads here
    for(int i = 0; i < numOfDirThreads; i++){
        if(pthread_join(dirThreadIDs[i], NULL)!=0){ // Change the 2nd parameter here if we want to use the exit status of a specific p_thread
            perror("pthread_join failure");
            // not sure how to handle killing all of the threads here
        }
    }


    // Clean Up -- Free everything
    fileQueue = destroyQueue(fileQueue);
    dirQueue = destroyQueue(dirQueue);
    free(fileNameSuffix);
    free(dirThreadIDs);
    free(args);

    if(errorInProgram == true)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}