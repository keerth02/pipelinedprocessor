#include <iostream>
#include <fstream>
#include <string.h>
using namespace std;

int PC;
char A[2],B[2],LMD[2],ALUout[2];  //registers
int Reg[16];       


int DATA_STALLS=0;
int CONTROL_STALLS=0;

//instructions of different types
int no_of_arith=0;
int no_of_logic=0;
int no_of_data=0;
int no_of_cont=0;
int no_of_halt=0;  


class Instruction
{
public:

    char I[4];
    string opcode;
    int src1;
    int src2;
    int dest;
    int result;

    Instruction()
    {
        dest = 0;
        src1 = 0;
        src2 = 0;
        result = 0;
        opcode="Not Set Yet";
    }
};

class Stage
{
public:
    int busy;
    int over;
    Instruction current;

    Stage()
    {
        busy = 0;
        over = 1;
    }
};

//functions to convert between different bases
int convert_to_int(char A[2])
{
    int num = 0;
    if (A[0] <= '7')
    {
        num += (A[0] - '0') * 16;
        if (A[1] <= '9')
        {
            num += (A[1] - '0');
        }
        else
        {
            num += A[1] - 87;
        }
    }
    else
    {
        if (A[0] <= '9')
        {
            num += (A[0] - '0')*16;
        }
        else
        {
            num += (A[0] - 87)*16;
        }

        if (A[1] <= '9')
        {
            num += (A[1] - '0');
        }
        else
        {
            num += A[1] - 87;
        }

        num = num - 128;
        num = (-128 + num);

    }

    return num;
}

int convert_to_int_1(char A)
{
    int num = 0;
    if (A <= '9')
    {
        num = (A - '0');
    }
    else
    {
            num = (A - 87) ;
    }
    
    
    return num;
}

void convert_2s_comp_hex(char res[2],int n)
{
   if(n>=0)
   {
     ; //already taken care of
   }
   else
   {
      int x=128+n;
      int m=x%16;
      
      
      if(m<=9)
       res[1]='0'+m;
      else
       res[1]='a'+m-10;
      
      int s=8;
      x=x/16;
      s+=x;
      
      
      if(s<=9)
       res[0]='0'+s;
      else
       res[0]='a'+s-10;
   }
}

char convert_to_hex(int a)
{
    if (a <= 9)
        return a + 48;
    else
        return a + 87;
}

//compute using ALU
int ALU(int op1, int op2, string code)
{
    if (code == "ADD")
        return op1 + op2;
    else if (code == "SUB")
        return op1 - op2;
    else if (code == "MUL")
        return op1 * op2;
    else if (code == "INC")
        return op1 + 1;
    else if (code == "AND")
        return op1 & op2;
    else if (code == "OR")
        return op1 | op2;
    else if (code == "NOT")
        return ~op1;
    else if (code == "XOR")
        return op1 ^ op2;
    else
        return 0;
}

//functions to determine offset and index to access the data cache
int findOffset(char A[2])
{
    int decimal = convert_to_int(A);
    int offset = 0;
    offset += decimal % 2;
    decimal /= 2;
    offset+= (decimal % 2)*2;
    return offset;
}

int findIndex(char A[2])
{
    int decimal = convert_to_int(A);
    int index = decimal >> 2;
    return index;
}

//assign opcodes
string set_opcode(int n)
{
   
    if (n == 0)
    {
        no_of_arith++;
        return "ADD";
    }
    else if (n == 1)
    {
        no_of_arith++;
        return "SUB";
    }
    else if (n == 2)
    {
        no_of_arith++;
        return "MUL";
    }
    else if (n == 3)
    {
         no_of_arith++;
        return "INC";
    }
    else if (n == 4)
    {
        no_of_logic++;
        return "AND";
    }
    else if (n == 5)
    {
        no_of_logic++;
        return "OR";
    }
    else if (n == 6)
    {
        no_of_logic++;
        return "NOT";
    }
    else if (n == 7)
    {
        no_of_logic++;
        return "XOR";
    }
    else if (n == 8)
    {
        no_of_data++;
        return "LOAD";
    }
    else if (n == 9)
    {
        no_of_data++;
        return "STORE";
    }
    else if (n == 10)
    {
        no_of_cont++;
        return "JMP";
    }
    else if (n == 11)
    {
        no_of_cont++;
        return "BEQZ";
    }
    else if (n == 15)
    {
        no_of_halt++;
        return "HLT";
    }
    else 
        return "ERROR";
}

