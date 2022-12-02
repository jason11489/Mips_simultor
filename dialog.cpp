#include "dialog.h"
#include <bitset>
#include <string.h>
#include <sstream>
#include <fstream>
#include "./ui_dialog.h"
#include <fstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>

QDir dir;
QFileInfoList list_tmp;

using namespace std;
typedef int int32;

//========================================
//==========================
//
//  main.cpp
//  MIPS Assembler
//
//  Created by Young Seok Kim
//  Copyright © 2015 TonyKim. All rights reserved.
//
// data structures
struct label {
    char name[100];
    int32_t location;
};

// Global variables
label label_list[100];
int labelindex = 0;

int32_t instructions[1000];
int instr_index = 0;

int32_t datasection[1000] = {0,};
int datasectionindex = 0;

// functions
void scanLabels(char *filename);
int countDataSection(char *filename);

void assembleLine(char *line);

void makeR_type(int op, int rs, int rt, int rd, int shamt, int funct);
void makeI_type(int op, int rs, int rt, int addr);
void makeJ_type(int op, int addr);

int regToInt(char *reg);
int labelToIntAddr(char *label);
int immToInt(char *immediate);
char *int2bin(int a, char *buffer, int buf_size);

//==============================================================

struct split_Instructions{

    char type;

    int32 opcode;
    int32 rs;
    int32 rt;
    int32 rd;
    int32 shamt;
    int32 funct;
    int32 immediate;
    int32 address;

};

int Register[32] = {0,5,0x10000000,5,4,0,0,0,123,123,123,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x10008000,0x7ffffe40,0,0};
split_Instructions ProgramCounter[100] = {0,};
int32 PregramCounter_size = 0;
int32 PC = 0x00400024;



void ins_print(split_Instructions x){

    qDebug() << "type = " << x.type << "\n";
    qDebug() << "opcode = " << hex <<x.opcode << "\n";
    qDebug() << "rs = " << x.rs << "\n";
    qDebug() << "rt = " << x.rt << "\n";
    qDebug() << "rd = " << x.rd << "\n";
    qDebug() << "shamt = " <<hex << x.shamt << "\n";
    qDebug() << "funct = " << hex <<x.funct << "\n";
    qDebug() << "immediate = " << hex << x.immediate << "\n";
    qDebug() << "address = " << hex << x.address << "\n";

}

struct IF_ID{

    int32 PC_if_id;
    split_Instructions IF_ID_instructions;

};

struct ID_EX{

//EX
    bool regDst;
    bool ALUop1;
    bool ALUop2;
    bool ALUSrc;

//MEM
    bool Branch;
    bool MemRead;
    bool MemWrite;

//WB
    bool RegWrite;
    bool MemtoReg;


//=========================


    // int32 PC_id_ex;

    int32 Read_data_1;
    int32 Read_data_2;

    int32 S_Ex; //flag
    int32 rt;
    int32 rd;
    int32 rs;

    int32 next_ALU_result;
    bool next_zero;

    int32 IF_FLUSH;
};

struct EX_MEM{

//MEM
    bool Branch;
    bool MemRead;
    bool MemWrite;

//WB
    bool RegWrite;
    bool MemtoReg;

//===============================

    int32 ALU_result;

    int32 Read_data_2;

    int32 mux_result;

};

struct MEM_WB{

    //WB
    bool RegWrite;
    bool MemtoReg;

//============================

    int32 Read_data;

    int32 ALU_result;

    int32 mux_result;

};

IF_ID If_id;
ID_EX id_ex;
EX_MEM ex_mem;
MEM_WB mem_wb;

int32 ALUOP_function(int32 ALUOP1, int32 ALUOP2, int32 functfield){

    if(ALUOP1 == 0 && ALUOP2 == 0){
        return 2;
    }else if(ALUOP2 == 1){
        return 6;
    }else if(ALUOP1 == 1 && functfield == 0){
        return 2;
    }else if(ALUOP1 == 1 && functfield == 2){
        return 6;
    }else if(ALUOP1 == 1 && functfield == 4){
        return 0;
    }else if(ALUOP1 == 1 && functfield == 5){
        return 1;
    }else if(ALUOP1 == 1 && functfield == 10){
        return 7;
    }
    return 0;

}
int32 ALU(int32 ALUOP,int32 A, int32 B){

    int32 result = 0;
    if(ALUOP == 0){
        result =  A & B;
        return result;
    }else if(ALUOP == 1){
        result =  A | B;
        return result;
    }else if(ALUOP == 2){
        result =  A + B;
        return result;
    }else if(ALUOP == 6){
        result =  A - B;
        return result;
    }else if(ALUOP == 7){
        result =  A < B;
        return result;
    }
    return 0;
}
int32 ForwardA_func(){

    if(ex_mem.RegWrite && ex_mem.mux_result == id_ex.rs){
        return 2;
    }else if(ex_mem.mux_result != id_ex.rt && mem_wb.RegWrite && mem_wb.mux_result == id_ex.rs){
        return 1;
    }else if(!ex_mem.RegWrite && mem_wb.RegWrite && mem_wb.mux_result == id_ex.rs){
        return 1;
    }else{
        return 0;
    }

}
int32 ForwardB_func(){
    if(ex_mem.RegWrite && ex_mem.mux_result == id_ex.rt){
        return 2;
    }else if(ex_mem.mux_result != id_ex.rt && mem_wb.RegWrite && mem_wb.mux_result == id_ex.rt){
        return 1;
    }else if(!ex_mem.RegWrite && mem_wb.RegWrite && mem_wb.mux_result == id_ex.rt){
        return 1;
    }else{
        return 0;
    }
}

