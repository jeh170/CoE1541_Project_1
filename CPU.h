
#ifndef TRACE_ITEM_H
#define TRACE_ITEM_H

// this is tpts
enum trace_item_type {
	ti_NOP = 0,
	ti_RTYPE,
	ti_ITYPE,
	ti_LOAD,
	ti_STORE,
	ti_BRANCH,
	ti_JTYPE,
	ti_SPECIAL,
	ti_JRTYPE,
	ti_FLUSHED,
	ti_EMPTY
};

enum stage_type {
	IF1 = 0,
	IF2,
	ID,
	EX,
	MEM1,
	MEM2,
	WB
};

enum hazard_type {
	hz_STRUCT = 1,
	hz_DATA1,
	hz_DATA2,
	hz_CTRL
};

struct trace_item {
	unsigned char type;			// see above
	unsigned char sReg_a;			// 1st operand
	unsigned char sReg_b;			// 2nd operand
	unsigned char dReg;			// dest. operand
	unsigned int PC;			// program counter
	unsigned int Addr;			// mem. address
};

#endif

#define TRACE_BUFSIZE 1024*1024

static FILE *trace_fd;
static int trace_buf_ptr;
static int trace_buf_end;
static struct trace_item *trace_buf;
static FILE *out_fd;

int is_big_endian(void)
{
	union {
		uint32_t i;
		char c[4];
	} bint = { 0x01020304 };

	return bint.c[0] == 1;
}

uint32_t my_ntohl(uint32_t x)
{
	u_char *s = (u_char *)&x;
	return (uint32_t)(s[3] << 24 | s[2] << 16 | s[1] << 8 | s[0]);
}

void trace_init()
{
	trace_buf = malloc(sizeof(struct trace_item) * TRACE_BUFSIZE);

	if (!trace_buf) {
		fprintf(stdout, "** trace_buf not allocated\n");
		exit(-1);
	}

	trace_buf_ptr = 0;
	trace_buf_end = 0;
}

void trace_uninit()
{
	free(trace_buf);
	fclose(trace_fd);
}

int trace_get_item(struct trace_item **item)
{
	int n_items;

	if (trace_buf_ptr == trace_buf_end) {	/* if no more unprocessed items in the trace buffer, get new data  */
		n_items = fread(trace_buf, sizeof(struct trace_item), TRACE_BUFSIZE, trace_fd);
		if (!n_items) return 0;				/* if no more items in the file, we are done */

		trace_buf_ptr = 0;
		trace_buf_end = n_items;			/* n_items were read and placed in trace buffer */
	}

	*item = &trace_buf[trace_buf_ptr];	/* read a new trace item for processing */
	trace_buf_ptr++;

	if (is_big_endian()) {
		(*item)->PC = my_ntohl((*item)->PC);
		(*item)->Addr = my_ntohl((*item)->Addr);
	}

	return 1;
}

int write_trace(struct trace_item item, char *fname)
{
	out_fd = fopen(fname, "a");
	int n_items;
	if (is_big_endian()) {
		(&item)->PC = my_ntohl((&item)->PC);
		(&item)->Addr = my_ntohl((&item)->Addr);
	}

	n_items = fwrite(&item, sizeof(struct trace_item), 1, out_fd);
	fclose(out_fd);
	if (!n_items) return 0;				/* if no more items in the file, we are done */

		
	return 1;
}

int push_stages_from(struct trace_item *stages[], int pushFrom, struct trace_item *out) {
	out = stages[WB];
	for (int i = WB; i > pushFrom; i--) {
		stages[i] = stages[i-1];	// push non-stalled instrs down pipeline
	}
	stages[pushFrom]->type = ti_NOP;	// insert nop to stall pipeline
	return 0;
}

int branch_check(struct trace_item *stages[], int mode);

