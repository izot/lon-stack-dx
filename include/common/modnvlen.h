//
// modvlen.h
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
 * Title: Network Variable Length Management
 *
 * Abstract:
 * This include file defines functions used by the Neuron C
 * Compiler <nv name>::nv_len feature, a built-in network
 * variable property that permits obtaining the length of a
 * network variable, and permits changing the length of a
 * network variable by assigning to the property.
 *
 * These functions can be used to set and get network
 * variable lengths.  Network variable length changes are
 * persistent.
 *
 * Caveats:
 *
 * 1. There is no explicit network management support for
 * setting network variable lengths.  Therefore, it is up to
 * the device application to change network variable lengths
 * when requested by a network tool.  Device applications must
 * implement a SCPTnvType configuration property for each
 * network variable that supports changeable types.  The device
 * application must use the nv_len feature to change the length
 * of a network variable when a network tool updates the network
 * variable's SCPTnvType configuration property, or must report
 * an error via the Node Object if the device does not support
 * the requested change.
 *
 * 2. The specified length must not exceed the length of the
 * network variable as originally defined by the Neuron C
 * application.  The nv_len feature and the functions below do
 * not enforce this.  One approach to enforce this restriction
 * is to declare these network variables with types using the
 * maximum length of 31 bytes.	For example, SNVT_str_asc is a
 * 31-byte type.  Another common solution is to restrict a
 * changeable type network variable to changing between
 * different scalar types so the supported types are all
 * 4-byte or less types.  Device applications that support a
 * maximum of less than 31-bytes should define a
 * SCPTmaxNVLength configuration property for each changeable
 * type network variable and set it to the maximum length
 * supported by the network variable so that network tools
 * can determine the maximum length type that is supported.
 * The SCPTmaxNVLength property is not required; but ,if not
 * implemented, network tools may attempt to change the type
 * to a size larger than supported by the application.	The
 * device application must detect this error and report it
 * via the Node Object.
 *
 * 3. Applications supporting changes to network variable
 * length must be resident in writable, non-volatile memory.
 * This may be implemented using flash memory, EEPROM, or
 * NVRAM.
 *
 * 4. Because the network variable length is stored in
 * checksummed application space, each modification of the
 * network variable length will result in a modification of
 * the application and an application checksum recalculation.
 * If the device is power cycled after the length has been
 * written and before the checksum has been updated, the device
 * will go applicationless.  Therefore, length modification
 * should only be done when application images are available
 * for reloading and a tool is available which supports
 * application loading.
 *
 * 5. If an LNS tool is used with a device that uses changeable
 * types, the SnvtId property for the NetworkVariable object
 * in LNS must be set to the new type for the network variable.
 * If the new type is a SNVT, the SnvtId property must be set
 * to the ID of the new SNVT.  If the new type is a UNVT, the
 * SnvtId proerty must be set to SNVT_TYPELESS (255).
 *
 * 6. This library requires version 12 or greater of the Neuron
 * Firmware.
 */

#ifndef _MODNVLEN_H
#define _MODNVLEN_H

 /*
  *  Function: get_nv_length
  *  Returns the current NV length of NV "nvIndex".
  *
  *  Returns:
  *  int
  *
  *  Remarks:
  *  Gets length of NV
  *
  */
extern   int  get_nv_length(int nvIndex);

/*
 *  Function: get_nv_type
 *  Returns the current NV length of NV "nvIndex".
 *
 *  Returns:
 *  int
 *
 *  Remarks:
 *  Gets type of NV
 *************************************************************
 * Use of the following function is obsolete.  The definition
 * here is only provided for compatibility with older code.
 * The function below only provides the SNVT index.  It will
 * return a zero if the network variable type is currently set
 * to a user type.  More complete type information for a
 * network variable is obtained by using the SCPTnvType
 * property.
 *************************************************************
 *
 * Returns the SNVT type for the "nvIndex" network variable.
 * The value is retrieved from the device's own self-
 * identification data.  A value of '0' means a user-defined
 * type (UNVT).  The index is the SIDATA index, not necessarily
 * identical to the NV's "global index" (which is also known
 * the as address-table index).
 *
 */
extern  int  get_nv_type(int nvIndex);

// FOR FIRMWARE VERSIONS BEGINNING WITH VERSION 14

// Neuron firmware version 14 supports an alternate method to control
// the length of a changeable-type/changeable-length network variable.
// This alternate method is instead of the one documented above (both
// techniques are supported in version 14, but the override feature
// is the one that is preferred, because it is more robust -- the older
// technique above can possibly corrupt memory in case of power failure
// at the instant that the network variable length is being modified
// using the set_nv_type() function (or assigning a value to the NV's
// ::nv_len property)).  The override function may be defined in
// the application program, and its prototype must match the following
// function (note that the override feature is optional, and must be
// enabled through use of a pragma).  See the documentation of the
// function below in the Neuron C Programmer's Guide, and in the
// Neuron C Reference Guide.

/*
 *  Function: get_nv_length_override
 *  Similar to free in  POSIX
 *
 *  Returns:
 *  unsigned int
 *
 *  Remarks:
 *  FOR FIRMWARE VERSIONS BEGINNING WITH VERSION 14
 *
 * Neuron firmware version 14 supports an alternate method to control
 * the length of a changeable-type/changeable-length network variable.
 * This alternate method is instead of the one documented above (both
 * techniques are supported in version 14, but the override feature
 * is the one that is preferred, because it is more robust -- the older
 * technique above can possibly corrupt memory in case of power failure
 * at the instant that the network variable length is being modified
 * using the set_nv_type() function (or assigning a value to the NV's
 * ::nv_len property)).  The override function may be defined in
 * the application program, and its prototype must match the following
 * function (note that the override feature is optional, and must be
 * enabled through use of a pragma).  See the documentation of the
 * function below in the Neuron C Programmer's Guide, and in the
 * Neuron C Reference Guide.
 *
 */
extern unsigned int  get_nv_length_override(unsigned nvindex);

#endif
