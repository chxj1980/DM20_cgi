#ifndef _SYSTEM_H_
#define _SYSTEM_H_

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
