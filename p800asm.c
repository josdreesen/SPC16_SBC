
/*-----------------------------------------------------------*/
/*  Philips P850  assembler                                  */
/*                                                           */
/*   Quick & dirty assembler for Philips P800 machines       */
/*   Matches mostly the Philips syntax                       */
/*                                                           */
/*   V1.0 : first usable version                             */
/*   V1.1 : -proper handling of spaces in datastrings        */
/*          - added include-file (RDSRC,EOF cmds)            */
/*   V1.2 : - added intel hex output                         */
/*          - parameter handling                             */
/*          - EXTRN / INTRN /RES handling                    */
/*   Todo :                                                  */
/*     improve reader / DATA routine                         */
/*     implement  COMN                                       */
/*     check for undefined variables                         */
/*                                Jos Dreesen 2024           */
/*-----------------------------------------------------------*/

#include  <stdlib.h>
#include  <stdio.h>
#include  <ctype.h>
#include  <string.h>
#include  <stdbool.h>
#include  <unistd.h>

/* include the mnemonics lookup table */
#include "p800_def.h"

/* Max name length */
#define WRDL      12

/* Max number of variables */
#define MAX_VARI   400
#define globaLs "extrn.lbl"

#define  hwm 32768
unsigned int code[hwm];

#define ll 250
char line[ll];

#define ihx_len 16

struct VARIAB { char name[WRDL]; unsigned int val; bool islbl; bool isglob; bool isdef;} Vari[MAX_VARI];
FILE *asm_ipf, *inc_file, *act_file;
unsigned short  VariCnt=0; // Label count
unsigned short  LinCnt=0,LinCntSave; // Line counter
unsigned short  Pc=0, From=0;        // Program Counter
char            ProjName[20];
bool            ProjDefined=false;
bool            List;                // List program on/off
short int       Pass;
bool 		End=false, HasGlobals=false;

/*-----------------------------------------------------------*/
/*          Output routines                                  */
/*-----------------------------------------------------------*/

void ErrorMsg(char *msg, char *para)
{ 
printf(" ERROR @ line %d : %s %s\n",LinCnt, msg, para); 
exit(1);
}  

// Count leading space 
short int cls(char *msg)
{  unsigned short j=0;
while  isspace(msg[j++]); 
return j-1;
}

/* remove text from inputline*/
void reduce(char *line,short len)
{ unsigned short i,l;
 l=strlen(line)-len+1;
 for (i=0;i<l;i++)  line[i]=line[i+len];
} 

void ListVariab()
{ int i;
printf("-------- List of variables / labels ---------");
for (i=0;i<VariCnt;i++)
  {    if ((i%3)==0) printf("\n");
   printf(" %8s--%4.4X",Vari[i].name,Vari[i].val);
   }
printf("\n---------------------------------------------\n");   
}

/*-----------------------------------------------------------*/
/*      Table lookup routines                                */
/*-----------------------------------------------------------*/

// Look up a label or variable name
int LookupVar(char *param)
{ int i; 
for (i=0;i<VariCnt ;i++)
  if (strcmp(Vari[i].name, param)==0) 
     return i;
return -1;     
}

// Look up a P85x mnemonic
int LookupMnemo(char *symbol)
{ int i, res; bool fnd = false; 
for (i=0;(i<NR_MNEM) && (fnd == false); i++)
  if (strcmp(mnem_table[i].name, symbol)==0) 
     return i;
if (fnd==false)  
   ErrorMsg("unidentified mnemonic",symbol);
}

// Look up assembl directives
int LookupCmd(char *cmd)
{ int i; 
for (i=0;i<NR_CMDS ;i++)
  if (strcmp(commands[i].name, cmd)==0) 
     return i;
return -1;     
}

// Look up a registerdefinition. Special case for UCSDpascal added.
int LookupReg(char *reg)
{ int i; 
for (i=0;i<NR_REGS ;i++)
  if (strcmp(regs[i].name, reg)==0) 
     return i & 0x0F; // restrict to 4 bits / because of  UCSD pascal alternative reg names)
ErrorMsg("Undefined Register",reg);         
}

