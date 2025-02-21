	NLIST
* =============================================	
*       R S 2 3 2   I N P U T 
* input from serial terminal. Uses A5 & A7
* read hex input, return a word, byte or nibble in A5.
* Results are shifted in right to left. 
* [A5,A7]
*================================================
SERWRD  CF	A14,SERBYT		
SERBYT	CF	A14,SERNIB 
SERNIB 	CF	A14,SERIN	* read a hex nibble
        CWK 	A7,'0'	
	RB(L) 	SERNIB		Smaller than '0', go for next byte	
        CWK 	A7,'F'	
	RB(G)	SERNIB		Greater than 'F', also ignore
        CWK 	A7,'A'	
	RF(L) 	CHR1		Smaller than 'A',
	SUK 	A7,7  
CHR1	ANK 	A7,/0F          isolate nibble
	SLL 	A5,4		Shift left 4 places
	ORR 	A5,A7		OR with previous contents
	RTN 	A14
*
* receive a character in A7 on RS232 chan 0
* [A7]
SERIN	INR 	A7,0,SR
	ANK	A7,/01
	RB(Z)	SERIN	        loop until RX not empty
	INR	A7,0,RHR
*	CF	A14,SEROUT      echo character read
	ANKL 	A7,/00FF	Mask out upper bits. These are invalid as the RS232 channel is only 8 bit wide.	
	RTN 	A14	
*	
* =============================================	
*     R S 2 3 2   O U T P U T 
* Textoutput on serial terminal. Uses A4 & A7
* Text is stored after initial subroutine call.
* Stack is updated to point to first instruction after call
* [A4,A7]
*================================================
TXTOUT  LDR     A4,A14          Copy system stack pointer  
	ADK     A4,4	        Find pointer to first text
	LDR*    A4,A4	        A13 now points to ASCII string
TXTR    LDR*    A7,A4		Load 2 characters
        ADK     A4,2		increment pointer
        SLC     A7,8		Swap bytes
        RF(Z)   TXTEX		if char=0 then exit
    	CF      A14,SEROUT	Output first byte
        SRL     A7,8	        Shift bytes
        RF(Z)   TXTEX		if char=0 then exit
     	CF      A14,SEROUT	Output second byte       
        RB(7)   TXTR	        Next character
TXTEX	ST      A4,/0004,A14    restore pointer
        RTN     A14		Exit from textroutine
*
* CR/LF on serial channel #1
* [A6,A7]
CRLF    LDK	A7,/0D          Load CR
	CF 	A14,SEROUT
        LDK	A7,/0A          Load LF
	CF 	A14,SEROUT
	RTN	A14
*
*---- Display word in A5 in hex on serial #1----
* [A5,A6,A7]
WRDOUT 	LDK	A7,/020         Load space
	CF 	A14,SEROUT
        SLC     A5,4            Rotate left 4 bit
	CF	A14,NIBBLE      output one nibble
	SLC     A5,4
	CF	A14,NIBBLE
        SLC     A5,4
	CF	A14,NIBBLE
        SLC     A5,4        
NIBBLE 	LDR 	A7,A5
	ANK	A7,/0F
	ORK	A7,/30
	CWK	A7,/003A
	RF(L)   SEROUT
	ADK	A7,/07 		Continues below !
* send a character in A7 to  RS232 chan 0
* [A6,A7]
SEROUT	INR 	A6,0,SR
	ANK	A6,/08
	RB(Z)	SEROUT	        loop until TX-empty
	OTR	A7,0,THR
	RTN 	A14
*
* =============================================	
*     R S 2 3 2    I N I T 
* initialize the RS232 chip to 9600 8n1
* [A7]
* ===========================================
SERSET 	LDK 	A7,/0
	OTR 	A7,0,IR	      disable all IRQ's
	LDK 	A7,/53	      Set to 8N1
	OTR 	A7,0,MR	      write mode register 1
	LDK 	A7,/08           
	OTR 	A7,0,MR	      write mode register 2
	LDK 	A7,/89		
	OTR 	A7,0,ACR      aux control
	LDK 	A7,/CC	      select 19200 baud
*	LDK 	A7,/BB	      select 9600 baud
	OTR 	A7,0,CSR      
	LDK 	A7,/05		
	OTR 	A7,0,CR       set rx & tx active
	RTN 	A14
*===================================================
*
	LIST
	EOF
