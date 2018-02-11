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
    
}

int get_prediction(const struct trace_item stages[]){
    return 0;
}