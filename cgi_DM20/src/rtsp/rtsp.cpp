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

#include <string.h>
#include <stdio.h>
#include "sys_inc.h"
#include "rtp.h"
#include "rtsp.h"
#include "mjpeg_rtp_rx.h"
#include "mpeg4_rtp_rx.h"
#include "BitVector.h"
#include "base64.h"

/***************************************************************************************/
void * rtsp_rx_thread(void * argv)
{
	CRtsp * pRtsp = (CRtsp *)argv;

	pRtsp->rx_thread();

	return NULL;
}

/***************************************************************************************/
CRtsp::CRtsp(void)
{
	memset(&m_rua, 0, sizeof(RUA));
	memset(&m_url, 0, sizeof(m_url));
	memset(&m_ip, 0, sizeof(m_ip));
	m_nport = 554;

	m_rua.session_timeout = 60;
	
	m_pNotify = 0;
	m_pUserdata = 0;
	m_pVideoCB = 0;
	m_pAudioCB = 0;
	m_pMutex = sys_os_create_mutex();

	m_h264_offset = 0;
	m_h265_offset = 0;
	m_jpeg_offset = 0;
	m_mpeg4_offset = 0;
	m_mpeg4_header_len = 0;

	m_bExpectDONFields = FALSE;
    m_nPreviousNALUnitDON = 0;
    m_nCurrentNALUnitAbsDon = (uint64)(~0);
	
	m_pAudioConfig = NULL;
	m_nAudioConfigLen = 0;
	m_aac_offset = 0;
	
    m_nSizeLength = 13;
	m_nIndexLength = 3;
	m_nIndexDeltaLength = 3;	
	
    m_bRunning = TRUE;
	m_rtspRxTid = 0;	
	
	m_VideoCodec = VideoCodec_Unknown;
	m_AudioCodec = AudioCodec_Unknown;
	m_nSamplerate = 0;
	m_nChannels = 0;
}

CRtsp::~CRtsp(void)
{
	rtsp_close();
}

BOOL CRtsp::rtsp_client_start()
{	
	m_rua.rport = m_nport;
	strcpy(m_rua.ripstr, m_ip);
	strcpy(m_rua.uri, m_url);

	if (rua_init_connect(&m_rua) == FALSE)
	{
		log_print(LOG_ERR, "rua_init_connect fail!!!\r\n");
		return FALSE;
	}

	this->m_rua.cseq = 1;
	HRTSP_MSG * tx_msg = rua_build_options(&m_rua);
	if (tx_msg)
	{
		send_free_rtsp_msg(&m_rua, tx_msg);
	}

	this->m_rua.state = RCS_OPTIONS;

	return TRUE;
}

BOOL CRtsp::rua_init_connect(RUA * p_rua)
{
#if 0
	SOCKET fd = socket(AF_INET,SOCK_STREAM,0);
	if (fd == -1)
	{
		log_print(LOG_ERR, "rua_init_connect::socket fail!!!\r\n");
		return FALSE;
	}

	log_print(LOG_INFO, "rua_init_connect::fd = %d\r\n",fd);

	if (p_rua->lport != 0)
	{
		struct sockaddr_in addr;

		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(p_rua->lport);

		if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		{
			log_print(LOG_ERR, "rua_init_connect::bind lport[%u] fail.\n", p_rua->lport);
			closesocket(fd);
			return FALSE;
		}
	}

	struct sockaddr_in raddr;

	raddr.sin_family = AF_INET;
	raddr.sin_addr.s_addr = inet_addr(p_rua->ripstr);
	raddr.sin_port = htons(p_rua->rport);

    struct timeval timeo = {5, 0};
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeo, sizeof(timeo));
	
	if (connect(fd, (struct sockaddr *)&raddr, sizeof(struct sockaddr_in)) == -1)
	{
		log_print(LOG_ERR, "rua_init_connect::connect %s:%u fail!!!\r\n", p_rua->ripstr, p_rua->rport);
		closesocket(fd);
		return FALSE;
	}
#else
	int fd = tcp_connect_timeout(inet_addr(p_rua->ripstr), p_rua->rport, 5000);
#endif

	if (fd > 0)
	{
		int len = 1024*1024;
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&len, sizeof(int)))
		{
			log_print(LOG_ERR, "setsockopt SO_RCVBUF error!\n");
			return FALSE;
		}
		
		p_rua->fd = fd;

		return TRUE;
	}

	return FALSE;
}


void CRtsp::rtsp_send_sps_pps_para(RUA * p_rua)
{
	int pt;
	char sps[1000], pps[1000] = {'\0'};
	
	if (!get_sdp_h264_desc(p_rua, &pt, sps, sizeof(sps)))
	{
		return;
	}

	char * ptr = strchr(sps, ',');
	if (ptr && ptr[1] != '\0')
	{
		*ptr = '\0';
		strcpy(pps, ptr+1);
	}

	unsigned char sps_pps[1000];
	sps_pps[0] = 0x0;
	sps_pps[1] = 0x0;
	sps_pps[2] = 0x0;
	sps_pps[3] = 0x1;
	
	int len = base64_decode(sps, sps_pps+4, sizeof(sps_pps)-4);
	if (len <= 0)
	{
		return;
	}

	sys_os_mutex_enter(m_pMutex);
	
	if (m_pVideoCB)
	{
		m_pVideoCB(sps_pps, len+4, 0, 0, m_pUserdata);
	}
	
	if (pps[0] != '\0')
	{		
		len = base64_decode(pps, sps_pps+4, sizeof(sps_pps)-4);
		if (len > 0)
		{
			if (m_pVideoCB)
			{
				m_pVideoCB(sps_pps, len+4, 0, 0, m_pUserdata);
			}
		}
	}

	sys_os_mutex_leave(m_pMutex);
}

void CRtsp::rtsp_send_vps_sps_pps_para(RUA * p_rua)
{
	int pt;
	char vps[256] = {'\0'}, sps[1000] = {'\0'}, pps[1000] = {'\0'};
	
	if (!get_sdp_h265_desc(p_rua, &pt, &m_bExpectDONFields, vps, sizeof(vps), sps, sizeof(sps), pps, sizeof(pps)))
	{
		return;
	}

	unsigned char buff[1000];
	buff[0] = 0x0;
	buff[1] = 0x0;
	buff[2] = 0x0;
	buff[3] = 0x1;

	sys_os_mutex_enter(m_pMutex);
	
	if (vps[0] != '\0')
	{
		int len = base64_decode(vps, buff+4, sizeof(buff)-4);
		if (len <= 0)
		{
			return;
		}

		if (m_pVideoCB)
		{
			m_pVideoCB(buff, len+4, 0, 0, m_pUserdata);
		}
	}

	if (sps[0] != '\0')
	{
		int len = base64_decode(sps, buff+4, sizeof(buff)-4);
		if (len <= 0)
		{
			return;
		}

		if (m_pVideoCB)
		{
			m_pVideoCB(buff, len+4, 0, 0, m_pUserdata);
		}
	}

	if (pps[0] != '\0')
	{
		int len = base64_decode(pps, buff+4, sizeof(buff)-4);
		if (len <= 0)
		{
			return;
		}

		if (m_pVideoCB)
		{
			m_pVideoCB(buff, len+4, 0, 0, m_pUserdata);
		}
	}

	sys_os_mutex_leave(m_pMutex);
}

