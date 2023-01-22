//
// IzotPlatform.h
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


/****************************************************************************
 *  Filename: IzotPlatform.h
 *
 *  Description:  This file contains platform dependant flags
 *  and basic data types. Many data types used with the LON DX Stack API
 *  are derived from the basic data types of this file, unless standard C or
 *  C-99 types are used.
 *
 *  Below this header, which contains further discussion and background
 *  information about the issues addressed by this file, there are a
 *  set of C-language typedef expressions and preprocessor macro
 *  definitions. These platform-preferences are grouped together by a
 *  preprocessor macro that indicates the compiler in use, and possibly
 *  the target platform in use.
 *
 *  If this file does not include definitions that are appropriate for your
 *  host and development environment.  You can derive your
 *  set of platform properties by copying one of the definition sets
 *  provided, preferably using a set matching a similar compiler or platform,
 *  and modifying them as needed.
 *
 *  You must make sure the correct compiler identifier (such as "_COSMIC")
 *  is defined at compile time; failure to do so will result in an error
 *  during compilation. Many compilers and development kits supply pre-defined
 *  definitions for this purpose. Alternatively, you can define a constant
 *  preprocessor symbol of your choice within your project definition or
 *  makefile.
 *
 *  -------- Portability Principles -----
 *
 *  The LON DX Stack API uses portable type definitions where necessary,
 *  and uses native types, including C-99 types, where possible. Portable
 *  types are primarily used in the definition of protocol and network data
 *  structures, because these definitions must generally match the protocol
 *  definition in size, alignment, layout, and related aspects.
 *
 *  C-99 Types
 *
 *  The portable types are based on C99 types, e.g. uint8_t, int16_t, etc.
 *  Definitions for compilers which support <stdint.h> should #include this
 *  standard C99 .H file within the compiler-specific definitions, below in
 *  this file. Definitions for other compilers can include type definitions
 *  for uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t in the same
 *  location.
 *
 *  Bitfields
 *
 *  Bitfields are generally not portable, because the location of a bit field
 *  within its container, and the type (size) of that container, is not
 *  defined or modifyable. Therefore, bit fields are defined with their
 *  enclosing byte, and macros are provided to extract or manipulate the bit
 *  field information. See the <IZOT_GET_ATTRIBUTE> and <IZOT_SET_ATTRIBUTE>
 *  macros.
 *
 *  Enumerations
 *
 *  Enumerations are implemented using a signed 8-bit data type, and members
 *  are defined through a number of preprocessor symbols.
 *  This eliminates concern about the size of an enumeration type.
 *
 *  Structures, Unions
 *
 *  Structures and unions are defined and implemented using utility
 *  definitions provided in this file. These utility definitions can map
 *  to plain C language phrases, and can also engage compiler-specific,
 *  non-standard keywords. These are often used to create structures and
 *  unions which meet the protocol requirements: byte-alignment, no padding.
 *
 *  Multi-byte Scalars
 *
 *  Multiple-byte scalars are presented as aggregates of multiple bytes, in
 *  the network and protocol byte order (big endian). Macros such as
 *  <IZOT_GET_UNSIGNED_WORD> and <IZOT_SET_UNSIGNED_WORD> are provided to
 *  convert between native scalars and multi-byte aggregates.
 *
 ***************************************************************************/
#include "../abstraction/IzotConfig.h"

#ifndef _IZOT_PLATFORM_H
#define _IZOT_PLATFORM_H