// Look up  condition code
unsigned short LookupCc(char *cc)
{ int i, res;
 res=0;
 if (isdigit(cc[0])) return (cc[0]-'0'); // numeric constant;
 if ((cc[0]=='N') && (cc[2]==')')) { res=4; cc[0]=cc[1]; }
 if (cc[0]=='Z')  return(res+0);
 if (cc[0]=='P')  return(res+1);
 if (cc[0]=='N')  return(res+2);
 if (cc[0]=='O')  return(res+3);
 if (cc[0]=='E')  return(res+0);
 if (cc[0]=='G')  return(res+1);
 if (cc[0]=='L')  return(res+2);
 if (cc[0]=='A')  return(res+0);
 if (cc[0]=='R')  return(res+1);
 if (cc[0]=='U')  return(res+3);
}

/* add a  labelvariable to Variables  */
void AddLbl(char *varname, unsigned int val, bool islbl, bool global, bool isdef)
{ int idx;
 if ((idx=LookupVar(varname))!=-1)
   {  Vari[idx].val=val;
      printf("Label %s redefined\n",varname); 
      } 
   else
    { strcpy(Vari[VariCnt].name,varname);
      Vari[VariCnt].val=val;
      Vari[VariCnt].islbl=true;
      Vari[VariCnt].isglob=global;
      Vari[VariCnt].isdef=isdef;
      VariCnt++;
      if (VariCnt==MAX_VARI)
          ErrorMsg("Variable space exhausted.",""); 
          }    
}

/*
unsigned int var(char *varname, bool define)
{ int i; bool fnd = false;

for (i=0;(i<VariCnt) && (fnd == false) ;i++) 
   fnd = (strcmp(varname,Vari[i].name)==0);
   
 if (fnd && (pass==1) && define)  
    ErrorMsg("Label already defined :",varname); 

 if (fnd) return Vari[--i].val;
  if ((fnd==false) && (pass==1) && define)  
   { strcpy(Vari[VariCnt].name,varname);
     Vari[VariCnt++].val=Pc;
     if (VariCnt==MAX_VARI)
          ErrorMsg("Variable space exhausted.","");     
    }
 if ((fnd==false) && (pass==2)) 
 	ErrorMsg("Unknown variable ",varname);
return 0;
}
*/

/* handle variables & labels  */
unsigned int ReadVar(char *varname)
{ short i; bool fnd = false;
  i=LookupVar(varname);
  if ((i==-1) && (Pass==2)) 
 	ErrorMsg("Unknown variable ",varname);   
  return Vari[i].val;
}

/*-----------------------------------------------------------*/
/*      intern / extern labels                               */
/*-----------------------------------------------------------*/

void ReadExtrnDef()
{ FILE *exlblf;
  char line[255], lbln[10];
  unsigned int lval;
  if ((exlblf  = fopen(globaLs ,"r"))==NULL) 
      ErrorMsg("Cannot open label def file ",globaLs);        
  while (fgets(line,ll,exlblf))
    {  sscanf(line,"%s %x",lbln,&lval);
       AddLbl(lbln, lval, true, true,true);
      }
  fclose(exlblf);    
  }

void WriteExtrnDef()
{ FILE *exlblf;
  char line[255], lbln[10];
  unsigned short i;
  for  (i=0;i<VariCnt;i++)
 	 if ((Vari[i].isglob) && ((Vari[i].isdef)==false)) 
	     ErrorMsg("Undefined global label",Vari[i].name);   
  if ((exlblf  = fopen(globaLs ,"w"))==NULL) 
      ErrorMsg("Cannot open label def file ",globaLs);        
  for  (i=0;i<VariCnt;i++)
          if (Vari[i].isglob) 
                fprintf(exlblf," %10s %4x\n",Vari[i].name,Vari[i].val); 
  fclose(exlblf); 
  }

/*-----------------------------------------------------------*/
/*          Input routines                                   */
/*-----------------------------------------------------------*/