void CRtsp::rtsp_get_mpeg4_config(RUA * p_rua)
{
	int pt;
	char config[1000];
	
	if (!get_sdp_mpeg4_desc(p_rua, &pt, config, sizeof(config)))
	{
		return;
	}

    unsigned configLen;
	unsigned char* configData = parseGeneralConfigStr(config, configLen);
    if (configData)
    {
    	m_mpeg4_header_len = configLen;
    	memcpy(m_fu_buf, configData, configLen);

        delete[] configData;   
    }
}

void CRtsp::rtsp_get_aac_config(RUA * p_rua)
{
	int pt = 0;
	int sizelength = 13;
	int indexlength = 3;
	int indexdeltalength = 3;
	char config[128];
	
	if (!get_sdp_aac_desc(p_rua, &pt, &sizelength, &indexlength, &indexdeltalength, config, sizeof(config)))
	{
		return;
	}

	m_nSizeLength = sizelength;
	m_nIndexLength = indexlength;
	m_nIndexDeltaLength = indexdeltalength;

    m_pAudioConfig = parseGeneralConfigStr(config, m_nAudioConfigLen);
}


/***********************************************************************
*
* Close RUA
*
************************************************************************/
void CRtsp::rtsp_client_stop(RUA * p_rua)
{
	if (p_rua->fd > 0)
	{
		HRTSP_MSG * tx_msg = rua_build_teardown(p_rua);
		if (tx_msg)
			send_free_rtsp_msg(p_rua,tx_msg);
	}
}

int CRtsp::rtsp_client_state(RUA * p_rua, HRTSP_MSG * rx_msg)
{
	HRTSP_MSG * tx_msg = NULL;

	if (rx_msg->msg_type == 0)	// Request message?
	{
		return -1;
	}

	switch (p_rua->state)
	{
	case RCS_NULL:
		break;

	case RCS_OPTIONS:
		if (rx_msg->msg_sub_type == 200)
		{
			//get supported command list
			p_rua->gp_cmd = rtsp_is_support_get_parameter_cmd(rx_msg);
			
			p_rua->cseq++;
			tx_msg = rua_build_describe(p_rua);
			if (tx_msg)
			{
				send_free_rtsp_msg(p_rua, tx_msg);
			}

			p_rua->state = RCS_DESCRIBE;
		}
		else if (rx_msg->msg_sub_type == 401)
		{
		    p_rua->need_auth = TRUE;
		    
		    if (rtsp_get_digest_info(rx_msg, &(p_rua->user_auth_info)))
			{
			    p_rua->auth_mode = 1;
				sprintf(p_rua->user_auth_info.auth_uri, "%s", p_rua->uri);                
			}
			else
			{
			    p_rua->auth_mode = 0;
			}

			p_rua->cseq++;
			tx_msg = rua_build_options(p_rua);
        	if (tx_msg)
        	{
        		send_free_rtsp_msg(p_rua, tx_msg);
        	}
		}
		break;
		
	case RCS_DESCRIBE:
		if (rx_msg->msg_sub_type == 200)
		{
			//Session
			get_rtsp_session_info(rx_msg, p_rua->sid, sizeof(p_rua->sid)-1, &p_rua->session_timeout);

			//if(p_rua->sid[0] == '\0')
			//{
			//	sprintf(p_rua->sid, "%x%x", rand(), rand());
			//}

			char cseq_buf[32];
			char cbase[256];

			get_rtsp_msg_cseq(rx_msg, cseq_buf, sizeof(cseq_buf)-1);

			//Content-Base			
			if (get_rtsp_cbase_info(rx_msg, cbase, sizeof(cbase)-1))
			{
				strncpy(p_rua->uri, cbase, sizeof(p_rua->uri)-1);
			}

			find_rtsp_sdp_control(rx_msg, p_rua->v_ctl, (char *)"video", sizeof(p_rua->v_ctl)-1);
			find_rtsp_sdp_control(rx_msg, p_rua->a_ctl, (char *)"audio", sizeof(p_rua->a_ctl)-1);

			if (get_rtsp_media_info(p_rua, rx_msg))
			{
				get_rtsp_video_media_info();
				get_rtsp_audio_media_info();
			}

			//Send SETUP
			if (p_rua->v_ctl[0] != '\0')
			{
				p_rua->v_interleaved = 0;
				p_rua->cseq++;
				tx_msg = rua_build_setup(p_rua,0);
				if(tx_msg)
					send_free_rtsp_msg(p_rua,tx_msg);
			}

			// rua_notify_emsg(p_rua,PUEVT_ALERT);

			p_rua->state = RCS_INIT_V;
		}
		else if (rx_msg->msg_sub_type == 401)
		{
		    p_rua->need_auth = TRUE;
		    
		    if (rtsp_get_digest_info(rx_msg, &(p_rua->user_auth_info)))
			{
			    p_rua->auth_mode = 1;
				sprintf(p_rua->user_auth_info.auth_uri, "%s", p_rua->uri);                
			}
			else
			{
			    p_rua->auth_mode = 0;
			}

			p_rua->cseq++;
			tx_msg = rua_build_describe(p_rua);
        	if (tx_msg)
        	{
        		send_free_rtsp_msg(p_rua, tx_msg);
        	}
		}
		break;

	case RCS_INIT_V:
		if (rx_msg->msg_sub_type == 200)
		{
			char cbase[256];
			
			//Content-Base			
			if (get_rtsp_cbase_info(rx_msg, cbase, sizeof(cbase)-1))
			{
				strncpy(p_rua->uri, cbase, sizeof(p_rua->uri)-1);
				sprintf(p_rua->user_auth_info.auth_uri, "%s", p_rua->uri);
			}
			
			//Session
			if(p_rua->sid[0] == '\0')
				get_rtsp_session_info(rx_msg, p_rua->sid, sizeof(p_rua->sid)-1, &p_rua->session_timeout);

			if(p_rua->sid[0] == '\0')
			{
				sprintf(p_rua->sid, "%x%x", rand(), rand());
			}
			
			p_rua->cseq++;

			if (p_rua->a_ctl[0] != '\0')
			{
				p_rua->a_interleaved = 2;
				tx_msg = rua_build_setup(p_rua, 1);
				if (tx_msg)
					send_free_rtsp_msg(p_rua,tx_msg);

				p_rua->state = RCS_INIT_A;
			}
			else
			{
				//only video without audio
				tx_msg = rua_build_play(p_rua);
				if (tx_msg)
					send_free_rtsp_msg(p_rua,tx_msg);
				p_rua->state = RCS_READY;
			}
		}
		break;

	case RCS_INIT_A:
		if (rx_msg->msg_sub_type == 200)
		{
			//Session
			if(p_rua->sid[0] == '\0')
				get_rtsp_session_info(rx_msg, p_rua->sid, sizeof(p_rua->sid)-1, &p_rua->session_timeout);
				
			p_rua->cseq++;
			tx_msg = rua_build_play(p_rua);
			if (tx_msg)
				send_free_rtsp_msg(p_rua,tx_msg);
			p_rua->state = RCS_READY;
		}
		else
		{
			//error handle
		}
		break;

	case RCS_READY:
		if (rx_msg->msg_sub_type == 200)
		{
			p_rua->state = RCS_PLAYING;
			p_rua->keepalive_time = sys_os_get_ms();

			log_print(LOG_DBG, "session timeout : %d\n", p_rua->session_timeout);

			if (m_AudioCodec == AudioCodec_AAC)
			{
				rtsp_get_aac_config(p_rua);
			}
			
			send_notify(RTSP_EVE_CONNSUCC);

			if (m_VideoCodec == VideoCodec_H264)
			{
				rtsp_send_sps_pps_para(p_rua);	//获取视频流
			}	
			else if (m_VideoCodec == VideoCodec_MPEG4)
			{
				rtsp_get_mpeg4_config(p_rua);
			}
			else if (m_VideoCodec == VideoCodec_H265)
			{
				rtsp_send_vps_sps_pps_para(p_rua);	//获取视频流1
			}
		}
		else
		{
			//error handle
			return -1;
		}
		break;

	case RCS_PLAYING:
		break;

	case RCS_RECORDING:
		break;
	}		

	return 0;
}


