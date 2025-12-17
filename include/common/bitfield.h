/*
 * bitfield.h
 *
 * Copyright (c) 2023-2025 EnOcean
 * SPDX-License-Identifier: MIT
 * See LICENSE file for details.
 * 
 * Title:   Bitfield Abstraction Macros
 * Purpose: Provides macros to define bitfields in structures that are
 *          portable across different compilers and platforms.
 * Notes:   These macros help define bitfields in a way that is portable.
 *          There are 8 BITS<n>() macros, one for each of 1..8 bitfields.
 *          BITS<n>() arguments are listed in little endian order (LSB first).
 *          If listed in big endian order, then define BITF_DECLARED_BIG_ENDIAN.
 *          Do not include a ';' after the macro, it will compile in C++ but not C.
 *          Unnamed bitfields are not possible.  Names must be unique within a
 *          structure.  Following is an example of BITS<n> usage:
 * 
 *              typedef struct {
 *                  BITS4      (field1,         2,      // Comment
 *                              field2,         2,      // Comment
 *                              field3,         2,      // Comment
 *                              field4,         2)      // Comment
 *              } Example;
 */

#ifndef _BITFIELD_H
#define _BITFIELD_H

#define UNNAMED
#define BITFIELD(name, length)  BitField name:length;

#ifdef	BITF_DECLARED_BIG_ENDIAN
#ifdef  BITF_LITTLE_ENDIAN
#undef  BITF_LITTLE_ENDIAN
#endif
#endif

#ifdef  BITF_LITTLE_ENDIAN
#define BFLE(name, length)  BITFIELD(name, length)
#define BFBE(name, length)
#else
#define BFLE(name, length)
#define BFBE(name, length)  BITFIELD(name, length)
#endif

/*
 *  Macro: BITS1
 *  Macro defines the ordering of bitfields in the case only one byte in the bitfield is used
 */
#define BITS1(f1,l1)\
        BFBE(f1,l1)                                                         BFLE(f1,l1)

/*
 *  Macro: BITS2
 *  Macro defines the ordering of bitfields in the case two bytes in the bitfield are used
 */
#define BITS2(f1,l1,f2,l2)\
        BFBE(f2,l2)     BITS1(f1,l1)                                        BFLE(f2,l2)

/*
 *  Macro: BITS3
 *  Macro defines the ordering of bitfields in the case three bytes in the bitfield are used
 */
#define BITS3(f1,l1,f2,l2,f3,l3)\
        BFBE(f3,l3)     BITS2(f1,l1,f2,l2)                                  BFLE(f3,l3)

/*
 *  Macro: BITS4
 *  Macro defines the ordering of bitfields in the case four bytes in the bitfield are used
 */
#define BITS4(f1,l1,f2,l2,f3,l3,f4,l4)\
        BFBE(f4,l4)     BITS3(f1,l1,f2,l2,f3,l3)                            BFLE(f4,l4)

/*
 *  Macro: BITS5
 *  Macro defines the ordering of bitfields in the case five bytes in the bitfield are used
 */
#define BITS5(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5)\
        BFBE(f5,l5)     BITS4(f1,l1,f2,l2,f3,l3,f4,l4)                      BFLE(f5,l5)

/*
 *  Macro: BITS6
 *  Macro defines the ordering of bitfields in the case six bytes in the bitfield are used
 */
#define BITS6(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6)\
        BFBE(f6,l6)     BITS5(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5)                BFLE(f6,l6)

/*
 *  Macro: BITS7
 *  Macro defines the ordering of bitfields in the case seven bytes in the bitfield are used
 */
#define BITS7(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6,f7,l7)\
        BFBE(f7,l7)     BITS6(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6)          BFLE(f7,l7)

/*
 *  Macro: BITS8
 *  Macro defines the ordering of bitfields in the case all eight bytes in the bitfield are used
 */
#define BITS8(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6,f7,l7,f8,l8)\
        BFBE(f8,l8)     BITS7(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6,f7,l7)    BFLE(f8,l8)

#endif // _BITFIELD_H
