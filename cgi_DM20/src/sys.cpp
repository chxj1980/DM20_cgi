/*
** system
** time   : 2018-10-23
** author : chenks (godence@126.com)
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "dmtype.h"
#include "dmsdk.h"
#include "system.h"




#define DM_MAGIC        0x22777722

#define DM_LOG_TITLE    "DMSDK"
#define DM_LOG_EXTEN    "log"

#define DM_LOG_DIR_DEF  "./log"


typedef struct
{
    int magic;  /*  DM_MAGIC */
    int port;
    char ip[16];
    char user[16];
    char pwd[16];
    DM_SYS_INFO sys_info;
	DM_USER_MANAGE user_manage;
	DM_NET_SET net_set;
    long hdl_rtsp;
    long session_rawdata;
}DM_CONFIG;


static int g_level = 3;
static int g_dm_log_init = 0;
static struct tm g_tm_cur;
static FILE *g_log_fp = NULL;
static DM_LOG g_dm_log;




static long handle_call(void *src, int len)
{
    void *dst = NULL;
    
    dst = malloc(len);
    CHECK_RET(dst != NULL, ERR_NO_MEMORY, "malloc failed");
    
    memcpy((char*)dst, (char*)src, len);

    return (long)dst;
}



/* 检查hdl有效性
** 返回值: 无效返回ERR_INVALID_HANDLE，有效返回DM_SUCCESS
**/
int handle_valid(long hdl)
{
    if((hdl == (long)NULL) || (hdl == ERR_NO_MEMORY)){
        return ERR_INVALID_HANDLE;
    }
    
    DM_CONFIG *config = (DM_CONFIG *)hdl;

    return (config->magic == DM_MAGIC)?DM_SUCCESS:ERR_INVALID_HANDLE;
}



/* 检查目录是否存在
** 返回值: 路径为空或者不存在返回-1，存在返回0
**/
static int is_dir_exist(char *dir_path)
{
    CHECK_RET(dir_path != NULL, ERR_INVALID_PARAM, "dir_path is NULL");
    
    if(access(dir_path, F_OK) != 0){
        printf("DIR is not exist, %02X %02X %02X %02X\r\n", 
            dir_path[0], dir_path[1], dir_path[2], dir_path[3]);
        return -1;        
    }else{
        return 0;
    }
}



/* 创建目录
**/
static int dir_create(char *dir_path)
{
    CHECK_RET(dir_path != NULL, ERR_INVALID_PARAM, "dir_path is NULL");

    mkdir(dir_path, 0755);

    printf("mkdir %s\r\n", dir_path);

    return DM_SUCCESS;
}



/* 删除一个文件
** 返回值: 成功返回0, 失败返回-1,如果文件不存在,也返回-1
**/
static int remove_file(char *filename)
{
    if(access(filename, F_OK) == 0){
        printf("%s remove [begin]\n", filename);
        
	    if(remove(filename) != 0){
            perror("remove");
            return -1;
        }

        sync();
        
        printf("%s remove [end]\n", filename);

        return 0;
    }else{
        printf("%s is not exist\n", filename);
        return -1;
    }
}



int remove_file_by_index(unsigned int index)
{
    int ret;
	char filename[256];

	memset(filename, 0, 256);    
	sprintf(filename, "%s/%s-%08u.%s", g_dm_log.path, DM_LOG_TITLE, index, DM_LOG_EXTEN);
    ret = remove_file(filename);
    if(ret < 0){
        return -1;
    }else{
        return 0;
    }
}



/* 文件名是否符合规范
** title: 文件开始的字符串
** exten: 文件扩展名
** 返回值: 1--符合，0--不符合
**/
static int meet_the_name(char *name, char *title, char *exten)
{
    int i = 0;

    if(strlen(name) != 18){
        return 0;
    }

    if(strncmp(name, title, 5) != 0){
        return 0;
    }

    if(strncmp(name+15, exten, 3) != 0){
        return 0;
    }

    for(i=0; i<8; i++){
        if(!isdigit(name[6+i])){
            return 0;
        }
    }
    
    return 1;
}