int CRtsp::rtsp_pkt_find_end(char * p_buf)
{
	int end_off = 0;
	int sip_pkt_finish = 0;

	while (p_buf[end_off] != '\0')
	{
		if ((p_buf[end_off] == '\r' && p_buf[end_off+1] == '\n') &&
			(p_buf[end_off+2] == '\r' && p_buf[end_off+3] == '\n'))
		{
			sip_pkt_finish = 1;
			break;
		}

		end_off++;
	}

	if (sip_pkt_finish)
		return (end_off + 4);
		
	return 0;
}

void CRtsp::rtsp_data_rx(LPBYTE lpData, int rlen)
{
	RILF * p_rilf = (RILF *)lpData;
	uint8 * p_rtp = (uint8 *)p_rilf + 4;
	int	rtp_len = rlen - 4;

	if (rtp_len >= 2 && RTP_PT_IS_RTCP(p_rtp[1])) // now, don't handle rtcp packet ...
	{
		return;
	}

	// Check for the 12-byte RTP header:
    if (rtp_len < 12) return;
    unsigned rtpHdr = ntohl(*(uint32*)p_rtp); p_rtp+=4; rtp_len-=4;
    BOOL rtpMarkerBit = (rtpHdr&0x00800000) != 0;
    unsigned rtpSeq = ntohs(rtpHdr&0xFFFF);
    unsigned rtpTimestamp = ntohl(*(uint32*)(p_rtp)); p_rtp+=4; rtp_len-=4;
    unsigned rtpSSRC = ntohl(*(uint32*)(p_rtp)); p_rtp+=4; rtp_len-=4;

	// Check the RTP version number (it should be 2):
    if ((rtpHdr&0xC0000000) != 0x80000000) return;

    // Skip over any CSRC identifiers in the header:
    unsigned cc = (rtpHdr>>24)&0x0F;
    if (rtp_len < cc*4) return;
    p_rtp+=cc*4; rtp_len-=cc*4;

    // Check for (& ignore) any RTP header extension
    if (rtpHdr&0x10000000) 
    {
		if (rtp_len < 4) return;
		unsigned extHdr = ntohl(*(uint32*)(p_rtp)); p_rtp+=4; rtp_len-=4;
		unsigned remExtSize = 4*(extHdr&0xFFFF);
		if (rtp_len < remExtSize) return;
		p_rtp+=remExtSize; rtp_len-=remExtSize;
    }

    // Discard any padding bytes:
    if (rtpHdr&0x20000000) 
    {
		if (rtp_len == 0) return;
		unsigned numPaddingBytes = (unsigned)p_rtp[rtp_len-1];
		if (rtp_len < numPaddingBytes) return;
		rtp_len -= numPaddingBytes;
    }
    
	if (p_rilf->channel == m_rua.v_interleaved)
	{
		video_rtp_rx(p_rtp, rtp_len, rtpSeq, rtpTimestamp, rtpMarkerBit);
	}
	else if (p_rilf->channel == m_rua.a_interleaved)
	{
		audio_rtp_rx(p_rtp, rtp_len, rtpSeq, rtpTimestamp, rtpMarkerBit);
	}	
}

int CRtsp::rtsp_msg_parser(RUA * p_rua)
{
	int rtsp_pkt_len = rtsp_pkt_find_end(p_rua->rcv_buf);
	if (rtsp_pkt_len == 0) // wait for next recv
		return RTSP_PARSE_MOREDATE;

	HRTSP_MSG * rx_msg = get_rtsp_msg_buf();
	if (rx_msg == NULL)
	{
		log_print(LOG_ERR, "rtsp_tcp_rx::get_rtsp_msg_buf return null!!!\r\n");
		return RTSP_PARSE_FAIL;
	}
	
	memcpy(rx_msg->msg_buf, p_rua->rcv_buf, p_rua->rcv_dlen);
	p_rua->rcv_buf[p_rua->rcv_dlen] = '\0';
	log_print(LOG_DBG, "RX << %s\r\n", p_rua->rcv_buf);

	int parse_len = rtsp_msg_parse_part1(rx_msg->msg_buf,rtsp_pkt_len,rx_msg);
	if (parse_len != rtsp_pkt_len)	//parse error
	{
		log_print(LOG_ERR, "rtsp_tcp_rx::rtsp_msg_parse_part1=%d, rtsp_pkt_len=%d!!!\r\n",parse_len,rtsp_pkt_len);
		free_rtsp_msg(rx_msg);

		p_rua->rcv_dlen = 0;
		return RTSP_PARSE_FAIL;
	}
	
	if(rx_msg->ctx_len > 0)	
	{
		if(p_rua->rcv_dlen < (parse_len + rx_msg->ctx_len))
		{
			free_rtsp_msg(rx_msg);
			return RTSP_PARSE_MOREDATE;
		}

		memcpy(rx_msg->msg_buf+rtsp_pkt_len, p_rua->rcv_buf+rtsp_pkt_len, rx_msg->ctx_len);

		int sdp_parse_len = rtsp_msg_parse_part2(rx_msg->msg_buf+rtsp_pkt_len,rx_msg->ctx_len,rx_msg);
		if(sdp_parse_len != rx_msg->ctx_len)
		{
		}
		parse_len += rx_msg->ctx_len;
	}
	
	if (parse_len < p_rua->rcv_dlen)
	{
		while(p_rua->rcv_buf[parse_len] == ' ' || 
			p_rua->rcv_buf[parse_len] == '\r' || p_rua->rcv_buf[parse_len] == '\n') parse_len++;

		memmove(p_rua->rcv_buf, p_rua->rcv_buf + parse_len, p_rua->rcv_dlen - parse_len);
		p_rua->rcv_dlen -= parse_len;
		p_rua->rcv_buf[p_rua->rcv_dlen] = '\0';
	}
	else
		p_rua->rcv_dlen = 0;

	rtsp_client_state(p_rua, rx_msg);	//获取视频流
	free_rtsp_msg(rx_msg);

	return RTSP_PARSE_SUCC;
}

