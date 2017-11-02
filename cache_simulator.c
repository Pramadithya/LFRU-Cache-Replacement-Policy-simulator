#include <unistd.h>
// Libraries
#include <stdio.h>
#include "cache_simulator.h"           // Header file with the structs and functions prototypes
#include <stdlib.h>
#include <string.h>
#include <time.h>                      // For the time stamp stuff
#define DEBUG 0                        // 0: FALSE, 1:TRUE
#define CACHEVIEW 0                        // 1: print the currently cache (data and upper), 0: do not print
#define BYTES_PER_WORD 1               // All words with 1 Byte (bytes_per_word)
#define DATA 1


int currently_clk=0;                   // To simulate a clock with a counter

/**************************** Functions ***************************************/

/**
 * This function makes the start of all cache with zeros and devide the blocks into privileged and unprivileged by associativity/2.
 */
int startCache(Cache *cache1, int number_of_sets, int associativity) {
      int index_i, block_i;
      for (index_i=0; index_i<number_of_sets; index_i++) {
          for (block_i=0; block_i<associativity; block_i++ ) {
              cache1->Cache_Data[index_i][block_i]    = 0;
              cache1->Cache_Upper[index_i][block_i]   = 0;
              cache1->T_Access[index_i][block_i]      = 0;
              cache1->Hit_Frequency[index_i][block_i] = 0;
			  cache1->Trait[index_i][block_i] = 0;
          }
      }
	  for (index_i=0; index_i<number_of_sets; index_i++) {
          for (block_i=0; block_i<(associativity/2); block_i++ ) {
			  cache1->Trait[index_i][block_i] = 1;
		  }
	  }
      return 0;
}

/**
 * This function get the line (upper) of a given address by the CPU
 * This upper is equal to the Tag information
 */
int make_upper(long unsigned address, int words_per_line, int bytes_per_word) {
    int line;
    line = ((address/words_per_line))/bytes_per_word;
    return line;
}

/**
 * This function generates the index for the set in the cache
 */
int make_index (int number_of_sets, long unsigned upper) {
    int index;
    //printf("NUMBER OF SETS: %d\n", number_of_sets);
    //printf("UPPER: %lu\n", upper);
    index = upper % number_of_sets;

    return index;                               // index of the set in the cache
}

/**
 * This function give the position in a set (by index) of a line with a determined upper
 */
int getPosUpper (Cache *cache, int index, long unsigned line, int associativity) {
    int block_i;
    //printf("INDEX POS UPPER %d, ASSOC: %d, LINE: %lu\n", index, associativity, line);
    for (block_i=0; block_i<associativity; block_i++) {
        if (cache->Cache_Upper[index][block_i] == line){
            return block_i;               // Upper inside of the set found
        }
    }
    return -1;                           // Upper inside of the set not found
}

/** This function verifies if there are or aren't free blocks (lines) available in
  *     a set (by index1) in the cache memory.
  * @return: 1 (set is full), -1 (There are free space in the set by index1)
  */
int there_Are_Space_Set(Cache *cache1, int index1, int associativity, int special) { //special = 1 (search only in privileged blocks)
    int block_i;
	if (special == 1) {
		for (block_i=0; block_i<associativity; block_i++) {
			if (cache1->Cache_Data[index1][block_i]==0) {
				if (cache1->Trait[index1][block_i]==1){
					return -1;
				}
			}
		}
	}
	else {
		for (block_i=0; block_i<associativity; block_i++) {
			if (cache1->Cache_Data[index1][block_i]==0) {
				return -1;
			}
		}
	}
    return 1;
}

/** This function gives a block (line) free in a set (by index1) of the cache
  *     memory with no particular order. It returns the first block (position of line)
  *      free found, independent of its positon at the set.
  */