void IF_ID_print(){
    qDebug() << "=================================================\n";
    qDebug() << "IF_ID_print_check\n";
    printf("PC = 0x%x\n",If_id.PC_if_id);
    ins_print(If_id.IF_ID_instructions);
    qDebug() << "\n\n\n";
}

void ID_EX_print(){
    qDebug() << "=================================================\n";

    qDebug() << "ID_EX_print_check\n";


    qDebug() << "regDst = " << id_ex.regDst << "\n";
    qDebug() << "ALUop1 = " << id_ex.ALUop1 << "\n";
    qDebug() << "ALUop2 = " << id_ex.ALUop2 << "\n";
    qDebug() << "ALUSrc = " << id_ex.ALUSrc << "\n";

    qDebug() << "Branch = " << id_ex.Branch << "\n";
    qDebug() << "MemRead = " << id_ex.MemRead << "\n";
    qDebug() << "MemWrite = " << id_ex.MemWrite << "\n";

    qDebug() << "RegWrite = " << id_ex.RegWrite << "\n";
    qDebug() << "MemtoReg = " << id_ex.MemtoReg << "\n";

    printf("Read_data_1 = 0x%x\n",id_ex.Read_data_1);
    printf("Read_data_2 = 0x%x\n",id_ex.Read_data_2);

    printf("S_Ex = 0x%x\n",id_ex.S_Ex);
    printf("rt = 0x%x\n",id_ex.rt);
    printf("rd = 0x%x\n",id_ex.rd);
    qDebug() << "\n\n\n";

}

void EX_MEM_print(){
    qDebug() << "=================================================\n";

    qDebug() << "EX_MEM_print_check\n";

    qDebug() << "Branch = " << ex_mem.Branch << "\n";
    qDebug() << "MemRead = " << ex_mem.MemRead << "\n";
    qDebug() << "MemWrite = " << ex_mem.MemWrite << "\n";

    qDebug() << "RegWrite = " << ex_mem.RegWrite << "\n";
    qDebug() << "MemtoReg = " << ex_mem.MemtoReg << "\n";

    printf("ALU_result = 0x%x\n",ex_mem.ALU_result);
    printf("Read_data_2 = 0x%x\n",ex_mem.Read_data_2);
    printf("mux_result = 0x%x\n",ex_mem.mux_result);
    qDebug() << "\n\n\n";

}

void MEM_WB_print(){
    qDebug() << "=================================================\n";

    qDebug() << "MEM_WB_print_check\n";

    qDebug() << "RegWrite = " << mem_wb.RegWrite << "\n";
    qDebug() << "MemtoReg = " << mem_wb.MemtoReg << "\n";

    printf("Read_data = 0x%x\n",mem_wb.Read_data);
    printf("ALU_result = 0x%x\n",mem_wb.ALU_result);
    printf("mux_result = 0x%x\n",mem_wb.mux_result);
    qDebug() << "\n\n\n";

}

void IF(){

    split_Instructions ins;

    if(id_ex.IF_FLUSH || If_id.IF_ID_instructions.type == 'J'){
        If_id.IF_ID_instructions.type = 'O';
        If_id.IF_ID_instructions.opcode = 0;;
        If_id.IF_ID_instructions.rs = 0;
        If_id.IF_ID_instructions.rt = 0;
        If_id.IF_ID_instructions.rd = 0;
        If_id.IF_ID_instructions.shamt=0;
        If_id.IF_ID_instructions.funct=0;
        If_id.IF_ID_instructions.immediate=0;
        If_id.IF_ID_instructions.address=0;

    }else{
        // PC + 4
        //mux -> 0
        ins = ProgramCounter[ (PC - 0x00400024) / 4 ];
        PC = PC + 4;

        If_id.PC_if_id = PC;
        If_id.IF_ID_instructions = ins;
    }


}

