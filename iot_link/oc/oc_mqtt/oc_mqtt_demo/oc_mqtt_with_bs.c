/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2019>, <Huawei Technologies Co., Ltd>
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
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <osal.h>
#include <oc_mqtt_al.h>

#define CFG_OC_MQTT_WITH_BOOTSTRAP_EN 1

#if CFG_OC_MQTT_WITH_BOOTSTRAP_EN

/* brief : the oceanconnect platform only support the ca_crt up tills now*/
/** the address product_id device_id password crt is only for the test  */

#define BS_SERVER_IPV4         "119.3.184.255"     ///<  server ip address
#define BS_SERVER_PORT         "8883"           ///<  server mqtt service port
#define DEMO_WITH_BOOTSTRAP_NODEID    "31415926"
#define DEMO_WITH_BOOTSTRAP_PASSWORD    "9f825c0ed3e95ea3d459"


static char s_mqtt_ca_crt[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIEwTCCAqmgAwIBAgIRdi8oqJITu4uSWV2C/dUS1swwDQYJKoZIhvcNAQELBQAw\r\n"
"PDELMAkGA1UEBhMCQ04xDzANBgNVBAoTBkh1YXdlaTEcMBoGA1UEAxMTSHVhd2Vp\r\n"
"IEVxdWlwbWVudCBDQTAeFw0xNzAyMTYwNjM0MzVaFw00MTEwMDgwNjM0MzVaME0x\r\n"
"CzAJBgNVBAYTAkNOMQ8wDQYDVQQKEwZIdWF3ZWkxLTArBgNVBAMTJEh1YXdlaSBD\r\n"
"bG91ZCBDb3JlIE5ldHdvcmsgUHJvZHVjdCBDQTCCASIwDQYJKoZIhvcNAQEBBQAD\r\n"
"ggEPADCCAQoCggEBAKQQz5uvp3lmtK9uzeje7cZUF8RlRKavEQj9EQk45ly9a/Kr\r\n"
"07TlGIhaZv7j+N9ZV1jTiwA0+XWlni1anjy01qsBQ22eIzX3HQ3fTMjPfk67SFhS\r\n"
"aHdzkJwO66/Nojnaum84HfUTBuXfgiBNH4C2Bc9YBongLktwunqMGvMWEKj4YqjN\r\n"
"bjjZQ1G1Qnhk15qpEWI9YUv0I5X94oT5idEMIH+V+2hcW/6GmztoOgCekW3GPHGl\r\n"
"UJLt3cSaDkp1b5IchnGpwocZLJMd+V3emcLhbjXewIY3meUIkXMrqJ12L3ltkRVp\r\n"
"nHElgmpvp3dBjUrBiITLakrGq8P/Ta7OO5kf9JUCAwEAAaOBrDCBqTAfBgNVHSME\r\n"
"GDAWgBQq+BBZJ4A1H6d8ujufKuRKqpuS6jBGBgNVHSAEPzA9MDsGBFUdIAAwMzAx\r\n"
"BggrBgEFBQcCARYlaHR0cDovL3N1cHBvcnQuaHVhd2VpLmNvbS9zdXBwb3J0L3Br\r\n"
"aTAPBgNVHRMECDAGAQH/AgEAMA4GA1UdDwEB/wQEAwIBBjAdBgNVHQ4EFgQU9CcR\r\n"
"7wi0DatgF91OjC2HvAn0bG4wDQYJKoZIhvcNAQELBQADggIBADli3WJezyDe4qVl\r\n"
"jSpF3kxRvBPf6hPhf81WT/A3lo5f7rTOEkRqTB515i5p8Xv8QgP8WTcMu22K5oZt\r\n"
"6iV4PpWCaEVaHgAYeI8sjjWqI498nKWrJ1kaJkdOxB3omVa2Fet7xDCL6MR2jDZq\r\n"
"dtZGF2XCIiNuZhvTYOTvKTfuzN5/gi/z8GD8xP95V4vX3sB2jqgleMTirFdeA+RB\r\n"
"HDbS55MSIfA2jeXJt0IHoB5FKshcCPNLIW/s0O7lbSH62o4d+5PeVV8tbMESQ6Ve\r\n"
"NSRt22+n6Z2Z6n/ZMfM2jSziEZNIyPXQtywltkcrhRIbxWoB0IEr0Ci+7FVr9CRu\r\n"
"ZlcpliSKemrxiLo5EkoznWwxfUP11i473lUVljloJRglYWh6uo9Ci5Cu001occ4L\r\n"
"9qs13MTtpC96LouOyrqBUOlUmJvitqCrHSfqOowyi8B0pxH/+m+Q8+fP9cBztw22\r\n"
"JId8bth5j0OUbNDoY7DnjQLCI0bEZt4RSpQN1c6xf90gHutM62qoGI6NKlb2IH+r\r\n"
"Yfun6P4jYTN9vMnaDfxBH7ofz4q9nj27UBkX9ebqM8kS+RijnUUy8L2N6KsUpp2R\r\n"
"01cjcmp699kFIJ7ShCOmI95ZC9cociTnhTK6WScCweBH7eBxZwoQLi5ER3VkDs1r\r\n"
"rqnNVUgf2j9TOshCI6zuaxsA35wr\r\n"
"-----END CERTIFICATE-----\r\n";

static void           *s_mqtt_handle;

//if your command is very fast,please use a queue here--TODO
#define cn_app_rcv_buf_len 256
static char            s_rcv_buffer[cn_app_rcv_buf_len];
static int             s_rcv_datalen;
static osal_semp_t     s_oc_rcv_sync;
static void           *s_mqtt_handle;

static int app_msg_deal(void *handle,mqtt_al_msgrcv_t *msg)
{
    int ret = -1;
    printf("topic:%s qos:%d\n\r",msg->topic.data,msg->qos);

    if(msg->msg.len < cn_app_rcv_buf_len)
    {
        memcpy(s_rcv_buffer,msg->msg.data,msg->msg.len );
        s_rcv_buffer[msg->msg.len] = '\0'; ///< the json need it
        s_rcv_datalen = msg->msg.len;

        printf("msg:%s\n\r",s_rcv_buffer);

        osal_semp_post(s_oc_rcv_sync);
        ret = 0;

    }
    return ret;
}







static int oc_mqtt_report_entry(void *args)
{
    int leftpower = 0;
    void *handle;
    tag_oc_mqtt_report  report;
    tag_key_value_list  lst;
    tag_oc_mqtt_config config;
    cJSON *root;
    char  *buf;

    int times= 0;

    {
        config.boot_mode = en_oc_boot_strap_mode_client_initialize;
        config.server = BS_SERVER_IPV4;
        config.port = BS_SERVER_PORT;
        config.msgdealer = app_msg_deal;
        config.code_mode = en_oc_mqtt_code_mode_json;
        config.sign_type = en_mqtt_sign_type_hmacsha256_check_time_no;
        config.device_type = en_oc_mqtt_device_type_static;
        config.auth_type = en_mqtt_auth_type_nodeid;
        config.device_info.s_device.deviceid = DEMO_WITH_BOOTSTRAP_NODEID;
        config.device_info.s_device.devicepasswd = DEMO_WITH_BOOTSTRAP_PASSWORD;

        config.security.type = en_mqtt_al_security_cas;
        config.security.u.cas.ca_crt.data = s_mqtt_ca_crt;
        config.security.u.cas.ca_crt.len = sizeof(s_mqtt_ca_crt); ///< must including the end '\0'

        handle = oc_mqtt_config(&config);
        if(NULL == handle)
        {
            printf("config err\r\n");
        }

        s_mqtt_handle = handle;
        leftpower =33;
        while(1)  //do the loop here
        {
            leftpower = (leftpower + 7 )%100;

            lst.item.name = "batteryLevel";
            lst.item.buf = (char *)&leftpower;
            lst.item.len = sizeof(leftpower);
            lst.item.type = en_key_value_type_int;
            lst.next = NULL;

            report.hasmore = en_oc_mqtt_has_more_no;
            report.paralst= &lst;
            report.serviceid = "Battery";
            report.eventtime = "20190508T112020Z";

            root = oc_mqtt_json_fmt_report(&report);
            if(NULL != root)
            {
                buf = cJSON_Print(root);
                if(NULL != buf)
                {
                    if(0 == oc_mqtt_report(handle,buf,strlen(buf),en_mqtt_al_qos_1))
                    {
                        printf("times:%d power:%d\r\n",times++,leftpower);
                    }
                    osal_free(buf);
                }

                cJSON_Delete(root);
            }

            osal_task_sleep(5*1000); ///< do a sleep here
        }
    }
    return 0;
}

static int oc_mqtt_cmd_entry( void *args)
{
    cJSON  *msg;
    cJSON  *mid;
    cJSON  *ioswitch;
    cJSON  *msgType;
    cJSON  *paras;
    cJSON  *serviceId;
    cJSON  *cmd;
    char   *buf;

    tag_oc_mqtt_response response;
    tag_key_value_list   list;

    int mid_int;
    int err_int;
    while(1)
    {
        if(osal_semp_pend(s_oc_rcv_sync,cn_osal_timeout_forever))
        {
            err_int = 1;
            mid_int = 1;
            msg = cJSON_Parse(s_rcv_buffer);

            if(NULL != msg)
            {
                serviceId = cJSON_GetObjectItem(msg,"serviceId");
                if(NULL != serviceId)
                {
                    printf("msgType:%s\n\r",serviceId->valuestring);
                }

                mid = cJSON_GetObjectItem(msg,"mid");
                if(NULL != mid)
                {
                    mid_int = mid->valueint;
                    printf("mid:%d\n\r",mid_int);
                }
                msgType = cJSON_GetObjectItem(msg,"msgType");
                if(NULL != msgType)
                {
                    printf("msgType:%s\n\r",msgType->valuestring);
                    if(strncmp(msgType, "re-bootstrap", strlen(msgType)) == 0)
                    {
                        //Do the re-bootstrap
                        oc_mqtt_demo_main();
                        osal_task_exit();
                    }
                }
                cmd =  cJSON_GetObjectItem(msg,"cmd");
                if(NULL != cmd)
                {
                    printf("cmd:%s\n\r",cmd->valuestring);
                }

                paras = cJSON_GetObjectItem(msg,"paras");
                if(NULL != paras)
                {
                    ioswitch = cJSON_GetObjectItem(paras,"cmd1");
                    printf("cmd1:%d\n\r",ioswitch->valueint);
                    err_int = 0;
                }
                cJSON_Delete(msg);

                list.item.name = "body_para";
                list.item.buf = "body_para";
                list.item.type = en_key_value_type_string;
                list.next = NULL;

                response.hasmore = 0;
                response.errcode = err_int;
                response.mid = mid_int;
                response.bodylst = &list;

                msg = oc_mqtt_json_fmt_response(&response);
                if(NULL != msg)
                {
                    buf = cJSON_Print(msg);
                    if(NULL != buf)
                    {
                        if(0 == oc_mqtt_report(s_mqtt_handle,buf,strlen(buf),en_mqtt_al_qos_1))
                        {
                            printf("SNDMSG:%s\n\r",buf);
                        }
                        osal_free(buf);
                    }
                    cJSON_Delete(msg);
                }
            }
        }
    }

    return 0;
}


int oc_mqtt_demo_main()
{
    void *ret;
    printf("demo main new\r\n");
    osal_semp_create(&s_oc_rcv_sync,1,0);


    osal_task_create("ocmqtt_report",oc_mqtt_report_entry,NULL,0x1000,NULL,10);

    osal_task_create("ocmqtt_cmd",oc_mqtt_cmd_entry,NULL,0x1000,NULL,10);

    return 0;
}


#endif