/* read a constant or a parameter, potentially including + or - operators */
unsigned int reader(char *is)
{ unsigned int res, rht;
  unsigned short i, l, p;
  char *frst, *rst, opr;
  bool fnd=false;
 
 if (is==NULL)  ErrorMsg("Missing operand.",""); 
 res=0; rht=0; p=0;
  // Look for an operand 
 opr=' '; l = strlen(is);
 for (i=0;(i<l) && (p==0);i++)
      if ((is[i]=='+') || (is[i]=='-'))
          { opr=is[i]; is[i]=0; p=i; }
  // split if operand found.
 if (opr!=' ')     // an operand was found...
   { rst=is+p+1;   // pointer juggling, don't look too close...
     rht = reader(rst); // recursive call to reader
       }
  // now the actual parsing....     
  if (is[0]=='/')  { sscanf(is,"/%x",&res); fnd=true; }               // Hex in Philips notation
   else if (((is[1] | 0x20) == 'x') && (is[0]=='0'))
                     { sscanf(is,"%x",&res);fnd=true; } // Hex in C not.
   else if (isdigit(is[0])) { sscanf(is,"%d",&res); fnd=true; }       // Decimal
   else if (is[0]=='\'')    { res=is[1]; fnd=true; }                  // Single char. in  quotes
   else if (is[0]=='*')     { res=Pc; fnd=true; }                     // Pc
   else res=ReadVar(is);                                            // must be variable, look it up
   // Apply operators if needed
   if (opr=='-')  res=res-rht;
   if (opr=='+')  res=res+rht;
   return res;
 }

/* Write Intel Hex format output file */
void WriteIhx(unsigned int from, unsigned int to, bool seyon  ) 
{  FILE *ihx_file;
   char fname[40];
   unsigned short ihcnt, ihcrc, i;
   unsigned int wrd;
 
  sprintf(fname,"p800.ihx");
  if (ProjDefined)  sprintf(fname,"%s.ihx",ProjName);
  ihx_file = fopen(fname, "w");
  printf("Writing Intel Hex format file %s\n",fname); 
  if (seyon) fprintf(ihx_file,"transmit L\n");
  ihcnt=0;
  for (i=from;i<to;i=i+2)
	 { wrd=code[i/2];
           if (ihcnt==0) // start new line
               { if (seyon) fprintf(ihx_file,"transmit ");
                 fprintf(ihx_file,":%2.2X%4.4X00",ihx_len,i);
                 ihcrc=  ihx_len + (i&0xff)+(i>>8);
                 }
           fprintf(ihx_file,"%4.4X",wrd);
           ihcrc=ihcrc+(wrd&0xff)+(wrd>>8);   
           ihcnt=ihcnt+2;
           if (ihcnt==ihx_len)
              { 
                fprintf(ihx_file,"%2.2X\n",((~ihcrc)+1)&0xFF); 
                ihcnt=0;
                }
             }
    if (ihcnt!=ihx_len)
    	{
           while   (ihcnt<ihx_len)   
                { fprintf(ihx_file,"0000"); ihcnt=ihcnt+2; }  
           fprintf(ihx_file,"%2.2X\n",((~ihcrc)&0xFF)+1);
           } 
   if (seyon) fprintf(ihx_file,"transmit ");
    fprintf(ihx_file,":00000001FF\n"); // closing marker
    fclose(ihx_file);     
 }
 
/* Write H and L Roms 8K each */
void WriteRoms() 
{ FILE  *rom_hi, *rom_lo;  
  char fname[50];
  unsigned int i;
 sprintf(fname,"hi.rom");
 if (ProjDefined)  sprintf(fname,"%s_H.rom",ProjName);
 rom_hi  = fopen(fname, "wb");
 printf("Writing ROM images %s and ",fname);
 sprintf(fname,"lo.rom");
 if (ProjDefined)  sprintf(fname,"%s_L.rom",ProjName);
 rom_lo  = fopen(fname, "wb");
 printf("%s\n",fname);
 for (i=0;i<8192;i++)
         { fputc(code[i]>>8,  rom_hi);
           fputc(code[i]&0xFF,rom_lo);
         }
 fclose(rom_hi);
 fclose(rom_lo);
}
 
/* add a word to memory and augment listing */
void add_word(unsigned int wrd, bool first)
{ 
 if ((Pass==2) && List)
   {
     if (first) 
           printf("%5d  %4.4X %2.2X %2.2X  |  %s",LinCnt,Pc,wrd >>8, wrd &0xFF,line);
        else 
           printf("       %4.4X %2.2X %2.2X  |\n",Pc,wrd >>8, wrd &0xFF);
    }
  code[Pc>>1]=wrd; // code array is 32K words of 16 bit 
  Pc=Pc+2;        // increments of 2
 }
 
 