#if defined(ARM_NONE_EABI_GCC)

    /*
     * ARM_NONE_EABI_GCC C programs.
     */
    #define ARM_EABI_GCC_HOSTED        // used with the CPM 4200 SDK

    /*
     * Indicate that a compiler/platform has been defined in this file.
     */
    #if !defined(_IZOT_PLATFORM_DEFINED)
        #define _IZOT_PLATFORM_DEFINED
    #else
        #error Multiple platform definitions
    #endif

    /*
     * If stdint.h is supported, simply #include this here,
     * replacing the following definitions.
     */
	 #include "stdint.h"
    
    /*
     * IZOT_(ENUM|STRUCT|UNION)_BEGIN and *_END macros are used to begin and
     * end type definitions for enumerations, structures and unions. These
     * must be defined such that the resulting type uses byte-alignment
     * without padding.
     *
     * The *_NESTED_* macros are used to define unions or structures within
     * surrounding union or structure definitions. These may require different
     * modifiers to accomplish the same compact, byte-aligned, image.
     *
     * IZOT_ENUM is used to implement a variable of an enumeration type. This
     * can expand to the actual enumeration type, if the compiler supports
     * a signed 8-bit enumerated type. Most compilers will map this to int8_t.
     */
    #define IZOT_ENUM_BEGIN(n)   enum
    #define IZOT_ENUM_END(n)     n
    #define IZOT_ENUM(n)         n

    #define IZOT_STRUCT_BEGIN(n) struct __attribute__((__packed__))
    #define IZOT_STRUCT_END(n)   n

    #define IZOT_STRUCT_NESTED_BEGIN(n)  struct __attribute__((__packed__))
    #define IZOT_STRUCT_NESTED_END(n)    n

    #define IZOT_UNION_BEGIN(n)  union __attribute__((__packed__))
    #define IZOT_UNION_END(n)    n

    #define IZOT_UNION_NESTED_BEGIN(n)   union __attribute__((__packed__))
    #define IZOT_UNION_NESTED_END(n)     n

	/*
     * Compiler-dependent types for signed and unsigned 8-bit, 16-bit scalars and 32-bit scalars.
     * All ShortStack-Builder generated types use NEURON C equivalent types which
     * are based on the following type definitions.
     *
	 * To enhance portability between different platforms, no aggregate shall contain
	 * multi-byte scalars, but shall use multiple byte-sized scalars instead. We will define
	 * only the basic type IzotByte and the rest (IzotWord, IzotDoubleWord) derive from it.
	 *
     * Note that "float" type variables are handled through a "float_type"
     * equivalent structure. Make sure to consult the ShortStack documentation
     * for more details about Builder-generated type definitions including
     * details about "float" type handling.
     */

    typedef unsigned char	IzotUbits8;			/* 8-bits */
    typedef signed   char	IzotBits8;			/* 8-bits, signed */
	typedef unsigned short  IzotUbits16;			/* 16-bits */
    typedef signed   short  IzotBits16;			/* 16-bits, signed */
	typedef unsigned long	IzotUbits32;         /* 32-bits */
    typedef signed   long	IzotBits32;			/* 32-bits, signed */
	
    typedef uint8_t        IzotByte;

    /*
     *  typedef: IzotWord
     *  Holds a 16-bit numerical value.
     *
     *  The IzotWord structure holds a 16-bit unsigned value in big-endian
     *  ordering through two seperate high-order and low-order bytes.
     *  Use the <IZOT_SET_SIGNED_WORD> or <IZOT_SET_UNSIGNED_WORD> macro to
     *  obtain the signed or unsigned numerical value in the correct byte
     *  ordering.
     */
    typedef IZOT_STRUCT_BEGIN(IzotWord)
    {
        IzotByte  msb;    /* high-order byte, the most significant byte, the 0x12 in 0x1234 */
        IzotByte  lsb;    /* low-order byte, the least significant byte, the 0x34 in 0x1234 */
    } IZOT_STRUCT_END(IzotWord);

    /*
     *  typedef: IzotDoubleWord
     *  Holds a 32-bit numerical value.
     *
     *  The IzotDoubleWord structure holds a 32-bit unsigned value in big-endian
     *  ordering through two seperate high-order and low-order <IzotWord> members.
     *  Use the <IZOT_SET_SIGNED_DOUBLEWORD> or <IZOT_SET_UNSIGNED_DOUBLEWORD>
     *  macro to obtain the signed or unsigned numerical value in the correct
     *  byte ordering.
     */
    typedef IZOT_STRUCT_BEGIN(IzotDoubleWord)
    {
        IzotWord  msw;    /* high-order word, the most significant word, the 0x1234 in 0x12345678 */
        IzotWord  lsw;    /* low-order word, the least significant word, the 0x5678 in 0x12345678 */
    } IZOT_STRUCT_END(IzotDoubleWord);

    /*
     * Basic boolean type. You must make sure to have a type
     * with name 'IzotBool' defined, and that type must accept TRUE and FALSE
     * (defined below).
     */
    typedef int        IzotBool;
    #ifndef TRUE
        #define TRUE   1
    #endif
    #ifndef FALSE
        #define FALSE  0
    #endif

