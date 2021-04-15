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

#define TRUE 1
#define FALSE 0

typedef struct word {
    char* word;
    double occurence;
    double frequency;
    double average;
    struct word* next;
} word;

typedef struct wfd_node{
    char* fileName;
    struct word* wordLL;            // LL holding all the words in the file in lexicographic order
} wfd_node;

typedef struct WFD{
    wfd_node* wfdArray;
    int insertIndex;
    int size;
    //pthread_mutex_t lock;      // Mutex lock
} WFD;

typedef struct jsdVals {
    double value;
    char* file1;
    char* file2;
    struct word* file1_LL;
    struct word* file2_LL;
} jsdVals;

struct word* tokenize(char* filename) {
    int fd;             // keeps tarck of file descriptor
    int byte;           // keeps track of bytes read
    int wordLen;        // keeps tracks of each wordss length when reading through file
    char currChar;      // used to store each invidividual character as we read through the file
    struct word* head = NULL;   // the head of a linked list that's used to store each word in the file
    int allWords = 0;           // counter used to keep track of how many words are in the file overall

    fd = open(filename, O_RDONLY);      // open the file

    int sizeofArray = 100;              // used as the initial size of the array we're gonna store each word into one at a time
    char* buffer = malloc(100 * sizeof(char));
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
                int read = FALSE;
                while(ptr != NULL) {
                    if(strcmp(ptr->word, newWord) == 0) {           // if the new word matches a word already in the linked list, then increment the occurence variable of the word
                        ptr->occurence++;
                        free(newWord);
                        read = TRUE;
                        wordLen = 0;
                        break;
                    }
                    prev = ptr;
                    ptr = ptr->next;
                }
                if(prev == NULL && read == FALSE) {                                  // if it's the first word, initialize the head of the link list to point to the word struct that holds the new word
                    struct word* insert = malloc(sizeof(struct word));
                    insert->word = newWord;
                    insert->occurence = 1;
                    insert->next = NULL;
                    head = insert;
                    wordLen = 0;
                    read = TRUE;
                }
                if(ptr == NULL && prev != NULL && read == FALSE) {                     // if we reach the end of the linked list and did not encounter the word, then it is a new word to the list, add it at the end of the linked list
                    struct word* insert = malloc(sizeof(struct word));
                    insert->word = newWord;
                    insert->occurence = 1;
                    insert->next = NULL;
                    read = TRUE;
          
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
        int read = FALSE;
        while(ptr != NULL) {
            if(strcmp(ptr->word, newWord) == 0) {           // if the new word matches a word already in the linked list, then increment the occurence variable of the word
                ptr->occurence++;
                free(newWord);
                read = TRUE;
                wordLen = 0;
                break;
            }
            prev = ptr;
            ptr = ptr->next;
        }
        if(prev == NULL && read == FALSE) {                                  // if it's the first word, initialize the head of the link list to point to the word struct that holds the new word
            struct word* insert = malloc(sizeof(struct word));
            insert->word = newWord;
            insert->occurence = 1;
            insert->next = NULL;
            head = insert;
            wordLen = 0;
            read = TRUE;
        }
        if(ptr == NULL && prev != NULL && read == FALSE) {                     // if we reach the end of the linked list and did not encounter the word, then it is a new word to the list, add it at the end of the linked list
            struct word* insert = malloc(sizeof(struct word));
            insert->word = newWord;
            insert->occurence = 1;
            insert->next = NULL;
            read = TRUE;
    
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

    struct word* temp = head;
    while (temp != NULL)
    {
        temp->frequency = (double)(temp->occurence/allWords);
        //printf("String: %s\tOccurence: %.0f\t Frequency: %f\n", temp->word, temp->occurence, temp->frequency);
        //struct word* temp = head;
        temp = temp->next;
        //free(temp);
    }

    return head;
}

double calcJSD(struct word* file1, struct word* file2) {
    struct word* ptri = file1;
    struct word* ptrj = file2;
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

        if(*ptri_word != '\0' || *ptrj_word != '\0') {
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
    ptri = file1;
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
    ptrj = file2;
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
    return jsd;
}

int main(int argc, char* argv[]) {
    struct word* file1 = tokenize(argv[1]);
    struct word* file2 = tokenize(argv[2]);

    printf("The JSD value of files %s and %s is: %f\n", argv[1], argv[2], calcJSD(file1, file2));

    while(file1 != NULL) {
        struct word* temp = file1;
        file1 = file1->next;
        free(temp->word);
        free(temp);
    }
     while(file2 != NULL) {
        struct word* temp = file2;
        file2 = file2->next;
        free(temp->word);
        free(temp);
    }

    return EXIT_SUCCESS;

}