#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/*REGISTER SYMBOL RANGE*/
// 0 <= reg <= 25
#define ASCII_A 65
#define ASCII_Z 90
// 26 <= reg <= 31
#define ASCII_a 97
#define ASCII_i 105

#define BYTE unsigned char
void str_cat(char *dest_str, char *addition_str);

enum func_sym
{
    MOV,   // 0
    CAL,   // 1
    RET,   // 2
    REF,   // 3
    ADD,   // 4
    PRINT, // 5
    NOT,   // 6
    EQU,   // 7
};

static const char *const functions_str[] = {
    [MOV] = "MOV",
    [CAL] = "CAL",
    [RET] = "RET",
    [REF] = "REF",
    [ADD] = "ADD",
    [PRINT] = "PRINT",
    [NOT] = "NOT",
    [EQU] = "EQU",
};

// data types
enum data_type
{
    VAL,
    REG,
    STK,
    PTR,
};

static const char *const data_type_str[] = {
    [VAL] = "VAL", // size: 8 bits // 00
    [REG] = "REG", // size: 3 bits // 01
    [STK] = "STK", // size: 5 bits // 10
    [PTR] = "PTR", // size: 5 bits // 11
};

static const BYTE data_type_bit_size[] = {
    [VAL] = 8, // size: 8 bits
    [REG] = 3, // size: 3 bits
    [STK] = 5, // size: 5 bits
    [PTR] = 5, // size: 5 bits
};

static const BYTE func_num_param[] = {
    [MOV] = 2,   // 2 param: register or stack, cant be val type, B->A
    [CAL] = 1,   // 1 param: byte (Value)
    [RET] = 0,   // no param
    [REF] = 2,   // 2 param: BOTH stack, B->A
    [ADD] = 2,   // 2 param: BOTH register, ? A = A+B ?
    [PRINT] = 1, // 1 param: any
    [NOT] = 1,   // 1 param: register
    [EQU] = 1,   // 1 param: register
};

// reading from left to right; when to start reading and when to stop reading a byte
BYTE read_byte_slice(BYTE source_byte, BYTE left_start, BYTE right_end);
// input next amount of required data and it will move the current read buffer along the file and update the parameters
BYTE next_parsed_data(BYTE *index, FILE *FP, BYTE *curr_buffer, BYTE next_data_num_bits, BYTE *already_read_bits);
// used to traverse the file backwards
BYTE backward_trav_and_update(BYTE *index, FILE *FP);

// converts stk/ptr value to ascii in range A-Z then a-e
char to_stk_symbol(BYTE input);
// union to store the value of parameters
union data
{
    BYTE VAL : 8;
    BYTE REG : 3;
    BYTE STK : 5;
    BYTE PTR : 5;
};
// 2 bits to store type
struct param_data_type
{
    BYTE type : 2;
};

struct op_data
{
    BYTE opcode : 3;
    struct param_data_type param_type[2];
    union data params[2];
};
struct Function
{
    BYTE func_label : 3;
    BYTE num_ops : 5;
    struct op_data op_arr[32];
};

// int bin(unsigned n);
#define MAX_CHAR 4096

struct Function_array
{
    struct Function func_arr[8];
    BYTE size;
    BYTE capacity;
};

int normalize_stk_ptr_symbols(struct Function_array *input_func_arr);

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

    BYTE curr_buffer;

    fseek(f_ptr, -1, SEEK_END);
    curr_buffer = fgetc(f_ptr);

    BYTE num_of_OPS;

    num_of_OPS = read_byte_slice(curr_buffer, 3, 0);

    if(num_of_OPS < 1 || read_byte_slice(curr_buffer, 0, 5) != RET)
    {
        printf("Invalid file, please pass in <x2017_binary>\n");
        return 0;
    }

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
            }
        }
    }

    normalize_stk_ptr_symbols(&read_in_funcs);

    char string_all[MAX_CHAR];
    string_all[0] = '\0';
    char indent[] = "    ";
    char line_str[30];
    char sub_str[15];
    line_str[0] = '\0';
    for (int i = read_in_funcs.size - 1; i >= 0; i--)
    {
        line_str[0] = '\0';
        sprintf(line_str, "FUNC LABEL %d\n", read_in_funcs.func_arr[i].func_label);
        str_cat(string_all, line_str);
        str_cat(string_all, "\0");
        for (int j = read_in_funcs.func_arr[i].num_ops - 1; j >= 0; j--)
        {
            line_str[0] = '\0';
            sub_str[0] = '\0';
            struct op_data curr_op_write = read_in_funcs.func_arr[i].op_arr[j];

            sprintf(sub_str, "%s%s", indent, functions_str[curr_op_write.opcode]);
            str_cat(line_str, sub_str);
            if (func_num_param[curr_op_write.opcode] != 0)
            {
                for (int k = 0; k < func_num_param[curr_op_write.opcode]; k++)
                {
                    sub_str[0] = '\0';
                    sprintf(sub_str, " %s", data_type_str[curr_op_write.param_type[k].type]);
                    str_cat(line_str, sub_str);

                    switch (curr_op_write.param_type[k].type)
                    {

                    case VAL:
                        sprintf(sub_str, " %d", curr_op_write.params[k].VAL);
                        break;

                    case REG:
                        sprintf(sub_str, " %d", curr_op_write.params[k].REG);
                        break;

                    case STK:
                        sprintf(sub_str, " %c", to_stk_symbol(curr_op_write.params[k].STK));
                        break;

                    case PTR:
                        sprintf(sub_str, " %c", to_stk_symbol(curr_op_write.params[k].PTR));
                        break;

                    default:
                        printf("uhhhhhhhhhhhh\n");
                        break;
                    }
                    str_cat(line_str, sub_str);
                }
            }

            str_cat(line_str, "\n");
            str_cat(string_all, line_str);
        }
    }
    str_cat(string_all, "\0");
    printf("%s", string_all);

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

// assumption 1: that the true main function is already in index 0
// assumption 2: non functions are labelled 0 exception for main
// void sort_index(int *arr, int size)
char to_stk_symbol(BYTE input)
{
    // BYTE temp = 0b00011111 & input;
    int temp_int;
    char res;
    if (input < 27)
    {
        temp_int = ASCII_A + input;
        res = (char)temp_int;
        return res;
    }
    else
    {
        temp_int = ASCII_a + input;
        res = (char)temp_int;
        return res;
    }
}

void str_cat(char *dest_str, char *addition_str)
{
    char copy[MAX_CHAR / 2];
    int i = 0;
    for (; dest_str[i] != '\0'; i++)
    {
        copy[i] = dest_str[i];
    }
    copy[i] = '\0';
    sprintf(dest_str, "%s%s", copy, addition_str);
}

// reading from left to right; when to start reading and when to stop reading a byte
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
/*
return byte of requested data in the specifications
requires file, file index, current buffer, size of next data in bits, 
current position in current byte
updates file index and position in parsing byte
*/
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
