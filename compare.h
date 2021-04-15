#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>
#include <math.h>

#ifndef DEBUG
#define DEBUG false // CHANGE THIS TO FALSE BEFORE SUBMITTING
#endif


/* Queue Code */
typedef struct node {
    char* data;
    struct node* next;
} node;

typedef struct Queue {
    node* head;
    node* tail;
    size_t size;
    pthread_mutex_t lock;      // Mutex lock
    pthread_cond_t read_ready; // Conditional thread flag to use when dequeuing
    pthread_cond_t close_file;
    size_t numOfThreadsRunning;
    int typeOfQueue;
} Queue;

/** This method must be set equal to the queue pointer variable that's being initialized. Otherwise it will result in a MEMORY LEAK!!! **/
/* Example: Queue* exampleQueue = init(NULL) */
Queue* initQueue(Queue* queue, int type) {
    if (queue != NULL) {                                                // If the queue is already initialized, then immdiately return the queue
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "The queue is already initialized");
        return queue;
    }
    queue = (Queue*) malloc(sizeof(Queue));                             // Malloc the new queue. Set all the initial node stuff to NULL/Empty/0.
    if (queue == NULL) {
        perror("Malloc Failed in Queue Initialization");
        return NULL;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->numOfThreadsRunning = 1;
    queue->typeOfQueue = type;
    pthread_mutex_init(&queue->lock, NULL);                             // Initialize the mutex lock and the conditional
    pthread_cond_init(&queue->read_ready, NULL);
    pthread_cond_init(&queue->close_file, NULL);
    return queue;                                                       // Return the mallocd pointer
}

int setMaxNumOfThreads(Queue* queue, int max){
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Enqueue Failed: The queue is not initialized");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutex
    queue->numOfThreadsRunning = max;
    pthread_mutex_unlock(&queue->lock);
    return EXIT_SUCCESS;
}

// data MUST BE a null terminated string
int enqueue(Queue* queue, char* data) {
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Enqueue Failed: The queue is not initialized");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutex
    if (queue->numOfThreadsRunning == 0) {                                     
        pthread_mutex_unlock(&queue->lock);
        return EXIT_FAILURE;
    }

    node* ptr = (node*) malloc(sizeof(node));                           // Create the new node -- malloc the space, set the data, set the next to null
    if (ptr == NULL) {                                                  // If malloc fails then call perror and exit failure
        perror("Malloc Failed in Enqueue for Node");
        return EXIT_FAILURE;
    }
    int dataLength = strlen(data);
    ptr->data = malloc((dataLength + 1) * sizeof(char));
    if (ptr->data == NULL) {                                            // If malloc fails then call perror and exit failure
        perror("Malloc Failed in Enqueue for Data");
        return EXIT_FAILURE;
    }
    for (int i = 0; i <= dataLength; i++) {
        ptr->data[i] = data[i];
    }
    ptr->next = NULL;

    queue->size += 1;                                                   // Increase queue size by 1
    // printf("\t made it to here too!\n");
    if (queue->head == NULL && queue->tail == NULL) {                   // If the queue has no elements, then set the head and tail to ptr.
        queue->head = ptr;
        queue->tail = ptr;
        pthread_cond_signal(&queue->read_ready);                        // Signal read_ready to true (this means elements can now be dequeued)
        pthread_mutex_unlock(&queue->lock);                             // Set the lock to unlocked
        return EXIT_SUCCESS;
    }
    queue->tail->next = ptr;                                            // Set the next element of the tail to ptr and then set tail to the last element
    queue->tail = queue->tail->next;
    pthread_cond_signal(&queue->read_ready);                            // Signal read_ready to true (this means elements can now be dequeued)
    pthread_mutex_unlock(&queue->lock);                                 // Set the lock to unlocked
    return EXIT_SUCCESS;
}