/* 当前有多少个日志文件
** 返回值: 成功返回文件个数，失败返回-1
**/
static int log_file_total(int *oldest_num)
{
    int total;
    int old_index;
    int cur_index;
	DIR *dir_log;
	struct dirent *d;


    total = 0;
    old_index = 99999999;
	
	/* 遍历录像目录，找最早的索引号 */
	do{
		dir_log = opendir(g_dm_log.path);
		if (dir_log == NULL){            
			break;
        }
		
		while((d = readdir(dir_log)) != NULL){
            if(meet_the_name(d->d_name, DM_LOG_TITLE, DM_LOG_EXTEN) == 0){
                continue;
            }
            
			sscanf(d->d_name, "DMSDK-%d.log", &cur_index);
            
            old_index = (old_index <= cur_index)?old_index:cur_index;

            total++;
		}

		closedir(dir_log);
	}while(0);

    if(oldest_num != NULL){
        *oldest_num = old_index;
    }

    return total;
}



/* 日志文件回滚
**/
static int log_file_roll(void)
{
    int total;
	int old_index;

    //printf(">>> %s, <In>\r\n", __FUNCTION__);
    
    do{
        total = log_file_total(&old_index);
        if(total <= g_dm_log.roll){
            break;
        }

        /* 删除最早日志文件 */
        remove_file_by_index(old_index);
    }while(1);


    //printf(">>> %s, <Out>\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



/* 组装日志文件名称
**/
static int log_file_fill(char *name, int len)
{
    CHECK_RET(name != NULL, ERR_INVALID_PARAM, "name is NULL");
    CHECK_RET(len > 0, ERR_INVALID_PARAM, "len is error");

    time_t now;
    struct tm *cur;
    
    time(&now);
    cur = localtime(&now);

    snprintf(name, len, "%s/%s-%04d%02d%02d.%s", g_dm_log.path, DM_LOG_TITLE,
            cur->tm_year+1900, cur->tm_mon+1, cur->tm_mday, DM_LOG_EXTEN);

    printf("log_file=%s\n", name);

    g_tm_cur = *cur;

    //printf("g_tm_cur: tm_year=%d, tm_mon=%d, tm_mday=%d\n", g_tm_cur.tm_year, g_tm_cur.tm_mon, g_tm_cur.tm_mday);

    return DM_SUCCESS;
}



/* 打开日志文件
**/
static int log_file_open(void)
{
    int ret;
    char name[64];
    
    ret = log_file_fill(name, sizeof(name));
    CHECK_RET(ret == DM_SUCCESS, ERR_INVALID_PARAM, "input is NULL");
    
    g_log_fp = fopen(name, "a+");
    CHECK_RET(g_log_fp != NULL, ERR_INVALID_PARAM, "fopen failed");

    log_file_roll();

    return DM_SUCCESS;
}



/* 日期发生改变，比如过了一天，或者修改了日期，日志文件需要重新创建
**/
static int log_file_renewal(void)
{
    time_t now;
    struct tm *cur;
    
    time(&now);
    cur = localtime(&now);

    //printf("     cur: tm_year=%d, tm_mon=%d, tm_mday=%d\n", cur->tm_year, cur->tm_mon, cur->tm_mday);
    //printf("g_tm_cur: tm_year=%d, tm_mon=%d, tm_mday=%d\n", g_tm_cur.tm_year, g_tm_cur.tm_mon, g_tm_cur.tm_mday);
    do{
        if((g_tm_cur.tm_year != cur->tm_year) || 
            (g_tm_cur.tm_mon != cur->tm_mon) ||
            (g_tm_cur.tm_mday != cur->tm_mday)){
            //printf("%s, need recreate\n", __FUNCTION__);
            break;
        }else{
            //printf("%s, success\n", __FUNCTION__);
            return DM_SUCCESS;
        }
    }while(0);

    if(g_log_fp != NULL){
        int ret;
        char buf[64];
        
        ret = snprintf(buf, sizeof(buf), "stop current log file, create new one!!!!!\r\n");
        fwrite(buf, ret, 1, g_log_fp);
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
    
    int ret;

    ret = log_file_open();
    CHECK_RET(ret == DM_SUCCESS, ERR_UNKNOWN, "log_file_open failed");

    return DM_SUCCESS;
}



/* 写日志文件
**/
static int log_file_write(char *buf, int len)
{
    static int cnt = 0;
    
    if(g_log_fp == NULL){
        return ERR_INVALID_PARAM;
    }
    
    if(buf == NULL){
        printf("%s, failed\n", __FUNCTION__);
        return ERR_INVALID_PARAM;
    }

    log_file_renewal();
    
    fwrite(buf, len, 1, g_log_fp);
    //printf("%s, len=%d, buf=%s", __FUNCTION__, len, buf);

    if(cnt++ >= 5){
        cnt = 0;
        fflush(g_log_fp);
    }

    return DM_SUCCESS;
}



int dm_rtsp_config_get(long hdl, char *ip, long *hdl_rtsp)
{
    CHECK_RET(handle_valid(hdl) == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");
    
    DM_CONFIG *config = (DM_CONFIG *)hdl;

    if(hdl_rtsp != NULL){
        *hdl_rtsp = config->hdl_rtsp;
    }
	
    if(ip != NULL){
        memcpy(ip, config->ip, sizeof(config->ip));
    }

    return DM_SUCCESS;
}

int dm_rawdata_config_get(long hdl, long *session_rawdata)
{
    CHECK_RET(handle_valid(hdl) == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");
    
    DM_CONFIG *config = (DM_CONFIG *)hdl;

    if(session_rawdata != NULL){
        *session_rawdata = config->session_rawdata;
    }
	
    return DM_SUCCESS;
}

int dm_log(int level, const char *format, ...)
{
    if(level > g_level){
        return 0;
    }

    int len, len1;
    char buf[1024];
    time_t now;
    struct tm *cur;
    va_list marker;
    
    time(&now);
    cur = localtime(&now);	

	va_start(marker, format);
    
    len1 = sprintf(buf, "<%d> %04d-%02d-%02d %02d:%02d:%02d  ",
            level, cur->tm_year+1900, cur->tm_mon+1, cur->tm_mday,
            cur->tm_hour, cur->tm_min, cur->tm_sec);

    len = vsprintf(buf+len1, format, marker);
    log_file_write(buf, len1+len);

    if(level <= 2){
        printf("%s", buf);
        //len = vprintf(format, marker); /* 由CHECK_RET调用，在64位系统会导致段错误*/
    }
    
	va_end(marker);    

    //printf("%s, g_log_fp=%ld\n", __FUNCTION__, (long)g_log_fp);

    return len;
}



int DM_Log_Level(int level)
{
    g_level = level;

    return 0;
}



void DM_Display_Hex(char *title, char *buf, int len)
{
    int i = 0;

    printf("\n------------------- [%s] len=%d---------------------\n", title, len);
    
    for(i=0; i<len ;i++){
        printf("%02X ", buf[i] & 0xFF);
        if((i+1)%8 == 0){
            printf("  ");
        }
        
        if((i+1)%16 == 0){
            printf("\n");
        }
    }

    if((i%16) != 0){
        printf("\n");
    }
    
    printf("---------------------------------------------------\n\n");
}



int dm_dbg_time_start(int *sec, int *usec)
{
    struct timeval tv;
    
    gettimeofday(&tv, NULL);

    *sec = tv.tv_sec;
    *usec = tv.tv_usec;

    return 0;
}



int dm_dbg_time_step(char *txt, int *sec, int *usec, int b_update)
{
    struct timeval tv;

    CHECK_RET((sec != NULL) && (usec != NULL), ERR_INVALID_PARAM, "input is NULL");
    
    gettimeofday(&tv, NULL);

    dm_log(5, "[%s] intv_time = %d ms\r\n", txt, (tv.tv_sec-*sec)*1000 + (tv.tv_usec-*usec)/1000);

    if(b_update){
        *sec = tv.tv_sec;
        *usec = tv.tv_usec;
    }

    return 0;
}



int DM_Open(long *hdl, const char *ip, int port, char *user, char *pwd, DM_ENABLE *en)
{
    CHECK_RET(hdl != NULL, ERR_INVALID_PARAM, "input is NULL");
    CHECK_RET(ip != NULL, ERR_INVALID_PARAM, "input is NULL");
    CHECK_RET((port >= 10) && (port <= 65535), ERR_INVALID_PARAM, "input is NULL");
    CHECK_RET(user != NULL, ERR_INVALID_PARAM, "input is NULL");
    CHECK_RET(pwd != NULL, ERR_INVALID_PARAM, "input is NULL");

    int ret;
    long l_hdl;
    DM_CONFIG config;
    
    memset((char*)&config, 0x00, sizeof(DM_CONFIG));

    config.magic = DM_MAGIC;
    config.port = port;
    strcpy(config.ip, ip);
    strcpy(config.user, user);
    strcpy(config.pwd, pwd);

	//sys_info
    ret = DM_Get_Sys_Info((long)&config, &config.sys_info);
    CHECK_RET(ret == DM_SUCCESS, ret, "DM_Get_Sys_Info failed");

    dm_log(5, "Sys Info: output=%d, ir_w=%d, ir_h=%d, dev_ver=%s\r\n", 
        config.sys_info.output, config.sys_info.ir_w, config.sys_info.ir_h, config.sys_info.dev_version);
	//user_info
	ret=DM_Get_User_info((long)&config, &config.user_manage, en);
	CHECK_RET(ret == DM_SUCCESS, ret, "DM_Get_User_Info failed");
	//net_set
	ret=DM_Get_Net_info((long)&config, &config.net_set, en);
	CHECK_RET(ret == DM_SUCCESS, ret, "DM_Get_Net_Info failed");

	//
    ret = rtsp_session_create(&config.hdl_rtsp);
    CHECK_RET(ret == DM_SUCCESS, ret, "rtsp_session_create failed");

    ret = rawdata_session_create(&config.session_rawdata);
    CHECK_RET(ret == DM_SUCCESS, ret, "rawdata_session_create failed");

	l_hdl = handle_call(&config, sizeof(DM_CONFIG));
    if(l_hdl == ERR_NO_MEMORY){
        return ERR_NO_MEMORY;
    }
    
    *hdl = l_hdl;

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}




int DM_Close(long hdl)
{
    CHECK_RET(handle_valid(hdl) == DM_SUCCESS, ERR_INVALID_HANDLE, "invalid handle");

    DM_CONFIG *config = (DM_CONFIG *)hdl;
    
    rtsp_session_destroy(config->hdl_rtsp);

	rawdata_session_destroy(config->session_rawdata);

    free((void*)hdl);

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
	return DM_SUCCESS;
}



int DM_Version(DM_VERSION *version)
{
    CHECK_RET(version != NULL, ERR_INVALID_PARAM, "input is NULL");

    sprintf(version->version, "V1.02.00, Build Time[%s, %s] Release\n", __DATE__, __TIME__);

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



int DM_Log_Init(DM_LOG *log)
{
    CHECK_RET(g_dm_log_init == 0, ERR_INVALID_PARAM, "log has open");
    
    CHECK_RET(log != NULL, ERR_INVALID_PARAM, "input is NULL");
    CHECK_RET((log->level[0] >= 0) && (log->level[0] <= MAX_LOG_LEVEL), ERR_INVALID_PARAM, "check log->level[0]");
    CHECK_RET((log->roll > 0) && (log->roll <= MAX_ROLL_NUM), ERR_INVALID_PARAM, "check log->roll");

    int ret;

    if(log->level[0] == 0){
        printf("%s, Disable\n", __FUNCTION__);
        return DM_SUCCESS;
    }else{
        g_level = log->level[0];
    }
    
    //printf("%s, line=%d\n", __FUNCTION__, __LINE__);
    
    ret = is_dir_exist(log->path);
    if(ret < 0){
        snprintf(log->path, sizeof(log->path), DM_LOG_DIR_DEF);
        
        ret = is_dir_exist(log->path);
        if(ret < 0){
            ret = dir_create(log->path);
            CHECK_RET(ret == DM_SUCCESS, ERR_UNKNOWN, "create log dir failed");
        }
    }

    g_dm_log = *log;
    
    ret = log_file_open();
    CHECK_RET(ret == DM_SUCCESS, ERR_UNKNOWN, "log_file_open failed");

    dm_log(3, "%s, success\r\n", __FUNCTION__);

    g_dm_log_init = 1;

    return DM_SUCCESS;
}



int DM_Log_Exit(void)
{
    if(g_log_fp != NULL){
        fclose(g_log_fp);
        g_log_fp = NULL;
    }

    g_dm_log_init = 0;

    return DM_SUCCESS;
}



/* 保存文件
** name: 文件名，包含路径
** 返回值: 成功返回0，失败返回错误码
**/
int DM_Save_File(const char *name, char *data, int size)
{
    int fd, ret, oft, len;


    CHECK_RET(name != NULL , -1, "name is NULL");
    CHECK_RET(data != NULL , -1, "data is NULL");
    CHECK_RET(size > 0 , -1, "size should big then 0");

    fd = open(name, O_RDWR|O_CREAT|O_TRUNC, 0666);
    if(fd <= 0){
        fd = open(name, O_RDWR|O_CREAT|O_TRUNC, 0666);
        CHECK_RET(fd > 0, -1, "open file error");
    }

    oft = 0;
    len = size;
    while(len > 0){
        ret = write(fd, (char*)data+oft, len);
        if(ret < 0){
            dm_log(2, "save <%s> failed, len=%d, ret=%d\r\n", name, len, ret);
            goto __OUT__;
        }

        len -= ret;
        oft += ret;
    }

    ret = 0;

    dm_log(3, "save <%s> ok, len=%d\r\n", name, size);

__OUT__:
    close(fd);
    return ret;
}



/* 坐标系转换
**/
static void dm_coordinate_switch(int src_w, int src_h, int dst_w, int dst_h, int *x1, int *y1)
{
    CHECK(src_w != 0, "input is NULL");
    CHECK(src_h != 0, "input is NULL");
    
    *x1 = *x1 * dst_w / src_w;
    *y1 = *y1 * dst_h / src_h;
}



/* 坐标系转换
** type: 0-osd坐标系，1-探测器坐标系
**  dir: 0-本地到远程，1-远程到本地
**/
int dm_pos_adjust(long hdl, int type, int dir, int *x, int *y)
{
    CHECK_RET(handle_valid(hdl) == DM_SUCCESS, ERR_INVALID_PARAM, "invalid handle");
    CHECK_RET(x != NULL, ERR_INVALID_PARAM, "input is NULL");
    CHECK_RET(y != NULL, ERR_INVALID_PARAM, "input is NULL");

    int src_w, src_h, dst_w, dst_h;
    DM_CONFIG *config;

    config = (DM_CONFIG*)hdl;

    src_w = config->sys_info.coor_w;
    src_h = config->sys_info.coor_h;

    if(type == COOR_T_OSD){
        dst_w = 640;
        dst_h = 480;
    }else if(type == COOR_T_IR){
        dst_w = config->sys_info.ir_w;
        dst_h = config->sys_info.ir_h;
    }else{
        CHECK_RET(0, ERR_INVALID_PARAM, "input is NULL");
    }

    dm_log(4, "%s, A pos(%d, %d)\r\n", __FUNCTION__, *x, *y);

    if(dir == COOR_DIR_H2N){
        dm_coordinate_switch(src_w, src_h, dst_w, dst_h, x, y);
    }else{
        dm_coordinate_switch(dst_w, dst_h, src_w, src_h, x, y);
    }
    
    dm_log(4, "%s, B pos(%d, %d)\r\n", __FUNCTION__, *x, *y);

    return DM_SUCCESS;
}



/* 红外坐标检测，主要用于判断测温坐标的有效性
**  dir: 0-本地到远程，1-远程到本地
** 成功返回DM_SUCCESS，失败返回ERR_INVALID_PARAM
**/
int dm_pos_check(long hdl, int dir, int x, int y)
{
    CHECK_RET(handle_valid(hdl) == DM_SUCCESS, ERR_INVALID_PARAM, "invalid handle");

    DM_CONFIG *config;

    config = (DM_CONFIG*)hdl;

    if((x < 0) || (x >= config->sys_info.coor_w)){
        dm_log(2, "%s, x=%d illegal, range[%d, %d)\r\n", __FUNCTION__, x, 0, config->sys_info.coor_w);
        return ERR_INVALID_PARAM;
    }

    
    if((y < 0) || (y >= config->sys_info.coor_h)){
        dm_log(2, "%s, y=%d illegal, range[%d, %d)\r\n", __FUNCTION__, y, 0, config->sys_info.coor_h);
        return ERR_INVALID_PARAM;
    }

    return DM_SUCCESS;
}



void http_uri_fill(long hdl, char *cmd, char *subcmd, char *uri)
{
    DM_CONFIG *config;

    config = (DM_CONFIG*)hdl;

    sprintf(uri, "http://%s:%d/cgi-bin/%s?%s", config->ip, config->port, cmd, subcmd);

    dm_log(5, "%s, uri=%s\r\n", __FUNCTION__, uri);
}



void http_uri_jpeg_fill(long hdl, int ch, char *uri)
{
    DM_CONFIG *config;

    config = (DM_CONFIG*)hdl;

    sprintf(uri, "http://%s:%d/stream?Source=Jpeg&Channel=%d&Frames=2", config->ip, 5000, ch);

    dm_log(5, "%s, uri=%s\r\n", __FUNCTION__, uri);
}


void http_uri_rawdata_fill(long hdl, int port, int interval, char *uri)
{
    DM_CONFIG *config;

    config = (DM_CONFIG*)hdl;

    sprintf(uri, "http://%s:%d/cgi-bin/stream?Source=Raw&Type=FRAME&Mode=TCP&Port=0&Heart-beat=No&Frames=-1&Interval=%d",
		config->ip, port, interval);

    dm_log(5, "%s, uri=%s\r\n", __FUNCTION__, uri);
}





