#ifndef  __DMTYPE_H__
#define  __DMTYPE_H__

#ifdef __cplusplus 
extern "C" { 
#endif


#define  MAX_NAME_LEN     32
#define  MAX_DATA_LEN     128

#define  PARAM_DELIMITER   '&'
#define  VALUE_DELIMITER   '='


#define COOR_T_OSD      0
#define COOR_T_IR       1
#define COOR_DIR_H2N    0
#define COOR_DIR_N2H    1


#define D1_WIDTH		720
#define D1_HEIGHT 	    576
#define D1_N_WIDTH		720
#define D1_N_HEIGHT 	480
#define VGA_WIDTH		640
#define VGA_HEIGHT		480



typedef struct
{
    char key[MAX_NAME_LEN];
    char value[MAX_DATA_LEN];
}DM_DICT;



typedef struct
{
    int count;
    DM_DICT dict[32];
}DM_DICTS;




#define CHECK(cond, txt)\
    do{\
        if(!(cond)){\
            dm_log(2, "[Func]:%s [Line]:%d  %s\r\n", __FUNCTION__, __LINE__, txt);\
            return;\
        }\
    }while(0);


#define CHECK_RET(cond, ret, txt)\
    do{\
        if(!(cond)){\
            dm_log(2, "[Func]:%s [Line]:%d  %s\r\n", __FUNCTION__, __LINE__, txt);\
            return (ret);\
        }\
    }while(0);


int dm_rtsp_config_get(long hdl, char *ip, long *hdl_rtsp);
int dm_rawdata_config_get(long hdl, long *session_rawdata);
int dm_log(int level, const char *format, ...);
int DM_Save_File(const char *name, char *data, int size);
int dm_pos_adjust(long hdl, int type, int dir, int *x, int *y);
int dm_pos_check(long hdl, int dir, int x, int y);
int dm_dbg_time_start(int *sec, int *usec);
int dm_dbg_time_step(char *txt, int *sec, int *usec, int b_update);


void http_uri_fill(long hdl, char *cmd, char *subcmd, char *uri);
void http_uri_jpeg_fill(long hdl, int ch, char *uri);
void http_uri_rawdata_fill(long hdl, int port, int interval, char *uri);

int http_post(long hdl, char *cmd, char *subcmd, DM_DICTS *dicts);
int http_get(long hdl, char *cmd, char *subcmd, DM_DICTS *dicts);
int http_get_jpeg(long hdl, int ch, const char *name, char *buf, int *len, int *r_size);

int prase_dicts(char *buf, char para_delimiter, char val_delimiter, DM_DICTS *dicts);
int fetch_dicts(DM_DICTS *dicts, char *key, char*value);


int rtsp_session_create(long *hdl);
int rtsp_session_destroy(long hdl);
int rawdata_session_create(long *hdl);
int rawdata_session_destroy(long hdl);


int handle_valid(long hdl);
void DM_Display_Hex(char *title, char *buf, int len);

#ifdef __cplusplus 
}
#endif

#endif



