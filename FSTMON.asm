* -----------------------------------------------------------
*
*  Simple software monitor to support operating my
*  SPC16/10 "FAST" microprocessor SBC.
*
*  jos.dreesen@greenmail.ch 2024
*  
*
*   Todo : split in smaller pieces    
*
* -----------------------------------------------------------
*
	IDENT FSTMON
	ENTRY TXTOUT,CRLF
	ENTRY SERSET,SERIN,SEROUT
	ENTRY SERWRD,SERBYT,SERNIB
* 
*
* Firmware memory map :
*-----------------------
* 0000-7FFF : 16K EPROM
* 8000-BFFF : unused
* C000-FFFF : 8K SRAM
*
* read external file with IO-definitions
	RDSRC io.def
* read external file with interrupt-definitions
	RDSRC intr.def
*
	LIST
	AORG 	/FD00
WRKA	EQU 	/FD00		Working area for monitor
	RES  	16 		Registers 0 to 15
	
IH_SA   DATA 0			Intel Hex Startaddress
IH_CNT	DATA 0			Intel Hex Counter
IH_PTR	DATA 0			Intel Hex Pointer
IH_CRC  DATA 0			Intel Hex CRC
IH_TOT	DATA 0			Intel Hex Total bytes

WORD    DATA 0			hex input in progress
MPTR	DATA 0			Monitor pointer
MSA     DATA 0			Monitor start address
	AORG 	/0200
*
* Dummy interrupt routines.
LKMVEC	LDK A7,'L'
	OTR A7,1,dsp_E
	HLT
INT2VC	LDK A7,'2'
	OTR A7,1,dsp_E
	HLT
INT3VC	LDK A7,'3'
	OTR A7,1,dsp_E
	HLT

*  Read external RS232 driver & IO routines 
	RDSRC	rs232.asm	
	
*================================================        
*============  "FAST"-chip monitor  =============
*================================================
	LIST
PWRON	INH			No interrupts (yet...)
	LDKL 	A15,/FE80	Set system stackpointer
	LDKL 	A14,/FE00	Set stackpointer
	LDKL	A7,/4641	Load LED boot message
	LDKL	A8,/5354
	LDKL	A9,/204D
	LDKL	A10,/4F4E	
	CF	A14,LEDDSP
	CF 	A14,SERSET	Setup serial channel to 9600/n/2 
	CF  	A14,HELP
*
* =======   Monitor main loop   =================
*	
*       A7 contains characters
*       A6 is working register
*	MPTR and WORD are pointer and input in progress
	ANK  	A7,0		
	ST   	A7,MPTR		Reset pointer
	ST   	A7,WORD		Reset word	
MAIN	LDK  	A7,'>'
	CF   	A14,SEROUT	Display prompt
MAINNP	CF   	A14,SERIN	Get a character from the serial channel	
	CWK  	A7,/060	        Lower case ?
	RF(L) 	NOLC		NO..
	ANKL 	A7,/DF	        Yes, shift to uppercase
NOLC 	CF   	A14,SEROUT	Echo the input
*---- show registers
	CWK  A7,'R'		'R' show registers
        RF(NE)  M1
	CF	A14,REGDMP
	RB(7)  	MAIN
*------show memory
M1      CWK    	A7,'M'		'M' show memory page
        RF(NE) 	M2
	CF     	A14,MEMDMP
	RB(7)  	MAIN
*------show next page
M2      CWK    	A7,/2B		'+' show next nemory page
        RF(NE) 	M3
        LDKL	A7,/100
        ADS   	A7,MPTR
	CF     	A14,MEMDMP
	RB(7)  	MAIN
*------show previous page
M3      CWK    	A7,/2D		'-' show previous memory page
        RF(NE) 	M4
        LD	A7,MPTR
        SUKL   	A7,/100
        ST	A7,MPTR
	CF     	A14,MEMDMP
	RB(7)  	MAIN
*------halt	
M4      CWK A7,'S'		'S' Halt/stop machine
        RF(NE) 	M5
        HLT
