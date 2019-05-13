/*
** system
** time   : 2018-10-23
** author : chenks (godence@126.com)
*/
#include <stdio.h>
#include "HTTPClient.h"

#include "dmtype.h"
#include "dmsdk.h"



#define HTTP_BUFFER_SIZE            256*1024

char g_buf_rx[HTTP_BUFFER_SIZE];









VOID HTTPDebug(const CHAR* FunctionName,const CHAR *DebugDump,UINT32 iLength,CHAR *DebugDescription,...) // Requested operation
{

    va_list            pArgp;
    char               szBuffer[2048];

    memset(szBuffer,0,2048);
    va_start(pArgp, DebugDescription);
    vsprintf((char*)szBuffer, DebugDescription, pArgp); //Copy Data To The Buffer
    va_end(pArgp);

    dm_log(2, "%s %s %s\r\n", FunctionName,DebugDump,szBuffer);
}



static inline int unhex( char c )
{
    return( c >= '0' && c <= '9' ? c - '0'
        : c >= 'A' && c <= 'F' ? c - 'A' + 10
        : c - 'a' + 10 );
}



/***********************************************************
Fuction : decode_string
Description:
    解码由encode_output编码的字符串
Input:
Output:
Return :
************************************************************/
static void decode_string( char *s )
{
/*
 * Remove URL hex escapes from s... done in place.  The basic concept for
 * this routine is borrowed from the WWW library HTUnEscape() routine.
 */
    char    *p;

    for ( p = s; *s != '\0'; ++s ) {
        if ( *s == '%' ) {
            if ( *++s != '\0' ) {
                *p = unhex( *s ) << 4;
            }
            if ( *++s != '\0' ) {
                *p++ += unhex( *s );
            }
        } else if ( *s == '+' ){
            *p++ = ' ';
        } else {
            *p++ = *s;
        }
    }

    *p = '\0';
}



#define MAX_ENCODE_BUFFER_LEN      1024

#define HEX(n) ( (unsigned char)(n) > 9 ? (n) - 10 + 'A': (n) + '0' )


static int encode_string( const unsigned char *string, char *output )
{
    /* take the simple route and encode everything */
    /* could possibly scan once to get length.     */

    unsigned char ch, high, low;
    int  len = 0;

    while ((ch = *string++)) {
        // very simple check for what to encode
        if (isalnum(ch)){
            output[len++] = ch;
        }else{
            output[len++] = '%';
            high = ch >> 4; low = ch & 0x0F;
            output[len++] = HEX(high);
            output[len++] = HEX(low);
        }
    }

    return len;
}



static int http_data_fill(DM_DICTS *dicts, char *data)
{
    int i, ret, len;
    unsigned char buf[MAX_ENCODE_BUFFER_LEN];

    
    memset(buf, 0x00, sizeof(buf));
    
    ret = 0;
    
    for(i=0; i< dicts->count; i++){
        if(i > 0){
            ret += sprintf((char*)buf+ret, "&");
        }
        
        ret += sprintf((char*)buf+ret, "%s=%s", dicts->dict[i].key, dicts->dict[i].value);
    }

    dm_log(4, "%s, data=%s\r\n", __FUNCTION__, buf);

    //DM_Display_Hex("AAA", (char*)buf, ret);
    len = encode_string(buf, data);
    //DM_Display_Hex("BBB", data, ret);

    return len;
}