void ID(){
    //Registers
    //WB part

    id_ex.Read_data_1 = Register[If_id.IF_ID_instructions.rs];
    id_ex.Read_data_2 = Register[If_id.IF_ID_instructions.rt];


    //Control

    if(If_id.IF_ID_instructions.type == 'R'){ // R-format

        id_ex.regDst = true;
        id_ex.ALUSrc = false;
        id_ex.MemtoReg = false;
        id_ex.RegWrite = true;
        id_ex.MemRead = false;
        id_ex.MemWrite = false;
        id_ex.Branch = false;
        id_ex.ALUop1 = true;
        id_ex.ALUop2 = false;

    }else if(If_id.IF_ID_instructions.type == 'I' && If_id.IF_ID_instructions.opcode == 0x23 ){// lw

        id_ex.regDst = false;
        id_ex.ALUSrc = true;
        id_ex.MemtoReg = true;
        id_ex.RegWrite = true;
        id_ex.MemRead = true;
        id_ex.MemWrite = false;
        id_ex.Branch = false;
        id_ex.ALUop1 = false;
        id_ex.ALUop2 = false;

    }else if(If_id.IF_ID_instructions.type == 'I' && If_id.IF_ID_instructions.opcode == 0x2b){// sw

        id_ex.regDst = true;
        id_ex.ALUSrc = true;
        id_ex.MemtoReg = false;
        id_ex.RegWrite = false;
        id_ex.MemRead = false;
        id_ex.MemWrite = true;
        id_ex.Branch = false;
        id_ex.ALUop1 = false;
        id_ex.ALUop2 = false;

    }else if(If_id.IF_ID_instructions.type == 'I' && If_id.IF_ID_instructions.opcode == 0x4){// beq

        id_ex.regDst = true;
        id_ex.ALUSrc = false;
        id_ex.MemtoReg = false;
        id_ex.RegWrite = false;
        id_ex.MemRead = false;
        id_ex.MemWrite = false;
        id_ex.Branch = true;
        id_ex.ALUop1 = false;
        id_ex.ALUop2 = true;

    }else if(If_id.IF_ID_instructions.type == 'J' && If_id.IF_ID_instructions.opcode == 0x2){// J


        id_ex.regDst = false;
        id_ex.ALUSrc = false;
        id_ex.MemtoReg = false;
        id_ex.RegWrite = false;
        id_ex.MemRead = false;
        id_ex.MemWrite = false;
        id_ex.Branch = false;
        id_ex.ALUop1 = false;
        id_ex.ALUop2 = false;

    }else if(If_id.IF_ID_instructions.type == 'O'){

        id_ex.regDst = false;
        id_ex.ALUSrc = false;
        id_ex.MemtoReg = false;
        id_ex.RegWrite = false;
        id_ex.MemRead = false;
        id_ex.MemWrite = false;
        id_ex.Branch = false;
        id_ex.ALUop1 = false;
        id_ex.ALUop2 = false;
    }

    // Branch
    int32 Branch_PC=0;

    //sign-extend
    int32 tmp = If_id.IF_ID_instructions.immediate;
    if((tmp >> 15) == 1){
        id_ex.S_Ex = tmp | 0xffff0000;
    }else{
        id_ex.S_Ex = tmp;
    }

    int32 shift_left_2 = 0;
    shift_left_2 = id_ex.S_Ex << 2;

    Branch_PC = shift_left_2 + If_id.PC_if_id;

    //======
    int32 zero;
    if(id_ex.Read_data_1 == id_ex.Read_data_2){
        zero = 1;
    }else{
        zero = 0;
    }

    if(zero && id_ex.Branch){
        id_ex.IF_FLUSH = 1;
        PC = Branch_PC;
    }else{
        id_ex.IF_FLUSH = 0;
    }

    //

    if(If_id.IF_ID_instructions.type == 'J'){
        PC = If_id.IF_ID_instructions.address << 2;
    }else{
        //check
        id_ex.rt = If_id.IF_ID_instructions.rt;
        id_ex.rd = If_id.IF_ID_instructions.rd;
        id_ex.rs = If_id.IF_ID_instructions.rs;
    }



}

