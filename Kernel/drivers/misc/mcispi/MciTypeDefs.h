/* linux/driver/misc/mcispi/MciQueue.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
//----------------------------------------------------------------------------
//
// File Name   :   MciTypeDefs.h
//
// Description :   The data type defined here will be used for all 
//                 SW code development for Mobile TV Solution Project.
//
// Reference Docment : System LSI System Software Lab. Coding Guideline
//                     (TN_SWL_GDL_CodingGuideline_030509)
//
// Revision History :
//        Date               Author             Detail description
//  ----------------    ----------------   ------------------------------
//   March 01, 2005         Mickey Kim          Created
//   July  26, 2005         Mickey Kim          Modified for MFC project 
//   Dec   12, 2005         Mickey Kim          Add 64bit float and integer type
//   Feb   02, 2006         Mickey Kim          Remove endian definition
//   Apr   12, 2007         Mickey Kim          Change file name
//   May   23, 2007         Mickey Kim          Change _ARM_ to _ADS_ in char typedef
//   Oct   20, 2008         Seunghan Kim        Modified for MCI Driver packages
//----------------------------------------------------------------------------
*/

#ifndef  _MCI_TYPE_DEFS_H_
#define  _MCI_TYPE_DEFS_H_

/*
// ---------------------------------------------------------------------------
// Include files
// ---------------------------------------------------------------------------
*/
// ---------------------------- NONE -----------------------------------------

/*
// ---------------------------------------------------------------------------
// Structure/Union Types and define
// ---------------------------------------------------------------------------
*/
// ---------------------------- NONE -----------------------------------------



/*************************************************************************/
/*               Mobile TV Solution Typdef Standard                      */
/*************************************************************************/

/*----------------------------- ---------  -------- ------------ -----   */
/*          Types                NewType    Prefix    Examples   Bytes   */
/*----------------------------- ---------  -------- ------------ -----   */
#if defined _STARCORE_ | defined _ADS_ | defined _RVDS_
  typedef signed       char        Int8; /*    b       bName       1     */
  typedef signed       char *     pInt8; /*   pb       pbName      1     */
#else
  typedef              char        Int8; /*    b       bName       1     */
  typedef              char *     pInt8; /*   pb       pbName      1     */
#endif  // defined _STARCORE_ | defined _ADS_ | defined _RVDS_

typedef   signed       char       SInt8; /*    b       bName       1     */

typedef unsigned       char       UInt8; /*   ub       ubCnt       1     */
typedef unsigned       char *    pUInt8; /*  pub       pubCnt      1     */

typedef          short int        Int16; /*    s       sCnt        2     */
typedef          short int *     pInt16; /*   ps       psCnt       2     */
typedef unsigned short int       UInt16; /*   us       usCnt       2     */
typedef unsigned short int *    pUInt16; /*  pus       pusCnt      2     */

typedef                int        Int32; /*    i       iCnt        4     */
typedef                int *     pInt32; /*   pi       piCnt       4     */
typedef unsigned       int       UInt32; /*   ui       uiCnt       4     */
typedef unsigned       int *    pUInt32; /*  pui       puiCnt      4     */

typedef              float        Float; /*    f       fCnt        4     */
typedef              float *     pFloat; /*   pf       pfCnt       4     */

typedef             double      Float64; /*    d       dCnt        8     */
typedef             double *   pFloat64; /*   pd       pdCnt       8     */


typedef               int           Bool; /* cond       condIsTrue  4     */
typedef               int  *       pBool;
typedef               void          Void; /*    v       vFlag       4     */
typedef               void *       pVoid; /*   pv       pvFlag      4     */



/*************************************************************************/
/*                   System Specific Standard                            */
/*************************************************************************/
typedef Int32                  ERRORCODE;  /* Error Code Define */

typedef             long            Long;
typedef unsigned    long	       ULong;
typedef unsigned    long *        pULong;
typedef	unsigned    char            Byte;
typedef unsigned    char *         pByte;
typedef             char            Char;
typedef             char *         pChar;
typedef	unsigned    char           UChar;
typedef unsigned    char *        pUChar;
#ifdef _WIN32
typedef             LPTSTR	       pTStr;
typedef             LPCTSTR		  pCTStr;
typedef             LPSTR	        pStr;
typedef             LPCSTR		   pCStr;
typedef             TCHAR		   TChar;
typedef             TCHAR*		  pTChar;
#endif

#ifndef STATIC
#define STATIC                      static
#endif // STATIC
    
#ifndef TRUE
#define TRUE                        (1)
#endif

#ifndef FALSE
#define FALSE                       (0)
#endif


#ifndef NULL
#ifdef __cplusplus
#define NULL                        (0)
#else
#define NULL                        ((void *)0)
#endif
#endif

/*************************************************************************/
/*                   OS Specific Object                                  */
/*************************************************************************/
typedef struct _MCIOSAL_EVENT	*		    H_EVENT;
typedef struct _MCIOSAL_CODELOCK*           H_LOCK;
typedef struct _MCIOSAL_FILE	*		    H_FILE;
typedef struct _MCIOSAL_THREAD	*		    H_THREAD;
typedef struct _MCIOSAL_DEVICE	*		    H_DRIVER;
typedef struct _MCIOSAL_PHYSMEM *           H_PHYSMEM;
//typedef struct _MCIOSAL_DEVDLL  *           H_DEVDLL;

#endif  /* _MCI_TYPE_DEFS_H_ */
