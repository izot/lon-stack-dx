// Copyright (C) 2022 Dialog Semiconductor
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/********************************************************************
          File:        spm.c

       Version:        1.7

       Purpose:        Special Purpose Mode using SPI port on 360.
                       This does not support continuous frame exchange.
                       This files also has functions to support
                       io buttons for testing purpose such as
                       manual service request, reset, io pin, and LEDs.

*********************************************************************/

/* START INFORMATIVE - Direct Mode */
/* This implementation is for special purpose mode transceivers only.
 * If the implementation needs to support direct mode transceivers,
 * this example serves only as a starting point for such an
 * implemenation. */
/* END INFORMATIVE - Direct Mode */

/********************************************************************
Section: Includes
********************************************************************/
#include <string.h>
#include <stdlib.h>

#include <eia709_1.h>
#include <node.h>
#include <link.h>
#include <physical.h>

/*******************************************************************
Section: Constant Definitions
*******************************************************************/
/* Used to indicate how often to check status of i/o buttons */
#define PHYIO_CHECK_INTERVAL 0.1 /* In Seconds */

/* Macro sets bit B of 16 bit word X */
#define SET_BIT(B,X) (((0x0001U << (B)) | (X))

/* Macro Clears bit B of 16 bit word X */
#define CLEAR_BIT(B,X) (( ~(0x0001U << (B)) ) & (X))

/* Dual Port Ram Base on Arnewsh Board */
#define DPRB            0x01000000UL

/* SPI Parameter Ram Base */
#define SPIB            (DPRB + 0xD80UL)

/* SCC2 Parameter Ram Base */
#define SCC2B           (DPRB + 0xD00UL)

/* Register Base = DPRB + 4 k  */
#define REGB            (DPRB + 0x1000UL)

/* 16bit CPM Command Register */
#define CR              (REGB + 0x5C0UL)

/* 16 bit Serial DMA Config Register */
#define SDCR            (REGB + 0x51EUL)

/* 24(32)bit CPM Interrupt Config Reg.*/
#define CICR            (REGB + 0x540UL)

/* 32 bit CPM Interrupt Pending Reg. */
#define CIPR            (REGB + 0x544UL)

/* 32 bit CPM Interrupt Mask Reg. */
#define CIMR            (REGB + 0x548UL)

/* Clear CISR bit by writing a 1*/
/* 32 bit CPM Interrupt Service Register*/
#define CISR            (REGB + 0x54CUL)

/* 32 bit SI Clock Route */
#define SICR            (REGB + 0x6ECUL)

/* 16 bit SPI Mode Register */
#define SPMODE          (REGB + 0x6A0UL)

/* Clear SPIE bit by writing a 1*/
/* 8 bit SPI Event Register */
#define SPIE            (REGB + 0x6A6UL)

/* 8 bit SPI Mask Register */
#define SPIM            (REGB + 0x6AAUL)

/* 8 bit SPI Command Register */
#define SPCOM           (REGB + 0x6ADUL)

/* 32 bit SCC2 general mode register low */
#define GSMRL2          (REGB + 0x620UL)

/* 32 bit SCC2 general mode register high */
#define GSMRH2          (REGB + 0x624UL)

/* 16 bit SCC2 protocol specific mode reg */
#define PSMR2           (REGB + 0x628UL)

/* 16 bit SCC2 transmit on demand */
#define TODR2           (REGB + 0x62CUL)

/* 16 bit SCC2 Data sync register */
#define DSR2            (REGB + 0x62EUL)

/* Clear SCCE bit by writing a 1 */
/* 16 bit SCC2 event register */
#define SCCE2           (REGB + 0x630UL)

/* 16 bit SCC2 mask register */
#define SCCM2           (REGB + 0x634UL)

/* 8 bit SCC2 status register */
#define SCCS2           (REGB + 0x637UL)

/* 32 bit Baud rate gen config register */
#define BRGC2           (REGB + 0x5F4UL)

/* 16 bit port A data direction reg */
#define PADIR           (REGB + 0x550UL)

/* 16 bit port A pin assignment reg */
#define PAPAR           (REGB + 0x552UL)

/* 16 bit port A open drain register */
#define PAODR           (REGB + 0x554UL)

/* 16 bit port A data register */
#define PADAT           (REGB + 0x558UL)

/* 32 bit port B direction register */
#define PBDIR           (REGB + 0x6B8UL)

/* 32 bit port B pin assignment register */
#define PBPAR           (REGB + 0x6BCUL)

/* 16 bit port B open drain register */
#define PBODR           (REGB + 0x6C2UL)

/* 32 bit port B data register */
#define PBDAT           (REGB + 0x6C4UL)

/* 16 bit port C direction register */
#define PCDIR           (REGB + 0x560UL)

/* 16 bit port C pin assignment reg */
#define PCPAR           (REGB + 0x562UL)

/* 16 bit port C special options */
#define PCSO            (REGB + 0x564UL)

/* 16 bit port C data register */
#define PCDAT           (REGB + 0x566UL)

/* 16 bit port C interrupt register */
#define PCINT           (REGB + 0x568UL)

/* Timer Related Constants */
/* 16 bit timer general config register */
#define TGCR      (REGB + 0x580UL)

/* 16 bit timer reference  reg 1 */
/* or 32 bit cascaded 1 & 2      */
#define TRR1      (REGB + 0x594UL)

/* 16 bit timer counter reg 1 */
/* or 32 bit cascaded 1 & 2   */
#define TCN1      (REGB + 0x59CUL)


/* 16 bit timer mode register 4 */
#define TMR2      (REGB + 0x592UL)

/* 16 bit timer reference register 4 */
#define TRR2      (REGB + 0x596UL)

/* 16 bit timer capture register 4 */
#define TCR2      (REGB + 0x59AUL)

/* 16 bit timer counter register 4 */
#define TCN2      (REGB + 0x59EUL)

/* 16 bit timer event register 4 */
#define TER2      (REGB + 0x5B2UL)


/* ISR defines */
/* Interrupt vector base on Arnewsh Board*/
#define VBASE           0x00000000UL

/* User Interrupt vector num 3 MSbits */
#define UIVN_MSB        0x4U

/* SPI Int.vector num 5 LSBits from CPIC table*/
#define SPIVN_LSB       0x5U

/********************************************************************
8 bit vector number = 3 msb set by user, 5 lsb from CPIC table shift
UIVN_MSB left 5 to bits 7 6 5 then OR with SPIVN_LSB to get vector
number vector address is 4 times vector number + vector base
********************************************************************/
/* 32 bit SPI Int. vector address */
#define SPIV      ((((UIVN_MSB << 5) | SPIVN_LSB) * 4UL) + VBASE)


/* need an ISR for int error since int. error cannot be masked */
/* CPM int. error int. vector. num 5 LSB  */
#define CERRVN_LSB      0x0UL

/* cpm error iv */
#define CERRV  ((((UIVN_MSB << 5) | CERRVN_LSB) * 4UL) + VBASE)

/* spurious interrupt exception  vector */
#define SPURINTV 0x60U + VBASE

/* bus error  exception  vector */
#define BUSERRV 0x8U + VBASE

/* SPI Channel Number for CPM Command Register Commands */
#define SPI_CH_NUM      0x5U

/* SCC2 channel number for CPM Command Register Command */
#define SCC2_CH_NUM     0x4U

/* Opcode for CPM Com. Reg. to Init Tx Rx Param Ram */
#define INIT_TRP_OPCODE 0x0U

/* number of receive (transmit) BDs for SPI */
#define NUM_BD 0x1U

/* RBD_BASE and TBD_BASE must be divisible by 8 */
/* receive buffer descriptor base */
#define RBD_BASE (DPRB + 0x500UL)

/* put Transmit buffers right after Receive,  8bytes per BD */
/* transmit buffer descriptor base */
#define TBD_BASE (RBD_BASE + (NUM_BD * 0x8U))

/********************************************************************
   FCR_INIT
        bits 7-5 unused,
        bit 4, Mot big endian = 1
        bits 3-0, function code dma = 8
********************************************************************/
/* SPI Param Function Code Register Init */
#define FCR_INIT        0x18U

/* max length of receive buffers 2 bytes per buffer*/
#define MAX_BUF_LEN 0x2U

/* CP Interrupt Request Level */
#define CPIR_LEVEL 0x4U

#define NUM_BD_SCC2     0x1U

/* scc2 receive buffer descriptor. base */
#define RBD_BASE_SCC2 (DPRB + 0x440UL)

/* scc2 transmit bd base */
#define TBD_BASE_SCC2 (RBD_BASE_SCC2 + (NUM_BD_SCC2 * 8U))

/* max buffer length scc2 */
#define MAX_BL_SCC2     16U

/* length of history arrays */
#define NUM_HIST 512

/* time limit before xcvr resets */
#define RESET_COUNT_LIMIT 0xFFFF

/* number of ticks to delay during restart processing */
#define RESTART_DELAY_TICKS 165000UL

/*  Constants related to Channel Access Algorithm */
/* maximum size of backlog for access algorithm */
#define MAX_BACKLOG 63U

/* # of additional slots per backlog increment */
#define W_BASE 16U

/* used in conversion from this specification's
   reference time to clock ticks on the 360 */
#define NICS_TICKS_BASE 480UL

/* used in computation of SPI-SPM bit clock */
#define BIT_CLOCK_BASE 312500UL

/* used in computation of bit clock */
#define RATIO_BASE 3UL

/********************************************************************
Implementation specific timing adjustments to channel access algorithm
This is where one would add or subtract from the respective times to
account for differences between the implementation specific delays and
latencies and those in this specification.
********************************************************************/
/* adjustment to the beta1 slot time */
#define BETA1_ADJUST_TICKS 0

/* adjustment to the beta2 slot time */
#define BETA2_ADJUST_TICKS 0

/* adjustment to the cycle timer time */
#define CYCLE_ADJUST_TICKS 0

/* adjustment to the begin of packet tx */
#define WAIT_TX_ADJUST_TICKS 0

/********************************************************************
Section: Type Definitions
********************************************************************/
/* 68360 communications processor register definitions */

typedef struct          /* CP Command Register 16 bit */
{
   unsigned reset  : 1; /* reset CPM */
   unsigned        : 3; /* unused */
   unsigned opcode : 4; /* opcode for command */
   unsigned chnum  : 4; /* channel number selects which port
                           SPI, SCC etc */
   unsigned        : 3; /* unused */
   unsigned flag   : 1; /* flag, command executes when set,
                           cp clears when done */
} CPCommandReg;

typedef struct          /* Serial DMA Config Register 16 bit */
{
   unsigned      : 1;   /* unused */
   unsigned frz  : 2;   /* freeze */
   unsigned      : 2;   /* unused */
   unsigned sism : 3;   /* SDMA interrupt service mask */
   unsigned      : 1;   /* unused */
   unsigned said : 3;   /* SDMA arbitration ID */
   unsigned      : 2;   /* unused */
   unsigned inte : 1;   /* interrupt error */
   unsigned intb : 1;   /* interrupt break point */
} SDMAConfigReg;

typedef struct  /* CPM interrupt configuration register 32 bit */
{
   unsigned             : 8; /* unused */
   unsigned        scdp : 2; /* scc port for priority slot d */
   unsigned        sccp : 2; /* scc port for priority slot c */
   unsigned        scbp : 2; /* scc port for priority slot b */
   unsigned        scap : 2; /* scc port for priority slot a */
   unsigned        irl  : 3; /* interrupt request level */
   unsigned        hpi  : 5; /* highest priority interrupt */
   unsigned        vba  : 3; /* vector base address number offset */
   unsigned             : 4; /* reserved */
   unsigned        sps  : 1; /* spread priority scheme */
} CIConfigReg;

/* CPM interrupt source 32 bit, same type for CIPR CIMR CISR */
typedef struct
{ /* names correspond to sources */
   unsigned pc0    : 1;            /* bit 31 */
   unsigned scc1   : 1;
   unsigned scc2   : 1;
   unsigned scc3   : 1;
   unsigned scc4   : 1;
   unsigned pc1    : 1;
   unsigned timer1 : 1;
   unsigned pc2    : 1;            /* bit 24  */
   unsigned pc3    : 1;            /* bit 23 */
   unsigned sdma   : 1;
   unsigned idma1  : 1;
   unsigned idma2  : 1;
   unsigned        : 1;            /* unused */
   unsigned timer2 : 1;
   unsigned rtt    : 1;
   unsigned        : 1;            /* bit 16 unused */
   unsigned pc4    : 1;            /* bit 15 */
   unsigned pc5    : 1;
   unsigned        : 1;            /* unused */
   unsigned timer3 : 1;
   unsigned pc6    : 1;
   unsigned pc7    : 1;
   unsigned pc8    : 1;
   unsigned        : 1;            /* bit 8 unused */
   unsigned timer4 : 1;            /* bit 7 */
   unsigned pc9    : 1;
   unsigned spi    : 1;            /* bit 5 */
   unsigned smc1   : 1;
   unsigned smc2   : 1;            /* also pip */
   unsigned pc10   : 1;
   unsigned pc11   : 1;
   unsigned        : 1;            /* bit 0 unused */
} CISourceReg;

/* serial interface clock route register 32 bit*/
typedef struct
{
   unsigned gr4   : 1;    /* grant support for scc4*/
   unsigned sc4   : 1;    /* connection for scc4*/
   unsigned r4cs  : 3;    /* receive clock source for scc4*/
   unsigned t4cs  : 3;    /* transmit clock source for scc4*/
   unsigned gr3   : 1;    /* grant support for scc3 */
   unsigned sc3   : 1;    /* connection for scc3 */
   unsigned r3cs  : 3;    /* receive clock source for scc3 */
   unsigned t3cs  : 3;    /* transmit clock source for scc3 */
   unsigned gr2   : 1;    /* grant support for scc2 */
   unsigned sc2   : 1;    /* connection for scc2 */
   unsigned r2cs  : 3;    /* receive clock source for scc2 */
   unsigned t2cs  : 3;    /* transmit clock source for scc2 */
   unsigned gr1   : 1;    /* grant support for scc1 */
   unsigned sc1   : 1;    /* connection for scc1 */
   unsigned r1cs  : 3;    /* receive clock source for scc1 */
   unsigned t1cs  : 3;    /* transmit clock source for scc1 */
} SIClockRouteReg;

typedef struct          /* SPI command register 8 bit*/
{
   unsigned str : 1;    /* start transmit and receive */
   unsigned res : 7;    /* reserved write with zeros */
} SPICommandReg;

typedef struct          /* SPI mode register 16 bit */
{
   unsigned       : 1;       /* unused */
   unsigned loop  : 1;       /* local loop back */
   unsigned ci    : 1;       /* clock invert */
   unsigned cp    : 1;       /* clock phase */
   unsigned div16 : 1;       /* divide brgclk by 16 for spiclk */
   unsigned rev   : 1;       /* reverse data from big endian to
                                little endian */
   unsigned ms    : 1;       /* master slave */
   unsigned en    : 1;       /* enable SPI */
   unsigned len   : 4;       /* number of bits per char = len + 1 */
   unsigned pms   : 4;       /* prescale modulus select, divide
                                down clock */
} SPIModeReg;

typedef struct  /* SPI event and mask registers 8 bit */
{
   unsigned     : 2;   /* unused */
   unsigned mme : 1;   /* multi-master error */
   unsigned txe : 1;   /* transmit error */
   unsigned     : 1;   /* unused */
   unsigned bsy : 1;   /* busy error */
   unsigned txb : 1;   /* transmit buffer complete */
   unsigned rxb : 1;   /* receive buffer complete */
} SPIEventMaskReg;

typedef struct          /* SPI Parameter RAM  40 bytes*/
{
   uint16 rbase;  /* base address of receive BD's, set by user */
   uint16 tbase;  /* base address of transmit BD's, set by user */
   uint8  rfcr;   /* receive function code, set by user  */
   uint8  tfcr;   /* transmit function code, set by user  */
   uint16 mrblr;  /* maximum receive buffer length register,
                     set by user  */
   uint32 rstate; /* rx internal state    */
   uint32 ridp;   /* rx internal data ptr */
   uint16 rbptr;  /* rx BD ptr */
   uint16 ribc;   /* rx internal byte count */
   uint32 rtemp;  /* rx temp */
   uint32 tstate; /* tx internal state    */
   uint32 tidp;   /* tx internal data ptr */
   uint16 tbptr;  /* tx BD ptr */
   uint16 tibc;   /* tx internal byte count */
   uint32 ttemp;  /* tx temp */
} SPIParamRam;


