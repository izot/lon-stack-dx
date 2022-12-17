//
// bitfield.h
//
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

#ifndef _BITFIELD_H
#define _BITFIELD_H

// Bit Field macros for hiding endian differences
// NOTES:
// 1. There are 8 BITS<n>() macros, one for each of 1..8 bitfields.
// 2. BITS<n>() arguments are listed in little endian order (LSB first).  If listed in big endian order, then define BITF_DECLARED_BIG_ENDIAN
// 3. If you include a ';' after the macro, it will compile in C++ but not C so it's best not to.
// 4. Unnamed bitfields are not possible.  So use names like "rsvd".  They must be unique with a structure.
// 5. The recommended format of invoking BITS<n> is:
/*
    typedef struct
    {
        BITS4      (field1,         2,      // Comment
                    field2,         2,      // Comment
                    field3,         2,      // Comment
                    field4,         2)      // Comment
    } Example;
*/
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
#define BITS1(f1,l1)\
        BFBE(f1,l1)                                                                         BFLE(f1,l1)
#define BITS2(f1,l1,f2,l2)\
        BFBE(f2,l2)     BITS1(f1,l1)                                                    BFLE(f2,l2)
#define BITS3(f1,l1,f2,l2,f3,l3)\
        BFBE(f3,l3)     BITS2(f1,l1,f2,l2)                                          BFLE(f3,l3)
#define BITS4(f1,l1,f2,l2,f3,l3,f4,l4)\
        BFBE(f4,l4)     BITS3(f1,l1,f2,l2,f3,l3)                                    BFLE(f4,l4)
#define BITS5(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5)\
        BFBE(f5,l5)     BITS4(f1,l1,f2,l2,f3,l3,f4,l4)                          BFLE(f5,l5)
#define BITS6(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6)\
        BFBE(f6,l6)     BITS5(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5)                    BFLE(f6,l6)
#define BITS7(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6,f7,l7)\
        BFBE(f7,l7)     BITS6(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6)          BFLE(f7,l7)
#define BITS8(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6,f7,l7,f8,l8)\
        BFBE(f8,l8)     BITS7(f1,l1,f2,l2,f3,l3,f4,l4,f5,l5,f6,l6,f7,l7)    BFLE(f8,l8)

#endif // _BITFIELD_H
