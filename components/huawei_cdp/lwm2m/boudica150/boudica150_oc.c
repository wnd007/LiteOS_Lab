/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/


//the bc95 module;

#include <at.h>
#include <boudica150_oc.h>
#include <oc_lwm2m_agent.h>

#define cn_boudica150_cmd_timeout  (2*1000)
#define cn_boudica150_try_times    (32)
#define cn_boudica150_rcvindex     "\r\n+NNMI:"
#define cn_urc_qlwevtind           "\r\n+QLWEVTIND:"
#define cn_boudica150_cachelen     128

typedef struct
{
    //initialize by the NB module init
    const char *plmn;
    const char *apn;
    const char *bands;

    //initialized by the applicaiton
    tag_oc_config_param  oc_param;

    //this is the control block
    bool_t      sndenable;
    uint8_t     sndbuf[cn_boudica150_cachelen];  //used for the receive
    uint8_t     rcvbuf[cn_boudica150_cachelen];
    //for the debug
    uint32_t   rcvlen;
    uint32_t   sndlen;
    //states for the lwm2m
    bool_t     lwm2m_observe;

}tag_boudica150_cb;
static tag_boudica150_cb   s_boudica150_oc_cb;

//bc95 common at command
static bool_t boudica150_atcmd(const char *cmd,const char *index)
{
    s32_t ret = 0;
    ret = at_command((uint8_t *)cmd,strlen(cmd),index,NULL,0,cn_boudica150_cmd_timeout);
    if(ret > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//nb command  with response
//bc95 common at command
static bool_t boudica150_atcmd_response(const char *cmd,const char *index,char *buf, int len)
{
    s32_t ret = 0;
    ret = at_command((uint8_t *)cmd,strlen(cmd),index,(uint8_t *)buf,len,cn_boudica150_cmd_timeout);
    if(ret > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}



//make a byte to 2 ascii hex
static int byte_to_hexstr(const unsigned char *bufin, int len, char *bufout)
{
    int i = 0;
    if (NULL == bufin || len <= 0 || NULL == bufout)
    {
        return -1;
    }
    for(i = 0; i < len; i++)
    {
        sprintf(bufout+i*2, "%02X", bufin[i]);
    }
    return 0;
}


static int boudica150_oc_report(uint8_t *buf,int len, int timeout)
{
    s32_t ret = -1;
    const char *cmd = "AT+NMGS=";
    const char *index = "OK";
    if ((NULL == buf) || (len >= cn_boudica150_cachelen/2 )||(false == s_boudica150_oc_cb.sndenable))
    {
        return ret;
    }
    memset(s_boudica150_oc_cb.sndbuf, 0, cn_boudica150_cachelen);
    snprintf((char *)s_boudica150_oc_cb.sndbuf,cn_boudica150_cachelen,"%s%d,",cmd,len);
    ret = byte_to_hexstr((unsigned char *)buf, len, (char *)&s_boudica150_oc_cb.sndbuf[strlen((char *)s_boudica150_oc_cb.sndbuf)]);
    s_boudica150_oc_cb.sndbuf[strlen((char *)s_boudica150_oc_cb.sndbuf)]='\r';
    ret = at_command((uint8_t *)s_boudica150_oc_cb.sndbuf,strlen((char *)s_boudica150_oc_cb.sndbuf),index,NULL,0,timeout);
    if(ret > 0)
    {
        ret = 0;
    }
    else
    {
        ret = -1;
    }

    return ret;
}


static int hexstr_to_byte(const char *bufin, int len, char *bufout)
{
    int i = 0;
    unsigned char tmp2 = 0x0;
    unsigned int tmp = 0;
    if (NULL == bufin || len <= 0 || NULL == bufout)
    {
        return -1;
    }
    for(i = 0; i < len; i = i+2)
    {
        tmp2 =  bufin[i];
        tmp2 =  tmp2 <= '9'?tmp2-0x30:tmp2-0x37;
        tmp =  bufin[i+1];
        tmp =  tmp <= '9'?tmp-0x30:tmp-0x37;
        bufout[i/2] =(tmp2<<4)|(tmp&0x0F);
    }
    return 0;
}

static s32_t boudica150_rcvdeal(uint8_t *data,s32_t len)
{
    s32_t ret = 0;
    s32_t datalen;

    char  *str;
    if(len <strlen(cn_boudica150_rcvindex))
    {
        printf("%s:invalid frame:%d byte:%s\n\r",__FUNCTION__,len,(char *)data);
        return ret;
    }
    //now deal the data
    str = strstr((char *)data,":");
    if(NULL == str)
    {
        printf("%s:format error\n\r",__FUNCTION__);
        return ret; //format error
    }
    str++;
    datalen = 0;
    for (; *str <= '9' && *str >= '0' ;str++)
    {
        datalen = (datalen * 10 + (*str - '0'));
    }
    str++;
    //now this is data of hex string
    if((uint32_t)(str + datalen*2 +2) > (uint32_t)(data + len))
    {
        printf("%s:implement error\n\r",__FUNCTION__);
        return ret; //
    }
    if(datalen > cn_boudica150_cachelen)
    {
        printf("%s:frame over: frame:%d  cachelen:%d \n\r",__FUNCTION__,datalen,cn_boudica150_cachelen);
        return ret; //
    }
    memset(s_boudica150_oc_cb.rcvbuf,0,cn_boudica150_cachelen);
    hexstr_to_byte(str,datalen*2,(char *)s_boudica150_oc_cb.rcvbuf);

    oc_lwm2m_msg_push(s_boudica150_oc_cb.rcvbuf,datalen);

    return len;
}

//enable the echooff
static bool_t boudica150_set_echo(int enable)
{
    bool_t ret ;
    char cmd[64];
    memset(cmd,0,64);
    snprintf(cmd,64,"ATE%d\r",enable);

    ret = boudica150_atcmd(cmd,"OK");

    return ret;
}

//use this function to do the module reboot
static bool_t boudica150_reboot(void)
{
    //do the module reset
    boudica150_set_echo (0);
    boudica150_set_echo (0);

    boudica150_atcmd("AT+NRB\r","REBOOTING");
    task_sleepms(10000); //wait for the module boot
    //do the module reset
    boudica150_set_echo (0);
    boudica150_set_echo (0);

    return true;
}

//enable the cfun
static bool_t boudica150_set_fun(int enable)  //unit second
{
    bool_t ret ;
    char cmd[64];

    memset(cmd,0,64);
    snprintf(cmd,64,"AT+CFUN=%d\r",enable);

    ret = boudica150_atcmd(cmd,"OK");
    //i think we should do some wait here
    task_sleepms(2000);

    return ret;
}

//use this function to set the band
static bool_t boudica150_set_bands(const char *bands)
{
    char cmd[64];
    char resp[64];
    bool_t ret = true;

    if(NULL != bands)  //which means we need to set if the default is not the same
    {
        memset(resp,0,64);
        ret = boudica150_atcmd_response("AT+NBAND?\r","OK",resp,64);

        memset(cmd,0,64);
        snprintf(cmd,63,"+NBAND:%s\r",bands);

        if((false == ret)||(NULL == strstr(resp,cmd)))//which means we need to set it
        {
            boudica150_set_fun(0);

            memset(cmd,0,64);
            snprintf(cmd,63,"AT+NBAND=%s\r",bands);
            ret = boudica150_atcmd(cmd,"OK");

            boudica150_set_fun(1);
        }
        else
        {
            ret = true;  //we need not to set
        }
    }
    return ret;
}

//set the plmn
static bool_t boudica150_set_plmn(const char *plmn)
{
    char cmd[64];
    char resp[64];
    bool_t ret;

    if(NULL != plmn)  //which measn we need to set if the default is not the same
    {
        memset(resp,0,64);
        ret = boudica150_atcmd_response("AT+COPS?\r","+COPS",resp,64);

        memset(cmd,0,64);
        snprintf(cmd,63,"+COPS:1,2,\"%s\"\r",plmn);

        if((false == ret)||(NULL == strstr(resp,cmd)))//which means we need to set it
        {
            memset(cmd,0,64);
            snprintf(cmd,63,"AT+COPS=1,2,\"%s\"\r",plmn);

            ret = boudica150_atcmd(cmd,"OK");
        }
        else
        {
            ret = true;  //we need not to set
        }
    }
    else  //set it to auto
    {
        memset(cmd,0,64);
        snprintf(cmd,64,"AT+COPS=0\r");
        ret = boudica150_atcmd(cmd,"OK");
    }

    return ret;
}

//set the apn
static bool_t boudica150_set_apn(const char *apn)
{
    char cmd[64];
    char resp[64];

    bool_t ret = true;

    if(NULL != apn)  //which measn we need to set if the default is not the same
    {
        memset(resp,0,64);
        ret = boudica150_atcmd_response("AT+CGDCONT?\r","OK",resp,64);

        memset(cmd,0,64);
        snprintf(cmd,63,"+CGDCONT:\"%s\"\r",apn); //TODO, check if it is right
        if((false == ret)||(NULL == strstr(resp,cmd)))//which means we need to set it
        {
            memset(cmd,0,64);
            snprintf(cmd,63,"AT+CGDCONT=%s\r",apn);
            ret = boudica150_atcmd(cmd,"OK");
        }
        else
        {
            ret = true;  //we need not to set
        }
    }
    return ret;
}

//set the cdp
static bool_t boudica150_set_cdp(const char *server,const char *port)
{
    char cmd[64];
    char resp[64];
    char cmp[64];
    bool_t ret = true;

    if(NULL != server)  //which measn we need to set if the default is not the same
    {
        memset(resp,0,64);
        memset(cmp,0,64);
        memset(cmd,0,64);
        ret = boudica150_atcmd_response("AT+NCDP?\r","OK",resp,64);
        snprintf(cmp,64,"+NCDP:%s,%s\r",server,port);
        if((false == ret)||(NULL == strstr(resp,cmp)))//which means we need to set it
        {
            memset(cmd,0,64);
            snprintf(cmd,64,"AT+NCDP=%s,%s\r",server,port);
            ret = boudica150_atcmd(cmd,"OK");
        }
        else
        {
            ret = true;  //we need not to set
        }
    }
    return ret;
}

//enable the cmee

static bool_t boudica150_set_cmee(int enable)
{
    bool_t ret;
    char cmd[64];

    memset(cmd,0,64);
    snprintf(cmd,64,"AT+CMEE=%d\r",enable);

    ret = boudica150_atcmd(cmd,"OK");

    return ret;
}

//enable the network
static bool_t boudica150_set_cgatt(int enable)  //unit second
{
    bool_t ret ;
    char cmd[64];

    memset(cmd,0,64);
    snprintf(cmd,64,"AT+CGATT=%d\r",enable);

    ret = boudica150_atcmd(cmd,"OK");

    return ret;
}

//enable the NNMI
static bool_t boudica150_set_nnmi(int enable)  //unit second
{
    bool_t ret ;
    char cmd[64];

    memset(cmd,0,64);
    snprintf(cmd,64,"AT+NNMI=%d\r",enable);

    ret = boudica150_atcmd(cmd,"OK");

    return ret;
}


//wait for the lwm2m observe
static s32_t urc_qlwevtind(uint8_t *data,s32_t len)
{

    int index_str;
    int ind;
    index_str = strlen(cn_urc_qlwevtind);

    if(len > index_str)
    {
        ind = data[index_str]-'0';
        printf("GET THE LWM2M:ind:%d\n\r",ind);
        if(ind == 3)
        {
            s_boudica150_oc_cb.lwm2m_observe = true;
        }
    }
    return len;
}


//check the lwm2m server observed
static bool_t boudica150_check_observe(s32_t time)  //unit second
{
    //wait for the server observed
    bool_t ret = false;
    s32_t times;

    for(times =0;times <time;times++ )
    {
        if(s_boudica150_oc_cb.lwm2m_observe )
        {
            ret = true;
            break;
        }
        task_sleepms(1000);
    }

    return ret;
}

//check if attached to the network
static bool_t boudica150_check_netattach(s32_t time)  //unit second
{
    //wait for the server observed
    bool_t ret = false;
    s32_t times;

    for(times =0;times <time;times++ )
    {
        if(boudica150_atcmd("AT+CGATT?\r", "CGATT:1"))
        {
            ret = true;
            break;
        }
        task_sleepms(1000);
    }

    return ret;
}

//set the autoconfig mode
static bool_t boudica150_set_autoconnect(int enable)
{
    char cmd[64];
    char resp[64];
    bool_t ret = true;
    const char *mode;

    if(enable)
    {
        mode = "AUTOCONNECT,TRUE";
    }
    else
    {
        mode = "AUTOCONNECT,FALSE";
    }

    memset(resp,0,64);
    ret = boudica150_atcmd_response("AT+NCONFIG?\r","+NCONFIG",resp,64);
    if((false == ret)||(NULL == strstr(resp,mode)))//which means we need to set it
    {
        memset(cmd,0,64);
        snprintf(cmd,64,"AT+NCONFIG=%s\r",mode);
        ret = boudica150_atcmd(cmd,"OK");

        boudica150_reboot();
    }
    else
    {
        ret = true;  //we need not to set
    }

    return ret;
}

//use this function to set the band,which corresponding with YUNYINGSHANG AND MOZU
static bool_t boudica150_boot(const char *plmn, const char *apn, const char *bands,const char *server,const char *port)
{
    memset(&s_boudica150_oc_cb,0,sizeof(s_boudica150_oc_cb));
    at_oobregister(urc_qlwevtind,cn_urc_qlwevtind);
    at_oobregister(boudica150_rcvdeal,cn_boudica150_rcvindex);

    while(1)
    {

        s_boudica150_oc_cb.lwm2m_observe = false;

        boudica150_reboot();

        boudica150_set_echo (0);

        boudica150_set_cmee(1);

        boudica150_set_autoconnect( 0);     //cgatt and cfun must be called if autoconnect is false

        boudica150_set_bands(bands);

        boudica150_set_fun(1);

        boudica150_set_plmn(plmn);

        boudica150_set_apn(apn);

        boudica150_set_cdp(server,port);

        boudica150_set_cgatt(1);

        boudica150_set_nnmi(1);

        if(false == boudica150_check_netattach(16))
        {
            continue;
        }

        if(false == boudica150_check_observe(16)) //TODO--maybe this only compatible with QUETEL MODULES
        {
            continue; //we should do the reboot for the nB
        }

        break;
    }
   //reach here means everything is ok, we can go now
    s_boudica150_oc_cb.sndenable = true;
    printf("NB MODULE RXTX READY NOW\n\r");
    return true;
}

static int boudica150_oc_config(tag_oc_config_param *param)
{
    int ret = -1;

    if(NULL != param)
    {
        s_boudica150_oc_cb.oc_param = *param;

        if(boudica150_boot(s_boudica150_oc_cb.plmn,s_boudica150_oc_cb.apn,s_boudica150_oc_cb.bands,\
                s_boudica150_oc_cb.oc_param.server,s_boudica150_oc_cb.oc_param.port))
        {
            ret = 0;
        }

    }

    return ret;
}

static int boudica150_oc_deconfig()
{
    return 0;
}


//this function to get the csq
int boudica150_get_csq(int *value)
{
    char cmd[64];
    char resp[64];
    const char *index = "OK";
    const char *str;
    int csq = 0;
    int ret = -1;

    if(NULL == value)
    {
        return ret;
    }

    memset(cmd,0,64);
    memset(resp,0,64);
    snprintf(cmd,64,"AT+CSQ\r");
    if(boudica150_atcmd_response(cmd,index,resp,64))
    {
        str = strstr((char *)resp,"+CSQ:");
        if(NULL != str)
        {
            str += strlen("+CSQ:");
            for (; *str <= '9' && *str >= '0' ;str++)
            {
                csq = (csq * 10 + (*str - '0'));
            }
            if(csq != 99) //99 is invalid
            {
                *value = csq;
                ret = 0;
            }
        }
    }
    return ret;
}



const tag_oc_lwm2m_opt  g_boudica150_oc_opt = \
{
    .config = boudica150_oc_config,
    .deconfig = boudica150_oc_deconfig,
    .report = boudica150_oc_report,
};


int boudica150_init(const char *plmn, const char *apn, const char *bands)
{
    int ret = -1;

    s_boudica150_oc_cb.plmn = plmn;
    s_boudica150_oc_cb.apn = apn;
    s_boudica150_oc_cb.bands = bands;

    ret = oc_lwm2m_register(&g_boudica150_oc_opt);

    return ret;
}





