#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#define RESV_LEN    64

typedef struct
{
    char dev_version[64];
    int output; /* 0:PAL, 1:NTSC */
    int ir_w;
    int ir_h;
    int coor_w;
    int coor_h;
    int unused;
    int resv[RESV_LEN];
}DM_SYS_INFO;

typedef struct
{
	char user_info[128];
	char ret_login[32];
	int resv[RESV_LEN];
}DM_USER_MANAGE;

typedef struct
{
	char ip_addr[16];
	char netmask[16];
	char gateway[16];
	char mac_addr[32];
	char dns[16];
	char dns2[16];
	char dhcp[4];
	int http_port;
	int rtsp_port;
}DM_TCP_IP;

typedef struct
{
	char enable[4];
	char server_ip[16];
	int port;
	char user_name[16];
	char password[32];
	char store_dir[32];
}DM_FTP;

typedef struct
{
	char enable[4];
	char addr[16];
	int port;
	char fromaddr[16];
	char username[8];
	char password[16];
	char receiptaddr1[16];
	char receiptaddr2[16];
	char receiptaddr3[16];
	char receiptaddr4[16];
}DM_MAIL;

typedef struct
{
	char enable[4];
	char id_server[32];
	char domain_server[16];
	char ip_server[16];
	int port_server;
	char auth_id[32];
	char password[16];
	int port_local;
	int expires;
	int hbtime;
	int hbcount;
	char rstatus[4];
	char video_id[32];
}DM_GB;

typedef struct
{
	DM_TCP_IP tcp_ip;
	DM_FTP ftp_d;
	DM_MAIL mail_d;
	DM_GB gb_d;
}DM_NET_SET;

typedef struct
{
    int x;      /* X pos */
    int y;      /* Y pos */
	int resv[4];
}DM_POS;


typedef struct
{
    int enable;         /* [0,1] */
    float value;        /* temperature, xx.x */
    DM_POS pos;         /* temperature measure point position */
    int resv[RESV_LEN];
}TEMP_PONIT;


typedef struct
{
    int enable;     /* [0,1] */
    float min;      /* min temperature, xx.x */
    float max;      /* max temperature, xx.x */
    float avg;      /* avg temperature, xx.x */
    DM_POS pos;     /* temperature measure area highest position */
    int resv[RESV_LEN];
}TEMP_AREA;


#define MAX_TMEP_NUM    6


typedef struct
{
    TEMP_PONIT point[MAX_TMEP_NUM + 10];
    TEMP_AREA area[MAX_TMEP_NUM + 10];
    int resv[RESV_LEN];
}DM_TEMP_INFO;


typedef struct
{
    int status;         /* alarm status£¬0:none, 1:alarm */
    float temp_alarm;   /* alarm temp */
    float temp_refer;   /* refer temp */
    int unused;         /* struct 8 bytes align */
    DM_POS pos;
    int resv[8];
}DM_ALARM_CELL;


typedef struct
{
    int status;         /* alarm status£¬0:none, 1:alarm */
    int unused;         /* struct 8 bytes align */
    DM_ALARM_CELL point[MAX_TMEP_NUM + 10];
    DM_ALARM_CELL area[MAX_TMEP_NUM + 10];
    int resv[RESV_LEN];
}DM_ALARM_INFO;


#define MAX_CAM_NAME_LEN        64


typedef struct
{
    int show;                       /* 0:not display£¬1:display */
    int unused;                     /* struct 8 bytes align */
    char name[MAX_CAM_NAME_LEN];    /* camera name, UTF-8 */
    DM_POS pos;                     /* camera pos */
    int resv[RESV_LEN];
}DM_OSD_CAM;



typedef struct
{
    int show;                       /* 0:not display£¬1:display */
    int unused;                     /* struct 8 bytes align */
    char name[MAX_CAM_NAME_LEN];    /* user define name, UTF-8 */
    DM_POS pos;                     /* user define name pos */
    int resv[RESV_LEN];
}DM_OSD_UDF;



typedef struct
{
    int show;                       /* 0:not display£¬1:display */
    int unused;                     /* struct 8 bytes align */
    DM_POS pos;                     /* camera pos */
    int resv[RESV_LEN];
}DM_OSD_TIME;


typedef enum
{
    DM_MEASURE_POINT = 0,
    DM_MEASURE_AREA
}DM_MEASURE_TYPE;



typedef struct
{
    int idx;                        /* [0,MAX_TMEP_NUM) */
    int type;                       /* DM_MEASURE_TYPE; 0:point£¬1:area */
    int enable;                     /* 0:disable£¬1:enable */
    int start_x;                    /* measure pos */
    int start_y;
    int end_x;                      /* measure end pos, valid by area */
    int end_y;
    int unused;                     /* struct 8 bytes align */
    int resv[RESV_LEN];
}DM_TEMP_MCONFIG;



typedef enum
{
    DM_EMAIL_TYPE_TXT = 0,
    DM_EMAIL_TYPE_TXT_JPEG
}DM_EMAIL_TYPE;


typedef struct
{
    int enable;         /* 0:disable£¬1:enable */
    float temp_thrld;   /* high temp alarm threshold  */
    int rec_en;         /* relation record */
    int email_en;       /* relation email */
    int email_type;     /* email type, DM_EMAIL_TYPE, 0:txt, 1:txt+jpeg */
    int ftp_en;         /* relation ftp upload jpeg */
    int rec_time;       /* record time,(second) */
    int prerec_time;    /* prepare record time,(second) */
    int resv[RESV_LEN];
}DM_TEMP_ACONFIG;


int DM_Get_Sys_Info(long hdl, DM_SYS_INFO *info);
int DM_Get_User_info(long hdl, DM_USER_MANAGE *info, DM_ENABLE *en);
int DM_Get_Net_info(long hdl, DM_NET_SET *info, DM_ENABLE *en);
int DM_Get_Alarm_Info(long hdl, DM_ALARM_INFO *info);
int DM_Get_Temp_Info(long hdl, DM_TEMP_INFO *info);
int DM_Set_Osd_Cam(long hdl, DM_OSD_CAM *osd);
int DM_Get_Osd_Cam(long hdl, DM_OSD_CAM *osd);
int DM_Set_Osd_Udf(long hdl, DM_OSD_UDF *osd);
int DM_Get_Osd_Udf(long hdl, DM_OSD_UDF *osd);
int DM_Set_Osd_Time(long hdl, DM_OSD_TIME *osd);
int DM_Get_Osd_Time(long hdl, DM_OSD_TIME *osd);
int DM_Set_Temp_MConfig(long hdl, DM_TEMP_MCONFIG *measure);
int DM_Get_Temp_MConfig(long hdl, DM_TEMP_MCONFIG *measure);
int DM_Set_Temp_AConfig(long hdl, DM_TEMP_ACONFIG *alarm);
int DM_Get_Temp_AConfig(long hdl, DM_TEMP_ACONFIG *alarm);

#endif