// item is a pointer to the location that you need to save the dequeued element to. MUST call free() on the dequeued element after its usage.
int dequeue(Queue* queue, char** item) {
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Dequeue Failed: The queue is not initialized");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutex
    if(queue->size == 0){
        queue->numOfThreadsRunning -= 1;
        if(queue->numOfThreadsRunning < 1){
            // printf("here before broadcast");
            pthread_cond_broadcast(&queue->read_ready);                         // Broadcast that conditional read_ready is true (elements can now be dequeued)
            pthread_cond_signal(&queue->close_file);
            pthread_mutex_unlock(&queue->lock);
            return EXIT_FAILURE;
        }
        while (queue->size == 0 && queue->numOfThreadsRunning > 0/*queue->stopQueue == false*/) {             // If there are no elements in the queue and the queue has not been stopped yet, then wait until an element is added
            pthread_cond_wait(&queue->read_ready, &queue->lock);
        }
        if (queue->size == 0) {                                             // If the queue size is 0, unlock the lock and return failure
            pthread_mutex_unlock(&queue->lock);
            return EXIT_FAILURE;
        }
        queue->numOfThreadsRunning += 1;
    }
    
    node* ptr = queue->head;                                            // Get the head, save the item, replace the head with the next element after it
    *item = ptr->data;
    queue->head = ptr->next;
    queue->size -= 1;                                                   // Decrease the size of the queue
    if (queue->head == NULL) {                                          // If there are no more elements, set tail to NULL and set size to 0
        queue->tail = NULL;
        queue->size = 0;
    }
    free(ptr);                                                          // Free the dequeued pointer address and unlock the mutex
    pthread_mutex_unlock(&queue->lock);
    return EXIT_SUCCESS;
}



int getSize(Queue* queue){
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Stop Queue Failed: The queue is not initialized");
        return -1;
    }
    int len = 0;
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutuex
    len = queue->size;
    pthread_mutex_unlock(&queue->lock);                                 // Unlock the mutex
    return len;
}

int getNumOfThreadsRunning(Queue* queue){
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Stop Queue Failed: The queue is not initialized");
        return -1;
    }
    int num = 0;
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutuex
    num = queue->numOfThreadsRunning;
    pthread_mutex_unlock(&queue->lock);                                 // Unlock the mutex
    return num;
}

void printQueue(Queue* queue) {
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Print Queue Failed: The queue is not initialized");
        return;
    }
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutex
    node* ptr = queue->head;                                            // Set ptr to head. Loop thru nodes in queue. Print them.
    printf("Printing Queue: ");
    while (ptr != NULL && ptr->data != NULL) {
        printf("%s, ", ptr->data);
        ptr = ptr->next;
    }
    printf("Queue is empty\n");
    pthread_mutex_unlock(&queue->lock);                                 // Unlock the lock
}

// item is a pointer to the location that you need to save the dequeued element to. MUST call free() on the dequeued element after its usage.
int fileDequeue(Queue* queue, char** item, Queue* directoryQueue) {
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Dequeue Failed: The queue is not initialized");
        return EXIT_FAILURE;
    }
     pthread_mutex_lock(&queue->lock);                                   // Lock the mutex
    if(queue->size == 0){
        if(getSize(directoryQueue) == 0 && getNumOfThreadsRunning(directoryQueue) == 0){
            pthread_cond_broadcast(&directoryQueue->close_file);                         // Broadcast that conditional read_ready is true (elements can now be dequeued)
            pthread_mutex_unlock(&queue->lock);
            return EXIT_FAILURE;
        }
        while(getNumOfThreadsRunning(directoryQueue) > 0 || getSize(directoryQueue) > 0){
            pthread_cond_wait(&directoryQueue->close_file, &queue->lock);
        }
       
        pthread_mutex_unlock(&queue->lock);
        int size = getSize(queue);
        pthread_mutex_lock(&queue->lock);
        if (size == 0) {                                             // If the queue size is 0, unlock the lock and return failure
            pthread_mutex_unlock(&queue->lock);
            return EXIT_FAILURE;
        }
    }
    
    node* ptr = queue->head;                                            // Get the head, save the item, replace the head with the next element after it
    *item = ptr->data;
    queue->head = ptr->next;
    queue->size -= 1;                                                   // Decrease the size of the queue
    if (queue->head == NULL) {                                          // If there are no more elements, set tail to NULL and set size to 0
        queue->tail = NULL;
        queue->size = 0;
    }
    free(ptr);                                                          // Free the dequeued pointer address and unlock the mutex
    pthread_mutex_unlock(&queue->lock);
    return EXIT_SUCCESS;
}

