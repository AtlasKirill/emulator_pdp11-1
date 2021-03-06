#ifndef PDP_PROCESSOR_H
#define PDP_PROCESSOR_H


#include <string>
#include "data_types.h"
#include "pdp_tester.h"
#include "icache.h"
#include "clocks.h"
//#include "pipeline.h"


using std::string;


#define NO_PARAM    0
#define HAS_SS      1
#define HAS_DD      2
#define HAS_NN      4
#define HAS_XX      8
#define HAS_R       16
#define HAS_R6      32
#define HAS_X       64
#define HAS_S       128

#define DOUBLE_OP_MASK      0170000
#define DOUBLE_REG_OP_MASK  0177000
#define SINGLE_OP_MASK      0177700
#define SINGLE_REG_OP_MASK  0177770
#define BRANCH_MASK         0177400
#define WITHOUT_MASK        0177777
#define REG_NUMBER_MASK     0000007
#define MODE_MASK           0000007
#define OFFSET_MASK         0000377 //  Check this out!!!!!!!!!!!

#define INSTRUCTIONS_NUMBER     76
#define PARSED_COMMANDS_SIZE    64*KB


class pdp_memory;
class icache;
class pipeline;
class wb_buffer;


typedef enum addr_mode {
    REGISTER                = 0,
    REGISTER_DEFERRED       = 1,
    AUTOINCREMENT           = 2,
    AUTOINCREMENT_DEFERRED  = 3,
    AUTODECREMENT           = 4,
    AUTODECREMENT_DEFERRED  = 5,
    INDEX                   = 6,
    INDEX_DEFERRED          = 7,
    BRANCH                  = 8,
    ADDR_MODES_NUMBER       = 9,
    NO_MODE                 = 10
} addr_mode;


typedef struct disasm_mode {
    string left;
    string mid;
    string right;
} disasm_mode;


typedef struct op {
    //addr_mode   mode;
    WORD        val;    //value
    ADDR        adr;	//address;
    WORD        res;
} operand;


typedef enum operand_type {
    SS              =   0,
    DD              =   1,
    R               =   2,
    XX              =   3,
    NN              =   4,
    OPERANDS_NUMBER =   5,
    NO_OPERAND_TYPE =   6
} op_type;


/*  TODO:
 *    - check PC incrementation in 1, 4, 5 modes
 */


class pdp_processor
{
    friend class pdp_tester;

private:
    //  Processor needs to communicate with memory
    pdp_memory *memory;


    //  Array to store SS, DD, R, XX, NN
    operand operands[OPERANDS_NUMBER] = {};

    //  Slot for saving execution result
    op_type result;

    //  Result without overflow
    unsigned long overflow_free_res;

    //  Flag for byte instructions
    bool    if_byte_flag;

    //  Clock counter
    int     clock_counter;

    //  Instruction counter
    int     instruction_counter;

    //  Current instruction
    WORD    current_instr;

    //  Instruction cache
    icache  *cache;

    //  Write back buffer
    wb_buffer   *wb_buf;

    //  Pipeline
    pipeline    *pipe;


/*
 *
 *  Support array of structures for simple inscturcion decoding
 *
 */
    typedef struct com_processing { // TODO: check this out: may be addr_mode doesn't need
        int index;

        op_type     first_op_type;
        addr_mode   first_op_addr_mode;    //  command >> 3
        void        (pdp_processor::*get_first_op)(WORD, int num);

        op_type     second_op_type;
        addr_mode   second_op_addr_mode;   //  command >> 9
        void        (pdp_processor::*get_second_op)(WORD, int num);

    } com_processing;
    com_processing parsed_commands[PARSED_COMMANDS_SIZE] = {};

    bool    parse_commands();


    //  Array of functions for operands extraction
    void (pdp_processor::*get_op[ADDR_MODES_NUMBER])(WORD, int);