#elif defined(_COSMIC)

    /*
     * Cosmic C programs.
     */
    #define COSMIC_HOSTED        /* runs in the COSMIC C environment */

    /*
     * Indicate that a compiler/platform has been defined in this file.
     */
    #if !defined(_IZOT_PLATFORM_DEFINED)
        #define _IZOT_PLATFORM_DEFINED
    #else
        #error Multiple platform definitions
    #endif

    /*
     * IZOT_(ENUM|STRUCT|UNION)_BEGIN and *_END macros are used to begin and
     * end type definitions for enumerations, structures and unions. These
     * must be defined such that the resulting type uses byte-alignment
     * without padding.
     *
     * The *_NESTED_* macros are used to define unions or structures within
     * surrounding union or structure definitions. These may require different
     * modifiers to accomplish the same compact, byte-aligned, image.
     *
     * IZOT_ENUM is used to implement a variable of an enumeration type. This
     * can expand to the actual enumeration type, if the compiler supports
     * a signed 8-bit enumerated type. Most compilers will map this to int8_t.
     */
    #define IZOT_ENUM_BEGIN(n)   enum
    #define IZOT_ENUM_END(n)     n
    #define IZOT_ENUM(n)         n

    #define IZOT_STRUCT_BEGIN(n) struct
    #define IZOT_STRUCT_END(n)   n

    #define IZOT_STRUCT_NESTED_BEGIN(n)  struct
    #define IZOT_STRUCT_NESTED_END(n)    n

    #define IZOT_UNION_BEGIN(n)  union
    #define IZOT_UNION_END(n)    n

    #define IZOT_UNION_NESTED_BEGIN(n)   union
    #define IZOT_UNION_NESTED_END(n)     n

    /*
     * If stdint.h is supported, simply #include this here,
     * replacing the following definitions.
     */
    typedef unsigned char  uint8_t;
    typedef signed char    int8_t;
    typedef unsigned short uint16_t;
    typedef signed short   int16_t;
    typedef unsigned long  uint32_t;
    typedef signed long    int32_t;

    typedef uint8_t        IzotByte;

    /*
     *  typedef: IzotWord
     *  Holds a 16-bit numerical value.
     *
     *  The IzotWord structure holds a 16-bit unsigned value in big-endian
     *  ordering through two seperate high-order and low-order bytes.
     *  Use the <IZOT_SET_SIGNED_WORD> or <IZOT_SET_UNSIGNED_WORD> macro to
     *  obtain the signed or unsigned numerical value in the correct byte
     *  ordering.
     */
    typedef IZOT_STRUCT_BEGIN(IzotWord)
    {
        IzotByte  msb;    /* high-order byte, the most significant byte, the 0x12 in 0x1234 */
        IzotByte  lsb;    /* low-order byte, the least significant byte, the 0x34 in 0x1234 */
    } IZOT_STRUCT_END(IzotWord);

    /*
     *  typedef: IzotDoubleWord
     *  Holds a 32-bit numerical value.
     *
     *  The IzotDoubleWord structure holds a 32-bit unsigned value in big-endian
     *  ordering through two seperate high-order and low-order <IzotWord> members.
     *  Use the <IZOT_SET_SIGNED_DOUBLEWORD> or <IZOT_SET_UNSIGNED_DOUBLEWORD>
     *  macro to obtain the signed or unsigned numerical value in the correct
     *  byte ordering.
     */
    typedef IZOT_STRUCT_BEGIN(IzotDoubleWord)
    {
        IzotWord  msw;    /* high-order word, the most significant word, the 0x1234 in 0x12345678 */
        IzotWord  lsw;    /* low-order word, the least significant word, the 0x5678 in 0x12345678 */
    } IZOT_STRUCT_END(IzotDoubleWord);

    /*
     * Basic boolean type. You must make sure to have a type
     * with name 'IzotBool' defined, and that type must accept TRUE and FALSE
     * (defined below).
     */
    typedef int        IzotBool;
    #ifndef TRUE
        #define TRUE   1
    #endif
    #ifndef FALSE
        #define FALSE  0
    #endif

