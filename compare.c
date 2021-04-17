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
    Queue* fileQueue = initQueue(NULL, 1);
    if(fileQueue == NULL){                              // Malloc Failure
        free(fileNameSuffix);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    Queue* dirQueue = initQueue(NULL,0);
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

    // Set the max number of threads for both Queues
    setMaxNumOfThreads(fileQueue, numOfFileThreads);
    setMaxNumOfThreads(dirQueue, numOfDirThreads);
    

    // Create variables to store thread IDs and the arguments to pass to each thread
    pthread_t* dirThreadIDs = malloc(numOfDirThreads * sizeof(pthread_t));
    if(dirThreadIDs == NULL){                                                       // Malloc Failure
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    thread_args* threadArgs = malloc(numOfDirThreads * sizeof(thread_args));
    if(threadArgs == NULL){                                                         // Malloc Failure
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        free(dirThreadIDs);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }

    // Create variables to store file thread IDs and the arguments to pass to each thread
    pthread_t* fileThreadIDs = malloc(numOfFileThreads * sizeof(pthread_t));
    if(fileThreadIDs == NULL){                                                      // Malloc Failure
        free(fileNameSuffix);
        free(threadArgs);
        free(dirThreadIDs);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    file_args* fileArgs = malloc(numOfFileThreads * sizeof(file_args));
    if(fileArgs == NULL){                                                           // Malloc Failure
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        free(dirThreadIDs);
        free(threadArgs);
        free(fileThreadIDs);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }

    // Create variables to store analysis thread IDs and the arguments to pass to each thread
    pthread_t* analThreadIDs = malloc(numOfAnalysisThreads * sizeof(pthread_t));
    if(analThreadIDs == NULL){                                                       // Malloc Failure
        free(fileNameSuffix);
        free(threadArgs);
        free(dirThreadIDs);
        free(fileThreadIDs);
        free(fileArgs);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }
    anal_args* analArgs = malloc(numOfAnalysisThreads * sizeof(anal_args));
    if(analArgs == NULL){                                                           // Malloc Failure
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        free(dirThreadIDs);
        free(threadArgs);
        free(fileThreadIDs);
        free(fileArgs);
        free(analThreadIDs);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }

    // Variables needed for the file reading threads
    WFD* wfd = init_WFD(NULL);     // an array of wfd_node pointers that is the WFD
    if(wfd == NULL){                                                                // Malloc Failure
        free(fileNameSuffix);
        fileQueue = destroyQueue(fileQueue);
        dirQueue = destroyQueue(dirQueue);
        free(dirThreadIDs);
        free(threadArgs);
        free(fileThreadIDs);
        free(fileArgs);
        free(analThreadIDs);
        free(analArgs);
        perror("Malloc Failure");
        return EXIT_FAILURE;
    }

    // Loop through the number of threads to create and make them
    for(int i = 0; i < numOfDirThreads; i++){
        threadArgs[i].dirQueue = dirQueue;
        threadArgs[i].fileQueue = fileQueue;
        threadArgs[i].id = i;
        threadArgs[i].exitCode = EXIT_SUCCESS;
        threadArgs[i].fileSuffix = fileNameSuffix;
        if(pthread_create(&dirThreadIDs[i], NULL, readDirectory, &threadArgs[i]) != 0){
            perror("pthread_create failure");
            exit(EXIT_FAILURE);
        }
    }

    // Loop through the number of threads to create and make them
    for(int i = 0; i < numOfFileThreads; i++){
        fileArgs[i].fileQueue = fileQueue;
        fileArgs[i].id = i;
        fileArgs[i].exitCode = EXIT_SUCCESS;
        fileArgs[i].wfd = wfd;
        fileArgs[i].dirQueue = dirQueue;
        if(pthread_create(&fileThreadIDs[i], NULL, readFile, &fileArgs[i]) != 0){
            perror("pthread_create failure");
            exit(EXIT_FAILURE);
        }
    }

    // Join all of the threads here
    for(int i = 0; i < numOfDirThreads; i++){
        if(pthread_join(dirThreadIDs[i], NULL)!=0){ // Change the 2nd parameter here if we want to use the exit status of a specific p_thread
            perror("pthread_join failure");
            exit(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < numOfFileThreads; i++){
        if(pthread_join(fileThreadIDs[i], NULL)!=0){ // Change the 2nd parameter here if we want to use the exit status of a specific p_thread
            perror("pthread_join failure");
            exit(EXIT_FAILURE);
        }
    }


    int sizeOfWFD = wfd->insertIndex;
    if(sizeOfWFD < 2){                                          // If the size of the WFD repository is less than 2, then free everything and exit_failure
        fileQueue = destroyFileQueue(fileQueue, dirQueue);
        dirQueue = destroyQueue(dirQueue);
        free(fileNameSuffix);
        free(dirThreadIDs);
        free(threadArgs);
        free(fileThreadIDs);
        free(fileArgs);
        destroy_wfd(wfd);
        free(analThreadIDs);
        free(analArgs);
        return EXIT_FAILURE;
    }

    // Create array to hold JSD values -- no need to make it dynamically allocated since we know the size beforehand
    jsdVals outputArray[((sizeOfWFD-1) * sizeOfWFD)/2];
    int index = 0;

    // Double loop through the wfd repository and precreate all of the file pairings for the JSD
    for(int i = 0; i < wfd->insertIndex-1; i++) {
        for(int j = i+1; j < wfd->insertIndex; j++) {
            wfd_node filei = wfd->wfdArray[i];
            wfd_node filej = wfd->wfdArray[j];

            outputArray[index].value = 0;

            outputArray[index].file1 = filei.fileName;
            outputArray[index].totalWords_file1 = filei.totalWordsCount;
            outputArray[index].file1_LL = filei.wordLL;


            outputArray[index].file2 = filej.fileName;
            outputArray[index].totalWords_file2 = filej.totalWordsCount;
            outputArray[index].file2_LL = filej.wordLL;

            index++;
        }
    }

    // Create the intervals that each analysis thread should analyze
    int interval = ((sizeOfWFD-1) * sizeOfWFD)/2 / numOfAnalysisThreads == 0 ? 1 : ((sizeOfWFD-1) * sizeOfWFD)/2 / numOfAnalysisThreads ;
    index = 0;

    // Start up the analysis threads and split up the work based on the interval
    for(int i = 0; i < numOfAnalysisThreads; i++){
        analArgs[i].id = i;
        analArgs[i].exitCode = EXIT_SUCCESS;
        analArgs[i].wfd = wfd;
        analArgs[i].array = outputArray;
        if(i >= ((sizeOfWFD-1) * sizeOfWFD)/2){
            analArgs[i].startIndex = -1;
            analArgs[i].endIndex = -1;
        }else{
            analArgs[i].startIndex = i * interval;
            analArgs[i].endIndex = i == numOfAnalysisThreads-1 ? ((sizeOfWFD-1) * sizeOfWFD)/2 : analArgs[i].startIndex + interval;
        }

        if(pthread_create(&analThreadIDs[i], NULL, runAnalysis, &analArgs[i]) != 0){
            perror("pthread_create failure");
            exit(EXIT_FAILURE);
        }
    }

    // Join the Analysis threads
    for(int i = 0; i < numOfAnalysisThreads; i++){
        if(pthread_join(analThreadIDs[i], NULL)!=0){ // Change the 2nd parameter here if we want to use the exit status of a specific p_thread
            perror("pthread_join failure");
            exit(EXIT_FAILURE);
        }
    }

    // Sort JSD and print output
    for(int i = 0; i < (((sizeOfWFD-1) * sizeOfWFD)/2); i++) {
        int max = outputArray[i].totalWords_file1 + outputArray[i].totalWords_file2;
        int maxIndex = i;
        for(int j = i+1; j < ((sizeOfWFD-1) * sizeOfWFD)/2; j++) {
            int currTotal = outputArray[j].totalWords_file1 + outputArray[j].totalWords_file2;
            if(currTotal > max) {
                max = currTotal;
                maxIndex = j;
            }
        }
        struct jsdVals temp = outputArray[i];
        outputArray[i] = outputArray[maxIndex];
        outputArray[maxIndex] = temp;
        printf("%f\t%s\t\t%s\n", outputArray[i].value, outputArray[i].file1, outputArray[i].file2);
    }

    // Clean Up -- Free everything and exit
    fileQueue = destroyFileQueue(fileQueue, dirQueue);
    dirQueue = destroyQueue(dirQueue);
    free(fileNameSuffix);
    free(dirThreadIDs);
    free(threadArgs);
    free(fileThreadIDs);
    free(fileArgs);
    destroy_wfd(wfd);
    free(analThreadIDs);
    free(analArgs);

    if(errorInProgram == true)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}