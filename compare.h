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

#ifndef DEBUG
#define DEBUG true // CHANGE THIS TO FALSE BEFORE SUBMITTING
#endif

/* Queue Code */
typedef struct node {
    char* data;
    struct node* next;
}node;

typedef struct Queue {
    node* head; 
    node* tail;
    size_t size;
    bool stopQueue;                     // Flag to signal that queue should not recieve any new items
    pthread_mutex_t lock;               // Mutex lock
    pthread_cond_t read_ready;          // Conditional thread flag to use when dequeuing
}Queue;

/** This method must be set equal to the queue pointer variable that's being initialized. Otherwise it will result in a MEMORY LEAK!!! **/
        /* Example: Queue* exampleQueue = init(NULL) */
Queue* initQueue(Queue* queue){
    if(queue != NULL){                                                      // If the queue is already initialized, then immdiately return the queue
        if(DEBUG == true)   
            fprintf(stderr, "%s\n", "The queue is already initialized");
        return queue;
    }
    queue = (Queue*) malloc(sizeof(Queue));                                 // Malloc the new queue. Set all the initial node stuff to NULL/Empty/0.
    if (queue == NULL){
        perror("Malloc Failed in Queue Initialization");
        return NULL;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->stopQueue = false;
    queue->size = 0;
    pthread_mutex_init(&queue->lock, NULL);                                 // Initialize the mutex lock and the conditional
    pthread_cond_init(&queue->read_ready, NULL);
    return queue;                                                           // Return the mallocd pointer
}

int enqueue ( Queue* queue, char* data ) {
    if(queue == NULL){                                                      // If the queue is not initialized, then immediately return failure
        if(DEBUG == true) 
            fprintf(stderr, "%s\n", "Enqueue Failed: The queue is not initialized");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&queue->lock);                                       // Lock the mutex
if(queue->stopQueue == true){                                               // If stopQueue has been flagged then unlock the mutex and exit failure
        pthread_mutex_unlock(&queue->lock);
        return EXIT_FAILURE;
    }

    node* ptr = (node*) malloc(sizeof(node));                               // Create the new node -- malloc the space, set the data, set the next to null
    if(ptr == NULL){                                                        // If malloc fails then call perror and exit failure
        perror("Malloc Failed in Enqueue");
        return EXIT_FAILURE;
    }
    ptr->data = data;
    ptr->next = NULL;

    queue->size += 1;                                                       // Increase queue size by 1
    if(queue->head == NULL && queue->tail == NULL){                         // If the queue has no elements, then set the head and tail to ptr.
        queue->head = ptr;
        queue->tail = ptr;
        pthread_cond_signal(&queue->read_ready);                            // Signal read_ready to true (this means elements can now be dequeued)
        pthread_mutex_unlock(&queue->lock);                                 // Set the lock to unlocked
        return EXIT_SUCCESS;
    }
    queue->tail->next = ptr;                                                // Set the next element of the tail to ptr and then set tail to the last element
    queue->tail = queue->tail->next;
    pthread_cond_signal(&queue->read_ready);                                // Signal read_ready to true (this means elements can now be dequeued)
    pthread_mutex_unlock(&queue->lock);                                     // Set the lock to unlocked
    return EXIT_SUCCESS;
}

// item is a pointer to the location that you need to save the dequeued element to.
int dequeue ( Queue* queue, char** item ) {
    if(queue == NULL){                                                      // If the queue is not initialized, then immediately return failure
        if(DEBUG == true) 
            fprintf(stderr, "%s\n", "Dequeue Failed: The queue is not initialized");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&queue->lock);                                       // Lock the mutex
    while (queue->size == 0 && queue->stopQueue == false){                  // If there are no elements in the queue and the queue has not been stopped yet, then wait until an element is added
        pthread_cond_wait(&queue->read_ready, &queue->lock);
    }
    if(queue->size == 0){                                                   // If the queue size is 0, unlock the lock and return failure
        pthread_mutex_unlock(&queue->lock);
        return EXIT_FAILURE;
    }
    node* ptr = queue->head;                                                // Get the head, save the item, replace the head with the next element after it
    *item = ptr->data;
    queue->head = ptr->next;
    queue->size -= 1;                                                       // Decrease the size of the queue
    if(queue->head == NULL){                                                // If there are no more elements, set tail to NULL and set size to 0
        queue->tail = NULL;
        queue->size = 0;
    }
    free(ptr);                                                              // Free the dequeued pointer address and unlock the mutex
    pthread_mutex_unlock(&queue->lock);
    return EXIT_SUCCESS;
}

// Used to indicate that no more elements should be added to the queue without destorying the queue (there may still be elements in it that we need to dequeue)
int stopQueue(Queue* queue){
    if(queue == NULL){                                                      // If the queue is not initialized, then immediately return failure
        if(DEBUG == true) 
            fprintf(stderr, "%s\n", "Stop Queue Failed: The queue is not initialized");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&queue->lock);                                       // Lock the mutuex
    queue->stopQueue = true;                                                // Set the stopQueue flag to true
    pthread_cond_broadcast(&queue->read_ready);                             // Broadcast that conditional read_ready is true (elements can now be dequeued)
    pthread_mutex_unlock(&queue->lock);                                     // Unlock the mutex
    return EXIT_SUCCESS;
}

/** This method must be set equal to the queue pointer variable that's being destroyed. Otherwise it will result in a MEMORY LEAK and UNDEFINED BEHAVIOR of the variable later on!!! **/
        /* Example: exampleQueue = destroy(exampleQueue) */
Queue* destroyQueue( Queue* queue ){
    if(queue == NULL){                                                      // If the queue is not initialized, then immediately return failure
        if(DEBUG == true) 
            fprintf(stderr, "%s\n", "Destroy Queue Failed: The queue is not initialized");
        return queue;
    }
    stopQueue(queue);                                                       // Call stop queue
    pthread_mutex_lock(&queue->lock);                                       // Lock the mutex
    while(queue->head != NULL){                                             // While there are elements in the queue
        char* item;
        pthread_mutex_unlock(&queue->lock); //This seems kinda sketchy      // Unlock the mutex before calling dequeue
        dequeue(queue, &item);                                              // Dequeue an element
        pthread_mutex_lock(&queue->lock);                                   // Relock the mutex
        if(DEBUG) printf("Dequeued: %s\n", item);
    }
    pthread_mutex_unlock(&queue->lock);                                     // Unlock the mutex
    pthread_mutex_destroy(&queue->lock);                                    // Destory the mutex and the conditional
    pthread_cond_destroy(&queue->read_ready);
    free(queue);                                                            // Free the queue
    return NULL;
}

void printQueue( Queue* queue ){
    if(queue == NULL){                                                      // If the queue is not initialized, then immediately return failure
        if(DEBUG == true) 
            fprintf(stderr, "%s\n", "Print Queue Failed: The queue is not initialized");
        return;
    }
    pthread_mutex_lock(&queue->lock);                                       // Lock the mutex
    node* ptr = queue->head;                                                // Set ptr to head. Loop thru nodes in queue. Print them.
    printf("Printing Queue: ");
    while(ptr != NULL && ptr->data != NULL){
        printf("%s, ", ptr->data);
        ptr = ptr->next;
    }
    printf("Queue is empty\n");
    pthread_mutex_unlock(&queue->lock);                                     // Unlock the lock
}

/* End of Queue Code */


/** Put non-Queue Stuff Below This **/

// Directory and File Reading Methods
// Checks if input string is a directory
int isDir(char *pathname) {
	struct stat data;
	
	if (stat(pathname, &data)) {    // checks for error
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
	
	if (stat(pathname, &data)) {    // checks for error
		return false;
	}
	
	if (S_ISREG(data.st_mode)) {
		return true;
	}
	return false;
}

// check if a string has ONLY positive numbers -- checks for "12abcd" and "1abcd2" cases
int isLegalAtoiInput(char* arr){
    for (char* ptr = arr; *ptr; ++ptr)
        if(!(*ptr <= 57 && *ptr >= 48)) // character is not btwn '0' and '9'
            return false;
    return true;
}

// Checks if input string is an optional parameter
int setOptionalParameter(char* string, int* numOfDirThreadsPtr, int* numOfFileThreadsPtr, int* numOfAnalysisThreadsPtr, char** fileNameSuffixPtr){
    int stringLength = strlen(string);
    if(stringLength < 3){
        if (stringLength >= 2 && string[1] == 's'){         // This means suffix is an empty string
            *fileNameSuffixPtr = realloc(*fileNameSuffixPtr,1);
            strncpy(*fileNameSuffixPtr, "", 1);
            return true;
        }
        fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
        return false;
    }
    if(string[1] == 'd'){                   // set numOfDirThreads
        int inputNum = atoi(string + 2);
        if (isLegalAtoiInput(string + 2) == false || inputNum <= 0){                // Checks if the input is a legal input and its not <= 0
            fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
            return false;
        }
        *numOfDirThreadsPtr = inputNum;
        return true;
    }else if(string[1] == 'f'){             // set numOfFileThreads

        int inputNum = atoi(string + 2);
        if (isLegalAtoiInput(string + 2) == false || inputNum <= 0){                // Checks if the input is a legal input and its not <= 0
            fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
            return false;
        }
        *numOfFileThreadsPtr = inputNum;
        return true;
    }else if(string[1] == 'a'){             // set numOfAnalysisThreads
        int inputNum = atoi(string + 2);
        if (isLegalAtoiInput(string + 2) == false || inputNum <= 0){                // Checks if the input is a legal input and its not <= 0
            fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
            return false;
        }
        *numOfAnalysisThreadsPtr = inputNum;
        return true;
    }else if (string[1] == 's'){            // set fileNameSuffix
        *fileNameSuffixPtr = realloc(*fileNameSuffixPtr, stringLength -1);
        strncpy(*fileNameSuffixPtr, string+2, stringLength-1);
        return true;
    }
    else{
        fprintf(stderr, "%s %s\n", string, "is an invalid optional argument.");
        return false;
    }
}