    bool    init_get_op_array();
    void    get_reg_op(WORD instr, int op_num);
    void    get_reg_def_op(WORD instr, int op_num);
    void    get_autoinc_op(WORD instr, int op_num);
    void    get_autoinc_def_op(WORD instr, int op_num);
    void    get_autodec_op(WORD instr, int op_num);
    void    get_autodec_def_op(WORD instr, int op_num);
    void    get_index_op(WORD instr, int op_num);
    void    get_index_def_op(WORD instr, int op_num);
    void    get_branch_op(WORD instr, int op_num);

    //  Array of operand fetch clocks
    int opfetch_clocks[ADDR_MODES_NUMBER] = {
        REGISTER_ACCESS,
        MEMORY_ACCESS,
        MEMORY_ACCESS + ONE_CLOCK,
        MEMORY_ACCESS + MEMORY_ACCESS + ONE_CLOCK,
        MEMORY_ACCESS + ONE_CLOCK,
        MEMORY_ACCESS + MEMORY_ACCESS + ONE_CLOCK,
        MEMORY_ACCESS + MEMORY_ACCESS_DATA_BUS + ONE_CLOCK,
        MEMORY_ACCESS + MEMORY_ACCESS + MEMORY_ACCESS_DATA_BUS + ONE_CLOCK,
        MEMORY_ACCESS
    };


/*
 *
 *      Instruction processing cycle
 *
 */
    bool    instruction_fetch();
    bool    instruction_decode();
    bool    op1_fetch();
    bool    op2_fetch();
    bool    execute();
    bool    write_back();


/*
 *      Flags handler
 *
 *  NOTE:   this func doesn't touch C flag
 */
    bool    set_flags(int c = -1, int v = -1, int z = -1, int n = -1);


/*
 *      Reset funcs
 */
    bool    reset_operands();
    bool    reset_if_byte_flag();
    bool    reset_result();
    bool    reset_curr_instr();


/*
 *
 *    List of pdp instructions
 *
 */
    typedef struct command {
        int     index;
        WORD    mask;
        WORD    opcode;
        const char *  name;
        // TODO: add more information about command
        void    (pdp_processor::*ex_func) (void);
        WORD    params;
        int     clocks;
    } command;

command commands_list[INSTRUCTIONS_NUMBER] = {
/*
 *  Single operand instructions
 */
//  General
{0,  SINGLE_OP_MASK,    0005000,    "CLR",      &pdp_processor::ex_clr,     HAS_DD, 5},
{1,  SINGLE_OP_MASK,    0105000,    "CLRB",     &pdp_processor::ex_clrb,    HAS_DD, 5},
{2,  SINGLE_OP_MASK,    0005100,    "COM",      &pdp_processor::ex_com,     HAS_DD, 5},
{3,  SINGLE_OP_MASK,    0105100,    "COMB",     &pdp_processor::ex_comb,    HAS_DD, 5},
{4,  SINGLE_OP_MASK,    0005200,    "INC",      &pdp_processor::ex_inc,     HAS_DD, 5},
{5,  SINGLE_OP_MASK,    0105200,    "INCB",     &pdp_processor::ex_incb,    HAS_DD, 5},
{6,  SINGLE_OP_MASK,    0005300,    "DEC",      &pdp_processor::ex_dec,     HAS_DD, 5},
{7,  SINGLE_OP_MASK,    0105300,    "DECB",     &pdp_processor::ex_decb,    HAS_DD, 5},
{8,  SINGLE_OP_MASK,    0005400,    "NEG",      &pdp_processor::ex_neg,     HAS_DD, 5},
{9,  SINGLE_OP_MASK,    0105400,    "NEGB",     &pdp_processor::ex_negb,    HAS_DD, 5},
{10, SINGLE_OP_MASK,    0005700,    "TST",      &pdp_processor::ex_tst,     HAS_DD, 5},
{11, SINGLE_OP_MASK,    0105700,    "TSTB",     &pdp_processor::ex_tstb,    HAS_DD, 5},

//  Shift and rotate
{12, SINGLE_OP_MASK,    0006200,    "ASR",      &pdp_processor::ex_asr,     HAS_DD, 5},
{13, SINGLE_OP_MASK,    0106200,    "ASRB",     &pdp_processor::ex_asrb,    HAS_DD, 5},
{14, SINGLE_OP_MASK,    0006300,    "ASL",      &pdp_processor::ex_asl,     HAS_DD, 5},
{15, SINGLE_OP_MASK,    0106300,    "ASLB",     &pdp_processor::ex_aslb,    HAS_DD, 5},
{16, SINGLE_OP_MASK,    0006000,    "ROR",      &pdp_processor::ex_ror,     HAS_DD, 5},
{17, SINGLE_OP_MASK,    0106000,    "RORB",     &pdp_processor::ex_rorb,    HAS_DD, 5},
{18, SINGLE_OP_MASK,    0006100,    "ROL",      &pdp_processor::ex_rol,     HAS_DD, 5},
{19, SINGLE_OP_MASK,    0106100,    "ROLB",     &pdp_processor::ex_rolb,    HAS_DD, 5},
{20, SINGLE_OP_MASK,    0000300,    "SWAP",     &pdp_processor::ex_swap,    HAS_DD, 5},

//  Multiple Precision
{21, SINGLE_OP_MASK,    0005500,    "ADC",      &pdp_processor::ex_adc,     HAS_DD, 5},
{22, SINGLE_OP_MASK,    0105500,    "ADCB",     &pdp_processor::ex_adcb,    HAS_DD, 5},
{23, SINGLE_OP_MASK,    0005600,    "SBC",      &pdp_processor::ex_sbc,     HAS_DD, 5},
{24, SINGLE_OP_MASK,    0105600,    "SBCB",     &pdp_processor::ex_sbcb,    HAS_DD, 5},
{25, SINGLE_OP_MASK,    0006700,    "SXT",      &pdp_processor::ex_sxt,     HAS_DD, 5},


/*
 *  Double operand instructions
 */
//  General
{26, DOUBLE_OP_MASK,    0010000,    "MOV",      &pdp_processor::ex_mov,     HAS_SS|HAS_DD, 5},
{27, DOUBLE_OP_MASK,    0110000,    "MOVB",     &pdp_processor::ex_movb,    HAS_SS|HAS_DD, 5},
{28, DOUBLE_OP_MASK,    0020000,    "CMP",      &pdp_processor::ex_cmp,     HAS_SS|HAS_DD, 5},
{29, DOUBLE_OP_MASK,    0120000,    "CMPB",     &pdp_processor::ex_cmpb,    HAS_SS|HAS_DD, 5},
{30, DOUBLE_OP_MASK,    0060000,    "ADD",      &pdp_processor::ex_add,     HAS_SS|HAS_DD, 5},
{31, DOUBLE_OP_MASK,    0160000,    "SUB",      &pdp_processor::ex_sub,     HAS_SS|HAS_DD, 5},

//  Logical
{32, DOUBLE_OP_MASK,    0030000,    "BIT",      &pdp_processor::ex_bit,     HAS_SS|HAS_DD, 5},
{33, DOUBLE_OP_MASK,    0130000,    "BITB",     &pdp_processor::ex_bitb,    HAS_SS|HAS_DD, 5},
{34, DOUBLE_OP_MASK,    0040000,    "BIC",      &pdp_processor::ex_bic,     HAS_SS|HAS_DD, 5},
{35, DOUBLE_OP_MASK,    0140000,    "BICB",     &pdp_processor::ex_bicb,    HAS_SS|HAS_DD, 5},
{36, DOUBLE_OP_MASK,    0050000,    "BIS",      &pdp_processor::ex_bis,     HAS_SS|HAS_DD, 5},
{37, DOUBLE_OP_MASK,    0150000,    "BISB",     &pdp_processor::ex_bisb,    HAS_SS|HAS_DD, 5},

//  Register
{38, DOUBLE_REG_OP_MASK,0070000,    "MUL",      &pdp_processor::ex_mul,     HAS_R6|HAS_DD, 5},
{39, DOUBLE_REG_OP_MASK,0071000,    "DIV",      &pdp_processor::ex_mul,     HAS_R6|HAS_DD, 5},
{40, DOUBLE_REG_OP_MASK,0072000,    "ASH",      &pdp_processor::ex_mul,     HAS_R6|HAS_DD, 5},
{41, DOUBLE_REG_OP_MASK,0073000,    "ASHC",     &pdp_processor::ex_mul,     HAS_R6|HAS_DD, 5},
{42, DOUBLE_REG_OP_MASK,0074000,    "XOR",      &pdp_processor::ex_mul,     HAS_R6|HAS_DD, 5},

/*
 *  Program control
 */
//  Branch
{43, BRANCH_MASK,       0000400,    "BR",       &pdp_processor::ex_br,      HAS_XX, 5},
{44, BRANCH_MASK,       0001000,    "BNE",      &pdp_processor::ex_bne,     HAS_XX, 5},
{45, BRANCH_MASK,       0001400,    "BEQ",      &pdp_processor::ex_beq,     HAS_XX, 5},
{46, BRANCH_MASK,       0100000,    "BPL",      &pdp_processor::ex_bpl,     HAS_XX, 5},
{47, BRANCH_MASK,       0100400,    "BMI",      &pdp_processor::ex_bmi,     HAS_XX, 5},
{48, BRANCH_MASK,       0102000,    "BVC",      &pdp_processor::ex_bvc,     HAS_XX, 5},
{49, BRANCH_MASK,       0102400,    "BVS",      &pdp_processor::ex_bvs,     HAS_XX, 5},
{50, BRANCH_MASK,       0103000,    "BCC",      &pdp_processor::ex_bcc,     HAS_XX, 5},
{51, BRANCH_MASK,       0103400,    "BCS",      &pdp_processor::ex_bcs,     HAS_XX, 5},

//  Signed conditional branch
{52, BRANCH_MASK,       0002000,    "BGE",      &pdp_processor::ex_bge,     HAS_XX, 5},
{53, BRANCH_MASK,       0002400,    "BLT",      &pdp_processor::ex_blt,     HAS_XX, 5},
{54, BRANCH_MASK,       0003000,    "BGT",      &pdp_processor::ex_bgt,     HAS_XX, 5},
{55, BRANCH_MASK,       0003400,    "BLE",      &pdp_processor::ex_ble,     HAS_XX, 5},

//  Unsigned conditional branch
{56, BRANCH_MASK,       0101000,    "BHI",      &pdp_processor::ex_bhi,     HAS_XX, 5},
{57, BRANCH_MASK,       0101400,    "BLOS",     &pdp_processor::ex_blos,    HAS_XX, 5},
{58, BRANCH_MASK,       0103000,    "BHIS",     &pdp_processor::ex_bhis,    HAS_XX, 5},
{59, BRANCH_MASK,       0103400,    "BLO",      &pdp_processor::ex_blo,     HAS_XX, 5},

//  Jump & Subroutine
{60, SINGLE_OP_MASK,    0000100,    "JMP",      &pdp_processor::ex_jmp,     HAS_DD, 5},
{61, DOUBLE_REG_OP_MASK,0004000,    "JSR",      &pdp_processor::ex_jsr,     HAS_R6|HAS_DD, 5},
{62, SINGLE_REG_OP_MASK,0000200,    "RTS",      &pdp_processor::ex_rts,     HAS_R, 5},//Comm parsing defined this as DD not R
{63, SINGLE_OP_MASK,    0006400,    "MARK",     &pdp_processor::ex_mark,    HAS_NN, 5},// Analogy with above
{64, DOUBLE_REG_OP_MASK,0077000,    "SOB",      &pdp_processor::ex_sob,     HAS_R6|HAS_NN, 5},

//  Trap & Interrupt
{65, WITHOUT_MASK,      0104000,    "EMP",      &pdp_processor::ex_emt,     NO_PARAM, 5},//TODO: EMT
{66, WITHOUT_MASK,      0104000,    "TRAP",     &pdp_processor::ex_trap,    NO_PARAM, 5},//TODO: TRAP
{67, WITHOUT_MASK,      0000003,    "BPT",      &pdp_processor::ex_bpt,     NO_PARAM, 5},
{68, WITHOUT_MASK,      0000004,    "IOT",      &pdp_processor::ex_iot,     NO_PARAM, 5},
{69, WITHOUT_MASK,      0000002,    "RTI",      &pdp_processor::ex_rti,     NO_PARAM, 5},
{70, WITHOUT_MASK,      0000006,    "RTT",      &pdp_processor::ex_rtt,     NO_PARAM, 5},

//  Miscellaneous
{71, WITHOUT_MASK,      0000000,    "HALT",     &pdp_processor::ex_halt,    NO_PARAM, 5},
{72, WITHOUT_MASK,      0000001,    "WAIT",     &pdp_processor::ex_wait,    NO_PARAM, 5},
{73, WITHOUT_MASK,      0000005,    "RESET",    &pdp_processor::ex_reset,   NO_PARAM, 5},
{74, SINGLE_OP_MASK,    0006500,    "MFPI",     &pdp_processor::ex_mfpi,    HAS_DD, 5},
{75, SINGLE_OP_MASK,    0006600,    "MTPI",     &pdp_processor::ex_mtpi,    HAS_DD, 5},
};

