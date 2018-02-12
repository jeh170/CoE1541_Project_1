#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

const struct trace_item FLUSHED = { .type = ti_FLUSHED };

int branch_predict(struct trace_item stages[], int mode){
    
}

int flush_stages(struct trace_item stages[]){
    for (int i = 0; i < 3; i++){
        stages[i] = FLUSHED;
    }

    return 0;
}

// Returns 1 if to be taken, 0 if not to be taken
int check_branch(const struct trace_item stages[]){
    int target_addr = stages[2].Addr;
    int next_addr = stages[1].PC;

    if (target_addr == next_addr)
        return -1;
    else 
        return 0;
}

int check_prediction(const struct trace_item stages[], int branch_result, int branch_pc){
    return branch_result == 0;
}