#elif defined(_HITECH)
    /*
     * Hi-Tech C programs.
     */
    #define HITECH_HOSTED       /* runs in the HITECH C environment */

    /*
     * Indicate that a compiler/platform has been defined
     */
    #if !defined(_IZOT_PLATFORM_DEFINED)
        #define _IZOT_PLATFORM_DEFINED
    #else
        #error Multiple platform definitions
    #endif

    #define IZOT_ENUM_BEGIN(n)   enum
    #define IZOT_ENUM_END(n)     n
    #define IZOT_ENUM(n)         n

    #define IZOT_STRUCT_BEGIN(n) struct
    #define IZOT_STRUCT_END(n)   n

    #define IZOT_STRUCT_NESTED_BEGIN(n)  struct
    #define IZOT_STRUCT_NESTED_END(n)    n

    #define IZOT_UNION_BEGIN(n)  union
    #define IZOT_UNION_END(n)    n

    #define IZOT_UNION_NESTED_BEGIN(n)   union
    #define IZOT_UNION_NESTED_END(n)     n

    /*
     * If stdint.h is supported, simply #include this here,
     * replacing the following definitions.
     */
    typedef unsigned char  uint8_t;
    typedef signed char    int8_t;
    typedef unsigned short uint16_t;
    typedef signed short   int16_t;
    typedef unsigned long  uint32_t;
    typedef signed long    int32_t;

    typedef uint8_t        IzotByte;

    typedef IZOT_STRUCT_BEGIN(IzotWord)
    {
        IzotByte  msb;    /* high-order byte, the most significant byte, the 0x12 in 0x1234 */
        IzotByte  lsb;    /* low-order byte, the least significant byte, the 0x34 in 0x1234 */
    } IZOT_STRUCT_END(IzotWord);

    typedef IZOT_STRUCT_BEGIN(IzotDoubleWord)
    {
        IzotWord  msw;    /* high-order word, the most significant word, the 0x1234 in 0x12345678 */
        IzotWord  lsw;    /* low-order word, the least significant word, the 0x5678 in 0x12345678 */
    } IZOT_STRUCT_END(IzotDoubleWord);

    typedef int     IzotBool;
    #ifndef TRUE
        #define TRUE   1
    #endif
    #ifndef FALSE
        #define FALSE  0
    #endif

