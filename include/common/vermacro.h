//
// vermacro.h
//
// Copyright (C) 2023 EnOcean
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
 * Title:- Version Macro Definitions
 *
 * Abstract:
 * This file is included by (possibly customized) VERSION.H to set 
 * version information and can also be included in the RC2 file to set
 * resource version information.
 */

#ifndef __VERMACRO_H__
#define __VERMACRO_H__


/*
 *  Macro: COPYRIGHT_FROM
 *   Default copyright date range.
 *   The values can be overridden by individual source files or
 *   by passing options to the compiler.
 */
#ifndef COPYRIGHT_FROM
#  define COPYRIGHT_FROM            2003
#endif

/*
 *  Macro: COPYRIGHT_TO
 *   Default copyright date range.
 *   The values can be overridden by individual source files or
 *   by passing options to the compiler.
 */
#ifndef COPYRIGHT_TO
#  define COPYRIGHT_TO              2023
#endif

/*
 *  Macro: STRING
 *  these (mystical) macros take the numbers above and create string equivalents
 */
#define STRING(s)                   #s
#define XSTRING(s)                  STRING(s)
#define JOIN(x, y)                  x ## y
#define JOIN3(x, y, z)              x ## y ## z
#define XJOIN(x, y)                 JOIN(x, y)
#define XJOIN3(x, y, z)             JOIN3(x, y, z)
#define STRING_JOIN(x, y)           XSTRING(XJOIN(x, y))

// WARNING - these are not decimal numbers (e.g. beware 08 & 09)
//     - use the _D versions below for decimal equivalents
#define VER_MAJOR                   RELEASE_NUMBER_MAJOR
#define VER_MINOR                   XJOIN(RELEASE_NUMBER_MINOR1, RELEASE_NUMBER_MINOR2)
#ifdef RELEASE_NUMBER_BUILD0
    // three-digit build number (beware of comparisons with older two-digit build numbers)
    #define VER_BUILD               XJOIN3(RELEASE_NUMBER_BUILD0, RELEASE_NUMBER_BUILD1, RELEASE_NUMBER_BUILD2)
#else
    // two-digit build number (for compatibility)
    #define VER_BUILD               XJOIN(RELEASE_NUMBER_BUILD1, RELEASE_NUMBER_BUILD2)
#endif
#define VER_MM                      XJOIN(VER_MAJOR, VER_MINOR)
//#define VER                         XJOIN(VER_MM, VER_BUILD)

#define VER_MAJOR_D                 RELEASE_NUMBER_MAJOR
#define VER_MINOR_D                 (10 * RELEASE_NUMBER_MINOR1 + RELEASE_NUMBER_MINOR2)
#ifdef RELEASE_NUMBER_BUILD0
    // three-digit build number (beware of comparisons with older two-digit build numbers)
    #define VER_BUILD_D             (100 * RELEASE_NUMBER_BUILD0 + 10 * RELEASE_NUMBER_BUILD1 + RELEASE_NUMBER_BUILD2)
#else
    // two-digit build number (for compatibility)
    #define VER_BUILD_D             (10 * RELEASE_NUMBER_BUILD1 + RELEASE_NUMBER_BUILD2)
#endif
#define VER_MM_D                    (100 * VER_MAJOR_D + VER_MINOR_D)
#ifdef RELEASE_NUMBER_BUILD0
    // three-digit build number (beware of comparisons with older two-digit build numbers)
    #define VER_D                   (1000 * VER_MM_D + VER_BUILD_D)
#else
    // two-digit build number (for compatibility)
    #define VER_D                   (100 * VER_MM_D + VER_BUILD_D)
#endif

#define VER_STRING2(v1, v2)         STRING(v1) "." STRING(v2) "\0"
#define VER_STRING3(v1, v2, v3)     STRING(v1) "." STRING(v2) "." STRING(v3) "\0"

#if COPYRIGHT_FROM == COPYRIGHT_TO
#define COPYRIGHT_STRING(from, to)  "Copyright (C) EnOcean " STRING(from) "\0"
#else
#define COPYRIGHT_STRING(from, to)  "Copyright (C) EnOcean " STRING(from) "-" STRING(to) "\0"
#endif


/////////////////////////////////////////////////////////////////////////////


// argument to FILEVERSION and PRODUCTVERSION resources (these are used by InstallShield)
#define VER_RES1                    VER_MAJOR,VER_MINOR,VER_BUILD,0

// argument to FileVersion and/or ProductVersion string table resources
#define VER_RES2                    VER_STRING3(VER_MAJOR, VER_MINOR, VER_BUILD)
#define VER_RES3                    VER_STRING2(VER_MAJOR, VER_MINOR)

// used as type library version
#define VER_RES4                    VER_MAJOR.VER_MINOR

// argument to LegalCopyright string table resource
#ifndef VER_COPYRIGHT
    #define VER_COPYRIGHT           COPYRIGHT_STRING(COPYRIGHT_FROM, COPYRIGHT_TO)
#endif

// argument to CompanyName string table resource
#ifndef VER_COMPANY
    #define VER_COMPANY             "EnOcaean"
#endif

// argument to LegalTrademarks string table resource
#ifndef VER_TRADEMARKS
    #define VER_TRADEMARKS			 "EnOcean, Echelon, LON, LonWorks, 3120, 3150, Digital Home, i.LON, LNS, LonBuilder, LonMaker, LonManager, LonScanner, LonTalk, LonUsers, Neuron, NodeBuilder, ShortStack, and SmartServer are trademarks of EnOcean that may be registered in the United States and other countries."
    #define VER_TRADEMARKS_PARAGRAPH "EnOcean, Echelon, LON, LonWorks, 3120, 3150, Digital Home, i.LON, LNS, LonBuilder, LonMaker, LonManager, LonScanner, LonTalk, LonUsers, Neuron, NodeBuilder, ShortStack, and SmartServer are trademarks of EnOcean that may be registered in the United States and other countries.\n"
#endif

// argument to ProductName string table resource
#ifndef VER_PRODUCT
    #define VER_PRODUCT             "LON Stack DX"
#endif

// backwards-compatibility macros - use the VER_ versions for new code
#define COPYRIGHT                   VER_COPYRIGHT  "\0"
#define COMPANY                     VER_COMPANY    "\0"
#define TRADEMARKS                  VER_TRADEMARKS "\0"
#define PRODUCT                     VER_PRODUCT    "\0"


// 'resync' version numbers to allow eventual resync with above numbers
// for released DLLs with existing major version higher than those used here
// e.g. last released LDV32 version number:          4.16.0.0
//      resyncing version number using VER_RES*_RS(4)
//      when normal version number would be 3.06.02: 4.306.2.0
// i.e. combines major and minor number into minor number field
#define VER_RES1_RS(M)              M, VER_MM, VER_BUILD, 0
#define VER_RES2_RS(M)              VER_STRING3(M, VER_MM, VER_BUILD)
#define VER_RES3_RS(M)              VER_STRING2(M, VER_MM)
#define VER_RES4_RS(M)              M.VER_MM

#endif // __VERMACRO_H__