void EX(){

    // Hazard detection

    if(id_ex.MemRead && id_ex.rt == If_id.IF_ID_instructions.rs){

        If_id.IF_ID_instructions.type = 'O';

        If_id.IF_ID_instructions.opcode = 0;
        If_id.IF_ID_instructions.rs=0;
        If_id.IF_ID_instructions.rt=0;
        If_id.IF_ID_instructions.rd=0;
        If_id.IF_ID_instructions.shamt=0;
        If_id.IF_ID_instructions.funct=0;
        If_id.IF_ID_instructions.immediate=0;
        If_id.IF_ID_instructions.address=0;

        PC = If_id.PC_if_id - 4;

    }else if(id_ex.MemRead && id_ex.rt == If_id.IF_ID_instructions.rs){

        If_id.IF_ID_instructions.type = 'O';

        If_id.IF_ID_instructions.opcode = 0;
        If_id.IF_ID_instructions.rs=0;
        If_id.IF_ID_instructions.rt=0;
        If_id.IF_ID_instructions.rd=0;
        If_id.IF_ID_instructions.shamt=0;
        If_id.IF_ID_instructions.funct=0;
        If_id.IF_ID_instructions.immediate=0;
        If_id.IF_ID_instructions.address=0;

        PC = If_id.PC_if_id - 4;

    }


    // Control check
    /** MEM*/
    ex_mem.Branch = id_ex.Branch;
    ex_mem.MemRead = id_ex.MemRead;
    ex_mem.MemWrite = id_ex.MemWrite;
    /** WB*/
    ex_mem.RegWrite = id_ex.RegWrite;
    ex_mem.MemtoReg = id_ex.MemtoReg;

    //======

    ex_mem.Read_data_2 = id_ex.Read_data_2;

    if(id_ex.regDst){
        ex_mem.mux_result = id_ex.rd;
    }else{
        ex_mem.mux_result = id_ex.rt;
    }

    ex_mem.ALU_result = id_ex.next_ALU_result;

}

void MEM(){

    // Control check

    mem_wb.RegWrite = ex_mem.RegWrite;
    mem_wb.MemtoReg = ex_mem.MemtoReg;


    // Data memeory

    if(ex_mem.MemWrite){ // sw

        datasection[(ex_mem.ALU_result - 0x10000000) / 4] = ex_mem.Read_data_2;

    }
    if(ex_mem.MemRead){  // lw

        mem_wb.Read_data = datasection[(ex_mem.ALU_result - 0x10000000) / 4];

    }

    // Check

    mem_wb.ALU_result = ex_mem.ALU_result;
    mem_wb.mux_result = ex_mem.mux_result;

}

void WB() {

    int32 mux_result=0;

    if(mem_wb.RegWrite){

        if(mem_wb.MemtoReg){
            Register[mem_wb.mux_result] = mem_wb.Read_data;
            mux_result = mem_wb.Read_data;
        }else{
            Register[mem_wb.mux_result] = mem_wb.ALU_result;
            mux_result = mem_wb.ALU_result;
        }
    }

    // Forwarding Unit

    int32 ForwardA = 0, ForwardB = 0;
    ForwardA = ForwardA_func();
    ForwardB = ForwardB_func();


    //==================

    int32 alu_input_1=0,alu_input_2 = 0;

    if(ForwardA == 2){
        alu_input_1 = ex_mem.ALU_result;
    }else if(ForwardA == 1){
        alu_input_1 = mux_result;
    }else{
        alu_input_1 = id_ex.Read_data_1;
    }

    if(id_ex.ALUSrc){
        //Forward
        alu_input_2 = id_ex.S_Ex;
    }else{

        if(ForwardB == 2){
            alu_input_2 = ex_mem.ALU_result;
        }else if(ForwardB == 1){
            alu_input_2 = mux_result;
        }else{
            alu_input_2 = id_ex.Read_data_2;
        }
    }

    // ALU control
    int32 ALUOP;
    int32 funct_field = id_ex.S_Ex & 0xf;
    ALUOP = ALUOP_function(id_ex.ALUop1,id_ex.ALUop2,funct_field);

    // ALU
    id_ex.next_ALU_result = ALU(ALUOP,alu_input_1,alu_input_2);

    if(alu_input_1 == alu_input_2){
        id_ex.next_zero = true;
    }else{
        id_ex.next_zero = false;
    }


}