int http_post(long hdl, char *cmd, char *subcmd, DM_DICTS *dicts)
{
    INT32 nRetCode;
    HTTP_SESSION_HANDLE pHTTP;
    char uri[128];
    int len;
    char buf[MAX_ENCODE_BUFFER_LEN * 3];


    CHECK_RET(handle_valid(hdl) == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");
    
    memset(uri, 0x00, sizeof(uri));
    
    http_uri_fill(hdl, cmd, subcmd, uri);

    len = http_data_fill(dicts, buf);

    pHTTP = HTTPClientOpenRequest(0);
    
#ifdef _HTTP_DEBUGGING_
    HTTPClientSetDebugHook(pHTTP, &HTTPDebug);
#endif

    nRetCode = HTTPClientSetVerb(pHTTP, VerbPost);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientSetVerb failed\r\n", __FUNCTION__);
        return nRetCode;
    }

    nRetCode = HTTPClientAddRequestHeaders(pHTTP, "Cookie", "Name=admin; Pass=Admin123; Type=Admin", 0);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientAddRequestHeaders failed\r\n", __FUNCTION__);
        return nRetCode;
    }
    
    nRetCode = HTTPClientSendRequest(pHTTP, uri, buf, len, TRUE, 0, 0);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientSendRequest failed, ret=%ld\r\n", __FUNCTION__, nRetCode);
        return nRetCode;
    }

    //nRetCode = HTTPClientWriteData(pHTTP, buf, len, 2);
    //CHECK_RET(nRetCode == HTTP_CLIENT_SUCCESS, -1, "HTTPClientWriteData failed");

    // Retrieve the the headers and analyze them
    nRetCode = HTTPClientRecvResponse(pHTTP, 3);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientRecvResponse failed\r\n", __FUNCTION__);
        return nRetCode;
    }


    UINT32 size, total = 0;

    memset(g_buf_rx, 0x00, sizeof(g_buf_rx));
    
    // Get the data until we get an error or end of stream code
    while(nRetCode == HTTP_CLIENT_SUCCESS || nRetCode != HTTP_CLIENT_EOS)
    {
        // Set the size of our buffer
        size = HTTP_BUFFER_SIZE;   
    
        // Get the data
        nRetCode = HTTPClientReadData(pHTTP, g_buf_rx+total, size, 0, &size);
        total += size;
    }

    HTTPClientCloseRequest(&pHTTP);

    decode_string(g_buf_rx);    
    dm_log(4, "total=%d, RSP: %s\r\n", (int)total, g_buf_rx);

    return DM_SUCCESS;
}



int http_get(long hdl, char *cmd, char *subcmd, DM_DICTS *dicts)
{
    INT32 nRetCode;
    HTTP_SESSION_HANDLE pHTTP;
    char uri[128];		//URI是统一资源标识符，每个web服务器都有一个URI标识符

	//检查hdl的有效性
    CHECK_RET(handle_valid(hdl) == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");
    
    memset(uri, 0x00, sizeof(uri));
    
    http_uri_fill(hdl, cmd, subcmd, uri);		//组合地址:赋值给uri

    pHTTP = HTTPClientOpenRequest(0);			//分配内存给http session
    
#ifdef _HTTP_DEBUGGING_
    HTTPClientSetDebugHook(pHTTP, &HTTPDebug);
#endif

    nRetCode = HTTPClientSetVerb(pHTTP, VerbGet);	//设置http向外请求的动词，动词包括:GET,POST,PUT,PATCH,DELETE
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientSetVerb failed\r\n", __FUNCTION__);
        return nRetCode;
    }

	//添加向外请求的头
    nRetCode = HTTPClientAddRequestHeaders(pHTTP, "Cookie", "Name=admin; Pass=Admin123; Type=Admin", 0);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientAddRequestHeaders failed\r\n", __FUNCTION__);
        return nRetCode;
    }
    //发送请求
    nRetCode = HTTPClientSendRequest(pHTTP, uri, NULL, 0, FALSE, 0, 0);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientSendRequest failed\r\n", __FUNCTION__);
        return nRetCode;
    }

    // 检索标题并分析他们
    nRetCode = HTTPClientRecvResponse(pHTTP, 3);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientRecvResponse failed\r\n", __FUNCTION__);
        return nRetCode;
    }


    UINT32 size, total = 0;

    memset(g_buf_rx, 0x00, sizeof(g_buf_rx));
    
    // 获取数据，直到我们得到一个错误或流代码已经结束
    while(nRetCode == HTTP_CLIENT_SUCCESS || nRetCode != HTTP_CLIENT_EOS)
    {
        // Set the size of our buffer
        size = HTTP_BUFFER_SIZE;   
    
        // Get the data
        nRetCode = HTTPClientReadData(pHTTP, g_buf_rx+total, size, 0, &size);
        total += size;
    }

    HTTPClientCloseRequest(&pHTTP);

    decode_string(g_buf_rx);    //解码
    dm_log(4, "%s, total=%d, RSP: %s\r\n", __FUNCTION__, (int)total, g_buf_rx);

    prase_dicts(g_buf_rx, '&', '=', dicts);	//解析字符串
    
    return DM_SUCCESS;
}



