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

/*********************************************************************
          File:        physical.h

       Version:        1.7

     Reference:        Protocol Spec. Section 4, 4.3

       Purpose:        Data Structures for Physical Layer

          Note:        None

         To Do:        None

*********************************************************************/
#ifndef _PHYSICAL_H
#define _PHYSICAL_H

/*------------------------------------------------------------------------------
Section: Includes
------------------------------------------------------------------------------*/
#include <eia709_1.h> /* needed for NUM_COMM_PARAMS */

/*-------------------------------------------------------------------
Section: Constant Definitions
-------------------------------------------------------------------*/
/* Length in bytes of Packet buffers for SPM ISR */
#define PKT_BUF_LEN 255

/*-------------------------------------------------------------------
Section: Type Definitions
-------------------------------------------------------------------*/
typedef enum
{
    RUN = 0,   /* The SPI is engaged in transfer */
    STOP,      /* The SPI is stopped usually due to error
                 must be re-initialized */
    OVERWRITE, /* receive new packet before old copied out */
} SPMMode;

typedef enum
{
    IDLE = 0,
    RECEIVE,
    WRITE,
    READ,
    REQ_TX,
    TRANSMIT,
    DONE_TX,
    DEBUG,
} SPMState;

typedef enum
{
    BUSY = 0,
    BETA1_IDLE,
    PRIORITY_IDLE,
    RANDOM_IDLE,
    PRIORITY_WAIT_TX,
    RANDOM_WAIT_TX,
    START_TX,
} AccessPhase;



typedef enum
{
    POST_RX,
    POST_TX,
} Beta1Kind;

/* user timer structure to keep elapsed time*/
typedef struct
{
    Boolean expired;  /* flag indicates if timer has expired
                        if use as countdown timer */
    uint32 start;     /* count when started timer */
    uint32 stop;      /* count when checked timer */
    uint32 elapsed;   /* elapsed time  ( start - stop) modulo 2^32 */
    uint32 limit;     /* value of elapsed when timer should expire */
    volatile uint32 * clock; /* pointer to mem mapped 32 bit io
                               counter register for clock */
} TimerData32;


/* Special Purpose Mode Receive Frame 16 bit */
typedef struct
{
    unsigned setTxFlag    : 1; /* XCVR accepts req to xmit packet */
    unsigned clrTxReqFlag : 1; /* XCVR acks req to xmit packet */
    unsigned rxDataValid  : 1; /* XCVR is passing data to 360
                                 in this frame */
    unsigned txDataCTS    : 1; /* 360 is clear to send byte of
                                 data to XCVR */
    unsigned setCollDet   : 1; /* XCVR has detected collision
                                 during preamble */
    unsigned rxFlag       : 1; /* XCVR had detected packet on
                                 network */
    unsigned rwAck        : 1; /* XCVR acknowledges read write
                                 to internal reg */
    unsigned txOn         : 1; /* XCVR is transmitting on network */
    uint8    data;             /* data byte */
} SPMRxFrame;

/* Special Purpose Mode Transmit Frame 16 bit */
typedef struct
{
    unsigned txFlag      : 1; /* 360 is transmitting packet */
    unsigned txReqFlag   : 1; /* 360 requests to transmit on network */
    unsigned txDataValid : 1; /* 360 is passing data to XCVR in this
                                frame */
    unsigned blank       : 1; /* unused*/
    unsigned txAddrRW    : 1; /* 360 reading/writing internal XCVR
                                reg, 1=read */
    unsigned txAddr      : 3; /* address  of internal XCVR reg */
    uint8    data;            /* data byte */
} SPMTxFrame;

/* Handshake parameters for MAC Layer, these are used
   to pass status and packet information between the ISR
   running the MAC sublayer and the physical, link, or network
   layers. This is the external interface for the Mac sublayer */

typedef struct
{
    uint8 altPathBit; /* alt path bit  for this packet */
    uint8 deltaBLTx;  /* delta backlog on current transmit packet */
    uint8 deltaBLRx;  /* delta backlog on last received packet */
    Boolean priorityPkt; /* is current packet from priority queue */
    Boolean tpr;     /* transmit packet ready to transmit
                        new packet is in tPkt */
    Boolean rpr;     /* receive packet ready ie new packet has
                       been put into rPkt */
    int16 tc;        /* transmit packet count, next byte in
                       packet to tx */
    int16 tl;        /* count of last byte in packet to to
                       tx = number of bytes -1 */
    int16 rc;        /* transmit packet count, next byte in packet
                       to tx */
    int16 rl;        /* count of last byte in packet to to
                       tx = number of bytes -1 */
    Byte tPkt[PKT_BUF_LEN]; /* buffer to hold transmit packet */
    Byte rPkt[PKT_BUF_LEN]; /* buffer to hold receive packet */

}   MACParam;

