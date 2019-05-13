/***************************************************************************************
 *
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 *
 *  By downloading, copying, installing or using the software you agree to this license.
 *  If you do not agree to this license, do not download, install, 
 *  copy or use the software.
 *
 *  Copyright (C) 2010-2016, Happytimesoft Corporation, all rights reserved.
 *
 *  Redistribution and use in binary forms, with or without modification, are permitted.
 *
 *  Unless required by applicable law or agreed to in writing, software distributed 
 *  under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 *  CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
 *  language governing permissions and limitations under the License.
 *
****************************************************************************************/

#ifndef	__H_RFC_MD5_H__
#define	__H_RFC_MD5_H__

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short UINT2;

/* UINT4 defines a four byte word */
typedef unsigned int UINT4;


/* MD5 context. */
typedef struct {
	UINT4 state[4];				/* state (ABCD) */
	UINT4 count[2];        		/* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];	/* input buffer */
} MD5_CTX;


#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

#define HASHLEN 16
typedef unsigned char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];


#ifdef __cplusplus
extern "C"{
#endif

ONVIF_API void MD5Init(MD5_CTX *);
ONVIF_API void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
ONVIF_API void MD5Final(unsigned char [16], MD5_CTX *);
ONVIF_API void MD5String(unsigned char *string, unsigned int len, unsigned char *result);

ONVIF_API void CvtHex(HASH Bin, HASHHEX Hex);

#ifdef __cplusplus
}
#endif

#endif	//		__H_RFC_MD5_H__




