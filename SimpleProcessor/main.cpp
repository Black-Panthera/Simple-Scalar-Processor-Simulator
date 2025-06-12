#include <bits/stdc++.h>
using namespace std;

constexpr uint8_t LOWER4 = 0b00001111;


/* Below are the classes that represent different buffers similar to the buffers in a simple proccessor

    Buffers store the computed data like the values in source registers,ALU unit etc..
    from it's respective pipeline stage at the end of the clockcycle 

    The data present from the buffer is taken by the next pipeline stage in the beginning of next clockcycle
*/

class fetchBuffer{
    public:
    uint8_t InstructionNumber ; // to help in debugging, to keep track of the instruction number
    // uint8_t because the max number of instructions is 128 ===256/2 , I$=256B , each instruction = 2B
    uint16_t IR ; // instruction register to store the encoded instruction present at PC
};

class decodeBuffer{
    public:
    uint8_t InstructionNumber;
    uint16_t IR; 
    uint8_t destReg; // no such thing like uint_4 in C++, so we use 8, and keep the upper most 4 bits 0
    int8_t A; // value at sourcereg 1
    int8_t B;  // if the operation is store,the B store the value at destreg else sourcereg2
    uint8_t shift; 
    int8_t offset; //offset can be negative
    uint8_t opCode; // the operation to be executed
};

class executeBuffer{
    public:
        uint8_t InstructionNumber;
        uint16_t IR;
        uint8_t opCode;
        uint8_t destReg;
        int8_t AluOut;
        int8_t B; // needed for the store operation
        
};

class memBuffer{
    public:
        uint8_t InstructionNumber;
        uint16_t IR;
        int8_t AluOut; 
        int8_t LMD;
        uint8_t destReg;
        uint8_t opCode;
        
};

/*  add debugger statements to check for out of bound accesses (omitted here)
    sometimes out of bound accesses may not show up in the compiler as errors
    instead may silently corrupt the memory leading to undefined behaviour like scribbling over C++ internals like double,cout,iostream etc..
    when used, the program crashes and silently dies
*/

