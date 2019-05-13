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
#ifndef MJPEG_RTP_RX_H
#define MJPEG_RTP_RX_H

#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************************/
enum 
{
	MARKER_SOF0		= 0xc0,		// start-of-frame, baseline scan
	MARKER_SOI		= 0xd8,		// start of image
	MARKER_EOI		= 0xd9,		// end of image
	MARKER_SOS		= 0xda,		// start of scan
	MARKER_DRI		= 0xdd,		// restart interval
	MARKER_DQT		= 0xdb,		// define quantization tables
	MARKER_DHT  	= 0xc4,		// huffman tables
	MARKER_APP_FIRST= 0xe0,
	MARKER_APP_LAST	= 0xef,
	MARKER_COMMENT	= 0xfe,
};


/***************************************************************************************/
void 	 makeDefaultQtables(unsigned char* resultTables, unsigned Q);
unsigned computeJPEGHeaderSize(unsigned qtlen, unsigned dri);
int	 	 createJPEGHeader(unsigned char* buf, unsigned type, unsigned w, unsigned h, unsigned char const* qtables, unsigned qtlen, unsigned dri);

#ifdef __cplusplus
}
#endif


#endif	// MJPEG_RTP_RX_H



