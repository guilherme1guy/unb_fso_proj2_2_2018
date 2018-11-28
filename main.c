#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define BACKING_STORE_FILE "BACKING_STORE.bin"
#define BACKING_STORE_FILE "/mnt/c/Users/guilh/Desktop/BACKING_STORE.bin"
#define MEM_SIZE 256
#define PAGE_SIZE 256
#define PAGE_FAULT -1
#define TLB_SIZE 16

int page_fault_aumont = 0;
int found_in_tlb = 0;
int total_memoy_access = 0;

struct tlb{

    // will use FIFO to control TLB
    int *pages;
    int *frames;

    int max_length;
    int current; // last used element
    int completed;

} typedef TLB;


TLB *start_tlb(int size){

    
    TLB *t = (TLB *) calloc(1, sizeof(TLB));
    if(t == NULL) exit(-1);

    t->pages = (int *) calloc(TLB_SIZE, sizeof(int));
    t->frames = (int *) calloc(TLB_SIZE, sizeof(int));

    if(t->pages == NULL || t->frames == NULL) exit(-1);

    t->current = -1;
    t->completed = 0;
    t->max_length = size;

    return t;
}

int search_TLB(int value, TLB *t){

    int start = 0;
    int end = t->max_length;

    if(t->completed == 0){
        end = t->current + 1;
    }


    for (int i = start; i < end; i++){
    
        // page in TLB
        if(t->pages[i] == value) return i;

    }

    return -1;
}


int insert_TLB(int page, int frame, TLB *t){

    // using FIFO

    if(search_TLB(page, t) != -1) return -1; // already on TLB


    // change the position of the current pointer
    // it should point to the last inserted position
    t->current++;
    if(t->current == t->max_length){
        t->completed = 1;
        t->current = 0;
    }

    // write values to tlb (overwrite the last value)
    t->pages[t->current] = page;
    t->frames[t->current] = frame;  
    
    return t->current;
}

int load_memory(int page_number, char **memory, int mem_size){

    FILE *b_store = fopen(BACKING_STORE_FILE, "rb");

    if(b_store == NULL){
        printf("Error oppening %s\n", BACKING_STORE_FILE);
        exit(-1);
    }

    fseek(b_store, 256 * page_number, SEEK_SET);

    char *page = (char *) calloc(256, sizeof(char));
    if(page == NULL) exit(-1);

    fread(page, sizeof(char), 256, b_store);

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
    char **memory = (char **) calloc(MEM_SIZE, sizeof(char *));
    if(memory == NULL) exit(-1);

    // represents a page table
    // page_table[page number] == frame
    int page_table[MEM_SIZE];
    for(int i = 0; i < MEM_SIZE; i++) page_table[i] = PAGE_FAULT;

    TLB *t = start_tlb(TLB_SIZE);

    do{
        unsigned int logical_address;
        fscanf(address_file, "%d", &logical_address);

        // a unsigned int has a size of 32 bits
        // we want to extract 16 bits from the right 
        
        // since a integer has 4 bytes (32 bits)
        // we can think of the maximum value as:
        // FFFF FFFF
        // we want only the 16 less significant bits
        // so we will use the following mask:
        // FFFF (65535)  
        unsigned int page_offset = logical_address & 0xFFFF;

        // the same idea applies to the extraction of page number
        // and the offset  

        // page number is shifted by 1 byte so we can get it
        // in the correct range  
        unsigned int page_number = (page_offset & 0xFF00) >> 8;
        unsigned int offset = page_offset & 0x00FF;

        printf("Logical address: %u - page+offset: %u+%u\n", logical_address, page_number, offset);

        int in_tlb = search_TLB(page_number, t);

        if(in_tlb == -1){

            if(page_table[(int) page_number] == PAGE_FAULT){
               
                page_fault_aumont++;
               
                int frame = load_memory((int) page_number, memory, MEM_SIZE);
                page_table[(int) page_number] = frame;
            }

            insert_TLB(page_number, page_table[page_number], t);
        }else{
            found_in_tlb++;
            printf("Found in TLB\n");
        }

        total_memoy_access++;

        printf("Physical address: %u - frame+offset: %u+%u\n", 256 * page_table[(int) page_number] + offset, page_table[(int) page_number], offset);


        char byte_value = *(memory[page_table[(int) page_number]] + offset);

        printf("Value of %u+%u: %c  (%u)\n\n", page_number, offset, byte_value, (unsigned int) byte_value);

    }while(feof(address_file) == 0);

    fclose(address_file);

    for(int i = 0; i < MEM_SIZE; i++) if(memory[i] != NULL) free(memory[i]);
    free(memory);

    free(t->pages);
    free(t->frames);
    free(t);

    printf(
        "\n\n\n=========\nPage faults: %d (%.2f %%)\n TLB uses: %d (%.2f %%)\n Total accesses: %d\n",
        page_fault_aumont,
        page_fault_aumont/(double) total_memoy_access * 100,
        found_in_tlb,
        found_in_tlb/(double) total_memoy_access * 100,
        total_memoy_access
    );

    return 0;
}
