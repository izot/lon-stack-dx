/*
 * lcs_physical.h
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   LON Physical Layer Data Structures
 * Purpose: Define data structures for the native LON Physical Layer (Layer 1).
 * Notes:   This file is not used for LON/IP or LON USB links.
 *          For LON/IP, the physical layer is abstracted by the IP layer.
 *          For LON USB, the physical layer is abstracted by the USB link layer.
 */

#ifndef _PHYSICAL_H
#define _PHYSICAL_H

#include "izot/IzotPlatform.h"

#if LINK_IS(MIP)

#include "lcs/lcs_eia709_1.h" /* needed for NUM_COMM_PARAMS */

/* Length in bytes of Packet buffers for SPM ISR */
#define PKT_BUF_LEN 255

typedef enum {
    RUN = 0,   /* The SPI is engaged in transfer */
    STOP,      /* The SPI is stopped usually due to error
                 must be re-initialized */
    OVERWRITE, /* receive new packet before old copied out */
} SPMMode;

typedef enum {
    IDLE = 0,
    RECEIVE,
    WRITE,
    READ,
    REQ_TX,
    TRANSMIT,
    DONE_TX,
    DEBUG,
} SPMState;

typedef enum {
    BUSY = 0,
    BETA1_IDLE,
    PRIORITY_IDLE,
    RANDOM_IDLE,
    PRIORITY_WAIT_TX,
    RANDOM_WAIT_TX,
    START_TX,
} AccessPhase;

typedef enum {
    POST_RX,
    POST_TX,
} Beta1Kind;

/* User timer structure to keep elapsed time*/
typedef struct {
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
typedef struct {
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
typedef struct {
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

typedef struct {
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
    uint8_t tPkt[PKT_BUF_LEN]; /* buffer to hold transmit packet */
    uint8_t rPkt[PKT_BUF_LEN]; /* buffer to hold receive packet */

}   MACParam;

/* Frame parameters for ISR */
typedef struct {
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
    uint8_t cra;     /* config register address should be
                       between 0 and 7 */
    uint8_t crData;  /* data byte for config register */
    Boolean srr;     /* read status register */
    uint8_t sra;     /* status register address should be between
                       0 and 7 */
    uint8_t srData;  /* data read from status register */
} SPMParam;

/*-------------------------------------------------------------------
  Section: Globals
  -------------------------------------------------------------------*/
/* Parameters for SPMIsr  */
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

#endif  // LINK_IS(MIP)
#endif  // _PHYSICAL_H