int hazard_detect(struct trace_item *stages[], int prediction_mode, struct trace_item* out) {
	int detected = 0;
	
	// check for control hazard (if hazard while branch/jump in EX, flush first 3 instrs)
	if (stages[EX]->type == ti_BRANCH) {
		printf("Control hazard\n");
		detected = branch_check(stages, prediction_mode);
	}

	// check for structural hazard (instr trying to WB while decoding)
	else if ((stages[WB]->type >= ti_RTYPE) && (stages[WB]->type <= ti_LOAD)) {
		printf("struct hazard detected\n");
		push_stages_from(stages, EX, out);
		detected = hz_STRUCT;
	}

	// check for data hazard (LW in EX/MEM1 or MEM1/MEM2 w/ data dependency in ID/EX)
	else if (stages[EX]->type == ti_LOAD) { 	// which stage to check from depends on how we load the pipeline
		if (stages[EX]->dReg == stages[ID]->dReg) {
			printf("data hazard detected\n");
			push_stages_from(stages, EX, out);
			detected = hz_DATA1;
		}
	}
	else if (stages[MEM1]->type == ti_LOAD) {
		if (stages[MEM1]->dReg == stages[ID]->dReg) {
			printf("data hazard detected\n");
			push_stages_from(stages, EX, out);
			detected = hz_DATA2;
		}
	}

	return detected;
}


/** BRANCH PREDICTION **/

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

struct trace_item FLUSHED = { .type = ti_FLUSHED };

static int* prediction_table;
static int table_size = 64;

int flush_stages(struct trace_item *stages[]);
int check_branch(struct trace_item *stages[]);
int get_prediction(struct trace_item *stages[], int mode);
int check_store_prediction(struct trace_item *stages[], int branch_result, int mode);

void set_up_table(int size)
{
    table_size = size;
    prediction_table = (int*)malloc(sizeof(int)*size);
}

// predicts whether a branch will be taken, affecting what gets loaded into the pipeline
// 0 -> not taken, 1 -> taken
int branch_predict(struct trace_item *stages[], int mode)
{
    if (stages[0]->type == ti_BRANCH)
        return get_prediction(stages, mode);
    return 1;
}

// checks the prediction stored and flushes IF1/2 and ID
int branch_check(struct trace_item *stages[], int mode)
{
    if (stages[3]->type == ti_BRANCH)
    {	printf("Checking branch\n");
        int branch_result = check_branch(stages);
        if (!check_store_prediction(stages, branch_result, mode))
            printf("checked prediction\n");
            return hz_CTRL;
    }
    return 0;
}


/*Utility Methods*/

// flushes stages in the event of an incorrect prediction
int flush_stages(struct trace_item *stages[])
{
    for (int i = 0; i < 4; i++){
        stages[i] = &FLUSHED;
    }

    return 0;
}

// Returns 1 if to be taken, 0 if not to be taken
int check_branch(struct trace_item *stages[])
{
    int target_addr = stages[3]->Addr;
    int next_addr = stages[2]->PC;

    if (target_addr == next_addr)
        return -1;
    return 0;
}

//Hashes address for table
int hash_for_table(int addr)
{
	printf("hashing\n");
	int temp = addr >> 3;
	printf("still hashing\n");
	printf("%d\n", table_size);
	return temp % table_size;
    return (addr >> 3) % table_size;
}

// Gets the prediction for this branch or stores a new one
int get_prediction(struct trace_item *stages[], int mode)
{
    int addr;
    if (mode != 0) 
        addr = hash_for_table(stages[0]->PC);
    
    switch (mode){
        case 0:
            return 0;
        case 1:
            return prediction_table[addr];
        case 2:
            return prediction_table[addr] > 1;
    }
    return 0;
}

// The prediction in order to determine flushing
// Returns -1 if prediction was correct, 0 if not
int check_store_prediction(struct trace_item *stages[], int branch_result, int mode)
{
	printf("checking stored prediction\n");
    int addr = hash_for_table(stages[0]->PC);

    if (mode == 0){
    	printf("no prediction mode check\n");
        return branch_result == 0;
    }
    if (mode == 1){
    	printf("1 bit check\n");
        int prediction = prediction_table[addr];
        if (branch_result == prediction)
            return -1;
        else{
            prediction_table[addr] = branch_result;
            return 0;
        }
    }
    if (mode == 2){
    	printf("1 bit check\n");
        int prediction = prediction_table[addr];
        int prediction_outcome = (prediction > 1);
        int predict_check = (branch_result == prediction_outcome);

        if (predict_check){
            if (prediction_outcome)
                prediction_table[addr] = br2_TAKEN;
            else
                prediction_table[addr] = br2_NOT_TAKEN;
        }
        else{
            if (prediction_outcome)
                prediction_table[addr] = br2_TAKEN_FAIL;
            else
                prediction_table[addr] = br2_NOT_TAKEN_FAIL;
        }
        return predict_check;
    }
    else
        return branch_result == 0;
    
}
