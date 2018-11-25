#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BACKING_STORE_FILE "BACKING_STORE.bin"
#define MEM_SIZE 256
#define PAGE_SIZE 256
#define PAGE_FAULT -1

int page_fault_aumont = 0;


int load_memory(int page_number, int **memory, int mem_size){

    FILE *b_store = fopen(BACKING_STORE_FILE, "rb");

    if(b_store == NULL){
        printf("Error oppening %s\n", BACKING_STORE_FILE);
        exit(-1);
    }

    fseek(b_store, 256 * page_number, SEEK_SET);

    int *page = (int *) calloc(1, PAGE_SIZE);
    if(page == NULL) exit(-1);

    fread(page, PAGE_SIZE, 1, b_store);

    fclose(b_store);

    int i = 0;
    while(memory[i] != NULL) i++;

    memory[i] = page;

    return i;
}

int main(int argc, char const *argv[]) {
    
    if (argc < 2){

        printf("Error. Usage: ./program <adress_file>\n");
        exit(-1);
    }

    char *filename = (char *) calloc(FILENAME_MAX, sizeof(char)); 
    if(filename == NULL) exit(-1);
    strcpy(filename, argv[1]);

    printf("Oppening %s\n", filename);

    FILE *address_file = fopen(filename, "r");
    if(address_file == NULL){
        printf("Could not open file. \n");
        exit(-1);
    }

    free(filename);

    // represents the real memory
    int **memory = (int **) calloc(MEM_SIZE, sizeof(int *));
    if(memory == NULL) exit(-1);

    // represents a page table
    // page_table[page number] == frame
    int page_table[MEM_SIZE];
    for(int i = 0; i < MEM_SIZE; i++) page_table[i] = PAGE_FAULT;

    do{
        unsigned int logical_address;
        fscanf(address_file, "%u", &logical_address);

        // a unsigned int has a size of 32 bits
        // we want to extract 16 bits from the right 
        
        // since a integer has 4 bytes (32 bits)
        // we can think of the maximum value as:
        // FFFF (65 553)
        // we want only the 2 less significant bytes
        // so we will use the following mask:
        // 00FF (255)  
        unsigned int page_offset = logical_address & 0x00FF;

        // the same idea applies to the extraction of page number
        // and the offset  
        unsigned int page_number = page_offset & 0x00F0;
        unsigned int offset = page_offset & 0x000F;

        printf("Logical address: %u Page+offset: %u\n", logical_address, page_offset);

        if(page_table[(int) page_number] == PAGE_FAULT){
            page_fault_aumont++;
            int frame = load_memory((int) page_number, memory, MEM_SIZE);
            page_table[(int) page_number] = frame;
        }

        unsigned int byte_value = *(memory[page_table[(int) page_number]] + offset);
        // since the objective the value of the byte,
        // and a unsigned int has a size of 4 bytes,
        // we want to shift it unitl only one byte is left;
        byte_value = byte_value >> 3*8; //3 times the syze of a byte
        // the shift operator takes shift aumount as bits, not bytes
                 
        printf("Value of %u+%u: %u\n\n", page_number, offset, byte_value);

    }while(feof(address_file) == 0);

    fclose(address_file);

    for(int i = 0; i < MEM_SIZE; i++) if(memory[i] != NULL) free(memory[i]);
    free(memory);

    printf("\n\n\n=========\nPage faults: %d\n", page_fault_aumont);

    return 0;
}
