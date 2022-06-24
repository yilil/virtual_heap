#include "virtual_alloc.c"
#include "virtual_sbrk.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void * virtual_heap = NULL;
void * programBreak = NULL;

void * virtual_sbrk(int32_t increment) {
    // Your implementation here (for your testing only)
    struct virtual_heap* heap = (struct virtual_heap*) virtual_heap;
    static int32_t increSize = 0;
    increSize += increment;

    void* new_heap = realloc(virtual_heap, heap->size + increSize);
    void* old_break = programBreak;
    if(new_heap == NULL){
        return (void *)(-1);
    }else{
        virtual_heap = new_heap;
        heap = (struct virtual_heap*) virtual_heap;
        programBreak = heap + sizeof(virtual_heap) + heap->size;
        return old_break;
    }
    return (void *)(-1);
}

int stdoutToFile(char* path){
    // Child process; save our current stdout file descriptor
    int saved_stdout = dup(1);
    //Open our file with writing permission, create a new file if the file does not exist 
    int file = open(path, O_WRONLY | O_CREAT, 0777);

    if(file == -1){
        printf("Error in output redirection!\n");
        return 2;
    }

    //Redirect our output to file
    int file2 = dup2(file, STDOUT_FILENO);
    close(file);

    //Printing results... this will be redirected to file
    virtual_info(virtual_heap);
    close(file2);

    //Reset our stdout
    dup2(saved_stdout, 1);
    return 0;
}

//Test on init fucntion
int initTest(){ 
    init_allocator(virtual_heap, 16, 8);
    stdoutToFile("./testcases/init.out");
    if(system("diff -w ./testcases/init.out ./testcases/init.txt") != 0){
        printf("initTest has failed!\n");
        return 1;
    }
    return 0;
}

//Test on malloc fucntion
int mallocTest(){ 
    init_allocator(virtual_heap, 15, 8);
    virtual_malloc(virtual_heap, 1000);
    virtual_malloc(virtual_heap, 2048);
    stdoutToFile("./testcases/malloc.out");
    if(system("diff -w ./testcases/malloc.out ./testcases/malloc.txt") != 0){
        printf("mallocTest has failed!\n");
        return 1;
    }
    return 0;
}

//Test on larger malloc function
int mallocLTest(){ 
    init_allocator(virtual_heap, 18, 8);
    virtual_malloc(virtual_heap, 8000);
    virtual_malloc(virtual_heap, 3000);
    virtual_malloc(virtual_heap, 4442);
    virtual_malloc(virtual_heap, 8192);
    stdoutToFile("./testcases/mallocL.out");
    if(system("diff -w ./testcases/mallocL.out ./testcases/mallocL.txt") != 0){
        printf("mallocLTest has failed!\n");
        return 1;
    }
    return 0;
}

//Test on free fucntion
int freeTest(){ 
    init_allocator(virtual_heap, 15, 8);
    void* block1 = virtual_malloc(virtual_heap, 1000);
    void* block2 = virtual_malloc(virtual_heap, 1000);
    virtual_free(virtual_heap, block1);
    virtual_free(virtual_heap, block2);
    stdoutToFile("./testcases/free.out");
    if(system("diff -w ./testcases/free.out ./testcases/free.txt") != 0){
        printf("freeTest has failed!\n");
        return 1;
    }
    return 0;
}

//Test on larger free fucntion
int freeLTest(){ 
    init_allocator(virtual_heap, 16, 10);
    void* block1 = virtual_malloc(virtual_heap, 3333);
    virtual_malloc(virtual_heap, 16245);
    void* block3 = virtual_malloc(virtual_heap, 1000);
    virtual_malloc(virtual_heap, 4444);
    virtual_free(virtual_heap, block1);
    virtual_free(virtual_heap, block3);
    stdoutToFile("./testcases/freeL.out");
    if(system("diff -w ./testcases/freeL.out ./testcases/freeL.txt") != 0){
        printf("freeLTest has failed!\n");
        return 1;
    }
    return 0;
}

//Test on realloc fucntion
int reallocTest(){ 
    init_allocator(virtual_heap, 15, 8);
    void* block1 = virtual_malloc(virtual_heap, 1000);
    void* block2 = virtual_malloc(virtual_heap, 5000);
    virtual_realloc(virtual_heap, block1, 3000);
    virtual_realloc(virtual_heap, block2, 2000);
    stdoutToFile("./testcases/realloc.out");
    if(system("diff -w ./testcases/realloc.out ./testcases/realloc.txt") != 0){
        printf("reallocTest has failed!\n");
        return 1;
    }
    return 0;
}

//Test on errors of each function
int errorTest(){ 
    init_allocator(virtual_heap, 10, 8);
    //Malloc a size greater than heap size
    void* block1 = virtual_malloc(virtual_heap, 32684);
    //Malloc with size 0
    void* block2 = virtual_malloc(virtual_heap, 0);
    //Free a block that was allocated
    virtual_free(virtual_heap, block1);
    virtual_free(virtual_heap, NULL);

    //Realloc a block that was not allocated
    virtual_realloc(virtual_heap, block1, 3000);
    //Realloc NULL
    virtual_realloc(virtual_heap, NULL, 0);
    //Realloc size 0
    virtual_realloc(virtual_heap, block2, 0);
    stdoutToFile("./testcases/error.out");
    if(system("diff -w ./testcases/error.out ./testcases/error.txt") != 0){
        printf("errorTest has failed!\n");
        return 1;
    }
    return 0;
}

//Test on large combos
int comboTest(){ 
    init_allocator(virtual_heap, 16, 8);
    void* block1 = virtual_malloc(virtual_heap, 1155);
    void* block2 = virtual_malloc(virtual_heap, 2048);
    void* block3 = virtual_malloc(virtual_heap, 2048);
    void* block4 = virtual_malloc(virtual_heap, 4444);
    block4 = virtual_realloc(virtual_heap, block4, 8888);
    virtual_free(virtual_heap, block2);
    virtual_free(virtual_heap, block1);
    block3 = virtual_realloc(virtual_heap, block3, 500);
    virtual_malloc(virtual_heap, 3333);
    stdoutToFile("./testcases/combo.out");
    if(system("diff -w ./testcases/combo.out ./testcases/combo.txt") != 0){
        printf("comboTest has failed!\n");
        return 1;
    }
    return 0;
}

int main() {
    // Your own testing code here
    printf("###############################\n");
    printf("---START RUNNING TEST CASES:---\n");
    int failed = 0;
    int success = 0;
    virtual_heap = malloc(1000000);

    initTest() == 1 ? failed++ : success++;
    mallocTest() == 1 ? failed++ : success++;
    mallocLTest() == 1 ? failed++ : success++;
    freeTest() == 1 ? failed++ : success++;
    freeLTest() == 1 ? failed++ : success++;
    reallocTest() == 1 ? failed++ : success++;
    errorTest() == 1 ? failed++ : success++;
    comboTest() == 1 ? failed++ : success++;

    printf("\n#RESULT OF TESTING: \n");
    printf("%d tests success! \n", success);
    printf("%d tests failed! \n", failed);
    printf("---END OF RUNNING TEST CASES:---\n");
    printf("###############################\n");
}