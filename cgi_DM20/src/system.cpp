/*
** system
** time   : 2018-10-23
** author : chenks (godence@126.com)
*/
#include <stdio.h>
#include "HTTPClient.h"
#include "dmtype.h"
#include "dmsdk.h"
#include "system.h"


/* 参数项使能状态
** 返回值: 0: not enable, 1: enable, -1: error
**/
static int para_enable(char *name)
{
    CHECK_RET(name != NULL, ERR_INVALID_PARAM, "input is NULL");

    if((strcmp(name, "Yes") == 0) || (strcmp(name, "On") == 0)){
        return 1;
    }else{
        return 0;
    }
}



/* 解析出点测温数据
** 输入数据: 0|57.4@1|0.0@2|0.0@3|0.0@4|0.0@5|0.0
** 输出结果: idx=0, val=57.4
**           idx=1, val=0.0  
*/
static int temp_point(long hdl, char *buf, TEMP_PONIT *point)
{
    int i, idx;
    DM_DICT *dict;
    DM_DICTS dicts;
    
    prase_dicts(buf, '@', '|', &dicts);

    for(i=0; i<dicts.count; i++){
        dict = &dicts.dict[i];
        idx = atoi(dict->key);
        
        if(idx < 0 || idx >= MAX_TMEP_NUM){
            continue;
        }
        
        sscanf(dict->value, "%f|%d,%d", &point[idx].value, &point[idx].pos.x, &point[idx].pos.y);

        if(point[idx].value == 0.0){
            point[idx].enable = 0;
        }else{
            point[idx].enable = 1;
        }        

        dm_pos_adjust(hdl, COOR_T_IR, COOR_DIR_N2H, &point[idx].pos.x, &point[idx].pos.y);
        
        dm_log(5, "%s, point[%d]: enable=%d, val=%.1f, x=%d, y=%d\n", 
            __FUNCTION__, idx, point[idx].enable, point[idx].value, point[idx].pos.x, point[idx].pos.y);
    }

    return 0;
}



/* 解析出点测温数据
** 输入数据: 0|66.1,58.2,58.8@1|0.0,0.0,0.0@2|0.0,0.0,0.0@3|0.0,0.0,0.0@4|0.0,0.0,0.0@5|0.0,0.0,0.0
** 输出结果: idx=0, max=66.1, min=58.2, avg=58.8
**           idx=1, max=0.0, min=0.0, avg=0.0  
*/
static int temp_area(long hdl, char *buf, TEMP_AREA *area)
{
    int i, idx;
    DM_DICT *dict;
    DM_DICTS dicts;
    
    prase_dicts(buf, '@', '|', &dicts);

    for(i=0; i<dicts.count; i++){
        dict = &dicts.dict[i];
        idx = atoi(dict->key);
        
        if(idx < 0 || idx >= MAX_TMEP_NUM){
            continue;
        }

        sscanf(dict->value, "%f,%f,%f|%d,%d", &area[idx].max, &area[idx].min, &area[idx].avg, 
            &area[idx].pos.x, &area[idx].pos.y);
        
        if((area[idx].max == 0.0) && (area[idx].min == 0.0)){
            area[idx].enable = 0;
        }else{
            area[idx].enable = 1;
        }        

        dm_pos_adjust(hdl, COOR_T_IR, COOR_DIR_N2H, &area[idx].pos.x, &area[idx].pos.y);
        
        dm_log(5, "%s, area[%d]: enable=%d, max=%.1f, min=%.1f, avg=%.1f, x=%d, y=%d\n", 
            __FUNCTION__, idx, area[idx].enable, area[idx].max, area[idx].min, area[idx].avg,
            area[idx].pos.x, area[idx].pos.y);
    }

    return 0;
}