typedef struct          /* Receive Buffer Descriptor 8 bytes */
{
   unsigned e  : 1;  /* empty buffer ready to receive*/
   unsigned    : 1;  /* unused */
   unsigned w  : 1;  /* wrap final bd so wrap to top */
   unsigned i  : 1;  /* interrupt, set rxb bit in spie register
                        when filled */
   unsigned l  : 1;  /* last, if slave then buffer has last bit */
   unsigned    : 1;  /* unused */
   unsigned cm : 1;  /* continuous mode buffer can be overwritten
                        e always 1 */
   unsigned    : 7;  /* unused */
   unsigned ov : 1;  /* receiver overrun only when slave */
   unsigned me : 1;  /* multiple master error */
   uint16   dataLen; /* data length = number of bytes written
                        into buffer */
   SPMRxFrame * dataPtr;  /* ptr to buffer of data */
} RBufferDesc;

typedef struct          /* Transmit Buffer Descriptor 8 bytes */
{
   unsigned r  : 1; /* ready to transmit buffer */
   unsigned    : 1; /* unused */
   unsigned w  : 1; /* wrap final bd so wrap to top */
   unsigned i  : 1; /* interrupt, set txb bit in spie register
                       when sent */
   unsigned l  : 1; /* last, if slave then buffer has last bit */
   unsigned    : 1; /* unused */
   unsigned cm : 1; /* continuous mode buffer can be re-sent
                       r always 1 */
   unsigned    : 7; /* unused */
   unsigned un : 1; /* transmit underrun only when slave */
   unsigned me : 1; /* multiple master error */
   uint16   dataLen; /* data length = number of bytes to send
                        from this buffer */
   SPMTxFrame * dataPtr;   /* ptr to buffer of data */
} TBufferDesc;

/* scc structs */

typedef struct          /* general scc mode register  high 32 bit */
{
   unsigned      : 15;     /* unused */
   unsigned gde  : 1;      /* glitch detect enable */
   unsigned tcrc : 2;      /* transparent crc */
   unsigned revd : 1;      /* reverse data */
   unsigned trx  : 1;      /* transparent receiver */
   unsigned ttx  : 1;      /* transparent transmitter */
   unsigned cdp  : 1;      /* cd pulse */
   unsigned ctsp : 1;      /* cts pulse */
   unsigned cds  : 1;      /* cd sampling */
   unsigned ctss : 1;      /* cts sampling */
   unsigned tfl  : 1;      /* transmit fifo length */
   unsigned rfw  : 1;      /* receive fifl width */
   unsigned txsy : 1;      /* transmitter synchronized to receiver */
   unsigned synl : 2;      /* sync length */
   unsigned rtsm : 1;      /* rts mode */
   unsigned rsyn : 1;      /* receive sync timing */
} GSMRegHigh;


typedef struct          /* general scc mode register  low 32 bit */
{
   unsigned      : 1;      /* unused */
   unsigned edge : 2;      /* DPLL clock edge */
   unsigned tci  : 1;      /* transmit clock invert */
   unsigned tsnc : 2;      /* transmit sense bits */
   unsigned rinv : 1;      /* dpll receive invert data*/
   unsigned tinv : 1;      /* dpll transmit invert data */
   unsigned tpl  : 3;      /* transmit preamble length */
   unsigned tpp  : 2;      /* transmit preamble pattern */
   unsigned tend : 1;      /* transmit frame ending */
   unsigned tdcr : 2;      /* transmit divide clock rate */
   unsigned rdcr : 2;      /* receive dpll clock rate */
   unsigned renc : 3;      /* receiver decoding method */
   unsigned tenc : 3;      /* transmitter encoding method */
   unsigned diag : 2;      /* diagnostic mode */
   unsigned enr  : 1;      /* enable receive */
   unsigned ent  : 1;      /* enable transmit */
   unsigned mode : 4;      /* channel protocol mode */
} GSMRegLow;

typedef struct          /* SCC event/mask register 16 bit */
{
   unsigned     : 3;     /* unused */
   unsigned glr : 1;     /* glitch on receive */
   unsigned glt : 1;     /* glitch on tranmit */
   unsigned dcc : 1;     /* dpll carrier sense changed */
   unsigned     : 2;     /* unused */
   unsigned gra : 1;     /* graceful stop complete */
   unsigned     : 2;     /* unused */
   unsigned txe : 1;     /* transmit error */
   unsigned rch : 1;     /* receive character or long word */
   unsigned bsy : 1;     /* busy condition */
   unsigned tx  : 1;     /* buffer transmitted */
   unsigned rx  : 1;     /* buffer received */
} SCCEventMaskReg;

typedef struct          /* scc status register 8 bit */
{
   unsigned    : 6;  /* unused */
   unsigned cs : 1;  /* carrier sense */
   unsigned    : 1;  /* unused */
} SCCStatusReg;

typedef struct   /* SCC Parameter RAM Transparent mode 56 bytes */
{
   uint16 rbase; /* base address of receive BD's, set by user */
   uint16 tbase; /* base address of transmit BD's, set by user */
   uint8  rfcr;  /* receive function code, set by user  */
   uint8  tfcr;  /* transmit function code, set by user  */
   uint16 mrblr; /* maximum receive buffer length register,
                    set by user  */
   uint32 rstate; /* rx internal state    */
   uint32 ridp;   /* rx internal data ptr */
   uint16 rbptr;  /* rx BD ptr */
   uint16 ribc;   /* rx internal byte count */
   uint32 rtemp;  /* rx temp */
   uint32 tstate; /* tx internal state    */
   uint32 tidp;   /* tx internal data ptr */
   uint16 tbptr;  /* tx BD ptr */
   uint16 tibc;   /* tx internal byte count */
   uint32 ttemp;  /* tx temp */
   uint32 rcrc;   /* temp receive crc */
   uint32 tcrc;   /* temp transmit crc */
   uint32 crcp;   /* crc preset for transparent mode */
   uint32 crcc;   /* crc constant for transparent mode */
} SCCParamRam;

typedef struct    /* SCC Receive Buffer Descriptor 8 bytes */
{
   unsigned e  : 1; /* empty buffer ready to receive*/
   unsigned    : 1; /* unused */
   unsigned w  : 1; /* wrap final bd so wrap to top */
   unsigned i  : 1; /* interrupt, set rxb bit in scce register
                       when filled */
   unsigned l  : 1; /* this buffer last in frame */
   unsigned f  : 1; /* first in frame */
   unsigned cm : 1; /* continuous mode buffer can be overwritten
                       e always 1 */
   unsigned    : 1; /* unused */
   unsigned de : 1; /* dpll error */
   unsigned    : 2; /* unused */
   unsigned no : 1; /* non octet error */
   unsigned    : 1; /* unused */
   unsigned cr : 1; /* crc error */
   unsigned ov : 1; /* receiver overrun  */
   unsigned cd : 1; /* carrier detect lost */
   uint16   dataLen; /* data length = number of bytes written
                        into buffer */
   Byte   * dataPtr; /* ptr to buffer of data */
} SCCReceiveBD;

typedef struct   /* SCC Transmit Buffer Descriptor 8 bytes */
{
   unsigned r  : 1; /* ready to transmit buffer */
   unsigned    : 1; /* unused */
   unsigned w  : 1; /* wrap final bd so wrap to top */
   unsigned i  : 1; /* interrupt, set txb bit in scce register
                       when sent */
   unsigned l  : 1; /* last byte in frame in this buffer */
   unsigned tc : 1; /* transmit crc */
   unsigned cm : 1; /* continuous mode buffer can be re-sent r always 1 */
   unsigned    : 7; /* unused */
   unsigned un : 1; /* transmit underrun */
   unsigned ct : 1; /* cts lost during frame transmission */
   uint16   dataLen; /* data length = number of bytes to send
                        from this buffer */
   Byte   * dataPtr; /* ptr to buffer of data */
} SCCTransmitBD;

typedef struct  /* BRGC baud rate generator config register 32 bit */
{
   unsigned       : 14;    /* unused */
   unsigned rst   : 1;     /* reset brg */
   unsigned en    : 1;     /* enable brg count */
   unsigned extc  : 2;     /* enable external clock source */
   unsigned atb   : 1;     /* autobaud */
   unsigned cd    : 12;    /* clock divider */
   unsigned div16 : 1;     /* div 16 clock */
} BRGConfigReg;

typedef struct    /* port a direction register 16 bit */
{
   unsigned dr15 : 1;      /* pin direction 0 = input 1 = output */
   unsigned dr14 : 1;
   unsigned dr13 : 1;
   unsigned dr12 : 1;
   unsigned dr11 : 1;
   unsigned dr10 : 1;
   unsigned dr9  : 1;
   unsigned dr8  : 1;
   unsigned dr7  : 1;
   unsigned dr6  : 1;
   unsigned dr5  : 1;
   unsigned dr4  : 1;
   unsigned dr3  : 1;
   unsigned dr2  : 1;
   unsigned dr1  : 1;
   unsigned dr0  : 1;
} PADirectionReg;

typedef struct          /* port a pin assignment register 16 bit */
{
   unsigned dd15 : 1;      /* 0 = general purpose io */
   unsigned dd14 : 1;
   unsigned dd13 : 1;
   unsigned dd12 : 1;
   unsigned dd11 : 1;
   unsigned dd10 : 1;
   unsigned dd9  : 1;
   unsigned dd8  : 1;
   unsigned dd7  : 1;
   unsigned dd6  : 1;
   unsigned dd5  : 1;
   unsigned dd4  : 1;
   unsigned dd3  : 1;
   unsigned dd2  : 1;
   unsigned dd1  : 1;
   unsigned dd0  : 1;
} PAPinAssignmentReg;

typedef struct          /* port a open drain register 16 bit */
{
   unsigned      : 8;   /* unused */
   unsigned od7  : 1;   /* 0 = active drive 1 = open drain */
   unsigned od6  : 1;
   unsigned od5  : 1;
   unsigned od4  : 1;
   unsigned od3  : 1;
   unsigned      : 1;    /* unused */
   unsigned od1  : 1;
   unsigned      : 1;    /* unused */
} PAOpenDrainReg;

typedef struct          /* port a data register 16 bit */
{
   unsigned d15 : 1;       /* value of pin */
   unsigned d14 : 1;
   unsigned d13 : 1;
   unsigned d12 : 1;
   unsigned d11 : 1;
   unsigned d10 : 1;
   unsigned d9  : 1;
   unsigned d8  : 1;
   unsigned d7  : 1;
   unsigned d6  : 1;
   unsigned d5  : 1;
   unsigned d4  : 1;
   unsigned d3  : 1;
   unsigned d2  : 1;
   unsigned d1  : 1;
   unsigned d0  : 1;
} PADataReg;

typedef struct          /* PBDIR port b direction register 32 bit */
{
   unsigned      : 14;             /* unused*/
   unsigned dr17 : 1;              /* pin with given # */
   unsigned dr16 : 1;              /* pin with same # */
   unsigned dr15 : 1;              /* pin with same # */
   unsigned dr14 : 1;              /* pin with same # */
   unsigned dr13 : 1;              /* pin with same # */
   unsigned dr12 : 1;              /* pin with same # */
   unsigned dr11 : 1;              /* pin with same # */
   unsigned dr10 : 1;              /* pin with same # */
   unsigned dr9  : 1;              /* pin with same # */
   unsigned dr8  : 1;              /* pin with same # */
   unsigned dr7  : 1;              /* pin with same # */
   unsigned dr6  : 1;              /* pin with same # */
   unsigned dr5  : 1;              /* pin with same # */
   unsigned dr4  : 1;              /* pin with same # */
   unsigned dr3  : 1;              /* pin with same # */
   unsigned dr2  : 1;              /* pin with same # */
   unsigned dr1  : 1;              /* pin with same # */
   unsigned dr0  : 1;              /* pin with same # */
} PBDirectionReg;

typedef struct  /* PBPAR port b pin assignment register 32 bit */
{
   unsigned      : 14;             /* unused*/
   unsigned dd17 : 1;              /* pin with given # */
   unsigned dd16 : 1;              /* pin with same # */
   unsigned dd15 : 1;              /* pin with same # */
   unsigned dd14 : 1;              /* pin with same # */
   unsigned dd13 : 1;              /* pin with same # */
   unsigned dd12 : 1;              /* pin with same # */
   unsigned dd11 : 1;              /* pin with same # */
   unsigned dd10 : 1;              /* pin with same # */
   unsigned dd9  : 1;              /* pin with same # */
   unsigned dd8  : 1;              /* pin with same # */
   unsigned dd7  : 1;              /* pin with same # */
   unsigned dd6  : 1;              /* pin with same # */
   unsigned dd5  : 1;              /* pin with same # */
   unsigned dd4  : 1;              /* pin with same # */
   unsigned dd3  : 1;              /* pin with same # */
   unsigned dd2  : 1;              /* pin with same # */
   unsigned dd1  : 1;              /* pin with same # */
   unsigned dd0  : 1;              /* pin with same # */
} PBPinAssignmentReg;

typedef struct          /* PBODR port b open drain register 16 bit */
{
   unsigned od15 : 1;              /* pin with same # */
   unsigned od14 : 1;              /* pin with same # */
   unsigned od13 : 1;              /* pin with same # */
   unsigned od12 : 1;              /* pin with same # */
   unsigned od11 : 1;              /* pin with same # */
   unsigned od10 : 1;              /* pin with same # */
   unsigned od9  : 1;              /* pin with same # */
   unsigned od8  : 1;              /* pin with same # */
   unsigned od7  : 1;              /* pin with same # */
   unsigned od6  : 1;              /* pin with same # */
   unsigned od5  : 1;              /* pin with same # */
   unsigned od4  : 1;              /* pin with same # */
   unsigned od3  : 1;              /* pin with same # */
   unsigned od2  : 1;              /* pin with same # */
   unsigned od1  : 1;              /* pin with same # */
   unsigned od0  : 1;              /* pin with same # */
} PBOpenDrainReg;

typedef struct      /* PBDAT port b data register 32 bit */
{
   unsigned      : 14;             /* unused*/
   unsigned d17 : 1;               /* pin with given # */
   unsigned d16 : 1;               /* pin with same # */
   unsigned d15 : 1;               /* pin with same # */
   unsigned d14 : 1;               /* pin with same # */
   unsigned d13 : 1;               /* pin with same # */
   unsigned d12 : 1;               /* pin with same # */
   unsigned d11 : 1;               /* pin with same # */
   unsigned d10 : 1;               /* pin with same # */
   unsigned d9  : 1;               /* pin with same # */
   unsigned d8  : 1;               /* pin with same # */
   unsigned d7  : 1;               /* pin with same # */
   unsigned d6  : 1;               /* pin with same # */
   unsigned d5  : 1;               /* pin with same # */
   unsigned d4  : 1;               /* pin with same # */
   unsigned d3  : 1;               /* pin with same # */
   unsigned d2  : 1;               /* pin with same # */
   unsigned d1  : 1;               /* pin with same # */
   unsigned d0  : 1;               /* pin with same # */
} PBDataReg;

typedef struct          /* port c direction register 16 bit */
{
   unsigned      : 4;   /* unused */
   unsigned dr11 : 1;   /* pin direction 0 = input 1 = output */
   unsigned dr10 : 1;
   unsigned dr9  : 1;
   unsigned dr8  : 1;
   unsigned dr7  : 1;
   unsigned dr6  : 1;
   unsigned dr5  : 1;
   unsigned dr4  : 1;
   unsigned dr3  : 1;
   unsigned dr2  : 1;
   unsigned dr1  : 1;
   unsigned dr0  : 1;
} PCDirectionReg;

typedef struct          /* port c pin assignment register 16 bit */
{
   unsigned      : 4;  /* unused */
   unsigned dd11 : 1;  /* 0 = general purpose io */
   unsigned dd10 : 1;
   unsigned dd9  : 1;
   unsigned dd8  : 1;
   unsigned dd7  : 1;
   unsigned dd6  : 1;
   unsigned dd5  : 1;
   unsigned dd4  : 1;
   unsigned dd3  : 1;
   unsigned dd2  : 1;
   unsigned dd1  : 1;
   unsigned dd0  : 1;
} PCPinAssignmentReg;