/* handle directives / assembler commands  */
void HandleCmd(int cmd, char *argu ) 
{ char *x; 
  unsigned short int val, i,  oddv, al, z;
  bool first;  // first byte of a word.
  bool finish; // we finished the current argument string.
  bool ok;     // am argument has been found
  bool odd;    // even / odd bytes in string
  char para[100], *lbls, k;
  
  sscanf(argu,"%s",para);
  switch (cmd ) 
    {
	case 0  :  sscanf(argu,"%s",ProjName); ProjDefined = true; break ;  // IDENT
	case 1  :  End=true; break; 				            // END
	case 14 :  End=true; break; 					    // END.	
	case 3  :  Pc=reader(para);
	           From=Pc;    
	           if ((Pass==2) && List) 
	                  printf(" -- %s",line);	               
	          break ;  // AORG
	          
         /* data statements need full scanning of input line   */
         case 8  : i=0; finish=false;   
                  first=true;  odd=false; ok=false;
                   al=strlen(argu);  
             //      printf(" DATA  %s\n",argu);
                   while (! finish)
                    { z=0; 
                      while ((i<al) && isspace(argu[i])) i++;     //skip leading space
                      if (i==al) 
                         if (ok) break; 
                            else  ErrorMsg("Missing DATA argument ","");    
                      if ((k=argu[i])==0x27)                      // start of string
                            { odd=false; first=true; 
                              i++;                              // skip leading ' char
                              while ((k=argu[i++])!=0x27)         //  end of string
                                   { if (odd==false) 
                                         { oddv=k; odd=true; } // first byte of argument 
                                         else
                                        { add_word((oddv<<8) | k,first); // add word
                                           first=false; odd=false; }
                                    }
                              if (odd)                            // End of string, add 0x20 if needed.
                                 { add_word((oddv<<8) | 0x20,first); // add word
                                   first=false;
                                 }
                             ok=true;    
                             continue;
                            }                                       // end of string handling
                                        
                      if  (k==',') {i++; continue;}                 // start of a new argument
                      while (!( (k==',')|| isspace(k) ))
                                 { k=argu[i]; para[z]=k; i++; z++;} // copy string to form parameter 
                      para[z-1]=0;                                  // terminate stringprintf("%c %d #%s#\n",k,isspace(k),para); 
               //       printf(" PARA  -%s-\n",para);
                      add_word(reader(para),first);                // evaluate it
                      ok=true;
                      finish=isspace(k);
                      }                      
	           break ; // DATA

	case 9  :  if (Pass==1) Vari[VariCnt-1].val=reader(para); break ;  // EQU : last label was an EQU
	case 11 :  if (Pass==2) printf("\n\n\n\nEJECT\n\n\n\n"); break ;        // EJECT
	case 12 :  List = true;  break ;  // List 
	case 13 :  List = false; break ;  // NList
	case 15 :  if ((inc_file  = fopen(para,"r"))==NULL)   // include file
                           { printf(" Error opening file `%s`\n",para), exit(1); }
                    printf("\n -- Now reading file `%s` --\n",para);
                    act_file=inc_file; 
                    LinCntSave=LinCnt; LinCnt=0; // reset linecounter for new file
                    break;
	case 16 :   fclose(act_file ); act_file=asm_ipf;    // EOF    : stop reading include file
	            printf(" -- return to main file \n");
	            LinCnt=LinCntSave;  // recall linepointer
                    break;
	case 2  : if (Pass==1) // add ENTRY's, as unknown externals.
		    { HasGlobals=true; 
		      lbls=strtok(para,","); 
	              while (lbls!=NULL)
	                { AddLbl(lbls, 0, true, true, false);
	                  lbls=strtok(NULL,",");
	                 }
	            }
	          break; // ENTRY
	case 17 : if (Pass==1) // check if EXTRN are defined.
		    { lbls=strtok(para,","); 
	    	      while (lbls!=NULL) 
	               {  if (LookupVar(lbls)==-1)
	                       ErrorMsg("Undefined EXTRN ",lbls); 
	                 lbls=strtok(NULL,",");
	                  }
	             }	                  
	          break; //  EXTRN
	case 10 :  Pc = Pc + reader(para)*2;
	           if ((Pass==2) && List) 
	                  printf(" -- %s",line);
	           break;// RES
	case 4  :   // RORG
	case 5  :   // IFF
	case 6  :   // IFT  
	case 7  :   // XIF
	    ErrorMsg("Unsupported command ",""); break ;  // END.
     }
}

