/*********************************************************************************************
* �ļ���lte-tcp.c
* ���ߣ�xuzhy 2018.1.11
* ˵����lte tcp����ģ��
*       
*       
*       
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/

#include "lte-tcp.h"

void gsm_uart_write(char *buf, int len);
int gsm_uart_read_ch(void);

process_event_t evt_tcp = 0xff;


#define DEBUG 0

#if DEBUG
#define Debug(x...) printf(x)
#else
#define Debug(x...)
#endif

#define CONNECT_NUMBER 12

#define TCP_BUF_SIZE  1500

/* ec20 һ��������֧��0-11��12tcp����*/
static tcp_t tcps[CONNECT_NUMBER];
static char send_buf[TCP_BUF_SIZE];
static int send_len = 0;
static void __gsm_tcp_close(int tid);

/*********************************************************************************************
* ���ƣ�on_gsm_tcp_open
* ���ܣ�����gsm_tcp_open�󣬱�ϵͳ���ã�����֪ͨtcp�򿪽����������evt_tcp�¼����û�����
* ������
* ���أ�
* �޸ģ�
* ע�ͣ��û���Ӧ���������ô˺���
*********************************************************************************************/
void on_gsm_tcp_open(int id, int err)
{
  if (tcps[id].p != NULL) {
    if (err == 0) {
      tcps[id].status = TCP_STATUS_CONNECTED;
    } else /*if (err == 563) */{ //�Ѿ���,��Ҫ�ر� 
      /*ǿ�ƹرգ����´�*/
      __gsm_tcp_close(id);
      tcps[id].status = TCP_STATUS_CLOSED;
    } /*else {
      tcps[id].status = TCP_STATUS_CLOSED;
    }*/
    process_post(tcps[id].p, evt_tcp, (process_data_t)&tcps[id]);
  }
}

/*********************************************************************************************
* ���ƣ�on_gsm_tcp_close
* ���ܣ���tcp���ӱ��ر�ʱ������evt_tcp�¼�֪ͨ�û�Ӧ�ó���
* ������
* ���أ�
* �޸ģ�
* ע�ͣ��û���Ӧ���������ô˺���
*********************************************************************************************/
void on_gsm_tcp_close(int id)
{
  if (tcps[id].p != NULL) {
    tcps[id].status = TCP_STATUS_CLOSED;
    process_post(tcps[id].p, evt_tcp, (process_data_t)&tcps[id]);
  }
}

/*********************************************************************************************
* ���ƣ�on_gsm_tcp_recv
* ���ܣ���tcp�����յ����ݺ󣬷���evt_tcp�¼�֪ͨ�û�Ӧ�ó���
* ������
* ���أ�
* �޸ģ�
* ע�ͣ��û���Ӧ���������ô˺���
*********************************************************************************************/
void on_gsm_tcp_recv(int id, int len, char *buf)
{
  if (tcps[id].p == NULL) {
    return;
  }
  /*
  char *p = malloc(len);
  if (p != NULL) {
    memcpy(p, buf, len);
    tcps[id].pdat = p;
    tcps[id].datlen = len;
    if (PROCESS_ERR_OK != process_post(tcps[id].p, evt_tcp, (process_data_t)&tcps[id])) {
      free(p);
      tcps[id].pdat = NULL;
    }
  }*/
  Debug("tcp >>> ");
  for (int i=0; i<len; i++) {
    Debug("%c", buf[i]);
  }
  Debug("\r\n");
  
  tcps[id].pdat = buf;
  tcps[id].datlen = len;
   if (PROCESS_ERR_OK != process_post(tcps[id].p, evt_tcp, (process_data_t)&tcps[id])) {
      free(buf);
      tcps[id].pdat = NULL;
    }
}

