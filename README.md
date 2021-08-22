Names: Rohit Rao and Pauli Peralta

# Project Description:
This is a multithreaded program which checks for similarity between files in directories -- similar to a plagiarism checker but without the database of documents to check from.

It takes one or more files and directories and up to four optional parameters (mentioned below) as input. Then, it calculates the jensen-shannon distance between the word frequencies for every pair of files given (including in the subdirectories). 

# Usage:
1.) Navigate to the directory of the program in terminal and create the execuatable file by typing: ```make```

2.) Run the program using ```./compare``` followed by the parameters shown [here.](https://drive.google.com/file/d/1m2AIZYu4bzUcto2nOOu226i2EBsahp4Z/view?usp=sharing)

# Description of Major Components and Debugging:
**User Input:**
This was the first component that we built. We built the options to add files, directory, or optional arguments. We tested this component of the program by running it against testcases with no files, no directories, only files, only directories, a mix of files and directories, no optional arguments, only optional arguments, repeated optional arguments, and a mixture of all three.

**Queue:**
We built a threadsafe queue. We created the basic threadsafe enqueue and dequeue methods along with specialized threadsafe methods specific for the project. We tested each of the initial important methods we made in a separate program to ensure that they behaved correctly.

**Directory Reading:**
Once the queues were made, we designed the directory reading code. This was largely similar to [Word Wrapper](https://github.com/rohitr02/Word-Wrapper) which had directory traversal as well. We also tested this component individually by giving it various incorrect and correct inputs to test if it behaved correctly. Next, we took the code and placed it inside the method that the directory threads would call and made a few edits to make it work with threading. We tested this component of the program by running it against invalid directories, directories with multiple subdirectories, directories with multiple files, and empty directories. We also varied the number of directory threads in order to test whether the code behaved correctly when run by multiple threads.

**Word Frequency Distribution (WFD):**
For the word frequency distribution we used a combination of an array and a linked list. We examined the words in a file one by one and put them in a lexicographically sorted linked list of a word struct -- a struct that contained the word itself, its occurence, frequency, and average (the average is not touched in this process). A tokenzier function was created that added each word to the linked list accordingly. 

The structure was tested by isolating the tokenizer function as a standalone program and was passed controlled files as arguments for which we knew how many words were present, which ones repeated, and how many times they were repeated. Once the tokenizer function was done tokenizing a file and populating the linked list, we would traverse the linked list again and assign each struct its appropriate frequency value by utilizing a variable that kept track of the total number of words encountered in the file. We then traversed the linked list to ensure that it was indeed in lexicographic order and that each field of the struct was assigned the correct value. We repeated this process with many different files, files with no words, repeated words, only unique words, and a mix of unique and repeated words until we felt that every case was accounted for. If the tokenizer produced a linked list that was not empty, we then created a wfd_node struct -- a struct that contains the file name, a pointer to the lexicographically sorted linked list, and the overall word count. This struct was then added to an array of wfd_node -- the WFD -- where we held the appropriate information for every other file that was analyzed. 

**File Reading:**
Once we had the tokenizer function working as a standalone program, we took that code and placed it into the function that the file threads would call. We made minor edits to make the code work inside of the function and then tested it with a single thread to begin with. Then, we ran the same testcases that we used against tokenizer but now we varied the number of file threads in order to make sure that the program was correctly handling threading. We also varied the number of directory threads to make sure that the fileQueue was behaving correctly both when there were more directory threads than file threads and when there were less directory threads than file threads.

**Jensen-Shannon Distance (JSD):**
The WFD populated by the file threads was then used to create another array that stored the jsd value. For n files, we knew that we'd be making n(n-1)/2 comaparisons, in other words, for n files there are going to be n(n-1)/2 jsd values. We took this information and created an array of size n(n-1)/2 and prepopulated it with every possible combination pair by using the file names and linked lists stored in WFD. The new prepopulated array consisted of jsdVal structs which held 2 file names, 2 linked lists of words (one for each file), 2 fields that indicated how many words were present in each file, and a jsd value field initially set to 0. Once the array was populated, we'd iterate through the array and then iterate through each linked list at each struct. Since the linked lists are lexicographically sorted, we traversed them at the same time and compared the words accordingly. At this point we made use of the average field present in the word struct that the linked list is made of and populated the field accordingly. At the end of running through each linked list, we'd have a linked list for each file that now contained each word in the file, the number of times it occurred in the file, its frequency, and now the average of the word between the current two files being analyzed. We then ran through the linked list again and calculated the Kullbeck-Leilbler Divergence (KLD) using the averages we had just found and then finally calculated the jsd between the two files. The math behind the JSD caclulation can be found [here.](https://drive.google.com/file/d/1g0J79Hp2xxV0moT7qt3tfxT4B6Q-94lZ/view?usp=sharing)

This part was tested with assitance of the tokenizer described above. We isolated the jsd function as a standalone program that took two files as an argument and returned the jsd value. The files were passed to the tokenizer which returned a linked list and then the jsd function would go through the process described above. We tested files that were identical, files that had no words in common, files that had both unqiue and common words, and on empty files. We used files that were relatively short so that we could easily track what the output was supposed to be.

**Analysis Threads:**
We then took the standalone JSD function and copied it into the function that the analysis threads would call. We retested the same cases under 1 thread to make sure that the behavior was still correct. Then, we started varying the number of analysis threads to make sure that the program behaved correctly under multiple threads. After that we started varying the number of file and directory threads to make sure that the code still behaved correctly.

**Sorting:**
After going through each element in the array that stored the jsd values, we then used insertion sort to sort the structs in descending order according to the sum of total words file in each file that the struct contained. We were able to directly test this within the larger program itself since it was a short addition to the overall program. Once again we tested it on smaller testcases where we already knew the expected order and were able to confirm that the sort worked.

**General Debugging:**
We made sure to test the program using UBSan, AddressSanitizer, and Valgrind

**Note:** This program has only been tested and is made to be run on the Rutgers ilabs machines.
