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

#ifndef _RTSP_H_
#define _RTSP_H_

#include "sys_buf.h"
#include "rua.h"
#include "codec.h"


typedef int (*notify_cb)(int, void *);
typedef int (*video_cb)(LPBYTE, int, unsigned int, unsigned short, void *);
typedef int (*audio_cb)(LPBYTE, int, unsigned int, unsigned short, void *);

#define RTSP_EVE_STOPPED    0
#define RTSP_EVE_CONNECTING 1
#define RTSP_EVE_CONNFAIL   2
#define RTSP_EVE_CONNSUCC   3
#define RTSP_EVE_NOSIGNAL   4
#define RTSP_EVE_RESUME     5
#define RTSP_EVE_AUTHFAILED 6
#define RTSP_EVE_NODATA   	7

#define RTSP_RX_FAIL        -1
#define RTSP_RX_TIMEOUT     1
#define RTSP_RX_SUCC        2

#define RTSP_PARSE_FAIL		-1
#define RTSP_PARSE_MOREDATE 0
#define RTSP_PARSE_SUCC		1



class CRtsp
{
public:
	CRtsp(void);
	~CRtsp(void);

public:
	BOOL    rtsp_start(char* url, char* ip, int port, const char * user, const char * pass);
	BOOL    rtsp_play();
	BOOL    rtsp_stop();
	BOOL    rtsp_pause();
	BOOL    rtsp_close();

	RUA *   get_rua() {return &m_rua;}
	char *  get_url() {return m_url;}
	char *  get_ip() {return  m_ip;}
	int     get_port() {return m_nport;}
	void    set_notify_cb(notify_cb notify, void * userdata);
	void    set_video_cb(video_cb cb);
	void    set_audio_cb(audio_cb cb);
	void 	get_sps_pps_para();
	void    get_sps_pps_para(uint8 * p_sps, int * sps_len, uint8 * p_pps, int * pps_len);
	void 	get_vps_sps_pps_para();

	AudioCodec audio_codec() {return m_AudioCodec;}
	VideoCodec video_codec() {return m_VideoCodec;}
	
	int     get_audio_samplerate() {return m_nSamplerate;}
	int     get_audio_channels() {return m_nChannels;}
	uint8 * get_audio_config() {return m_pAudioConfig;}
	int     get_audio_config_len() {return m_nAudioConfigLen;}
	
    void    rx_thread();		

private:
    int     rtsp_pkt_find_end(char * p_buf);
    BOOL    rtsp_client_start();
	BOOL    rua_init_connect(RUA * p_rua);
	void    rtsp_client_stop(RUA * p_rua);
	int     rtsp_client_state(RUA * p_rua, HRTSP_MSG * rx_msg);	
    int     rtsp_tcp_rx();
    int     rtsp_msg_parser(RUA * p_rua);
    void    rtsp_keep_alive();
    
	void    audio_rtp_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit);
	void    video_rtp_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit);
    void    send_notify(int event);
    
    BOOL 	get_rtsp_video_media_info();
    BOOL 	get_rtsp_audio_media_info();
    
	void 	rtsp_data_rx(LPBYTE lpData, int rlen);
	BOOL 	rtp_mjpeg_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit);
    BOOL 	rtp_h264_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit);
    BOOL 	rtp_mpeg4_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit);
	BOOL    rtp_h265_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit);

    BOOL    rtp_aac_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit);
	BOOL    rtp_g726_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit);
	BOOL    rtp_pcm_rx(LPBYTE lpData, int rlen, unsigned seq, unsigned ts, BOOL markbit);
		
	void 	rtsp_send_sps_pps_para(RUA * p_rua);
    void	rtsp_get_mpeg4_config(RUA * p_rua);
	void    rtsp_send_vps_sps_pps_para(RUA * p_rua);
	void    rtsp_get_aac_config(RUA * p_rua);
	
    void    computeAbsDonFromDON(uint64 DON);
    
private:
	RUA		        m_rua;
	char            m_url[256];
	char            m_ip[32];
	int             m_nport;
	notify_cb       m_pNotify;
	void *          m_pUserdata;
	video_cb        m_pVideoCB;
	audio_cb        m_pAudioCB;
	void *			m_pMutex;

	unsigned char   m_fu_buf[1024*1024*2];
	unsigned int	m_h264_offset;
	unsigned int	m_h265_offset;
	unsigned int	m_jpeg_offset;
	unsigned int	m_mpeg4_offset;
    uint32          m_mpeg4_header_len;

    // for h265 rx
    BOOL            m_bExpectDONFields;
    uint64	        m_nPreviousNALUnitDON;
    uint64	        m_nCurrentNALUnitAbsDon;

    unsigned char   m_fu_audio[8192];
    uint8 *         m_pAudioConfig;
	unsigned int    m_nAudioConfigLen;
	unsigned int	m_aac_offset;
	
    // for aac rx
    int             m_nSizeLength;
	int             m_nIndexLength;
	int             m_nIndexDeltaLength;
	
	BOOL            m_bRunning;
	pthread_t       m_rtspRxTid;

	VideoCodec		m_VideoCodec;
	
	AudioCodec		m_AudioCodec;
	int             m_nSamplerate;
	int             m_nChannels;
};



#endif	// _RTSP_H_



