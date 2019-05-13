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

#ifndef	__H_UTIL_H__
#define	__H_UTIL_H__


/*************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************/


/*************************************************************************/
#if __LINUX_OS__
#define stricmp(ds,ss) 		strcasecmp(ds,ss)
#define strnicmp(ds,ss,len) strncasecmp(ds,ss,len)
#endif

/*************************************************************************/
ONVIF_API int 			get_if_nums();
ONVIF_API unsigned int 	get_if_ip(int index);
ONVIF_API unsigned int 	get_route_if_ip(unsigned int dst_ip);
ONVIF_API unsigned int 	get_default_if_ip();
ONVIF_API int 			get_default_if_mac(unsigned char * mac);
ONVIF_API unsigned int 	get_address_by_name(const char *host_name);
ONVIF_API const char  * get_default_gateway();
ONVIF_API const char  * get_dns_server();
ONVIF_API const char  * get_mask_by_prefix_len(int len);
ONVIF_API int 			get_prefix_len_by_mask(const char *mask);


/*************************************************************************/
ONVIF_API char        * lowercase(char *str);
ONVIF_API char        * uppercase(char *str);
ONVIF_API int 			unicode(char **dst, char *src);

ONVIF_API char        * printmem(char *src, size_t len, int bitwidth);
ONVIF_API char 		  * scanmem(char *src, int bitwidth);

/*************************************************************************/
ONVIF_API time_t 	    get_time_by_string(char * p_time_str);

ONVIF_API SOCKET        tcp_connect_timeout(unsigned int rip, int port, int timeout);

/*************************************************************************/

#if __LINUX_OS__
int             daemon_init();
#endif

#ifdef __cplusplus
}
#endif

#endif	//	__H_UTIL_H__