/** This method must be set equal to the queue pointer variable that's being destroyed. Otherwise it will result in a MEMORY LEAK and UNDEFINED BEHAVIOR of the variable later on!!! **/
/* Example: exampleQueue = destroy(exampleQueue) */
Queue* destroyQueue(Queue* queue) {
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Destroy Queue Failed: The queue is not initialized");
        return queue;
    }
    // stopQueue(queue);                                                   // Call stop queue
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutex
    while (queue->head != NULL) {                                       // While there are elements in the queue
        char* item;
        pthread_mutex_unlock(&queue->lock); // This seems kinda sketchy // Unlock the mutex before calling dequeue
            dequeue(queue, &item);                                          // Dequeue an element
        pthread_mutex_lock(&queue->lock);                               // Relock the mutex
        if (DEBUG)
            printf("Dequeued: %s\n", item);
        free(item);
    }
    pthread_mutex_unlock(&queue->lock);                                 // Unlock the mutex
    pthread_mutex_destroy(&queue->lock);                                // Destory the mutex and the conditional
    pthread_cond_destroy(&queue->read_ready);
    pthread_cond_destroy(&queue->close_file);
    free(queue);                                                        // Free the queue
    return NULL;
}

Queue* destroyFileQueue(Queue* queue, Queue* dQ) {
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Destroy Queue Failed: The queue is not initialized");
        return queue;
    }
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutex
    while (queue->head != NULL) {                                       // While there are elements in the queue
        char* item;
        pthread_mutex_unlock(&queue->lock); // This seems kinda sketchy // Unlock the mutex before calling dequeue
            fileDequeue(queue, &item, dQ);                                          // Dequeue an element
        pthread_mutex_lock(&queue->lock);                               // Relock the mutex
        if (DEBUG)
            printf("Dequeued: %s\n", item);
        free(item);
    }
    pthread_mutex_unlock(&queue->lock);                                 // Unlock the mutex
    pthread_mutex_destroy(&queue->lock);                                // Destory the mutex and the conditional
    pthread_cond_destroy(&queue->read_ready);
    pthread_cond_destroy(&queue->close_file);
    free(queue);                                                        // Free the queue
    return NULL;
}


/* End of Queue Code */


/* Directory Reading and File Queue Writing Methods */

// Checks if input string is a directory
int isDir(char* pathname) {
    struct stat data;

    if (stat(pathname, &data)) { // checks for error
        return false;
    }

    if (S_ISDIR(data.st_mode)) {
        return true;
    }
    return false;
}

// Checks if input string is a file
int isRegFile(char *pathname) {
    struct stat data;

    if (stat(pathname, &data)) { // checks for error
        return false;
    }

    if (S_ISREG(data.st_mode)) {
        return true;
    }
    return false;
}

// check if a string has ONLY positive numbers -- checks for "12abcd" and "1abcd2" cases
int isLegalAtoiInput(char *arr) {
    for (char *ptr = arr; *ptr; ++ptr)
        if (!(*ptr <= 57 && *ptr >= 48)) // character is not btwn '0' and '9'
            return false;
    return true;
}

// Checks if input string is an optional parameter
int setOptionalParameter(char* string, int* numOfDirThreadsPtr, int* numOfFileThreadsPtr, int* numOfAnalysisThreadsPtr, char** fileNameSuffixPtr) {
    int stringLength = strlen(string);
    if (stringLength < 3) {
        if (stringLength >= 2 && string[1] == 's') { // This means suffix is an empty string
            *fileNameSuffixPtr = realloc(*fileNameSuffixPtr, 1);
            strncpy(*fileNameSuffixPtr, "", 1);
            return true;
        }
        fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
        return false;
    }

    if (string[1] == 'd') { // set numOfDirThreads
        int inputNum = atoi(string + 2);
        if (isLegalAtoiInput(string + 2) == false || inputNum <= 0) { // Checks if the input is a legal input and its not <= 0
            fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
            return false;
        }
        *numOfDirThreadsPtr = inputNum;
        return true;
    } else if (string[1] == 'f') { // set numOfFileThreads

        int inputNum = atoi(string + 2);
        if (isLegalAtoiInput(string + 2) == false || inputNum <= 0) { // Checks if the input is a legal input and its not <= 0
            fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
            return false;
        }
        *numOfFileThreadsPtr = inputNum;
        return true;
    } else if (string[1] == 'a') { // set numOfAnalysisThreads
        int inputNum = atoi(string + 2);
        if (isLegalAtoiInput(string + 2) == false || inputNum <= 0) { // Checks if the input is a legal input and its not <= 0
            fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
            return false;
        }
        *numOfAnalysisThreadsPtr = inputNum;
        return true;
    } else if (string[1] == 's') { // set fileNameSuffix
        *fileNameSuffixPtr = realloc(*fileNameSuffixPtr, stringLength - 1);
        strncpy(*fileNameSuffixPtr, string + 2, stringLength - 1);
        return true;
    } else {
        fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
        return false;
    }
}