Instruction decode(char inst[4])
{
    
    int op_c = convert_to_int_1(inst[0]);
    Instruction D;

    D.opcode = set_opcode(op_c);
    D.src1 = convert_to_int_1(inst[2]);
    D.src2 = convert_to_int_1(inst[3]);
    D.dest = convert_to_int_1(inst[1]);
    
    if(D.opcode=="INC")
    {
      D.src1 = convert_to_int_1(inst[1]);
      D.dest = convert_to_int_1(inst[1]);           
    }
    
    if(D.opcode=="BEQZ")
    {
       D.src1 = convert_to_int_1(inst[1]);
       D.src2 = -1;
       D.dest = -1;
    }
    
    for(int j=0;j<4;j++)
    {
       (D.I)[j]=inst[j];
    }

    return D;
}

int curr_time;   //total no of cycles
Stage IF, ID, EX, MEM, WB;   //5 stages of the pipeline

int main()
{
    ifstream ic;
    fstream dc, rf, stats;
    
    //input files
    ic.open("ICache.txt");
    dc.open("DCache.txt", ios::out | ios::in);
    rf.open("RF.txt", ios::out | ios::in);

    int no_of_instructions,no_of_inst_from_fetch=0;
    ic.seekg(0,ios::end);
    no_of_instructions=ic.tellg()/6;
    ic.seekg(0,ios::beg);
       
    for (int i = 0; i < 16; i++)
      Reg[i] = 1;   //correct data, ready to supply

    Instruction inst;
    int dont_read_anymore=0;

    PC = 0;
    int cond;
    int INST_DECODED=0;
    int waiting_for_branch=0;

    do
    {
        //WRITE BACK STAGE
        if (WB.busy)
        {
        
            inst = WB.current;
            //cout<<"WB stage has "<<inst.I<<endl;
            
            if (!WB.over)
            {
                if(inst.opcode=="LOAD")
                {
                   rf.seekg((inst.dest) * 3, ios::beg);                   
                   rf.write(LMD,2);
                   
                   Reg[inst.dest]=1;
                                      
                }
                else if(inst.opcode=="STORE")
                {
                   WB.over = 1;  
                }
                else if (inst.dest >= 0 && inst.dest < 16&&inst.opcode!="JMP"&&inst.opcode!="BEQZ"&&inst.opcode!="HLT")
                {
                    rf.seekg((inst.dest) * 3, ios::beg);
                    
                    char res[2];
                    if(inst.result>=0)
                    {
                       res[0] = convert_to_hex(inst.result/16);
                       res[1] = convert_to_hex(inst.result%16);
                    }
                    else
                    {
                       convert_2s_comp_hex(res,inst.result);
                    }
                                        
                    rf.write(res,2);
                    
                    Reg[inst.dest]=1;
                    
                   
                }
                WB.over = 1;
            }
            WB.busy = 0;
        }

            //Memory Stage
            if (MEM.busy)
            {
                inst = MEM.current;
                //cout<<"MEM stage has "<<inst.I<<endl;
                             
                if (MEM.over != 1)
                {
                    if (inst.opcode == "STORE")
                    {
                        
                        
                        int index = findIndex(ALUout);
                        int offset = findOffset(ALUout);
                        
                        dc.seekg(index * 12 + offset*3, ios::beg);
                        
                        char temp[2];    
                        
                        rf.seekg(inst.dest * 3, ios::beg);
                        rf.read(temp, 2);
                     
                        dc.write(temp,2);
                                               

                        if (!WB.busy)
                        {
                            MEM.busy = 0;
                            WB.current = inst;
                            WB.busy = 1;
                            WB.over = 0;
                        }
                       
                    }
                    else if (inst.opcode == "LOAD")
                    {
                        int index = findIndex(ALUout);
                        int offset = findOffset(ALUout);
                        
                        dc.seekg(index * 12 + offset*3, ios::beg);
                        dc.read(LMD, 2);
                        
                                               
                                                
                        if (!WB.busy)
                        {
                            MEM.busy = 0;
                            WB.current = inst;
                            WB.busy = 1;
                            WB.over = 0;
                        }
                    }
                    else
                    {
                       if (!WB.busy)
                        {
                            MEM.busy = 0;
                            WB.current = inst;
                            WB.busy = 1;
                            WB.over = 0;
                        }                    
                    }
                    MEM.over = 1;
                }
            }
        
            //EXECUTE STAGE
            if (EX.busy)
            {
                inst = EX.current;
                //cout<<"EX stage has "<<inst.I<<endl;
                
                if (EX.over != 1)
                {
                    if (inst.opcode == "LOAD" || inst.opcode == "STORE")
                    {
                        int R2 = convert_to_int(A);
                        
                        
                        char offs[2];
                        offs[1]=inst.I[3];
                        
                        if(inst.I[3]<'8')
                          offs[0]='0';
                        else
                          offs[0]='f';
                          
                        int X = convert_to_int(offs);
                        
                        
                        int address = ALU(R2, X, "ADD");
                        
                        
                        ALUout[0] = convert_to_hex(address / 16);
                        ALUout[1] = convert_to_hex(address % 16);
                        
                    }
                    else if (inst.opcode == "JMP")
                    {
                        char t[2];
                        t[0]=inst.I[1];
                        t[1]=inst.I[2];
                        int L2=convert_to_int(t);
                        
                        int addr = PC + (L2*6); //L2 has to be sign extended offset
                        PC=addr;
                       
                        waiting_for_branch=0;
                        
                        inst.result=addr;
                        
                        ALUout[0] = convert_to_hex(addr / 16);
                        ALUout[1] = convert_to_hex(addr%16);
                        
                    }
                    else if (inst.opcode == "BEQZ")
                    {
                        
                        if (A[0] == '0' && A[1] == '0')
                            cond = 1;
                        else cond = 0;
                        
                        if(cond==1)
                        {
                           
                           char t[2];
                           t[0]=inst.I[2];
                           t[1]=inst.I[3];
                           int L2=convert_to_int(t);
                           
                           int addr = PC + (L2*6); //L2 has to be sign extended offset
                           
                           PC=addr;
                           
                        }
                        
                        
                        waiting_for_branch=0;
                        
                    }
                    else
                    {
                        int a = convert_to_int(A);
                        int b = convert_to_int(B);
                        
                        
                        
                        int res = ALU(a, b, inst.opcode);
                        
                        inst.result=res;
                        
                        
                        ALUout[0] = convert_to_hex(res / 16);
                        
                       
                        
                        ALUout[1] = convert_to_hex(res%16);
                        
                    }

                    EX.over = 1;
                }
                if (!MEM.busy)
                {
                    MEM.busy = 1;
                    MEM.over = 0;
                    MEM.current = inst;
                    EX.busy = 0;
                }
                
            }

        //INSTRUCTION DECODE STAGE
        if (ID.busy)
        {
            inst = ID.current;
            //cout<<"ID stage has "<<inst.I<<endl;
             
             
            if (ID.over != 1)
            {
                if(INST_DECODED!=1)
                {
                  ID.current = decode(inst.I);
                  //cout<<"Decoded Instruction is "<<ID.current.opcode<<" "<<ID.current.src1<<" "<<ID.current.src2<<" "<<ID.current.dest<<endl;
                  
                  if(ID.current.opcode=="HLT")  //don't read any more instructions
                  {
                     dont_read_anymore=1;
                     ID.busy=0;
                     IF.busy=0;
                     continue;
                  }
                  
                  INST_DECODED=1;
                  inst = ID.current;
                                  
                }
                  if(inst.opcode == "JMP")
                  {
                    waiting_for_branch=1;
                    
                       if (!EX.busy)
               	{
             		   EX.current = ID.current;
                          EX.busy = 1;
                	   EX.over = 0;
               	   ID.busy = 0;
               	   INST_DECODED=0;
               	   
           		}
                    
                  }
                  
                  else if(inst.opcode == "BEQZ")
                  {
                      waiting_for_branch=1;
                      
                      if (Reg[inst.src1] == 1)
                      {
                         
                         rf.seekg(inst.src1 * 3, ios::beg);
                         rf.read(A, 2);
                         
                         
                         if (!EX.busy)
               	  {
             		   EX.current = ID.current;
                          EX.busy = 1;
                	   EX.over = 0;
               	   ID.busy = 0;
               	   INST_DECODED=0;
               	   
           		  }
                      }
                      else
                      {
                        //cout<<"BEQZ waiting"<<endl;
                      }
                      
                       
                  }

                if (inst.opcode != "LOAD" && inst.opcode != "STORE" && inst.opcode != "JMP" && inst.opcode != "BEQZ" && inst.opcode != "HLT")
                {
                    if (Reg[inst.src1] == 1 && Reg[inst.src2] == 1)
                    {
                        
                        rf.seekg(inst.src1 * 3, ios::beg);
                        rf.read(A, 2);

                        rf.seekg(inst.src2 * 3, ios::beg);
                        rf.read(B, 2);
                        ID.over = 1;
                        
                        Reg[inst.dest] = 0;                                                
                        
                        if (!EX.busy)
               	 {
             		    EX.current = ID.current;
                           EX.busy = 1;
                	    EX.over = 0;
               	    ID.busy = 0;
               	    INST_DECODED=0;
               	    
           		 }
           		 
                    }
                    else
                    {
                      DATA_STALLS ++;
                    }
                }
               
                else if(inst.opcode == "LOAD")
                {
                   if (Reg[inst.src1] == 1)
                   {
                       rf.seekg(inst.src1 * 3, ios::beg);
                       rf.read(A, 2);
                       
                       Reg[inst.dest] = 0;
                        
                       if (!EX.busy)
               	{
             		   EX.current = ID.current;
                          EX.busy = 1;
                	   EX.over = 0;
               	   ID.busy = 0;
               	   INST_DECODED=0;
               	   
               	   
           		}
                   }
                   else
                   {
                       DATA_STALLS ++;
                   }
                   
                }
                else if(inst.opcode == "STORE")
                {
                   if (Reg[inst.src1] == 1 && Reg[inst.dest] == 1)
                   {
                       rf.seekg(inst.src1 * 3, ios::beg);
                       rf.read(A, 2);
                       
                       rf.seekg(inst.dest * 3, ios::beg);
                       rf.read(B, 2);
                       
                        
                       inst.result=convert_to_int(B);  
                       
                       
                                              
                       if (!EX.busy)
               	{
             		   EX.current = ID.current;
                          EX.busy = 1;
                	   EX.over = 0;
               	   ID.busy = 0;
               	   INST_DECODED=0;
               	   
           		}
                   }
                   else
                   {
                       DATA_STALLS ++;
                   }
                   
                }
                
            }
    
        
        }
        
        //INSTRUCTION FETCH STAGE
        if (!IF.busy&&ic&&!dont_read_anymore)
        {
            if(waiting_for_branch)
            {
              CONTROL_STALLS++;              
            }
            else
            {
            char temp_i[3],dummy[2];
            ic.seekg(PC, ios::beg);
            ic.read(temp_i, 2);
            inst.I[0]=temp_i[0];
            inst.I[1]=temp_i[1];
            
            ic.seekg(1, ios::cur);
            ic.read(temp_i, 2);
            inst.I[2]=temp_i[0];
            inst.I[3]=temp_i[1];
            
            ic.read(dummy,2);
            
            IF.current=inst;
            no_of_inst_from_fetch++;
            
            //cout<<"Inst read "<<inst.I<<endl;
           
            PC = PC + 6;
            
            IF.over = 1;
            IF.busy=1;
            
            if (!ID.busy)
            {
                ID.current = inst;
                ID.busy = 1;
                ID.over = 0;
                IF.busy = 0;
            }
            
            }
        }
        else if(IF.busy&&!dont_read_anymore)
        {
            //cout<<"IF stage has "<<inst.I<<endl;
            if (!ID.busy)
            {
                ID.current = IF.current;
                ID.busy = 1;
                ID.over = 0;
                IF.busy = 0;
            }
        }

        curr_time++;   //update time counter
        
        

    } while ((WB.busy||MEM.busy||EX.busy||ID.busy||IF.busy));
    
    ofstream foutput;
    foutput.open("Output.txt");

    foutput<<"Total number of instructions: "<<no_of_inst_from_fetch<<"\n";
    foutput<<"Number of instructions in each class"<<"\n";
    foutput<<"Arithmetic instructions: "<<no_of_arith<<"\n";
    foutput<<"Logical instructions: "<<no_of_logic<<"\n";
    foutput<<"Data instructions: "<<no_of_data<<"\n";
    foutput<<"Control instructions: "<<no_of_cont<<"\n";
    foutput<<"Halt instructions: "<<no_of_halt<<"\n";
    
    //foutput << "Total time = " << curr_time << endl;
    
    foutput<<"Cycles Per Instruction: "<<(float) curr_time/no_of_inst_from_fetch<<"\n";
    foutput<<"Total number of stalls: "<<DATA_STALLS+CONTROL_STALLS<<"\n";
    foutput<<"Data stalls (RAW): "<<DATA_STALLS<<"\n";
    foutput<<"Control stalls: "<<CONTROL_STALLS<<"\n";


}