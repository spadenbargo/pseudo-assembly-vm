#ifndef VM_X2017
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define BYTE unsigned char
/******************************************************************************
* Enums and constant values
*/
// enum for function symbols to their corresponding bit value
enum func_sym
{
    MOV,   // 000
    CAL,   // 001
    RET,   // 010
    REF,   // 011
    ADD,   // 100
    PRINT, // 101
    NOT,   // 110
    EQU,   // 111
};
// enum for data type to their corresponding bit value
enum data_type
{
    VAL,   // 00
    REG,   // 01
    STK,   // 10
    PTR,   // 11
};
// enum data type mapped to their corresponding bit size used for bit requests when parsing
static const BYTE data_type_bit_size[] = {
    [VAL] = 8, // size: 8 bits
    [REG] = 3, // size: 3 bits
    [STK] = 5, // size: 5 bits
    [PTR] = 5, // size: 5 bits
};
// enum function parameters mapped to corresponding number of paramter each function takes 
static const BYTE func_num_param[] = {
    [MOV] = 2, // 2 param: register or stack, cant be val type, B->A
    [CAL] = 1, // 1 param: byte (Value)
    [RET] = 0, // no param
    [REF] = 2, // 2 param: BOTH stack, B->A
    [ADD] = 2, // 2 param: BOTH register, ? A = A+B ?
    [PRINT] = 1, // 1 param: any
    [NOT] = 1, // 1 param: register
    [EQU] = 1, // 1 param: register
};

union data
{
    BYTE VAL : 8;
    BYTE REG : 3;
    BYTE STK : 5;
    BYTE PTR : 5;
};
/******************************************************************************
* Data structures
*/
// 2 bits to hold type of data
struct param_data_type
{
    BYTE type : 2;
};
// opcode data contains its parameters, type information and opcode to be executed 
struct op_data
{
    BYTE opcode : 3;
    struct param_data_type param_type[2];
    union data params[2];
};
// function that contains the maximum ammount of operations per function and also contains:
// function label for it to be identified with
// 5 bits to represent how many operations exist within this function
// number of symbols to count how many stk/ptr references there are
struct Function
{
    BYTE func_label : 3;
    BYTE num_ops : 5;
    struct op_data op_arr[32];
    BYTE num_symbols : 3;
};
// wrapper struct that contains all of the functions also size shows how many exist
// capacity is 8 which is set upon initialisation
struct Function_array
{
    struct Function func_arr[8];
    BYTE size;
    BYTE capacity;
};
/******************************************************************************
* F U N C T I O N S
*******************************************************************************/
/******************************************************************************
* File reading functions
*/

// enter parameters of current write and the function will return next requested amount of bits
// with leading 0s
BYTE next_parsed_data(BYTE *index, FILE *FP, BYTE *curr_buffer, BYTE next_data_num_bits, BYTE *already_read_bits);
// rea
BYTE read_byte_slice(BYTE source_byte, BYTE left_start, BYTE right_end);

BYTE backward_trav_and_update(BYTE *index, FILE *FP);
int sort_funcs(struct Function_array *unsorted_arr, int main_func_index);
// order stk/ptr by first oder appearance to ascii
int normalize_stk_ptr_symbols(struct Function_array *input_func_arr);
// recursive vm calls self when another function is called within
BYTE run_vm(BYTE *VM_REGISTERS, BYTE *VM_RAM, struct Function_array *code_space);

// vm functions
BYTE MOV_FUNC(BYTE *A, BYTE *B); // 000
BYTE CAL_FUNC(BYTE *A);          // 001
BYTE RET_FUNC();                 // 010
BYTE REF_FUNC(BYTE *A, BYTE *B); // 011
BYTE ADD_FUNC(BYTE *A, BYTE *B); // 100
BYTE PRINT_FUNC(BYTE *A);        // 101
BYTE NOT_FUNC(BYTE *A);          // 110
BYTE EQU_FUNC(BYTE *A);          // 111
// vm functions sorted by number of parameters
BYTE(*func_PTR_2arg[])
(BYTE *, BYTE *) =
    {
        [MOV] = MOV_FUNC, // 000
        [REF] = REF_FUNC, // 011
        [ADD] = ADD_FUNC, // 100
};

BYTE(*func_PTR_1arg[])
(BYTE *) =
    {
        [CAL] = CAL_FUNC,     // 001
        [PRINT] = PRINT_FUNC, // 101
        [NOT] = NOT_FUNC,     // 110
        [EQU] = EQU_FUNC,     // 111
};

BYTE(*func_PTR_0arg[])
() =
    {
        [RET] = RET_FUNC, // 010
};

#endif