bool input_ins(const char* argv[]){


//=======================open source======================

    if (argv[1] == NULL) {
        printf("Please type in input file name.\n");
        return 0;
    }

    char filename[20];
    strcpy(filename, argv[1]);

    //strcpy(filename, "example_mod.s");
    //strcpy(filename, "example2_mod.s");
    //strcpy(filename, "example3.s");
    //strcpy(filename, "example4.s");

    int dataSectionSize = countDataSection(filename);
    scanLabels(filename);

    string line;
    ifstream inputfile(filename);
    if (inputfile.is_open())
    {
        while (getline(inputfile, line) )
        {
            if(strstr(line.c_str(),".text") != NULL){
                break;
            }
        }
        while (getline(inputfile, line) )
        {
            if (strstr(line.c_str(), ":") != NULL) {
                continue;
            } else {
                char codeline[100];
                strcpy(codeline, line.c_str());
                assembleLine(codeline);
            }
        }
        inputfile.close();
    }
    else qDebug() << "Unable to open file\n";



//=======================open source======================


    for(int i = 0;i < instr_index ; i++){
        split_Instructions tmp;
        tmp.opcode = (instructions[i] >> 26) & 0x3f;
        tmp.rs = (instructions[i] >> 21) & 0x1f;
        tmp.rt = (instructions[i] >> 16) & 0x1f;
        tmp.rd = (instructions[i] >> 11) & 0x1f;
        tmp.shamt = (instructions[i] >> 6) & 0x1f;
        tmp.funct = instructions[i] & 0x3f;
        tmp.immediate = instructions[i] & 0xffff;
        tmp.address = instructions[i] & 0x3ffffff;
        if(tmp.opcode == 0){
            tmp.type = 'R';
        }
        else if(tmp.opcode == 0x23 || tmp.opcode == 0x2b ||tmp.opcode == 0x4) //lw or sw or beq
        {
            tmp.type = 'I';
        }
        else if (tmp.opcode == 0x2)
        {
            tmp.type = 'J';
        }

        ProgramCounter[i] = tmp;
        // ins_print(tmp);
    }

    return 0;

}

void Register_print()
{
    qDebug() << "R0   [r0] = " << hex << Register[0] << "\n";
    qDebug() << "R1   [at] = " << hex << Register[1] << "\n";
    qDebug() << "R2   [v0] = " << hex << Register[2] << "\n";
    qDebug() << "R3   [v1] = " << hex << Register[3] << "\n";

    qDebug() << "R4   [a0] = " << hex << Register[4] << "\n";
    qDebug() << "R5   [a1] = " << hex << Register[5] << "\n";
    qDebug() << "R6   [a2] = " << hex << Register[6] << "\n";
    qDebug() << "R7   [a3] = " << hex << Register[7] << "\n";

    qDebug() << "R8   [t0] = " << hex << Register[8] << "\n";
    qDebug() << "R9   [t1] = " << hex << Register[9] << "\n";
    qDebug() << "R10  [t2] = " << hex << Register[10] << "\n";
    qDebug() << "R11  [t3] = " << hex << Register[11] << "\n";

    qDebug() << "R12  [t4] = " << hex << Register[12] << "\n";
    qDebug() << "R13  [t5] = " << hex << Register[13] << "\n";
    qDebug() << "R14  [t6] = " << hex << Register[14] << "\n";
    qDebug() << "R15  [t7] = " << hex << Register[15] << "\n";

    qDebug() << "R16  [s0] = " << hex << Register[16] << "\n";
    qDebug() << "R17  [s1] = " << hex << Register[17] << "\n";
    qDebug() << "R18  [s2] = " << hex << Register[18] << "\n";
    qDebug() << "R19  [s3] = " << hex << Register[19] << "\n";

    qDebug() << "R20  [s4] = " << hex << Register[20] << "\n";
    qDebug() << "R21  [s5] = " << hex << Register[21] << "\n";
    qDebug() << "R22  [s6] = " << hex << Register[22] << "\n";
    qDebug() << "R23  [s7] = " << hex << Register[23] << "\n";

    qDebug() << "R24  [t8] = " << hex << Register[24] << "\n";
    qDebug() << "R25  [t9] = " << hex << Register[25] << "\n";
    qDebug() << "R26  [k0] = " << hex << Register[26] << "\n";
    qDebug() << "R27  [k1] = " << hex << Register[27] << "\n";

    qDebug() << "R28  [gp] = " << hex << Register[28] << "\n";
    qDebug() << "R29  [sp] = " << hex << Register[29] << "\n";
    qDebug() << "R30  [s8] = " << hex << Register[30] << "\n";
    qDebug() << "R31  [ra] = " << hex << Register[31] << "\n";
}



//============================assembler open source code================
