    disasm_mode mode_list[ADDR_MODES_NUMBER] = {
        {"",  "",    ""},
        {"",  "(",   ")"},
        {"",  "(",   ")+"},
        {"",  "@(",  ")+"},
        {"",  "-(",  ")"},
        {"",  "-@(", ")"},
        {"",  "(",   ")"},
        {"@", "(",   ")"},
        {"",  "",    ""},
    };


    void     ex_clr();
    void     ex_clrb();
    void     ex_com();
    void     ex_comb();
    void     ex_inc();
    void     ex_incb();
    void     ex_dec();
    void     ex_decb();
    void     ex_neg();
    void     ex_negb();
    void     ex_tst();
    void     ex_tstb();

    void     ex_asr();
    void     ex_asrb();
    void     ex_asl();
    void     ex_aslb();
    void     ex_ror();
    void     ex_rorb();
    void     ex_rol();
    void     ex_rolb();
    void     ex_swap();

    void     ex_adc();
    void     ex_adcb();
    void     ex_sbc();
    void     ex_sbcb();
    void     ex_sxt();

    void     ex_mov();
    void     ex_movb();
    void     ex_cmp();
    void     ex_cmpb();
    void     ex_add();
    void     ex_sub();

    void     ex_bit();
    void     ex_bitb();
    void     ex_bic();
    void     ex_bicb();
    void     ex_bis();
    void     ex_bisb();

