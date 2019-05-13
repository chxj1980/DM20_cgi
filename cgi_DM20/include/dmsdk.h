#ifndef  __DMSDK_H__
#define  __DMSDK_H__


#ifdef __cplusplus
extern "C" {
#endif


#define DM_SUCCESS          (0)

#define ERR_INVALID_PARAM   (-1)  /* invlalid input parameter */
#define ERR_NOT_SUPPORT     (-2)  /* not support request */

#define STREAM_EVE_STOPPED    0
#define STREAM_EVE_CONNECTING 1
#define STREAM_EVE_CONNFAIL   2
#define STREAM_EVE_CONNSUCC   3
#define STREAM_EVE_RESUME     5
#define STREAM_EVE_NODATA     7
#define STREAM_EVE_CONNERR    8

typedef enum
{
    ERR_UNKNOWN = 1,        // Unknown error
    ERR_INVALID_HANDLE,     // an Invalid handle or possible bad pointer was passed to a function
    ERR_NO_MEMORY,          // Buffer too small or a failure while in memory allocation
    ERR_SOCKET_INVALID,     // an attempt to use an invalid socket handle was made
    ERR_SOCKET_CANT_SET,    // Can't send socket parameters
    ERR_SOCKET_RESOLVE,     // Error while resolving host name
    ERR_SOCKET_CONNECT,     // Error while connecting to the remote server
    ERR_SOCKET_TIME_OUT,    // socket time out error
    ERR_SOCKET_RECV,        // Error while receiving data
    ERR_SOCKET_SEND = 10,   // Error while sending data
    ERR_HEADER_RECV,        // Error while receiving the remote HTTP headers
    ERR_HEADER_NOT_FOUND,   // Could not find element within header
    ERR_HEADER_BIG_CLUE,    // The headers search clue was too large for the internal API buffer
    ERR_HEADER_NO_LENGTH,   // No content length was specified for the outgoing data. the caller should specify chunking mode in the session creation
    ERR_CHUNK_TOO_BIG,      // The HTTP chunk token that was received from the server was too big and possibly wrong
    ERR_AUTH_HOST,          // Could not authenticate with the remote host
    ERR_AUTH_PROXY,         // Could not authenticate with the remote proxy
    ERR_BAD_VERB,           // Bad or not supported HTTP verb was passed to a function
    ERR_LONG_INPUT,         // a function received a parameter that was too large
    ERR_BAD_STATE = 20,     // The session state prevents the current function from proceeding
    ERR_CHUNK,              // Could not parse the chunk length while in chunked transfer
    ERR_BAD_URL,            // Could not parse curtail elements from the URL (such as the host name, HTTP prefix act')
    ERR_BAD_HEADER,         // Could not detect key elements in the received headers
    ERR_BUFFER_RSIZE,       // Error while attempting to resize a buffer
    ERR_BAD_AUTH,           // Authentication schema is not supported
    ERR_AUTH_MISMATCH,      // The selected authentication schema does not match the server response
    ERR_NO_DIGEST_TOKEN,    // an element was missing while parsing the digest authentication challenge
    ERR_NO_DIGEST_ALG,      // Digest algorithem could be MD5 or MD5-sess other types are not supported
    ERR_SOCKET_BIND,        // Binding error
    ERR_TLS_NEGO = 30,      // Tls negotiation error
    ERR_NOT_IMPLEMENTED = 64, // Feature is not (yet) implemented
}DM_ERR_CODE;



#define RESV_LEN    64



typedef struct
{
    char version[64];		//°æ±¾
    int resv[RESV_LEN];
}DM_VERSION;


#define LOG_LEVEL_DEFAULT   3
#define MAX_LOG_LEVEL       8
#define MAX_ROLL_NUM        90


/* Log init
**  path: log file save path, savein current path when path set NULL or not exist
** level: log level, level[0] valid, level[1-15] unused; range [0,MAX_LOG_LEVEL],
**        0:disable, the high value the more log ouput
**  roll: max rollback days,[1,MAX_ROLL_NUM]
**/
typedef struct
{
    char path[64];      /* log file save path */
    int level[16];      /* log level, level[0] valid */
    int roll;           /* rollback,[1,MAX_ROLL_NUM] */
    int unused;
    int resv[RESV_LEN];
}DM_LOG;




/* Open a server
**  hdl: in/out
**   ip: the device ip address
** port: the device server ip, default 80
** user: the device user, always "admin"
**  pwd: the user's passwd, you should know it
**/
int DM_Open(long *hdl, const char *ip, int port, char *user, char *pwd);


/* close a server
**/
int DM_Close(long hdl);


/* version infomation
**/
int DM_Version(DM_VERSION *version);


/* Log init
** return: success return 0, other return error code
**/
int DM_Log_Init(DM_LOG *log);


/* Log exit
** return: success return 0, other return error code
**/
int DM_Log_Exit(void);

typedef int (*VIDEO_NOTIFY_CB)(int, void *);
typedef int (*VIDEO_STREAM_CB)(unsigned char*, int, unsigned int, unsigned short, void *);

typedef int (*INFRARED_NOTIFY_CB)(long hdl, int, void *);
typedef int (*INFRARED_DATA_CB)(long hdl, unsigned char*, int, int, int, void *);


/* System init
** return: success return 0, other return error code
**/
int DM_Sys_Init(void);


/* System uninit
** return: success return 0, other return error code
**/
int DM_Sys_UnInit(void);


/* open stream
**  hdl: [IN], handle
** port: [IN], rtsp port
**
** success retrun 0, fail return error code
**/
int DM_Video_Stream_Start(long hdl, int port);
int DM_Video_Stream_Stop(long hdl);
int DM_Video_Stream_Register(long hdl, VIDEO_NOTIFY_CB notify_cb, VIDEO_STREAM_CB video_cb);


int DM_Infrared_Stream_Start(long hdl, int port, int framerate);
int DM_Infrared_Stream_Stop(long hdl);
int DM_Infrared_Stream_Register(long hdl, INFRARED_NOTIFY_CB notify_cb, INFRARED_DATA_CB data_cb, void *arg);

int DM_Gray_To_Temperature(long hdl, int gray, float *pfTemp);

#ifdef __cplusplus
}
#endif


#endif



