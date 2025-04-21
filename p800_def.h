/*
===== parameter types ===============

r1  = R  register	 [b5..b8]
r2  = T  register	 [b11..b14]
r2  = U  possible register r2
r3  = P  registe r1..r7	 [b5..b7]
m   = M  mem adr   	 [second word]
n   = N  number 4 bit    [b5..b8]
s   = S  bits to shift    [b11..b15]
dev = X  io device 6 bit [b10..b15]
f   = F  flag 1/0 	 [b9]
k   = K  constant  8 bit [b8..b15]
lk  = L  long constant  16 bit
cc  = C  condition code   [b5..b7]
      G  forward relative address
      H  backward relative address
===================================

Several instructions share the same basecode.
Differentiation is done via parameter restrictions.
These are not yet checked in this assembler.
*/

#define NR_MNEM 163
struct mnem  { char name[6]; int val; char tpe[4]; short cnt; } mnem_table [NR_MNEM] = {
{"###",  0x000, "   ", 0 },  /* dummy*/
// Load / Store
{"LD",   0x8040,  "RMU", 2}, 
{"LDA",  0xF000,  "RTM", 2}, 
{"LDR",  0x8000,  "RT ", 1},
{"LDK",  0x0000,  "PK ", 1},
{"LDKL", 0x8020,  "RL ", 2}, 
{"ST",   0x8041,  "RMU", 2},
{"STR",  0x8021,  "RT ", 1},
{"ML",   0xB840,  "NMU", 2},
{"MLK",  0xB820,  "N  ", 0},
{"MLR",  0xB820,  "NT ", 1},
{"MS",   0xB841,  "NMU", 2},
{"MSR",  0xB821,  "NT ", 1},
// Arithmetic
{"ADK",  0x1000,  "PK ", 1}, 
{"ADKL", 0x9020,  "RL ", 2},
{"ADR",  0x9000,  "RT ", 1}, 
{"ADRS", 0x9021,  "RT ", 1}, 
{"AD",   0x9040,  "RMU", 2},
{"ADS",  0x9041,  "RMU", 2},
{"IMR",  0x9021,  "T  ", 1},
{"IM",   0x9041,  "MU ", 2}, 
{"SUK",  0x1800,  "PK ", 1},
{"SUKL", 0x9820,  "RL ", 2},
{"SUR",  0x9800,  "RT ", 1}, 
{"SURS", 0x9821,  "RT ", 1}, 
{"SU",   0x9840,  "RMU", 2},
{"SUS",  0x9841,  "RMU", 2},
{"CWK",  0xE820,  "RL ", 2},
{"CWR",  0xE800,  "RT ", 1}, 
{"CW",   0xE840,  "RMU", 2},
{"C1",   0xF840,  "RMU", 2},
{"C1S",  0xF841,  "MU ", 2}, 
{"C1R",  0xF800,  "RT ", 1},
{"C1RS", 0xF821,  "T  ", 1},
{"NGR",  0x9801,  "RT ", 1},
{"C2R",  0x9821,  "T  ", 1},
{"C2",   0x9841,  "MT ", 2}, 
{"CMR",  0xA021,  "T  ", 1},
{"CM",   0xA041,  "MU ", 2},
{"MUK",  0xC020,  "L  ", 2},
{"MUR",  0xC000,  "T  ", 1},
{"MU",   0xC040,  "MU ", 2}, 
{"DVK",  0xC820,  "L  ", 2},
{"DVR",  0xC800,  "T  ", 1},
{"DV",   0xC840,  "MU ", 2}, 
{"DAK",  0xD020,  "LL ", 3},
{"DAR",  0xD000,  "T  ", 1},
{"DA",   0xD040,  "MU ", 2}, 
{"DSK",  0xD820,  "LL ", 3},
{"DSR",  0xD800,  "T  ", 1},
{"DS",   0xD840,  "MU ", 2}, 
// Logical
{"ANK",  0x2000,  "PK ", 1}, 
{"ANKL", 0xA020,  "RL ", 2},
{"ANR",  0xA000,  "RT ", 1},
{"ANRS", 0xA021,  "RT ", 1},
{"AN",   0xA040,  "RMU", 2},
{"ANS",  0xA041,  "RMU", 2},
{"ORK",  0x2800,  "PK ", 1},
{"ORKL", 0xA820,  "RL ", 2},
{"ORR",  0xA800,  "RT ", 1},
{"ORRS", 0xA821,  "RT ", 1},
{"OR",   0xA840,  "RMU", 2},
{"ORS",  0xA841,  "RMU", 2},
{"XRK",  0x3000,  "PK ", 1},
{"XRKL", 0xB020,  "RL ", 2},
{"XR",   0xB040,  "RMU", 2},
{"XRS",  0xB041,  "RMU", 2},
{"XRR",  0xB000,  "RT ", 1},
{"XRRS", 0xB021,  "RT ", 1},
{"TM",   0xA001,  "RT ", 1},
{"TNM",  0xB001,  "RT ", 1},
// Character
{"ECR",  0xE000,  "RT ", 1},
{"LCK",  0xE020,  "RL ", 2},
{"LCR",  0xE020,  "RT ", 1},
{"LC",   0xE040,  "RMU", 2},
{"SCR",  0xE021,  "RT ", 1},
{"SC",   0xE041,  "RMU", 2},
{"CCK",  0xE821,  "RL ", 2},
{"CCR",  0xE821,  "RT ", 1},
{"CC",   0xE841,  "RMU", 2},
// Branches
{"AB",   0x0800,  "K C", 1},
{"ABL",  0x8820,  "L C", 2}, 
{"ABR",  0x8800,  "T C", 1},
{"ABI",  0x8840,  "L C", 2}, 
{"RF",   0x5000,  "G C", 1},
{"RB",   0x5800,  "H C", 1},
{"CF",   0xF021,  "RM ", 2}, 
{"CFR",  0xF001,  "RT ", 1},
{"CFI",  0xF041,  "RMU", 2}, 
{"RTN",  0xF020,  "T  ", 1},
{"RTF",  0xF040,  "T  ", 1}, /* ERROR : unknown code, best guess! */
{"EXR",  0xF001,  "T  ", 1},
{"EXK",  0xF021,  "L  ", 2}, 
{"EX",   0xF041,  "L  ", 2}, 
// Shifts'
{"SLA",  0x3800,  "PS ", 1},
{"SRA",  0x3820,  "PS ", 1},
{"SLL",  0x3840,  "PS ", 1},
{"SRL",  0x3860,  "PS ", 1},
{"SLC",  0x38C0,  "PS ", 1},
{"SRC",  0x38E0,  "PS ", 1},
{"SLN",  0x3880,  "PT ", 1},
{"SRN",  0x38A0,  "PT ", 1},
{"DLA",  0x3800,  "S  ", 1},
{"DRA",  0x3820,  "S  ", 1},
{"DLL",  0x3840,  "S  ", 1},
{"DRL",  0x3860,  "S  ", 1},
{"DLC",  0x38C0,  "S  ", 1},
{"DRC",  0x38E0,  "S  ", 1},
{"DLN",  0x3880,  "T  ", 1},
{"DRN",  0x38A0,  "T  ", 1},
// Control
{"HLT",  0x207F,  "   ", 1},
{"INH",  0x20BF,  "   ", 1},
{"ENB",  0x2840,  "   ", 1},
{"LKM",  0x2804,  "   ", 1},
{"RIT",  0x20C0,  "   ", 1},
{"SMD",  0x2801,  "   ", 1},
// IO & extxfer
{"CIO",  0x4080,  "PFX", 1 }, 
{"INR",  0x4800,  "PFX", 1 }, 
{"OTR",  0x4000,  "PFX", 1 }, 
{"SST",  0x48C0,  "PX ", 1 }, 
{"TST",  0x4880,  "PX ", 1 }, 
{"WER",  0x7000,  "PK ", 1 }, 
{"RER",  0x7800,  "PK ", 1 }, 
// FPP instructions (P857 only )
{"FFL",   0xC900, "   ", 1 },
{"FFX",   0xC901, "   ", 1 },
{"FADR",  0xCC20, "T  ", 1 },
{"FADRS", 0xCC21, "T  ", 1 },
{"FAD",   0xCC40, "MU ", 2 },
{"FADS",  0xCC41, "MU ", 2 },
{"FSUR",  0xCD20, "T  ", 1 },
{"FSURS", 0xCD21, "T  ", 1 },
{"FSU",   0xCD40, "MU ", 2 },
{"FSUS",  0xCD41, "MU ", 2 },
{"FMUR",  0xCE20, "T  ", 1 },
{"FMURS", 0xCE21, "T  ", 1 },
{"FMU",   0xCE40, "MU ", 2 },
{"FMUS",  0xCE41, "MU ", 2 },
{"FDVR",  0xCF20, "T  ", 1 },
{"FDVRS", 0xCF21, "T  ", 1 },
{"FDV",   0xCF40, "MU ", 2 },
{"FDVS",  0xCF41, "MU ", 2 },
{"FLDR",  0xC120, "T  ", 1 },
{"FLD",   0xC140, "MU ", 2 },
{"FSTR",  0xC121, "T  ", 1 },
{"FST",   0xC141, "MU ", 2 },
// MMU instructions (P857 only )" ERRORS in Philips Docu 
{"EL",    0xD040, "RMU", 2 },
{"ELR",   0xD020, "RT ", 1 },
{"ES",    0xD041, "RMU", 2 },
{"ESR",   0xD021, "RT ", 1 },
{"MVF",   0x7000, "T  ", 1 },
{"MVB",   0x7800, "T  ", 1 },
{"MVUS",  0x7880, "T  ", 1 },
{"MVSU",  0x7080, "T  ", 1 },
{"TLR",   0xB820, "T  ", 1 }, /* probably wrong in docu */
{"TL",    0xB800, "MU ", 2 },
{"TSR",   0xB821, "T  ", 1 }, /**/
{"TS",    0xB801, "MU ", 2 }
};