static int temp_measure_set(long hdl, DM_DICTS *dicts, DM_TEMP_MCONFIG *measure)
{
    int cnt, lx, ly;

    
    CHECK_RET(dicts != NULL, ERR_INVALID_PARAM, "input is NULL");
    CHECK_RET(measure != NULL, ERR_INVALID_PARAM, "input is NULL");

    cnt = 0;
    memset(dicts, 0x00, sizeof(DM_DICTS));

    sprintf(dicts->dict[cnt].key, "Index");
    sprintf(dicts->dict[cnt].value, "%d", measure->idx);
    cnt++;
    
    sprintf(dicts->dict[cnt].key, "Enable");
    sprintf(dicts->dict[cnt].value, "%s", measure->enable?"On":"Off");
    cnt++;

    lx = measure->start_x;
    ly = measure->start_y;
    dm_pos_adjust(hdl, COOR_T_IR, COOR_DIR_H2N, &lx, &ly);
    sprintf(dicts->dict[cnt].key, "StartX");
    sprintf(dicts->dict[cnt].value, "%d", lx);
    cnt++;
    
    sprintf(dicts->dict[cnt].key, "StartY");
    sprintf(dicts->dict[cnt].value, "%d", ly);
    cnt++;

    if(measure->type == DM_MEASURE_AREA){
        lx = measure->end_x;
        ly = measure->end_y;
        dm_pos_adjust(hdl, COOR_T_IR, COOR_DIR_H2N, &lx, &ly);

        sprintf(dicts->dict[cnt].key, "EndX");
        sprintf(dicts->dict[cnt].value, "%d", lx);
        cnt++;
        
        sprintf(dicts->dict[cnt].key, "EndY");
        sprintf(dicts->dict[cnt].value, "%d", ly);
        cnt++;
    }

    dicts->count = cnt;
    
    return DM_SUCCESS;
}



static int temp_measure_get(long hdl, DM_DICTS *dicts, DM_TEMP_MCONFIG *measure)
{
    int i, ret;
    char value[16];
    DM_DICT *dict;
    

    CHECK_RET(dicts != NULL, ERR_INVALID_PARAM, "input is NULL");
    CHECK_RET(measure != NULL, ERR_INVALID_PARAM, "input is NULL");

    ret = fetch_dicts(dicts, "Index", value);
    CHECK_RET(ret == 0, ERR_INVALID_PARAM, "has no [Index]");

    measure->idx = atoi(value);

    for(i=0; i<dicts->count; i++){
        dict = &dicts->dict[i];
        
        if(strcmp(dict->key, "Enable") == 0){
            measure->enable = para_enable(dict->value);
        }else if(strcmp(dict->key, "StartX") == 0){
            measure->start_x = atoi(dict->value);
        }else if(strcmp(dict->key, "StartY") == 0){
            measure->start_y = atoi(dict->value);
        }else if(strcmp(dict->key, "EndX") == 0){
            measure->end_x = atoi(dict->value);
        }else if(strcmp(dict->key, "EndY") == 0){
            measure->end_y = atoi(dict->value);
        }
    }

    dm_pos_adjust(hdl, COOR_T_IR, COOR_DIR_N2H, &measure->start_x, &measure->start_y);
    dm_pos_adjust(hdl, COOR_T_IR, COOR_DIR_N2H, &measure->end_x, &measure->end_y);
    
    return 0;
}



static int get_sys_info1(long hdl, DM_SYS_INFO *info)
{
    int i, ret;
    DM_DICTS dicts;

    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");

    ret = http_get(hdl, "system", "Module=Query&Type=DM6xResolution;SystemVersion", &dicts);
    CHECK_RET(ret == 0, ret, "GET Query failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
    
    for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "DM6xResolution") == 0){
            sscanf(dicts.dict[i].value, "%d*%d", &info->ir_w, &info->ir_h);
        }else if(strcmp(dicts.dict[i].key, "SystemVersion") == 0){
            strcpy(info->dev_version, dicts.dict[i].value);
        }else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }

    return DM_SUCCESS;
}



static int get_sys_info2(long hdl, DM_SYS_INFO *info)
{
    int i, ret;
    DM_DICTS dicts;
    
    ret = http_get(hdl, "system", "Module=System", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET System failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
    
    for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "Output") == 0){
            info->output = atoi(dicts.dict[i].value);
        }else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }
    
    return DM_SUCCESS;
}