#elif defined(WIN32)
    /*
     * WIN32 C programs.
     */
    #define WIN32_HOSTED       /* runs in the WIN32 environment */

    /*
     * indicate a compiler/platform has been defined
     */
    #if !defined(_IZOT_PLATFORM_DEFINED)
        #define _IZOT_PLATFORM_DEFINED
    #else
        #error Multiple platform definitions
    #endif

    #include <stdint.h>

    typedef uint8_t             IzotByte;

    #define IZOT_ENUM_BEGIN(n)   enum
    #define IZOT_ENUM_END(n)     n
    #define IZOT_ENUM(n)         IzotByte

    #define IZOT_STRUCT_BEGIN(n) struct __declspec(align(1))
    #define IZOT_STRUCT_END(n)   n

    #define IZOT_STRUCT_NESTED_BEGIN(n)  struct
    #define IZOT_STRUCT_NESTED_END(n)    n

    #define IZOT_UNION_BEGIN(n)  union __declspec(align(1))
    #define IZOT_UNION_END(n)    n

    #define IZOT_UNION_NESTED_BEGIN(n)   union
    #define IZOT_UNION_NESTED_END(n)     n

    typedef IZOT_STRUCT_BEGIN(IzotWord)
    {
        IzotByte  msb;    /* high-order byte, the most significant byte, the 0x12 in 0x1234 */
        IzotByte  lsb;    /* low-order byte, the least significant byte, the 0x34 in 0x1234 */
    } IZOT_STRUCT_END(IzotWord);

    typedef IZOT_STRUCT_BEGIN(IzotDoubleWord)
    {
        IzotWord  msw;    /* high-order word, the most significant word, the 0x1234 in 0x12345678 */
        IzotWord  lsw;    /* low-order word, the least significant word, the 0x5678 in 0x12345678 */
    } IZOT_STRUCT_END(IzotDoubleWord);

   typedef int     IzotBool;
    #ifndef TRUE
        #define TRUE   1
    #endif
    #ifndef FALSE
        #define FALSE  0
    #endif

#elif defined(ARM7_IAR)
    /*
     * ARM7 C programs using the IAR toolchain.
     *
     */
    #define ARM7_HOSTED       /* runs in the ARM7 environment */

    /*
     * indicate a compiler/platform has been defined
     */
    #if !defined(_IZOT_PLATFORM_DEFINED)
        #define _IZOT_PLATFORM_DEFINED
    #else
        #error Multiple platform definitions
    #endif

    /*
     * Definition for ARM7 specific pragmas, definitions, and so on.
     * For example, packing directives to align objects on byte boundary.
     */
    #define INCLUDE_IZOT_BEGIN_END

    #include <stdint.h>

    typedef uint8_t             IzotByte;

    #define IZOT_ENUM_BEGIN(n)   enum
    #define IZOT_ENUM_END(n)     n
    #define IZOT_ENUM(n)         n

    #define IZOT_STRUCT_BEGIN(n) struct
    #define IZOT_STRUCT_END(n)   n

    #define IZOT_STRUCT_NESTED_BEGIN(n)  struct
    #define IZOT_STRUCT_NESTED_END(n)    n

    #define IZOT_UNION_BEGIN(n)  union
    #define IZOT_UNION_END(n)    n

    #define IZOT_UNION_NESTED_BEGIN(n)   union
    #define IZOT_UNION_NESTED_END(n)     n

    typedef IZOT_STRUCT_BEGIN(IzotWord)
    {
        IzotByte  msb;    /* high-order byte, the most significant byte, the 0x12 in 0x1234 */
        IzotByte  lsb;    /* low-order byte, the least significant byte, the 0x34 in 0x1234 */
    } IZOT_STRUCT_END(IzotWord);

    typedef IZOT_STRUCT_BEGIN(IzotDoubleWord)
    {
        IzotWord  msw;    /* high-order word, the most significant word, the 0x1234 in 0x12345678 */
        IzotWord  lsw;    /* low-order word, the least significant word, the 0x5678 in 0x12345678 */
    } IZOT_STRUCT_END(IzotDoubleWord);

   typedef int     IzotBool;
    #ifndef TRUE
        #define TRUE   1
    #endif
    #ifndef FALSE
        #define FALSE  0
    #endif