*------execute instruction	
M5      CWK     A7,'X'		'X' : execute instruction
        RF(NE)  M6
        LD      A7,WORD
	EXR     A7
 	RB(7)   MAIN     
*------load intel hex file	
M6      CWK     A7,'L'		'L' : load intel hex file
        RF(NE)  M7
        CF      A14,LOADHEX
 	RB(7)   MAIN     
*------reset
M7      CWK     A7,'Z'		'Z' : reset
        RF(NE)  M8
        ABL	PWRON
 	RB(7)   MAIN     
*------help	
M8      CWK     A7,'H'		Is it 'H' help
        RF(NE)  M9
        CF      A14,HELP
 	RB(7)   MAIN     
*------help	
M9      CWK     A7,'?'		Is it '?' help
        RF(NE)  M10
        CF      A14,HELP 	
 	RB(7)   MAIN
*------go
M10     CWK     A7,'G'		Is it 'G' go
        RF(NE)  M11
        LD	A7,MPTR
        CFR	A14,A7         Subroutine call to MPTR
 	RB(7)   MAIN
*-------set pointer	
M11     CWK     A7,'P'	        Set the pointer
        RF(NE)  M12
        CF      A14,SETPTR
        RB(7)   MAIN	
*-------write value into memory	
M12     CWK     A7,'W'		write into memory
        RF(NE)  M13
        CF      A14,WRMEM
   	RB(7)   MAIN	      
*-----check for hex input 
M13     CWK     A7,'0'	
	RF(L)   UNKNOW		Smaller than '0', ignore
        CWK     A7,'F'	
	RF(G)   UNKNOW		Greater than 'F', ignore
        CWK     A7,'A'	
	RF(L)   CHR		Smaller than 'A'
	SUK     A7,7  
CHR	LD      A6,WORD
	SLL     A6,4		Shift left 4 places
	ANK     A7,/0F
	ORR     A6,A7		OR with previous contents
	ST	A6,WORD
	RB(7)   MAINNP
UNKNOW  LDK     A7,'?'
	CF      A14,SEROUT	Display prompt
	RB(7)   MAIN	
*	
* =======  End of main monitor loop   =================	
*
* monitor : set pointer
SETPTR  CF	A14,TXTOUT
        DATA	/0A0D,' PTR <-',0
        LD 	A5,WORD
        ST 	A5,MPTR
        CF      A14,WRDOUT
        CF      A14,CRLF
        RTN 	A14 
* monitor : write into memory
WRMEM   CF      A14,CRLF
	LD	A5,MPTR        
        CF      A14,WRDOUT
        CF      A14,TXTOUT
        DATA 	' <-',0        
        LD	A5,WORD
        CF      A14,WRDOUT
        CF      A14,CRLF
        LD	A5,WORD
	ST*	A5,MPTR
	LDK	A5,2
   	ADS	A5,MPTR		increment pointer by 2
        RTN 	A14
* -----------------------------------------------
*
* ======================================
*  Help text 
* ======================================
	NLIST
HELP  	CF A14,TXTOUT
	DATA /0A0D,'=SPC16/10 Monitor=',/0A0D,0
	CF A14,TXTOUT
	DATA '=====================',/0A0D,0
	CF A14,TXTOUT
	DATA 'P)....Set pointer',/0A0D,0
	CF A14,TXTOUT
	DATA 'W)....Write into mem',/0A0D,0
 	CF A14,TXTOUT
	DATA 'R)....Show registers',/0A0D,0
 	CF A14,TXTOUT
	DATA 'M)....Show memory page',/0A0D,0
 	CF A14,TXTOUT
	DATA '+)....Show next page',/0A0D,0
 	CF A14,TXTOUT
	DATA '-)....Show previous page',/0A0D,0
 	CF A14,TXTOUT
	DATA 'X)....Execute instruction',/0A0D,0
 	CF A14,TXTOUT
	DATA 'L)....Load Intel hex',/0A0D,0
 	CF A14,TXTOUT
	DATA 'S)....Halt machine',/0A0D,0
 	CF A14,TXTOUT
	DATA 'G)....Go : jump to pointer',/0A0D,0
	CF A14,TXTOUT
	DATA 'H)....Show help text',/0A0D,0
        RTN A14
        LIST