int CRtsp::rtsp_tcp_rx()
{
	RUA * p_rua = &(this->m_rua);

	if (p_rua->fd <= 0)
		return -1;

    int sret;
    fd_set fdread;
    struct timeval tv = {1, 0};

    FD_ZERO(&fdread);
    FD_SET(p_rua->fd, &fdread); 
    
    sret = select(p_rua->fd+1, &fdread, NULL, NULL, &tv); 
    if (sret == 0) // Time expired 
    { 
        return RTSP_RX_TIMEOUT; 
    }
    else if (!FD_ISSET(p_rua->fd, &fdread))
    {
        return RTSP_RX_TIMEOUT;
    }
    
	if (p_rua->rtp_rcv_buf == NULL || p_rua->rtp_t_len == 0)
	{
		int rlen = recv(p_rua->fd, p_rua->rcv_buf+p_rua->rcv_dlen, 2048-p_rua->rcv_dlen, 0);
		if (rlen <= 0)
		{
			log_print(LOG_WARN, "RTST recv thread exit, ret = %d, err = %s\r\n",rlen,sys_os_get_socket_error());	//recv error, connection maybe disconn?
			return RTSP_RX_FAIL;
		}

		p_rua->rcv_dlen += rlen;
	}
	else
	{
		int rlen = recv(p_rua->fd, p_rua->rtp_rcv_buf+p_rua->rtp_rcv_len, p_rua->rtp_t_len-p_rua->rtp_rcv_len, 0);
		if (rlen <= 0)
		{
			log_print(LOG_WARN, "RTST recv thread exit，ret = %d, err = %s\r\n",rlen,sys_os_get_socket_error());	//recv error, connection maybe disconn?
			return RTSP_RX_FAIL;
		}

		p_rua->rtp_rcv_len += rlen;
		if(p_rua->rtp_rcv_len == p_rua->rtp_t_len)
		{
			rtsp_data_rx((uint8*)p_rua->rtp_rcv_buf, p_rua->rtp_rcv_len);
			
			free(p_rua->rtp_rcv_buf);
			p_rua->rtp_rcv_buf = NULL;
			p_rua->rtp_rcv_len = 0;
			p_rua->rtp_t_len = 0;
		}
		
		return RTSP_RX_SUCC;
	}

rx_point:

	if (is_rtsp_msg(p_rua->rcv_buf))	//Is RTSP Packet?
	{
		int ret = rtsp_msg_parser(p_rua);	//获取视频流
		if (ret == RTSP_PARSE_FAIL)
		{
			return RTSP_RX_FAIL;
		}
		else if (ret == RTSP_PARSE_MOREDATE)
		{
			return RTSP_RX_SUCC;
		}

		if (p_rua->rcv_dlen > 16)
		{
			goto rx_point;
		}
	}
	else
	{
		RILF * p_rilf = (RILF *)(p_rua->rcv_buf);
		if (p_rilf->magic != 0x24)
		{		
			log_print(LOG_WARN, "rtsp_tcp_rx::p_rilf->magic[0x%02X]!!!\r\n", p_rilf->magic);
			return RTSP_RX_FAIL;
		}
		
		unsigned short rtp_len = ntohs(p_rilf->rtp_len);
		if (rtp_len > (p_rua->rcv_dlen - 4))
		{			
			p_rua->rtp_rcv_buf = (char *)malloc(rtp_len+4);
			if (p_rua->rtp_rcv_buf == NULL) 
			    return RTSP_RX_FAIL;
			    
			memcpy(p_rua->rtp_rcv_buf, p_rua->rcv_buf, p_rua->rcv_dlen);
			p_rua->rtp_rcv_len = p_rua->rcv_dlen;
			p_rua->rtp_t_len = rtp_len+4;

			p_rua->rcv_dlen = 0;

			return RTSP_RX_SUCC;
		}
			
		if (p_rilf->channel == p_rua->v_interleaved || p_rilf->channel == p_rua->a_interleaved)
		{
			rtsp_data_rx((uint8*)p_rilf, rtp_len+4);
		}

		p_rua->rcv_dlen -= rtp_len+4;
		if (p_rua->rcv_dlen > 0)
		{
			memmove(p_rua->rcv_buf, p_rua->rcv_buf+rtp_len+4, p_rua->rcv_dlen);
		}

		if (p_rua->rcv_dlen > 16)
		{
			goto rx_point;
		}
	}

	return RTSP_RX_SUCC;
}


BOOL CRtsp::rtsp_start(char* url, char* ip, int port, const char * user, const char * pass)
{
	if (m_rua.state != RCS_NULL)
	{
		rtsp_play();
		return TRUE;
	}

	memset(&m_rua, 0, sizeof(RUA));
	memset(m_url, 0, sizeof(m_url));
	memset(m_ip, 0, sizeof(m_ip));

	if (user)
	{
		strcpy(m_rua.user_auth_info.auth_name, user);
	}

	if (pass)
	{
		strcpy(m_rua.user_auth_info.auth_pwd, pass);
	}
	
	if (url)
	{
		strcpy(m_url, url);
	}
	
	if (ip)
	{
		strcpy(m_ip, ip);
	}
	
	m_nport = port;

    m_bRunning = TRUE;
	//获取视频流的线程
	m_rtspRxTid = sys_os_create_thread((void *)rtsp_rx_thread, this);
	if (m_rtspRxTid == 0)
	{
		log_print(LOG_ERR, "start_video::pthread_create rtsp_rx_thread failed!!!\r\n");
		return FALSE;
	}
	
	return TRUE;
}

BOOL CRtsp::rtsp_play()
{
	m_rua.cseq++;
	HRTSP_MSG * tx_msg = rua_build_play(&m_rua);	
	if (tx_msg)
		send_free_rtsp_msg(&m_rua,tx_msg);
	return TRUE;
}

BOOL CRtsp::rtsp_stop()
{
	m_rua.cseq++;
	HRTSP_MSG * tx_msg = rua_build_teardown(&m_rua);
	if (tx_msg)
		send_free_rtsp_msg(&m_rua,tx_msg);	
	m_rua.state = RCS_NULL;
	return TRUE;
}

BOOL CRtsp::rtsp_pause()
{
	m_rua.cseq++;
	HRTSP_MSG * tx_msg = rua_build_pause(&m_rua);	
	if(tx_msg)
		send_free_rtsp_msg(&m_rua,tx_msg);	
	return TRUE;
}

BOOL CRtsp::rtsp_close()
{
	sys_os_mutex_enter(m_pMutex);
	m_pAudioCB = NULL;
	m_pVideoCB = NULL;
	m_pNotify = NULL;
	m_pUserdata = NULL;
	sys_os_mutex_leave(m_pMutex);
	
    m_bRunning = FALSE;
	while(m_rtspRxTid != 0)
	{
		usleep(10 * 1000);	// 10ms wait
	}
	
	m_h264_offset = 0;
	m_h265_offset = 0;
	m_jpeg_offset = 0;
	m_mpeg4_offset = 0;
	m_mpeg4_header_len = 0;

	if (m_pAudioConfig)
	{
		delete [] m_pAudioConfig;
		m_pAudioConfig = NULL;
	}	
	
	if (m_pMutex)
	{
		sys_os_destroy_sig_mutx(m_pMutex);
		m_pMutex = NULL;
	}
	
	return TRUE;
}