typedef struct          /* port c data register 16 bit */
{
   unsigned     : 4;   /* unused */
   unsigned d11 : 1;   /* value of pin */
   unsigned d10 : 1;
   unsigned d9  : 1;
   unsigned d8  : 1;
   unsigned d7  : 1;
   unsigned d6  : 1;
   unsigned d5  : 1;
   unsigned d4  : 1;
   unsigned d3  : 1;
   unsigned d2  : 1;
   unsigned d1  : 1;
   unsigned d0  : 1;
} PCDataReg;

typedef struct          /* port c interrupt control register 16 bit */
{
   unsigned       : 4;   /* unused */
   unsigned edm11 : 1;   /* edge detect mode for line*/
   unsigned edm10 : 1;
   unsigned edm9  : 1;
   unsigned edm8  : 1;
   unsigned edm7  : 1;
   unsigned edm6  : 1;
   unsigned edm5  : 1;
   unsigned edm4  : 1;
   unsigned edm3  : 1;
   unsigned edm2  : 1;
   unsigned edm1  : 1;
   unsigned edm0  : 1;
} PCInterruptReg;

typedef struct          /* port c special options register 16 bit */
{
   unsigned      : 4;              /* unused */
   unsigned cd4  : 1;              /* Carrier detect */
   unsigned cts4 : 1;              /* clear to send */
   unsigned cd3  : 1;              /* Carrier detect */
   unsigned cts3 : 1;              /* clear to send */
   unsigned cd2  : 1;              /* Carrier detect */
   unsigned cts2 : 1;              /* clear to send */
   unsigned cd1  : 1;              /* Carrier detect */
   unsigned cts1 : 1;              /* clear to send */
   unsigned      : 4;              /* unused */
} PCSpecialOptionsReg;

/* Timer related TypeDefs */

typedef struct    /* timer general config register  16 bit */
{
   unsigned cas4 : 1;   /* cascade timer 4 */
   unsigned frz4 : 1;   /* freeze timer 4 */
   unsigned stp4 : 1;   /* stop timer 4 */
   unsigned rst4 : 1;   /* reset timer 4 */
   unsigned gm2  : 1;   /* gate mode pin 2 */
   unsigned frz3 : 1;   /* freeze timer 3 */
   unsigned stp3 : 1;   /* stop timer 3 */
   unsigned rst3 : 1;   /* reset timer 3 */
   unsigned cas2 : 1;   /* cascade timer 2 */
   unsigned frz2 : 1;   /* freeze timer 2 */
   unsigned stp2 : 1;   /* stop timer  2 */
   unsigned rst2 : 1;   /* reset timer 2 */
   unsigned gm1  : 1;   /* gate mode pin 1 */
   unsigned frz1 : 1;   /* freeze timer 1 */
   unsigned stp1 : 1;   /* stop timer 1 */
   unsigned rst1 : 1;   /* reset timer 1 */
} TimerGenConfigReg;

typedef struct    /*  timer mode register 16 bit */
{
   unsigned ps   : 8;    /* prescaler */
   unsigned ce   : 2;    /* capture edge */
   unsigned om   : 1;    /* output mode */
   unsigned ori  : 1;    /* output ref interrupt enable */
   unsigned frr  : 1;    /* free run/restart */
   unsigned iclk : 2;    /* input clock source */
   unsigned ge   : 1;    /* gate enable */
} TimerModeReg;

typedef struct    /* timer event register 16 bit */
{
   unsigned     : 14;   /* unused */
   unsigned ref : 1;    /* output reference event */
   unsigned cap : 1;    /* input capture event */
} TimerEventReg;


/* structure used for byte by byte crc checking on receive */

typedef struct
{
   uint16 poly;
   uint16 crc;
   uint8 crcBit;
   uint8 dataBit;
   uint8 dataByte;
} CRCParam;

/* structures used for debugging purposes */

typedef struct  /* contains record of SPM frame */
{
   SPMState state;   /* state when received status byte */
   SPMRxFrame rf;    /*  RX frame */
   SPMTxFrame tf;    /* TX frames */
   uint32 duration;  /* time to run isr */
   uint32 start;     /* start time of isr */
   uint16 rb;        /* index to receive buffer */
   uint16 tb;        /* index to transmit bufer */
} Record;


typedef struct /* contains history of frame records */
{
   int index; /* current record wraps around when full */
   Record records[NUM_HIST];  /* array of records */
} History;

/********************************************************************
Section: Local Globals
********************************************************************/
/* pointers to SPI registers  all are constant pointers to volatiles
   volatile is needed to insure mem access occurs at each reference */

static volatile CPCommandReg  * const crPtrGbl =
           (volatile CPCommandReg *) CR;
static volatile SDMAConfigReg  * const sdcrPtrGbl =
           (volatile SDMAConfigReg *) SDCR;

static volatile CIConfigReg  * const cicrPtrGbl =
           (volatile CIConfigReg *) CICR;
static volatile CISourceReg  * const ciprPtrGbl =
           (volatile CISourceReg *) CIPR;
static volatile CISourceReg  * const cimrPtrGbl =
           (volatile CISourceReg *) CIMR;
static volatile CISourceReg  * const cisrPtrGbl =
           (volatile CISourceReg *) CISR;

static volatile SIClockRouteReg  * const sicrPtrGbl =
           (volatile SIClockRouteReg *) SICR;

static volatile SPIModeReg * const spmodePtrGbl =
           (volatile SPIModeReg *) SPMODE;
static volatile SPIEventMaskReg  * const spiePtrGbl =
           (volatile SPIEventMaskReg *) SPIE;

static volatile SPIEventMaskReg  * const spimPtrGbl =
           (volatile SPIEventMaskReg *) SPIM;
static volatile SPICommandReg  * const spcomPtrGbl =
           (volatile SPICommandReg *) SPCOM;

static volatile SPIParamRam  * const spiParamPtrGbl =
           (volatile SPIParamRam *) SPIB;
static volatile RBufferDesc  * const rbdPtrGbl =
           (volatile RBufferDesc *) RBD_BASE;
static volatile TBufferDesc  * const tbdPtrGbl =
           (volatile TBufferDesc *) TBD_BASE;


/* SCC2 registers */
static volatile GSMRegLow  * const gsmrl2PtrGbl =
           (volatile GSMRegLow *) GSMRL2;
static volatile GSMRegHigh  * const gsmrh2PtrGbl =
           (volatile GSMRegHigh *) GSMRH2;
static volatile uint16 * const dsr2PtrGbl =
           (volatile uint16 *) DSR2;
static volatile uint16 * const todr2PtrGbl =
           (volatile uint16 *) TODR2;
static volatile SCCEventMaskReg  * const scce2PtrGbl =
           (volatile SCCEventMaskReg *) SCCE2;
static volatile SCCEventMaskReg  * const sccm2PtrGbl =
           (volatile SCCEventMaskReg *) SCCM2;
static volatile SCCStatusReg  * const sccs2PtrGbl =
           (volatile SCCStatusReg *) SCCS2;
static volatile BRGConfigReg  * const brgc2PtrGbl =
           (volatile BRGConfigReg *) BRGC2;

static volatile SCCParamRam  * const scc2ParamPtrGbl =
           (volatile SCCParamRam *) SCC2B;
static volatile SCCReceiveBD  * const scc2rbdPtrGbl =
           (volatile SCCReceiveBD *) RBD_BASE_SCC2;
static volatile SCCTransmitBD  * const scc2tbdPtrGbl =
           (volatile SCCTransmitBD *) TBD_BASE_SCC2;

/* allocate storage for scc transmit and receive buffers,
   init to zeros */
static volatile Byte scc2rBufGbl[NUM_BD_SCC2][MAX_BL_SCC2] = {0};
static volatile Byte scc2tBufGbl[NUM_BD_SCC2][MAX_BL_SCC2] = {0};

/* Port A registers */
static volatile PADirectionReg  * const padirPtrGbl =
           (volatile PADirectionReg *) PADIR;
static volatile PAPinAssignmentReg  * const paparPtrGbl =
           (volatile PAPinAssignmentReg *) PAPAR;
static volatile PAOpenDrainReg  * const paodrPtrGbl =
           (volatile PAOpenDrainReg *) PAODR;
static volatile PADataReg  * const padatPtrGbl =
           (volatile PADataReg *) PADAT;

/* Port B registers */
static volatile PBDirectionReg  * const pbdirPtrGbl =
           (volatile PBDirectionReg *) PBDIR;
static volatile PBPinAssignmentReg  * const pbparPtrGbl =
           (volatile PBPinAssignmentReg *) PBPAR;
static volatile PBOpenDrainReg  * const pbodrPtrGbl =
           (volatile PBOpenDrainReg *) PBODR;
static volatile PBDataReg  * const pbdatPtrGbl =
           (volatile PBDataReg *) PBDAT;
/* Port C registers */
static volatile PCDirectionReg  * const pcdirPtrGbl =
           (volatile PCDirectionReg *) PCDIR;
static volatile PCPinAssignmentReg  * const pcparPtrGbl =
           (volatile PCPinAssignmentReg *) PCPAR;
static volatile PCDataReg  * const pcdatPtrGbl =
           (volatile PCDataReg *) PCDAT;
static volatile PCSpecialOptionsReg * const pcsoPtrGbl =
           (volatile PCSpecialOptionsReg *) PCSO;
static volatile PCInterruptReg * const pcintPtrGbl =
           (volatile PCInterruptReg *) PCINT;

/* const ptr to ptr to function returning void with void params */
static volatile void  (** const spivPtrGbl)(void) =
           (volatile void  (**)(void)) SPIV; /* ptr to vector */
static volatile void  (** const cerrvPtrGbl)(void) =
           (volatile void  (**)(void)) CERRV; /* ptr to vector */
static volatile void  (** const spurintvPtrGbl)(void) =
           (volatile void  (**)(void)) SPURINTV; /* ptr to vector */
static volatile void  (** const buserrvPtrGbl)(void) =
           (volatile void  (**)(void)) BUSERRV; /* ptr to vector */

/* Timer 1-2 stuff */
static volatile TimerGenConfigReg  * const tgcrPtrGbl =
           (volatile TimerGenConfigReg *) TGCR;

static volatile uint32  * const trr12PtrGbl =
           (volatile uint32 *) TRR1; /* cascaded 32 bit ref */
static volatile uint32  * const tcn12PtrGbl =
           (volatile uint32 *) TCN1; /* cascaded 32 bit timer */

static volatile TimerModeReg  * const tmr2PtrGbl =
           (volatile TimerModeReg *) TMR2;
static volatile uint16  * const trr2PtrGbl =
           (volatile uint16 *) TRR2;
static volatile uint16  * const tcr2PtrGbl =
           (volatile uint16 *) TCR2;
static volatile uint16  * const tcn2PtrGbl =
           (volatile uint16 *) TCN2;
static volatile TimerEventReg  * const ter2PtrGbl =
           (volatile TimerEventReg *) TER2;

/********************************************************************
Section: Globals
********************************************************************/
/* parameters for SPMIsr execution history for debugging purposes */
volatile MACParam macGbl;   /* extern in physical.h */
volatile SPMParam spmGbl;   /* extern in physical.h */
volatile CRCParam crcGbl;

volatile History hBufGbl = {0}; /* Initialize to zero */

/* exception count for debugging puposes. Incremented in exception
   service routines for bus error and spurious interrupt */
volatile uint32 exceptions = 0;

/* Timer for PHYIO */
LonTimer phyIOTimer;

/*-------------------------------------------------------------------
Section: Local Function Prototypes
-------------------------------------------------------------------*/
/* Special Purpose Mode Init */
static int16   SPMInit(void);

/* Special Purpose Mode Int. Service Routine */
static void    SPMIsr(void);

/* CPM int error Int. Service Routive */
static void    CErrIsr(void);

/* Spurious interrupt error ISR */
static void    SpurIntIsr(void);

/* bus error ISR */
static void    BusErrIsr(void);

/* Initialize MAC hardware timer */
static uint32 * MACTimerInit(void);

/* delay function waits for delay ticks */
static Boolean DelayTicks(uint32 delay);

/* uses MACTimer hardware timer */
static void    UpdateElapsedTimer(TimerData32 * t);
static void    StartElapsedTimer(TimerData32 * t);
/* increments backlog */
static void    IncrementBacklog(uint8 deltaBacklog);
/* decrement backlog */
static void    DecrementBacklog(uint8 deltaBacklog);

/*-------------------------------------------------------------------
Section: Function Definitions
-------------------------------------------------------------------*/
/* #define SPM_TEST */    /* To include a main to test SPM only */
/* #define SPM_HISTORY */ /* To debug without break points */


/*****************************************************************
Function:  main
Returns:
Reference:
Purpose:   To test mac layer only
Comments:
******************************************************************/

#ifdef SPM_TEST
void main()
{
   long count = 0;

   PHYInitSPM(TRUE);


   macGbl.tl = 8;  /* 5 byte packet */
   macGbl.tc = 0;
   macGbl.tPkt[0] = 1;
   macGbl.tPkt[1] = 2;
   macGbl.tPkt[2] = 3;
   macGbl.tPkt[3] = 4;
   macGbl.tPkt[4] = 5;
   macGbl.tPkt[5] = 6;
   macGbl.tPkt[6] = 7;
   macGbl.tPkt[7] = 8;
   macGbl.tpr = TRUE;

   count = 0;
   while (macGbl.tpr != FALSE )
   {
      count++;
   }


   count = 0;
   while (spmGbl.mode != STOP )
   {
      count++;
   }
   return;
}

#endif

/*****************************************************************
Function:  PHYInitSPM
Returns:
Reference:
Purpose:   Set up special purpose mode
           SPMInit() initializes all the 68360 registers
           MACTimerInit() starts up a 32 bit hardware timer (25Mhz)
           PHYEnableSPMIsr() configures the Interrupt Service Routine
Comments:
******************************************************************/
void PHYInitSPM(BOOLEAN firstReset)
{
	if (firstReset)
	{
		int16 initOK = 0;

		exceptions = 0; /* for debugging */

		initOK = SPMInit();

		/* initialize and start MAC hardware timer */
		spmGbl.clock = MACTimerInit();

		/* reconfigure SPI SPMODE */
		/* spmodePtrGbl->loop  = 1; */   /* local loop back for test */

		hBufGbl.index = 0;  /* start history at 0 */
	}

	/* updates communications parameters
		brings out of reset and starts frame exchange
		and loads config regs
	*/
	PHYEnableSPMIsr();
}