* -----------------------------------------------
*
*================================================
* Load Intel hex file from serial line
* [A4,A5,A6,A7]
*================================================
LOADHEX CF	A14,TXTOUT
	DATA 	'Reading Intel Hex input..',/0A0D,00
	CM	IH_TOT          clear total count
LDH	CF      A14,SERIN	Get a character from the serial channel	
        CWK     A7,':'	        Is it ':' start of string ?
        RB(NE)  LDH
*  --- Get bytecount ---      
        CF      A14,SERBYT	Get bytecount
        ANK	A5,/FF
        ST      A5,IH_CRC	Start CRC count	
        ST      A5,IH_CNT	set Intel Hex Counter
        ADS	A5,IH_TOT	update total number of bytes
*  --- Get intel hex pointer ---   
 	CF	A14,SERBYT	High byte
 	ADS	A5,IH_CRC	Add to CRC count
 	CF	A14,SERBYT      Low byte   
 	ADS	A5,IH_CRC       Add to CRC count
        ST	A5,IH_PTR	set Intel Hex Pointer        
*  --- Get record type ---           
	CF      A14,SERBYT  	Get hex record type ( 0=data, 1=EOF, 3=startaddress)
	ANK	A5,/FF		Reset high byte
 	ADS	A5,IH_CRC       Add to CRC count
        CWK     A5,00 		-> data statement ?
	RF(EQ)  DATHEX 
        CWK     A5,/01 		->  EOF marker ?
	RF(EQ)  EOFHEX
	CWK     A5,/03          -> start address ?
        RF(EQ)  STRHEX 
* not a known recordtype !      
ERRHEX  CF A14,TXTOUT	
	DATA ' Rec. typ ?',/0A0D,0   
*       RTN A14        		finish
*  --- Get start address ---    	       
STRHEX  CF	A14,SERBYT
 	ADS	A5,IH_CRC
 	CF	A14,SERBYT
 	ADS	A5,IH_CRC
        ST	A5,MSA		...set monitor start address  
        ST	A5,MPTR		...set pointer
        CF	A14,SERBYT	Get checksum
        AD	A5,IH_CRC	calculate checksum
        ANK	A5,/FF		Mask off high byte
        RF(NZ)	CRCERR
        RB(7)	LOADHEX		Next line
*  --- Get data bytes  ---       
DATHEX  LDK	A4,0		Bytecounter to zero
	LD	A5,IH_PTR	Display address
      	CF	A14,WRDOUT  
      	LDK     A7,'<'		display marker
	CF     A14,SEROUT
NDATHX  CF	A14,SERBYT      Start of loop	
 	ADS	A5,IH_CRC
 	CF	A14,SERBYT
 	ADS	A5,IH_CRC
   	ST*	A5,IH_PTR	Store word
   	CF	A14,WRDOUT      Display word
   	LDK	A5,2
	ADS	A5,IH_PTR	Update pointer
   	ADK     A4,2
	CW	A4,IH_CNT	All databytes had ?
        RB(NE)  NDATHX		Next word 
        CF	A14,SERBYT	Get checksum
        ADS	A5,IH_CRC
	LD	A5,IH_CRC
	ANK	A5,/FF	        Mask off high byte
        RF(NZ)	CRCERR
     	CF 	A14,TXTOUT
	DATA   ' OK',/0A0D,0
        RB(7)	LDH		Next line        
*  --- intel hex error msg ---   
CRCERR  CF      A14,TXTOUT
	DATA    ' *CRC*',/0A0D,0   
	RB(7)	LDH		Next line         
*  --- process EOF statement ---          
EOFHEX  CF	A14,SERBYT	Get checksum
        AD	A5,IH_CRC	calculate checksum
        ANK	A5,/FF	        Mask off high byte
        RB(NZ)  CRCERR  
        LD	A5,IH_TOT
        CF	A14,WRDOUT
        CF	A14,TXTOUT
        DATA   '(h) bytes read',/0A0D,0       
        RTN	A14       	finished reading, return.
