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

#ifndef	__H_LINKED_LIST_H__
#define __H_LINKED_LIST_H__

/************************************************************************************/
typedef struct LINKED_NODE
{
	struct LINKED_NODE * p_next;
	struct LINKED_NODE * p_previous;
	void* p_data;
} LINKED_NODE;

/************************************************************************************/
typedef struct LINKED_LIST
{
	LINKED_NODE *		p_first_node;
	LINKED_NODE *		p_last_node;
	void	*			list_semMutex;
} LINKED_LIST;


#ifdef __cplusplus
extern "C" {
#endif

ONVIF_API LINKED_LIST * h_list_create (BOOL bNeedMutex);
ONVIF_API void          h_list_free_container (LINKED_LIST* p_linked_list);
ONVIF_API void          h_list_free_all_node (LINKED_LIST* p_linked_list);

ONVIF_API void          get_ownership(LINKED_LIST* p_linked_list);
ONVIF_API void          giveup_ownership(LINKED_LIST* p_linked_list);

ONVIF_API BOOL          h_list_remove(LINKED_LIST* p_linked_list,LINKED_NODE * p_node);
ONVIF_API BOOL          h_list_remove_data(LINKED_LIST* p_linked_list,void * p_data);

ONVIF_API void          h_list_remove_from_front (LINKED_LIST* p_linked_list);
ONVIF_API void          h_list_remove_from_front_no_lock (LINKED_LIST* p_linked_list);
ONVIF_API void          h_list_remove_from_back (LINKED_LIST* p_linked_list);

ONVIF_API BOOL          h_list_add_at_front (LINKED_LIST* p_linked_list, void* p_item);
ONVIF_API BOOL          h_list_add_at_back (LINKED_LIST* p_linked_list, void* p_item);

ONVIF_API uint32        h_list_get_number_of_nodes (LINKED_LIST* p_linked_list);

ONVIF_API BOOL          h_list_insert(LINKED_LIST * p_linked_list,LINKED_NODE * p_pre_node,void *p_item);

ONVIF_API LINKED_NODE * h_list_lookup_start(LINKED_LIST * p_linked_list);
ONVIF_API LINKED_NODE * h_list_lookup_next(LINKED_LIST * p_linked_list, LINKED_NODE * p_node);
ONVIF_API void          h_list_lookup_end(LINKED_LIST * p_linked_list);

ONVIF_API BOOL          h_list_is_empty(LINKED_LIST* p_list);
ONVIF_API LINKED_NODE * h_list_get_from_front(LINKED_LIST * p_list);
ONVIF_API LINKED_NODE * h_list_get_from_back(LINKED_LIST * p_list);

#ifdef __cplusplus
}
#endif

#endif // __H_LINKED_LIST_H__


