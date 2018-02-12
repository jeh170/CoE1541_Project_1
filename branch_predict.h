#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

enum branch_result {
    br_NOT_TAKEN = 0,
    br_TAKEN
};

const struct trace_item FLUSHED = { .type = ti_FLUSHED };

static int* prediciton_table; 
static int table_size;

int flush_stages(struct trace_item stages[]);
int check_branch(const struct trace_item stages[]);
int get_store_prediction(const struct trace_item stages[], int mode);
int check_prediction(const struct trace_item stages[], int branch_result, int mode);

void set_up_table(int size)
{
    table_size = size;
    prediciton_table = (int*)malloc(sizeof(int)*size);
}

// predicts whether a branch will be taken, affecting what gets loaded into the pipeline
// 0 -> not taken, 1 -> taken
int branch_predict(struct trace_item stages[], int mode){
    if (stages[0].type == ti_BRANCH)
        return get_store_prediction(stages, mode);
    return 1;
}

int branch_check(struct trace_item stages[], int mode){
    if (stages[3].type == ti_BRANCH)
    {
        int branch_result = check_branch(stages);
        if (!check_prediction(stages, branch_result, mode))
            flush_stages(stages);
    }
}

// flushes stages in the event of an incorrect prediction
int flush_stages(struct trace_item stages[]){
    for (int i = 0; i < 4; i++){
        stages[i] = FLUSHED;
    }

    return 0;
}

// Returns 1 if to be taken, 0 if not to be taken
int check_branch(const struct trace_item stages[]){
    int target_addr = stages[3].Addr;
    int next_addr = stages[2].PC;

    if (target_addr == next_addr)
        return -1;
    return 0;
}

int hash_for_table(int addr)
{
    return (addr >> 3) % table_size;
}

// Gets the prediciton for this branch or stores a new one
int get_store_prediction(const struct trace_item stages[], int mode){
    int addr = hash_for_table(stages[0].PC);
    
    switch (mode){
        case 0:
            return 0;
        case 1:
            return prediciton_table[addr];
        case 2:
            return 0; //TODO: add prediciton table
    }
}

// The prediction in order to determine flushing
int check_prediction(const struct trace_item stages[], int branch_result, int mode){
    int addr = hash_for_table(stages[0].PC);

    switch (mode){
        case 0:
            return branch_result == 0;
        case 1:
            int prediction = prediciton_table[addr];
            if (branch_result == prediction)
                return -1;
            else
            {
                prediciton_table[addr] = branch_result;
                return 0;
            }
        case 2:
            return 0;
    }
    
    return branch_result == 0;
}