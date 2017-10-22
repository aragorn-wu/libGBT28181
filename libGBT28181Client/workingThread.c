#include "workingThread.h"
#include "clientConfigDefine.h"
#include "XMLUtility.h"
#include "PlayH264DLL.h"

void* register_working_thread(void* arg)
{
    client_configurations* thread_parameter = (client_configurations*)arg;
    int result = OSIP_SUCCESS;

    while(thread_parameter->thread_loop)
    {
        osip_usleep((thread_parameter->expiration_interval - 10) * 1000 * 1000);
        eXosip_lock(thread_parameter->exosip_context);
        result = eXosip_register_send_register(
            thread_parameter->exosip_context,
            thread_parameter->registration_ID,
            NULL);
        if(0 > result)
        {
            osip_thread_exit();
            return NULL;
        }
        eXosip_unlock(thread_parameter->exosip_context);
    }

    osip_thread_exit();
#ifdef _DEBUG
    printf("register_working_thread exited\n");
#endif // _DEBUG
    return NULL;
}

void string_to_lower_case(char* string)
{
    size_t len = strlen(string);
    for(size_t i = 0; i < len; i++)
    {
        string[i] = tolower(string[i]);
    }
}

void* event_working_thread(void* arg)
{
    client_configurations* thread_parameter = (client_configurations*)arg;
    eXosip_event_t* event = NULL;
    osip_message_t* sip_message_answer = NULL;
    osip_content_type_t* content_type = NULL;
    osip_body_t* message_body = NULL;
    int result = OSIP_SUCCESS;
    xmlDocPtr xml_document_pointer = NULL;
    xmlDocPtr xml_current_node = NULL;
    char* content_type_type = NULL;
    char* content_type_subtype = NULL;

    while(thread_parameter->thread_loop)
    {
        event = eXosip_event_wait(thread_parameter->exosip_context, 0, 50);
        if(NULL == event)
        {
            continue;
        }
        eXosip_automatic_action(thread_parameter->exosip_context);
        switch(event->type)
        {
            case EXOSIP_REGISTRATION_SUCCESS:
#ifdef _DEBUG
                printf("registrered successfully\n");
#endif // _DEBUG
                if(NULL == osip_thread_create(20000, keepalive_working_thread, thread_parameter))
                {
                    // to do: handle error
                }
                break;
            case EXOSIP_REGISTRATION_FAILURE:
#ifdef _DEBUG
                printf("registrered failed\n");
#endif // _DEBUG
                break;
            case EXOSIP_MESSAGE_NEW:
                if(0 == osip_strcasecmp(event->request->sip_method, "REGISTER"))
                {
                    uint32_t device_number = get_dy_array_element_number(thread_parameter->register_device_info);
                    registered_device* p_registered_device_temp = NULL;
                    uint32_t i;
                    if(0 != event->request->contacts.nb_elt)
                    {
                        for(i = 0; i < device_number; i++)
                        {
                            p_registered_device_temp = get_dy_array_element_by_index(thread_parameter->register_device_info, i);
                            if(0 == osip_strcasecmp(p_registered_device_temp->device_name, osip_contact_get_displayname(event->request->contacts.node->element)))
                            {
                                break;
                            }
                        }
                        if(i == device_number)
                        {
                            p_registered_device_temp = calloc(1, sizeof(registered_device));
                            if(NULL != p_registered_device_temp)
                            {
                                p_registered_device_temp->device_domain = osip_malloc(50);
                                p_registered_device_temp->device_IP = osip_malloc(50);
                                p_registered_device_temp->device_name = osip_malloc(50);
								p_registered_device_temp->sz_device_port;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                    if(0 == event->request->authentication_infos.nb_elt)
                    {
                        result = eXosip_lock(thread_parameter->exosip_context);
                        result = eXosip_message_build_answer(thread_parameter->exosip_context, event->tid, 401, &sip_message_answer);
                        result = eXosip_message_send_answer(thread_parameter->exosip_context, event->tid, 401, sip_message_answer);
                        result = eXosip_unlock(thread_parameter->exosip_context);
                    }
                    else
                    {
                        result = 0;
                    }
                    break;
                }
                if(0 == osip_strcasecmp(event->request->sip_method, "MESSAGE"))
                {
                    result = eXosip_lock(thread_parameter->exosip_context);
                    result = eXosip_message_build_answer(thread_parameter->exosip_context, event->tid, 200, &sip_message_answer);
                    result = eXosip_message_send_answer(thread_parameter->exosip_context, event->tid, 200, sip_message_answer);
                    result = eXosip_unlock(thread_parameter->exosip_context);
                }

                content_type = osip_message_get_content_type(event->request);
                if(NULL != content_type)
                {
#ifdef _DEBUG
                    printf("%s\n", event->request->sip_method);
                    printf("%s/%s\n", content_type->type, content_type->subtype);
#endif // _DEBUG
                    content_type_type = strdup(content_type->type);
                    content_type_subtype = strdup(content_type->subtype);

                    string_to_lower_case(content_type_type);
                    string_to_lower_case(content_type_subtype);

                    if(0 == strncmp(content_type_type, "application", strlen("application")) &&
                       0 == strncmp(content_type_subtype, "manscdp+xml", strlen("MANSCDP+xml")))
                    {
                        result = osip_message_get_body(event->request, 0, &message_body);

                        if(OSIP_SUCCESS == result)
                        {
                            //size_t wchar_length = 2 * (message_body->length + 100);
                            //wchar_t* xml_in_wide_char = osip_malloc(wchar_length + 100);
                            //MultiByteToWideChar(936, MB_PRECOMPOSED, message_body->body, message_body->length, xml_in_wide_char, message_body->length);

                            //size_t UTF8_length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, xml_in_wide_char, wchar_length, NULL, 0, NULL, NULL);
                            //char* xml_in_UTF_8 = osip_malloc(UTF8_length + 100);
                            //WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, xml_in_wide_char, wchar_length, xml_in_UTF_8, UTF8_length, NULL, NULL);
                            //osip_free(xml_in_wide_char);

                            LIBXML_TEST_VERSION

                                xml_document_pointer = xmlReadMemory(message_body->body, message_body->length, "no_name.xml", NULL, 0);
                            //osip_free(xml_in_UTF_8);
                            if(NULL == xml_document_pointer)
                            {
                                break;
                            }
                            xml_current_node = xml_document_pointer;

                            if(NULL != xml_current_node->children)
                            {
                                xml_current_node = (xmlDocPtr)xml_current_node->children;
                                MANSCDP_xml* p_MANSCDP_xml = osip_malloc(sizeof(MANSCDP_xml));
                                xmlDocPtr xmlDocPtr_temp = NULL;
                                p_MANSCDP_xml->xml_type = get_xml_type(xml_current_node->name);
                                p_MANSCDP_xml->p_client_configurations = thread_parameter;
                                if(MANSCDP_xml_Unknown != p_MANSCDP_xml->xml_type)
                                {
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "CmdType");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->command_type = get_MANSCDP_command_type(xmlDocPtr_temp->children->content);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "SN");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->MANSCDP_SN = strtoull(xmlDocPtr_temp->children->content, NULL, 10);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "DeviceID");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->DeviceID = osip_strdup(xmlDocPtr_temp->children->content);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "Result");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->Result = osip_strdup(xmlDocPtr_temp->children->content);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "DeviceType");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->DeviceType = osip_strdup(xmlDocPtr_temp->children->content);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "Manufacturer");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->Manufacturer = osip_strdup(xmlDocPtr_temp->children->content);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "Model");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->Model = osip_strdup(xmlDocPtr_temp->children->content);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "Firmware");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->Firmware = osip_strdup(xmlDocPtr_temp->children->content);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "MaxCamera");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->MaxCamera = strtoull(xmlDocPtr_temp->children->content, NULL, 10);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "MaxAlarm");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->MaxAlarm = strtoull(xmlDocPtr_temp->children->content, NULL, 10);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "Online");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->online = get_MANSCDP_online(xmlDocPtr_temp->children->content);
                                    }
                                    xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "Status");
                                    if(NULL != xmlDocPtr_temp)
                                    {
                                        p_MANSCDP_xml->status = get_MANSCDP_statues(xmlDocPtr_temp->children->content);
                                    }
                                    if(MANSCDP_xml_Response == p_MANSCDP_xml->xml_type && MANSCDP_Catalog == p_MANSCDP_xml->command_type)
                                    {
                                        xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "SumNum");
                                        p_MANSCDP_xml->catalog_sum_num = strtoull(xmlDocPtr_temp->children->content, NULL, 10);
                                        if(0 < p_MANSCDP_xml->catalog_sum_num)
                                        {
                                            p_MANSCDP_xml->p_MANSCDP_device = osip_malloc(sizeof(MANSCDP_device) * p_MANSCDP_xml->catalog_sum_num);
                                            xmlDocPtr_temp = find_element((xmlDocPtr)xml_current_node->children, "DeviceList");
                                            parse_MANSCDP_xml_device_list(xmlDocPtr_temp, p_MANSCDP_xml->catalog_sum_num, p_MANSCDP_xml->p_MANSCDP_device);
                                        }
                                    }

                                    if(NULL == osip_thread_create(20000, MANSCDP_xml_message_working_thread, p_MANSCDP_xml))
                                    {
                                        free_MANSCDP_xml_pointer(&p_MANSCDP_xml);
                                    }
                                }
                                else
                                {
                                    free_MANSCDP_xml_pointer(&p_MANSCDP_xml);
                                }
                            }

                            xmlFreeDoc(xml_document_pointer);

                            xmlCleanupParser();
                        }
                    }
                    else
                    {
                        // to do: response content not acceptable
                    }
                    free(content_type_type);
                    free(content_type_subtype);
                }
                break;
            case EXOSIP_MESSAGE_ANSWERED:
#ifdef _DEBUG
                printf("SIP %d, received\n", osip_message_get_status_code(event->response));
#endif // _DEBUG
                break;
            case EXOSIP_CALL_PROCEEDING:
#ifdef _DEBUG
                printf("SIP %d, received\n", osip_message_get_status_code(event->response));
#endif // _DEBUG
                break;
            case EXOSIP_CALL_ANSWERED:
                result = osip_message_get_status_code(event->response);
                osip_message_get_body(event->response, 0, &message_body);
                printf("\n\n%s\n\n", message_body->body);
                if(200 == result)
                {
                    uint32_t i;
                    int type;
                    for(i = 0; i < thread_parameter->max_live_video_number; i++)
                    {
                        if(event->cid == thread_parameter->live_video_context_pointer_array[i]->call_id)
                        {
                            thread_parameter->live_video_context_pointer_array[i]->dialog_id = event->did;
                            break;
                        }
                    }

                    if(NULL != strstr(message_body->body, "PS"))
                    {
                        result = set_RTP_session_payload_type(
                            thread_parameter->live_video_context_pointer_array[i]->session_handle,
                            payload_type_PS);
                        type = 3;
                    }
                    else if(NULL != strstr(message_body->body, "H264"))
                    {
                        result = set_RTP_session_payload_type(
                            thread_parameter->live_video_context_pointer_array[i]->session_handle,
                            payload_type_H264);
                        type = 1;
                    }
                    else
                    {
                        MessageBoxA(NULL, "No media in SDP, please close this RTP session", "Warning", MB_OK);
                        break;
                    }

                    result = RTP_session_start(thread_parameter->live_video_context_pointer_array[i]->session_handle);
                    if(LIBRTP_OK != result)
                    {
                        // to do
                    }

                    myparamInput param;
                    param.fps = 0;
                    param.height = 0;
                    param.isDecode = true;
                    param.playChannle = 0;
                    param.playHandle = thread_parameter->live_video_context_pointer_array[i]->hWnd;
                    param.playHeight = 0;
                    param.playWidth = 0;
                    param.stopPlay = 0;
                    param.width = 0;

                    result = initial_decode_parameter(
                        thread_parameter->live_video_context_pointer_array[i]->instance,
                        &param,
                        type);

                    result = eXosip_lock(thread_parameter->exosip_context);
                    result = eXosip_call_build_ack(thread_parameter->exosip_context, event->did, &sip_message_answer);
                    result = eXosip_call_send_ack(thread_parameter->exosip_context, event->did, sip_message_answer);
                    result = eXosip_unlock(thread_parameter->exosip_context);
                }
                break;
            default:
#ifdef _DEBUG
                printf("recieved unknown eXosip event (type, did, cid) = (%d, %d, %d)\n", event->type, event->did, event->cid);
#endif // _DEBUG
                break;
        }
        eXosip_event_free(event);
    }

    osip_thread_exit();
#ifdef _DEBUG
    printf("event_working_thread exited\n");
#endif // _DEBUG
    return NULL;
}

void* keepalive_working_thread(void* arg)
{
    client_configurations* thread_parameter = (client_configurations*)arg;
    int result = OSIP_SUCCESS;
    osip_message_t* keepalive_message = NULL;
    char* from = NULL;
    char* to = NULL;
    char* message_body = NULL;

    from = osip_malloc(512);
    to = osip_malloc(512);
    message_body = osip_malloc(512);

    if(NULL == from || NULL == to || NULL == message_body)
    {
        osip_free(from);
        osip_free(to);
        osip_free(message_body);
        return NULL;
    }

    snprintf(
        from,
        512,
        "sip:%s@%s:%d",
        thread_parameter->client_user_name,
        thread_parameter->client_IP,
        thread_parameter->client_port);
    snprintf(
        to,
        512,
        "sip:%s@%s:%d",
        thread_parameter->server_ID,
        thread_parameter->server_domain,
        thread_parameter->server_port);

    while(thread_parameter->thread_loop)
    {
        result = eXosip_message_build_request(
            thread_parameter->exosip_context,
            &keepalive_message,
            "MESSAGE",
            to,
            from,
            NULL);
        if(OSIP_SUCCESS != result)
        {
            // to do: handle error
            return NULL;
        }

        result = osip_message_set_content_type(keepalive_message, "application/MANSCDP+xml");
        if(OSIP_SUCCESS != result)
        {
            // to do: handle error
            return NULL;
        }

        snprintf(message_body, 512, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Notify><CmdType>Keepalive</CmdType><SN>%u</SN><DeviceID>%s</DeviceID><Status>OK</Status></Notify>", thread_parameter->MANSCDP_SN, thread_parameter->client_user_name);

        result = osip_message_set_body(keepalive_message, message_body, strnlen(message_body, 512));
        if(OSIP_SUCCESS != result)
        {
            // to do: handle error
            return NULL;
        }

        eXosip_lock(thread_parameter->exosip_context);
        result = eXosip_message_send_request(thread_parameter->exosip_context, keepalive_message);
        eXosip_unlock(thread_parameter->exosip_context);
        if(OSIP_SUCCESS != result)
        {
            // to do: handle error
        }

        EnterCriticalSection(&thread_parameter->critical_section);
        thread_parameter->MANSCDP_SN++;
        LeaveCriticalSection(&thread_parameter->critical_section);

        osip_usleep(thread_parameter->heartbeat_interval * 1000 * 1000);
    }

    osip_free(from);
    osip_free(to);
    osip_free(message_body);

#ifdef _DEBUG
    printf("keepalive_working_thread exited\n");
#endif // _DEBUG
    return NULL;
}

void* MANSCDP_xml_message_working_thread(void* arg)
{
    MANSCDP_xml* p_MANSCDP_xml = (MANSCDP_xml*)arg;
    int result = OSIP_SUCCESS;
    osip_message_t* MANSCDP_xml_message = NULL;
    char* from = osip_malloc(512);
    char* to = osip_malloc(512);
    char* xml_buffer = osip_malloc(1500);
    if(NULL == from || NULL == to || NULL == xml_buffer)
    {
        osip_free(from);
        osip_free(to);
        osip_free(xml_buffer);
        return NULL;
    }
    memset(xml_buffer, 0x0, 1500);

    snprintf(
        from,
        512,
        "sip:%s@%s:%d",
        p_MANSCDP_xml->p_client_configurations->client_user_name,
        p_MANSCDP_xml->p_client_configurations->client_IP,
        p_MANSCDP_xml->p_client_configurations->client_port);
    snprintf(
        to,
        512,
        "sip:%s@%s:%d",
        p_MANSCDP_xml->p_client_configurations->server_ID,
        p_MANSCDP_xml->p_client_configurations->server_IP,
        p_MANSCDP_xml->p_client_configurations->server_port);

    switch(p_MANSCDP_xml->xml_type)
    {
        case MANSCDP_xml_Unknown:
            break;
        case MANSCDP_xml_Control:
            break;
        case MANSCDP_xml_Query:
            switch(p_MANSCDP_xml->command_type)
            {
                case MANSCDP_command_type_unknown:
                    break;
                case MANSCDP_DeviceInfo:
                    snprintf(xml_buffer, 1500, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Response><CmdType>DeviceInfo</CmdType><SN>%u</SN><DeviceID>%s</DeviceID><Result>OK</Result><DeviceType>libGBT28181Client</DeviceType><Manufacturer>libGBT28181Client</Manufacturer><Model>libGBT28181Client</Model><Firmware>libGBT28181Client</Firmware><MaxCamera>0</MaxCamera><MaxAlarm>0</MaxAlarm></Response>", p_MANSCDP_xml->MANSCDP_SN, p_MANSCDP_xml->p_client_configurations->client_user_name);
                    break;
                case MANSCDP_DeviceStatus:
                    snprintf(xml_buffer, 1500, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Response><CmdType>DeviceStatus</CmdType><SN>%u</SN><DeviceID>%s</DeviceID><Result>OK</Result><Online>ONLINE</Online><Status>OK</Status><Alarmstatus Num=\"0\"></Alarmstatus></Response>", p_MANSCDP_xml->MANSCDP_SN, p_MANSCDP_xml->p_client_configurations->client_user_name);
                    break;
                case MANSCDP_Catalog:
                    snprintf(xml_buffer, 1500, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Response><CmdType>Catalog</CmdType><SN>%u</SN><DeviceID>%s</DeviceID><SumNum>1</SumNum><DeviceList Num=\"1\"><Item><DeviceID>%s</DeviceID><Name>libGBT28181Client</Name><Manufacturer>libGBT28181Client</Manufacturer><Model>GBT28181Client</Model><Owner>Owner</Owner><CivilCode>CivilCode</CivilCode><Address>Address</Address><Parental>0</Parental><SafetyWay>0</SafetyWay><RegisterWay>1</RegisterWay><Secrecy>0</Secrecy><Status>ON</Status></Item></DeviceList></Response>", p_MANSCDP_xml->MANSCDP_SN, p_MANSCDP_xml->p_client_configurations->client_user_name, p_MANSCDP_xml->p_client_configurations->client_user_name);
                    break;
            }
            break;
        case MANSCDP_xml_Notify:
            break;
        case MANSCDP_xml_Response:
            switch(p_MANSCDP_xml->command_type)
            {
                case MANSCDP_command_type_unknown:
                    break;
                case MANSCDP_DeviceInfo:
                    if(NULL != p_MANSCDP_xml->p_client_configurations->give_out_query_device_info_result)
                    {
                        p_MANSCDP_xml->p_client_configurations->give_out_query_device_info_result(
                            p_MANSCDP_xml->DeviceID,
                            p_MANSCDP_xml->DeviceType,
                            p_MANSCDP_xml->Manufacturer,
                            p_MANSCDP_xml->Model,
                            p_MANSCDP_xml->Firmware,
                            p_MANSCDP_xml->MaxCamera,
                            p_MANSCDP_xml->MaxAlarm);
                    }
                    break;
                case MANSCDP_DeviceStatus:
                    if(NULL != p_MANSCDP_xml->p_client_configurations->give_out_query_device_status_result)
                    {
                        p_MANSCDP_xml->p_client_configurations->give_out_query_device_status_result(
                            p_MANSCDP_xml->DeviceID,
                            p_MANSCDP_xml->online,
                            p_MANSCDP_xml->status);
                    }
                    break;
                case MANSCDP_Catalog:
                    if(NULL != p_MANSCDP_xml->p_client_configurations->give_out_query_catalog_result)
                    {
                        p_MANSCDP_xml->p_client_configurations->give_out_query_catalog_result(
                            p_MANSCDP_xml->DeviceID,
                            p_MANSCDP_xml->catalog_sum_num,
                            p_MANSCDP_xml->p_MANSCDP_device);
                    }
                    break;
            }
            break;
    }

    while(true)
    {
        if(10 < xmlStrlen(xml_buffer))
        {
            result = eXosip_message_build_request(
                p_MANSCDP_xml->p_client_configurations->exosip_context,
                &MANSCDP_xml_message,
                "MESSAGE",
                to,
                from,
                NULL);
            if(OSIP_SUCCESS != result)
            {
                // to do: handle error
                break;
            }

            result = osip_message_set_content_type(MANSCDP_xml_message, "application/MANSCDP+xml");
            if(OSIP_SUCCESS != result)
            {
                // to do: handle error
                break;
            }

            result = osip_message_set_body(MANSCDP_xml_message, xml_buffer, strnlen(xml_buffer, 1500));
            if(OSIP_SUCCESS != result)
            {
                // to do: handle error
                break;
            }

            eXosip_lock(p_MANSCDP_xml->p_client_configurations->exosip_context);
            result = eXosip_message_send_request(p_MANSCDP_xml->p_client_configurations->exosip_context, MANSCDP_xml_message);
            eXosip_unlock(p_MANSCDP_xml->p_client_configurations->exosip_context);
            if(OSIP_SUCCESS != result)
            {
                // to do: handle error
                break;
            }

            break;
        }
        break;
    }

    osip_free(from);
    osip_free(to);
    osip_free(xml_buffer);
    free_MANSCDP_xml_pointer(&p_MANSCDP_xml);

#ifdef _DEBUG
    printf("MANSCDP_xml_message_working_thread exited\n");
#endif // _DEBUG
    return NULL;
}