int random_free_space_set (Cache *cache1, int index1, int associativity, int special) {//special = search free space only on x trait blocks
    int block_i;
	if (special == 1) { // find on privileged blocks
		for (block_i=0; block_i<associativity; block_i++) {
			if (cache1->Cache_Data[index1][block_i]==0) {
				if (cache1->Trait[index1][block_i]==1){
					return block_i;
				}	
			}
		}
	}
	else {
		if (special == 0) { // find on unprivileged blocks
			for (block_i=0; block_i<associativity; block_i++) {
				if (cache1->Cache_Data[index1][block_i]==0) {
					if (cache1->Trait[index1][block_i]==0){
						return block_i;
					}	
				}
			}
		}
		else { // find on all blocks
			for (block_i=0; block_i<associativity; block_i++) {
				if (cache1->Cache_Data[index1][block_i]==0) {
					return block_i;
				}
			}
		}
	}
    return -1;                                   // None block free to be filled
}

int findLessAccessTSset (Cache *cache1, int index1, int associativity, int special) { //special = 1 (searching on privileged block)
    int block_i;
	long unsigned lessAc;
    int p_lessAc=0; // Position when is the less T_Access in the set (by index)
	if (special == 1) {
		//searching only on privileged block
		for (block_i=0; block_i<associativity; block_i++) {
			if (cache1->Trait[index1][block_i] == 1) {
				long unsigned lessAc = cache1->T_Access[index1][block_i];
			}
		}
		for (block_i=0; block_i<associativity; block_i++) {
			if (cache1->T_Access[index1][block_i] < lessAc) {
				if (cache1->Trait[index1][block_i] == 1) {
					lessAc = cache1->T_Access[index1][block_i];
					p_lessAc = block_i;
				}	
			}
		}	
    }
	else {
		//searching on all block
		lessAc = cache1->T_Access[index1][0];
		for (block_i=0; block_i<associativity; block_i++) {
			if (cache1->T_Access[index1][block_i] < lessAc) {
				lessAc = cache1->T_Access[index1][block_i];
				p_lessAc = block_i;
			}
		}
	}
    return p_lessAc;
}

int findLessHitFrequencyset (Cache *cache1, int index1, int associativity, int special) { //special = 1 (searching on unprivileged block)
    int block_i;
	long unsigned lessFr;
    int p_lessFr=0;  // Position when is the less T_Access in the set (by index)
	if (special == 1) {
		//searching only on unprivileged block
		for (block_i=0; block_i<associativity; block_i++) {
			if (cache1->Trait[index1][block_i] == 0) {
				long unsigned lessFr = cache1->T_Access[index1][block_i];
			}
		}
		for (block_i=0; block_i<associativity; block_i++) {
			if (cache1->Hit_Frequency[index1][block_i] < lessFr) {              // b < b+1
				if (cache1->Trait[index1][block_i] == 0) {
					lessFr = cache1->Hit_Frequency[index1][block_i];
					p_lessFr = block_i;
				}
			}	
		}
	}	
    lessFr = cache1->Hit_Frequency[index1][0];
    for (block_i=0; block_i<associativity; block_i++) {
        if (cache1->Hit_Frequency[index1][block_i] < lessFr) {              // b < b+1
            lessFr = cache1->Hit_Frequency[index1][block_i];
            p_lessFr = block_i;
        }
    }
    return p_lessFr;
}

