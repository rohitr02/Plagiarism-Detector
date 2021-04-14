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

    pthread_mutex_init(&fileReadingLock, NULL);
    pthread_cond_init(&file_ready, NULL);

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
    
    int numOfDirArgs = 0;

    // Read the inputs
    for(int i = 1; i < argc; i++){
        if(isRegFile(argv[i]) == true){             // Check if it is a regular file and enqueue directly to fileQueue
            enqueue(fileQueue, argv[i]);
        }else if(isDir(argv[i]) == true){           // Check if it is a directory and enqueue directly to dirQueue
            enqueue(dirQueue, argv[i]);
            numOfDirArgs++;
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


    setMaxNumOfThreads(fileQueue, numOfFileThreads);
    setMaxNumOfThreads(dirQueue, numOfDirThreads);
    

    // if(getSize(dirQueue) == 0){
    //     printf("No directories\n");
    //     free(fileNameSuffix);
    //     fileQueue = destroyQueue(fileQueue);
    //     dirQueue = destroyQueue(dirQueue);
    //     return EXIT_SUCCESS;
    // }


    // Create variables to store thread IDs and the arguments to pass to each thread
    pthread_t* dirThreadIDs = malloc(numOfDirThreads * sizeof(pthread_t));
    if(dirThreadIDs == NULL){
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    thread_args* threadArgs = malloc(numOfDirThreads * sizeof(thread_args));
    if(threadArgs == NULL){
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        free(dirThreadIDs);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }

    // Create variables to store file thread IDs and the arguments to pass to each thread
    pthread_t* fileThreadIDs = malloc(numOfFileThreads * sizeof(pthread_t));
    if(fileThreadIDs == NULL){
        free(fileNameSuffix);
        // free(threadArgs);
        // free(dirThreadIDs);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    file_args* fileArgs = malloc(numOfFileThreads * sizeof(file_args));
    if(fileArgs == NULL){
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        // free(dirThreadIDs);
        // free(threadArgs);
        free(fileThreadIDs);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }

    // Variables needed for the file reading threads
    WFD* wfd = init_WFD(NULL);     // an array of wfd_node pointers that is the WFD

    // Loop through the number of threads to create and make them
    for(int i = 0; i < numOfDirThreads; i++){
        threadArgs[i].dirQueue = dirQueue;
        threadArgs[i].fileQueue = fileQueue;
        threadArgs[i].id = i;
        threadArgs[i].exitCode = EXIT_SUCCESS;
        threadArgs[i].fileSuffix = fileNameSuffix;
        if(pthread_create(&dirThreadIDs[i], NULL, readDirectory, &threadArgs[i]) != 0){
            perror("pthread_create failure");
            // not sure how to handle killing all of the threads here
        }
    }

    while((numOfDirArgs > 0 && getSize(fileQueue) == 0)){
        pthread_cond_wait(&file_ready, &fileReadingLock);
    }
    // Loop through the number of threads to create and make them
    for(int i = 0; i < numOfFileThreads; i++){
        fileArgs[i].fileQueue = fileQueue;
        fileArgs[i].id = i;
        fileArgs[i].exitCode = EXIT_SUCCESS;
        fileArgs[i].wfd = wfd;
        if(pthread_create(&fileThreadIDs[i], NULL, readFile, &fileArgs[i]) != 0){
            perror("pthread_create failure");
            // not sure how to handle killing all of the threads here
        }
    }

    // Join all of the threads here
    for(int i = 0; i < numOfDirThreads; i++){
        if(pthread_join(dirThreadIDs[i], NULL)!=0){ // Change the 2nd parameter here if we want to use the exit status of a specific p_thread
            perror("pthread_join failure");
            // not sure how to handle killing all of the threads here
        }
    }

    for(int i = 0; i < numOfFileThreads; i++){
        if(pthread_join(fileThreadIDs[i], NULL)!=0){ // Change the 2nd parameter here if we want to use the exit status of a specific p_thread
            perror("pthread_join failure");
            // not sure how to handle killing all of the threads here
        }
    }

    print_wfd(wfd);

    // Clean Up -- Free everything
    fileQueue = destroyQueue(fileQueue);
    dirQueue = destroyQueue(dirQueue);
    free(fileNameSuffix);
    free(dirThreadIDs);
    free(threadArgs);
    free(fileThreadIDs);
    free(fileArgs);
    destroy_wfd(wfd);
    pthread_mutex_destroy(&fileReadingLock);
    pthread_cond_destroy(&file_ready);

    if(errorInProgram == true)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}