    void     ex_mul();
    void     ex_div();
    void     ex_ash();
    void     ex_ashc();
    void     ex_xor();

    void     ex_br();
    void     ex_bne();
    void     ex_beq();
    void     ex_bpl();
    void     ex_bmi();
    void     ex_bvc();
    void     ex_bvs();
    void     ex_bcc();
    void     ex_bcs();

    void     ex_bge();
    void     ex_blt();
    void     ex_bgt();
    void     ex_ble();

    void     ex_bhi();
    void     ex_blos();
    void     ex_bhis();
    void     ex_blo();

    void     ex_jmp();
    void     ex_jsr();
    void     ex_rts();
    void     ex_mark();
    void     ex_sob();

    void     ex_emt();
    void     ex_trap();
    void     ex_bpt();
    void     ex_iot();
    void     ex_rti();
    void     ex_rtt();

    void     ex_halt();
    void     ex_wait();
    void     ex_reset();
    void     ex_mfpi();
    void     ex_mtpi();


public:

    pdp_processor(pdp_memory *mem, pipeline *p);
    bool        process_instruction();
    string      disasm_curr_instr();
    bool        reset();
    ic_stat_t   get_icstat();

    //  Just for testing
    command get_command(int index);
    com_processing get_parsed_command(int index);
};

#endif // PDP_PROCESSOR_H