/* Structure to hold the arguments passed into the thread */
typedef struct thread_args {
    Queue* dirQueue;
    Queue* fileQueue;
    int id;
    int exitCode;
    char* fileSuffix;
} thread_args;

/* Thread Functions to Call */
void* readDirectory(void* arguments) {
    thread_args* args = arguments;
    char* directory = NULL; // used to store the name of the dir for readibility
    
    while(dequeue(args->dirQueue, &directory) == EXIT_SUCCESS) {
        if (DEBUG == true) {
            printf("Hello from dir thread #%d \t ", args->id);
            printQueue(args->dirQueue);
        }
        DIR *folder = opendir(directory); // open the current directory
        if (folder == NULL) {
            perror("opendir failed");
            // not sure how to handle this error here
            continue;
        }
        struct dirent *currentFile;
        currentFile = readdir(folder);


        while (currentFile != NULL) { // loops through each file
            char* suffix = strrchr(currentFile->d_name, args->fileSuffix[0]);
            if (strncmp(currentFile->d_name, ".", 1) != 0) {                                // only interacts with file that do not start with "."

                int directoryLength = strlen(directory);                                    // used to store the length of the dir name
                int fileLength = strlen(currentFile->d_name);                               // used to store the length of the file name
                char *filePath = malloc(sizeof(char) * (directoryLength + fileLength + 2)); // an array that allocates space for "dir/filename"

                if (filePath == NULL) {
                    perror("Malloc Failure");
                    free(filePath);
                    // closedir(folder); --this was orig uncommented
                    // not sure how to handle this error here
                    continue;
                }


                // populate filePath with the actual "dir/file" path
                char* filename = currentFile->d_name;
                int index = 0;
                for( index = 0; index < directoryLength; index++) {
                    filePath[index] = directory[index];
                }
                filePath[index++] = '/';
                for(int q = 0; q <= fileLength; q++) {
                    filePath[index++] = filename[q];
                }

                // Check if its a regular file or directory
                struct stat data;
                if (stat(filePath, &data)==0) {
                    if (S_ISREG(data.st_mode) && suffix != NULL && !strcmp(suffix, args->fileSuffix)) { 
                        enqueue(args->fileQueue, filePath); // Need to add an error check here
                    }else if(S_ISDIR(data.st_mode)) {
                        enqueue(args->dirQueue, filePath);  // Need to add an error check here
                    }
                }
                free(filePath);                // frees "dir/filename"
                currentFile = readdir(folder); // get next file
            } else {
                currentFile = readdir(folder); // get next file
            }
        }
        free(directory);
        closedir(folder);    // close directory
    }
    
    return NULL;
}


/** File and WSD Code */
typedef struct word {
    char* word;
    double occurence;
    double frequency;
    double average;
    struct word* next;
} word;

typedef struct wfd_node{
    char* fileName;
    word* wordLL;            // LL holding all the words in the file in lexicographic order
    int sizeofLL;
} wfd_node;

typedef struct WFD{
    wfd_node* wfdArray;
    int insertIndex;
    int size;
    pthread_mutex_t lock;      // Mutex lock
} WFD;

