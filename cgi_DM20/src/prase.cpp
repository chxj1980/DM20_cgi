/*
** system
** time   : 2018-10-23
** author : chenks (godence@126.com)
*/
#include <stdio.h>
#include "HTTPClient.h"
#include "dmtype.h"





/* 
** para_delimiter: 不同参数分隔符，比如'&'
**  val_delimiter: 具体成员分隔符，比如'='
**/
static char* get_name_value(char *src, char *name, char *value, char para_delimiter, char val_delimiter)
{
    int count;

    count = 0;
    while(*src != 0){
        if(*src == val_delimiter){
            src++;
            break;
        }
        
        if(*src == para_delimiter){
            *name = 0;
            *value = 0;
            return ++src;
        }
        
        if(count++ < MAX_NAME_LEN){
            *name++ = *src;
        }
        
        src++;
    }

    
    *name = 0;
    count = 0;
    
    while(*src != 0){
        if(*src == para_delimiter){
            src++;
            break;
        }
        
        if(count++ < MAX_DATA_LEN){
            *value++ = *src;
        }
        
        src++;
    }

    *value = 0;
    
    return src;
}



/* 解析字符串
** 例如收到字符串: Point=0|0.0@1|0.0@2|0.0@3|0.0@4|0.0@5|0.0&Area=0|66.1,58.2,58.8@1|0.0,0.0,0.0@2|0.0,0.0,0.0@3|0.0,0.0,0.0@4|0.0,0.0,0.0@5|0.0,0.0,0.0
** 解析后的内容为: key=Point; value=0|0.0@1|0.0@2|0.0@3|0.0@4|0.0@5|0.0
**                 key=Area; value=0|66.1,58.2,58.8@1|0.0,0.0,0.0@2|0.0,0.0,0.0@3|0.0,0.0,0.0@4|0.0,0.0,0.0@5|0.0,0.0,0.0
**/
int prase_dicts(char *buf, char para_delimiter, char val_delimiter, DM_DICTS *dicts)
{
    char *ptr;
    DM_DICT *dict;
    

    dicts->count = 0;
    ptr = buf;			//
    
    while(*ptr != 0){
        dict = &dicts->dict[dicts->count];		//
        
        ptr = get_name_value(ptr, dict->key, dict->value, para_delimiter, val_delimiter);
        
        dicts->count++;
    }

#if 1    
    int i;

    for(i=0; i<dicts->count; i++){
        dm_log(4, "%s, key=%s, value=%s\r\n", __FUNCTION__, dicts->dict[i].key, dicts->dict[i].value);
    }    
#endif

    return 0;
}



/* 从一个字典里面，根据key获取value
** 输入: dicts
** 输入: key
** 输出: value
** 返回: 成功返回0，失败返回-1
**/
int fetch_dicts(DM_DICTS *dicts, char *key, char*value)
{
    int i;
    
    CHECK_RET(dicts !=  NULL, -1, "dicts is NULL");
    CHECK_RET(key !=  NULL, -1, "key is NULL");
    CHECK_RET(value !=  NULL, -1, "value is NULL");

    for(i=0; i<dicts->count; i++){
        if(strcmp(dicts->dict[i].key, key) == 0){
            strcpy(value, dicts->dict[i].value);
            return 0;
        }
    }
    
    return -1;
}



