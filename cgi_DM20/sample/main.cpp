/*
** system
** time   : 2018-10-23
** author : chenks (godence@126.com)


** 2019-02-21:
		删除无用的函数
		修改 DM_Infrared_Stream_Register 函数传入用户数据


*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "dmsdk.h"
#include "../src/system.h"

#define CHECK(cond, txt)\
    do{\
        if(!(cond)){\
            printf("[Func]:%s [Line]:%d  %s\n", __FUNCTION__, __LINE__, txt);\
            return;\
        }\
    }while(0);


#define CHECK_RET(cond, ret, txt)\
    do{\
        if(!(cond)){\
            printf("[Func]:%s [Line]:%d  %s\n", __FUNCTION__, __LINE__, txt);\
            return (ret);\
        }\
    }while(0);



typedef void (*FUNC)(long hdl, int argc, const char *argv[]);

typedef struct _mycmd{
    char *cmd;
    FUNC fun;
    char *help;
}MYCMD;

typedef struct _userMessage{
	int notifi_cb;
	int data_cb;
}USER_MESSAGE;

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

static void usage(const char *exe)
{
    printf(" Usage: %s ip item [param]\n", exe);
    printf("-----------------------------------\n");
	printf("rtsp [port]\n");
	printf("raw  [port] [framerate]\n");
    printf("\n");
}

static int rtsp_notify_cb(int event, void * puser)
{
	if (event == STREAM_EVE_CONNECTING)
	{
		printf("Connecting\n");
	}
	else if (event == STREAM_EVE_CONNFAIL)
	{
		printf("Connect failed\n");
	}
	else if (event == STREAM_EVE_CONNSUCC)
	{
		printf("Connect success\n");
	}
	else if (event == STREAM_EVE_NODATA)
	{
		printf("NO Data\n");
	}
	else if (event == STREAM_EVE_RESUME)
	{
		printf("Resume\n");
	}
	else if (event == STREAM_EVE_STOPPED)
	{
		printf("Disconnect\n");
	}

    return 0;
}



static int rtsp_video_cb(unsigned char* pdata, int len, unsigned int ts, unsigned short seq, void *puser)
{
	printf("V: data=0x%lX, len=%d\n", (long)pdata, len);

	return 0;
}

static int raw_data_cb(long hdl, unsigned char* pdata, int len, int width, int height, void *puser)
{

	USER_MESSAGE *p = (USER_MESSAGE *) puser;

	printf("D: data=0x%lX, len=%d, w=%d, h=%d -- %#X\n", (long)pdata, len, width, height, p->data_cb);
/*
	FILE *fp;
	fp = fopen("save", "ab+");
	fwrite(pdata,len,1,fp);
	fclose(fp);
*/
	return 0;
}

static int raw_notify_cb(long hdl, int event, void * puser)
{

	USER_MESSAGE *p = (USER_MESSAGE *) puser;

	printf("notify -- %#X", p->notifi_cb);

	if (event == STREAM_EVE_CONNECTING)
	{
		printf("Connecting\n");
	}
	else if (event == STREAM_EVE_CONNFAIL)
	{
		printf("Connect failed\n");
	}
	else if (event == STREAM_EVE_CONNSUCC)
	{
		printf("Connect success\n");
	}
	else if (event == STREAM_EVE_STOPPED)
	{
		printf("Disconnect\n");
	}
	else if (event == STREAM_EVE_CONNERR)
	{
		printf("Connection has error\n");
	}

    return 0;
}



static void do_blk_rtsp(long hdl, int argc, const char *argv[])
{
    int port;

	port = 554;

    if(argc >= 5){
		port = atoi(argv[4]);
    }

    DM_Video_Stream_Register(hdl, rtsp_notify_cb, rtsp_video_cb);
	DM_Video_Stream_Start(hdl, port);

	sleep(1);
	printf("input any key for exit !!!\n");
	getchar();

	DM_Video_Stream_Stop(hdl);
}

static void do_blk_raw(long hdl, int argc, const char *argv[])
{
    int port;
	int framerate;
	USER_MESSAGE *pstUser;

	pstUser = (USER_MESSAGE *) malloc(sizeof (USER_MESSAGE));

	pstUser->notifi_cb = 0x12345678;
	pstUser->data_cb = 0x87654321;

	port = 5000;
	framerate = 25;

    if(argc >= 6){
		port = atoi(argv[4]);
    }

    if(argc >= 6){
		framerate = atoi(argv[5]);
    }

	DM_Infrared_Stream_Register(hdl, raw_notify_cb, raw_data_cb, (void *)pstUser);
	DM_Infrared_Stream_Start(hdl, port, framerate);

	sleep(1);
	printf("input any key for exit !!!\n");
	getchar();

	DM_Infrared_Stream_Stop(hdl);
}
/*************************************************************************************/

void check_enable(DM_ENABLE *en, const char *argv[])
{
	if(strcmp("user_login", argv[2]) == 0){
		en->enable_login = 1;
	}
	if(strcmp("user_add", argv[2]) == 0){
		en->enable_add = 1;
	}
	if(strcmp("user_del", argv[2]) == 0){
		en->enable_del = 1;
	}
	if(strcmp("user_chpass", argv[2]) == 0){
		en->enable_chpass = 1;
	}
}

static void do_user_query(long hdl, int argc, const char *argv[])
{
	DM_CONFIG *config = (DM_CONFIG *)hdl;
	printf("%s\n",config->user_manage.user_info);
}

static void do_user_login(long hdl, int argc, const char *argv[])
{
	DM_CONFIG *config = (DM_CONFIG *)hdl;
	printf("%s\n",config->user_manage.ret_login);
}