/*****************************************************************
Function:  SPMInit
Returns:   1 if ok, 0 if error.
Reference:
Purpose:    set up special purpose mode
            Basic approach: Use SPI to tx and rx frames from xcvr.
            The spi bit clock is wired externally to clk3 input
            This is used to clock the scc2 port which generates
            the frame clock.
Comments:
******************************************************************/
static int16 SPMInit()
{
   int16 i;                       /* for loop counter */
   SDMAConfigReg sdcrTemp;        /* scratch to set up sdcr */
   CPCommandReg crTemp;           /* scratch to set up cr */
   SCCReceiveBD srbdTemp;         /* scratch */
   SCCTransmitBD stbdTemp;        /* scratch */
   SCCEventMaskReg scceTemp;      /* scratch */
   CIConfigReg cicrTemp;          /* scratch to set up cicr */
   SPIEventMaskReg spiemTemp;     /* scratch to set up spie or spim*/
   SPIModeReg spmodeTemp;         /* scratch to set up spmode */

   /* initialize registers and vectors*/

   /* initialize SDMA Config Register */
   sdcrTemp.frz = 0;       /* ignore freeze */
   sdcrTemp.sism = 7;      /* level 7 interrupt mask */
   sdcrTemp.said = 4;      /* level 4 bus arbitration priority */
   sdcrTemp.inte = 0;      /* no error interrupts */
   sdcrTemp.intb = 0;      /* no break interrupts */
   *sdcrPtrGbl = sdcrTemp; /* write out sdcr */


   /* set sicr */
   sicrPtrGbl->sc2 = 0;    /* scc2 in nmsi mode */
   sicrPtrGbl->r2cs = 6;   /* receive clock from clk3 */
   sicrPtrGbl->t2cs = 6;   /* transmit clock from clk3 */

   /****************************************************/
   /* initialize SCC2 Port for frame clock*/
   /*****************************************************/
   /* initialize RBASE and TBASE in SCC2 Param RAM */
   scc2ParamPtrGbl->rbase = (uint16)  RBD_BASE_SCC2;
   scc2ParamPtrGbl->tbase = (uint16)  TBD_BASE_SCC2;

   /* initialize other parts of SCC2 parameter ram using CP command */
   crTemp.reset =0;                 /* no cpm reset */
   crTemp.chnum = SCC2_CH_NUM;      /* SCC2 Channel */
   crTemp.opcode = INIT_TRP_OPCODE; /* init param ram */
   crTemp.flag = 1;                 /* start command */
   *crPtrGbl = crTemp;              /* write to command register */

#ifndef SIMULATION
   /* wait for command to finish */
   while ( crPtrGbl->flag == 1)
   {
      ; /* Do nothing */
   }
#endif

   /* initialize function codes for param ram  and buffer length
      for scc2 */
   /* endian and function code */
   scc2ParamPtrGbl->rfcr = (uint8) FCR_INIT;
   /* endian and function code */
   scc2ParamPtrGbl->tfcr = (uint8) FCR_INIT;
   /*  max receive buffer length */
   scc2ParamPtrGbl->mrblr = (uint16) 2;

   /* initialize scc receive buffer descriptors */
   srbdTemp.e = 1;          /* 1 = buffer ready to receive */
   srbdTemp.w = 0;          /* 0 = don't wrap this bd */
   srbdTemp.i = 0;          /* 0 = disable interrupts */
   srbdTemp.l = 0;          /* 0 = not last */
   srbdTemp.cm = 1;         /* 1 = put in continuous mode */
   srbdTemp.de = 0;         /* clear out dpll error*/
   srbdTemp.no = 0;         /* clear out non octet error*/
   srbdTemp.cr = 0;         /* clear out crc error*/
   srbdTemp.ov = 0;         /* clear out receiver overrun error */
   srbdTemp.cd = 0;         /* clear out carrier detect lost error */
   srbdTemp.dataLen = 0;    /* superfluous  set by cp */
   /* set to first buffer but change below */
   srbdTemp.dataPtr = &scc2rBufGbl[0][0];

   for (i = 0; i < NUM_BD_SCC2; i++) /* loop thru buf descriptors */
   {
      scc2rbdPtrGbl[i] = srbdTemp; /* assign defaults from temp */
      /* assign ptr to buffer */
      scc2rbdPtrGbl[i].dataPtr = &scc2rBufGbl[i][0];
   }
   scc2rbdPtrGbl[NUM_BD_SCC2 - 1].w = 1; /* wrap last rbd */

   /* initialize scc transmit buffer descriptors */
   stbdTemp.r = 1;          /* 1 = buffer ready to transmit */
   stbdTemp.w = 0;          /* 0 = don't wrap this bd */
   stbdTemp.i = 0;          /* 0 = disable interrupts  */
   stbdTemp.l = 0;          /* 0 = not last */
   stbdTemp.tc = 0;         /* 0 = don't transmit crc */
   stbdTemp.cm = 1;         /* 1 = put in continuous mode */
   stbdTemp.un = 0;         /* clear out transmit underrunn error */
   stbdTemp.ct = 0;         /* clear out cts lost error */
   stbdTemp.dataLen = 2;    /* set to buffer length should be
                               2 bytes */
   /* set to first buffer but change below */
   stbdTemp.dataPtr = &scc2tBufGbl[0][0];

   for (i = 0; i < NUM_BD_SCC2; i++) /* loop thru buf descriptors */
   {
      scc2tbdPtrGbl[i] = stbdTemp; /* assign defaults from temp */
      /* assign ptr to buffer */
      scc2tbdPtrGbl[i].dataPtr = &scc2tBufGbl[i][0];
   }
   scc2tbdPtrGbl[NUM_BD_SCC2 - 1].w = 1; /* wrap last tbd */

   /* initialize port A and C for SCC2 Transparent mode operation */
   padirPtrGbl->dr2 = 0;   /* RXD for scc2 */
   paparPtrGbl->dd2 = 1;   /* rxd connect internal */

   paodrPtrGbl->od3 = 0;   /* not open drain  for txd */
   padirPtrGbl->dr3 = 0;   /* TXD for scc2 */
   paparPtrGbl->dd3 = 1;   /* txd  connect internal */

   padirPtrGbl->dr10 = 0;  /* clk3 sc2 */
   paparPtrGbl->dd10 = 1;  /*  connect internal */
   /* enable rts for scc2 */
   pcdirPtrGbl->dr1 = 0;
   pcparPtrGbl->dd1 = 1;
   /* general purpose io so CD always asserted */
   pcdirPtrGbl->dr7 = 1;
   pcparPtrGbl->dd7 = 0;
   pcsoPtrGbl->cd2 = 0;
   /* use cts2 to initiate transmission. configure as gen purpose
      io so always asserted low */
   pcdirPtrGbl->dr6 = 1;
   pcparPtrGbl->dd6 = 0;
   pcsoPtrGbl->cts2 = 0; /* configure as low always */

   /* set gsmr */

   gsmrh2PtrGbl->tcrc = 0; /* 16 bit ccitt crc */
   gsmrh2PtrGbl->revd = 1; /* 1= send msbit of each byte out first */
   gsmrh2PtrGbl->trx = 1;  /* receiver in transparent mode */
   gsmrh2PtrGbl->ttx = 1;  /* transmitter in transparent mode */
   gsmrh2PtrGbl->cdp = 1;  /* cd pulse mode */
   gsmrh2PtrGbl->ctsp = 1; /* cts pulse mode */
   gsmrh2PtrGbl->cds = 1;  /* cd synchronous  mode */
   gsmrh2PtrGbl->ctss = 1; /* cts synchronous mode */
   gsmrh2PtrGbl->synl = 0; /* receive sync on cd not on
                              sync pattern */
   gsmrh2PtrGbl->tfl = 1;  /* 1 byte fifo for lower transmit
                              latency  */

   gsmrl2PtrGbl->diag = 1;  /* local loop back for test*/

   /* initialize transparent mode crc type */
   scc2ParamPtrGbl->crcp = 0x0000FFFFUL;  /* 16 bit CCITT CRC */
   scc2ParamPtrGbl->crcc = 0x0000F0B8UL;   /* 16 bit CCITT CRC */

   /* clear any previous scc2 interrupt events, write 1 to clear */
   scceTemp.glr = 1;
   scceTemp.glt = 1;
   scceTemp.dcc = 1;
   scceTemp.gra = 1;
   scceTemp.txe = 1;
   scceTemp.rch = 1;
   scceTemp.bsy = 1;
   scceTemp.tx  = 1;
   scceTemp.rx  = 1;
   *scce2PtrGbl = scceTemp;  /* write it out to SCCE2 */

   /* enable/disable interrupts write 1 to enable */
   scceTemp.glr = 0;
   scceTemp.glt = 0;
   scceTemp.dcc = 0;
   scceTemp.gra = 0;
   scceTemp.txe = 0;
   scceTemp.rch = 0;
   scceTemp.bsy = 0;
   scceTemp.tx  = 0;
   scceTemp.rx  = 0;
   *sccm2PtrGbl = scceTemp;  /* write it out to sccm2 */

   /* clear any old scc2 interrupts from CISR */
   /* clear by writing 1, spi interrupt in service */
   cisrPtrGbl->scc2 = 1;

   /* enable/disable system interrupt for spi in CIMR*/
   cimrPtrGbl->scc2 = 0;           /* enable by writing 1 */

   /********************************************/
   /*   SPI Port Initialization */
   /*******************************************/

   /* initialize RBASE and TBASE in SPI Param RAM */
   /* start of Receive BDs */
   spiParamPtrGbl->rbase = (uint16) RBD_BASE;
   /* start of transmit BDs */
   spiParamPtrGbl->tbase = (uint16) TBD_BASE;

   /* initialize other parts of SPI parameter ram using CP command */
   crTemp.reset =0; /* no cpm reset */
   crTemp.chnum = SPI_CH_NUM; /* SPI Channel */
   crTemp.opcode = INIT_TRP_OPCODE; /* init param ram */
   crTemp.flag = 1; /* start command */
   *crPtrGbl = crTemp; /* write to command register */

#ifndef  SIMULATION
   /* wait for command to finish */
   while ( crPtrGbl->flag == 1)
   {
      ;
   }
#endif

   /* initialize function codes for param ram  and buffer length*/
   /* endian and function code */
   spiParamPtrGbl->rfcr = (uint8) FCR_INIT;
   /* endian and function code */
   spiParamPtrGbl->tfcr = (uint8) FCR_INIT;
   /* maximum buffer length */
   spiParamPtrGbl->mrblr = (uint16) MAX_BUF_LEN;

   /* initialize receive buffer descriptors */
   rbdPtrGbl->e = 1;           /* 1 = buffer ready to receive */
   rbdPtrGbl->w = 1;           /* 1 = wrap this bd */
   rbdPtrGbl->i = 1;           /* 1 = enable interrupts of rxb */
   rbdPtrGbl->cm = 1;          /* 1 = continuous mode */
   rbdPtrGbl->l = 0;           /* clear out */
   rbdPtrGbl->ov = 0;          /* clear out */
   rbdPtrGbl->me = 0;          /* clear out */
   rbdPtrGbl->dataLen = 0;     /* superfluous set by cp*/
   rbdPtrGbl->dataPtr = &spmGbl.rf;        /* assign ptr to buffer */

   /* initialize transmit buffer descriptors */
   tbdPtrGbl->r = 1; /* 1 = buffer ready to transmit */
   tbdPtrGbl->w = 1; /* 1 = wrap this bd */
   tbdPtrGbl->i = 0; /* 0 = don't enable interrupts of txb */
   tbdPtrGbl->cm = 1; /* 1 = continuous mode */
   tbdPtrGbl->l = 1; /* 1 = last  */
   tbdPtrGbl->un = 0; /* clear out */
   tbdPtrGbl->me = 0; /* clear out */
   tbdPtrGbl->dataLen = 2; /* set to buffer length should be 2bytes */
   tbdPtrGbl->dataPtr = &spmGbl.tf; /* assign ptr to buffer */

/********************************************************************
  Initialize Globals Structure for Channel Access Algorithm
  and Special Purpose Mode
********************************************************************/

   /* initialize receive frame buffers just to prevent garbage
      in case out of sync */
   spmGbl.rf.setTxFlag = 0;    /* frame status not transmitting */
   spmGbl.rf.clrTxReqFlag = 0; /* don't clear */
   spmGbl.rf.rxDataValid = 0;  /* no valid data this frame */
   spmGbl.rf.txDataCTS = 0;    /* no clear to send */
   spmGbl.rf.setCollDet = 0;   /* no collision detected */
   spmGbl.rf.rxFlag = 0;       /* not receiving */
   spmGbl.rf.rwAck = 0;        /* not acknowledged */
   spmGbl.rf.txOn = 0;         /* not receiving */
   spmGbl.rf.data = 0;         /*  */

   /* initialize receive frame buffers just to prevent garbage in case out of sync*/
   spmGbl.tf.txFlag = 0;      /* frame status not transmitting */
   spmGbl.tf.txReqFlag = 0;   /* don't req */
   spmGbl.tf.txDataValid = 0; /* no valid data this frame */
   spmGbl.tf.blank = 0;       /* unused */
   spmGbl.tf.txAddrRW = 0;    /* write */
   spmGbl.tf.txAddr = 0;      /* default address */
   spmGbl.tf.data = 0;        /*  */

   /* Initialize spm global parameters */

   spmGbl.mode = STOP;     /* no errors yet */
   spmGbl.state = IDLE;    /* state machine start state */
   macGbl.tpr = FALSE;     /* packet not ready to transmit */
   macGbl.tc = 0;          /* transmit byte count 0 */
   macGbl.tl = 0;          /* last byte is 0 */
   macGbl.rpr = FALSE;     /* packet not yet received */
   macGbl.rc = 0;          /* receive byte count 0 */
   macGbl.rl = 0;          /* last byte is 0 */
   spmGbl.crw = FALSE;     /* don't write config register */
   spmGbl.cra = 0;         /* address zero nothing */
   spmGbl.crData = 0;      /* empty data */
   spmGbl.srr = FALSE;     /* don't read from status register */
   spmGbl.sra = 0;         /* empty address */
   spmGbl.srData = 0;      /* put nothing in */
   spmGbl.resetCount = 0;  /* zero to start */
   spmGbl.collisionsThisPkt = 0 ; /* # of collisions this packet */
   macGbl.priorityPkt = FALSE; /* FALSE = not a priority packet */

   /* FALSE = channel access algo not complete */
   spmGbl.accessApproved = FALSE;
   /* TRUE means cycle timer to be reset and started */
   spmGbl.cycleTimerRestart = TRUE;

   spmGbl.backlog = 0;  /* current channel backlog */
   /* delta backlog on current transmit packet */
   macGbl.deltaBLTx = 0;
   macGbl.deltaBLRx = 0;  /* delta backlog on last receive packet */
   macGbl.altPathBit = 0; /* alternate path bit */
   /* signal to write alternate path bit */
   spmGbl.writeAltPathBit = FALSE;
   /* alt path bit written state for this pkt */
   spmGbl.altPathBitWritten = FALSE;
   spmGbl.nodePriority = 0; /* node's priority slot number */
   /* comm parameters for this node */
   for ( i = 0; i < NUM_COMM_PARAMS; i++)
   {
      spmGbl.configData[i] = 0;
   }
   spmGbl.phase = RANDOM_IDLE; /* Idle for a long time */

   spmGbl.kind = POST_RX;    /* beta1 time slot type */
   spmGbl.nicsToTicks = 15;  /* conversion factor this specification's
                                time base to 68360 ticks */
   spmGbl.bitClockRate = 156250; /* in units of Hz */
   spmGbl.beta2Ticks = 0;   /* length of beta2 in 68360 ticks
                               40ns each */
   spmGbl.beta1Ticks = 0;   /* length of beta1 in 68360 ticks
                               40ns each */
   spmGbl.beta1PostTxTicks = 0;   /* length of beta1 in 68360 ticks
                                     40ns each */
   spmGbl.beta1PostRxTicks = 0;   /* length of beta1 in 68360 ticks
                                     40ns each */
   spmGbl.baseTicks = 0; /* duration of wbase in 68360 ticks
                            40ns each */
   spmGbl.cycleTicks = 0;   /* duration of avg packet cycle in
                               68360 ticks 40ns each */
   spmGbl.priorityChPostTxTicks = 0; /* duration of channel priority
                                       slots */
   spmGbl.priorityChPostRxTicks = 0; /* duration of channel priority
                                       slots */
   spmGbl.priorityIdleTicks = 0;  /* duration of priority idle wait */
   /* duration until node's priority slot */
   spmGbl.priorityNodeTicks  = 0;
   spmGbl.randomTicks        = 0;
   /* timers for channel access algorithm */
   spmGbl.idleTimerStart     = 0;
   spmGbl.baseTimerStart     = 0;
   spmGbl.cycleTimerStart    = 0;
   spmGbl.transmitTimerStart = 0;
   spmGbl.elapsed            = 0;
   spmGbl.stopped            = 0;
   spmGbl.lastTime           = 0;

   /* initialize crcGbl  for good form */
   crcGbl.poly = 0;
   crcGbl.crc = 0;
   crcGbl.crcBit = 0;
   crcGbl.dataBit = 0;
   crcGbl.dataByte = 0;

   /* initialize port B for SPI Master operation */
   /* pin0 = spisel or chip select if single master */
   /* pin1 = spiclk   clock*/
   /* pin2 = spimosi master out slave in */
   /* pin3 = spimiso master in slave out */
   /* pin5 = ~`reset */

   pbodrPtrGbl->od0 = 0;  /*0=active driven 1=open drain(3 state), if output */
   pbodrPtrGbl->od1 = 0;
   pbodrPtrGbl->od2 = 0;
   pbodrPtrGbl->od3 = 0;
   pbodrPtrGbl->od5 = 0;  /* not open drain for reset? */

   pbparPtrGbl->dd0 = 0;   /* general purpose pin chip select*/
   pbparPtrGbl->dd1 = 1;   /* internal connect spiclk */
   pbparPtrGbl->dd2 = 1;   /* internal connect spimosi */
   pbparPtrGbl->dd3 = 1;   /* internal connect spimiso */
   pbparPtrGbl->dd5 = 0;   /* general purpose i/o */

   pbdirPtrGbl->dr0 = 1;   /* output */
   pbdirPtrGbl->dr1 = 1;   /* output */
   pbdirPtrGbl->dr2 = 1;   /* output */
   /* although input must config as out else brgo4 */
   pbdirPtrGbl->dr3 = 1;
   pbdirPtrGbl->dr5 = 1;   /* output */


   /* initialize port B for IO pins : reset switch, manual service request,
      ioswitch, and LEDs reset-service and IOLED
      pb8 = led reset service
      pb9 = general purpose out led
      pb12 = reset switch
      pb13 = manual service request
      pb14 = general purpose in switch
   */

   pbodrPtrGbl->od8 = 0;   /* 0 = active driven not open drain   */
   pbodrPtrGbl->od9 = 0;
   /* since input NA, however open drain for pull down */
   pbodrPtrGbl->od12 = 1;
   /* since input NA, however open drain for pull down */
   pbodrPtrGbl->od13 = 1;
   /* since input NA, however no pull down */
   pbodrPtrGbl->od14 = 0;

   pbparPtrGbl->dd8 = 0;        /* general purpose pin chip select*/
   pbparPtrGbl->dd9 = 0;        /* general purpose pin chip select*/
   pbparPtrGbl->dd12 = 0;       /* general purpose pin chip select*/
   pbparPtrGbl->dd13 = 0;       /* general purpose pin chip select*/
   pbparPtrGbl->dd14 = 0;       /* general purpose pin chip select*/

   pbdirPtrGbl->dr8 = 1;        /* output */
   pbdirPtrGbl->dr9 = 1;        /* output */
   pbdirPtrGbl->dr12 = 0;       /* input */
   pbdirPtrGbl->dr13 = 0;       /* input */
   pbdirPtrGbl->dr14 = 0;       /* input */

   /* set initial values */
   pbdatPtrGbl->d8 = 0;
   pbdatPtrGbl->d9 = 0;
   pbdatPtrGbl->d12 = 1; /* input so NA */
   pbdatPtrGbl->d13 = 1; /* input so NA */
   pbdatPtrGbl->d14 = 0; /* input so NA */

   /* reset xcvr */
   /* driven low to reset hold in reset until change. */
   pbdatPtrGbl->d5 = 0;

   /* clear any previous spi interrupt events write 1 to clear */
   spiemTemp.mme = 1;
   spiemTemp.txe = 1;
   spiemTemp.bsy = 1;
   spiemTemp.txb = 1;
   spiemTemp.rxb = 1;
   *spiePtrGbl = spiemTemp;  /* write it out to SPIE */

   /* assign address of isr functions to vector table entry */
   *spivPtrGbl = &SPMIsr;         /* spi isr */
   *cerrvPtrGbl = &CErrIsr;       /* comm error isr */
   *spurintvPtrGbl = &SpurIntIsr; /* spurious interrupt isr */
   /* *buserrvPtrGbl = &BusErrIsr; */ /* spurious interrupt isr */

   /* enable interrupts write 1 to enable */
   spiemTemp.mme = 0;
   spiemTemp.txe = 0;
   spiemTemp.bsy = 0;
   spiemTemp.txb = 0;  /* isr will run after transmit */
   spiemTemp.rxb = 1;  /* isr will run after receive buffer filled */
   *spimPtrGbl = spiemTemp;  /* write it out to SPIM */

   /* init CICR register */
   cicrTemp.scdp = 3; /* scc4 has priority d */
   cicrTemp.sccp = 2; /* scc3 has priority c */
   cicrTemp.scbp = 1; /* scc2 has priority b */
   cicrTemp.scap = 0; /* scc1 has priority a */
   cicrTemp.irl  = CPIR_LEVEL; /* level 4 interrupt request level */
   cicrTemp.hpi  = 31; /* default highest priority = 31 = 1F hex*/
   cicrTemp.vba  = UIVN_MSB; /* user interrupt vector number offset */
   cicrTemp.sps  = 0; /* use grouped priority scheme */
   *cicrPtrGbl = cicrTemp; /* write to cicr */

   /* clear any old SPI interrupts from CISR */
   cisrPtrGbl->spi = 1; /* clear by writing 1,
                           spi interrupt in service */
   /* enable system interrupt for spi in CIMR*/
   cimrPtrGbl->spi = 1; /* enable by writing 1 */

   /* init SPMODE register */
   spmodeTemp.loop  = 0; /* 1 = local loop back
                            0 = no local loop back */
   spmodeTemp.ci    = 0; /* don't invert clock */
   spmodeTemp.cp    = 1; /* toggle clock at begin of data transfer */
   spmodeTemp.div16 = 0; /* don't divide brgclk by 16 */
   spmodeTemp.rev   = 1; /* use bigendian MSB first */
   spmodeTemp.ms    = 1; /* master mode */
   spmodeTemp.len   = 7; /* 8 bits per character = len + 1 */
   spmodeTemp.pms   = 4; /* prescale modulus select,
                            4 gives 1.25 Mbps */
   spmodeTemp.en    = 1; /* enable spi  */
   *spmodePtrGbl = spmodeTemp; /* write to spmode register */

   /* initialize SPCOM (probably don't need to do */
   spcomPtrGbl->res = 0;  /* should never have to change */
   spcomPtrGbl->str = 0;  /* superfluous auto cleared one
                             clock cycle after set */

   /***************************************************************
    Enable interrupts by setting interrupt priority mask level
    in Status Register to 0, must use in line assembly code
    assumes CPU is in supervisor mode, Bits 10 9 8 are
    interrupt priority mask level
   ***************************************************************/

   asm(" ANDI #~0x0700,SR"); /* sets priority mask to level 000 */


   /* Set up frame clock using scc2*/
   /* put data into transmit buffer */
   scc2tBufGbl[0][0] = 0x01U;    /* first byte 7 bit delay
                                    to sync frame to spi */
   scc2tBufGbl[0][1] = 0x00U;    /* second byte */
   scc2tbdPtrGbl[0].dataLen = 2; /* send just two bytes */


   /* enable frame clock */
   gsmrl2PtrGbl->enr = 1;  /* receive */
   gsmrl2PtrGbl->ent = 1;  /* transmit */


   return(1);
}