static int get_sys_info3(long hdl, DM_SYS_INFO *info)
{
    int i, ret;
    DM_DICTS dicts;
    
    ret = http_get(hdl, "system", "Module=Codec", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET Codec failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
    
    for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "MajorSize") == 0){
            sscanf(dicts.dict[i].value, "%dx%d", &info->coor_w, &info->coor_h);

            if(info->coor_w < VGA_WIDTH || info->coor_w > D1_WIDTH){
                info->coor_w = D1_WIDTH;
            }
            
            if(info->coor_h < VGA_HEIGHT || info->coor_h > D1_HEIGHT){
                info->coor_w = D1_HEIGHT;
            }
        }else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }
    
    return DM_SUCCESS;
}
/********************************************************************************/
static int get_user_info1(long hdl, DM_USER_MANAGE *info)
{
	int i,ret;
    DM_DICTS dicts;

	ret = http_get(hdl, "user", "Operation=Query", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET Query failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");

	strcat(info->user_info, "Name=");
	for(i=0; i<dicts.count; i++){  
        strcat(info->user_info, dicts.dict[i].value);
		if(i == dicts.count - 1)
			break;

		strcat(info->user_info, "    Type=");
        dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
    }

    return DM_SUCCESS;
}

static int get_user_info2(long hdl, DM_USER_MANAGE *info)
{
	int i,ret;
	char b[128],c[20],d[20];
    DM_DICTS dicts;

	printf("Name=");
	scanf("%s",c);
	printf("Pass=");
	scanf("%s",d);

	sprintf(b,"Operation=Login&Name=%s&Pass=%s",c,d);
	ret = http_get(hdl, "user", b, &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET Login failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
 
	strcat(info->ret_login, "Type=");
    strcat(info->ret_login, dicts.dict[0].value);
	strcat(info->ret_login, "   Version=");
	strcat(info->ret_login, dicts.dict[1].value);
		
    dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);

	return DM_SUCCESS;
}

static int get_user_info3(long hdl, DM_USER_MANAGE *info)
{
	int i,ret;
	char b[128],c[20],d[20],e[6];
    DM_DICTS dicts;

	printf("Name=");
	scanf("%s",c);
	printf("Pass=");
	scanf("%s",d);
	printf("(Admin or User)Type=");
	scanf("%s",e);

	sprintf(b,"Operation=Add&Name=%s&Pass=%s&Type=%s",c,d,e);
	ret = http_get(hdl, "user", b, &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET Add User failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
		
    dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);

	return DM_SUCCESS;
}

static int get_user_info4(long hdl, DM_USER_MANAGE *info)
{
	int i,ret;
	char b[128],c[20];
	DM_DICTS dicts;
	
	printf("Name=");
	scanf("%s",c);
	
	sprintf(b,"Operation=Del&Name=%s",c);
	ret = http_get(hdl, "user", b, &dicts);
	CHECK_RET(ret == DM_SUCCESS, ret, "GET Delete User failed");
	
	CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
			
	dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
	
	return DM_SUCCESS;
}

static int get_user_info5(long hdl, DM_USER_MANAGE *info)
{
	int i,ret;
	char b[256],c[20],d[20],e[20];
    DM_DICTS dicts;
	//admin login
	ret = http_get(hdl, "user", "Operation=Login&Name=admin&Pass=Admin123", &dicts);
	CHECK_RET(ret == DM_SUCCESS, ret, "GET Login failed");
	
	CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");

	printf("Name=");
	scanf("%s",c);
	printf("Pass=");
	scanf("%s",d);
	printf("NewPass=");
	scanf("%s",e);

	sprintf(b,"Operation=ChgPass&Name=%s&Pass=%s&NewPass=%s&Type=Admin",c,d,e);
	ret = http_post(hdl, "user", b, &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET change password failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
		
    dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);

	return DM_SUCCESS;
}

static int get_net_info1(long hdl, DM_NET_SET *info)
{
	int i, ret;
    DM_DICTS dicts;

    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");

    ret = http_get(hdl, "system", "Module=Network", &dicts);
    CHECK_RET(ret == 0, ret, "GET tcp/ip failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
    
    for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "IPAddr") == 0){
            strcpy(info->tcp_ip.ip_addr, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "Netmask") == 0){
            strcpy(info->tcp_ip.netmask, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "Gateway") == 0){
        	strcpy(info->tcp_ip.gateway, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "MACAddr") == 0){
        	strcpy(info->tcp_ip.mac_addr, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "DNS") == 0){
        	strcpy(info->tcp_ip.dns, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "DNS2") == 0){
        	strcpy(info->tcp_ip.dns2, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "DHCP") == 0){
        	strcpy(info->tcp_ip.dhcp, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "HttpPort") == 0){
        	info->tcp_ip.http_port = atoi(dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "RtspPort") == 0){
        	info->tcp_ip.rtsp_port = atoi(dicts.dict[i].value);
		}else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }

    return DM_SUCCESS;
}

