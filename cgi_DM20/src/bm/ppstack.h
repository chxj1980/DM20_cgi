/***************************************************************************************
 *
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 *
 *  By downloading, copying, installing or using the software you agree to this license.
 *  If you do not agree to this license, do not download, install, 
 *  copy or use the software.
 *
 *  Copyright (C) 2010-2016, Happytimesoft Corporation, all rights reserved.
 *
 *  Redistribution and use in binary forms, with or without modification, are permitted.
 *
 *  Unless required by applicable law or agreed to in writing, software distributed 
 *  under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 *  CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
 *  language governing permissions and limitations under the License.
 *
****************************************************************************************/

#ifndef	__H_PPSTACK_H__
#define __H_PPSTACK_H__


/***************************************************************************************/

typedef struct PPSN	//ppstack_node
{
	unsigned long		prev_node;
	unsigned long		next_node;
	unsigned long		node_flag;	//0:idle£»1:in FreeList 2:in UsedList 
}PPSN;

typedef struct PPSN_CTX
{
	char *		        fl_base;	
	unsigned int		head_node;	
	unsigned int		tail_node;
	unsigned int		node_num;
	unsigned int		low_offset;
	unsigned int		high_offset;
	unsigned int		unit_size;
	void	*			ctx_mutex;
	unsigned int		pop_cnt;
	unsigned int		push_cnt;
}PPSN_CTX;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************/

ONVIF_API PPSN_CTX* pps_ctx_fl_init(unsigned long node_num,unsigned long content_size,BOOL bNeedMutex);
ONVIF_API PPSN_CTX* pps_ctx_fl_init_assign(char *  mem_addr, unsigned long mem_len, unsigned long node_num, unsigned long content_size, BOOL bNeedMutex);

ONVIF_API void 		pps_fl_free(PPSN_CTX * fl_ctx);
ONVIF_API void 		pps_fl_reinit(PPSN_CTX * fl_ctx);

ONVIF_API BOOL 		ppstack_fl_push(PPSN_CTX * pps_ctx,void * content_ptr);
ONVIF_API BOOL 		ppstack_fl_push_tail(PPSN_CTX * pps_ctx,void * content_ptr);
ONVIF_API void 	  * ppstack_fl_pop(PPSN_CTX * pps_ctx);

ONVIF_API PPSN_CTX* pps_ctx_ul_init(PPSN_CTX * fl_ctx,BOOL bNeedMutex);
ONVIF_API BOOL 		pps_ctx_ul_init_assign(PPSN_CTX * ul_ctx, PPSN_CTX * fl_ctx,BOOL bNeedMutex);
ONVIF_API BOOL 		pps_ctx_ul_init_nm(PPSN_CTX * fl_ctx,PPSN_CTX * ul_ctx);

ONVIF_API void 		pps_ul_reinit(PPSN_CTX * ul_ctx);
ONVIF_API void 		pps_ul_free(PPSN_CTX * ul_ctx);

ONVIF_API BOOL 		pps_ctx_ul_del(PPSN_CTX * ul_ctx,void * content_ptr);

ONVIF_API PPSN 	  * pps_ctx_ul_del_node_unlock(PPSN_CTX * ul_ctx,PPSN * p_node);
ONVIF_API void 	  * pps_ctx_ul_del_unlock(PPSN_CTX * ul_ctx,void * content_ptr);

ONVIF_API BOOL 		pps_ctx_ul_add(PPSN_CTX * ul_ctx,void * content_ptr);
ONVIF_API BOOL 		pps_ctx_ul_add_head(PPSN_CTX * ul_ctx,void * content_ptr);

ONVIF_API uint32    pps_get_index(PPSN_CTX * pps_ctx,void * content_ptr);
ONVIF_API void 	  * pps_get_node_by_index(PPSN_CTX * pps_ctx,unsigned long index);

/***************************************************************************************/
ONVIF_API void 	  * pps_lookup_start(PPSN_CTX * pps_ctx);
ONVIF_API void 	  * pps_lookup_next(PPSN_CTX * pps_ctx, void * ct_ptr);
ONVIF_API void		pps_lookup_end(PPSN_CTX * pps_ctx);

ONVIF_API void 	  * pps_lookback_start(PPSN_CTX * pps_ctx);
ONVIF_API void 	  * pps_lookback_next(PPSN_CTX * pps_ctx, void * ct_ptr);
ONVIF_API void 		pps_lookback_end(PPSN_CTX * pps_ctx);

ONVIF_API void 		pps_wait_mutex(PPSN_CTX * pps_ctx);
ONVIF_API void 		pps_post_mutex(PPSN_CTX * pps_ctx);

ONVIF_API BOOL 		pps_safe_node(PPSN_CTX * pps_ctx,void * content_ptr);
ONVIF_API BOOL 		pps_idle_node(PPSN_CTX * pps_ctx,void * content_ptr);
ONVIF_API BOOL 		pps_exist_node(PPSN_CTX * pps_ctx,void * content_ptr);
ONVIF_API BOOL 		pps_used_node(PPSN_CTX * pps_ctx,void * content_ptr);

/***************************************************************************************/
ONVIF_API int 		pps_node_count(PPSN_CTX * pps_ctx);
ONVIF_API void 	  * pps_get_head_node(PPSN_CTX * pps_ctx);
ONVIF_API void 	  * pps_get_tail_node(PPSN_CTX * pps_ctx);
ONVIF_API void 	  * pps_get_next_node(PPSN_CTX * pps_ctx, void * content_ptr);
ONVIF_API void	  * pps_get_prev_node(PPSN_CTX * pps_ctx, void * content_ptr);

#ifdef __cplusplus
}
#endif

#endif /* __H_PPSTACK_H__ */