BOOL CRtsp::rtp_mjpeg_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit) 
{
	unsigned char* headerStart = lpData;
	unsigned packetSize = rlen;
	unsigned resultSpecialHeaderSize = 0;
	
	unsigned char* qtables = NULL;
	unsigned qtlen = 0;
	unsigned dri = 0;

	// There's at least 8-byte video-specific header
	/*
	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	| Type-specific |              Fragment Offset                  |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|      Type     |       Q       |     Width     |     Height    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */
  	if (packetSize < 8) return FALSE;

  	resultSpecialHeaderSize = 8;

	unsigned Offset = (unsigned)((uint32)headerStart[1] << 16 | (uint32)headerStart[2] << 8 | (uint32)headerStart[3]);
	unsigned Type = (unsigned)headerStart[4];
	unsigned type = Type & 1;
	unsigned Q = (unsigned)headerStart[5];
	unsigned width = (unsigned)headerStart[6] * 8;
	unsigned height = (unsigned)headerStart[7] * 8;
	
	if (width == 0) width = 256*8; 		// special case
	if (height == 0) height = 256*8; 	// special case

	if (Type > 63) 
	{
		// Restart Marker header present
		/*
		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		|       Restart Interval        |F|L|       Restart Count       |
		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		  */
		if (packetSize < resultSpecialHeaderSize + 4) return FALSE;

		unsigned RestartInterval = (unsigned)((uint16)headerStart[resultSpecialHeaderSize] << 8 | (uint16)headerStart[resultSpecialHeaderSize + 1]);
		dri = RestartInterval;
		resultSpecialHeaderSize += 4;
	}

	if (Offset == 0) 
	{
		if (Q > 127) 
		{
			// Quantization Table header present
			/*
			0                   1                   2                   3
			0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			|      MBZ      |   Precision   |             Length            |
			+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			|                    Quantization Table Data                    |
			|                              ...                              |
			+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			*/
			if (packetSize < resultSpecialHeaderSize + 4) return FALSE;

			unsigned MBZ = (unsigned)headerStart[resultSpecialHeaderSize];
			if (MBZ == 0) 
			{
				// unsigned Precision = (unsigned)headerStart[resultSpecialHeaderSize + 1];
				unsigned Length = (unsigned)((uint16)headerStart[resultSpecialHeaderSize + 2] << 8 | (uint16)headerStart[resultSpecialHeaderSize + 3]);

				//ASSERT(Length == 128);

				resultSpecialHeaderSize += 4;

				if (packetSize < resultSpecialHeaderSize + Length) return FALSE;

				qtlen = Length;
				qtables = &headerStart[resultSpecialHeaderSize];

				resultSpecialHeaderSize += Length;
			}
		}
	}

	// If this is the first (or only) fragment of a JPEG frame
	if (Offset == 0) 
	{
		unsigned char newQtables[128];
		if (qtlen == 0) 
		{
			// A quantization table was not present in the RTP JPEG header,
			// so use the default tables, scaled according to the "Q" factor:
			makeDefaultQtables(newQtables, Q);
			qtables = newQtables;
			qtlen = sizeof newQtables;
		}
		
		m_jpeg_offset = createJPEGHeader(m_fu_buf, type, width, height, qtables, qtlen, dri);
	}

	if ((m_jpeg_offset+packetSize-resultSpecialHeaderSize) >= sizeof(m_fu_buf))
	{
		log_print(LOG_ERR, "MJPEG fragment Reassembly Packet Too Big %d!!!",m_jpeg_offset+packetSize-resultSpecialHeaderSize);
		return FALSE;
	}
	
	memcpy(m_fu_buf+m_jpeg_offset, headerStart+resultSpecialHeaderSize, packetSize - resultSpecialHeaderSize);
	m_jpeg_offset += packetSize - resultSpecialHeaderSize;

	// The RTP "M" (marker) bit indicates the last fragment of a frame:
	if (markbit)
	{
		if (m_jpeg_offset >= 2 && !(m_fu_buf[m_jpeg_offset-2] == 0xFF && m_fu_buf[m_jpeg_offset-1] == MARKER_EOI)) 
		{
    		m_fu_buf[m_jpeg_offset++] = 0xFF;
    		m_fu_buf[m_jpeg_offset++] = MARKER_EOI;
  		}

  		sys_os_mutex_enter(m_pMutex);
		if (m_pVideoCB)
	    {
	        m_pVideoCB(m_fu_buf, m_jpeg_offset, ts, seq, m_pUserdata);
	    }
	    sys_os_mutex_leave(m_pMutex);

	    m_jpeg_offset = 0;
	}

	return TRUE;
}

BOOL CRtsp::rtp_h264_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit) 
{
	unsigned char* headerStart = lpData;
	unsigned packetSize = rlen;
	unsigned numBytesToSkip;
	unsigned char fCurPacketNALUnitType;	
	BOOL fCurrentPacketBeginsFrame = FALSE;
	BOOL fCurrentPacketCompletesFrame = FALSE;

	// Check the 'nal_unit_type' for special 'aggregation' or 'fragmentation' packets:
	if (packetSize < 1) return FALSE;
	fCurPacketNALUnitType = (headerStart[0]&0x1F);
		
	switch (fCurPacketNALUnitType) 
	{
	case 24:  // STAP-A
		numBytesToSkip = 1; // discard the type byte
		break;
	case 25: case 26: case 27:  // STAP-B, MTAP16, or MTAP24
		numBytesToSkip = 3; // discard the type byte, and the initial DON
		break;
	case 28: case 29: 
	{ 	// // FU-A or FU-B
	    // For these NALUs, the first two bytes are the FU indicator and the FU header.
	    // If the start bit is set, we reconstruct the original NAL header into byte 1:
	    if (packetSize < 2) return FALSE;
	    unsigned char startBit = headerStart[1]&0x80;
	    unsigned char endBit = headerStart[1]&0x40;
	    if (startBit) 
	    {
			fCurrentPacketBeginsFrame = TRUE;

			headerStart[1] = (headerStart[0]&0xE0)|(headerStart[1]&0x1F);
			numBytesToSkip = 1;
	    } 
	    else 
	    {
			// The start bit is not set, so we skip both the FU indicator and header:
			fCurrentPacketBeginsFrame = FALSE;
			numBytesToSkip = 2;
	    }
		fCurrentPacketCompletesFrame = (endBit != 0);
		break;
	}
	default: 
		// This packet contains one complete NAL unit:
		fCurrentPacketBeginsFrame = fCurrentPacketCompletesFrame = TRUE;
		numBytesToSkip = 0;
		break;
	}

	if (fCurrentPacketBeginsFrame)
	{
		m_h264_offset = 0;
	}

  	if ((m_h264_offset+4+packetSize-numBytesToSkip) >= sizeof(m_fu_buf))
	{
		log_print(LOG_ERR, "H264 fragment Reassembly Packet Too Big %d!!!",m_h264_offset+4+packetSize-numBytesToSkip);
		return FALSE;
	}
	
	memcpy(m_fu_buf+m_h264_offset+4, headerStart+numBytesToSkip, packetSize-numBytesToSkip);
	m_h264_offset += packetSize-numBytesToSkip;

	if (fCurrentPacketCompletesFrame)
	{
		m_fu_buf[0] = 0;
		m_fu_buf[1] = 0;
		m_fu_buf[2] = 0;
		m_fu_buf[3] = 1;

		sys_os_mutex_enter(m_pMutex);
		if (m_pVideoCB)
		{
			m_pVideoCB(m_fu_buf, m_h264_offset+4, ts, seq, m_pUserdata);
		}
		sys_os_mutex_leave(m_pMutex);

		m_h264_offset = 0;
	} 

	return TRUE;
}

