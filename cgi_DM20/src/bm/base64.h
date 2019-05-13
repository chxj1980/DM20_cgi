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

#ifndef BASE64_H
#define BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

ONVIF_API int base64_encode(unsigned char *source, unsigned int sourcelen, char *target, unsigned int targetlen);
ONVIF_API int base64_decode(const char *source, unsigned char *target, unsigned int targetlen);

#ifdef __cplusplus
}
#endif


#endif /* BASE64_H */


