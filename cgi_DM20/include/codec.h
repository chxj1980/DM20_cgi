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

#ifndef CODEC_H
#define CODEC_H


typedef enum
{
	VideoCodec_Unknown,
	VideoCodec_H264,
	VideoCodec_JPEG,
	VideoCodec_MPEG4,
	VideoCodec_H265,
} VideoCodec;

typedef enum
{
	AudioCodec_Unknown,
	AudioCodec_PCMU,
	AudioCodec_PCMA,
	AudioCodec_G726,
	AudioCodec_AAC
} AudioCodec;


#endif