int main(){
    vector<int8_t> D(256);
    vector<uint8_t> I(256);
    vector<int8_t> R(16); // register file/bank, here it has 16 8-bit registers
    
    ifstream I_Cache ("input/ICache.txt");
    ifstream D_Cache ("input/DCache.txt");
    ifstream Rf ("input/RF.txt");

    for(int i=0;i<256;i++){
        string temp;
        I_Cache>>temp;
        I[i] = static_cast<uint8_t> (stoi(temp,nullptr,16)); //converts the string into uint8_t with base = 16
    }
    for(int i=0;i<256;i++){
        string temp;
        D_Cache>>temp;
        D[i] = static_cast<int8_t> (stoi(temp,nullptr,16));
    }
    for(int i = 0;i<16;i++){
        string temp;
        Rf >> temp;
        R[i] = static_cast <int8_t> (stoi(temp,nullptr,16));
    }

    int totalInstructions=0;
    int arthimeticInstructions=0;
    int logicalInstructions=0;
    int shiftInstructions=0;
    int memoryInstructions=0;
    int haltInstructions=0;
    int controlInstructions=0;
    
    int totalStalls=0;
    int dataStalls=0;
    int controlStalls=0;

    int clockcycles=0; //clock cycles

    vector<bool> dependency(16,false); // to check if there's a dependency, for RAW hazards
    
    int pc=0;

    fetchBuffer fetchBuff;
    decodeBuffer decodeBuff;
    executeBuffer executeBuff;
    memBuffer memBuff;

    int stall=-1; //to represent the type of stall, if raw, then 1, if control then 2, if -1 no stall

    bool fetch = true;
    bool execute = false;
    bool decode = false;
    bool mem = false;
    bool writeback = false;

    // indicates which pipelines to be executed parallely in the current clock cycle
    // in the first cycle, only fetch stage runs

    /* the stages are written in reverse manner in the while loop, cause if it's written in the correct order 
        it behaves unintendedly

        e.g initially once the fetch is ran, we give green signal to decode, if written in correct order then, it runs decode
        then execute so on.. this will be recorded as single clock cyle, which is incorrect, the actual value is 5

        look at the timeline of the instructions to get a clear picture
    */

    //make sure all the buffer elements are filled appropriately at the end of execution of a stage
    // cout<<"start"<<"\n";
    while(true){
        clockcycles++;
        if(stall!=3) fetch=true; //stall=3 meaning the halt instruction was hit

        if(writeback){
            if(memBuff.opCode==15){
                break; //once the halt instruction reaches writeback stage, all the prev instructions must have been completed
                //now we can happily break out of the while loop
            }
            else if(memBuff.opCode>=0 && memBuff.opCode<=10){
                R[memBuff.destReg]=memBuff.AluOut;
                dependency[memBuff.destReg]=false;
            }
            else if(memBuff.opCode==11){
                R[memBuff.destReg] = memBuff.LMD;
                dependency[memBuff.destReg]=false;
            }
            
        }

        if(mem){
            writeback=true;

            memBuff.InstructionNumber = executeBuff.InstructionNumber;
            memBuff.IR = executeBuff.IR;
            memBuff.destReg = executeBuff.destReg;
            memBuff.AluOut = executeBuff.AluOut;
            memBuff.opCode = executeBuff.opCode;
            
            if(memBuff.opCode==11){
                memBuff.LMD =  D[memBuff.AluOut];
            }
            else if(memBuff.opCode==12){
                D[memBuff.AluOut]=executeBuff.B;
            }
        }
        else{
            writeback=false;
        }

        if(execute){
            mem = true;
            totalInstructions++;

            executeBuff.InstructionNumber = decodeBuff.InstructionNumber;
            executeBuff.IR =  decodeBuff.IR;
            executeBuff.opCode = decodeBuff.opCode;
            executeBuff.destReg = decodeBuff.destReg;

            uint8_t opCode = executeBuff.opCode;
            
            if(opCode==0){
                arthimeticInstructions++;
                executeBuff.AluOut = decodeBuff.A + decodeBuff.B;
            }
            else if(opCode==1){
                arthimeticInstructions++;
                executeBuff.AluOut = decodeBuff.A - decodeBuff.B;
            }
            else if(opCode==2){
                arthimeticInstructions++;
                executeBuff.AluOut = decodeBuff.A*decodeBuff.B;
            }
            else if(opCode==3){
                arthimeticInstructions++;
                executeBuff.AluOut = decodeBuff.A+1;
            }
            else if(opCode==4){
                logicalInstructions++;
                executeBuff.AluOut = decodeBuff.A & decodeBuff.B;
            }
            else if(opCode==5){
                logicalInstructions++;
                executeBuff.AluOut = decodeBuff.A | decodeBuff.B;
            }
            else if(opCode==6){
                logicalInstructions++;
                executeBuff.AluOut = decodeBuff.A ^ decodeBuff.B;
            }
            else if(opCode==7){
                logicalInstructions++;
                executeBuff.AluOut = ~ decodeBuff.A;
            }
            else if(opCode==8){
                shiftInstructions++;
                executeBuff.AluOut = decodeBuff.A << decodeBuff.shift;
            }
            else if(opCode==9){
                shiftInstructions++;
                uint8_t temp = static_cast<uint8_t>(decodeBuff.A);
                temp = temp >> decodeBuff.shift;
                executeBuff.AluOut = static_cast<int8_t> (temp);
                // sign is not preserved, negative value can become a positive value
                // if shift operation is performed on signed int value, sign gets carried
            }
            else if(opCode==10){
                memoryInstructions++;
                executeBuff.AluOut = decodeBuff.offset; //immediate
            }
            else if(opCode==11){
                memoryInstructions++;
                executeBuff.AluOut = decodeBuff.A + decodeBuff.offset;
            }
            else if(opCode==12){
                memoryInstructions++;
                executeBuff.AluOut = decodeBuff.A+ decodeBuff.offset;
                executeBuff.B =  decodeBuff.B;
            }
            else if(opCode==13){
                controlInstructions++;
                pc = pc + 2*decodeBuff.offset;
                fetch=false;

            }
            else if(opCode==14){
                controlInstructions++;
                fetch=false;
                if(decodeBuff.A ==0) pc = pc + 2*decodeBuff.offset;
            }
            else if(opCode==15){
                haltInstructions++;
                decode = false;
            }

        }
        else{
            mem = false;
        }


        if(decode){
            decodeBuff.InstructionNumber= fetchBuff.InstructionNumber;
            decodeBuff.IR = fetchBuff.IR;

            decodeBuff.opCode = (decodeBuff.IR>>12);

            int IR= decodeBuff.IR;
            int opCode= decodeBuff.opCode;
            decodeBuff.destReg= (IR>>8) & LOWER4;
            execute=true;

            if(opCode==0 || opCode==1 || opCode==2 || opCode==4 || opCode ==5 || opCode==6){
                //operations of the form destreg<-sourereg1 op sourcereg2 
                
                uint8_t sourceReg1 = (IR>>4) & LOWER4;
                uint8_t sourceReg2 = (IR&LOWER4);

                if(!dependency[sourceReg1] && !dependency[sourceReg2]){
                    dependency[decodeBuff.destReg]=true;
                    decodeBuff.A = R[sourceReg1];
                    decodeBuff.B = R[sourceReg2];

                }
                else {
                    stall=1; //raw hazard
                    execute=false;
                    fetch=false;
                }

            }

            else if(opCode==3){
                uint8_t sourceReg = decodeBuff.destReg;

                if(!dependency[sourceReg]){
                    dependency[decodeBuff.destReg]=true;
                    decodeBuff.A= R[sourceReg];
                }
                else{
                    execute=false;
                    fetch=false;
                    stall=1;
                }
            }

            else if(opCode==7){

                uint8_t sourceReg = (IR>>4) & LOWER4;
                 if(!dependency[sourceReg]){
                    dependency[decodeBuff.destReg]=true;
                    decodeBuff.A= R[sourceReg];
                }
                else{
                    execute=false;
                    fetch=false;
                    stall=1;
                }
            }

            else if(opCode==8 || opCode==9){
                uint8_t sourceReg = (IR>>4) & LOWER4;
               
                if(!dependency[sourceReg]){
                    dependency[decodeBuff.destReg]=true;
                    decodeBuff.A= R[sourceReg];
                    decodeBuff.shift= IR & LOWER4;
                }
                else{
                    execute=false;
                    fetch=false;
                    stall=1;
                }
            }

            else if(opCode==10){
                dependency[decodeBuff.destReg]=true;
                decodeBuff.offset = static_cast<int8_t>(IR&0xFF);
            }

            else if(opCode==11){
                uint8_t sourceReg = (IR>>4)&LOWER4;
                if(dependency[sourceReg]){
                    stall=1;
                    execute=false;
                    fetch=false;
                }
                else {
                    dependency[decodeBuff.destReg]=true;
                    decodeBuff.A = R[sourceReg];
                    uint8_t offset = IR & LOWER4;
                    if(offset & 0b00001000!=0){
                        offset = offset | 0b11110000;
                    }
                    //for handling negative offset, note that offset is encoded in 4 bits only
                    // sign extension

                    offset = static_cast<int8_t>(offset);
                    decodeBuff.offset = offset;
                }
            }

            else if(opCode==12){

                uint8_t sourceReg1 = (IR>>4) & LOWER4;
                uint8_t sourceReg2 = (IR>>8) & LOWER4;

                if(dependency[sourceReg1]||dependency[sourceReg2]){
                    stall=1;
                    execute=false;
                    fetch=true;
                }
                else{
                    uint8_t offset = IR & LOWER4;
                    if((offset & 0b00001000)!=0){
                        offset = offset | 0b11110000;
                    }
                    offset = static_cast<int8_t>(offset);
                    decodeBuff.offset = offset;
                    decodeBuff.A =  R[sourceReg1];
                    decodeBuff.B = R[sourceReg2];

                }

            }

            if(opCode==13){
                decodeBuff.offset = static_cast<int8_t>((IR>>4)&0xFF);
                fetch=false;
                stall=2; //control hazard
            }

            else if(opCode==14){
                uint8_t sourceReg = (IR>>8)&LOWER4;
                if(dependency[sourceReg]){
                    stall=1;
                    execute=false;
                    fetch=false;
                }
                else{
                    decodeBuff.A = R[sourceReg];
                    uint8_t offset = IR;
                    decodeBuff.offset = static_cast<int8_t> (offset);
                    stall=2;
                    fetch=false;
                }

            }
            
            else if(opCode==15){
                fetch=false; 
                stall=3; // 3 here represents halt
                // : do not use break, breaking at this point causes the previous insturctions to stop in between
                //break in writeback
            }
        }
        else{
            execute=false;
            }


        if(fetch){
            if(pc+1>=256) break; 
            uint16_t upper= I[pc]<<8;
            uint16_t lower= I[pc+1];

            // instruction is encoded in 16 bits, each line in I is 8 bit long, follows little-endian
            fetchBuff.IR= upper|lower;
            fetchBuff.InstructionNumber=pc;
            pc=pc+2;
            decode=true;

        }
        else{
            decode=false;
            if(stall==1) {
                dataStalls++;
                totalStalls++;
                decode=true;
            }
            if(stall==2){
                controlStalls++;
                totalStalls++;
            }

        }
    }
    
    ofstream Dout ("output/DCache.txt");
    ofstream out ("output/Output.txt");
    
    for(int8_t v:D){
        uint8_t temp = (v>>4)&LOWER4;
        if(temp>=10){
            Dout<<char('a'+temp-10);
        }
        else{
            Dout<<char('0'+temp);
        }
        temp=v&LOWER4;
        if(temp>=10){
            Dout<<char('a'+temp-10)<<"\n";
        }
        else{
            Dout<<char('0'+temp)<<"\n";
        }
    }
    
    double CPI = (double)(clockcycles)/(double)(totalInstructions);
    
    out << "Total number of instructions executed :"<<totalInstructions<<"\n";
    out << "Number of instructions in each class"<<"\n";
    out << "Arithmetic instructions               :"<<arthimeticInstructions<<"\n";
    out << "Logical instructions                  :"<<logicalInstructions<<"\n";
    out << "Shift instructions                    :"<<shiftInstructions<<"\n";
    out << "Memory instructions                   :"<<memoryInstructions<<"\n";
    out << "Control instructions                  :"<<controlInstructions<<"\n";
    out << "Halt instructions                     :"<<haltInstructions<<"\n";
    out << "Cycles Per Instruction                :"<<CPI<<"\n";
    out << "Total number of stalls                :"<<totalStalls<<"\n";
    out << "Data stalls (RAW)                     :"<<dataStalls<<"\n";
    out << "Control stalls                        :"<<controlStalls<<"\n";
}