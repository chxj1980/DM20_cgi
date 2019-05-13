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
#include "sys_inc.h"
#include "mpeg4_rtp_rx.h"


static BOOL getNibble(char const*& configStr, unsigned char& resultNibble) 
{
    char c = configStr[0];
    if (c == '\0') return FALSE; // we've reached the end

    if (c >= '0' && c <= '9') 
    {
        resultNibble = c - '0';
    } 
    else if (c >= 'A' && c <= 'F') 
    {
        resultNibble = 10 + c - 'A';
    } 
    else if (c >= 'a' && c <= 'f') 
    {
        resultNibble = 10 + c - 'a';
    } 
    else 
    {
        return FALSE;
    }

    ++configStr; // move to the next nibble
    return TRUE;
}

static BOOL getByte(char const*& configStr, unsigned char& resultByte) 
{
    resultByte = 0; // by default, in case parsing fails

    unsigned char firstNibble;
    if (!getNibble(configStr, firstNibble)) return FALSE;
    resultByte = firstNibble<<4;

    unsigned char secondNibble = 0;
    if (!getNibble(configStr, secondNibble) && configStr[0] != '\0') 
    {
        // There's a second nibble, but it's malformed
        return FALSE;
    }
    resultByte |= secondNibble;

    return TRUE;
}

uint8 * parseGeneralConfigStr(char const* configStr, unsigned& configSize) 
{
    unsigned char* config = NULL;
    do 
    {
        if (configStr == NULL) break;
        configSize = (strlen(configStr)+1)/2;

        config = new unsigned char[configSize];
        if (config == NULL) break;

        unsigned i;
        for (i = 0; i < configSize; ++i) 
        {
            if (!getByte(configStr, config[i])) break;
        }
        if (i != configSize) break; // part of the string was bad

        return config;
    } while (0);

    configSize = 0;
    delete[] config;
    return NULL;
}