static int get_net_info2(long hdl, DM_NET_SET *info)
{
	int i, ret;
    DM_DICTS dicts;

    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");

    ret = http_get(hdl, "system", "Module=FTP", &dicts);
    CHECK_RET(ret == 0, ret, "GET tcp/ip failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
    
    for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "FtpEnable") == 0){
            strcpy(info->ftp_d.enable, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "FtpServer") == 0){
            strcpy(info->ftp_d.server_ip, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "FtpPort") == 0){
        	info->ftp_d.port = atoi(dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "UserName") == 0){
        	strcpy(info->ftp_d.user_name, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "Password") == 0){
        	strcpy(info->ftp_d.password, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "StoreDirectory") == 0){
        	strcpy(info->ftp_d.store_dir, dicts.dict[i].value);
		}else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }

    return DM_SUCCESS;
}

static int get_net_info3(long hdl, DM_NET_SET *info)
{
	int i, ret;
    DM_DICTS dicts;

    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");

    ret = http_get(hdl, "system", "Module=SMTP", &dicts);
    CHECK_RET(ret == 0, ret, "GET mail failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");

	for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "SmtpEnable") == 0){
            strcpy(info->mail_d.enable, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "SmtpServer") == 0){
            strcpy(info->mail_d.addr, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "SmtpPort") == 0){
        	info->mail_d.port = atoi(dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "UserName") == 0){
        	strcpy(info->mail_d.username, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "Password") == 0){
        	strcpy(info->mail_d.password, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "FromAddr") == 0){
        	strcpy(info->mail_d.fromaddr, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "ReceiptAddr1") == 0){
        	strcpy(info->mail_d.receiptaddr1, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "ReceiptAddr2") == 0){
        	strcpy(info->mail_d.receiptaddr2, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "ReceiptAddr3") == 0){
        	strcpy(info->mail_d.receiptaddr3, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "ReceiptAddr4") == 0){
        	strcpy(info->mail_d.receiptaddr4, dicts.dict[i].value);
		}else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
	}

    return DM_SUCCESS;
}

static int get_net_info4(long hdl, DM_NET_SET *info)
{
	int i, ret;
    DM_DICTS dicts;

    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");

    ret = http_get(hdl, "system", "Module=gb28181", &dicts);
    CHECK_RET(ret == 0, ret, "GET GB28181 failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");

	for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "Enable") == 0){
            strcpy(info->gb_d.enable, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "ServerID") == 0){
            strcpy(info->gb_d.id_server, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "ServerDomain") == 0){
        	strcpy(info->gb_d.domain_server, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "ServerIP") == 0){
        	strcpy(info->gb_d.ip_server, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "ServerPort") == 0){
        	info->gb_d.port_server = atoi(dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "AuthID") == 0){
        	strcpy(info->gb_d.auth_id, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "Password") == 0){
        	strcpy(info->gb_d.password, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "LocalPort") == 0){
        	info->gb_d.port_local = atoi(dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "Expires") == 0){
        	info->gb_d.expires = atoi(dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "HeartBeatTime") == 0){
        	info->gb_d.hbtime = atoi(dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "HeartBeatCount") == 0){
        	info->gb_d.hbcount = atoi(dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "RegisterStatus") == 0){
        	strcpy(info->gb_d.rstatus, dicts.dict[i].value);
		}else if(strcmp(dicts.dict[i].key, "VideoID") == 0){
        	strcpy(info->gb_d.video_id, dicts.dict[i].value);
		}else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
	}

    return DM_SUCCESS;
}

/********************************************************************************/
static int fill_alarm_info(long hdl, DM_ALARM_INFO *info_alarm)
{
    CHECK_RET(info_alarm != NULL, ERR_INVALID_PARAM, "input is NULL");
    
    int ret;
    
    DM_TEMP_INFO info_temp;
    
    ret = DM_Get_Temp_Info(hdl, &info_temp);
    CHECK_RET(ret == DM_SUCCESS, ret, "DM_Get_Temp_Info failed");
    
    DM_TEMP_ACONFIG config;
    
    ret = DM_Get_Temp_AConfig(hdl, &config);
    CHECK_RET(ret == DM_SUCCESS, ret, "DM_Get_Temp_AConfig failed");

    float refer = config.temp_thrld;

    int i;
    TEMP_PONIT *point;
    TEMP_AREA *area;
    DM_ALARM_CELL *cell;    
    
    for(i=0; i<MAX_TMEP_NUM; i++){
        point = &info_temp.point[i];
        cell = &info_alarm->point[i];

        if(point->enable == 0){
            continue;
        }
        
        cell->temp_alarm = point->value;
        cell->temp_refer = refer;
        cell->status = (cell->temp_alarm > cell->temp_refer)?1:0;
        cell->pos = point->pos;        
    }

    for(i=0; i<MAX_TMEP_NUM; i++){
        area = &info_temp.area[i];
        cell = &info_alarm->area[i];

        if(area->enable == 0){
            continue;
        }
        
        cell->temp_alarm = area->max;
        cell->temp_refer = refer;
        cell->status = (cell->temp_alarm > cell->temp_refer)?1:0;
        cell->pos = area->pos;
    }

    return DM_SUCCESS;
}