void CRtsp::computeAbsDonFromDON(uint64 DON) 
{
	if (!m_bExpectDONFields) 
	{
		// Without DON fields in the input stream, we just increment our "AbsDon" count each time:
		++m_nCurrentNALUnitAbsDon;
	}
	else 
	{
		if (m_nCurrentNALUnitAbsDon == (uint64)(~0))
		{
			// This is the very first NAL unit, so "AbsDon" is just "DON":
			m_nCurrentNALUnitAbsDon = (uint64)DON;
		} 
		else
		{
			// Use the previous NAL unit's DON and the current DON to compute "AbsDon":
			//     AbsDon[n] = AbsDon[n-1] + (DON[n] - DON[n-1]) mod 2^16
			short signedDiff16 = (short)(DON - m_nPreviousNALUnitDON);
			int64 signedDiff64 = (int64)signedDiff16;
			m_nCurrentNALUnitAbsDon += signedDiff64;
		}

		m_nPreviousNALUnitDON = DON; // for next time
	}
}

BOOL CRtsp::rtp_h265_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit) 
{
	unsigned char* headerStart = lpData;
	unsigned packetSize = rlen;
	unsigned short DONL = 0;
	unsigned numBytesToSkip;
	unsigned char fCurPacketNALUnitType;
	BOOL fCurrentPacketBeginsFrame = FALSE;
	BOOL fCurrentPacketCompletesFrame = FALSE;
	
	// Check the Payload Header's 'nal_unit_type' for special aggregation or fragmentation packets:
	if (packetSize < 2) return FALSE;
	fCurPacketNALUnitType = (headerStart[0]&0x7E)>>1;
		
	switch (fCurPacketNALUnitType) 
	{
	
	case 48: 
	{ 
		// Aggregation Packet (AP)
		// We skip over the 2-byte Payload Header, and the DONL header (if any).
		if (m_bExpectDONFields) 
		{
			if (packetSize < 4) return FALSE;
			DONL = (headerStart[2]<<8)|headerStart[3];
			numBytesToSkip = 4;
		} 
		else
		{
			numBytesToSkip = 2;
		}
		break;
	}
		
	case 49: 
	{ 
		// Fragmentation Unit (FU)
		// This NALU begins with the 2-byte Payload Header, the 1-byte FU header, and (optionally)
		// the 2-byte DONL header.
		// If the start bit is set, we reconstruct the original NAL header at the end of these
		// 3 (or 5) bytes, and skip over the first 1 (or 3) bytes.
		if (packetSize < 3) return FALSE;
		unsigned char startBit = headerStart[2]&0x80; // from the FU header
		unsigned char endBit = headerStart[2]&0x40; // from the FU header
		if (startBit) 
		{
			fCurrentPacketBeginsFrame = TRUE;

			unsigned char nal_unit_type = headerStart[2]&0x3F; // the last 6 bits of the FU header
			unsigned char newNALHeader[2];
			newNALHeader[0] = (headerStart[0]&0x81)|(nal_unit_type<<1);
			newNALHeader[1] = headerStart[1];

			if (m_bExpectDONFields) 
			{
				if (packetSize < 5) return FALSE;
				DONL = (headerStart[3]<<8)|headerStart[4];
				headerStart[3] = newNALHeader[0];
				headerStart[4] = newNALHeader[1];
				numBytesToSkip = 3;
			}
			else 
			{
				headerStart[1] = newNALHeader[0];
				headerStart[2] = newNALHeader[1];
				numBytesToSkip = 1;
			}
		} 
		else 
		{
			// The start bit is not set, so we skip over all headers:
			fCurrentPacketBeginsFrame = FALSE;
			if (m_bExpectDONFields) 
			{
				if (packetSize < 5) return FALSE;
				DONL = (headerStart[3]<<8)|headerStart[4];
				numBytesToSkip = 5;
			}
			else 
			{
				numBytesToSkip = 3;
			}
		}
		
		fCurrentPacketCompletesFrame = (endBit != 0);
		break;
	}
	
	default: 
	{
		// This packet contains one complete NAL unit:
		fCurrentPacketBeginsFrame = fCurrentPacketCompletesFrame = TRUE;
		numBytesToSkip = 0;
		break;
	}
	
	}

	computeAbsDonFromDON(DONL);

	if (fCurrentPacketBeginsFrame)
	{
		m_h265_offset = 0;
	}

  	if ((m_h265_offset+4+packetSize-numBytesToSkip) >= sizeof(m_fu_buf))
	{
		log_print(LOG_ERR, "H265 fragment Reassembly Packet Too Big %d!!!",m_h265_offset+4+packetSize-numBytesToSkip);
		return FALSE;
	}
	
	memcpy(m_fu_buf+m_h265_offset+4, headerStart+numBytesToSkip, packetSize-numBytesToSkip);
	m_h265_offset += packetSize-numBytesToSkip;

	if (fCurrentPacketCompletesFrame)
	{
		m_fu_buf[0] = 0;
		m_fu_buf[1] = 0;
		m_fu_buf[2] = 0;
		m_fu_buf[3] = 1;

		sys_os_mutex_enter(m_pMutex);
		if (m_pVideoCB)
		{
			m_pVideoCB(m_fu_buf, m_h265_offset+4, ts, seq, m_pUserdata);
		}
		sys_os_mutex_leave(m_pMutex);

		m_h265_offset = 0;
	} 
	
	return TRUE;
}

BOOL CRtsp::rtp_mpeg4_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit) 
{
	BOOL fCurrentPacketBeginsFrame = FALSE;
	BOOL fCurrentPacketCompletesFrame = FALSE;
	
	// The packet begins a frame iff its data begins with a system code 
	// (i.e., 0x000001??)  
	fCurrentPacketBeginsFrame = (rlen >= 4 && lpData[0] == 0 && lpData[1] == 0 && lpData[2] == 1); 

	if (fCurrentPacketBeginsFrame)
	{
		m_mpeg4_offset = 0;
	}

  	if ((m_mpeg4_offset+rlen+m_mpeg4_header_len) >= sizeof(m_fu_buf))
	{
		log_print(LOG_ERR, "MPEG4 fragment Reassembly Packet Too Big %d!!!",m_mpeg4_offset+rlen+m_mpeg4_header_len);
		return FALSE;
	}

	memcpy(m_fu_buf+m_mpeg4_offset+m_mpeg4_header_len, lpData, rlen);
	m_mpeg4_offset += rlen;
	
	if (markbit)
	{
		sys_os_mutex_enter(m_pMutex);
		if (m_pVideoCB)
		{
			m_pVideoCB(m_fu_buf, m_mpeg4_offset+m_mpeg4_header_len, ts, seq, m_pUserdata);
		}
		sys_os_mutex_leave(m_pMutex);

		m_mpeg4_offset = 0;
	}

	return TRUE;
}