int http_get_jpeg(long hdl, int ch, const char *name, char *buf, int *len, int *r_size)
{
    int sec, usec;
    INT32 nRetCode;
    HTTP_SESSION_HANDLE pHTTP;
    char uri[128];


    CHECK_RET(handle_valid(hdl) == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");
    
    memset(uri, 0x00, sizeof(uri));

    http_uri_jpeg_fill(hdl, ch, uri);

    pHTTP = HTTPClientOpenRequest(HTTP_CLIENT_FLAG_NO_CACHE);
    
#ifdef _HTTP_DEBUGGING_
    HTTPClientSetDebugHook(pHTTP, &HTTPDebug);
#endif

    nRetCode = HTTPClientSetVerb(pHTTP, VerbGet);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientSetVerb failed\r\n", __FUNCTION__);
        return nRetCode;
    }
    
    nRetCode = HTTPClientAddRequestHeaders(pHTTP, "Cookie", "Name=admin; Pass=Admin123; Type=Admin", 0);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientAddRequestHeaders failed\r\n", __FUNCTION__);
        return nRetCode;
    }

    dm_dbg_time_start(&sec, &usec);
    
    nRetCode = HTTPClientSendRequest(pHTTP, uri, NULL, 0, FALSE, 0, 0);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientSendRequest failed, ret=%ld\r\n", __FUNCTION__, nRetCode);
        return nRetCode;
    }

    dm_dbg_time_step("SendRequest", &sec, &usec, 1);
    
    // Retrieve the the headers and analyze them
    nRetCode = HTTPClientRecvResponse(pHTTP, 3);
    if(nRetCode != HTTP_CLIENT_SUCCESS){
        dm_log(2, "%s, HTTPClientRecvResponse failed\r\n", __FUNCTION__);
        return nRetCode;
    }

    dm_dbg_time_step("RecvResponse", &sec, &usec, 1);

#if 1
    UINT32 size, total = 0;

    memset(g_buf_rx, 0x00, sizeof(g_buf_rx));
    
    // Get the data until we get an error or end of stream code
    while(nRetCode == HTTP_CLIENT_SUCCESS || nRetCode != HTTP_CLIENT_EOS)
    {
        // Set the size of our buffer
        //size = HTTP_BUFFER_SIZE - total;
        size = MIN(HTTP_BUFFER_SIZE-total, 10240);
    
        // Get the data
        nRetCode = HTTPClientReadData(pHTTP, g_buf_rx+total, size, 0, &size);
        total += size;
        dm_log(4, "func:%s, line:%d, ret=%ld, size=%ld, total=%ld\r\n", __FUNCTION__, __LINE__, nRetCode, size, total);

        if(total >= HTTP_BUFFER_SIZE){
            dm_log(4, "%s, buffer is overflow\r\n", __FUNCTION__);
            break;
        }

        if(nRetCode == HTTP_CLIENT_ERROR_SOCKET_RECV){
            dm_log(4, "%s, recv error\r\n", __FUNCTION__);
            break;
        }
    }
#endif

    dm_dbg_time_step("ReadData", &sec, &usec, 1);
    HTTPClientCloseRequest(&pHTTP);

    if(nRetCode != HTTP_CLIENT_EOS){
        dm_log(4, "%s, recv failed\r\n", __FUNCTION__);
        return nRetCode;
    }

    if(name != NULL){
        DM_Save_File(name, g_buf_rx, total);
    }

    if((buf != NULL) && (len != NULL)){
        size = MIN(total, (UINT32)*len);
        memcpy(buf, g_buf_rx, size);
    }

    if(len != NULL){
        *len = total;
    }

    if(r_size != NULL){
        *r_size = total;
    }

    return DM_SUCCESS;
}