#if 0
#endif



int DM_Get_Sys_Info(long hdl, DM_SYS_INFO *info)
{
    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");

    memset((char*)info, 0x00, sizeof(DM_SYS_INFO));

    get_sys_info1(hdl, info);
    get_sys_info2(hdl, info);
    get_sys_info3(hdl, info);

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}

int DM_Get_User_info(long hdl, DM_USER_MANAGE *info, DM_ENABLE *en)
{
	dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");

    memset((char*)info, 0x00, sizeof(DM_USER_MANAGE));
	
	get_user_info1(hdl, info);		//user_query
	if(en->enable_login == 1)
	{
		get_user_info2(hdl, info);			//user_login
	}
	if(en->enable_add == 1)
	{
		get_user_info3(hdl, info);			//user_add
	}
	if(en->enable_del == 1)
	{
		get_user_info4(hdl, info);			//user_del
	}
	if(en->enable_chpass == 1)
	{
		get_user_info5(hdl, info);			//user_chpass
	}
	
	dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}

int DM_Get_Net_info(long hdl, DM_NET_SET *info, DM_ENABLE *en)
{
	dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");

    memset((char*)info, 0x00, sizeof(DM_NET_SET));

	get_net_info1(hdl, info);		//tcp/ip
	get_net_info2(hdl, info);		//ftp
	get_net_info3(hdl,info);		//mail
	get_net_info4(hdl,info);		//gb28181

	
	dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}

int DM_Set_Osd_Cam(long hdl, DM_OSD_CAM *osd)
{
    int ret, cnt;
    DM_POS pos;
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(osd != NULL, ERR_INVALID_PARAM, "input is NULL");
    
    ret = dm_pos_check(hdl, COOR_DIR_H2N, osd->pos.x, osd->pos.y);
    CHECK_RET(ret == DM_SUCCESS, ret, "invalid param");

    cnt = 0;
    memset(&dicts, 0x00, sizeof(DM_DICTS));
    
    sprintf(dicts.dict[cnt].key, "CameraName");
    memcpy(dicts.dict[cnt].value, osd->name, sizeof(osd->name));
    cnt++;

    sprintf(dicts.dict[cnt].key, "CameraNameDisplay");
    sprintf(dicts.dict[cnt].value, "%s", osd->show?"On":"Off");
    cnt++;

    pos = osd->pos;    
    dm_pos_adjust(hdl, COOR_T_OSD, COOR_DIR_H2N, &pos.x, &pos.y);
    sprintf(dicts.dict[cnt].key, "CameraNamePos");
    sprintf(dicts.dict[cnt].value, "%d,%d", pos.x, pos.y);
    cnt++;

    dicts.count = cnt;
    
    ret = http_post(hdl, "system", "Module=System", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "POST System failed");

    dm_log(3, "%s, success\r\n", __FUNCTION__);

    return DM_SUCCESS;
}



int DM_Get_Osd_Cam(long hdl, DM_OSD_CAM *osd)
{
    int i, ret;
    DM_POS pos;
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(osd != NULL, ERR_INVALID_PARAM, "input is NULL");
    
    ret = http_get(hdl, "system", "Module=System", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET System failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
    
    for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "CameraNameDisplay") == 0){
            osd->show = para_enable(dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "CameraName") == 0){
            strcpy(osd->name, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "CameraNamePos") == 0){
            sscanf(dicts.dict[i].value, "%d,%d", &pos.x, &pos.y);
            dm_pos_adjust(hdl, COOR_T_OSD, COOR_DIR_N2H, &pos.x, &pos.y);
            osd->pos = pos;
        }else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }

    dm_log(3, "%s, success\r\n", __FUNCTION__);

    return DM_SUCCESS;
}