/*****************************************************************
Function:  MACTimerInit
Returns:   pointer to hardware timer counter register
Reference:
Purpose:   initialize MAC  hardware timer
Comments:
******************************************************************/

uint32 * MACTimerInit(void)
{
   tgcrPtrGbl->cas2 = 1; /* cascade 1 and 2 */
   tgcrPtrGbl->frz2 = 0; /* ignore freeze pin */
   tgcrPtrGbl->stp2 = 0; /* normal unstopped operation */
   tgcrPtrGbl->rst2 = 0; /* reset timer */

   tmr2PtrGbl->ps   = 0; /* prescaler */
   tmr2PtrGbl->ce   = 0; /* input capture disabled */
   tmr2PtrGbl->om   = 0; /* output mode pulse */
   tmr2PtrGbl->ori  = 0; /* diable output reference interrupt */
   tmr2PtrGbl->frr  = 0; /* 0= free running counter */
   tmr2PtrGbl->iclk = 1; /* 1 = internal general system clk source */
   tmr2PtrGbl->ge   = 0; /* tgate is ignored */

   /* set reference to max all ones */
   *trr12PtrGbl = (uint32) 0xFFFFFFFFU;

   /* writing a 1 resets the reference event bit */
   ter2PtrGbl->ref = 1;
   /* writing a 1 resets the capture event bit */
   ter2PtrGbl->cap = 1;

   *tcn12PtrGbl = 0; /* start at zero */

   tgcrPtrGbl->rst2 = 1; /* start timer running */

   return(tcn12PtrGbl);
}


/*****************************************************************
Function:  DelayTicks
Returns:   Boolean state of mac timer running = true stopped = false
Reference:
Purpose:   Delay for a "delay" number of ticks
           Used during various startup routines
           to wait for xcvr hardware to respond
Comments:
******************************************************************/

Boolean DelayTicks(uint32 delay)
{
   uint32 start;

   if (tgcrPtrGbl->rst2 == 0)
   {
      /* timer not running so exit immediately with false */
      return (FALSE);
   }
   else
   {
      start = *tcn12PtrGbl;

      while ((*tcn12PtrGbl - start) < delay)
      {
         ; /* keep waiting */
      }

      return (TRUE);
   }
}


/*****************************************************************
Function:  PHYSoftResetSPMXCVR
Returns:
Reference:
Purpose:  Software reset Special Purpose Mode XCVR by reading
          from register zero
Comments:
******************************************************************/

void PHYSoftResetSPMXCVR(void)
{
   uint16 count;

   spmGbl.sra = 0;
   spmGbl.srr = TRUE;

   count = 0;
   while ((spmGbl.srr != FALSE) && (count < 0xFFFF))
   {
      count++;
   }
   spmGbl.srData = 0;

   return;
}

/*****************************************************************
Function:  PHYHardResetSPMXCVR
Returns:
Reference:
Purpose:   hardware reset Special Purpose Mode XCVR by
           asserting reset pin. then configures xcvr
           this is used when xcvr hangs in tx_on mode
           e.g. spmGbl.resetCount times out
Comments:
******************************************************************/

void PHYHardResetSPMXCVR(void)
{
   unsigned long count;
   int i;
   Boolean good;


   pbdatPtrGbl->d5 = 0; /* drive xcvr into reset */

   /* wait for a while for everything to settle down */
   good = DelayTicks(RESTART_DELAY_TICKS);


   pbdatPtrGbl->d5   = 1; /* bring xcvr out of reset */

   /* initialize config registers on PLT-20 transceiver */

   /* load in reverse order */
   i = NUM_COMM_PARAMS - 1; /* should be 6 */
   while ( i >= 0)
   {
      spmGbl.crData = spmGbl.configData[i];
      /* config registers start at 7 down to 1 */
      spmGbl.cra = (Byte) i + 1;
      spmGbl.crw = TRUE;
      count = 0;
      /* wait for ack but time out so not infinite loop */
      while ((spmGbl.crw != FALSE) && (count < 0xFFFFF))
      {
         count++;
      }
      if (spmGbl.crw == TRUE) /* xcvr never acked try again */
      {
         pbdatPtrGbl->d5   = 0; /* reset xcvr */

         /* wait for a while for everything to settle down */
         good = DelayTicks(RESTART_DELAY_TICKS);


         pbdatPtrGbl->d5   = 1; /* bring xcvr out of reset */
         i = NUM_COMM_PARAMS - 1; /* start over again */
      } /* NOTE!!! if the xcvr never acks this code will hang */
      else
      {
         i--;
      }
   }

   spmGbl.crData = 0;
   spmGbl.cra = 0;

   return;
}

/*****************************************************************
Function:  PHYDisableSPMIsr
Returns:
Reference:
Purpose:   To diable the Isr temporarily on node reset.
           Must use PHYEnableIsr to reenable correctly.
Comments:
******************************************************************/

void PHYDisableSPMIsr(void)
{
   /* once the first line below is executed, the ISR will stop
      running. the next time it runs */
   spmGbl.mode  = STOP;

   pbdatPtrGbl->d5 = 0; /* drive xcvr into reset */

   spmGbl.state = DEBUG;


   macGbl.tpr   = FALSE;
   macGbl.tc    = 0;
   macGbl.tl    = 0;
   macGbl.rpr   = FALSE;
   macGbl.rc    = 0;
   macGbl.rl    = 0;
   spmGbl.crw   = FALSE;
   spmGbl.srr   = FALSE;
   spmGbl.sra = 0;

   return;
}

/*****************************************************************
Function:  PHYEnableSPMIsr
Returns:
Reference:
Purpose:   To Enable the Isr after Being disabled on node reset
           Also to enable on powerup etc.
           Computes clock rates etc based on node comm parameters
Comments:
******************************************************************/