void CRtsp::video_rtp_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit)
{
	if (VideoCodec_H264 == m_VideoCodec)
	{
		rtp_h264_rx(lpData, rlen, seq, ts, markbit);	
	}
	else if (VideoCodec_JPEG == m_VideoCodec)
	{
		rtp_mjpeg_rx(lpData, rlen, seq, ts, markbit);    
	}
	else if (VideoCodec_MPEG4 == m_VideoCodec)
	{
		rtp_mpeg4_rx(lpData, rlen, seq, ts, markbit);
	}
	else if (VideoCodec_H265 == m_VideoCodec)
	{
		rtp_h265_rx(lpData, rlen, seq, ts, markbit);
	}
}

BOOL CRtsp::rtp_aac_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit)
{
	////////// AUHeader //////////
	struct AUHeader {
	  unsigned size;
	  unsigned index; // indexDelta for the 2nd & subsequent headers
	};

	unsigned char* headerStart = lpData;
	unsigned packetSize = rlen;
	unsigned resultSpecialHeaderSize = 0;

	unsigned fNumAUHeaders = 0; // in the most recently read packet
  	unsigned fNextAUHeader = 0; // index of the next AU Header to read
  	struct AUHeader* fAUHeaders;

	// default values:

	fAUHeaders = NULL;

	if (m_nSizeLength == 0) 
	{
		return FALSE;
	}
	
	// The packet begins with a "AU Header Section".  Parse it, to
	// determine the "AU-header"s for each frame present in this packet:
	resultSpecialHeaderSize += 2;
	if (packetSize < resultSpecialHeaderSize)
	{
		return FALSE;
	}

	unsigned AU_headers_length = (headerStart[0]<<8)|headerStart[1];
	unsigned AU_headers_length_bytes = (AU_headers_length+7)/8;
	if (packetSize < resultSpecialHeaderSize + AU_headers_length_bytes)
	{
		return FALSE;
	}
	resultSpecialHeaderSize += AU_headers_length_bytes;

	// Figure out how many AU-headers are present in the packet:
	int bitsAvail = AU_headers_length - (m_nSizeLength + m_nIndexLength);
	if (bitsAvail >= 0 && (m_nSizeLength + m_nIndexDeltaLength) > 0)
	{
		fNumAUHeaders = 1 + bitsAvail/(m_nSizeLength + m_nIndexDeltaLength);
	}
	
	if (fNumAUHeaders > 0) 
	{
		fAUHeaders = new AUHeader[fNumAUHeaders];
		// Fill in each header:
		BitVector bv(&headerStart[2], 0, AU_headers_length);
		fAUHeaders[0].size = bv.getBits(m_nSizeLength);
		fAUHeaders[0].index = bv.getBits(m_nIndexLength);

		for (unsigned i = 1; i < fNumAUHeaders; ++i) 
		{
			fAUHeaders[i].size = bv.getBits(m_nSizeLength);
			fAUHeaders[i].index = bv.getBits(m_nIndexDeltaLength);
		}
	}

	lpData += resultSpecialHeaderSize;
	rlen -= resultSpecialHeaderSize;
	
	if (fNumAUHeaders == 1 && rlen < fAUHeaders[0].size) 
	{
		if (fAUHeaders[0].size > 8192) 
		{                
            return FALSE;
        }

        memcpy(m_fu_audio+m_aac_offset, lpData, rlen);
		m_aac_offset += rlen;

		if (markbit)
		{
			if (m_aac_offset != fAUHeaders[0].size) 
			{
				m_aac_offset = 0;
				return FALSE;
			}

			sys_os_mutex_enter(m_pMutex);

			if (m_pAudioCB)
		    {    	
		        m_pAudioCB(m_fu_audio, m_aac_offset, ts, seq, m_pUserdata);
		    }

		    sys_os_mutex_leave(m_pMutex);
		}
	}
	else
	{
		for (int i = 0; i < fNumAUHeaders; i++)
		{
			if (rlen < fAUHeaders[i].size) 
			{
	    		return FALSE;
			}

			memcpy(m_fu_audio, lpData, fAUHeaders[i].size);
			lpData += fAUHeaders[i].size;
			rlen -= fAUHeaders[i].size;

			sys_os_mutex_enter(m_pMutex);

			if (m_pAudioCB)
		    {    	
		        m_pAudioCB(m_fu_audio, fAUHeaders[i].size, ts, seq, m_pUserdata);
		    }

		    sys_os_mutex_leave(m_pMutex);
		}
	}

	return TRUE;
}

BOOL CRtsp::rtp_g726_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit)
{
	sys_os_mutex_enter(m_pMutex);
	
	if (m_pAudioCB)
    {    	
        m_pAudioCB(lpData, rlen, ts, seq, m_pUserdata);
    }

    sys_os_mutex_leave(m_pMutex);
    
	return TRUE;
}

BOOL CRtsp::rtp_pcm_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit)
{
	sys_os_mutex_enter(m_pMutex);
	
	if (m_pAudioCB)
    {    	
        m_pAudioCB(lpData, rlen, ts, seq, m_pUserdata);
    }

    sys_os_mutex_leave(m_pMutex);

    return TRUE;
}

void CRtsp::audio_rtp_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit)
{
	if (AudioCodec_AAC == m_AudioCodec)
	{
		rtp_aac_rx(lpData, rlen, seq, ts, markbit);
	}
	else if (AudioCodec_G726 == m_AudioCodec)
	{
		rtp_g726_rx(lpData, rlen, seq, ts, markbit);
	}
	else if (AudioCodec_PCMA == m_AudioCodec || AudioCodec_PCMU == m_AudioCodec)
	{
		rtp_pcm_rx(lpData, rlen, seq, ts, markbit);
	}
}

void CRtsp::rtsp_keep_alive()
{
	int ms = sys_os_get_ms();
	if (ms - m_rua.keepalive_time >= (m_rua.session_timeout - 10) * 1000)
	{
		m_rua.keepalive_time = ms;
		
		m_rua.cseq++;
		HRTSP_MSG * tx_msg;

		if (m_rua.gp_cmd) // the rtsp server supports GET_PARAMETER command
		{
			tx_msg = rua_build_get_parameter(&m_rua);
		}
		else
		{
			tx_msg = rua_build_options(&m_rua);
		}
		
		if (tx_msg)
    	{
    		send_free_rtsp_msg(&m_rua, tx_msg);
    	}
	}
}

void CRtsp::rx_thread()
{
	int  ret;
	int  tm_count = 0;
    BOOL nodata_notify = FALSE;
    
	send_notify(RTSP_EVE_CONNECTING);
	
	if (!rtsp_client_start())	
	{
		send_notify(RTSP_EVE_CONNFAIL);
		goto rtsp_rx_exit;
	}
    
	while (m_bRunning)
	{
	    ret = rtsp_tcp_rx();	//获取视频流
	    if (ret == RTSP_RX_FAIL)
	    {
	        break;
	    }
	    else if (ret == RTSP_RX_TIMEOUT)
    	{
    		tm_count++;
	        if (tm_count >= 10 && !nodata_notify)    // without data
	        {
	            nodata_notify = TRUE;
	            send_notify(RTSP_EVE_NODATA);
	        }
        }
        else // should be RTSP_RX_SUCC
        {
        	if (nodata_notify)
	        {
	            nodata_notify = FALSE;
	            send_notify(RTSP_EVE_RESUME);
	        }
	        
        	tm_count = 0;
        }

	    if (m_rua.state == RCS_PLAYING)
	    {	    	
	    	rtsp_keep_alive();
	    }
	}

    if (m_rua.fd > 0)
	{
		closesocket(m_rua.fd);
		m_rua.fd = 0;
	}

    if (m_rua.rtp_rcv_buf)
    {
        free(m_rua.rtp_rcv_buf);
        m_rua.rtp_rcv_buf = NULL;
    }
    
	send_notify(RTSP_EVE_STOPPED);

rtsp_rx_exit:

	m_rtspRxTid = 0;
	log_print(LOG_DBG, "rtsp_rx_thread exit\r\n");
}


