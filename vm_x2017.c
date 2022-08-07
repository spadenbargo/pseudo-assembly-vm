#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "vm_x2017.h"


BYTE run_vm(BYTE *VM_REGISTERS, BYTE *VM_RAM, struct Function_array *code_space);

int main(int argc, char **argv)
{
    char *filename = argv[1];
    if (argc != 2)
    {
        printf("Invalid arguments, please pass in <x2017_binary>\n");
        return -1;
    }

    FILE *f_ptr = fopen(filename, "rb");
    fseek(f_ptr, 0, SEEK_END);
    size_t file_length = ftell(f_ptr);


    struct Function_array read_in_funcs;
    read_in_funcs.size = 0;
    read_in_funcs.capacity = 8;
    
    for (int i = 0; i < read_in_funcs.capacity; i++)
    {
        read_in_funcs.func_arr[i].func_label = 0;
    }
    int main_func_count = 0;
    int main_func_index = -1;

    BYTE curr_buffer;

    fseek(f_ptr, -1, SEEK_END);
    curr_buffer = fgetc(f_ptr);

    BYTE num_of_OPS;

    num_of_OPS = read_byte_slice(curr_buffer, 3, 0);

    BYTE function_found = 1; // true

    fseek(f_ptr, -2, SEEK_END);
    curr_buffer = fgetc(f_ptr);

    BYTE next_num_read_bits;
    BYTE already_read_bits = 0;

    BYTE curr_func;
    BYTE readin_datatype;
    BYTE curr_data_val;

    int func_count = 0;
    BYTE curr_func_label;

    BYTE find_next_ret;
    BYTE find_next_num_OPS;

    for (BYTE i = 2; i < file_length + 1;) // whole file
    {
        if (function_found)
        {
            for (BYTE j = 0; j < num_of_OPS - 1; j++) // num of opcodes in function exluding RET
            {
                read_in_funcs.func_arr[func_count].num_ops = num_of_OPS;
                next_num_read_bits = 3; // 3 bits to get OPCODE

                curr_func = next_parsed_data(&i, *&f_ptr, &curr_buffer,
                                            next_num_read_bits, &already_read_bits);
                read_in_funcs.func_arr[func_count].op_arr[j + 1].opcode = curr_func;
                // printf("\n\nCurr func: %s\n", functions_str[read_in_funcs.func_arr[func_count].op_arr[j + 1].opcode]);
                for (int k = 0; k < func_num_param[curr_func]; k++) // for each param in func
                {
                    next_num_read_bits = 2; // 2 bits determine the next data type being read
                    readin_datatype = next_parsed_data(&i, *&f_ptr, &curr_buffer,
                                                    next_num_read_bits, &already_read_bits);
                    read_in_funcs.func_arr[func_count].op_arr[j + 1].param_type[k].type = readin_datatype;

                    next_num_read_bits = data_type_bit_size[readin_datatype]; // number bits corresponding to read in data type
                    curr_data_val = next_parsed_data(&i, *&f_ptr, &curr_buffer,
                                                    next_num_read_bits, &already_read_bits);
                    switch (readin_datatype)
                    {
                    case VAL:
                        read_in_funcs.func_arr[func_count].op_arr[j + 1].params[k].VAL = curr_data_val;
                        break;
                    case REG:
                        read_in_funcs.func_arr[func_count].op_arr[j + 1].params[k].REG = curr_data_val;
                        break;
                    case STK:
                        read_in_funcs.func_arr[func_count].op_arr[j + 1].params[k].STK = curr_data_val;
                        break;
                    case PTR:
                        read_in_funcs.func_arr[func_count].op_arr[j + 1].params[k].PTR = curr_data_val;
                        break;
                    default:
                        break;
                    }
                    // printf("dataype: %d, dataVal: %d\n", readin_datatype, curr_data_val);
                }
            }
            next_num_read_bits = 3; //
            curr_func_label = next_parsed_data(&i, *&f_ptr, &curr_buffer,
                                                next_num_read_bits, &already_read_bits);
            read_in_funcs.func_arr[func_count].func_label = curr_func_label;
            // printf("readin funlabel: %d\n", read_in_funcs.func_arr[func_count].func_label);

            if (curr_func_label == 0)
            {
                main_func_count++;
                main_func_index = func_count;
            }
            // printf("func label: %d\n", curr_func_label);
            read_in_funcs.size++;
            function_found = 0;
        }
        else
        {
            //            printf("lookin for func\n");
            next_num_read_bits = 5; // 5 bits to get num of OPCODE in a function
            find_next_num_OPS = next_parsed_data(&i, *&f_ptr, &curr_buffer,
                                                next_num_read_bits, &already_read_bits);
            next_num_read_bits = 3; // see if next bits resemble RET function
            find_next_ret = next_parsed_data(&i, *&f_ptr, &curr_buffer,
                                            next_num_read_bits, &already_read_bits);
            if (find_next_num_OPS > 0 && find_next_ret == RET)
            {
                function_found = 1;
                num_of_OPS = find_next_num_OPS;
                func_count++;
                //                printf("found!!!\n");
            }
        }
    }
    normalize_stk_ptr_symbols(&read_in_funcs);

    if(main_func_count != 1)
    {
        printf("Only 1 main function allowed\n");
        return 1;
    }
    sort_funcs(&read_in_funcs, main_func_index);

    BYTE VM_REGISTERS[] = {
        0b00000000, // 000 // general purpose
        0b00000000, // 001 // general purpose
        0b00000000, // 010 // general purpose
        0b00000000, // 011 // general purpose
        0b00000000, // 100 // personal use
        0b00000000, // 101 // personal use
        0b00000000, // 110 // personal use
        0b00000000, // 111 // Program Counter
    };

    BYTE VM_RAM[256];
    run_vm(VM_REGISTERS, VM_RAM, &read_in_funcs);

    // base case is calling func 0
    // if cal is called then

    // VM START

    return 0;
}