/* frame parameters for ISR */
typedef struct
{

    SPMMode mode;    /* status of SPM activity */
    SPMState state;  /* state */
    AccessPhase phase;  /* state of channel access algorithm */
    Beta1Kind kind;  /* type of beta1 slot either post rx or post tx */
    uint16 resetCount; /* counter to time if should reset xcvr if
                         tx_on not cleared */
    uint16 collisionsThisPkt; /* number of collisions this packet */
    uint8 configData[NUM_COMM_PARAMS];  /* config data for special
                                          purpose mode xcvr */

    Boolean writeAltPathBit;  /* flag to signal need to write alt
                                path bit config reg */
    Boolean altPathBitWritten; /* flag to indicate that the alt
                                 path bit was written */


    Boolean accessApproved; /* channel access algorithm complete
                              ok for transmit */

    Boolean cycleTimerRestart; /* flag enabling updates of cycle timer */

    uint8 backlog;    /* current channel backlog */

    uint8 nodePriority; /* number of node's priority slot */

    uint32 nicsToTicks;  /* conversion factor for this specification's time
                           base to 68360 ticks */
    uint32 bitClockRate; /* in units of Hz */
    uint32 beta2Ticks;   /* duration of beta2 in 68360 ticks
                           40ns each */
    uint32 beta1Ticks;   /* beta1 for this cycle either posttx or
                           postrx */
    uint32 beta1PostTxTicks; /* length of beta1 in 68360 ticks
                               40ns each */
    uint32 beta1PostRxTicks;   /* length of beta1 in 68360 ticks
                                 40ns each */
    uint32 baseTicks; /* duration of wbase in 68360 ticks 40ns each */
    uint32 cycleTicks;   /* duration of avg packet cycle in
                           68360 ticks 40ns each */
    uint32 priorityChPostTxTicks; /* duration of channel priority
                                   slots time  */
    uint32 priorityChPostRxTicks; /* duration of channel priority
                                   slots time  */
    uint32 priorityIdleTicks;  /* duration to wait before random access */

    uint32 priorityNodeTicks; /* duration until node's priority slot */
    uint32 randomTicks; /* duration of transmit timer random wait */
    uint32 idleTimerStart; /* timers for channel access algorithm
                             length of idle*/
    uint32 baseTimerStart; /* wbase timer for decrements */
    uint32 cycleTimerStart; /* avg packet cycle timer for decrements */
    uint32 transmitTimerStart; /* transmit slot timer */
    uint32 * clock;   /* address of MAC timer clock register */
    uint32 elapsed;   /* elasped time on a timer */
    uint32 stopped;   /* time timer stopped */
    uint32 lastTime;  /* last time timer stopped, used to update cycle timer */
    SPMRxFrame rf;   /* copy of recent RX frame */
    SPMTxFrame tf;   /* copy of next TX frame */
    Boolean crw;     /* write config register */
    Byte cra;        /* config register address should be
                       between 0 and 7 */
    Byte crData;     /* data byte for config register */
    Boolean srr;     /* read status register */
    Byte sra;        /* status register address should be between
                       0 and 7 */
    Byte srData;     /* data read from status register */


} SPMParam;

/*-------------------------------------------------------------------
Section: Globals
-------------------------------------------------------------------*/
/* parameters for SPMIsr  */
extern volatile MACParam macGbl;
extern volatile SPMParam spmGbl;

/*-------------------------------------------------------------------
Section: Function Prototypes
-------------------------------------------------------------------*/
void PHYReset(void);
void PHYSend(void);
void PHYReceive(void);
void PHYInitSPM(BOOLEAN firstReset);
void PHYSoftResetSPMXCVR(void);
void PHYHardResetSPMXCVR(void);
void PHYDisableSPMIsr(void);
void PHYEnableSPMIsr(void);
void PHYIO(void);
void PHYIOInit(void);
#endif

/*************************End of physical.h*************************/