WFD* init_WFD(WFD* ptr){
    if(ptr!=NULL){       // Already initialized
        if(DEBUG == true){
            fprintf(stderr, "%s", "The WFD has already been initialized!");
        }
        return ptr;
    }
    ptr = malloc(sizeof(WFD));
    if (ptr == NULL) {                                                  // If malloc fails then call perror and exit failure
        perror("Malloc Failed in init_WFD for the WFD address");
        return NULL;
    }
    int minSize = 1;
    ptr->wfdArray = malloc(minSize *sizeof(wfd_node));
    if (ptr == NULL) {                                                  // If malloc fails then call perror and exit failure
        perror("Malloc Failed in init_WFD for the WFD address");
        return NULL;
    }
    ptr->insertIndex = 0;
    ptr->size = minSize;
    pthread_mutex_init(&ptr->lock, NULL);
    return ptr;
}

int resize_wfdArray(WFD* wfd){
    if(wfd == NULL){
        if(DEBUG == true){
            fprintf(stderr, "%s", "The WFD or wfd_node has not been initialized!");
        }
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&wfd->lock);
    wfd->size *= 2;
    wfd->wfdArray = realloc(wfd->wfdArray, wfd->size*sizeof(wfd_node));
    if(wfd->wfdArray == NULL){
        perror("Realloc Failed in init_WFD for the WFD address");
        pthread_mutex_unlock(&wfd->lock);
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&wfd->lock);
    return EXIT_SUCCESS;
}

int add_wfd_node(WFD* wfd, wfd_node* wfdnode){
    if(wfd == NULL || wfdnode == NULL){
        if(DEBUG == true){
            fprintf(stderr, "%s", "The WFD or wfd_node has not been initialized!");
        }
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&wfd->lock);
    if(wfd->insertIndex >= wfd->size){
        pthread_mutex_unlock(&wfd->lock);
        resize_wfdArray(wfd);
        pthread_mutex_lock(&wfd->lock);
    }
    wfd->wfdArray[wfd->insertIndex++] = *wfdnode;
    free(wfdnode);
    pthread_mutex_unlock(&wfd->lock);
    return EXIT_SUCCESS;
}

int destroy_wfd(WFD* wfd){
    if(wfd == NULL){
        if(DEBUG == true){
            fprintf(stderr, "%s", "The WFD or wfd_node has not been initialized!");
        }
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&wfd->lock);
    for(int i =0; i< wfd->insertIndex; i++){
        word* head = wfd->wfdArray[i].wordLL;
        while(head!=NULL){
            word* temp = head;
            head = head->next;  
            free(temp->word);
            free(temp);
        }
        free((wfd->wfdArray[i].fileName));
    }
    free((wfd->wfdArray));
    free(wfd);
    pthread_mutex_unlock(&wfd->lock);
    return EXIT_SUCCESS;
}

int print_wfd(WFD* wfd){
    if(wfd == NULL){
        if(DEBUG == true){
            fprintf(stderr, "%s", "The WFD or wfd_node has not been initialized!");
        }
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&wfd->lock);
    for(int i =0; i< wfd->insertIndex; i++){
        printf("FILE: \n%s", wfd->wfdArray[i].fileName);
        word* head = wfd->wfdArray[i].wordLL;
        while(head!=NULL){
            if(DEBUG == true)
                printf("String: %s\tOccurence: %.0f\t Frequency: %f\n", head->word, head->occurence, head->frequency);
            head = head->next;
        }
        printf("\n");
    }
    pthread_mutex_unlock(&wfd->lock);
    return EXIT_SUCCESS;
}

typedef struct file_args{
    Queue* fileQueue;
    WFD* wfd;
    int id;
    int exitCode;
    Queue* dirQueue;
} file_args;