#elif defined(AVR_TINY13)
    /*
     * AVR Tiny-13 & C programs.
     */
    #define AVR_TINY13_HOSTED       /* runs in the AVR_TINY13 environment */

    /*
     * indicate a compiler/platform has been defined
     */
    #if !defined(_IZOT_PLATFORM_DEFINED)
        #define _IZOT_PLATFORM_DEFINED
    #else
        #error Multiple platform definitions
    #endif

   /*
     * If stdint.h is supported, simply #include this here,
     * replacing the following definitions.
     */
    typedef unsigned char  uint8_t;
    typedef signed char    int8_t;
    typedef unsigned short uint16_t;
    typedef signed short   int16_t;
    typedef unsigned long  uint32_t;
    typedef signed long    int32_t;

    typedef uint8_t        IzotByte;

    #define IZOT_ENUM_BEGIN(n)   enum
    #define IZOT_ENUM_END(n)     n
    #define IZOT_ENUM(n)         n

    #define IZOT_STRUCT_BEGIN(n) struct
    #define IZOT_STRUCT_END(n)   n

    #define IZOT_STRUCT_NESTED_BEGIN(n)  struct
    #define IZOT_STRUCT_NESTED_END(n)    n

    #define IZOT_UNION_BEGIN(n)  union
    #define IZOT_UNION_END(n)    n

    #define IZOT_UNION_NESTED_BEGIN(n)   union
    #define IZOT_UNION_NESTED_END(n)     n

    typedef IZOT_STRUCT_BEGIN(IzotWord)
    {
        IzotByte  msb;    /* high-order byte, the most significant byte, the 0x12 in 0x1234 */
        IzotByte  lsb;    /* low-order byte, the least significant byte, the 0x34 in 0x1234 */
    } IZOT_STRUCT_END(IzotWord);

    typedef IZOT_STRUCT_BEGIN(IzotDoubleWord)
    {
        IzotWord  msw;    /* high-order word, the most significant word, the 0x1234 in 0x12345678 */
        IzotWord  lsw;    /* low-order word, the least significant word, the 0x5678 in 0x12345678 */
    } IZOT_STRUCT_END(IzotDoubleWord);

    typedef int     IzotBool;
    #ifndef TRUE
        #define TRUE   1
    #endif
    #ifndef FALSE
        #define FALSE  0
    #endif

#elif defined(GCC_NIOS)
    /*
     * GCC_NIOS C programs.  Assume GCC_NIOS is defined.
     *
     * This is the correct choice for the Nios II platform.
     * This section also assumes that C programs run on a LITTLE_ENDIAN machine,
     * which is the typical case.  If you are running a C program on a BIG_ENDIAN
     * processor, be sure to adjust the definitions within this section.
     */
    #define GCC_NIOS_HOSTED       /* runs in the GCC_NIOS C environment */

    /*
     * indicate a compiler/platform has been defined
     */
    #if !defined(_IZOT_PLATFORM_DEFINED)
        #define _IZOT_PLATFORM_DEFINED
    #else
        #error Multiple platform definitions
    #endif

    #include <stdint.h>

    typedef uint8_t     IzotByte;

    #define IZOT_ENUM_BEGIN(n)   enum
    #define IZOT_ENUM_END(n)     n
    #define IZOT_ENUM(n)         IzotByte

    #define IZOT_STRUCT_BEGIN(n) struct
    #define IZOT_STRUCT_END(n)   __attribute((__packed__)) n

    #define IZOT_STRUCT_NESTED_BEGIN(n)  struct
    #define IZOT_STRUCT_NESTED_END(n)    __attribute((__packed__)) n

    #define IZOT_UNION_BEGIN(n)  union
    #define IZOT_UNION_END(n)    __attribute((__packed__)) n

    #define IZOT_UNION_NESTED_BEGIN(n)   union
    #define IZOT_UNION_NESTED_END(n)     __attribute((__packed__)) n

    typedef IZOT_STRUCT_BEGIN(IzotWord)
    {
        IzotByte  msb;    /* high-order byte, the most significant byte, the 0x12 in 0x1234 */
        IzotByte  lsb;    /* low-order byte, the least significant byte, the 0x34 in 0x1234 */
    } IZOT_STRUCT_END(IzotWord);

    typedef IZOT_STRUCT_BEGIN(IzotDoubleWord)
    {
        IzotWord  msw;    /* high-order word, the most significant word, the 0x1234 in 0x12345678 */
        IzotWord  lsw;    /* low-order word, the least significant word, the 0x5678 in 0x12345678 */
    } IZOT_STRUCT_END(IzotDoubleWord);

    typedef int     IzotBool;
    #ifndef TRUE
        #define TRUE   1
    #endif
    #ifndef FALSE
        #define FALSE  0
    #endif