void scanLabels(char *filename) {
    int datasizecnt = 0;
    int instruction_count = 0;

    string line;
    ifstream inputfile(filename);
    if (inputfile.is_open())
    {
        // finds the .data section and count the number of .word before .text section appears
        while (getline(inputfile, line))
        {
            if (strstr(line.c_str(),".data") != NULL){
                break;
            }
        }
        while (getline(inputfile, line)) {
            // Labels inside .data section.
            if (strstr(line.c_str(),".word") != NULL){
                label newLabel;
                char temp[100];
                strcpy(temp, line.c_str());
                strcpy(newLabel.name , strtok(temp,":"));
                newLabel.location = 0x10000000+(datasizecnt)*4;
                datasizecnt++;
                label_list[labelindex++]=newLabel;
            }
            if (strstr(line.c_str(),".text") != NULL){
                break;
            }
        }
        // Labels inside .text section (function labels)
        while (getline(inputfile, line)) {
            if (strstr(line.c_str(), ":") != NULL) {
                label newLabel;
                char temp[100];
                strcpy(temp, line.c_str());
                strcpy(newLabel.name , strtok(temp,":"));
                newLabel.location = 0x400000 + instruction_count*4;
                label_list[labelindex++]=newLabel;
            } else {
                char temp[100];
                strcpy(temp, line.c_str());

                char * one;
                char * two;
                char * three = NULL;
                char key[] = " ,\t";
                one = strtok(temp, key);
                if(one != NULL)
                    two = strtok(NULL,key);
                if(two != NULL)
                    three = strtok(NULL,key);

                if (strcmp(one, "la") == 0){
                    int location = labelToIntAddr(three);
                    if ((location & 65535) == 0) {          // if lower 16bit address is 0x0000
                        instruction_count++;                // lui instruction
                    } else {
                        instruction_count += 2;             // lui instruction + ori instruction
                    }
                } else {
                    instruction_count++;
                }
            }
        }


        inputfile.close();
    }
    else qDebug() << "Unable to open file\n";
}
int countDataSection(char *filename) {
    int count = 0;

    string line;
    ifstream inputfile(filename);
    if (inputfile.is_open())
    {
        // finds the .data section and count the number of .word before .text section appears
        while (getline(inputfile, line))
        {
            if(strstr(line.c_str(),".data") != NULL){
                break;
            }
        }
        while (getline(inputfile, line)) {
            if(strstr(line.c_str(),".text") != NULL){
                break;
            }
            if(strstr(line.c_str(),".word") != NULL){
                count++;
                char * one;
                char * two;
                char * three;
                char temp[100];
                strcpy(temp, line.c_str());

                if (strstr(temp,":") != NULL) {
                    one = strtok(temp,":");
                    two = strtok(NULL, " \t");
                    three = strtok(NULL, " \t");
                    datasection[datasectionindex++] = immToInt(three);
                } else {
                    one = strtok(temp, " \t");
                    two = strtok(NULL, " \t");
                    datasection[datasectionindex++] = immToInt(two);
                }

            }
        }
        inputfile.close();
    }
    else qDebug() << "Unable to open file\n";
    return count;
}
/* Assembling Function */
void assembleLine(char *line){

    char key[] = " ,\t";
    char * one;
    char * two;
    char * three;
    char * four = NULL;

    one = strtok(line,key);

    if(one != NULL)
        two = strtok(NULL,key);
    if(two != NULL)
        three = strtok(NULL,key);
    if(three != NULL)
        four = strtok(NULL,key);


    if (strcmp(one, "addiu") == 0) {
        // I-type
        // addiu $t,$s,C
        // $t = $s + C (unsigned)
        // R[rt] = R[rs] + SignExtImm
        makeI_type(9, regToInt(three), regToInt(two), immToInt(four));

    }else if(strcmp(one, "add") == 0){
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 32);
    }else if(strcmp(one, "sub") == 0){
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 34);
    }
     else if (strcmp(one, "addu") == 0) {
        // R-type
        // addu $d,$s,$t
        // $d = $s + $t
        // R[rd] = R[rs] + R[rt]
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 33);

    } else if (strcmp(one, "andi") == 0) {
        // I-type
        // andi $t,$s,C
        // $t = $s & C
        // R[rt] = R[rs] + SignExtImm
        makeI_type(12, regToInt(three), regToInt(two), immToInt(four));

    } else if (strcmp(one, "and") == 0) {
        // R-type
        // and $d,$s,$t
        // $d = $s & $t
        // R[rd] = R[rs] & R[rt]
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 36);

    } else if (strcmp(one, "beq") == 0) {
        // I-type
        // beq $s,$t,C
        // if ($s == $t) go to PC+4+4*C
        // if(R[rs]==R[rt]) PC=PC+4+BranchAddr
        int reladdr = labelToIntAddr(four) - (0x400000 + ((instr_index)*4)+4);
        makeI_type(4, regToInt(two), regToInt(three), (reladdr/4));          // relative address

    } else if (strcmp(one, "bne") == 0) {
        // I-type
        // bne $s,$t,C
        // if ($s != $t) go to PC+4+4*C
        // if(R[rs]!=R[rt]) PC=PC+4+BranchAddr
        int reladdr = labelToIntAddr(four) - (0x400000 + ((instr_index)*4)+4);
        makeI_type(5, regToInt(two), regToInt(three), (reladdr/4));          // relative address

    } else if (strcmp(one, "jal") == 0) {
        // J-type
        // jal C
        // $31 = PC + 4; PC = PC+4[31:28] . C*4
        // R[31]=PC+8;PC=Addr
        makeJ_type(3, (labelToIntAddr(two)>>2));                             // absolute address

    } else if (strcmp(one, "jr") == 0) {
        // R-type
        // jr $s
        // goto address $s
        // PC=R[rs]
        makeR_type(0, regToInt(two), 0, 0, 0, 8);

    } else if (strcmp(one, "j") == 0) {
        // J-type
        // j C
        // PC = PC+4[31:28] . C*4
        // PC=Addr
        makeJ_type(2, (labelToIntAddr(two)>>2));                             // absolute address
    }
    else if (strcmp(one, "lui") == 0) {
        // I-type
        // lui $t,C
        // $t = C << 16
        // R[rt] = {imm, 16’b0}
        makeI_type(15, 0, regToInt(two), immToInt(three));

    } else if (strcmp(one, "lw") == 0) {
        // I-type
        // lw $t,C($s)
        // $t = Memory[$s + C]
        // R[rt] = M[R[rs]+SignExtImm]
        char * offset = strtok(three,"()");
        char * rs = strtok(NULL,"()");
        makeI_type(35, regToInt(rs), regToInt(two), immToInt(offset));

    } else if (strcmp(one, "la") == 0) { // special
        // If the lower 16bit address is 0x0000, the ori instruction is useless.
        /*
         Case1) load address is 0x1000 0000
         lui $2, 0x1000

         Case2) load address is 0x1000 0004
         lui $2, 0x1000
         ori $2, $2, 0x0004
        */
        int location = labelToIntAddr(three);                                   // absolute label address
        if ((location & 65535) == 0) { // if lower 16bit address is 0x0000
            makeI_type(15, 0, regToInt(two), (location>>16));                   // lui instruction
        } else {
            makeI_type(15, 0, regToInt(two), (location>>16));                   // lui instruction
            makeI_type(13, regToInt(two), regToInt(two), (location & 65535));   // ori instruction
        }

    } else if (strcmp(one, "nor") == 0) {
        // R-type
        // nor $d,$s,$t
        // $d = ~ ($s | $t)
        // R[rd] = ~ (R[rs] | R[rt])
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 39);

    } else if (strcmp(one, "ori") == 0) {
        // I-type
        // ori $t,$s,C
        // $t = $s | C
        // R[rt] = R[rs] | ZeroExtImm
        makeI_type(13, regToInt(three), regToInt(two), immToInt(four));

    } else if (strcmp(one, "or") == 0) {
        // R-type
        // or $d,$s,$t
        // $d = $s | $t
        // R[rd] = R[rs] | R[rt]
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 37);

    } else if (strcmp(one, "sltiu") == 0) {
        // I-type
        // sltiu $t,$s,C
        // R[rt] = (R[rs] < SignExtImm) ? 1 :0
        makeI_type(11, regToInt(three), regToInt(two), immToInt(four));

    }else if(strcmp(one, "slt") == 0){
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two),0, 42);

    }
     else if (strcmp(one, "sltu") == 0) {
        // R-type
        // sltu $d,$s,$t
        // R[rd] = (R[rs] < R[rt]) ? 1 : 0
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 43);

    } else if (strcmp(one, "sll") == 0) {
        // R-type
        // sll $d,$t,shamt
        // $d = $t << shamt
        // R[rd] = R[rt] << shamt
        makeR_type(0, 0, regToInt(three), regToInt(two), immToInt(four), 0);

    } else if (strcmp(one, "srl") == 0) {
        // R-type
        // srl $d,$t,shamt
        // $d = $t >> shamt
        // R[rd] = R[rt] >> shamt
        makeR_type(0, 0, regToInt(three), regToInt(two), immToInt(four), 2);

    } else if (strcmp(one, "sw") == 0) {
        // I-type
        // sw $t,C($s)
        // Memory[$s + C] = $t
        // M[R[rs]+SignExtImm] = R[rt]
        char * offset = strtok(three,"()");
        char * rs = strtok(NULL,"()");
        makeI_type(43, regToInt(rs), regToInt(two), immToInt(offset));

    } else if (strcmp(one, "subu") == 0) {
        // R-type
        // subu $d,$s,$t
        // $d = $s - $t
        // R[rd] = R[rs] - R[rt]
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 35);
    }
}
// MIPS Fields
/*

 < R-type >
  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |    op(6bits)    |   rs(5bits)  |   rt(5bits)  |   rd(5bits)  | shamt(5bits) |   fucnt(6bits)  |
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+


 < I-type >
  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |    op(6bits)    |   rs(5bits)  |   rt(5bits)  |         constant or address (16bits)          |
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+


 < J-type >
  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |    op(6bits)    |                            address (30bits)                                 |
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

*/
void makeR_type(int op, int rs, int rt, int rd, int shamt, int funct) {
    int32_t ans = 0;
    ans = (op << 26);
    ans |= (rs << 21);
    ans |= (rt << 16);
    ans |= (rd << 11);
    ans |= (shamt << 6);
    ans |= funct;
    instructions[instr_index++] = ans;
}
void makeI_type(int op, int rs, int rt, int addr) {
    int32_t ans = 0;
    ans = (op << 26);
    ans |= (rs << 21);
    ans |= (rt << 16);
    ans |= (addr & 65535);
    instructions[instr_index++] = ans;
}
void makeJ_type(int op, int addr) {
    int32_t ans = 0;
    ans = (op << 26);
    ans |= addr;
    instructions[instr_index++] = ans;
}
// Conversion functions
int regToInt(char *reg){
    //$ 빼고 int로 바꾸기.
            /** v */
    if(!strcmp(reg,"$v0")){return 2;}        else if (!strcmp(reg,"$v1")){return 3;}
    /** a */
    else if (!strcmp(reg,"$a0")){return 4;}  else if (!strcmp(reg,"$a1")){return 5;}  else if (!strcmp(reg,"$a2")){return 6;}  else if (!strcmp(reg,"$a3")){return 7;}
    /** t */
    else if (!strcmp(reg,"$t0")){return 8;}  else if (!strcmp(reg,"$t1")){return 9;}  else if (!strcmp(reg,"$t2")){return 10;} else if (!strcmp(reg,"$t3")){return 11;}
    else if (!strcmp(reg,"$t4")){return 12;} else if (!strcmp(reg,"$t5")){return 13;} else if (!strcmp(reg,"$t6")){return 14;} else if (!strcmp(reg,"$t7")){return 15;}
    /** s */
    else if (!strcmp(reg,"$s0")){return 16;} else if (!strcmp(reg,"$s1")){return 17;} else if (!strcmp(reg,"$s2")){return 18;} else if (!strcmp(reg,"$s3")){return 19;}
    else if (!strcmp(reg,"$s4")){return 20;} else if (!strcmp(reg,"$s5")){return 21;} else if (!strcmp(reg,"$s6")){return 22;} else if (!strcmp(reg,"$s7")){return 23;}
    /** t */
    else if (!strcmp(reg,"$t8")){return 24;} else if (!strcmp(reg,"$t9")){return 25;}
    /** k */
    else if (!strcmp(reg,"$k0")){return 26;} else if (!strcmp(reg,"$k1")){return 27;}
    /** 나머지 */
    else if (!strcmp(reg,"$gp")){return 28;} else if (!strcmp(reg,"$sp")){return 29;}
    else if (!strcmp(reg,"$fp")){return 30;} else if (!strcmp(reg,"$ra")){return 31;}
    //=====
    else{
        char * cut = strtok(reg,"$");
        return atoi(cut);
    }

}
int labelToIntAddr(char *label) {

    if(strcmp(label, "main") == 0){
        return 0x00400024;
    }
    int i;
    for (i=0; i<labelindex; i++) {
        if (strcmp(label_list[i].name, label) == 0) {
            return label_list[i].location;
        }
    }
    return -1;
}
int immToInt(char *immediate){
    // 2 cases - hexadecimal and decimal.
    if (strstr(immediate,"0x") != NULL) {
        return (int)strtol(immediate, NULL, 0);
    } else {
        return atoi(immediate);
    }
}
char *int2bin(int a, char *buffer, int buf_size) {
    buffer += (buf_size - 1);
    for (int i = 31; i >= 0; i--) {
        *buffer-- = (a & 1) + '0';
        a >>= 1;
    }
    return buffer;
}



//====================================================================




Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    data = 100;
    qDebug() << data;
    data = 200;
    qDebug() << data;
}

Dialog::~Dialog()
{
    delete ui;
}

void check(){

    qDebug() << "flag";

}


//cycle
void Dialog::on_pushButton_clicked()
{
    WB();
    MEM();
    EX();
    ID();
    IF();
}

// IF_Id_print
void Dialog::on_pushButton_2_clicked()
{
    IF_ID_print();
}




void Dialog::on_pushButton_3_clicked()
{
    ID_EX_print();
}


void Dialog::on_pushButton_4_clicked()
{
    EX_MEM_print();
}


void Dialog::on_pushButton_5_clicked()
{
    MEM_WB_print();
}


void Dialog::on_pushButton_6_clicked()
{
    Register_print();
}


void Dialog::on_pushButton_7_clicked()
{



}

