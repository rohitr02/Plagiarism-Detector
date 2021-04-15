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