int DM_Set_Osd_Udf(long hdl, DM_OSD_UDF *osd)
{
    int ret, cnt;
    DM_POS pos;
    DM_DICTS dicts;
    

    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(osd != NULL, ERR_INVALID_PARAM, "input is NULL");
    
    ret = dm_pos_check(hdl, COOR_DIR_H2N, osd->pos.x, osd->pos.y);
    CHECK_RET(ret == DM_SUCCESS, ret, "invalid param");

    cnt = 0;
    memset(&dicts, 0x00, sizeof(DM_DICTS));
    
    sprintf(dicts.dict[cnt].key, "UserDefineFieldsN");
    sprintf(dicts.dict[cnt].value, "%d", 1);
    cnt++;

    sprintf(dicts.dict[cnt].key, "UserDefineFields");
    memcpy(dicts.dict[cnt].value, osd->name, sizeof(osd->name));
    cnt++;

    sprintf(dicts.dict[cnt].key, "UserDefineFieldsDisplay");
    sprintf(dicts.dict[cnt].value, "%s", osd->show?"On":"Off");
    cnt++;

    pos = osd->pos;    
    dm_pos_adjust(hdl, COOR_T_OSD, COOR_DIR_H2N, &pos.x, &pos.y);
    sprintf(dicts.dict[cnt].key, "UserDefineFieldsPos");
    sprintf(dicts.dict[cnt].value, "%d,%d", pos.x, pos.y);
    cnt++;

    dicts.count = cnt;
    
    ret = http_post(hdl, "system", "Module=System", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "POST System failed");

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



int DM_Get_Osd_Udf(long hdl, DM_OSD_UDF *osd)
{
    int i, ret;
    DM_POS pos;
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(osd != NULL, ERR_INVALID_PARAM, "input is NULL");
    
    ret = http_get(hdl, "system", "Module=System", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET System failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
    
    for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "UserDefineFieldsDisplay") == 0){
            osd->show = para_enable(dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "UserDefineFields") == 0){
            strcpy(osd->name, dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "UserDefineFieldsPos") == 0){
            sscanf(dicts.dict[i].value, "%d,%d", &pos.x, &pos.y);
            dm_pos_adjust(hdl, COOR_T_OSD, COOR_DIR_N2H, &pos.x, &pos.y);
            osd->pos = pos;
        }else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}