void* readFile(void* arguments){
    file_args* args = arguments;
    if (DEBUG == true) {
        printf("Hello from file thread #%d , fileThread active threads = %ld\t", args->id, args->fileQueue->numOfThreadsRunning);
        printQueue(args->fileQueue);
    }
    
    
    char* filename = NULL;
    while(fileDequeue(args->fileQueue, &filename, args->dirQueue) == EXIT_SUCCESS){
            int fd;             // keeps tarck of file descriptor
            int byte;           // keeps track of bytes read
            int wordLen;        // keeps tracks of each wordss length when reading through file
            char currChar;      // used to store each invidividual character as we read through the file
            word* head = NULL;   // the head of a linked list that's used to store each word in the file
            int allWords = 0;           // counter used to keep track of how many words are in the file overall

            int sizOfLL = 0;

        fd = open(filename, O_RDONLY);      // open the file

        int sizeofArray = 100;              // used as the initial size of the array we're gonna store each word into one at a time
        char* buffer = malloc(sizeofArray * sizeof(char));
        if(buffer == NULL) {
            return NULL;
        }


        byte = read(fd, &currChar, 1);          // reading the first charcater in the file
        wordLen = 0;                            // initialize the length of the upcoming word to 0
        while(byte > 0) {
            if(!isspace(currChar)) {            // if byte read is not a whitespace, do the following...
                if(wordLen == sizeofArray) {    // if the array where we're storing the word becomes full, double the size of it and keep recording
                    char* temp = realloc(buffer, sizeofArray * 2); 
                    if(temp == NULL) {
                        free(buffer);
                        close(fd);
                        return NULL;
                    }
                    buffer = temp;
                    sizeofArray *= 2;
                }
                if(isalpha(currChar)) buffer[wordLen++] = tolower(currChar);  // if char is a letter, add it the lowercase version to the word array
                if(currChar == '-') buffer[wordLen++] = currChar;              // if the char is '-', add it to the word array
                if(isdigit(currChar)) buffer[wordLen++] = currChar; 
                byte = read(fd, &currChar, 1);
            }
            else {
                if(wordLen > 0) {                                               // if a white space is encountered and we have stuff in the word array, then that indicates the end of that word
                    char* newWord = malloc(wordLen+1 * sizeof(char));
                    if(newWord == NULL) {
                        free(buffer);
                        close(fd);
                        return NULL;
                    }
                    for(int i = 0; i < wordLen; i++) {                  // copy word from buffer to newWord 
                        newWord[i] = buffer[i];
                    }
                    newWord[wordLen] = '\0';                            // null terminate the word
                    allWords++;                                         // increase the overall word tracker by 1

                    struct word* prev = NULL;                           // inserting the new word into the link list with all of the other words found in the file
                    struct word* ptr = head;
                    bool readVar = false;
                    while(ptr != NULL) {
                        if(strcmp(ptr->word, newWord) == 0) {           // if the new word matches a word already in the linked list, then increment the occurence variable of the word
                            ptr->occurence++;
                            readVar = true;
                            free(newWord);
                            wordLen = 0;
                            break;
                        }
                        prev = ptr;
                        ptr = ptr->next;
                    }
                    if(prev == NULL && readVar == false) {                                  // if it's the first word, initialize the head of the link list to point to the word struct that holds the new word
                        struct word* insert = malloc(sizeof(struct word));
                        insert->word = newWord;
                        insert->occurence = 1;
                        insert->next = NULL;
                        head = insert;
                        readVar = true;
                        sizOfLL++;
                        wordLen = 0;
                    }
                    else if(ptr == NULL && prev != NULL && readVar == false) {                     // if we reach the end of the linked list and did not encounter the word, then it is a new word to the list, add it at the end of the linked list
                        struct word* insert = malloc(sizeof(struct word));
                        insert->word = newWord;
                        insert->occurence = 1;
                        insert->next = NULL;

                        sizOfLL++;
                        prev = NULL;
                        ptr = head;
                        while(ptr != NULL) {
                            int str1size = strlen(newWord);
                            int str2size = strlen(ptr->word);
                            int minSize = str1size < str2size ? str1size : str2size;
                            int add = -1;

                            for(int i = 0; i < minSize; i++) {
                                if(newWord[i] < ptr->word[i]) {
                                    add = 1;
                                    break;
                                }
                                if(ptr->word[i] < newWord[i]) {
                                    add = 0;
                                    break;
                                }
                            }
                            if((add == -1) && (str1size <= str2size)) add = 1;
                            if(add == 1) {  
                                if(prev == NULL) {
                                    insert->next = head;
                                    head = insert;
                                    break;
                                }
                                else {
                                    prev->next = insert;
                                    insert->next = ptr;
                                    break;
                                }
                            }
                            prev = ptr;
                            ptr = ptr->next;
                        }
                        if(ptr == NULL)  {
                            prev->next = insert;
                            insert->next = NULL;
                        }
                        wordLen = 0;

                    }
                }
                byte = read(fd, &currChar, 1);
            }
        }

        if(wordLen > 0) {                                               // if a white space is encountered and we have stuff in the word array, then that indicates the end of that word
            char* newWord = malloc(wordLen+1 * sizeof(char));
            if(newWord == NULL) {
                free(buffer);
                close(fd);
                return NULL;
            }
            for(int i = 0; i < wordLen; i++) {                  // copy word from buffer to newWord 
                newWord[i] = buffer[i];
            }
            newWord[wordLen] = '\0';                            // null terminate the word
            allWords++;                                         // increase the overall word tracker by 1

            struct word* prev = NULL;                           // inserting the new word into the link list with all of the other words found in the file
            struct word* ptr = head;
            bool readVar = false;
            while(ptr != NULL) {
                if(strcmp(ptr->word, newWord) == 0) {           // if the new word matches a word already in the linked list, then increment the occurence variable of the word
                    ptr->occurence++;
                    readVar = true;
                    free(newWord);
                    wordLen = 0;
                    break;
                }
                prev = ptr;
                ptr = ptr->next;
            }
            if(prev == NULL && readVar == false) {                                  // if it's the first word, initialize the head of the link list to point to the word struct that holds the new word
                struct word* insert = malloc(sizeof(struct word));
                insert->word = newWord;
                insert->occurence = 1;
                insert->next = NULL;
                head = insert;
                sizOfLL++;
                readVar = true;
                wordLen = 0;
            }
            else if(ptr == NULL && prev != NULL && readVar == false) {                     // if we reach the end of the linked list and did not encounter the word, then it is a new word to the list, add it at the end of the linked list
                struct word* insert = malloc(sizeof(struct word));
                insert->word = newWord;
                insert->occurence = 1;
                insert->next = NULL;
                sizOfLL++;
        
                prev = NULL;
                ptr = head;
                while(ptr != NULL) {
                    int str1size = strlen(newWord);
                    int str2size = strlen(ptr->word);
                    int minSize = str1size < str2size ? str1size : str2size;
                    int add = -1;

                    for(int i = 0; i < minSize; i++) {
                        if(newWord[i] < ptr->word[i]) {
                            add = 1;
                            break;
                        }
                        if(ptr->word[i] < newWord[i]) {
                            add = 0;
                            break;
                        }
                    }
                    if((add == -1) && (str1size <= str2size)) add = 1;
                    if(add == 1) {  
                        if(prev == NULL) {
                            insert->next = head;
                            head = insert;
                            break;
                        }
                        else {
                            prev->next = insert;
                            insert->next = ptr;
                            break;
                        }
                    }
                    prev = ptr;
                    ptr = ptr->next;
                }
                if(ptr == NULL)  {
                    prev->next = insert;
                    insert->next = NULL;
                }
                wordLen = 0;

            }
        }

        free(buffer);
        close(fd);
        
        word* temp = head;
        while (temp != NULL)
        {
            temp->frequency = (double)(temp->occurence/allWords);
            temp = temp->next;
        }

        if(head != NULL){
            wfd_node* wfdnode = malloc(sizeof(wfd_node));
            wfdnode->fileName = filename;
            wfdnode->wordLL = head;
            wfdnode->sizeofLL = sizOfLL++;
            add_wfd_node(args->wfd, wfdnode);
        }
    }
    
    return NULL;
}