void write_cache (Cache *cache1, Results *result1, int index1, long unsigned line1, int data1, int associativity) {
    /** Writing the data in the set (by index) in the position that contains the
      *     upper (by line).
      */
    int is_full;
    int is_privileged_full;
    int free_block;
    int fall, out; 

    int position_that_has_this_upper = getPosUpper(cache1, index1, line1, associativity);

    /****************************** Write Miss ********************************/
    if (position_that_has_this_upper == -1) { // position with the right upper was not found
        /** None position contains a line with the solicited upper.
          * The cache can be full
          * If it still a place in the cache, it's enough to draft one of free
          *     lines to put the new upper and the "new" data.
          * To know if are free places in the set (by index1) of cache, it is
          *     used the function there_Are_Space_Set(...).
          */
        result1->write_misses++; // (*result1).write_misses++  this points to the cache_mem in the main

        /** It is necessary to know if the set is full (it will use FIFO or LRU
          *     replacement policy) or not.
          */
        is_full = there_Are_Space_Set(cache1, index1, associativity, 0);

        /*************************** Set is full *****************************/
        if (is_full == 1) {
            // **SET IS FULL**
			fall = findLessAccessTSset(cache1, index1, associativity, 1); // search only on privileged blocks, the data will fall to unprivileged blocks
			out = findLessHitFrequencyset(cache1, index1, associativity, 1); // search only on unprivileged blocks
			cache1->Cache_Upper[index1][out] = cache1->Cache_Upper[index1][fall]; // now the privileged cache fallen to the unprivileged trait blocks
            cache1->Cache_Data[index1][out] = cache1->Cache_Data[index1][fall]; // the least frequent block data is now gone
            cache1->T_Access[index1][out] = cache1->T_Access[index1][fall]; 
            cache1->Hit_Frequency[index1][out]   = cache1->Hit_Frequency[index1][fall];
			cache1->Trait[index1][out]   = 0; // assign the unprivileged data with unprivileged trait variable
			cache1->Cache_Upper[index1][fall] = line1; //assigning the new data
			cache1->Cache_Data[index1][fall] = DATA;
			cache1->T_Access[index1][fall] = currently_clk; // Update the T_Access
			cache1->Trait[index1][fall] = 1;   // Assign Trait = privileged
			cache1->Hit_Frequency[index1][fall] = 1;
        }
        /**********************************************************************/

        /************************** Set isn't full ****************************/
        else { // There are a free block in the set (by index1)
            is_privileged_full = there_Are_Space_Set (cache1, index1, associativity, 1); //searching on the privileged blocks
			if (is_privileged_full == -1) { // there's space left on privileged blocks
				free_block = random_free_space_set(cache1, index1, associativity, 1); // search free block on privileged blocks
				cache1->Cache_Upper[index1][free_block] = line1;
				cache1->Cache_Data[index1][free_block] = DATA;
				cache1->T_Access[index1][free_block] = currently_clk; // Update the T_Access
				cache1->Trait[index1][free_block] = 1;   // Assign Trait = privileged
				cache1->Hit_Frequency[index1][free_block] = 1;
			}
			else { // there's no space left on privileged blocks, but there are space left in unprivileged blocks.
				fall = findLessAccessTSset(cache1, index1, associativity, 1); //search only in privileged blocks, the data will fall to unprivileged blocks
				free_block = random_free_space_set(cache1, index1, associativity, 0); // search free block on unprivileged blocks
				cache1->Trait[index1][fall] = 0; // now the privileged block fallen to the unprivileged blocks set
				cache1->Cache_Upper[index1][free_block] = line1;
				cache1->Cache_Data[index1][free_block] = DATA;
				cache1->T_Access[index1][free_block] = currently_clk; // Update the T_Access
				cache1->Trait[index1][free_block] = 1;   // Assign Trait = privileged
				cache1->Hit_Frequency[index1][free_block] = 1;
			}
        }
    }
    /**************************************************************************/

    /****************************** Write Hit ********************************/
    else {               // position with the right upper was found
        /** The position with the upper was found in the passed set (index1)
         *     data is writed in a position at the set in cache that already have
         *     an another data.
         */
        result1->write_hits++;
        cache1->Cache_Upper[index1][position_that_has_this_upper] = line1;
        cache1->T_Access[index1][position_that_has_this_upper] = currently_clk; // Update the T_Access
		cache1->Hit_Frequency[index1][position_that_has_this_upper] = cache1->Hit_Frequency[index1][position_that_has_this_upper] + 1; // update the Hit_Frequency
		/**moving the data to the privileged blocks*/
		fall = findLessAccessTSset(cache1, index1, associativity, 1); //search only in privileged blocks, the data will fall to unprivileged blocks
		cache1->Trait[index1][fall] = cache1->Trait[index1][position_that_has_this_upper];
		cache1->Trait[index1][position_that_has_this_upper] = 1;
	}
}