* -----------------------------------------------
*
*================================================
*  Memory dump  on the serial terminal
*  A4 is loopvariable
*  [A4,A5,A6,A7]
*================================================	
MEMDMP   CF     A14,CRLF
	 LD     A4,MPTR		Load startaddress
	 ANKL   A4,/FF00	Align on 128 word boundary
MLOOP1	 LDR    A5,A4		get address
         CF     A14,WRDOUT	print it
	 LDK    A7,/20		print space 
	 CF     A14,SEROUT	
	 LDK    A7,'|'		print divider
	 CF     A14,SEROUT	
MLOOP2	 LDR*   A5,A4		Load word
	 ADK    A4,2	        Point to next word
	 CF     A14,WRDOUT
	 LDR    A7,A4
	 ANK    A7,/0F		Isolate lower 4 bits
	 RB(NZ)	MLOOP2
	 LDK    A7,/20		print space
	 CF     A14,SEROUT	
	 LDK    A7,'|'		print divider
	 CF     A14,SEROUT
*------ repeat for ascii printout ------	 
	 SUK    A4,/10	        reset counter to start of line
MLOOP3   LDR*   A5,A4		Load word
	 ADK    A4,2	        Point to next word
	 ECR 	A7,A5	
	 CF     A14,MASCI
	 LDR 	A7,A5			
	 CF     A14,MASCI
	 LDR    A7,A4
	 ANKL   A7,/000F	Isolate lower 4 bits
	 RB(NZ)	MLOOP3	 	
	 CF     A14,CRLF
	 LDR    A7,A4
	 ANK    A7,/00F0	Isolate middle 4 bits
	 RB(NZ)	MLOOP1
	 RTN    A14		128 words/256 bytes have been printed
MASCI	 ANK    A7,/7F	
	 CWK	A7,/7F          DEL ?
	 RF(EQ) MPT
	 CWK	A7,/1F		if ASCII > 1F then printable
	 RF(G)  MASY
MPT	 LDK    A7,'.'		else print a .
MASY	 CF     A14,SEROUT
	 RTN    A14
* -----------------------------------------------
*
*================================================
* Dump the registers on the serial terminal
* [A3,A4,A5,A6,A7]
*================================================
REGDMP  MS  	15,WRKA		Store all registers in mem
	LDK	A3,0
	CF	A14,CRLF
	LDKL    A4,/3031
RLOOP	LDK     A7,'A'		
	CF      A14,SEROUT
	ECR	A7,A4
	CF      A14,SEROUT
	LDR	A7,A4
	CF      A14,SEROUT	
	LDK     A7,'='
	CF      A14,SEROUT
	LD	A5,WRKA,A3
	CF	A14,WRDOUT
	LDK     A7,/20		
	CF      A14,SEROUT
	ADK	A3,2
	ADK	A4,1
	CWK	A4,/303A
	RF(NE)  NOOV
	LDKL    A4,/3130
NOOV	CWK	A3,/000A
	RF(NE)  RLOP1
	CF	A14,CRLF
RLOP1	CWK	A3,/0020
	RF(NE)  RLOP2
RLOP2	CWK	A3,/0014	
	RF(NE)  RLOP3
	CF	A14,CRLF
RLOP3   CWK	A3,/001E
	RB(NE)  RLOOP
	CF	A14,CRLF
 	RTN     A14
* -----------------------------------------------
*
* =============================================	
* Textoutput on LED Display 
* Text is stored in A7..A10
* =============================================
LEDDSP	OTR	A7,1,dsp_B	
	ECR 	A7,A7
	OTR 	A7,1,dsp_A
	LDR	A7,A8
	OTR	A7,1,dsp_D	
	ECR 	A7,A7
	OTR 	A7,1,dsp_C
	LDR	A7,A9
	OTR	A7,1,dsp_F	
	ECR 	A7,A7
	OTR 	A7,1,dsp_E
	LDR	A7,A10
	OTR	A7,1,dsp_H	
	ECR 	A7,A7
	OTR 	A7,1,dsp_G
	RTN	A14	

	END