void PHYEnableSPMIsr(void)
{
   long  i;
   uint32 inputClock;
   uint32 commClock;
   uint32 rxPad;
   uint32 txPad;
   uint32 beta2;
   Boolean good;

   /* compute conversion factor nics_To_Tick  */
   /* converts from this specification's time base to 683660 ticks */
   inputClock = (uint32) eep->configData.inputClock;
   commClock = (uint32) eep->configData.commClock;
   spmGbl.nicsToTicks = NICS_TICKS_BASE >> inputClock;

   /* compute channel access algorithm parameters */

   beta2 = (eep->configData.reserved[2] * 20) + 40;

   /* compute beta1 = postpacket + networkidlewait +
                      interpacketpad + prepacket */

   if (eep->configData.reserved[4] < 128)
   {
      rxPad = eep->configData.reserved[4] * 41;
   }
   else
   {
      rxPad = (eep->configData.reserved[4] - 128) * 145;
   }

   if (eep->configData.reserved[3] < 128)
   {
      txPad = eep->configData.reserved[3] * 41;
   }
   else
   {
      txPad = (eep->configData.reserved[3] - 128) * 145;
   }

   /* set beta1 time as per formula with implementation
      specific adjustment BETA1_ADJUST_TICKS */
   spmGbl.beta1PostRxTicks =
      (285 + beta2 + rxPad + 317) * spmGbl.nicsToTicks +
      BETA1_ADJUST_TICKS;
   spmGbl.beta1PostTxTicks =
      (307 + beta2 + txPad + 317) * spmGbl.nicsToTicks +
      BETA1_ADJUST_TICKS;

   /* set beta2 time as per formula with implementation
      specific adjustment BETA2_ADJUST_TICKS */
   spmGbl.beta2Ticks =
      beta2 * spmGbl.nicsToTicks + BETA2_ADJUST_TICKS;
   spmGbl.baseTicks = spmGbl.beta2Ticks * W_BASE;
   /* set cycle time as per formula with implementation
      specific adjustment CYCLE_ADJUST_TICKS */
   spmGbl.cycleTicks =
      eep->configData.reserved[1] * 1794 * spmGbl.nicsToTicks;

   spmGbl.beta1Ticks = 0; /* this is set by isr to either post rx or
                             post tx beta1 as appropriate */

   spmGbl.nodePriority =
      eep->configData.nodePriority; /* set node's priority slot # */

   /* Add W_Base slots to wait time for post tx access only when there
      are non zero number of channel priority slots */
   if (eep->configData.channelPriorities > 0)
   {
       /* duration of channel priority slots */
      spmGbl.priorityChPostTxTicks = (eep->configData.channelPriorities +
                                       W_BASE ) * spmGbl.beta2Ticks;
       /* duration of channel priority slots */
      spmGbl.priorityChPostRxTicks = eep->configData.channelPriorities
                                 * spmGbl.beta2Ticks;
   }
   else
   {
      /* duration of channel priority slots */
      spmGbl.priorityChPostTxTicks = 0;
      spmGbl.priorityChPostRxTicks = 0;
   }
   /* duration of priority idle wait */
   spmGbl.priorityIdleTicks = spmGbl.priorityChPostTxTicks;

   /* set time of nodes priority slot. If eep->configData.nodePriority is 0,
      then this field will be negative, but it is not used in this case. */
   spmGbl.priorityNodeTicks =
      (eep->configData.nodePriority - 1) * spmGbl.beta2Ticks;
   spmGbl.randomTicks = 0;
   spmGbl.bitClockRate = (BIT_CLOCK_BASE << inputClock) >>
      (RATIO_BASE + commClock);


   /* make local copy of xcvr parameters so can reset xcvr free of changes to eep
      without following node reset. Needed in case xcvr locks up, See
      PHYHardResetSPMXCVR(void)
      */
   for (i = 0; i < NUM_COMM_PARAMS; i++)
   {
      spmGbl.configData[i] = eep->configData.param.xcvrParams[i];
   }

   /* reinitialize remainder of spmGbl */
   spmGbl.mode = RUN;      /* get ready to go */
   spmGbl.state = IDLE;    /* state machine start state */
   macGbl.tpr = FALSE;     /* packet not ready to transmit */
   macGbl.tc = 0;          /* transmit byte count 0 */
   macGbl.tl = 0;          /* last byte is 0 */
   macGbl.rpr = FALSE;     /* packet not yet received */
   macGbl.rc = 0;          /* receive byte count 0 */
   macGbl.rl = 0;          /* last byte is 0 */
   spmGbl.crw = FALSE;     /* don't write config register */
   spmGbl.cra = 0;         /* address zero nothing */
   spmGbl.crData = 0;      /* empty data */
   spmGbl.srr = FALSE;     /* don't read from status register */
   spmGbl.sra = 0;         /* empty address */
   spmGbl.srData = 0;      /* put nothing in */
   spmGbl.resetCount = 0;  /* zero to start */
   spmGbl.collisionsThisPkt = 0 ; /* # of collisions this packet */
   macGbl.altPathBit = 0; /* alternate path bit */
   spmGbl.writeAltPathBit = FALSE; /* signal to write
                                      alternate path bit */
   spmGbl.altPathBitWritten = FALSE; /* alt path bit written
                                        state for this pkt */
   macGbl.priorityPkt = FALSE; /* FALSE = not a priority packet */

   spmGbl.accessApproved = FALSE; /* FALSE = channel access algo
                                     not complete */
   spmGbl.cycleTimerRestart = TRUE;    /* indicates whether cycle
                                      timer can be updated */

   spmGbl.backlog = 0;  /* current channel backlog */
   macGbl.deltaBLTx = 0;  /* delta backlog on curr transmit packet */
   macGbl.deltaBLRx = 0;  /* delta backlog on last receive packet */
   spmGbl.phase = RANDOM_IDLE; /* Idle for a long time */

   spmGbl.kind = POST_RX;    /* beta1 time slot type */
   spmGbl.idleTimerStart = 0;
   spmGbl.baseTimerStart = 0;
   spmGbl.transmitTimerStart = 0;
   spmGbl.elapsed = 0;
   /* spmGbl.clock is initialized in PHYInitSPM */
   spmGbl.stopped = *(spmGbl.clock);
   spmGbl.cycleTimerStart = spmGbl.stopped; /* start up 1st time */
   spmGbl.lastTime = spmGbl.stopped;


   /* configure bit clock */
   /* div16 = 1 and pms = 4 gives 78.125 khz */
   /* div16 = 1 and pms = 3 gives 97.656 khz */
   /* div16 = 1 and pms = 2 gives 130.208 khz */
   /* div16 = 1 and pms = 1 gives 195.312 khz */
   /* div16 = 1 and pms = 0 gives 390.625 khz */
   /* div16 = 0 and pms = 9 gives 625.000 khz */

/****************************************************************
      Since the 360 implementation runs in noncontinuous mode the
      effective frame period is the actual frame period plus the
      processing time for the ISR at the end of the frame. Therefore
      we need to run at a higher bit clock than specified by the
      network management so that the effective frame rate is the same
      or better than the network manager specifies. For a bit rate of
      156.25 Khz the frame period is 16 * 6.4 usec = 102.4
      usec. Suppose the worst case time for ISR is 58 micro seconds
      then the time left over for the actual frame is 102.4 - 58 =
      44.4 usec. This corresponds to a bit rate of 16 * 1/(44.4 usec)
      = 360.36 khz. The closest bit rate greater than this that the
      360 supports is 390.625 khz. The effective frame period becomes
      58 usec + (16 * (1/390.625 khz)) = 98.96 usec. This is faster so
      OK. Effective bit rate is 16 * 1/(98.96 usec) = 161.68 kHz.
      Unless we can more than half the time for the ISR to run we
      can't run fast enough to make the next step which is the 312.5
      Khz bit rate.
******************************************************************/

   switch (spmGbl.bitClockRate)
   {
      case 78125:
         /* run at 130 khz to get same or faster effective rate */
         spmodePtrGbl->div16 = 1; /* divide by 16  */
         spmodePtrGbl->pms   = 2; /* prescale modulus select */
         break;
      case 156250:
         /* run at 390 khz to get same or faster effective rate */
         spmodePtrGbl->div16 = 1; /* divide by 16  */
         spmodePtrGbl->pms   = 0; /* prescale modulus select */
         break;
      default:
         /* 156250 */
         /* run at 390 khz to get same or faster eff rate */
         spmodePtrGbl->div16 = 1; /* divide by 16  */
         spmodePtrGbl->pms   = 0; /* prescale modulus select */
         break;
   }

   /* reinitialize receive frame buffers */
   spmGbl.rf.setTxFlag    = 0; /* frame status not transmitting */
   spmGbl.rf.clrTxReqFlag = 0; /* don't clear */
   spmGbl.rf.rxDataValid  = 0; /* no valid data this frame */
   spmGbl.rf.txDataCTS    = 0; /* no clear to send */
   spmGbl.rf.setCollDet   = 0; /* no collision detected */
   spmGbl.rf.rxFlag       = 0; /* not receiving */
   spmGbl.rf.rwAck        = 0; /* not acknowledged */
   spmGbl.rf.txOn         = 0; /* not receiving */
   spmGbl.rf.data         = 0; /*  */

   /* reinitialize receive frame buffers */
   spmGbl.tf.txFlag      = 0; /* frame status not transmitting */
   spmGbl.tf.txReqFlag   = 0; /* don't req */
   spmGbl.tf.txDataValid = 0; /* no valid data this frame */
   spmGbl.tf.blank       = 0; /* unused */
   spmGbl.tf.txAddrRW    = 0; /* write */
   spmGbl.tf.txAddr      = 0; /* default address */
   spmGbl.tf.data        = 0; /*  */

   /* initialize crcGbl  for good form */
   crcGbl.poly = 0;
   crcGbl.crc = 0;
   crcGbl.crcBit = 0;
   crcGbl.dataBit = 0;
   crcGbl.dataByte = 0;


   /* Send First Frame  ISR keeps it going */
   tbdPtrGbl->r     = 1; /* buffer ready to transmit */
   spcomPtrGbl->str = 1;

   /* wait for a while for everything to settle down */
   good = DelayTicks(RESTART_DELAY_TICKS);


   PHYHardResetSPMXCVR();  /* this also writes config registers */

   return;

}

/*****************************************************************
Function:  PHYIOInit
Returns:   None
Reference: None
Purpose:   To Init variables related to PHYIO function.
Comments:  None
******************************************************************/
void PHYIOInit(void)
{
   SetLonTimer(&phyIOTimer, (uint16)(PHYIO_CHECK_INTERVAL * 1000));
   gp->resetPinPrevState    = 1; /* Allow Reset Button Push */
   gp->manualServiceRequestPrevState  = 1; /* Allow Manual Service Request Push */
   gp->ioInputPin0PrevState = 1;
}

/*****************************************************************
Function:  PHYIO
Returns:   None
Reference: None
Purpose:   To handle Input switches (reset service and IO) and
           to set value of ouput leds
           Manual Service Request and Reset pin share same LED to indicate
           switch pushed
Comments:  None
******************************************************************/
void PHYIO(void)
{
	static BOOLEAN phyioInitialized = FALSE;

	if (!phyioInitialized)
	{
	   PHYIOInit(); 
		phyioInitialized = TRUE;
	}

   /* Perform Pin related checks only on timer expiry
      to avoid bounce problem */
   if (LonTimerExpired(&phyIOTimer))
   {
      /* check reset switch */
      /* reset pulled low &&
         reset not enabled so need to enable &&
         have not yet set this pin push */
      if (( pbdatPtrGbl->d12 == 0) &&
          !gp->resetNode &&
          (gp->resetPinPrevState != 0))
      {
         gp->resetNode = TRUE; /* signal scheduler there is reset */
         nmp->resetCause = EXTERNAL_RESET;
         /* NodeReset function will clear when reset is finished */
         /* prevents sending more than 1 reset until pin is toggled */
         gp->resetPinPrevState = 0;
      }

      /* reset pin high */
      if (pbdatPtrGbl->d12 == 1)
      {
         gp->resetPinPrevState = 1;
      }

      if ((pbdatPtrGbl->d13 == 0) && /* manual service request pulled low */
          !gp->manualServiceRequest &&         /* manualServiceRequest not enabled */
          (gp->manualServiceRequestPrevState != 0)) /* have not yet set
                                             this pin push */
      {
         gp->manualServiceRequestPrevState = 0;
         gp->manualServiceRequest = TRUE; /* notify stack that
                                             manual service request enabled */
      }

      if (pbdatPtrGbl->d13 == 1) /* manual service request high */
      {
         gp->manualServiceRequestPrevState = 1; /* manual service request toggled so
                                         next press will allow push */
      }

      if(pbdatPtrGbl->d14 == 1 &&
         (gp->ioInputPin0PrevState != 0))  /* debouncer, check
                                              gen purpose io input */
      {
         gp->ioInputPin0 = 1;
      }
      else
      {
         gp->ioInputPin0 = 0;
      }

      if (pbdatPtrGbl->d14 == 1) /* io pin high */
      {
         gp->ioInputPin0PrevState = 1; /* io pin toggled so next
                                          press will allow push */
      }

      SetLonTimer(&phyIOTimer,
               (uint16)(PHYIO_CHECK_INTERVAL * 1000));
   }


   if (gp->resetNode || gp->manualServiceRequest || gp->ioOutputPin0)
   {
      pbdatPtrGbl->d8 = 1; /* turn on reset-service light */
   }
   else
   {
      pbdatPtrGbl->d8 = 0; /* turn off reset-service light */
   }


   if(gp->ioOutputPin1 == 0) /* set io output pin */
   {
      pbdatPtrGbl->d9 = 0;
   }
   else
   {
      pbdatPtrGbl->d9 = 1;
   }

   return;
}


/*****************************************************************
Function:  IncrementBacklog
Returns:
Reference:
Purpose:   increments backlog and handles backlog overflow
Comments:
******************************************************************/

static void    IncrementBacklog(uint8 deltaBacklog)
{
   if (deltaBacklog > MAX_BACKLOG)
   {
      return; /* Erroneous deltaBacklog value */
   }
   /* spmGbl.backlog size must allow 2 * MAX_BACKLOG without
      wrap around */
   spmGbl.backlog += deltaBacklog;
   if (spmGbl.backlog > MAX_BACKLOG)
   {
      spmGbl.backlog = MAX_BACKLOG;
      INCR_STATS(nmp->stats.backlogOverflow);
   }
}


/*****************************************************************
Function:  DecrementBacklog
Returns:
Reference:
Purpose:   decrements backlog and handles backlog underflow
Comments:
******************************************************************/

static void    DecrementBacklog(uint8 deltaBacklog)
{
   if (deltaBacklog > MAX_BACKLOG)
   {
      return; /* Erroneous deltaBacklog value */
   }
   if (spmGbl.backlog <= deltaBacklog)
   {
      spmGbl.backlog = 0;
   }
   else
   {
      spmGbl.backlog -= deltaBacklog;
   }
}

/*****************************************************************
Function:  SPMIsr
Returns:   none, no arguments allowed either
Reference:
Purpose:
    Interrupt Service Routine

    This ISR serves two primary functions. the first is to execute the
    channel access algorithm. the second is to performs the special
    purpose mode transfers with the XCVR. For the channel access
    algorithm a single hardware timer is used as reference for a set
    of global values which are the expire times for the various
    channel access algorithm timers. The ISR polls the hardware timer
    each time the ISR runs and checks for expiration of the various
    timers.  The ISR has two coupled state machines. the first
    indicated by spmGbl.phase controls the channel access algorithm
    and the second indicated by spmGbl.state controls the SPM
    transfers.

    The ISR runs after each 16 SPI transfers or clock transitions.
    An Interrupt is generated by an rxb event when a receive buffer
    is full.

Comments:  must use interrupt pragma
******************************************************************/
/*  interrupt pragma informs compiler to make next function
    an interrupt handler with RTE and saved state of registers */

#pragma interrupt()