void RangeCheck(unsigned int x, unsigned int max)
{
//printf("%d %d\n",x,max);
if ((x>max) & (Pass==2))
   ErrorMsg("Branch outside range",""); 
}

/* handle one P850 instruction */
void HandleMnemo(int mnemo,char *argu, bool star, unsigned short cc)
{  char tpl[4], *x; 
  unsigned int instr, cnt, i, rv;
  unsigned int para; // parameter to be added to instruction
  unsigned int sec;  // second word of instruction

// lookup nr of words in instruction
cnt  = mnem_table[mnemo].cnt;
// lookup basic instruction
instr   = mnem_table[mnemo].val;
// set bit 10 if starred 
if (star) instr=instr | 0x20;
// handle arguments
para= 0;
sec = 0;
strcpy(tpl,mnem_table[mnemo].tpe);
x=strtok(argu,",");
// Parse arguments as defined in mnemonis table
for (i=0;i<3;i++)
  {
   switch(tpl[i])
  	 {
         case 'R'  : para=LookupReg(x)<<7;   break;        // r1
  	 case 'T'  : para=LookupReg(x)<<1;   break;        // r2
  	 case 'U'  : if (x!=NULL) para=LookupReg(x)<<1; break; // optional r2  	
  	 case 'P'  : rv=LookupReg(x);
  	  	     if (rv&0x01) ErrorMsg("Register outside range 1--7",x);
  	  	     rv=rv>>1;
  	  	     para=rv<<8;   break;                   // r3
  	 case 'M'  : sec  = reader(x);              break;  // Memory address
  	 case 'N'  : para = (reader(x) & 0X0F) <<7; break;  // 4 bit number
  	 case 'S'  : para = (reader(x) & 0X01F);    break;  // number of bits to shift
  	 case 'X'  : para = reader(x) & 0x3F;       break;  // Device IO address
  	 case 'K'  : para = reader(x) & 0xFF;       break;  // 8 bit constant
  	 case 'L'  : sec  = reader(x);              break;  //16 bit word
  	 case 'F'  : para = (reader(x) & 0x01)<<6;  break;  // flag 0/1
  	 case 'C'  : para = cc << 8;                break;  // Condition code
 	 case 'G'  : para = ((reader(x) - Pc-2) & 0xFFFE); RangeCheck(para,252); break; // forward relative
 	 case 'H'  : para = ((Pc - reader(x)+2) & 0xFFFE); RangeCheck(para,252); break; // backward relative
  	 case ' '  : break;                               // NO-OP
  	 default   : ErrorMsg("Undefind operand descriptor",tpl); 
  	 } 
     x=strtok(NULL,",");
     instr = instr | para;	  
  }
  add_word(instr,true);
  if (cnt==2)  add_word(sec,false);
}
 
void show_help()
 {
 printf("Usage : p800asm -if <inputfile> [-rom] [-ihf] [-help] \n");
 printf("        [-rom]....create a set of ROM images\n");
 printf("        [-ihf]....create Intel Hex file\n");
 printf("        [-ihs]....create Intel Hex as Seyon script\n");
 printf("        [-log]....echo ASM input on screen\n");
 printf("        [-help]...this help text \n");
 } 
 
/*-----------------------------------------------------------*/
/*          Main routine                                     */
/*-----------------------------------------------------------*/
 