int DM_Set_Osd_Time(long hdl, DM_OSD_TIME *osd)
{
    int ret, cnt;
    DM_POS pos;
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(osd != NULL, ERR_INVALID_PARAM, "input is NULL");
    
    ret = dm_pos_check(hdl, COOR_DIR_H2N, osd->pos.x, osd->pos.y);
    CHECK_RET(ret == DM_SUCCESS, ret, "invalid param");

    cnt = 0;
    memset(&dicts, 0x00, sizeof(DM_DICTS));

    sprintf(dicts.dict[cnt].key, "SystemDateDisplay");
    sprintf(dicts.dict[cnt].value, "%s", osd->show?"On":"Off");
    cnt++;
    
    pos = osd->pos;    
    dm_pos_adjust(hdl, COOR_T_OSD, COOR_DIR_H2N, &pos.x, &pos.y);
    sprintf(dicts.dict[cnt].key, "SystemDatePos");
    sprintf(dicts.dict[cnt].value, "%d,%d", pos.x, pos.y);
    cnt++;

    dicts.count = cnt;
    
    ret = http_post(hdl, "system", "Module=System", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "POST System failed");

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



int DM_Get_Osd_Time(long hdl, DM_OSD_TIME *osd)
{
    int i, ret;
    DM_POS pos;
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(osd != NULL, ERR_INVALID_PARAM, "input is NULL");
    
    ret = http_get(hdl, "system", "Module=System", &dicts);
    CHECK_RET(ret == 0, ret, "GET System failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
    
    for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "SystemDateDisplay") == 0){
            osd->show = para_enable(dicts.dict[i].value);
        }else if(strcmp(dicts.dict[i].key, "SystemDatePos") == 0){
            sscanf(dicts.dict[i].value, "%d,%d", &pos.x, &pos.y);
            dm_pos_adjust(hdl, COOR_T_OSD, COOR_DIR_N2H, &pos.x, &pos.y);
            osd->pos = pos;
        }else{
            dm_log(5, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



/* 告警信息查询
** 返回值: 
**/
int DM_Get_Alarm_Info(long hdl, DM_ALARM_INFO *info)
{
    int ret;
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);

    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");
    memset(info, 0x00, sizeof(DM_ALARM_INFO));
    
    ret = http_get(hdl, "system", "Module=AlarmInquiry", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET AlarmInquiry failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");

    if(strcmp(dicts.dict[0].value, "Yes") == 0){
        info->status = 1;
        
        ret = fill_alarm_info(hdl, info);
        CHECK_RET(ret == DM_SUCCESS, ret, "fill_alarm_info failed");
    }else{
        info->status = 0;
    }

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



/* 获取测温温度
** 返回值: 
**/
int DM_Get_Temp_Info(long hdl, DM_TEMP_INFO *info)
{
    int i, ret;
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(info != NULL, ERR_INVALID_PARAM, "input is NULL");
    memset(info, 0x00, sizeof(DM_TEMP_INFO));
    
    ret = http_get(hdl, "system", "Module=DMTtl", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET DMTtl failed");

    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");

    for(i=0; i<dicts.count; i++){
        if(strcmp(dicts.dict[i].key, "Point") == 0){
            temp_point(hdl, dicts.dict[i].value, info->point);
        }else if(strcmp(dicts.dict[i].key, "Area") == 0){
            temp_area(hdl, dicts.dict[i].value, info->area);
        }else{
            dm_log(4, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



/* 设置测温配置
** 返回值: 
**/
int DM_Set_Temp_MConfig(long hdl, DM_TEMP_MCONFIG *measure)
{
    int ret;
    char value[64];
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(measure != NULL, ERR_INVALID_PARAM, "input is NULL");

    ret = dm_pos_check(hdl, COOR_DIR_H2N, measure->start_x, measure->start_y);
    CHECK_RET(ret == DM_SUCCESS, ret, "invalid param");

    if(measure->type == DM_MEASURE_AREA){
        ret = dm_pos_check(hdl, COOR_DIR_H2N, measure->end_x, measure->end_y);
        CHECK_RET(ret == DM_SUCCESS, ret, "invalid param");
    }

    ret = temp_measure_set(hdl, &dicts, measure);
    CHECK_RET(ret == DM_SUCCESS, ret, "invalid param");

    snprintf(value, sizeof(value), "Module=DMMeasure&Type=%s", 
        (measure->type==DM_MEASURE_POINT)?"Point":"Area");

    ret = http_post(hdl, "system", value, &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "POST DMMeasure failed");

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



/* 获取测温配置
** 返回值: 成功返回0， 失败返回-1
**/
int DM_Get_Temp_MConfig(long hdl, DM_TEMP_MCONFIG *measure)
{
    int ret;
    char value[64];
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);

    CHECK_RET(measure != NULL, ERR_INVALID_PARAM, "input is NULL");

    snprintf(value, sizeof(value), "Module=DMMeasure&Type=%s&Index=%d", 
        (measure->type==DM_MEASURE_POINT)?"Point":"Area", measure->idx);
    
    ret = http_get(hdl, "system", value, &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET DMMeasure failed");
    
    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");

    temp_measure_get(hdl, &dicts, measure);

    dm_log(4, "%s, idx=%d, type=%d, enable=%d, start_x=%d, start_y=%d, end_x=%d, end_y=%d\n", 
        __FUNCTION__, measure->idx, measure->type, measure->enable, 
        measure->start_x, measure->start_y, measure->end_x, measure->end_y);

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



int DM_Set_Temp_AConfig(long hdl, DM_TEMP_ACONFIG *alarm)
{
    int ret, cnt;
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(alarm != NULL, ERR_INVALID_PARAM, "input is NULL");

    cnt = 0;
    memset(&dicts, 0x00, sizeof(DM_DICTS));

    sprintf(dicts.dict[cnt].key, "EnableAlarm");
    sprintf(dicts.dict[cnt].value, "%s", alarm->enable?"On":"Off");
    cnt++;
    
    sprintf(dicts.dict[cnt].key, "AlarmTemp100");
    sprintf(dicts.dict[cnt].value, "%.1f", alarm->temp_thrld);
    cnt++;
    
    sprintf(dicts.dict[cnt].key, "RecTime");
    sprintf(dicts.dict[cnt].value, "%d", alarm->rec_time);
    cnt++;
    
    sprintf(dicts.dict[cnt].key, "PreRecordTime");
    sprintf(dicts.dict[cnt].value, "%d", alarm->prerec_time);
    cnt++;
    
    sprintf(dicts.dict[cnt].key, "MailContentType");
    sprintf(dicts.dict[cnt].value, "%d", (alarm->email_type==DM_EMAIL_TYPE_TXT)?1:2);
    cnt++;

    sprintf(dicts.dict[cnt].key, "AlarmLinkOutInfo");
    ret = 0;
    if(alarm->rec_en){
        strcat(dicts.dict[cnt].value, "Rec");
        ret++;
    }
    
    if(alarm->email_en){
        if(ret){
            strcat(dicts.dict[cnt].value, "|");
        }
        
        strcat(dicts.dict[cnt].value, "EMail");
        ret++;
    }
    
    if(alarm->ftp_en){
        if(ret){
            strcat(dicts.dict[cnt].value, "|");
        }
        
        strcat(dicts.dict[cnt].value, "FTP");
        ret++;
    }
    cnt++;

    dicts.count = cnt;

    ret = http_post(hdl, "system", "Module=AlarmCfg", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "POST System failed");

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}



int DM_Get_Temp_AConfig(long hdl, DM_TEMP_ACONFIG *alarm)
{
    int i, ret;
    DM_DICT *dict;
    DM_DICTS dicts;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    CHECK_RET(alarm != NULL, ERR_INVALID_PARAM, "input is NULL");
    
    ret = http_get(hdl, "system", "Module=AlarmCfg", &dicts);
    CHECK_RET(ret == DM_SUCCESS, ret, "GET AlarmCfg failed");
    
    CHECK_RET(dicts.count > 0, ERR_NOT_SUPPORT, "rsp is wrong");
    
    for(i=0; i<dicts.count; i++){
        dict = &dicts.dict[i];
        
        if(strcmp(dict->key, "EnableAlarm") == 0){
            alarm->enable = para_enable(dict->value);
        }else if(strcmp(dict->key, "AlarmTemp100") == 0){
            sscanf(dict->value, "%f", &alarm->temp_thrld);
        }else if(strcmp(dict->key, "RecTime") == 0){
            alarm->rec_time = atoi(dict->value);
        }else if(strcmp(dict->key, "PreRecordTime") == 0){
            alarm->prerec_time = atoi(dict->value);
        }else if(strcmp(dict->key, "MailContentType") == 0){
            alarm->email_type = (atoi(dict->value) > 1)?1:0;
        }else if(strcmp(dict->key, "AlarmLinkOutInfo") == 0){
            alarm->rec_en = (strstr(dict->value, "Rec") != NULL)?1:0;
            alarm->email_en = (strstr(dict->value, "EMail") != NULL)?1:0;
            alarm->ftp_en = (strstr(dict->value, "FTP") != NULL)?1:0;
        }else{
            dm_log(4, "%s, key=%s not match\n", __FUNCTION__, dicts.dict[i].key);
        }
    }

    dm_log(4, "%s, enable=%d, temp_thrld=%.1f, rec_en=%d, email_en=%d, ftp_en=%d, rec_time=%d, prerec_time=%d\n", 
        __FUNCTION__, alarm->enable, alarm->temp_thrld, alarm->rec_en, alarm->email_en, alarm->ftp_en,
        alarm->rec_time, alarm->prerec_time);

    dm_log(3, "%s, success\r\n", __FUNCTION__);
    
    return DM_SUCCESS;
}




int DM_Get_Jpeg(long hdl, int ch, const char *name, char *buf, int *len)
{
    int ret, cnt, r_size;


    dm_log(3, "%s, <In>\r\n", __FUNCTION__);
    
    cnt = 0;
    r_size = 0;
    
    while(cnt < 20){
        ret = http_get_jpeg(hdl, ch, name, buf, len, &r_size);
        if(ret == DM_SUCCESS){
            if(r_size <= 0){
                dm_log(2, "%s, try=%d, error unknow\n", __FUNCTION__, cnt);
                return ERR_UNKNOWN;
            }else{
                dm_log(3, "%s, success\r\n", __FUNCTION__);
                return DM_SUCCESS;
            }
        }else{
            cnt++;
            dm_log(4, "%s, try=%d\n", __FUNCTION__, cnt);
            //usleep(10000);
        }
    }

    dm_log(2, "%s, failed, ret=%d\r\n", __FUNCTION__, ret);
    
    return ret;
}



