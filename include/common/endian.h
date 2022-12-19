//
// endian.h
//
// Copyright (C) 2022 EnOcean
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

/*
 * Title: Endianness header file
 *
 * Abstract:
 * This file contains the macros for defining which kind of a system it is.
 * And thus based on the definitions of the macros how memory can be arranged or
 * read is decided.
 */

#ifndef _ENDIAN_H
#define _ENDIAN_H

#ifdef LITTLE_ENDIAN
#ifdef BIG_ENDIAN
ERROR: Need to define just one of these!
#endif
#elif !defined(BIG_ENDIAN)
ERROR: Need to define either LITTLE_ENDIAN or BIG_ENDIAN
#endif

#ifdef LITTLE_ENDIAN
#define hton16(s) (((UInt16)(s) << 8) + ((UInt16)(s) >> 8))
#define ntoh16(s)  hton16(s)
#elif defined(BIG_ENDIAN)
#define hton16(s) (s)
#define ntoh16(s) (s)
#endif

#ifdef LITTLE_ENDIAN
#define hton32(s) (((UInt32)(s) << 24) + (((UInt32)(s) << 8)&0xFF0000) + (((UInt32)(s) >> 8)&0xFF00) + (((UInt32)(s) >> 24)&0xFF))
#define ntoh32(s) hton32(s)
#else
#define hton32(s) (s)
#define ntoh32(s) (s)
#endif

#endif // _ENDIAN_H
