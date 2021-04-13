#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

struct word {
    char* word;
    double occurence;
    double frequency;
    struct word* next;
};

int compareStr(char* str1, char* str2) {
    int str1size = strlen(str1);
    int str2size = strlen(str2);
    int minSize = str1size < str2size ? str1size : str2size;

    for(int i = 0; i < minSize; i++) {
        if(str1[i] < str2[i]) return 1;
        if(str2[i] < str1[i]) return 2;
    }

    if(str1size <= str2size) return 1;

    return 0;
}



int tokenize(char* filename) {
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
        return EXIT_FAILURE;
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
                    return EXIT_FAILURE;
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
                    return EXIT_FAILURE;
                }
                for(int i = 0; i < wordLen; i++) {                  // copy word from buffer to newWord 
                    newWord[i] = buffer[i];
                }
                newWord[wordLen] = '\0';                            // null terminate the word
                allWords++;                                         // increase the overall word tracker by 1

                struct word* prev = NULL;                           // inserting the new word into the link list with all of the other words found in the file
                struct word* ptr = head;
                while(ptr != NULL) {
                    if(strcmp(ptr->word, newWord) == 0) {           // if the new word matches a word already in the linked list, then increment the occurence variable of the word
                        ptr->occurence++;
                        wordLen = 0;
                        break;
                    }
                    prev = ptr;
                    ptr = ptr->next;
                }
                if(prev == NULL) {                                  // if it's the first word, initialize the head of the link list to point to the word struct that holds the new word
                    struct word* insert = malloc(sizeof(struct word));
                    insert->word = newWord;
                    insert->occurence = 1;
                    insert->next = NULL;
                    head = insert;
                    wordLen = 0;
                }
                if(ptr == NULL && prev != NULL) {                     // if we reach the end of the linked list and did not encounter the word, then it is a new word to the list, add it at the end of the linked list
                struct word* insert = malloc(sizeof(struct word));
                    insert->word = newWord;
                    insert->occurence = 1;
                    insert->next = NULL;

          
                    prev = NULL;
                    ptr = head;
                    while(ptr != NULL) {
                        if(compareStr(newWord, ptr->word) == 1) {  
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
            return EXIT_FAILURE;
        }
        for(int i = 0; i < wordLen; i++) {                  // copy word from buffer to newWord 
            newWord[i] = buffer[i];
        }
        newWord[wordLen] = '\0';                            // null terminate the word
        allWords++;                                         // increase the overall word tracker by 1

        struct word* prev = NULL;                           // inserting the new word into the link list with all of the other words found in the file
        struct word* ptr = head;
        while(ptr != NULL) {
            if(strcmp(ptr->word, newWord) == 0) {           // if the new word matches a word already in the linked list, then increment the occurence variable of the word
                ptr->occurence++;
                wordLen = 0;
                break;
            }
            prev = ptr;
            ptr = ptr->next;
        }
        if(prev == NULL) {                                  // if it's the first word, initialize the head of the link list to point to the word struct that holds the new word
            struct word* insert = malloc(sizeof(struct word));
            insert->word = newWord;
            insert->occurence = 1;
            insert->next = NULL;
            head = insert;
            wordLen = 0;
        }
       if(ptr == NULL && prev != NULL) {                     // if we reach the end of the linked list and did not encounter the word, then it is a new word to the list, add it at the end of the linked list
            struct word* insert = malloc(sizeof(struct word));
            insert->word = newWord;
            insert->occurence = 1;
            insert->next = NULL;

    
            prev = NULL;
            ptr = head;
            while(ptr != NULL) {
                if(compareStr(newWord, ptr->word) == 1) {  
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

    while (head != NULL)
    {
        head->frequency = (double)(head->occurence/allWords);
        printf("String: %s\tOccurence: %.0f\t Frequency: %f\n", head->word, head->occurence, head->frequency);
        struct word* temp = head;
        head = head->next;
        free(temp);
    }
    
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    tokenize(argv[1]);

    return EXIT_SUCCESS; 
}