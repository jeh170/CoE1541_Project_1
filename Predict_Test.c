#include "branch_predict.h"

int run_test(int (*t)(), int test_num);

int check_branch_taken_TEST();
int check_branch_nottaken_TEST();

int main(){
    run_test((*check_branch_taken_TEST), 1);
    run_test((*check_branch_nottaken_TEST), 2);
}

int run_test(int (*t)(), int test_num){
    if (t())
        printf("%i: PASSED\n", test_num);
    else
        printf("%i: FAILED\n", test_num);
}

int check_branch_taken_TEST(){
    struct trace_item stages[7];
    struct trace_item if2, id;
    if2.PC = 1234;
    id.Addr = 1234;

    stages[2] = if2;
    stages[3] = id;

    if (check_branch(stages))
        return -1;
    return 0;
}

int check_branch_nottaken_TEST(){
    struct trace_item stages[7];
    struct trace_item if2, id;
    if2.PC = 1234;
    id.Addr = 4321;

    stages[2] = if2;
    stages[3] = id;

    if (!check_branch(stages))
        return -1;
    return 0;
}