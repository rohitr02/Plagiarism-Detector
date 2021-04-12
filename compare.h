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

#ifndef DEBUG
#define DEBUG true // CHANGE THIS TO FALSE BEFORE SUBMITTING
#endif

pthread_cond_t closeDirQueue;

/* Queue Code */
typedef struct node {
    char* data;
    struct node* next;
} node;

typedef struct Queue {
    node* head;
    node* tail;
    size_t size;
    bool stopQueue;            // Flag to signal that queue should not recieve any new items
    pthread_mutex_t lock;      // Mutex lock
    pthread_cond_t read_ready; // Conditional thread flag to use when dequeuing
} Queue;

/** This method must be set equal to the queue pointer variable that's being initialized. Otherwise it will result in a MEMORY LEAK!!! **/
/* Example: Queue* exampleQueue = init(NULL) */
Queue* initQueue(Queue* queue) {
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
    queue->stopQueue = false;
    queue->size = 0;
    pthread_mutex_init(&queue->lock, NULL);                             // Initialize the mutex lock and the conditional
    pthread_cond_init(&queue->read_ready, NULL);
    return queue;                                                       // Return the mallocd pointer
}

// data MUST BE a null terminated string
int enqueue(Queue* queue, char* data) {
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Enqueue Failed: The queue is not initialized");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutex
    if (queue->stopQueue == true) {                                     // If stopQueue has been flagged then unlock the mutex and exit failure
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
    while (queue->size == 0 && queue->stopQueue == false) {             // If there are no elements in the queue and the queue has not been stopped yet, then wait until an element is added
        pthread_cond_wait(&queue->read_ready, &queue->lock);
    }
    if (queue->size == 0) {                                             // If the queue size is 0, unlock the lock and return failure
        pthread_mutex_unlock(&queue->lock);
        return EXIT_FAILURE;
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

// Used to indicate that no more elements should be added to the queue without destorying the queue (there may still be elements in it that we need to dequeue)
int stopQueue(Queue* queue) {
    if (queue == NULL) {                                                // If the queue is not initialized, then immediately return failure
        if (DEBUG == true)
            fprintf(stderr, "%s\n", "Stop Queue Failed: The queue is not initialized");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&queue->lock);                                   // Lock the mutuex
    queue->stopQueue = true;                                            // Set the stopQueue flag to true
    pthread_cond_broadcast(&queue->read_ready);                         // Broadcast that conditional read_ready is true (elements can now be dequeued)
    pthread_mutex_unlock(&queue->lock);                                 // Unlock the mutex
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
    stopQueue(queue);                                                   // Call stop queue
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
    free(queue);                                                        // Free the queue
    return NULL;
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
        if (DEBUG == true) 
            printf("Hello from thread #%d\n", args->id);
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

        // Check if the dirQueue is empty, if it is, then signal the main thread to increment the dirThreadCompleteCounter.
        // Possible Issues: If a thread is complete and it dirThreadCompleteCounter gets incremented, but then another background thread enqueues something, then the next time this thread gets called, it will not go to sleep. 
        // On the next iteration this thread will once again be marked complete. So I think it is possible for one thread to repeatedly be marked complete even if it doesn't go to sleep on the next iteration.
        if(getSize(args->dirQueue) == 0){
            pthread_cond_signal(&closeDirQueue);
        }
    }
    return NULL;
}


/** File and WSD Code */
typedef struct word {
    char* letters;      // holds the string
    size_t count;       // number of times this word has been seen
    double freq;        // freq of this word
} word;

typedef struct wfd_node{
    char* fileName;
    word* wordArray;            // array holding all the words in the file
    size_t totalNumOfWords;
    size_t insertIndex;         // insert index into the array
    size_t size;
} wfd_node;

// init a wfd_node -- letters MUST BE MALLOC'd -- DO NOT free letters after calling this method
word* init_word(word* w, char* letters){
    if(w != NULL){          // if the word is already initialized then return immediately
        fprintf(stderr, "%s", "This wfd_node has already been initialized!");
        return w;
    }
    w = malloc(sizeof(word));       // malloc space
    if (w == NULL) {                                                  // If malloc fails then call perror and exit failure
        perror("Malloc Failed in init_word for Word");
        return EXIT_FAILURE;
    }
    w->letters = letters;
    w->count = 1;
    w->freq = 0;
    return w;
}

// Free the word's letters
int destroy_word(word w){
    free(w.letters);
    return EXIT_SUCCESS;
}

// init a wfd_node -- fileName MUST BE MALLOC'd -- DO NOT free filename after calling this method
wfd_node* init_wfd_node(wfd_node* ptr, char* fileName){
    if(ptr!=NULL){       // Already initialized
        if(DEBUG == true){
            fprintf(stderr, "%s", "This wfd_node has already been initialized!");
        }
        return ptr;
    }
    int minSize = 1;
    ptr = malloc(sizeof(wfd_node));
    if (ptr == NULL) {                                                  // If malloc fails then call perror and exit failure
        perror("Malloc Failed in init_wfd_node for the wfd_node address");
        return EXIT_FAILURE;
    }
    ptr->fileName = fileName;
    ptr->wordArray = malloc(minSize*sizeof(word));
    if (ptr->wordArray == NULL) {                                       // If malloc fails then call perror and exit failure
        perror("Malloc Failed in init_wfd_node for wordArray");
        return EXIT_FAILURE;
    }
    ptr->totalNumOfWords = 0;
    ptr->insertIndex = 0;
    ptr->size = minSize;
    return ptr;
}

// resize the wfd_node wordArray
int resize_wordArray(wfd_node* wfdnode){
    if(wfdnode == NULL){
        fprintf(stderr, "%s", "This wfd_node has not been initialized!");
        return EXIT_FAILURE;
    }
    wfdnode->wordArray = realloc(wfdnode->wordArray, wfdnode->size * sizeof(word));
    if (wfdnode->wordArray == NULL) {                                   // If realloc fails then call perror and exit failure
        perror("Realloc Failed in resize_wfdArray for Node");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// add a word to the wfd_node -- DO NOT free the word* after calling this because it is handled by the destroy_wfd_node method
int wfd_add(wfd_node* wfdnode, word* w){
    if(wfdnode == NULL){
        fprintf(stderr, "%s", "This wfd_node has not been initialized!");
        return EXIT_FAILURE;
    }
    if(wfdnode->insertIndex >= wfdnode->size){
        wfdnode->size *= 2;
        if (resize_wordArray(wfdnode) == EXIT_FAILURE)
            return EXIT_FAILURE;
    }
    
    wfdnode->wordArray[wfdnode->insertIndex++] = *w;                // increment the index and add the word to the word array
    free(w);
    return EXIT_SUCCESS;
}


// If you newly malloc'd the 2nd parameter here, then YOU must free it -- if it is the exact same recycled address of an existing element in wfd then DO NOT free it after.
int wfd_indexOf(wfd_node* wfdnode, char* w){
    if(wfdnode == NULL){
        fprintf(stderr, "%s", "This wfd_node has not been initialized!");
        return -1;
    }
    for(int i = 0; i < wfdnode->insertIndex; i++){                  // Loop thru all the words up till insertIndex
        if(strcmp(wfdnode->wordArray[i].letters, w) == 0){          // if the current word is equal to w, return index
            return i;
        }
    }
    return -1;                                                      // return -1 if not found
}

// Destroy's a wfd_node -- free everything
int destroy_wfd_node(wfd_node* wfdnode){
    if(wfdnode == NULL){
        fprintf(stderr, "%s", "This wfd_node has not been initialized!");
        return EXIT_FAILURE;
    }
    for(int i = 0; i < wfdnode->insertIndex; i++){
        destroy_word(wfdnode->wordArray[i]);
    }
    free(wfdnode->wordArray);
    free(wfdnode->fileName);
    free(wfdnode);
    return EXIT_SUCCESS;
}

// Print all the words in a wfd_node
void print_wfd_node(wfd_node* wfdnode){
    if(wfdnode == NULL){
        fprintf(stderr, "%s", "This wfd_node has not been initialized!");
        return;
    }
    printf("Printing Word Array: ");
    for(int i = 0; i < wfdnode->insertIndex; i++){
        printf("%s, ", wfdnode->wordArray[i].letters);
    }
    printf("Done\n");
}