/*********************************************************************************************
* ���ƣ�__gsm_tcp_close
* ���ܣ������ر�tcp���ӵײ�ӿ�
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
static void __gsm_tcp_close(int tid)
{
  char buf[32];
  sprintf(buf, "at+qiclose=%d\r\n", tid);
  gsm_request_at(buf, 3000, NULL);
}

/*********************************************************************************************
* ���ƣ�gsm_tcp_close
* ���ܣ��ر�tcp����
* ������ptcp ��ģ������tcp���ӿ��ƿ�
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void  gsm_tcp_close(tcp_t *ptcp)
{
  __gsm_tcp_close(ptcp->id);
  ptcp->status = TCP_STATUS_CLOSED;
  process_post(ptcp->p, evt_tcp, (process_data_t)ptcp);
 
}


/*********************************************************************************************
* ���ƣ�on_gsm_tcp_open
* ���ܣ���tcp����
* ������ptcp������tcp_alloc�����tcp���ƿ�
*       sip��������ip��ַ
*       sport���������˿�
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
tcp_t* gsm_tcp_open(tcp_t *ptcp, char *sip, int sport)
{

  char buf[64];
  
  /*��ʼ��tcp�¼�*/
  if (evt_tcp == 0xff) {
    evt_tcp = process_alloc_event();
  }
  
  //ptcp->p = PROCESS_CURRENT();
  //ptcp->id = i;
  ptcp->status = TCP_STATUS_OPEN;
  sprintf(buf, "at+qiopen=1,%d,\"TCP\",\"%s\", %u,0,1\r\n", ptcp->id, sip, sport);
  gsm_request_at(buf, 10000, NULL);
  return ptcp;
}



/*********************************************************************************************
* ���ƣ�__on_send_data
* ���ܣ�
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
static tcp_t *current_tcp = NULL;

static void __on_send_data(char *rsp)
{
  extern void trim(char *);
  if (rsp == NULL) { //timeout
    send_len = 0;
    return;
  }
  else if (rsp[0] == '>') {
    /*  write data */
    Debug("tcp <<< ");
    for (int i=0; i<send_len; i++) {
      Debug("%c", send_buf[i]);
    }
    Debug("\r\n");
    
    gsm_uart_write(send_buf, send_len);
    send_len = 0;
  }
  else {
    //error ?
    send_len = 0;
    trim(rsp);
    Debug("__on_send_data(),rsp: %s\r\n", rsp);
    if((strlen(rsp)>0) && (strcmp(rsp, "SEND OK") == 0) )
    {
    }
    if((strlen(rsp)>0) && ((strcmp(rsp, "SEND FAIL") == 0) || (strcmp(rsp, "ERROR") == 0)))
    {
      printf("__on_send_data(),rsp: %s\r\n", rsp);
      //gsm_tcp_close(current_tcp);
    }
  }
}

/*********************************************************************************************
* ���ƣ�gsm_tcp_send
* ���ܣ�tcp���ӷ�������
* ������ptcp��tcp���ӿ��ƿ�
*       dat�����������ݻ����ַ
*       len�����������ݳ���
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
int gsm_tcp_send(tcp_t *ptcp,int len)
{
  char buf[32];
  if (len > TCP_BUF_SIZE) return -1;
  if (send_len != 0) return -2; //busy
  
  if (ptcp->status == TCP_STATUS_CONNECTED) {
    send_len = len;
    sprintf(buf, "at+qisend=%d,%d\r\n",ptcp->id, len);
    current_tcp = ptcp;
    gsm_request_at(buf, 1000, __on_send_data);
  }
  return -3;
}

/*********************************************************************************************
* ���ƣ�gsm_tcp_send
* ���ܣ�tcp���ӷ�������
* ������ptcp��tcp���ӿ��ƿ�
*       dat�����������ݻ����ַ
*       len�����������ݳ���
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
char *gsm_tcp_buf(void)
{
  if (send_len != 0) return NULL; //busy
  return send_buf;
}

/*********************************************************************************************
* ���ƣ�tcp_alloc
* ���ܣ�����tcp���ӿ��ƿ�
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
tcp_t* tcp_alloc()
{
  if (PROCESS_CURRENT() == NULL) return NULL;
  
  for (int i=0; i<CONNECT_NUMBER; i++) {
    if (tcps[i].p == NULL) {
      tcps[i].p = PROCESS_CURRENT();
      tcps[i].id = i;
      tcps[i].status = TCP_STATUS_CLOSED;
      return &tcps[i];
    }
  }
  return NULL;
}

/*********************************************************************************************
* ���ƣ�tcp_free
* ���ܣ��ͷ�tcp���ӿ��ƿ�
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void tcp_free(tcp_t *ptcp)
{
  if (ptcp->status != TCP_STATUS_CLOSED) {
    __gsm_tcp_close(ptcp->id);
  }
  ptcp->p = NULL;
}
/*
void gsm_tcp_init(void)
{
  evt_tcp = process_alloc_event();
}
*/