// Order below must be maintained !
// See documentation for handling r3 type register definitions
#define NR_REGS 32
struct registers  { char name[8];} regs [NR_REGS] = {
"P",  // Program counter
"A8",
"A1",
"A9",
"A2",
"A10",
"A3",
"A11",
"A4",
"A12",
"A5",
"A13",
"A6",
"A14",
"A7", // User stack ( suggested )
"A15",   // System stack
// Repeat registerset with new names for UCSD project.
// ONLY used for the UCSD Pascal
"A0",
"A8",
"A1",
"NP",	 //  A9  P-MACHINE:HEAP POINTER
"A2",
"A10",
"BASE",	 //  A3  P-MACHINE:BASE OF GLOBAL DATA SEGMENT
"A11",
"IPC",	 //  A4  P-MACHINE:INTERPRETER PROGRAM COUNTER
"A12",
"MP",	 //  A5 P-MACHINE:BASE OF LOCAL DATA SEGMENT
"GOBACK",// A13 P-MACHINE:AUXILIARY REGISTER 
"SP",	 //  A6	P-MACHINE:STACK POINTER
"RETURN",// A14 P-MACHINE:AuxI REGISTER 
"A7",
"A15"
};

// assembler psuedo functions
#define NR_CMDS  18
struct cmds  { char name[6];} commands [NR_CMDS] = {
"IDENT",
"END",
"ENTRY",
"AORG",
"RORG",
"IFF",
"IFT",
"XIF",
"DATA",
"EQU",
"RES",
"EJECT",
"LIST",
"NLIST",
"END.",
"RDSRC",
"EOF",
"EXTRN"
};
