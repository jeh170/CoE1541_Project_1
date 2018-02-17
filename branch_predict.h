#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

enum branch_result_1bit {
    br1_NOT_TAKEN = 0,
    br1_TAKEN
};

enum branch_result_2bit{
    br2_NOT_TAKEN = 0,
    br2_NOT_TAKEN_FAIL,
    br2_TAKEN_FAIL,
    br2_TAKEN
};

const struct trace_item FLUSHED = { .type = ti_FLUSHED };

static int* prediciton_table; 
static int table_size;

int flush_stages(struct trace_item stages[]);
int check_branch(const struct trace_item stages[]);
int get_prediction(const struct trace_item stages[], int mode);
int check_store_prediction(const struct trace_item stages[], int branch_result, int mode);

void set_up_table(int size)
{
    table_size = size;
    prediciton_table = (int*)malloc(sizeof(int)*size);
}

// predicts whether a branch will be taken, affecting what gets loaded into the pipeline
// 0 -> not taken, 1 -> taken
int branch_predict(struct trace_item stages[], int mode)
{
    if (stages[0].type == ti_BRANCH)
        return get_prediction(stages, mode);
    return 1;
}

// checks the prediction stored and flushes IF1/2 and ID
int branch_check(struct trace_item stages[], int mode)
{
    if (stages[3].type == ti_BRANCH)
    {
        int branch_result = check_branch(stages);
        if (!check_store_prediction(stages, branch_result, mode))
            flush_stages(stages);
    }
}


/*Utility Methods*/

// flushes stages in the event of an incorrect prediction
int flush_stages(struct trace_item stages[])
{
    for (int i = 0; i < 4; i++){
        stages[i] = FLUSHED;
    }

    return 0;
}

// Returns 1 if to be taken, 0 if not to be taken
int check_branch(const struct trace_item stages[])
{
    int target_addr = stages[3].Addr;
    int next_addr = stages[2].PC;

    if (target_addr == next_addr)
        return -1;
    return 0;
}

//Hashes address for table
int hash_for_table(int addr)
{
    return (addr >> 3) % table_size;
}

// Gets the prediciton for this branch or stores a new one
int get_prediction(const struct trace_item stages[], int mode)
{
    int addr;
    if (mode != 0) 
        addr = hash_for_table(stages[0].PC);
    
    switch (mode){
        case 0:
            return 0;
        case 1:
            return prediciton_table[addr];
        case 2:
            return prediciton_table[addr] > 1;
    }
}

// The prediction in order to determine flushing
// Returns -1 if prediciton was correct, 0 if not
int check_store_prediction(const struct trace_item stages[], int branch_result, int mode)
{
    int addr = hash_for_table(stages[0].PC);

    if (mode == 0)
        return branch_result == 0;
    if (mode == 1){
        int prediction = prediciton_table[addr];
        if (branch_result == prediction)
            return -1;
        else{
            prediciton_table[addr] = branch_result;
            return 0;
        }
    }
    if (mode == 2){
        int prediction = prediciton_table[addr];
        int prediciton_outcome = prediction > 1;
        int predict_check = branch_result == prediciton_outcome;

        if (predict_check){
            if (prediciton_outcome)
                prediciton_table[addr] = br2_TAKEN;
            else
                prediciton_table[addr] = br2_NOT_TAKEN;
        }
        else{
            if (prediciton_outcome)
                prediciton_table[addr] = br2_TAKEN_FAIL;
            else
                prediciton_table[addr] = br2_NOT_TAKEN_FAIL;
        }
        return predict_check;
    }
    else
        return branch_result == 0;
    
}