int main(int argc,char *argv[])
{  char fname[50];
   char cpy[ll];
   char instr[20], argu[50], lbl[20], cond[20], mne[20];
   unsigned int  mcpc=0,  log=0, cmd, mnemo, pos, i, idx;
   unsigned short cc; // Condition code
   bool star, MakeRom=false, MakeIhf=false, Seyon=false,  Log=false;

 i=0;
 if (argc==1) show_help();
  while (i<argc)
     {
     if (strcmp (argv[i], "-if")==0) 
               sscanf(argv[++i],"%s",fname);  
     if (strcmp (argv[i], "-rom")==0) MakeRom=true;
     if (strcmp (argv[i], "-ihf")==0) MakeIhf=true;
     if (strcmp (argv[i], "-ihx")==0)  { MakeIhf=true; Seyon=true; } 
     if (strcmp (argv[i], "-log")==0) Log=true;
     if (strcmp (argv[i], "-help")==0) show_help();
     i++;
     }

// Read file witch external globals if it exists
if (access(globaLs, F_OK) == 0) 
     { HasGlobals=true; 
       ReadExtrnDef();
        }

if ((asm_ipf  = fopen(fname,"r"))==NULL) 
   { printf(" Error opening file `%s`\n",fname), exit(1); }
   else
   {
    act_file=asm_ipf; // set active input file
   /* set  the code buffer to all ones */
   for (Pc=0;Pc<hwm;Pc++) code[Pc]=0xFFFF;
   
   /*  two passes of the input file(s)  */
  for (Pass=1; Pass<3; Pass++)
   {
 	printf("\n-----------------\n---- PASS %d ----\n----------------\n", Pass);
 	printf("\n -  Reading input file `%s`\n\n",fname);
 	rewind(act_file); 
 	LinCnt=0; Pc=0; List=true; End=false;
	
	 while (fgets(line,ll,act_file) && (End==false))
	 { LinCnt++; 
	   if (Log) printf("%4d %s",LinCnt,line);
	   if (strlen(line)==cls(line))   continue;  // this was an empty line  
	   if (line[0]=='*')     
	   	{ if ((Pass==2) && List)  printf("                   | %s", line);
	   	  continue;  /* Full line comment */
	   	  }
	   	  
	  /*  Check for a label  	*/   
	   if (isalpha(line[0]))                   // found a label 
	         { sscanf(line,"%s",lbl);
	           if (Pass==1)      // labels are defined in first pass
	            { if ((idx=LookupVar(lbl))==-1) 
	           	 AddLbl(lbl,Pc,true,false,true); // new internal label
	               else 
	                   { Vari[idx].val=Pc;     // must be an INTERN, set value
	                    Vari[idx].isdef=true;  // set as defined
	                   }
	           }
	           if ((Pass==2) && List)
	           	 printf("-- %8s %4.4X --|\n", lbl,Pc);
	           reduce(line,strlen(lbl));    // remove label from line
	           };
	           
	   if (strlen(line)==cls(line))   continue;   /* line has only a label, otherwise empty */ 
	
	   sscanf(line,"%s %s",instr, argu);   /* get command & argument  */  
	   reduce(line,cls(line));             /*  get rid of leading space */ 
	     
	   /* Is it a directive ? */
	   if ((cmd=LookupCmd(instr))!=-1) 
	  	  {   strcpy(cpy,line);           /* copy the line */
	  	      reduce(cpy,strlen(instr));  /* get rid of command string */
	  	      HandleCmd(cmd,cpy);        
	  	      continue; }
 
	   /* now it would have to be a mnemonic */			  
	   star=(instr[strlen(instr)-1]=='*');   /* Check for * at the end */
	   if (star) instr[strlen(instr)-1]=0;   /* remove if found */
	
	   /* Check for conditionals / presence of a "(" */ 
	   pos = 0; 
	   cc  = 7;  // default condition code
	   for (i=0;i<strlen(instr);i++)
	       if (instr[i]=='(') pos=i; 
	   if (pos!=0)  // found a CC
	      { instr[pos]=' '; 
	        sscanf(instr,"%s %s %s",mne, cond, argu);
	        cc=LookupCc(cond); // xlate CC to a number
	        instr[pos]=0; // remove CC from mnemonic string by setting an eol 0
	       }
     
       	   /* Now look up mnemonic */
	   if ((mnemo=LookupMnemo(instr))!=0) 
  		{ HandleMnemo(mnemo, argu, star, cc);   		
  		  continue; } 
   
	  /* Check if parsed correctly, normally should never reach here */
	    ErrorMsg("Could not parse ",line);
	    exit(0); 
	    }
	 if (act_file!=asm_ipf) ErrorMsg("Missing 'EOF' in include file ","");
	 if (Pass==1)  ListVariab();
	}

  printf("\nassembly done..\n");

  fclose(asm_ipf);
  
  if (HasGlobals) WriteExtrnDef();  // (re)write file with externals
  
  if (MakeIhf) WriteIhx(From, Pc, Seyon);  // Intel Hex file output
  
  if (MakeRom) WriteRoms();         // Hi and Lo Rom images

  }
}
