#include "sys_inc.h"
#include "rtsp.h"
#include "HTTPClient.h"

#include "dmtype.h"
#include "dmsdk.h"

#define HTTP_RECV_BUF_SIZE (8*1024+1)
#define RAW_FRAME_BUF_SIZE (2*1024*1024)
#define RAW_TEPM_PARAM_SIZE (80)
#define RAW_FRAME_HEADER_SIZE (24)
#define DLV_FILE_HEADER_SIZE (16)
#define RAW_DLV_PARAGRAPH_SIZE (20)
#define RAW_DLV_TTH_SIZE (428)
#define RAW_PARAM_FRAME_LEN (RAW_FRAME_HEADER_SIZE+RAW_DLV_PARAGRAPH_SIZE+RAW_DLV_TTH_SIZE+4+RAW_TEPM_PARAM_SIZE)
#define RAW_FISRT_PARAM_FRAME_LEN (RAW_PARAM_FRAME_LEN+DLV_FILE_HEADER_SIZE)


int rtsp_session_create(long *hdl)
{
	CHECK_RET(hdl != NULL, ERR_INVALID_PARAM, "rtsp_session, input is NULL");

	CRtsp *p_rtsp = new CRtsp;
	CHECK_RET(p_rtsp != NULL, -1, "rtsp_session, new failed");

	*hdl = (long)p_rtsp;

	return DM_SUCCESS;
}



int rtsp_session_destroy(long hdl)
{
	CRtsp *p_rtsp = (CRtsp *)hdl;
	CHECK_RET(p_rtsp != NULL, -1, "rtsp_session_destroy, input is NULL");

	delete p_rtsp;

	return DM_SUCCESS;
}



/*
**/
int DM_Sys_Init(void)
{
	int ret;

	ret = sys_buf_init();
	CHECK_RET(ret == TRUE, -1, "DM_Stream_Init, failed");

	ret = rtsp_parse_buf_init();
	CHECK_RET(ret == TRUE, -1, "rtsp_parse_buf_init, failed");

	return DM_SUCCESS;
}



/*
**/
int DM_Sys_UnInit(void)
{
	rtsp_parse_buf_deinit();
	sys_buf_deinit();

	return DM_SUCCESS;
}