static void SPMIsr()
{
   SPIEventMaskReg spieTemp;  /* local copy of event register */
   int j;  /* for loop index */

   spieTemp = *spiePtrGbl;  /* read SPIE into local copy */

   /* clear spie ASAP so as not to miss any events while in isr */
   /* write all ones to clear */
   *((uint8 * ) spiePtrGbl) = (uint8) 0xFFU;

   /* each event handled by this isr needs a separate if statement */
   if ( spieTemp.rxb == 1)
   {
      /* handle interrupt for receive buffer filled */
      /* transfer can be stopped by physical layer or for error */
      if (spmGbl.mode == STOP) /* stop spm exchanges  */
      {
         spmGbl.state = DEBUG;
         macGbl.tc = 0;
         macGbl.tl = 0;
         macGbl.tpr = FALSE;
         macGbl.rc = 0;
         macGbl.rl = 0;
         macGbl.rpr = FALSE;
         spmGbl.crw = FALSE;
         spmGbl.srr = FALSE;
      }
      else    /* State Machines for Transmit and Receive */
      {

         /* debug */
         /* last received frame is in spmGbl.rf  */
         /* update history for debugging purposes*/
#ifdef SPM_HISTORY
         hBufGbl.records[hBufGbl.index].rf = spmGbl.rf;
         hBufGbl.records[hBufGbl.index].state = spmGbl.state;
         hBufGbl.records[hBufGbl.index].rb = spiParamPtrGbl->rbptr;
         hBufGbl.records[hBufGbl.index].tb = spiParamPtrGbl->tbptr;

         hBufGbl.records[hBufGbl.index].start = *(spmGbl.clock);
#endif
         /***********************************************/
         /* state machine for channel access algorithm */
         /**********************************************/
         switch (spmGbl.phase)
         {
            case BUSY: /* some node is transmitting */
               /* waits here until channel is idle */

               if ((spmGbl.rf.rxFlag == 0) &&
                   (spmGbl.rf.txOn == 0))
               {
                  /* channel has become idle neither receive or transmit */
                  /* start idle timer */
                  spmGbl.idleTimerStart = *(spmGbl.clock);

                  /* set beta1 duration based on last packet,
                     either rx or tx */
                  if (spmGbl.kind == POST_RX)
                  {
                     spmGbl.beta1Ticks = spmGbl.beta1PostRxTicks;
                     spmGbl.priorityIdleTicks = spmGbl.priorityChPostRxTicks;
                  }
                  else
                  {
                     spmGbl.beta1Ticks = spmGbl.beta1PostTxTicks;
                     spmGbl.priorityIdleTicks = spmGbl.priorityChPostTxTicks;
                  }

                  /* Previous Packet has ended so reset altPathBit flags.
                     Once tpr goes true then we will know to write
                     altPathBit once before tx */
                  spmGbl.writeAltPathBit = FALSE;
                  spmGbl.altPathBitWritten = FALSE;

                  /* start waiting out beta1 time */
                  spmGbl.phase = BETA1_IDLE;
               }
               else /* channel still busy */
               {
                  if (spmGbl.rf.rxFlag == 1)
                  {
                     /* busy receiving packet */
                     spmGbl.kind = POST_RX;
                  }
                  else
                  {
                     /* busy transmitting packet */
                     spmGbl.kind = POST_TX;
                  }


                  spmGbl.phase = BUSY;
               }
               break;
            case BETA1_IDLE:  /* waits out beta1 time */

               if ((spmGbl.rf.rxFlag == 1) ||
                   (spmGbl.rf.txOn == 1))
               {
                  /* channel is now busy */
                  /* other node must have used channel or
                     xcvr is in error with bad txOn*/

                  spmGbl.phase = BUSY;
               }
               else if ((macGbl.tpr == TRUE) &&
                        (spmGbl.writeAltPathBit == FALSE) &&
                        (spmGbl.state == IDLE)) /* in case doing status register read */
               {
                  /* tx packet is ready */
                  /* always do unconfirmed write
                     of altPathBit before tx */
                  spmGbl.writeAltPathBit = TRUE;

                  /* wait until next frame before begin channel
                     access */
                  spmGbl.phase = BETA1_IDLE;
               }
               else
               {
                  /* check idle timer to see if beta1 time expired */
                  spmGbl.elapsed =
                     *(spmGbl.clock) - spmGbl.idleTimerStart;

                  if ( spmGbl.elapsed >= spmGbl.beta1Ticks )
                  {
                     /* idle timer expired */
                     /* check if transmit packet is still ready and
                        set priority access type */
                     /* conditions for priority access
                        Packet marked as priority
                        Collisions less than 2
                        Node has a priority slot
                        Only after  received packet
                        only after valid crc on last received pkt
                     */
                     if ( (macGbl.tpr == TRUE) &&
                          (macGbl.priorityPkt == TRUE) &&
                          (spmGbl.kind == POST_RX) && /* only after  rx */
                          (spmGbl.collisionsThisPkt <= 1) &&
                          (spmGbl.nodePriority > 0) &&
                          (spmGbl.state == IDLE)) /* in case doing status register read */
                     {

                        /* attempt priority slot access */
                        spmGbl.transmitTimerStart = spmGbl.idleTimerStart + spmGbl.beta1Ticks;


                        spmGbl.phase = PRIORITY_WAIT_TX;
                     }
                     else
                     {

                        /* do not attempt priority slot access */
                        /* start waiting out priority slots */
                        spmGbl.phase = PRIORITY_IDLE;

                     }
                  }
                  else
                  {
                     spmGbl.phase = BETA1_IDLE; /* keep waiting */
                  }
               }
               break;
            case PRIORITY_IDLE:
               if ((spmGbl.rf.rxFlag == 1) ||
                   (spmGbl.rf.txOn == 1)) /* busy */
               {
                  /* other node must have used channel or
                     xcvr is in error with bad txOn*/

                  spmGbl.phase = BUSY;
               }
               else if ((macGbl.tpr == TRUE) &&
                        (spmGbl.writeAltPathBit == FALSE) &&
                        (spmGbl.state == IDLE)) /* in case doing status register read */
               {
                  /* tx packet is ready */
                  /* always do unconfirmed write of altPathBit
                     before tx */
                  spmGbl.writeAltPathBit = TRUE;

                  /* have to wait until next frame before
                     access channel */
                  spmGbl.phase = PRIORITY_IDLE;
               }
               else
               {
                  /* check idle timer for priority slots to end */
                  spmGbl.elapsed =
                     *(spmGbl.clock) - spmGbl.idleTimerStart;
                  if ( spmGbl.elapsed >=
                       (spmGbl.priorityIdleTicks + spmGbl.beta1Ticks))  /* idle timer expired */
                  {
                     /* timer expired so check if transmit packet ready */
                     if ( (macGbl.tpr == TRUE) &&
                        (spmGbl.state == IDLE)) /* in case doing status register read */
                     {
                        /* start transmit timer */
                        spmGbl.transmitTimerStart = spmGbl.idleTimerStart +
                                                      spmGbl.beta1Ticks +
                                                      spmGbl.priorityIdleTicks;
                        /* start base timer */
                        /* base timer runs when waiting for randomized tx access */

                     /* spmGbl.baseTimerStart = spmGbl.idleTimerStart +
                                                      spmGbl.beta1Ticks +
                                                      spmGbl.priorityIdleTicks; */

                        spmGbl.baseTimerStart = spmGbl.transmitTimerStart; /* faster way */


                        srand((unsigned int) spmGbl.transmitTimerStart);  /* change seed */

                        /* attempt random slot access */
                        /* random slot between 0 and backlog * wbase - 1*/

                        spmGbl.randomTicks = (rand() % ((spmGbl.backlog + 1) * W_BASE))
                           * spmGbl.beta2Ticks ;

                        spmGbl.phase = RANDOM_WAIT_TX; /* go wait for slot */
                     }
                     else  /* priority idle timer has expired but transmit packet is not ready */
                     {
                        spmGbl.phase = RANDOM_IDLE; /* go wait */
                     }
                  }
                  else /* timer not elapsed so keep waiting for end of priority slots */
                  {
                     spmGbl.phase = PRIORITY_IDLE;
                  }
               }
               break;
            case RANDOM_IDLE:
               if ((spmGbl.rf.rxFlag == 1) || (spmGbl.rf.txOn == 1)) /* busy */
               {
                  /* other node must have used channel or
                     xcvr is in error with bad txOn*/
                  spmGbl.phase = BUSY;
               }
               else if ((macGbl.tpr == TRUE) && /* tx packet ready */
                        (spmGbl.writeAltPathBit == FALSE) &&
                        (spmGbl.state == IDLE)) /* in case doing status register read */
               {
                  /* always do unconfirmed write of altPathBit before tx */
                  spmGbl.writeAltPathBit = TRUE;

                  /* have to wait until next frame to start channel access */
                  spmGbl.phase = RANDOM_IDLE;
               }
               else
               {
                  /* check if transmit packet still ready  */
                  if ( (macGbl.tpr == TRUE)  &&
                     (spmGbl.state == IDLE) ) /* in case doing status register read */
                  {
                     spmGbl.transmitTimerStart = *(spmGbl.clock); /* start transmit timer */

                     /* base timer runs when waiting for randomized tx access */
                     spmGbl.baseTimerStart = spmGbl.transmitTimerStart; /* start base timer */

                     srand((unsigned int) spmGbl.transmitTimerStart);  /* change seed */

                     /* attempt random slot access */
                     /* random slot between 0 and backlog * wbase - 1*/
                     spmGbl.randomTicks = (rand() % ((spmGbl.backlog + 1) * W_BASE))
                        * spmGbl.beta2Ticks ;

                     spmGbl.phase = RANDOM_WAIT_TX; /* go wait for slot */
                  }
                  else
                  {
                     spmGbl.phase = RANDOM_IDLE; /* nothing to send keeping waiting */
                  }
               }
               break;
            case PRIORITY_WAIT_TX:
               if ((spmGbl.rf.rxFlag == 1) || (spmGbl.rf.txOn == 1)) /* busy */
               {
                   /* other node must have used channel or
                     xcvr is in error with bad txOn*/
                  spmGbl.phase = BUSY;
               }
               else
               {
                  /* check tx timer wait for our priority slot to come up*/
                  spmGbl.elapsed = *(spmGbl.clock) - spmGbl.transmitTimerStart;
                  if (spmGbl.elapsed >= spmGbl.priorityNodeTicks)
                  {
                     /* approve transmission */
                     spmGbl.accessApproved = TRUE; /* enable spm state machine to begin tx */
                     spmGbl.phase = START_TX;
                  }
                  else
                  {
                     spmGbl.phase = PRIORITY_WAIT_TX;
                  }
               }
               break;
            case RANDOM_WAIT_TX:
               if ((spmGbl.rf.rxFlag == 1) || (spmGbl.rf.txOn == 1)) /* busy */
               {
                  /* other node must have used channel or
                     xcvr is in error with bad txOn*/
                  spmGbl.phase = BUSY;
               }
               else
               {

                  /* Get current time. Since used for two checks we want to be synchronized*/
                  spmGbl.stopped = *(spmGbl.clock);

                  /* check tx timer wait for out random slot to come up */
                  spmGbl.elapsed = spmGbl.stopped - spmGbl.transmitTimerStart;
                  if (spmGbl.elapsed >= spmGbl.randomTicks)
                  {
                     /* approve transmission */
                     spmGbl.accessApproved = TRUE; /* enable spm state machine to begin tx */
                     spmGbl.phase = START_TX;
                  }
                  else
                  {
                     spmGbl.phase = RANDOM_WAIT_TX;
                  }


                  /* check base timer, base timer running when we do random slot access  */
                  /* remember baseTimerStart is the same as transmitTimerStart only on
                     the first iteration */
                  spmGbl.elapsed = spmGbl.stopped - spmGbl.baseTimerStart;
                  if (spmGbl.elapsed >= spmGbl.baseTicks)
                  {
                     spmGbl.baseTimerStart = spmGbl.stopped; /* restart base timer */
                     DecrementBacklog(1);  /* decrement back log */
                  }

               }
               break;
            case START_TX:
               if ((spmGbl.rf.rxFlag == 1) || (spmGbl.rf.txOn == 1)) /* channel busy  */
               {
                  /* other node must have used channel or packet has started */

                  spmGbl.accessApproved = FALSE;
                  spmGbl.phase = BUSY;
               }
               else
               {
                  if ( macGbl.tpr == FALSE) /* macGbl.tpr == FALSE packet cancelled */
                  {
                      spmGbl.accessApproved = FALSE; /* deny access */
                      spmGbl.phase = RANDOM_IDLE;    /* back to idle */
                  }
                  else
                  {

                     spmGbl.phase = START_TX;
                  }

                  /* check base timer on more time in case expires while waiting
                     for txOn. only if non priority slot access */

                  spmGbl.stopped = *(spmGbl.clock);
                  spmGbl.elapsed = spmGbl.stopped - spmGbl.baseTimerStart;
                  if (spmGbl.elapsed >= spmGbl.baseTicks)
                  {
                     spmGbl.baseTimerStart = spmGbl.stopped; /* restart base timer */
                     DecrementBacklog(1);  /* decrement back log */
                  }

               }
               break;
            default:
               spmGbl.phase = RANDOM_IDLE;
               break;
         }  /* End access algo switch */


         /* Cycle Timer only runs when the Mac Layer is Idle that is when
            the node is not tranmitting or receiving or waiting to transmit
            or counting down beta1 or counting down priority slots or counting
            down the extra 16 beta2 slots after a transmission
            This directly corresponds to the Phase RANDOM_IDLE

            Anytime the MAC layer is busy ie in some phase other than RANDOM_IDLE
            then the cycle timer stops. The cycle timer will then resume when the
            mac layer returns to the RANDOM_IDLE phase. The cycle timer is reset when
            spmGbl.cycleTimerRestart == TRUE. This is set to true
            1) following a successful transmission
            2) following a valid crc receive

            We use a time difference between the current value of the hardware timer
            and the stored value when we started the cycleTimer to determine
            when the cycleTimer expires.
            Since the hardware timer is free running and does not stop when
            the cycle timer stops , we must keep shifting the
            cycleTimerStart forward whenever the cycleTimer is stopped. This keeps
            the relative time difference correct so that when it resumes
            the expiration calculation will be valid.
         */

         spmGbl.stopped = *(spmGbl.clock);
         if (  (spmGbl.phase == RANDOM_IDLE) ) /* cycle timer is running */
         {
            spmGbl.elapsed = spmGbl.stopped - spmGbl.cycleTimerStart;

            if (spmGbl.cycleTimerRestart) /* cycle timer needs to be restarted */
            {
               spmGbl.cycleTimerStart = spmGbl.stopped;
               spmGbl.cycleTimerRestart = FALSE;
            }
            else if (spmGbl.elapsed >= spmGbl.cycleTicks)
            {
               spmGbl.cycleTimerStart = spmGbl.stopped; /* restart */
               DecrementBacklog(1);
            }
         }
         else  /* shift cycleTimerStart forward since cycle timer stopped */
         {
            spmGbl.cycleTimerStart += spmGbl.stopped - spmGbl.lastTime;
         }
         spmGbl.lastTime = spmGbl.stopped; /* save last stopped value */

         /**************************************************************
           End Channel Access Algorithm State machine
           ***********************************************/

         /******************************************************
           Begin SPM Transfer State Machine
           ***********************************************/
         /* state machine for handshaking with xcvr*/
         switch ( spmGbl.state)
         {

            case IDLE:
               if ( spmGbl.rf.rxFlag == 1)          /* XCVR has detected packet to receive */
               {
                  macGbl.rc = 0; /* init receive byte count */
                  macGbl.rl = 0;
                  if ( macGbl.rpr == TRUE )  /* last packet not copied out */
                  {
                     spmGbl.mode =  OVERWRITE; /* stepped on last packet */
                     macGbl.rpr = FALSE;/* what else to do ? */
                  }

                  /* initialize  crc stuff */
                  crcGbl.poly = 0x1021;
                  crcGbl.crc = 0xffff;
                  crcGbl.crcBit = 0;
                  crcGbl.dataBit = 0;
                  crcGbl.dataByte = 0;


                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */
                  spmGbl.state = RECEIVE;
               }
               else if ( spmGbl.rf.txOn == 1 ) /*  XCVR xmitting on network so don't do anything */
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

                  /* increment reset count */
                  spmGbl.resetCount ++;
                  if (spmGbl.resetCount >= RESET_COUNT_LIMIT)
                  {
                     PHYHardResetSPMXCVR();
                  }

                  spmGbl.state = IDLE;
               }
               else if (( spmGbl.writeAltPathBit) && (!spmGbl.altPathBitWritten))
               {
                  /* need to write alt path bit unacked write */
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 1;
                    spmGbl.tf.data = spmGbl.configData[0];
                    */

                  *((uint8 *)&(spmGbl.tf)) = (uint8) 1;  /* fast way */
                  if (macGbl.altPathBit == 1)
                  {
                     *((uint8 *)&(spmGbl.tf.data)) =
                        (uint8) ( 0x80 | spmGbl.configData[0]);  /* fast way */
                  }
                  else
                  {
                     *((uint8 *)&(spmGbl.tf.data)) = (uint8) ( 0x7F & spmGbl.configData[0]);
                  }
                  spmGbl.altPathBitWritten = TRUE;
                  spmGbl.state = IDLE;
               }
               else if ((macGbl.tpr == TRUE) && /* transmit packet ready */
                        (spmGbl.accessApproved == TRUE)) /* channel access approved */
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 1;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x4000U;  /* fast way */

                  macGbl.tc = 0;
                  spmGbl.state = REQ_TX;
               }

               else if ( (spmGbl.crw == TRUE) &&  /* config register write */
                         (spmGbl.cra > 0) &&            /* 0< config reg address <=7 */
                         (spmGbl.cra <= 7) )
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = spmGbl.cra;
                    spmGbl.tf.data = spmGbl.crData;
                    */

                  *((uint8 *)&(spmGbl.tf)) = (uint8) spmGbl.cra;  /* fast way */
                  *((uint8 *)&(spmGbl.tf.data)) = (uint8) spmGbl.crData;  /* fast way */

                  spmGbl.state = WRITE;
               }
               else if ( (spmGbl.srr == TRUE) &&   /* status register read */
                         (spmGbl.sra >= 0) &&       /* 0< status reg address <=7 */
                         (spmGbl.sra <= 7) )
               {
                  spmGbl.tf.txFlag = 0;
                  spmGbl.tf.txReqFlag = 0;
                  spmGbl.tf.txDataValid = 0;
                  spmGbl.tf.blank = 0;
                  spmGbl.tf.txAddrRW = 1;
                  spmGbl.tf.txAddr = spmGbl.sra;
                  spmGbl.tf.data = 0;

                  spmGbl.state = READ;
               }
               else   /* Nothing can be done just wait */
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */
                  spmGbl.state = IDLE;
               }
               break;
            case RECEIVE:
               /*
                 spmGbl.tf.txFlag = 0;
                 spmGbl.tf.txReqFlag = 0;
                 spmGbl.tf.txDataValid = 0;
                 spmGbl.tf.blank = 0;
                 spmGbl.tf.txAddrRW = 0;
                 spmGbl.tf.txAddr = 0;
                 spmGbl.tf.data = 0;
                 */

               *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

               if (spmGbl.rf.rxFlag == 0)  /* packed ended */
               {
                  macGbl.rl = macGbl.rc; /* set last byte count */
                  /* is packet of valid length */
                  if ( (macGbl.rl >= 8) && /* minimum valid packet is 8 bytes */
                       (macGbl.rl < PKT_BUF_LEN) ) /* must be less than buffer */
                  {
                     /* do last step of crc */
                     crcGbl.crc = crcGbl.crc ^ 0xffff;


                     /* is crc valid */
                     if ( crcGbl.crc == (*((uint16 *) &macGbl.rPkt[macGbl.rl - 2])) )
                     {
                        /* copy out backlog */
                        macGbl.deltaBLRx = macGbl.rPkt[0] & 0x3F;

                        /* update cycle timer and set flag */
                        if (macGbl.deltaBLRx <= 0)
                        {
                           DecrementBacklog(1);
                        }
                        else
                        {
                           IncrementBacklog(macGbl.deltaBLRx);
                        }
                        /* Cycle Timer needs to be restarted after valid crc rx */
                        spmGbl.cycleTimerRestart = TRUE;

                        /* Attempt to copy packet into Link Layer receive Queue */
                        if ( (*gp->lkInQTailPtr == 0) && /* == 0 means free */
                              (gp->lkInBufSize >= macGbl.rl + 3) )  /* buffer big enough for packet */
                        {
                          /* Copy the packet to link layer input queue */
                           *(uint16 *)(gp->lkInQTailPtr + 1) = (uint16) macGbl.rl; /* copy length */
                           memcpy(gp->lkInQTailPtr + 3, macGbl.rPkt, macGbl.rl);
                           /* Update Link Layer Input Queue Fields */
                           *gp->lkInQTailPtr = 1; /* lets link layer know packet is ready to process */
                           gp->lkInQTailPtr += gp->lkInBufSize;
                           if (gp->lkInQTailPtr ==
                               gp->lkInQ + gp->lkInBufSize * gp->lkInQCnt) /* past end of queue */
                           {
                              gp->lkInQTailPtr = gp->lkInQ; /* wrap to beginning */
                           }
                        }
                        else /* packet discarded */
                        {
                           /* Discard packet as either there is no buffer available or
                               it is too big for buffer */
                           INCR_STATS(nmp->stats.missedMessages); /* update stats */
                        }
                     }
                     else /* invalid crc */
                     {
                        /* update stats for bad crc packet */
                        INCR_STATS(nmp->stats.transmissionErrors);
                     }
                  }
                  else /* update stats invalid packet length */
                  {
                     INCR_STATS(nmp->stats.transmissionErrors);
                  }

                  /* Reset macGbl's receive packet fields */
                  macGbl.rc = 0; /* reinit to be safe */
                  macGbl.rl = 0;
                  macGbl.rpr = FALSE;  /* ready to receive another packet */

                  spmGbl.state = IDLE; /*  all done */
               }
               else if (spmGbl.rf.rxDataValid  == 1)   /* valid byte this frame */
               {
                  if(macGbl.rc < PKT_BUF_LEN) /* don't go past end of buffer */
                  {
                     macGbl.rPkt[macGbl.rc] = spmGbl.rf.data;  /* copy out byte */
                     macGbl.rc++; /* increment counter */

                     /* do 1 byte of crc. Delay by two bytes so we do not computer
                        crc on the two crc bytes in the packet itself */
                     if (macGbl.rc >= 3)
                     {
                        /* make copy of byte */
                        crcGbl.dataByte = macGbl.rPkt[macGbl.rc - 3];
                        for (j = 0; j < 8; j++)
                        {
                           crcGbl.crcBit = (crcGbl.crc & 0x8000) ? 1 : 0;
                           crcGbl.dataBit = (crcGbl.dataByte & 0x80) ? 1 : 0;
                           crcGbl.dataByte = crcGbl.dataByte << 1;
                           crcGbl.crc = crcGbl.crc << 1;
                           if ( crcGbl.crcBit != crcGbl.dataBit)
                           {
                              crcGbl.crc = crcGbl.crc ^ crcGbl.poly;
                           }
                        }
                     }
                  }



                  spmGbl.state = RECEIVE; /* look for more */
               }
               else  /* no valid data but still receiving */
               {
                  spmGbl.state = RECEIVE; /* keep looking */
               }
               break;
            case WRITE:
               if ( spmGbl.rf.rxFlag == 1)          /* XCVR has detected packet to receive */
               {
                  macGbl.rc = 0; /* init receive byte count */
                  macGbl.rl = 0;
                  if ( macGbl.rpr == TRUE )  /* last packet not copyied out */
                  {
                     spmGbl.mode =  OVERWRITE; /* stepped on last packet */
                     macGbl.rpr = FALSE;/* what else to do ? */
                  }

                  /* initialize  crc stuff */
                  crcGbl.poly = 0x1021;
                  crcGbl.crc = 0xffff;
                  crcGbl.crcBit = 0;
                  crcGbl.dataBit = 0;
                  crcGbl.dataByte = 0;

                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

                  spmGbl.state = RECEIVE;
               }
               else if ((spmGbl.rf.rwAck == 0) &&  /* still not written */
                        (spmGbl.crw == TRUE) &&
                        (spmGbl.cra > 0) &&            /* 0< config reg address <=7 */
                        (spmGbl.cra <= 7) )
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = spmGbl.cra;
                    spmGbl.tf.data = spmGbl.crData;
                    */

                  *((uint8 *)&(spmGbl.tf)) = (uint8) spmGbl.cra;  /* fast way */
                  *((uint8 *)&(spmGbl.tf.data)) = (uint8) spmGbl.crData;  /* fast way */

                  spmGbl.state = WRITE; /* try again */
               }
               else  /* write successful or recalled */
               {
                  spmGbl.crw = FALSE;

                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */


                  spmGbl.state = IDLE;
               }
               break;
            case READ:
               if ( spmGbl.rf.rxFlag == 1)          /* XCVR has detected packet to receive */
               {
                  macGbl.rc = 0; /* init receive byte count */
                  macGbl.rl = 0;
                  if ( macGbl.rpr == TRUE )  /* last packet not copied out */
                  {
                     spmGbl.mode =  OVERWRITE; /* stepped on last packet */
                     macGbl.rpr = FALSE;/* what else to do ? */
                  }

                  /* initialize  crc stuff */
                  crcGbl.poly = 0x1021;
                  crcGbl.crc = 0xffff;
                  crcGbl.crcBit = 0;
                  crcGbl.dataBit = 0;
                  crcGbl.dataByte = 0;

                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

                  spmGbl.state = RECEIVE;
               }
               else if ((spmGbl.rf.rwAck == 0) &&  /* still not read */
                        (spmGbl.srr == TRUE) &&
                        (spmGbl.sra >= 0) &&           /* 0< config reg address <=7 */
                        (spmGbl.sra <= 7) )
               {

                  spmGbl.tf.txFlag = 0;
                  spmGbl.tf.txReqFlag = 0;
                  spmGbl.tf.txDataValid = 0;
                  spmGbl.tf.blank = 0;
                  spmGbl.tf.txAddrRW = 1;
                  spmGbl.tf.txAddr = spmGbl.sra;
                  spmGbl.tf.data = 0;

                  spmGbl.state = READ;  /* keep trying */
               }
               else if ( spmGbl.rf.rwAck == 1)
               {
                  spmGbl.srData = spmGbl.rf.data;
                  spmGbl.srr = FALSE;

                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */
                  spmGbl.state = IDLE;
               }
               else
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */
                  spmGbl.state = IDLE;
               }
               break;
            case REQ_TX:
               if ( spmGbl.rf.rxFlag == 1)          /* XCVR has detected packet to receive */
               {
                  macGbl.rc = 0; /* init receive byte count */
                  macGbl.rl = 0;
                  if ( macGbl.rpr == TRUE )  /* last packet not copied out */
                  {
                     spmGbl.mode =  OVERWRITE; /* stepped on last packet */
                     macGbl.rpr = FALSE;/* what else to do ? */
                  }

                  /* initialize  crc stuff */
                  crcGbl.poly = 0x1021;
                  crcGbl.crc = 0xffff;
                  crcGbl.crcBit = 0;
                  crcGbl.dataBit = 0;
                  crcGbl.dataByte = 0;

                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

                  spmGbl.state = RECEIVE;
               }
               else if (spmGbl.rf.clrTxReqFlag == 1) /* either accept or deny */
               {
                  if ( (spmGbl.rf.setTxFlag == 1) && /* request accepted */
                        (spmGbl.rf.txDataCTS == 1) )
                  {
                     /*
                    spmGbl.tf.txFlag = 1;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 1;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data =  macGbl.tPkt[macGbl.tc];
                    */

                     *((uint8 *)&(spmGbl.tf)) = (uint8) 0xA0;  /* fast way */
                     *((uint8 *)&(spmGbl.tf.data)) = (uint8) macGbl.tPkt[macGbl.tc];

                     macGbl.tc++; /* increment count */

                     spmGbl.state = TRANSMIT; /* accepted */
                  }
                  else  /* request denied */
                  {
                     /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                     *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

                     spmGbl.state = IDLE; /* dont want to tx anymore */

                  }
               }
               else  /* wait for xcvr to respond to request */
               {
                  /* typically there is one blank frame from xcvr
                     after REQ_TX bit is set but before CTS or ClrTxReqFlag

                     Possible bug in Echelon powerline spm xcvr.
                     Sending multiple REQ_TX without waiting for
                     response may cause xcvr to hang. So only
                     send zeros while waiting
                  */

                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

                  spmGbl.state = REQ_TX; /* Wait for response */
               }
               break;
            case TRANSMIT:
               if ( spmGbl.rf.setCollDet == 1)  /* collision so start over */
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

                  macGbl.tc = 0;

                  IncrementBacklog(1);  /* collision so increment backlog */

                  spmGbl.collisionsThisPkt++; /* increment collison count this packet */

                  INCR_STATS(nmp->stats.collisions);/* increment collision stats */

                  if (spmGbl.collisionsThisPkt >= 255)
                  {
                     /* throw away packet */
                     macGbl.tc = 0;
                     spmGbl.resetCount = 0; /* reset count for txOn */
                     spmGbl.collisionsThisPkt = 0;
                     macGbl.tpr = FALSE;
                  }

                  spmGbl.state = IDLE; /* start over */
               }
               else if (spmGbl.rf.txDataCTS == 1)
               {
                  /*
                    spmGbl.tf.txFlag = 1;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 1;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data =  macGbl.tPkt[macGbl.tc];
                    */

                  *((uint8 *)&(spmGbl.tf)) = (uint8) 0xA0;  /* fast way */
                  *((uint8 *)&(spmGbl.tf.data)) = (uint8) macGbl.tPkt[macGbl.tc];  /* fast way */


                  macGbl.tc++; /* increment count */

                  if( macGbl.tc >= macGbl.tl) /* last byte sent this frame */
                  {
                     spmGbl.state = DONE_TX; /* dont want to tx anymore */
                  }
                  else
                  {
                     spmGbl.state = TRANSMIT;
                  }
               }
               else if ((spmGbl.rf.txOn == 1) &&
                        (macGbl.tpr == TRUE) &&
                        (macGbl.tc < macGbl.tl) ) /* wait for cts */
               {
                  /*
                    spmGbl.tf.txFlag = 1;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data =  0;
                    */

                  *((uint8 *)&(spmGbl.tf)) = (uint8) 0x80;  /* fast way */
                  *((uint8 *)&(spmGbl.tf.data)) = (uint8) 0;  /* fast way */

                  spmGbl.resetCount++;
                  if (spmGbl.resetCount >= RESET_COUNT_LIMIT)
                  {
                     PHYHardResetSPMXCVR();
                     spmGbl.state = IDLE;
                  }
                  else
                  {
                     spmGbl.state = TRANSMIT;
                  }
               }
               else /* something wrong here throw away packet and get out */
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

                  macGbl.tc = 0;
                  spmGbl.resetCount = 0; /* reset count for txOn */
                  spmGbl.collisionsThisPkt = 0;
                  macGbl.tpr = FALSE;
                  spmGbl.state = IDLE;
               }
               break;
            case DONE_TX: /* wait for tx_on to go off */
               if ( spmGbl.rf.setCollDet == 1)  /* collision so start over */
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data = 0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

                  macGbl.tc = 0;

                  IncrementBacklog(1);  /* collision so increment backlog */

                  spmGbl.collisionsThisPkt++; /* increment collison count this packet */

                  INCR_STATS(nmp->stats.collisions);/* increment collision stats */

                  if (spmGbl.collisionsThisPkt >= 255)
                  {
                     /* throw away packet */
                     macGbl.tc = 0;
                     spmGbl.resetCount = 0; /* reset count for txOn */
                     spmGbl.collisionsThisPkt = 0;
                     macGbl.tpr = FALSE;
                  }

                  spmGbl.state = IDLE; /* start over */
               }
               else if (spmGbl.rf.txOn == 1) /* still sending last byte(s) */
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data =  0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0;  /* fast way */

                  spmGbl.resetCount++;
                  if (spmGbl.resetCount >= RESET_COUNT_LIMIT)
                  {
                     PHYHardResetSPMXCVR();
                     spmGbl.state = IDLE;
                  }
                  else
                  {
                     spmGbl.state = DONE_TX;
                  }
               }
               else /* (spmGbl.rf.txOn == 0) done sending */
               {
                  /*
                    spmGbl.tf.txFlag = 0;
                    spmGbl.tf.txReqFlag = 0;
                    spmGbl.tf.txDataValid = 0;
                    spmGbl.tf.blank = 0;
                    spmGbl.tf.txAddrRW = 0;
                    spmGbl.tf.txAddr = 0;
                    spmGbl.tf.data =  0;
                    */

                  *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0;  /* fast way */

                  /* successful transmission so update backlog */
                  if (macGbl.deltaBLTx <= 0)
                  {
                     DecrementBacklog(1);
                  }
                  else
                  {
                     IncrementBacklog(macGbl.deltaBLTx);
                  }

                  /* Cycle Timer needs to be restarted after valid tx  */

                  spmGbl.cycleTimerRestart = TRUE;


                  /* clean up */
                  macGbl.tc = 0;
                  spmGbl.resetCount = 0; /* reset count for txOn */
                  spmGbl.collisionsThisPkt = 0;
                  macGbl.tpr = FALSE;
                  spmGbl.state = IDLE; /* dont want to tx anymore */
               }
               break;
            case DEBUG:
            default:
               /*
                 spmGbl.tf.txFlag = 0;
                 spmGbl.tf.txReqFlag = 0;
                 spmGbl.tf.txDataValid = 0;
                 spmGbl.tf.blank = 0;
                 spmGbl.tf.txAddrRW = 0;
                 spmGbl.tf.txAddr = 0;
                 spmGbl.tf.data = 0;
                 */

               *((uint16 *)&(spmGbl.tf)) = (uint16) 0x0U;  /* fast way */

               spmGbl.state = DEBUG;
         }       /* end switch spm state machine */