#elif defined(__ARMEL__)
    /*
     * This is the correct choice for the Raspberry Pi platform using the
     * arm-linux-gnueabihf toolchain.
     */
    #define RASPBERRY_PI_HOSTED       /* runs on the Raspberry Pi */

    /*
     * indicate a compiler/platform has been defined
     */
    #if !defined(_IZOT_PLATFORM_DEFINED)
        #define _IZOT_PLATFORM_DEFINED
    #else
        #error Multiple platform definitions
    #endif

    /*
     * Compiler-dependent types for signed and unsigned 8-bit, 16-bit scalars,
     * and 32-bit scalars. All Izot Interface Developer-Builder generated
     * types use NEURON C equivalent types which are based on the following
     * type definitions.
     *
     * To enhance portability between different platforms, no aggregate shall
     * contain multi-byte scalars, but shall use multiple byte-sized scalars
     * instead. We will define only the basic type IzotByte and the rest
     * (IzotWord, IzotDoubleWord) derive from it.
     *
     * Note that "float" type variables are handled through a "float_type"
     * equivalent structure. See the ShortStack or IZOT documentation
     * for more details about Builder-generated type definitions including
     * details about "float" type handling.
     */

    #include <stdint.h>
    typedef uint8_t       IzotByte;

    #define IZOT_ENUM_BEGIN(n)   enum
    #define IZOT_ENUM_END(n)     n
    #define IZOT_ENUM(n)         IzotByte

    #define IZOT_STRUCT_BEGIN(n) struct
    #define IZOT_STRUCT_END(n)   __attribute__((__packed__, aligned(1))) n

    #define IZOT_STRUCT_NESTED_BEGIN(n)  struct
    #define IZOT_STRUCT_NESTED_END(n)    __attribute__((__packed__, aligned(1))) n

    #define IZOT_UNION_BEGIN(n)  union
    #define IZOT_UNION_END(n)    __attribute__((__packed__, aligned(1))) n

    #define IZOT_UNION_NESTED_BEGIN(n)   union
    #define IZOT_UNION_NESTED_END(n)     __attribute__((__packed__, aligned(1))) n

    typedef IZOT_STRUCT_BEGIN(IzotWord)
    {
        IzotByte  msb;    /* high-order byte, the most significant byte, the 0x12 in 0x1234 */
        IzotByte  lsb;    /* low-order byte, the least significant byte, the 0x34 in 0x1234 */
    } IZOT_STRUCT_END(IzotWord);

    typedef IZOT_STRUCT_BEGIN(IzotDoubleWord)
    {
        IzotWord  msw;    /* high-order word, the most significant word, the 0x1234 in 0x12345678 */
        IzotWord  lsw;    /* low-order word, the least significant word, the 0x5678 in 0x12345678 */
    } IZOT_STRUCT_END(IzotDoubleWord);

    typedef int     IzotBool;
    #ifndef TRUE
        #define TRUE   1
    #endif
    #ifndef FALSE
        #define FALSE  0
    #endif