static void do_user_add(long hdl, int argc, const char *argv[])
{
	printf("add user!!!\n");
}
static void do_user_del(long hdl, int argc, const char *argv[])
{
	printf("delete user!!!\n");
}
static void do_user_chpass(long hdl, int argc, const char *argv[])
{
	printf("password changed!!!\n");
}

static void do_net_ip(long hdl, int argc, const char *argv[])
{
	DM_CONFIG *config = (DM_CONFIG *)hdl;

	printf("ip: %s\nnetmask: %s\n",config->net_set.tcp_ip.ip_addr, config->net_set.tcp_ip.netmask);
	printf("gateway: %s\nmac_addr: %s\ndns: %s\ndns2: %s\nDHCP: %s\nHttpPort: %d\nRtspPort: %d\n",\
		config->net_set.tcp_ip.gateway, config->net_set.tcp_ip.mac_addr,\
		config->net_set.tcp_ip.dns, config->net_set.tcp_ip.dns2,\
		config->net_set.tcp_ip.dhcp, config->net_set.tcp_ip.http_port,\
		config->net_set.tcp_ip.rtsp_port);
}

static void do_net_ftp(long hdl, int argc, const char *argv[])
{
	DM_CONFIG *config = (DM_CONFIG *)hdl;

	printf("FtpEnable: %s\nFtpServer: %s\nFtpPort: %d\nUserName: %s\nPassword: %s\nStoreDirectory: %s\n",\
		config->net_set.ftp_d.enable, config->net_set.ftp_d.server_ip,\
		config->net_set.ftp_d.port, config->net_set.ftp_d.user_name,\
		config->net_set.ftp_d.password, config->net_set.ftp_d.store_dir);
}

static void do_net_mail(long hdl, int argc, const char *argv[])
{
	DM_CONFIG *config = (DM_CONFIG *)hdl;

	printf("SmtpEnable: %s\nSmtpServer: %s\nSmtpPort: %d\nFromAddr: %s\nUserName: %s\nPassword: %s\nReceiptAddr1: %s\nReceiptAddr2: %s\nReceiptAddr2: %s\nReceiptAddr2: %s\n",\
		config->net_set.mail_d.enable, config->net_set.mail_d.addr,\
		config->net_set.mail_d.port, config->net_set.mail_d.fromaddr, \
		config->net_set.mail_d.username, config->net_set.mail_d.password, \
		config->net_set.mail_d.receiptaddr1, config->net_set.mail_d.receiptaddr2,\
		config->net_set.mail_d.receiptaddr3, config->net_set.mail_d.receiptaddr4);
}

static void do_net_gb28181(long hdl, int argc, const char *argv[])
{
	DM_CONFIG *config = (DM_CONFIG *)hdl;

	printf("Enable: %s\nServerID: %s\nServerDomain: %s\nServerIP: %s\nServerPort: %d\nAuthID: %s\nPassword: %s\nLocalPort: %d\nExpires: %d\nHeartBeatTime: %d\nHeartBeatCount: %d\nRegisterStatus: %s\nVideoID: %s\n",\
		config->net_set.gb_d.enable, config->net_set.gb_d.id_server, config->net_set.gb_d.domain_server,\
		config->net_set.gb_d.ip_server, config->net_set.gb_d.port_server, \
		config->net_set.gb_d.auth_id, config->net_set.gb_d.password, \
		config->net_set.gb_d.port_local, config->net_set.gb_d.expires,\
		config->net_set.gb_d.hbtime, config->net_set.gb_d.hbcount,\
		config->net_set.gb_d.rstatus, config->net_set.gb_d.video_id);
}

static MYCMD blk_cmd_table[] =
{
    {"rtsp",             do_blk_rtsp,             "rtsp"},
	{"raw",              do_blk_raw,              "raw"},
    {"user_query",       do_user_query,           "user_query"},
    {"user_login",       do_user_login,           "user_login"},
    {"user_add",         do_user_add,             "user_add"},
    {"user_del",         do_user_del,             "user_del"},
    {"user_chpass",      do_user_chpass,          "user_chpass"},
    {"net_ip",           do_net_ip,                "ip/tcp"},
    {"net_ftp",          do_net_ftp,               "ftp"},
    {"net_mail",         do_net_mail,              "mail"},
    {"net_gb28181",      do_net_gb28181,           "gb28181 platform"},
};

static void fun_blk(long hdl, int argc, const char *argv[])
{
    int i;

    for(i=0; i<sizeof(blk_cmd_table)/sizeof(blk_cmd_table[0]); i++){
        if(strcmp(argv[2], blk_cmd_table[i].cmd) == 0){
            blk_cmd_table[i].fun(hdl, argc, argv);
            return;
        }
    }

    printf("[%s] not support\n", argv[2]);
	usage(argv[0]);
}

int main(int argc, const char *argv[])
{
    int ret;
    long hdl;
    const char *ip;
	DM_ENABLE en;

    if(argc < 3){
        usage(argv[0]);
        return 0;
    }

    DM_VERSION ver;

    DM_Version(&ver);
    printf("%s\n", ver.version);

	DM_Sys_Init();

    DM_LOG log;

    memset((char*)&log, 0x00, sizeof(log));
    log.level[0] = LOG_LEVEL_DEFAULT;
    log.roll = 7;
    snprintf(log.path, sizeof(log.path), "./log");

    DM_Log_Init(&log);

    ip = argv[1];

	check_enable(&en, argv);
	
    ret = DM_Open(&hdl, ip, 80, "admin", "Admin123", &en);
    if(ret != DM_SUCCESS){
        printf("DM_Open failed, ret=%d\n", ret);
        return 0;
    }

    fun_blk(hdl, argc, argv);

    DM_Close(hdl);

    DM_Log_Exit();

	DM_Sys_UnInit();

	return 0;
}