void read_cache (Cache *cache1, Results *result1, int index1, long unsigned line1, int data1, int associativity) {
    /** Reading the data in the set (by index) in the position that contains the
      *     upper (by line).
      */
    int is_full;
    int is_privileged_full;
    int free_block;
    int fall, out;
    int position_that_has_this_upper = getPosUpper(cache1, index1, line1, associativity);

    /****************************** Read Misses ********************************/
    if (position_that_has_this_upper == -1) { // position with the right upper was not found
        /** None position contains a line with the solicited upper.
          * If it still a place in the cache, it's enough to draft one of free
          *     lines to put the new upper and the "new" data.
          * To know if are free places in the set (by index1) of cache, it is
          *     used the function there_Are_Space_Set(...).
          */
        result1->read_misses++; // (*result1).write_misses++  this points to the cache_mem in the main

        /** It is necessary to know if the set is full (it will use FIFO or LRU
          *     replacement policy) or not.
          */
        is_full = there_Are_Space_Set(cache1, index1, associativity, 0);

        /*************************** Set is full ******************************/
        if (is_full == 1) {
            // **SET IS FULL**
			fall = findLessAccessTSset(cache1, index1, associativity, 1); // search only on privileged blocks, the data will fall to unprivileged blocks
			out = findLessHitFrequencyset(cache1, index1, associativity, 1); // search only on unprivileged blocks
			cache1->Cache_Upper[index1][out] = cache1->Cache_Upper[index1][fall]; // now the privileged cache fallen to the unprivileged trait blocks
            cache1->Cache_Data[index1][out] = cache1->Cache_Data[index1][fall]; // the least frequent block data is now gone
            cache1->T_Access[index1][out] = cache1->T_Access[index1][fall]; 
            cache1->Hit_Frequency[index1][out]   = cache1->Hit_Frequency[index1][fall];
			cache1->Trait[index1][out]   = 0; // assign the unprivileged data with unprivileged trait variable
			cache1->Cache_Upper[index1][fall] = line1; //assigning the new data
			cache1->Cache_Data[index1][fall] = DATA;
			cache1->T_Access[index1][fall] = currently_clk; // Update the T_Access
			cache1->Trait[index1][fall] = 1;   // Assign Trait = privileged
			cache1->Hit_Frequency[index1][fall] = 1;
        }
        /**********************************************************************/

        /************************** Set isn't full ****************************/
        else { // There are a free block in the set (by index1)
            is_privileged_full = there_Are_Space_Set (cache1, index1, associativity, 1); //searching on the privileged blocks
			if (is_privileged_full == -1) { // there's space left on privileged blocks
				free_block = random_free_space_set(cache1, index1, associativity, 1); // search free block on privileged blocks
				cache1->Cache_Upper[index1][free_block] = line1;
				cache1->Cache_Data[index1][free_block] = DATA;
				cache1->T_Access[index1][free_block] = currently_clk; // Update the T_Access
				cache1->Trait[index1][free_block] = 1;   // Assign Trait = privileged
				cache1->Hit_Frequency[index1][free_block] = 1;
			}
			else { // there's no space left on privileged blocks, but there are space left in unprivileged blocks.
				fall = findLessAccessTSset(cache1, index1, associativity, 1); //search only in privileged blocks, the data will fall to unprivileged blocks
				free_block = random_free_space_set(cache1, index1, associativity, 0); // search free block on unprivileged blocks
				cache1->Trait[index1][fall] = 0; // now the privileged block fallen to the unprivileged blocks set
				cache1->Cache_Upper[index1][free_block] = line1;
				cache1->Cache_Data[index1][free_block] = DATA;
				cache1->T_Access[index1][free_block] = currently_clk; // Update the T_Access
				cache1->Trait[index1][free_block] = 1;   // Assign Trait = privileged
				cache1->Hit_Frequency[index1][free_block] = 1;
			}
        }
    }
    /**************************************************************************/

    /****************************** Read Hit ********************************/
    else {               // position with the right upper was found
        /** The position with the upper was found in the passed set (index1)
         *     The T_Access and T_Load is updated in this case, because a number_of_writes
         *     data is writed in a position at the set in cache that already have
         *     an another data.
         */
        result1->read_hits++;
        cache1->Cache_Upper[index1][position_that_has_this_upper] = line1;
        cache1->T_Access[index1][position_that_has_this_upper] = currently_clk; // Update the T_Access
		cache1->Hit_Frequency[index1][position_that_has_this_upper] = cache1->Hit_Frequency[index1][position_that_has_this_upper] + 1; // update the Hit_Frequency
		/**moving the data to the privileged blocks*/
		fall = findLessAccessTSset(cache1, index1, associativity, 1); //search only in privileged blocks, the data will fall to unprivileged blocks
		cache1->Trait[index1][fall] = cache1->Trait[index1][position_that_has_this_upper];
		cache1->Trait[index1][position_that_has_this_upper] = 1;
	}
}