#elif defined(LINUX32_GCC)
    /*
     * This is the correct choice for a 32-bit standard Linux desktop
     * configuration using the standard GCC tool chain.
     */
    #define LINUX32_HOSTED       /* runs on a Linux32 system */

    /*
     * indicate a compiler/platform has been defined
     */
    #if !defined(_IZOT_PLATFORM_DEFINED)
        #define _IZOT_PLATFORM_DEFINED
    #else
        #error Multiple platform definitions
    #endif

   #include <stdint.h>

   typedef uint8_t       IzotByte;

    #define IZOT_ENUM_BEGIN(n)   enum
    #define IZOT_ENUM_END(n)     n
    #define IZOT_ENUM(n)         IzotByte

    #define IZOT_STRUCT_BEGIN(n) struct
    #define IZOT_STRUCT_END(n)   __attribute__((__packed__, aligned(1))) n

    #define IZOT_STRUCT_NESTED_BEGIN(n)  struct
    #define IZOT_STRUCT_NESTED_END(n)    __attribute__((__packed__, aligned(1))) n

    #define IZOT_UNION_BEGIN(n)  union
    #define IZOT_UNION_END(n)    __attribute__((__packed__, aligned(1))) n

    #define IZOT_UNION_NESTED_BEGIN(n)   union
    #define IZOT_UNION_NESTED_END(n)     __attribute__((__packed__, aligned(1))) n

    typedef IZOT_STRUCT_BEGIN(IzotWord)
    {
        IzotByte  msb;    /* high-order byte, the most significant byte, the 0x12 in 0x1234 */
        IzotByte  lsb;    /* low-order byte, the least significant byte, the 0x34 in 0x1234 */
    } IZOT_STRUCT_END(IzotWord);

    typedef IZOT_STRUCT_BEGIN(IzotDoubleWord)
    {
        IzotWord  msw;    /* high-order word, the most significant word, the 0x1234 in 0x12345678 */
        IzotWord  lsw;    /* low-order word, the least significant word, the 0x5678 in 0x12345678 */
    } IZOT_STRUCT_END(IzotDoubleWord);

    typedef int32_t     IzotBool;
    #ifndef TRUE
        #define TRUE   1
    #endif
    #ifndef FALSE
        #define FALSE  0
    #endif

#else
    /*
     * Add compiler and platform-specifics here.
     */
    #error No compiler and platform-specifics defined in IzotPlatform.h
#endif

/*
 *  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  No change should be required to the following section, because it
 *  does not contain compiler or target platform dependencies.
 *  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/* some error checking first: */
#if !defined(_IZOT_PLATFORM_DEFINED)
    #error  Platform definition is missing
#endif


/*
 *  **************************************************************************
 *  NEURON C type equivalents:
 *  These types are used by Izot Interface Developer-Builder generated type
 *  definitions. Each NEURON C equivalent type is a host-platform dependent
 *  type definition that is equivalent to the respective NEURON C type.
 *
 *  For your information, a NEURON C "int" and "short" are both 8-bit scalars,
 *  a NEURON C "long" is a 16 bit variable. See the ShortStack or IZOT
 *  documentation for more details about Izot Interface Developer-Builder
 *  generated types.
 *  **************************************************************************
 */

typedef IzotUbits8   ncuChar;	/* equivalent of NEURON C "unsigned char"   */
typedef IzotUbits8   ncuShort;   /* equivalent of NEURON C "unsigned short"  */
typedef IzotUbits8   ncuInt;     /* equivalent of NEURON C "unsigned int"    */
typedef IzotWord     ncuLong;    /* equivalent of NEURON C "unsigned long"   */
typedef IzotBits8    ncsChar;    /* equivalent of NEURON C "signed char"     */
typedef IzotBits8    ncsShort;   /* equivalent of NEURON C "signed short"    */
typedef IzotBits8    ncsInt;     /* equivalent of NEURON C "signed int"      */
typedef IzotWord     ncsLong;    /* equivalent of NEURON C "signed long"     */


#endif  /* _IZOT_PLATFORM_H */