/* run in pthread
**/
int DM_Video_Stream_Start(long hdl, int port)
{
	int ret;
	long hdl_rtsp;
	char filename[] = "ONVIFMedia";
	char ip[16];
	const char * username = NULL;
	const char * password = NULL;
	CRtsp *p_rtsp;

	ret = dm_rtsp_config_get(hdl, ip, &hdl_rtsp);
	CHECK_RET(ret == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");

	p_rtsp = (CRtsp *)hdl_rtsp;

	ret = p_rtsp->rtsp_start(filename, ip, port, username, password);
	CHECK_RET(ret != FALSE, ERR_SOCKET_CONNECT, "rtsp_start failed");

	p_rtsp->rtsp_play();

	return DM_SUCCESS;
}



int DM_Video_Stream_Stop(long hdl)
{
	int ret;
	long hdl_rtsp;
	CRtsp *p_rtsp;

	ret = dm_rtsp_config_get(hdl, NULL, &hdl_rtsp);
	CHECK_RET(ret == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");

	p_rtsp = (CRtsp *)hdl_rtsp;

	p_rtsp->rtsp_stop();
	p_rtsp->rtsp_close();

	return DM_SUCCESS;
}



int DM_Video_Stream_Register(long hdl, VIDEO_NOTIFY_CB notify_cb, VIDEO_STREAM_CB video_cb)
{
	int ret;
	long hdl_rtsp;
	CRtsp *p_rtsp;

	ret = dm_rtsp_config_get(hdl, NULL, &hdl_rtsp);
	CHECK_RET(ret == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");

	p_rtsp = (CRtsp *)hdl_rtsp;
	p_rtsp->set_notify_cb(notify_cb, (void*)hdl);
	p_rtsp->set_video_cb(video_cb);

	return DM_SUCCESS;
}



typedef struct
{
	HTTP_SESSION_HANDLE pHTTP;
	INFRARED_NOTIFY_CB notify_callback;
	INFRARED_DATA_CB data_callback;
	int is_infrared_thread_running;
	pthread_t infrared_thread_id;
	unsigned char temp_param[RAW_TEPM_PARAM_SIZE];
	void *arg;
}RAW_DATA_SESSION;


int rawdata_session_create(long *hdl)
{
	CHECK_RET(hdl != NULL, ERR_INVALID_PARAM, "rawdata_session_create, input is NULL");

	RAW_DATA_SESSION *session = (RAW_DATA_SESSION *)malloc(sizeof(RAW_DATA_SESSION));
	CHECK_RET(session != NULL, -1, "rawdata_session_create, malloc failed");

	session->data_callback = NULL;
	session->notify_callback = NULL;
	session->is_infrared_thread_running = FALSE;
	session->infrared_thread_id = 0;
	memset((void *)session->temp_param, 0, RAW_TEPM_PARAM_SIZE);

	*hdl = (long)session;

	return DM_SUCCESS;
}



int rawdata_session_destroy(long hdl)
{
	RAW_DATA_SESSION *session = (RAW_DATA_SESSION *)hdl;
	CHECK_RET(session != NULL, -1, "rawdata_session_destroy, input is NULL");

	free(session);

	return DM_SUCCESS;
}

void * infrared_rx_thread(void * argv)
{
	long hdl;
    INT32 nRetCode;
    HTTP_SESSION_HANDLE pHTTP;
	INFRARED_NOTIFY_CB notify_callback;
	INFRARED_DATA_CB data_callback;
	unsigned char *temp_param_buf;
	long raw_session;
	RAW_DATA_SESSION *p_raw_session;
	void *arg;

	unsigned char http_buf_rx[HTTP_RECV_BUF_SIZE];
	memset(http_buf_rx, 0x00, sizeof(http_buf_rx));

	hdl = (INT32)argv;
	int ret = dm_rawdata_config_get(hdl, &raw_session);
	p_raw_session = (RAW_DATA_SESSION *)raw_session;
	if( ret!=DM_SUCCESS ){
		printf("[Func]:%s [Line]:%d	invalid handle\r\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	pHTTP = p_raw_session->pHTTP;
	notify_callback = p_raw_session->notify_callback;
	data_callback = p_raw_session->data_callback;
	temp_param_buf = p_raw_session->temp_param;
	arg = p_raw_session->arg;

	UINT32 size, total = 0;
	int i;
	unsigned char frame_buf[RAW_FRAME_BUF_SIZE];
	unsigned char output_buf[RAW_FRAME_BUF_SIZE];
	int head_pos = 0;
	int tail_pos = -1;
	int expect_len = 0;
	int RAW_W = 640;
	int RAW_H = 480;
	int RAW_SIZE = RAW_W*RAW_H*2;
	while (p_raw_session->is_infrared_thread_running)
	{
		//recv a piece of data
		nRetCode = HTTPClientReadData(pHTTP, (void *)http_buf_rx, HTTP_RECV_BUF_SIZE, 0, &size);

		if(nRetCode != HTTP_CLIENT_SUCCESS){
			printf("%s, recv failed ret=%d\r\n", __FUNCTION__, nRetCode);
			notify_callback(hdl, STREAM_EVE_CONNERR, arg);
			break;
		}

		if (size==0)
		{
			continue;
		}
		total += size;
		//printf("func:%s, line:%d, ret=%ld, size=%ld, total=%ld\r\n", __FUNCTION__, __LINE__, nRetCode, size, total);

		//copy this piece of data to frame buffer
		int data_len = (tail_pos<0)?0 : (RAW_FRAME_BUF_SIZE+tail_pos-head_pos+1-1)%RAW_FRAME_BUF_SIZE+1;
		int total_left = RAW_FRAME_BUF_SIZE-data_len;
		int tail_left = RAW_FRAME_BUF_SIZE-1-tail_pos;

		if(tail_left>=size) //has enough space in tail
		{
			memcpy((void *)(frame_buf+tail_pos+1), (void *)http_buf_rx, size);
			tail_pos += size;
		}
		else //write part of data in tail,then write left data in head
		{
			memcpy((void *)(frame_buf+tail_pos+1), (void *)http_buf_rx, tail_left);
			memcpy((void *)(frame_buf), (void *)http_buf_rx+tail_left, size-tail_left);
			tail_pos = (tail_pos+size)%RAW_FRAME_BUF_SIZE;
		}

		if (total_left<size) //has to overwrite
		{
			head_pos = (tail_pos+1)%RAW_FRAME_BUF_SIZE;
		}

		data_len = (RAW_FRAME_BUF_SIZE+tail_pos-head_pos+1-1)%RAW_FRAME_BUF_SIZE+1;
		if (data_len<=RAW_FRAME_HEADER_SIZE)
		{
			continue;
		}

		//search magic and calculate expect data length
		if (expect_len==0) //need to search magic
		{
			//search magic
			int find_magic = 0;
			for(i=0;i<data_len-4;i++)
			{
				if(   frame_buf[(head_pos+i)%RAW_FRAME_BUF_SIZE]==0x55
					&&frame_buf[(head_pos+i+1)%RAW_FRAME_BUF_SIZE]==0xAA
					&&frame_buf[(head_pos+i+2)%RAW_FRAME_BUF_SIZE]==0xAA
					&&frame_buf[(head_pos+i+3)%RAW_FRAME_BUF_SIZE]==0x55)
				{
					//printf("--> %d %d\n", head_pos, i);
					head_pos = (head_pos+i)%RAW_FRAME_BUF_SIZE;
					find_magic = 1;
					break;
				}
			}
			if (find_magic==0)
			{
				continue;
			}

			data_len = (RAW_FRAME_BUF_SIZE+tail_pos-head_pos+1-1)%RAW_FRAME_BUF_SIZE+1;
			if (data_len<=RAW_FRAME_HEADER_SIZE)
			{
				continue;
			}

			//get frame width and height
			RAW_W = frame_buf[(head_pos+4)%RAW_FRAME_BUF_SIZE];
			RAW_W = RAW_W*256+frame_buf[(head_pos+4+1)%RAW_FRAME_BUF_SIZE];
			RAW_H = frame_buf[(head_pos+4+2)%RAW_FRAME_BUF_SIZE];
			RAW_H = RAW_H*256+frame_buf[(head_pos+4+3)%RAW_FRAME_BUF_SIZE];
			RAW_SIZE = RAW_W*RAW_H*2;

			//calculate expect data length
			expect_len = frame_buf[(head_pos+RAW_FRAME_HEADER_SIZE-4)%RAW_FRAME_BUF_SIZE];
			expect_len = expect_len*256+frame_buf[(head_pos+RAW_FRAME_HEADER_SIZE-3)%RAW_FRAME_BUF_SIZE];
			expect_len = expect_len*256+frame_buf[(head_pos+RAW_FRAME_HEADER_SIZE-2)%RAW_FRAME_BUF_SIZE];
			expect_len = expect_len*256+frame_buf[(head_pos+RAW_FRAME_HEADER_SIZE-1)%RAW_FRAME_BUF_SIZE];

			head_pos = (head_pos+RAW_FRAME_HEADER_SIZE)%RAW_FRAME_BUF_SIZE;

		}

		//output data frame or param frame
		data_len = (RAW_FRAME_BUF_SIZE+tail_pos-head_pos+1-1)%RAW_FRAME_BUF_SIZE+1;
		if (data_len<expect_len)
		{
			continue;
		}
		else
		{
			int output_head_pos;
			int tail_size;
			int head_size;
			//output frame
			if (expect_len==(RAW_PARAM_FRAME_LEN-RAW_FRAME_HEADER_SIZE)
				||expect_len==(RAW_FISRT_PARAM_FRAME_LEN-RAW_FRAME_HEADER_SIZE)) //param
			{
				output_head_pos = (head_pos+expect_len-RAW_TEPM_PARAM_SIZE)%RAW_FRAME_BUF_SIZE;
				if( (RAW_FRAME_BUF_SIZE-output_head_pos)>=RAW_TEPM_PARAM_SIZE ) //directly
				{
					memcpy((void *)temp_param_buf, (void *)(frame_buf+output_head_pos), RAW_TEPM_PARAM_SIZE);
				}
				else //tail,then head
				{
					int tail_size = RAW_FRAME_BUF_SIZE-output_head_pos;
					int head_size = RAW_TEPM_PARAM_SIZE-tail_size;
					memcpy((void *)temp_param_buf, (void *)(frame_buf+output_head_pos), tail_size);
					memcpy((void *)temp_param_buf+tail_size, (void *)(frame_buf), head_size);
				}
			}
			else if (expect_len==(RAW_SIZE+RAW_DLV_PARAGRAPH_SIZE)) //data
			{
				output_head_pos = (head_pos+expect_len-RAW_SIZE)%RAW_FRAME_BUF_SIZE;
				if( (RAW_FRAME_BUF_SIZE-output_head_pos)>=RAW_SIZE ) //directly
				{
					memcpy((void *)output_buf+(RAW_FRAME_BUF_SIZE/2), (void *)(frame_buf+output_head_pos), RAW_SIZE);
				}
				else //tail,then head
				{
					int tail_size = RAW_FRAME_BUF_SIZE-output_head_pos;
					int head_size = RAW_SIZE-tail_size;
					memcpy((void *)output_buf+(RAW_FRAME_BUF_SIZE/2), (void *)(frame_buf+output_head_pos), tail_size);
					memcpy((void *)output_buf+(RAW_FRAME_BUF_SIZE/2)+tail_size, (void *)(frame_buf), head_size);
				}
				if(data_callback)
				{
					for(i=0;i<RAW_W*RAW_H;i++)
					{
						output_buf[2*i]=output_buf[(RAW_FRAME_BUF_SIZE/2)+i];
						output_buf[2*i+1]=output_buf[(RAW_FRAME_BUF_SIZE/2)+RAW_W*RAW_H+i];
					}
					//make first point same as second point
					output_buf[0] = output_buf[2];
					output_buf[1] = output_buf[3];

					data_callback(hdl, output_buf, RAW_SIZE, RAW_W, RAW_H, arg);
				}
			}

			//clear the output frame
			if (data_len==expect_len)
			{
				//printf("-->1 expect_len=%d\n", expect_len);
				head_pos = 0;
				tail_pos = -1;
				expect_len = 0;
			}
			else
			{
				//printf("-->2 expect_len=%d\n", expect_len);
				head_pos = (head_pos+expect_len)%RAW_FRAME_BUF_SIZE;
			}
			expect_len = 0;
		}
	}

	return NULL;

}

void get_gray_temp_params(long hdl, void *pDst)
{
	long raw_session;
	RAW_DATA_SESSION *p_raw_session;
	int ret = dm_rawdata_config_get(hdl, &raw_session);
	p_raw_session = (RAW_DATA_SESSION *)raw_session;
	if( ret!=DM_SUCCESS ){
		printf("[Func]:%s [Line]:%d	invalid handle\r\n", __FUNCTION__, __LINE__);
		return ;
	}
	memcpy(pDst, p_raw_session->temp_param, 80);
}

typedef struct tagDLVTempPoint{
    int s32Gray;
    int s32Temp;
}DM_GRAY_TEMP_ST;

int DM_Gray_To_Temperature(long hdl, int gray, float *pfTemp)
{
    DM_GRAY_TEMP_ST astGrayTemp[10];
    unsigned int u32DiffGray;
    unsigned int u32DiffTemp;
    float f32Rate = 0;

    get_gray_temp_params(hdl, astGrayTemp);

    for (int i=1; i<10; i++) {
        if (gray < astGrayTemp[i].s32Gray) {
            u32DiffGray = astGrayTemp[i].s32Gray - astGrayTemp[i-1].s32Gray;
            u32DiffTemp = astGrayTemp[i].s32Temp - astGrayTemp[i-1].s32Temp;
            f32Rate = 1.0 * u32DiffTemp / u32DiffGray;

            *pfTemp = (astGrayTemp[i].s32Temp - (astGrayTemp[i].s32Gray - gray) * f32Rate)/100;

            return 0;
        }
    }

    u32DiffGray = astGrayTemp[9].s32Gray - astGrayTemp[8].s32Gray;
    u32DiffTemp = astGrayTemp[9].s32Temp - astGrayTemp[8].s32Temp;
    f32Rate = 1.0 * u32DiffTemp / u32DiffGray;

    *pfTemp = ((gray - astGrayTemp[9].s32Gray) * f32Rate + astGrayTemp[9].s32Temp)/100;

    return 0;
}

int DM_Infrared_Stream_Start(long hdl, int port, int framerate)
{
    INT32 nRetCode;
	long raw_session;
	RAW_DATA_SESSION *p_raw_session;
    HTTP_SESSION_HANDLE pHTTP;
	INFRARED_NOTIFY_CB notify_callback;
    char uri[128];
	int ret;

    memset(uri, 0x00, sizeof(uri));
	http_uri_rawdata_fill(hdl, port, (1000/framerate), uri);

	ret = dm_rawdata_config_get(hdl, &raw_session);
	CHECK_RET(ret == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");
	p_raw_session = (RAW_DATA_SESSION *)raw_session;
	notify_callback = p_raw_session->notify_callback;

	notify_callback(hdl, STREAM_EVE_CONNECTING, p_raw_session->arg);

    pHTTP = HTTPClientOpenRequest(HTTP_CLIENT_FLAG_NO_CACHE);
	p_raw_session->pHTTP = pHTTP;

    nRetCode = HTTPClientSetVerb(pHTTP, VerbGet);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        printf("%s, HTTPClientSetVerb failed\r\n", __FUNCTION__);
		notify_callback(hdl, STREAM_EVE_CONNFAIL, p_raw_session->arg);
        return nRetCode;
    }

    nRetCode = HTTPClientSendRequest(pHTTP, uri, NULL, 0, FALSE, 0, 0);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        printf("%s, HTTPClientSendRequest failed, ret=%ld\r\n", __FUNCTION__, nRetCode);
		notify_callback(hdl, STREAM_EVE_CONNFAIL, p_raw_session->arg);
        return nRetCode;
    }

    // Retrieve the the headers and analyze them
    nRetCode = HTTPClientRecvResponse(pHTTP, 3);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        printf("%s, HTTPClientRecvResponse failed\r\n", __FUNCTION__);
		notify_callback(hdl, STREAM_EVE_CONNFAIL, p_raw_session->arg);
        return nRetCode;
    }

	notify_callback(hdl, STREAM_EVE_CONNSUCC, p_raw_session->arg);

    p_raw_session->is_infrared_thread_running = TRUE;

	ret = pthread_create(&(p_raw_session->infrared_thread_id), NULL, (void *(*)(void *))infrared_rx_thread, (void *)hdl);
	if (ret != 0){
        printf("%s, pthread_create failed, ret=%d\r\n", __FUNCTION__, ret);
        return ret;
    }
	//pthread_detach(p_raw_session->infrared_thread_id);

	return DM_SUCCESS;
}

int DM_Infrared_Stream_Stop(long hdl)
{
	long raw_session;
	RAW_DATA_SESSION *p_raw_session;
	INFRARED_NOTIFY_CB notify_callback;

	int ret = dm_rawdata_config_get(hdl, &raw_session);
	CHECK_RET(ret == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");

	p_raw_session = (RAW_DATA_SESSION *)raw_session;
	notify_callback = p_raw_session->notify_callback;

    p_raw_session->is_infrared_thread_running = FALSE;
	pthread_join(p_raw_session->infrared_thread_id, NULL);

	HTTPClientCloseRequest(&(p_raw_session->pHTTP));

	notify_callback(hdl, STREAM_EVE_STOPPED, p_raw_session->arg);

	return DM_SUCCESS;
}



int DM_Infrared_Stream_Register(long hdl, INFRARED_NOTIFY_CB notify_cb, INFRARED_DATA_CB data_cb, void *arg)
{
	long raw_session;
	RAW_DATA_SESSION *p_raw_session;

	int ret = dm_rawdata_config_get(hdl, &raw_session);
	CHECK_RET(ret == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");

	p_raw_session = (RAW_DATA_SESSION *)raw_session;

	p_raw_session->notify_callback = notify_cb;
	p_raw_session->data_callback = data_cb;
	p_raw_session->arg = arg;

	return DM_SUCCESS;
}