void CRtsp::send_notify(int event)
{
	sys_os_mutex_enter(m_pMutex);
	
	if (m_pNotify)
	{
		m_pNotify(event, m_pUserdata);
	}

	sys_os_mutex_leave(m_pMutex);
}

void CRtsp::get_sps_pps_para()
{
	rtsp_send_sps_pps_para(&m_rua);
}

void CRtsp::get_sps_pps_para(uint8 * p_sps, int * sps_len, uint8 * p_pps, int * pps_len)
{
	int pt;
	char sps[1000], pps[1000] = {'\0'};
	
	if (!get_sdp_h264_desc(&m_rua, &pt, sps, sizeof(sps)))
	{
		return;
	}

	char * ptr = strchr(sps, ',');
	if (ptr && ptr[1] != '\0')
	{
		*ptr = '\0';
		strcpy(pps, ptr+1);
	}

	unsigned char sps_pps[1000];
	sps_pps[0] = 0x0;
	sps_pps[1] = 0x0;
	sps_pps[2] = 0x0;
	sps_pps[3] = 0x1;
	
	int len = base64_decode(sps, sps_pps+4, sizeof(sps_pps)-4);
	if (len <= 0)
	{
		return;
	}

	memcpy(p_sps, sps_pps, len+4);
	*sps_len = len+4;
	
	if (pps[0] != '\0')
	{		
		len = base64_decode(pps, sps_pps+4, sizeof(sps_pps)-4);
		if (len > 0)
		{
			memcpy(p_pps, sps_pps, len+4);
			*pps_len = len+4;
		}
	}
}

void CRtsp::get_vps_sps_pps_para()
{
	rtsp_send_vps_sps_pps_para(&m_rua);
}

BOOL CRtsp::get_rtsp_video_media_info()
{
	if (m_rua.remote_video_cap_count == 0)
	{
		return FALSE;
	}

	if (m_rua.remote_video_cap[0] == 26)
	{
		m_VideoCodec = VideoCodec_JPEG;
	}

	int i;
	int rtpmap_len = strlen("a=rtpmap:");

	for(i=0; i<MAX_AVN; i++)
	{
		char * ptr = m_rua.remote_video_cap_desc[i];
		if(memcmp(ptr, "a=rtpmap:", rtpmap_len) == 0)
		{
			char pt_buf[16];
			char code_buf[64];
			int next_offset = 0;
			ptr += rtpmap_len;

			if(GetLineWord(ptr, 0, strlen(ptr), pt_buf, sizeof(pt_buf), &next_offset, WORD_TYPE_NUM) == FALSE)
				return FALSE;
			
			GetLineWord(ptr, next_offset, strlen(ptr)-next_offset, code_buf, sizeof(code_buf),  &next_offset, WORD_TYPE_STRING);

			if(stricmp(code_buf, "H264/90000") == 0)
			{
				m_VideoCodec = VideoCodec_H264;
			}
			else if(stricmp(code_buf, "JPEG/90000") == 0)
			{
				m_VideoCodec = VideoCodec_JPEG;
			}
			else if(stricmp(code_buf, "MP4V-ES/90000") == 0)
			{
				m_VideoCodec = VideoCodec_MPEG4;
			}
			else if(stricmp(code_buf, "H265/90000") == 0)
			{
				m_VideoCodec = VideoCodec_H265;
			}

			break;
		}
	}

	return TRUE;
}

BOOL CRtsp::get_rtsp_audio_media_info()
{
	if (m_rua.remote_audio_cap_count == 0)
	{
		return FALSE;
	}

	if (m_rua.remote_audio_cap[0] == 0)
	{
		m_AudioCodec = AudioCodec_PCMU;
	}
	else if (m_rua.remote_audio_cap[0] == 8)
	{
		m_AudioCodec = AudioCodec_PCMA;
	}
	else if (m_rua.remote_audio_cap[0] == 2)
	{
		m_AudioCodec = AudioCodec_G726;
	}

	int i;
	int rtpmap_len = strlen("a=rtpmap:");

	for(i=0; i<MAX_AVN; i++)
	{
		char * ptr = m_rua.remote_audio_cap_desc[i];
		if(memcmp(ptr, "a=rtpmap:", rtpmap_len) == 0)
		{
			char pt_buf[16];
			char code_buf[64];
			int next_offset = 0;
			ptr += rtpmap_len;

			if(GetLineWord(ptr, 0, strlen(ptr), pt_buf, sizeof(pt_buf), &next_offset, WORD_TYPE_NUM) == FALSE)
				return FALSE;
			
			GetLineWord(ptr, next_offset, strlen(ptr)-next_offset, code_buf, sizeof(code_buf),  &next_offset, WORD_TYPE_STRING);

			uppercase(code_buf);

			if (strstr(code_buf, "G726-32"))
			{
				m_AudioCodec = AudioCodec_G726;
			}
			else if (strstr(code_buf, "PCMU"))
			{
				m_AudioCodec = AudioCodec_PCMU;
			}
			else if (strstr(code_buf, "PCMA"))
			{
				m_AudioCodec = AudioCodec_PCMA;
			}
			else if (strstr(code_buf, "MPEG4-GENERIC"))
			{
				m_AudioCodec = AudioCodec_AAC;
			}

			char * p = strchr(code_buf, '/');
			if (p)
			{
				p++;
				
				char * p1 = strchr(p, '/');
				if (p1)
				{
					*p1 = '\0';
					m_nSamplerate = atoi(p);

					p1++;
					if (p1 != '\0')
					{
						m_nChannels = atoi(p1);
					}
					else
					{
						m_nChannels = 1;
					}
				}
				else
				{
					m_nSamplerate = atoi(p);
					m_nChannels = 1;
				}
			}

			break;
		}
	}

	return TRUE;
}

void CRtsp::set_notify_cb(notify_cb notify, void * userdata) 
{
	sys_os_mutex_enter(m_pMutex);	
	m_pNotify = notify;
	m_pUserdata = userdata;
	sys_os_mutex_leave(m_pMutex);
}

void CRtsp::set_video_cb(video_cb cb) 
{
	sys_os_mutex_enter(m_pMutex);
	m_pVideoCB = cb;		//获取视频流的回调函数
	sys_os_mutex_leave(m_pMutex);
}

void CRtsp::set_audio_cb(audio_cb cb)
{
	sys_os_mutex_enter(m_pMutex);
	m_pAudioCB = cb;
	sys_os_mutex_leave(m_pMutex);
}