#ifdef SPM_HISTORY
         hBufGbl.records[hBufGbl.index].tf = spmGbl.tf;
         /* time period of  isr */
         /* C unsigned math is congruent mod 2^n so negatives wrap around OK */
         hBufGbl.records[hBufGbl.index].duration = *(spmGbl.clock) -
            hBufGbl.records[hBufGbl.index].start;

         hBufGbl.index = (hBufGbl.index + 1) % NUM_HIST;
#endif
         /* Send Frame */
         tbdPtrGbl->r = 1;         /* buffer ready to transmit */
         spcomPtrGbl->str = 1;
      } /* end if spmGbl.mode == STOP */
   }  /* endif rxb event */

   cisrPtrGbl->spi = 1; /* clear spi bit in CISR by writing a 1 */
   return;
}

/*****************************************************************
Function:  CErrIsr
Returns:   none, no arguments allowed either
Reference:
Purpose:   interrupt service routine for CPM Int. Error
Comments:  must use interrupt pragma
******************************************************************/
/*  interrupt pragma informs compiler to make next function
        an interrupt handler with RTE and saved state of registers */

#pragma interrupt()

static void CErrIsr(void)
{

   return;
}

/*****************************************************************
Function:  SpurIntIsr
Returns:   none, no arguments allowed either
Reference:
Purpose:   interrupt service routine for spurious interrupt Error
Comments:  must use interrupt pragma
******************************************************************/
/*  interrupt pragma informs compiler to make next function
        an interrupt handler with RTE and saved state of registers */

#pragma interrupt()

static void SpurIntIsr(void)
{
   exceptions++;

   return;
}

/*****************************************************************
Function:  BusErrIsr
Returns:   none, no arguments allowed either
Reference:
Purpose:   interrupt service routine for Bus errors
Comments:  must use interrupt pragma
******************************************************************/
/*  interrupt pragma informs compiler to make next function
        an interrupt handler with RTE and saved state of registers */

#pragma interrupt()

static void BusErrIsr(void)
{
   exceptions++;

   return;
}

/*****************************************************************
Function:  GetTransceiverStatus
Returns:   none
Purpose:   To determine the status of transceiver registers.
           The status registers are different from config registers.
Comments:  The array should have space for all the register values.
******************************************************************/
void GetTransceiverStatus(Byte transceiverStatusOut[])
{
   int i;
   unsigned long count;
   Boolean good;

   /* read config registers on PLT-20 transceiver */

   /* read in reverse order */
   i = NUM_COMM_PARAMS - 1; /* should be 6 */
   while ( i >= 0)
   {
      /* read status registers starting at 7 down to 1 */
      spmGbl.sra = (Byte) i + 1;
      spmGbl.srr = TRUE;
      count = 0;
      /* wait for ack but time out so not infinite loop */
      while ((spmGbl.srr != FALSE) && (count < 0xFFFFF))
      {
         count++;
      }
      if (spmGbl.srr == TRUE) /* xcvr never acked try again */
      {
         pbdatPtrGbl->d5   = 0; /* first reset xcvr */

         /* wait for a while for everything to settle down */
         good = DelayTicks(RESTART_DELAY_TICKS);


         pbdatPtrGbl->d5   = 1; /* bring xcvr out of reset */
         i = NUM_COMM_PARAMS - 1; /* start over again */
      } /* NOTE!!! if the xcvr never acks this code will hang */
      else
      {
         transceiverStatusOut[i] = spmGbl.srData;
         i--;
      }
   }

   spmGbl.srData = 0;
   spmGbl.sra = 0;


   return;

}

/*************************End of spm.c*************************/