/**
 * This function generates a formated output with the results of cache simulation
 */
void generate_output(Results cache_results, char *output_name){
    FILE *ptr_file_output;
    ptr_file_output=fopen(output_name, "wb");

    if (!ptr_file_output) {
        printf("\nThe file of output can't be writed\n\n");
    }
    else {
        fprintf(ptr_file_output, "Access count:%d\n", cache_results.acess_count);
        fprintf(ptr_file_output, "Read hits:%d\n", cache_results.read_hits);
        fprintf(ptr_file_output, "Read misses:%d\n", cache_results.read_misses);
        fprintf(ptr_file_output, "Write hits:%d\n", cache_results.write_hits);
        fprintf(ptr_file_output, "Write misses:%d\n", cache_results.write_misses);
    }

    fclose(ptr_file_output);                  // Close the output file (results)
}
/******************************************************************************/

int main(int argc, char **argv)               // Files are passed by a parameter
{
    printf("\n*** Cache Simulator ***\n\n");

    Desc    cache_description;
    Results cache_results;
    Cache   cache_mem;   // This contains the data, access and load informations

    // Init of results
    cache_results.acess_count = 0;
    cache_results.read_hits = 0;
    cache_results.read_misses = 0;
    cache_results.write_hits = 0;
    cache_results.write_misses = 0;

    char *description = argv[1];
    char *input       = argv[2];
    char *output_name = argv[3];
    FILE *ptr_file_specs_cache;
    FILE *ptr_file_input;
    char RorW;                  // Read or Write (address)
    int number_of_reads = 0;    // Number of read request operations by the CPU
    int number_of_writes = 0;   // Number of write request operations by the CPU
    /*Informations of each address*/
    long unsigned address;                      // Address passed by the CPU
    int words_per_line;
    int number_of_sets;                         // number_of_lines/associativity
    long unsigned line;
    int index;
    int data = 1;

    /*********************** Cache Description ********************************/
    ptr_file_specs_cache = fopen(description, "rb");

    if (!ptr_file_specs_cache) {
        printf("\nThe file of cache description is unable to open!\n\n");
        return -1;
    }
    else {                                                 // File can be opened
        fscanf(ptr_file_specs_cache, "line size = %d\n", &cache_description.line_size);
        fscanf(ptr_file_specs_cache, "number of lines = %d\n", &cache_description.number_of_lines);
        fscanf(ptr_file_specs_cache, "associativity = %d\n", &cache_description.associativity);
    }
    fclose(ptr_file_specs_cache);            // Close the cache description file
    // DEBUG prints
    #if DEBUG == 1
    printf("%d\n",   cache_description.line_size);
    printf("%d\n",   cache_description.number_of_lines);
    printf("%d\n",   cache_description.associativity);
    #endif
    /**************************************************************************/

    words_per_line = cache_description.line_size/BYTES_PER_WORD; // 1 byte is the size of a word in this simulator
    number_of_sets = cache_description.number_of_lines / cache_description.associativity;

    /**************** Alloc space for Cache Memory Data ***********************/
    int i;                         // index for the allocation with the loop for
    cache_mem.Cache_Data = malloc( number_of_sets * sizeof(int *));
    for (i=0; i<number_of_sets; i++) {
        cache_mem.Cache_Data[i] = malloc (cache_description.associativity * sizeof(int));
    }
    /**************************************************************************/

    /******************* Alloc space for Cache Upper **************************/
    cache_mem.Cache_Upper = malloc( number_of_sets * sizeof(long unsigned *));
    for (i=0; i<number_of_sets; i++) {
        cache_mem.Cache_Upper[i] = malloc (cache_description.associativity * sizeof(long unsigned));
    }
    /**************************************************************************/

    /**************** Alloc space for Access Time Stamp************************/
    cache_mem.T_Access = malloc( number_of_sets * sizeof(long unsigned *));
    for (i=0; i<number_of_sets; i++) {
        cache_mem.T_Access[i] = malloc (cache_description.associativity * sizeof(long unsigned));
    }
    /**************************************************************************/

    /**************** Alloc space for load Time Stamp************************/
    cache_mem.Hit_Frequency = malloc( number_of_sets * sizeof(long unsigned *));
    for (i=0; i<number_of_sets; i++) {
        cache_mem.Hit_Frequency[i] = malloc (cache_description.associativity * sizeof(long unsigned));
    }
    /**************************************************************************/
	
	/**************** Alloc space for Cache Trait************************/
    cache_mem.Trait = malloc( number_of_sets * sizeof(int *));
    for (i=0; i<number_of_sets; i++) {
        cache_mem.Trait[i] = malloc (cache_description.associativity * sizeof(int));
    }
    /**************************************************************************/

    startCache(&cache_mem, number_of_sets, cache_description.associativity);

    /***************** Input Trace File and simulation ************************/

    ptr_file_input = fopen(input, "rb");
    if (!ptr_file_input) {
        printf("\nThe file of Input is unable to open!\n\n");
        return -1;
    }
    else {
        while (fscanf(ptr_file_input, "%lu %c\n", &address, &RorW) != EOF){
            currently_clk += 1;                                     // Increment the clock
            //printf("ADDRESS: %lu e RW: %c\n", address, RorW);
            cache_results.acess_count++;
            if (RorW == 'R') {
                line = make_upper(address, BYTES_PER_WORD, words_per_line);
                index = make_index (number_of_sets, line);

                // DEBUG prints
                #if DEBUG == 1
                printf("Index: %d\n", index);
                printf("The line: %lu\n", line);
                #endif

                number_of_reads++;
                read_cache(&cache_mem, &cache_results, index, line, data, cache_description.associativity);
            }
            else if (RorW == 'W'){
                line  = make_upper(address, BYTES_PER_WORD, words_per_line);
                index = make_index (number_of_sets, line);
                number_of_writes++;
                write_cache(&cache_mem, &cache_results, index, line, data, cache_description.associativity);
            }
            else {
                printf("\nUndefined operation request detected\n");

            }
        }

        #if DEBUG == 1
        printf("\nR:%d, W:%d\n", number_of_reads, number_of_writes);
        #endif
    }
    /**************************************************************************/

    /****************************** Output ************************************/
    generate_output(cache_results, output_name);
    /**************************************************************************/

    #if CACHEVIEW == 1
    printf("\n\nCurrently Cache\n");
    int m, n, vazio=0;
    for (m=0; m<number_of_sets; m++) {
        for (n=0; n<cache_description.associativity; n++) {
            printf("%lu ", cache_mem.Cache_Upper[m][n]);
            if (cache_mem.Cache_Data[m][n]==0)
                vazio++;
        }
        printf("\n");
    }
    printf ("\nNAO MODIFS: %d\n", vazio);

    printf("\n\nCurrently Cache\n");
    for (m=0; m<number_of_sets; m++) {
        for (n=0; n<cache_description.associativity; n++) {
            printf("%d ", cache_mem.Cache_Data[m][n]);
        }
        printf("\n");
    }
    #endif

    cache_mem.Cache_Data    = NULL;  // "Free" in the memory for the Cache_Data[][]
    cache_mem.Cache_Upper   = NULL;  // "Free" in the memory for the Cache_Upper[][]
    cache_mem.T_Access      = NULL;  // "Free" in the memory for the T_Access[][]
    cache_mem.Hit_Frequency = NULL;  // "Free" in the memory for the Hit_Frequency[][]
	cache_mem.Trait         = NULL;  // "Free" in the memory for the Trait[][]

    fclose(ptr_file_input);                 // Close the input file (trace file)
	
	
   return 0;
}
