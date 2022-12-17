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

// This file defines build versions.  It can be customized by different
// products to define different version numbers, if necessary.

#ifndef __ECHVERSION_H__
#define __ECHVERSION_H__

#ifdef SUPPORT_PRAGMA_ONCE
#pragma once
#endif

#ifndef __EchVersion_h
#define __EchVersion_h



//
// These version numbers should be updated before a system build.
// This can be done by a program searching for the magic markers.
// Alternatively, the values can be overridden by individual source
// files or by passing options to the compiler, e.g.
//
//      cc /DRELEASE_NUMBER_MAJOR=1 ...
//

#ifndef RELEASE_NUMBER_MAJOR
#  define RELEASE_NUMBER_MAJOR      3       // :MAJOR:   DO NOT DELETE THIS MARKER!
#endif
#ifndef RELEASE_NUMBER_MINOR1
#  define RELEASE_NUMBER_MINOR1     0       // :MINOR1:  DO NOT DELETE THIS MARKER!
#endif
#ifndef RELEASE_NUMBER_MINOR2
#  define RELEASE_NUMBER_MINOR2     0       // :MINOR2:  DO NOT DELETE THIS MARKER!
#endif
#ifndef RELEASE_NUMBER_BUILD0
#  define RELEASE_NUMBER_BUILD0     0       // :BUILD0:  DO NOT DELETE THIS MARKER!
#endif
#ifndef RELEASE_NUMBER_BUILD1
#  define RELEASE_NUMBER_BUILD1     6       // :BUILD1:  DO NOT DELETE THIS MARKER!
#endif
#ifndef RELEASE_NUMBER_BUILD2
#  define RELEASE_NUMBER_BUILD2     7       // :BUILD2:  DO NOT DELETE THIS MARKER!
#endif


#define DEVEL_BIND_NUM 255


// pull-in (standard) macros for operating on these versions numbers
#if DCXGEN < 3
#define VER_PRODUCT             "NES DC-1000 Data Concentrator"
#else
#define VER_PRODUCT             "NES Data Concentrator"
#endif

#define COPYRIGHT_FROM			2003

#include "vermacro.h"

#endif // __ECHVERSION_H__

#endif		// __EchVersion_h
