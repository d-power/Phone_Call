#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

#include <netinet/if_ether.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <net/ethernet.h>

#include <linux/if.h>
#include <linux/route.h>
#include <linux/if_packet.h>
#include <linux/fb.h>
#include <arpa/inet.h>

#include <PhoneCallCore.h>

#include <video/vd/VoApi.h>

#define MSG_PHONECALL       (10008)

#define CALL_ERROR          (1)
#define CALL_BUSY           (2)
#define CALL_TRANSBREAK     (3)
#define CALL_RING           (4)
#define CALL_ACCEPT         (5)
#define CALL_HUNGUP         (6)
#define CALL_NEWCALLIN      (7)
#define CALL_MESSAGE        (8)
#define CALL_MSGERR         (10)
#define CALL_UNLOCK         (11)

#define VIDEO_CHANEL        (1)
#define VIDEO_LAYER         (0)
#define DISP_WIDTH          (320)
#define DISP_HEIGHT         (240)

#define	WAIT_FOREVER	    (0xffffffff)

#define STR_OK              "[\x1b[1;31m OK \x1b[0m]"
#define STR_FAIL            "[\x1b[1;31mFAIL\x1b[0m]"

#define checkvalue(c)                                                                   \
    do {                                                                                \
        int r = (c);                                                                    \
        if (r)                                                                          \
            printf(" " STR_OK "  %s %s():%d  %s\n", __FILE__, __func__, __LINE__, #c);  \
        else                                                                            \
            printf(" " STR_FAIL "  %s %s():%d  %s\n", __FILE__, __func__, __LINE__, #c);\
        assert(r);                                                                      \
    } while(0)


static uint32_t SessionID = 0;
static uint32_t CallType = 0;
static char *LocalPhoneIP = NULL;
static char *LocalPhoneNumber = NULL;
static char *RemotePhoneIp = NULL;
static char *RemotePhoneNumber = NULL;
static void *MessageQueue = NULL;

typedef struct _SYS_MSG_S
{
    uint32_t msg;
    uint32_t wParam;
    uint32_t lParam;
    uint32_t zParam;
} SYS_MSG;

typedef struct _MSG_Q_S
{
    uint32_t wptr;          //  写偏移量
    uint32_t rptr;          //  读偏移量
    uint32_t itemsize;      //  每个类型的MsgQ的大小
    uint32_t maxsize;       //  消息队列最大数量
    uint32_t count;         //  当前未读消息总数
    bool bInit;	            //  该MsgQueue是否已经初始化
    sem_t semid;            //  信号量
    pthread_mutex_t mutex;  //  互斥锁
    char msgdata[0];        //  消息数据
} MSG_Q;

int MsgQueue_Create(uint32_t maxsize, uint32_t itemsize, void** pRoot)
{
    MSG_Q* pMsg = (MSG_Q*)malloc(sizeof(MSG_Q) + maxsize * itemsize);

    if(pMsg == NULL)
        return -1;

    memset(pMsg, 0, sizeof(MSG_Q) + maxsize * itemsize);
    pMsg->itemsize = itemsize;
    pMsg->maxsize = maxsize;
    pMsg->bInit = true;

    sem_init(&pMsg->semid, 0, 0);

    pthread_mutex_init(&pMsg->mutex, NULL);

    if(pRoot == NULL)
        return -2;

    *pRoot = pMsg;

    return 0;
}

int MsgQueue_Write(void* pRoot, void* data, uint32_t len)
{
    int result = -1;
    MSG_Q* pMsg = (MSG_Q*)pRoot;

    if(pMsg == NULL)
        return -1;

    if(len != pMsg->itemsize)
    {
        printf("MsgQueue itemsize len error %d %d \n", len, pMsg->itemsize);
        return result;
    }

    pthread_mutex_lock(&pMsg->mutex);

    if(pMsg->count < pMsg->maxsize)
    {
        memcpy(pMsg->msgdata + pMsg->wptr * pMsg->itemsize, data, len);
        pMsg->wptr++;
        pMsg->wptr %= pMsg->maxsize;
        pMsg->count++;
        sem_post(&pMsg->semid);
        result = 0;
    }
    else
    {
        printf("MsgQueue OverFlow %d %d \n", pMsg->count, pMsg->maxsize);
    }

    pthread_mutex_unlock(&pMsg->mutex);

    return result;
}

int MsgQueue_Read(void* pRoot, void* data, uint32_t len, uint32_t timeout)
{
    MSG_Q* pMsg = (MSG_Q*)pRoot;

    if(pMsg == NULL)
        return -1;

    struct timespec abstime;
    int ret = -1;
    int wret;

    if(timeout == 0)
        wret = sem_trywait(&pMsg->semid);
    else if(timeout == WAIT_FOREVER)
        wret = sem_wait(&pMsg->semid);
    else
    {
        clock_gettime(CLOCK_REALTIME, &abstime);

        abstime.tv_sec += timeout / 1000;
        abstime.tv_nsec += (timeout % 1000) * 1000000;
        abstime.tv_sec += abstime.tv_nsec / 1000000000;
        abstime.tv_nsec = abstime.tv_nsec % 1000000000;

        wret = sem_timedwait(&pMsg->semid, &abstime);
    }

    if(wret == 0)   //有信号
    {
        pthread_mutex_lock(&pMsg->mutex);

        if(pMsg->count > 0)
        {
            memcpy(data, pMsg->msgdata + pMsg->rptr * pMsg->itemsize, len);
            pMsg->rptr++;
            pMsg->rptr %= pMsg->maxsize;
            pMsg->count--;
            ret = 0;
        }

        pthread_mutex_unlock(&pMsg->mutex);
    }

    return ret;
}

void MsgQueue_Destory(void* pRoot)
{
    MSG_Q* pMsg = (MSG_Q*)pRoot;

    if(pMsg != NULL)
    {
        if(pMsg->bInit == true)
        {
            pMsg->bInit = false;

            sem_destroy(&pMsg->semid);

            pthread_mutex_destroy(&pMsg->mutex);

            free(pMsg);
            pMsg = NULL;
        }
    }
}

static void DPPostMessage(uint32_t msg, uint32_t wParam, uint32_t lParam, uint32_t zParam)
{
    SYS_MSG Msg;
    Msg.msg = msg;
    Msg.wParam = wParam;
    Msg.lParam = lParam;
    Msg.zParam = zParam;
    MsgQueue_Write(MessageQueue, &Msg, sizeof(SYS_MSG));
}

const char* PhoneMsgTypeToString(PhoneMsgType MsgType)
{
    switch (MsgType)
    {
        case PHONE_MSG_CALLACK:
            return "PHONE_MSG_CALLACK";
        case PHONE_MSG_CALLDISTRACT:
            return "PHONE_MSG_CALLDISTRACT";
        case PHONE_MSG_CALLIN:
            return "PHONE_MSG_CALLIN";
        case PHONE_MSG_REMOTEHANGUP:
            return "PHONE_MSG_REMOTEHANGUP";
        case PHONE_MSG_REMOTEACCEPT:
            return "PHONE_MSG_REMOTEACCEPT";
        case PHONE_MSG_REMOTEHOLD:
            return "PHONE_MSG_REMOTEHOLD";
        case PHONE_MSG_REMOTEWAKE:
            return "PHONE_MSG_REMOTEWAKE";
        case PHONE_MSG_ERROR:
            return "PHONE_MSG_ERROR";
        case PHONE_MSG_MESSAGE:
            return "PHONE_MSG_MESSAGE";
        case PHONE_MSG_MESSAGEERROR:
            return "PHONE_MSG_MESSAGEERROR";
        case PHONE_MSG_CHECK_HUNGUP:
            return "PHONE_MSG_CHECK_HUNGUP";
        default:
            return "UNKONW PHONE_MSG";
    }
}

static bool GetXmlNode(char *Buf, int Len, const char *Str, const char *Elem)
{
    char pMatch[32];
    sprintf(pMatch, "<%s>", Elem);

    const char *pStart = strstr(Str, pMatch);

    if(pStart == NULL)
        return false;

    pStart += strlen(pMatch);

    sprintf(pMatch, "</%s>", Elem);
    const char *pEnd = strstr(pStart, pMatch);

    if(pEnd == NULL)
        return false;

    if(pEnd - pStart >= Len)
    {
        printf("GetXmlElem %s overflow Len:%d, except:%d %s\n", Elem, Len, pEnd - pStart, Str);
        return false;
    }

    strncpy(Buf, pStart, pEnd - pStart);
    Buf[pEnd - pStart] = 0;
    return true;
}

int PhoneMsgProcess(int SessionID, PhoneMsgType MsgType, char *pMsg, int Length)
{
    printf("PhoneCallBackMsg MsgType[%s], pMsg[%s], Length[%d] \n", PhoneMsgTypeToString(MsgType), pMsg, Length);

    switch(MsgType)
    {
        case PHONE_MSG_CALLACK:
            {
                printf("PHONE_MSG_CALLACK[%s] \n", pMsg);

                if(strcmp(pMsg, "ring") == 0)
                {
                    printf("Remote Ring \n");
                }
                else
                {
                    if(SessionID != 0)
                    {
                        Phone_HangUp(SessionID);
                        SessionID = 0;
                    }
                }
            }
            break;

        case PHONE_MSG_CALLDISTRACT:
            break;

        case PHONE_MSG_CALLIN:
            {
                if(Length > 13)
                {
                    char RemoteNumber[32] = {0};
                    char CalledNumber[32] = {0};

                    GetXmlNode(RemoteNumber, sizeof(RemoteNumber), pMsg, "Number");
                    GetXmlNode(CalledNumber, sizeof(CalledNumber), pMsg, "CalledNumber");

                    sprintf(RemotePhoneNumber, "%s", CalledNumber);

                    printf("RemoteNumber:%s, CalledNumber:%s \n", RemoteNumber, CalledNumber);
                }
                else
                {
                    sprintf(RemotePhoneNumber, "%s", pMsg);
                }

                DPPostMessage(MSG_PHONECALL, CALL_NEWCALLIN, SessionID, 0);
            }
            break;

        case PHONE_MSG_REMOTEHANGUP: //对方挂机
            {
                DPPostMessage(MSG_PHONECALL, CALL_HUNGUP, SessionID, 0);
            }
            break;

        case PHONE_MSG_REMOTEACCEPT: //对方接听
            {
                DPPostMessage(MSG_PHONECALL, CALL_ACCEPT, SessionID, 0);
            }
            break;

        case PHONE_MSG_REMOTEHOLD: //对方保持呼叫，本机应进入呼叫等待
            break;

        case PHONE_MSG_REMOTEWAKE: //对方唤醒呼叫，本机应退出呼叫等待
            break;

        case PHONE_MSG_ERROR: //呼叫错误，消息字串可能为：sendfail（发送消息失败）
            {
                if(SessionID != 0)
                {
                    Phone_HangUp(SessionID);
                    SessionID = 0;
                }
            }
            break;

        case PHONE_MSG_MESSAGE: //收到对方发来的消息
            break;

        case PHONE_MSG_MESSAGEERROR: //消息发送失败
            break;

        case PHONE_MSG_CHECK_HUNGUP:
            break;
    }

    return 0;
}

static void SysMsgProcess(SYS_MSG *pMsg)
{
    if(pMsg->msg == MSG_PHONECALL)
    {
        switch(pMsg->wParam)
        {
            case CALL_NEWCALLIN:
                {
                    SessionID = pMsg->lParam;

                    if((LocalPhoneNumber[0] == '1' || LocalPhoneNumber[0] == '6') && \
                        (RemotePhoneNumber[0] == '2' || RemotePhoneNumber[0] == '3' || RemotePhoneNumber[0] == '7'))
                    {
                        Phone_SetVideoDisPlayArea(SessionID, 0, 0, 0, DISP_WIDTH, DISP_HEIGHT);
                    }

                    usleep(100 * 1000);

                    Phone_Accept(SessionID);
                    Phone_SetAudioVolume(SessionID, 100);

                    if((LocalPhoneNumber[0] == '1' || LocalPhoneNumber[0] == '6') && \
                        (RemotePhoneNumber[0] == '1' || RemotePhoneNumber[0] == '6'))
                    {
                        Phone_SetVideoDisPlayArea(SessionID, 0, 0, 0, DISP_WIDTH, DISP_HEIGHT);
                    }
                }
                break;

            case CALL_HUNGUP:
                if(SessionID == pMsg->lParam)
                {
                    Phone_HangUp(pMsg->lParam);

                    SessionID = 0;
                }
                break;
           case CALL_ACCEPT:
                {
                    SessionID = pMsg->lParam;

                    if(SessionID != 0)
                    {
                        if((LocalPhoneNumber[0] == '1' || LocalPhoneNumber[0] == '6') && \
                            (RemotePhoneNumber[0] == '1' || RemotePhoneNumber[0] == '6'))
                        {
                            Phone_SetAudioVolume(SessionID, 100);
                            Phone_SetVideoDisPlayArea(SessionID, 0, 0, 0, 320, 240);
                        }

                        if((LocalPhoneNumber[0] == '1' || LocalPhoneNumber[0] == '6') && \
                            (RemotePhoneNumber[0] == '2' || RemotePhoneNumber[0] == '3' || RemotePhoneNumber[0] == '7'))
                        {
                            Phone_SetVideoDisPlayArea(SessionID, 0, 0, 0, 320, 240);
                        }
                    }
                }
                break;
        }
    }
}

bool SetIPAddress(const char *pIpAddr, const char *pMask, const char *pGateway, const char *pNetCardName)
{
    struct sockaddr_in sin;
    struct ifreq ifr;

    printf("SetIPAddress %s %s %s\n", pIpAddr, pMask, pGateway);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (0 > fd)
    {
        printf( "socket error: %s\n", strerror(errno));
        return false;
    }

    // eth0
    strcpy(ifr.ifr_name, pNetCardName != NULL ? pNetCardName : "eth0");
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    //  IP
    sin.sin_addr.s_addr = inet_addr(pIpAddr == NULL ? "0" : pIpAddr);
    memcpy(&ifr.ifr_addr, &sin, sizeof(sin));

    if (ioctl(fd, SIOCSIFADDR, &ifr) < 0)
    {
        printf( "SIOCSIFADDR error: %s \n", strerror(errno));
        close(fd);
        return false;
    }

    // MASK
    sin.sin_addr.s_addr = inet_addr(pMask == NULL ? "255.255.255.0" : pMask);
    memcpy(&ifr.ifr_addr, &sin, sizeof(sin));

    if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0)
    {
        printf( "SIOCSIFNETMASK errno: %s \n", strerror(errno));
        close(fd);
        return false;
    }

    struct rtentry rt;
    memset(&rt, 0, sizeof(struct rtentry));
    rt.rt_dst.sa_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr = 0;
    rt.rt_genmask.sa_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = 0;

    if (pGateway)
    {
        sin.sin_addr.s_addr = inet_addr(pGateway);
        memcpy(&rt.rt_gateway, &sin, sizeof(sin));
        ((struct sockaddr_in *)&rt.rt_dst)->sin_family = AF_INET;
        ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
        rt.rt_flags = RTF_UP | RTF_GATEWAY;

        if (ioctl(fd, SIOCADDRT, &rt) < 0)
        {
            if (errno != EEXIST)
                printf( "Gateway SIOCADDRT errno: %s %d \n", strerror(errno), errno);
        }
    }

    close(fd);

    //  添加一个路由
    char cmdline[256] = {0};
    sprintf(cmdline, "/sbin/route add -net 224.0.0.0 netmask 240.0.0.0 dev %s", pNetCardName == NULL ? "eth0" : pNetCardName);
    system(cmdline);

    return true;
}

void signal_handler(int sig)
{
    printf("Ctrl-C pressed, cleaning up and exiting..\n");

    if(SessionID != 0)
    {
        Phone_HangUp(SessionID);
        SessionID = 0;
    }

    usleep(100 * 1000);

    Phone_UnInit();

    exit(0);
}

int main(int argc, char *argv[])
{
    CallType          = (argc < 2) ? 0 : strtol(argv[1], NULL, 10);
    LocalPhoneIP      = (argc < 3) ? NULL : argv[2];
    LocalPhoneNumber  = (argc < 4) ? NULL : argv[3];
    RemotePhoneIp     = (argc < 5) ? NULL : argv[4];
    RemotePhoneNumber = (argc < 6) ? NULL : argv[5];

    if (argc < 4)
    {
        fprintf(stderr, "\n\n========================================================\n\n");
        fprintf(stderr, "Usage: %s CallType LocalIP LocalNumber RemoteIP RemoteNumber \n\n", argv[0]);
        fprintf(stderr, "[1/5] CallType     -- 0:waitcall, 1:callout, 2:montior).\n");
        fprintf(stderr, "[2/5] LocalIP      -- example 192.168.1.94 .\n");
        fprintf(stderr, "[3/5] LocalNumber  -- example 1010401010101.\n");
        fprintf(stderr, "[4/5] RemoteIP     -- example 192.168.1.95 .\n");
        fprintf(stderr, "[5/5] RemoteNumber -- example 1010401010102.\n");
        fprintf(stderr, "\n\n========================================================\n\n");

        exit(1);
    }

    if(RemotePhoneNumber == NULL)
        RemotePhoneNumber = malloc(32);

    signal(SIGINT, signal_handler);

#if 0
    system("echo 51 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio51/direction");
    system("echo 0 > /sys/class/gpio/gpio51/value");

    system("amixer cset numid=17 1");

    system("echo 73 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio73/direction");
    system("echo 1 > /sys/class/gpio/gpio73/value");
#endif

    SensorParam_S param;

    memset(&param, 0, sizeof(param));

    param.SrcWidth  = 640;
    param.SrcHeight = 480;
    param.DstWidth  = 640;
    param.DstHeight = 480;
    param.BitRate   = 1 * 1024 * 1024;

    Phone_SetSensorParam(&param);

    PhoneInfo_S PhoneInfo;

    memset(&PhoneInfo, 0, sizeof(PhoneInfo));

    PhoneInfo.CalledNum = false;
    PhoneInfo.LocalCode = LocalPhoneNumber;
    PhoneInfo.LocalIp   = inet_addr(LocalPhoneIP);
    PhoneInfo.NetWorkCardName = "eth0";
    PhoneInfo.EnableUnicast = false;
    // PhoneInfo.Selmi     = true;
    // PhoneInfo.CustomInfo = "XG";

    checkvalue(Phone_Init_Info(&PhoneInfo) == true);

    Phone_RegisterMsgCallBack(&PhoneMsgProcess);

    SYS_MSG msg;

    checkvalue(MsgQueue_Create(100, sizeof(SYS_MSG), &MessageQueue) == 0);

    if(CallType == 1)
    {
        checkvalue((SessionID = Phone_CallOut(RemotePhoneNumber, RemotePhoneIp))!= 0);
    }
    else if(CallType == 2)
    {
        checkvalue((SessionID = Phone_Monitor(RemotePhoneNumber, RemotePhoneIp)) != 0);
    }

    printf("\n\n ------ Press  Ctrl + C  To Exit ------\n\n");

    while(1)
    {
        checkvalue(MsgQueue_Read(MessageQueue, &msg, sizeof(SYS_MSG), WAIT_FOREVER) == 0);
        SysMsgProcess(&msg);
    }

    return 0;
}

static void __attribute__ ((constructor)) video_display_init()
{
    printf("VO Enable \n");
    VO_Enable();
}

static void __attribute__ ((destructor)) video_display_deinit()
{
    printf("VO Disable \n");
    VO_Disable();
}