BYTE run_vm(BYTE *VM_REGISTERS, BYTE *VM_RAM, struct Function_array *code_space)
{
    // BYTE *REG_4_func_ptr = &VM_REGISTERS[4]; // not in use

    BYTE *REG_5_frame_ptr = &VM_REGISTERS[5];
    BYTE *REG_6_stack_ptr = &VM_REGISTERS[6];
    BYTE *REG_7_PC = &VM_REGISTERS[7];

    BYTE curr_function = read_byte_slice((*REG_6_stack_ptr), 0, 5); // read >xxx<xxxxx

    for (; (*REG_7_PC) < code_space->func_arr[curr_function].num_ops; (*REG_7_PC)++)
    {
        BYTE PC_backwards = code_space->func_arr[curr_function].num_ops - (*REG_7_PC) - 1;

        BYTE curr_op_num_param = func_num_param[code_space->func_arr[curr_function].op_arr[PC_backwards].opcode];

        BYTE *loaded_params[2];
        BYTE loaded_vals[2];

        for (int i = 0; i < curr_op_num_param; i++)// fetch
        {
            BYTE type_param = code_space->func_arr[curr_function].op_arr[PC_backwards].param_type[i].type;
            // BYTE stackframe_index = (*REG_5_frame_ptr) + read_byte_slice((*REG_6_stack_ptr), 3, 0); // curr stackframe + stack pointer offset (xxx>xxxxx<) // for checking stack overflow :TODO
            BYTE local_stk_size = read_byte_slice((*REG_6_stack_ptr), 3, 0);

            switch (type_param)
            {
            case VAL:
                loaded_vals[i] = code_space->func_arr[curr_function].op_arr[PC_backwards].params[i].VAL;
                loaded_params[i] = &loaded_vals[i];
                break;
            case REG:
                loaded_params[i] = &VM_REGISTERS[code_space->func_arr[curr_function].op_arr[PC_backwards].params[i].REG];
                break;
            case STK:
                // for(int j = 0; j < code_space->func_arr[curr_function].num_symbols)
                if (code_space->func_arr[curr_function].op_arr[PC_backwards].params[i].STK + 1 > local_stk_size)
                {
                    (*REG_6_stack_ptr)++;
                }
                loaded_params[i] = &VM_RAM[code_space->func_arr[curr_function].op_arr[PC_backwards].params[i].STK + (*REG_5_frame_ptr)];
                break;
            case PTR:
                if (code_space->func_arr[curr_function].op_arr[PC_backwards].params[i].PTR + 1 > local_stk_size)
                {
                    (*REG_6_stack_ptr)++;
                }
                loaded_params[i] = &VM_RAM[code_space->func_arr[curr_function].op_arr[PC_backwards].params[i].PTR + (*REG_5_frame_ptr)];
                // loaded_vals[i] = code_space->func_arr[curr_function].op_arr[PC_backwards].params[i].PTR;
                // loaded_params[i] = &loaded_vals[i];
                break;
            default:
                break;
            }
        }

        switch (curr_op_num_param) // filter operations by number of parameters
        {
        case 0:
            *REG_5_frame_ptr = (*REG_5_frame_ptr) - (*REG_6_stack_ptr); // pop off current stack frame

            if((*REG_5_frame_ptr ) >= 2)
            {
                (*REG_5_frame_ptr)--;
                *REG_6_stack_ptr = VM_RAM[(*REG_5_frame_ptr)]; // return to prev func label stack pointer
                (*REG_5_frame_ptr)--;
                *REG_7_PC = VM_RAM[(*REG_5_frame_ptr)]; // return to prev PC

                return func_PTR_0arg[RET]();
            }
            return func_PTR_0arg[RET]();
            
            break;
        case 1:
            func_PTR_1arg[code_space->func_arr[curr_function].op_arr[PC_backwards].opcode](*&loaded_params[0]);
            if (code_space->func_arr[curr_function].op_arr[PC_backwards].opcode == CAL)
            {
                // return address stores current PC in ram
                // changes current fram ptr to next lot of free space
                VM_RAM[(*REG_6_stack_ptr) + (*REG_5_frame_ptr)] = PC_backwards; // prev program counter == frame ptr-2
                (*REG_6_stack_ptr)++;
                *REG_7_PC = 0b00000000; // new PC in new function
                VM_RAM[(*REG_6_stack_ptr) + (*REG_5_frame_ptr)] = (*REG_6_stack_ptr); // prev func_label | stack pointer
                (*REG_6_stack_ptr)++; // next free spot     TODO: implement stack overflow checker
                *REG_5_frame_ptr += read_byte_slice((*REG_6_stack_ptr),3,0); // xxx|xxxxx|
                // new offset stack address xxx >xxxxx<
                *REG_6_stack_ptr = 0b00000000;
                // new function label >xxx< xxxxx
                *REG_6_stack_ptr = (*loaded_params[0]) << 5 | (*REG_6_stack_ptr);
                run_vm(*&VM_REGISTERS, *&VM_RAM, *&code_space);
                // BYTE next_funct = run_vm(*&VM_REGISTERS, *&VM_RAM, *&code_space);
            }
            break;
        case 2:
            func_PTR_2arg[code_space->func_arr[curr_function].op_arr[PC_backwards].opcode](*&loaded_params[0], *&loaded_params[1]);
            break;
        default:
            break;
        }
    }

        return 0;
}
int normalize_stk_ptr_symbols(struct Function_array *input_func_arr)
{
    for (int i = input_func_arr->size - 1; i >= 0; i--)
    {
        input_func_arr->func_arr[i].op_arr[0].opcode = RET;
        int normalize_symbols[32];
        int normalize_symbols_count = 0;
        for (int h = 0; h < 32; h++)
        {
            normalize_symbols[h] = -1;
        }
        for (int j = input_func_arr->func_arr[i].num_ops - 1; j >= 0; j--)
        {

            struct op_data *curr_edit = &(input_func_arr->func_arr[i].op_arr[j]);
            for (int k = 0; k < func_num_param[curr_edit->opcode]; k++)
            {
                switch (curr_edit->param_type[k].type)
                {
                case VAL:
                    break;
                case REG:
                    break;
                case STK:
                    if (normalize_symbols[curr_edit->params[k].STK] == -1)
                    {
                        normalize_symbols[curr_edit->params[k].STK] = normalize_symbols_count;
                        normalize_symbols_count++;
                    }
                    break;
                case PTR:
                    if (normalize_symbols[curr_edit->params[k].PTR] == -1)
                    {
                        normalize_symbols[curr_edit->params[k].PTR] = normalize_symbols_count;
                        normalize_symbols_count++;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        for (int j = input_func_arr->func_arr[i].num_ops - 1; j >= 0; j--)
        {
            struct op_data *curr_edit = &(input_func_arr->func_arr[i].op_arr[j]);
            for (int k = 0; k < func_num_param[curr_edit->opcode]; k++)
            {
                switch (curr_edit->param_type[k].type)
                {
                case VAL:
                    break;
                case REG:
                    break;
                case STK:
                    if (normalize_symbols[curr_edit->params[k].STK] != -1)
                    {
                        curr_edit->params[k].STK = normalize_symbols[curr_edit->params[k].STK];
                    }
                    break;
                case PTR:
                    if (normalize_symbols[curr_edit->params[k].PTR] != -1)
                    {
                        curr_edit->params[k].PTR = normalize_symbols[curr_edit->params[k].PTR];
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
    return 0;
}

int sort_funcs(struct Function_array *unsorted_arr,int main_func_index)// insert check for duplicate functions
{
    struct Function temp_main = unsorted_arr->func_arr[main_func_index];
    unsorted_arr->func_arr[main_func_index] = unsorted_arr->func_arr[0];
    unsorted_arr->func_arr[0] = temp_main;
    for (int i = 1; i < unsorted_arr->capacity; i++)
    {
        if (unsorted_arr->func_arr[i].func_label != i)
        {
            struct Function temp = unsorted_arr->func_arr[i];
            for (int j = 0; j < unsorted_arr->capacity; j++)
            {
                if (unsorted_arr->func_arr[j].func_label == i)
                {
                    unsorted_arr->func_arr[i] = unsorted_arr->func_arr[j];
                    unsorted_arr->func_arr[j] = temp;
                    break;
                }
            }
        }
    }
    return 0;
}

// copy B to A accept registers or stack A != VAL
BYTE MOV_FUNC(BYTE *A, BYTE *B) // 000
{
    *A = (*B);
    return 1;
}
// takes in VAL: A containing function label
BYTE CAL_FUNC(BYTE *A) // 001
{
    // create new stack and push instructions
    return 1;
}
// terminate current function leave current stack frame
BYTE RET_FUNC() // 010
{
    // terminate current function
    return 0;
}
// stores stack symbol B in A: A <- B
BYTE REF_FUNC(BYTE *A, BYTE *B) // 011
{
    *A = (*B);
    return 1;
}
// takes two register address and adds their values and storing in first address
BYTE ADD_FUNC(BYTE *A, BYTE *B)
{
    *A += (*B);
    return 1;
}
// any address type and prints contents to new line of standard output as unsigned int
BYTE PRINT_FUNC(BYTE *A)
{
    printf("%d\n", (unsigned int)(*A));
    return 1;
}
// performs bitwise not operation on A and stores it in A. takes in register address
BYTE NOT_FUNC(BYTE *A)
{
    *A = ~(*A);
    return 1;
}
// takes register address and tests if it equals 0 if true then set 1 else set 0: stored in same reg
BYTE EQU_FUNC(BYTE *A)
{
    if ((*A) == 0)
    {
        *A = 0b00000001;
        return 1;
    }
    else
    {
        *A = 0b00000000;
        return 1;
    }
}

BYTE read_byte_slice(BYTE source_byte, BYTE left_start, BYTE right_end)
{
    BYTE all_ones = 0b11111111;
    BYTE result = source_byte & ((all_ones >> left_start) & (all_ones << right_end));
    result = result >> right_end;
    return result;
}

BYTE backward_trav_and_update(BYTE *index, FILE *FP)
{
    BYTE buff;
    (*index)++;
    fseek(FP, -(*index), SEEK_END); // update reading buffer
    buff = fgetc(FP);               // update reading buffer
    return buff;
}

BYTE next_parsed_data(BYTE *index, FILE *FP, BYTE *curr_buffer, BYTE next_data_num_bits, BYTE *already_read_bits)
{
    // read next 2 bits
    BYTE result;
    BYTE bit_pos_after_read = next_data_num_bits + (*already_read_bits);
    BYTE stop_read_from_left;

    if (bit_pos_after_read == 8)
    {
        stop_read_from_left = 8 - bit_pos_after_read; // recipocal
        result = read_byte_slice((*curr_buffer), stop_read_from_left, (*already_read_bits));
        (*curr_buffer) = backward_trav_and_update(*&index, FP); // move to next byte
        (*already_read_bits) = 0;
        return result;
    }
    else if (bit_pos_after_read > 8) // current byte doesnt contain enough bits for current read
    {
        stop_read_from_left = 16 - bit_pos_after_read; // recipocal
        BYTE buffer_lag = (*curr_buffer);
        (*curr_buffer) = backward_trav_and_update(*&index, FP); // update reading buffer
        BYTE read_bits_in_lag = 8 - (*already_read_bits);       // read all bits in prev lag so
        BYTE left_portion = read_byte_slice((*curr_buffer), stop_read_from_left, 0);
        BYTE right_portion = read_byte_slice(buffer_lag, 0, (*already_read_bits));
        // bitwise "OR" tocombine current buffer (A) with the previous buffer (B). res = A + B
        result = (left_portion << read_bits_in_lag) | right_portion;
        (*already_read_bits) = bit_pos_after_read - 8;
        return result;
    }
    else
    {
        stop_read_from_left = 8 - bit_pos_after_read; // recipocal
        result = read_byte_slice((*curr_buffer), stop_read_from_left, (*already_read_bits));
        (*already_read_bits) = bit_pos_after_read;
        return result;
    }
}
