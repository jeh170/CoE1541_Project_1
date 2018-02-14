#define TEST
#include "branch_predict.h"

int run_test(int (*t)(), int test_num);

int check_branch_taken_TEST();
int check_branch_nottaken_TEST();
int check_get_store_prediction0_TEST();
int check_get_store_prediction1_0_TEST();
int check_get_store_prediction1_1_TEST();

int main(){
    run_test((*check_branch_taken_TEST), 1);
    run_test((*check_branch_nottaken_TEST), 2);
    run_test((*check_get_store_prediction0_TEST), 3);
    run_test((*check_get_store_prediction1_0_TEST), 4);
    run_test((*check_get_store_prediction1_1_TEST), 5);
}

int run_test(int (*t)(), int test_num){
    if (t())
        printf("%i: PASSED\n", test_num);
    else
        printf("%i: FAILED\n", test_num);
}

// creates senario where branch is taken
int check_branch_taken_TEST(){
    struct trace_item stages[7];
    struct trace_item id, exe;
    id.type = ti_BRANCH;
    id.PC = 1234;
    exe.type = ti_LOAD;
    exe.Addr = 1234;

    stages[2] = id;
    stages[3] = exe;

    if (check_branch(stages))
        return -1;
    return 0;
}

// creates senario where branch is not taken
int check_branch_nottaken_TEST(){
    struct trace_item stages[7];
    struct trace_item id, exe;
    id.type = ti_BRANCH;
    id.PC = 1234;
    exe.type = ti_LOAD;
    exe.Addr = 4321;

    stages[2] = id;
    stages[3] = exe;

    if (!check_branch(stages))
        return -1;
    return 0;
}

int check_get_store_prediction0_TEST(){
    return get_prediction(NULL, 0) == 0;
}

int check_get_store_prediction1_0_TEST(){
    struct trace_item stages[7];
    struct trace_item if1;
    if1.PC = 32;
    if1.Addr = 4321;

    stages[0] = if1;

    set_up_table(64);
    prediciton_table[4] = 0;

    return get_prediction(stages, 1) == 0;
}

int check_get_store_prediction1_1_TEST(){
    struct trace_item stages[7];
    struct trace_item if1;
    if1.PC = 32;
    if1.Addr = 4321;

    stages[0] = if1;

    set_up_table(64);
    prediciton_table[4] = 1;

    return get_prediction(stages, 1) == 1;
}