typedef struct jsdVals {
    double value;
    char* file1;
    char* file2;
    struct word* file1_LL;
    struct word* file2_LL;
} jsdVals;

typedef struct anal_args{
    WFD* wfd;
    jsdVals* array;
    int startIndex;
    int endIndex;
    int id;
    int exitCode;
} anal_args;

void* runAnalysis(void* arguments){
    anal_args* args = arguments;
    if(DEBUG == true) {
        printf("\nHello from thread #%d\n", args->id);
        printf("\tThis thread starts at startIndex = %d and ends at endIndex = %d\n", args->startIndex, args->endIndex);
    }
    if(args->startIndex == -1 || args->endIndex == -1) return NULL;     // This means there are more analysis threads than # of files

    // Loops thru the start index and end index
    for(int i = args->startIndex; i < args->endIndex; i++){
        if(DEBUG == true) {
            printf("\tThis thread is scanning the files: %s and %s\n", args->array[i].file1, args->array[i].file2);
        }
        struct word* ptri = args->array[i].file1_LL;
        struct word* ptrj = args->array[i].file2_LL;
        //printf("%s\n", ptrj->word);
        while((ptri != NULL) || (ptrj != NULL)) {
            if(ptri == NULL) {
                ptrj->average = ptrj->frequency * 0.5;
                //printf("file2: %s\n", ptrj->word);
                //printf("%f\n", ptrj->average);
                ptrj = ptrj->next;
                continue;
            }
            if(ptrj == NULL) {
                ptri->average = ptri->frequency * 0.5;
                //printf("file1: %s\n", ptri->word);
                ptri = ptri->next;
                continue;
            }
            char* currWord_filei = ptri->word;
            char* currWord_filej = ptrj->word;
            if(strcmp(currWord_filei, currWord_filej) == 0) {
                double totalFrequency = ptri->frequency + ptrj->frequency;
                double avg = totalFrequency * 0.5;
                ptri->average = avg;
                ptrj->average = avg;
                ptri = ptri->next;
                ptrj = ptrj->next;
                continue;
            }

            char firstChar_i = currWord_filei[0];
            char firstChar_j = currWord_filej[0];
            if(firstChar_i < firstChar_j) {
                //printf("file1: %s\n", ptri->word);
                ptri->average = ptri->frequency * 0.5;;
                //printf("%f\n", ptri->average);
                ptri = ptri->next;
                continue;
            }

            if(firstChar_j < firstChar_i) {
                //printf("file2: %s\n", ptrj->word);
                ptrj->average = ptrj->frequency * 0.5;;
                ptrj = ptrj->next;
                continue;
            }

            int currWord_filei_size = strlen(currWord_filei);
            int currWord_filej_size = strlen(currWord_filej);
            int minSize = currWord_filei_size < currWord_filej_size ? currWord_filei_size : currWord_filej_size;

            char* ptri_word = currWord_filei;
            char* ptrj_word = currWord_filej;

            while(*ptri_word != '\0' && *ptrj_word != '\0') {
                if(*ptri_word < *ptrj_word) {
                    ptri->average = ptri->frequency * 0.5;
                    ptri = ptri->next;
                    break;
                }
                if(*ptrj_word < *ptri_word) {
                    ptrj->average = ptrj->frequency * 0.5;
                    ptrj = ptrj->next;
                    break;
                }
                ptri_word++;
                ptrj_word++;
            }

            if(*ptri_word == '\0' || *ptrj_word == '\0') {
                if(minSize == currWord_filei_size) {
                    ptri->average = ptri->frequency * 0.5;
                    ptri = ptri->next;
                }
                else {
                    ptrj->average = ptrj->frequency * 0.5;
                    ptrj = ptrj->next;
                }
            }
        }

        
        double filei_KLD = 0;
        ptri = args->array[i].file1_LL;
        while(ptri != NULL) {
            //printf("wordi: %s\tfrequencyi: %f\tavaeragei: %f\tlog2i: %f\n",ptri->word, ptri->frequency,ptri->average, log_2i);
            double x = (ptri->frequency/ptri->average);
            filei_KLD = filei_KLD + (ptri->frequency * log2(x));
            //printf("KLDi: %f\n", filei_KLD);
            ptri = ptri->next;
        }
        //printf("KLD1: %f\n",filei_KLD);
        //putchar('\n');

        double filej_KLD = 0;
        ptrj = args->array[i].file2_LL;
        while(ptrj != NULL) {
            //printf("wordj: %s\tfrequencyj: %f\tavaeragej: %f\tlog2j: %f\n",ptrj->word, ptrj->frequency,ptrj->average, log_2j);
            double x = (ptrj->frequency/ptrj->average);
            filej_KLD = filej_KLD + (ptrj->frequency * log2(x));
            //printf("KLDj: %f\n", filej_KLD);
            ptrj = ptrj->next;
        }
        //printf("KLD2: %f\n",filej_KLD);

        double filei_jsd = filei_KLD * 0.5;
        double filej_jsd = filej_KLD * 0.5;
        double jsd = sqrt(filei_jsd + filej_jsd);
        //printf("%f\n", jsd);
        args->array[i].value = jsd;
    }


    return NULL;
}