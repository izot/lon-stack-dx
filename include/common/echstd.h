//
// echstd.h
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

#ifndef ECHSTD_H
#define ECHSTD_H

#include "BuildOptions.h"

// Do not include in Neuron C.
#ifndef _ECHELON

//
//	Override VC default warnings at level 3
//

#ifdef _MSC_VER
#pragma warning(disable:4244)	// 'argument' : conversion from 'type1' to 'type2', possible loss of data
#ifdef STRICTER_WARNINGS
//#pragma warning (3:	4100)	// unreferenced formal parameter (normally level 4)
#pragma warning (3:		4189)	// local variable is initialized but not referenced (normally level 4)
#pragma warning (3:		4701)	// potentially uninitialized local variable used (normally level 4)
#pragma warning (3:		4706)	// assignment within conditional expression (normally level 4)
#endif	// STRICTER_WARNINGS
#endif	// _MSC_VER

//
// Define means by which unused parameter warnings can be suppressed.
//
#ifdef _MSC_VER
#define WARNING_PUSH                warning (push)
#define WARNING_OFF_UNUSED_PARAM    warning (disable : 4100)
#define WARNING_OFF_INT_TO_BOOL     warning (disable : 4800)
#define WARNING_POP                 warning (pop)
#else
#define WARNING_PUSH
#define WARNING_OFF_UNUSED_PARAM
#define WARNING_OFF_INT_TO_BOOL
#define WARNING_POP
#endif

//
// Macros to suppress warnings for unused parameters
//
#ifdef _DEBUG
#define UNUSED_REL(x)
#else
#define UNUSED_REL(x) x
#endif

#ifdef _MSC_VER
#define UNUSED_ALWAYS(x) x
#else
#define UNUSED_ALWAYS(x)
#endif

// Packing control directives
#define _DCX_PACKING	1	// (not used anymore because gcc does not support symbolic pack() argument
// The AVOID_PACKING macro must always be defined, to make sure packing is avoided when necessary.
// It must be used only as #if !AVOID_PACKING, not #ifndef AVOID_PACKING or similar
#if DCXGEN >= 3
#define AVOID_PACKING 1	// Don't pack
#else
#define AVOID_PACKING 0 // Do pack
#endif

#define MAKE_UNIQUE_NAME(baseName)      MAKE_CONCAT_NAME(baseName,__LINE__)
#define MAKE_CONCAT_NAME(baseName,line) MAKE_CONCAT_NAME2(baseName,line)
#define MAKE_CONCAT_NAME2(baseName,line) baseName ## line

#define COMPILE_TIME_DEF_ASSERT(expr)       \
  enum { MAKE_UNIQUE_NAME(ccAssert_) = 1/(expr) }	


#define COMPILE_TIME_FUNC_ASSERT(expr) \
do { \
	enum { ASSERT_ENUM_FUNC__ = 1/(expr) }; \
} while (0)

//
// The following are the standard definitions to be used by 
// Echelon products.  The definitions in EchelonStandardDefinitions.h
// are expected to be reasonable to export with API products.  Those
// that are not should be included in this file directly.
//
#include "EchelonStandardDefinitions.h"

#ifdef WIN32
#define __monitor
#define BIGADDR
#endif

#endif	// !_ECHELON

#endif
