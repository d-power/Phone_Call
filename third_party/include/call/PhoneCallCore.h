/******************************************************************************
Copyright(c) 2011 - 2018 Digital Power Inc.
File name: PhoneCallCore.h
Author: liaosenlin
Version: 1.0.0
Date: 2018/09/17
Description: 呼叫相关的函数定义
History:
Bug report: liaosenlin@d-power.com.cn
******************************************************************************/
#ifndef __PHONECALLCORE_H__
#define __PHONECALLCORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

/* 视频回调函数类型 */
typedef void (*VideoFrameCb)(unsigned int UserData, int ftype, char *pdata, int dlen);

/* 音频回调函数类型 */
typedef void (*AudioFrameCb)(unsigned int UserData, char *pdata, int dlen);

/* yuv回调函数类型 */
typedef void (*YuvFrameCb)(unsigned int UserData, int width, int height, char *pdata0, char *pdata1);

/* 消息类型定义 */
typedef enum
{
    /* 呼叫结果, 消息字串可能为：busy（对方忙），ring（对方振铃），hold（对方挂机）*/
    PHONE_MSG_CALLACK  = 2000,

    /* 呼叫转移, 消息字串为转移号码 */
    PHONE_MSG_CALLDISTRACT,

    /* 新的呼入, 消息字串为呼入号码 */
    PHONE_MSG_CALLIN,

    /* 对方挂机 */
    PHONE_MSG_REMOTEHANGUP,

    /* 对方接听 */
    PHONE_MSG_REMOTEACCEPT,

    /* 对方保持呼叫，本机应进入呼叫等待 */
    PHONE_MSG_REMOTEHOLD,

    /* 对方唤醒呼叫，本机应退出呼叫等待 */
    PHONE_MSG_REMOTEWAKE,

    /* 呼叫错误，消息字串可能为：sendfail（发送消息失败） */
    PHONE_MSG_ERROR,

    /* 收到对方发来的消息 */
    PHONE_MSG_MESSAGE,

    /* 消息发送失败 */
    PHONE_MSG_MESSAGEERROR,

	/* 检查是否需要挂断(特定项目使用,一般不会有这条消息) */
	PHONE_MSG_CHECK_HUNGUP
} PhoneMsgType;

/* 抓拍jpg相关属性 */
typedef struct _Phone_JpgInfo_S
{
    /* 路径+文件名 */
    char 			FileName[32];

    /* 图片宽  */
    unsigned int 	Width;

    /* 图片高 */
    unsigned int 	Height;

    /* 码流(越高图像质量越好,占用体积越大) */
    unsigned int 	BitRate;
} Phone_JpgInfo_S;

/* 摄像头相关属性 */
typedef struct _SensorParam_S
{
    unsigned int 	SrcWidth;
    unsigned int 	SrcHeight;
    unsigned int 	DstWidth;
    unsigned int 	DstHeight;
    unsigned int 	BitRate;
} SensorParam_S;

/* 写音视频帧相关属性 */
typedef struct _StreamPacket_S
{
    /* 对于视频来说 I帧填1 其他帧暂定为0 */
    unsigned int 	isKeyFrame;

    /* 帧长度 */
    unsigned int 	FrameSize;

    /* 帧数据指针 */
    unsigned char 	*FrameData;
} StreamPacket_S;

/* 初始化通话库相关属性 */
typedef struct _PhoneInfo_S
{
    /* 本机终端编码对应ip */
    int  			LocalIp;

    /* 本机终端编码例如室内机1010101010101 */
    const char* 	LocalCode;

    /* 模拟门口机号码(无模拟门口机或者分机填空) */
    const char* 	AnalogCode;

    /* 厂商代号(无特殊要求填空) */
    const char* 	CustomInfo;

    /* 指定网卡名称(无特殊要求填空,默认使用eth0) */
    const char* 	NetWorkCardName;

	/* 是否开启检查挂断 */
	bool 			CheckHangUp;

	/* 呼叫是否使用CalledNum节点 */
	bool 			CalledNum;

	/* 呼叫是否使用Selmi节点 */
	bool 			Selmi;

	/* 是否启用写视频码流模式 */
	bool            WriteVieoMode;

	/* 是否启用门口机视频单播模式 */
	bool			EnableUnicast;

	/* 指定图层，默认填0即可 */
	char			SpecifiedLayer;
} PhoneInfo_S;

/******************************************************************************
Function: PhoneMsgCallBack
Description: 即时消息回调函数
Param:
	SessionID  in 会话id
	MsgType    in 消息类型定义同上
	pMsg  	   in 消息字符串
	Length     in 消息字符串长度
Return:
	失败返回 0
	成功返回 > 0的会话id
Others: None
******************************************************************************/
typedef int (*PhoneMsgCallBack)(int SessionID, PhoneMsgType MsgType, char *pMsg, int Length);

/******************************************************************************
Function: PhoneVideoStateCb (配合喂流接口使用)
Description: 视频使能关闭回调函数
Param:
	is_start  in true 需开启视频 false 需关闭视频
Return: None
Others: None
******************************************************************************/
typedef void (*PhoneVideoStateCb)(bool is_start);

/******************************************************************************
Function: Phone_Init
Description: 初始化呼叫库, 网卡默认使用eth0
Param:
    localcode 	in  本机终端编码例如室内机1010101010101
    localip		in  本机终端编码对应ip
    CustomInfo  in  出厂商
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_Init(const char *localcode, int localip, const char* CustomInfo);

/******************************************************************************
Function: Phone_Init_Info
Description: 初始化呼叫库, 网卡默认使用eth0
Param:
	*PhoneInfo	in 传入初始化通话库的相关信息
Return: None
Others: None
******************************************************************************/
bool Phone_Init_Info(PhoneInfo_S *PhoneInfo);

/******************************************************************************
Function: Phone_UnInit
Description: 析构呼叫库
Param:  None
Return: None
Others: None
******************************************************************************/
void Phone_UnInit();

/******************************************************************************
Function: Phone_CallOut
Description: 呼出函数
Param:
	Remotecode in 被呼方终端号码
	Remoteip   in 被呼方终端号码对应ip
Return:
	失败返回 0
	成功返回 > 0的会话id
Others: 调用前需检索配置表得出被呼方号码,函数内部不做有效性检索
******************************************************************************/
int Phone_CallOut(const char *Remotecode, const char *Remoteip);

/******************************************************************************
Function: Phone_CallOut2
Description: 呼出函数
Param:
	Localcode  in 主呼方终端号码
	Remotecode in 被呼方终端号码
	Remoteip   in 被呼方终端号码对应ip
Return:
	失败返回 0
	成功返回 > 0的会话id
Others: 调用前需检索配置表得出被呼方号码,函数内部不做有效性检索
******************************************************************************/
int Phone_CallOut2(const char *Localcode, const char *Remotecode, const char *Remoteip);

/******************************************************************************
Function: Phone_AnalogCallOut
Description: 模拟门口机呼出函数
Param:
	Remotecode in 被呼方终端号码
	Remoteip   in 被呼方终端号码对应ip
	bMontior   in 监视模拟门口机还是模拟门口机呼出
Return:
	失败返回 0
	成功返回 > 0的会话id
Others: 调用前需检索配置表得出被呼方号码,函数内部不做有效性检索
******************************************************************************/
int Phone_AnalogCallOut(const char *Remotecode, const char *Remoteip, bool bMontior);

/******************************************************************************
Function: Phone_Monitor
Description: 监视函数
Param:
	Remotecode in 被呼方终端号码
	Remoteip   in 被呼方终端号码对应ip
Return:
	失败返回 0
	成功返回 > 0的会话id
Others: None
******************************************************************************/
int Phone_Monitor(const char *Remotecode, const char *Remoteip);

/******************************************************************************
Function: Phone_Monitor
Description: 监视模拟门口机函数
Param:
	Remotecode in 被呼方终端号码(填1号室内机的号码)
	Remoteip   in 被呼方终端号码对应ip(填1号室内机的ip)
Return:
	失败返回 0
	成功返回 > 0的会话id
Others: None
******************************************************************************/
int Phone_MonitorAnalog(const char *Remotecode, const char *Remoteip);

/******************************************************************************
Function: Phone_Accept
Description: 通话接听函数
Param:
	SessionID in 呼出得到的会话id(被呼方才需要做接听)
Return:
	成功返回	 true
	失败返回	 false
Others: 默认启用回声消除
******************************************************************************/
bool Phone_Accept(int SessionID);

/******************************************************************************
Function: Phone_HangUp
Description: 通话挂断函数
Param:
	SessionID in 传入呼出或者呼入得到的会话id
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_HangUp(int SessionID);

/******************************************************************************
Function: Phone_RegisterMsgCallBack
Description: 即时消息回调注册函数
Param:
	cb in 传入应用层写的会话管理器函数指针
Return: None
Others: None
******************************************************************************/
void Phone_RegisterMsgCallBack(PhoneMsgCallBack cb);

/******************************************************************************
Function: Phone_SetSensorParam
Description: 设置门口机摄像头参数
Param:
	pSensorParam in 摄像头参数指针
Return: None
Others:
	在调Phone_Init或Phone_InitEx之前调用有效
******************************************************************************/
void Phone_SetSensorParam(SensorParam_S *pSensorParam);

/******************************************************************************
Function: Phone_SetVideoDisPlayArea
Description: 视频开启和视频显示区域设置函数
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	index	  in 第几路视频
	x         in 视频显示起始横坐标
	y         in 视频显示起始纵坐标
	w         in 视频显示起始宽度
	h         in 视频显示起始高度
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetVideoDisPlayArea(int SessionID, int index, int x, int y, int w, int h);

/******************************************************************************
Function: Phone_SetAudioVolume
Description: 通话中设置喇叭播放声音大小
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	vol	  	  in 音量大小(0 ~ 100)
Return:
	成功返回  	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetAudioVolume(int SessionID, int vol);

/******************************************************************************
Function: Phone_SetAudioRecVolume
Description: 通话中设置麦克风声音大小
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	vol	  	  in 音量大小(0 ~ 100)
Return:
	成功返回  	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetAudioRecVolume(int SessionID, int vol);

/******************************************************************************
Function: Phone_SetVideoDecCallBack
Description: 视频解码回调设置函数
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	cb	  	  in 视频回调函数
	UserData  in 用户数据
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetVideoDecCallBack(int SessionID, VideoFrameCb cb, unsigned int UserData);

/******************************************************************************
Function: Phone_SetVideoDecCallBack
Description: 视频编码回调设置函数
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	cb	  	  in 视频回调函数
	UserData  in 用户数据
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetVideoEncCallBack(int SessionID, VideoFrameCb cb, unsigned int UserData);

/******************************************************************************
Function: Phone_SetAudioRecCallBack
Description: 音频录音回调设置函数
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	cb	  	  in 音频回调函数
	UserData  in 用户数据
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetAudioRecCallBack(int SessionID, AudioFrameCb cb, unsigned int UserData);

/******************************************************************************
Function: Phone_SetAudioPlyCallBack
Description: 音频放音回调设置函数
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	cb	  	  in 音频回调函数
	UserData  in 用户数据
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetAudioPlyCallBack(int SessionID, AudioFrameCb cb, unsigned int UserData);

/******************************************************************************
Function: Phone_SetDecYuvCallBack
Description: 视频解码H264后YUV回调
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	cb	  	  in YUV回调函数
	UserData  in 用户数据
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetDecYuvCallBack(int SessionID, YuvFrameCb cb, unsigned int UserData);

/******************************************************************************
Function: Phone_SetDecYuvCallBack
Description: 视频编码码H264前的YUV回调
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	cb	  	  in YUV回调函数
	UserData  in 用户数据
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetEncYuvCallBack(int SessionID, YuvFrameCb cb, unsigned int UserData);

/******************************************************************************
Function: Phone_ChangeVideoSize
Description: 设置视频显示区域
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	x         in 视频显示起始横坐标
	y         in 视频显示起始纵坐标
	w         in 视频显示起始宽度
	h         in 视频显示起始高度
Return:
	成功返回	 true
	失败返回   	 false
Others: None
******************************************************************************/
bool Phone_ChangeVideoSize(int SessionID, int x, int y, int w, int h);

/******************************************************************************
Function: Phone_SetFileRecordMode
Description: 将对方发过來的音頻流保存成wav文件
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	enable    in 是否开启此模式
	FileName  in 文件所在路径(只支持wav)
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetFileRecordMode(int SessionID, bool enable, const char *FileName);

/******************************************************************************
Function: Phone_SetFilePlayMode
Description: 将wav文件发送给对方
Param:
	SessionID in 传入呼出或者呼入得到的会话id
	enable    in 是否开启此模式
	FileName  in 文件所在路径(只支持wav)
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_SetFilePlayMode(int SessionID, bool enable, const char *FileName);

/******************************************************************************
Function: Phone_CaptureVideoImage
Description: 室内机抓拍jpg
Param:
	SessionID 		 in 传入呼出或者呼入得到的会话id
	Phone_JpgInfo_S  in 编码jpg相关属性
Return:
	成功返回	 true
	失败返回	 false
Others: None
******************************************************************************/
bool Phone_CaptureVideoImage(int SessionID, Phone_JpgInfo_S *pInfo);

/******************************************************************************
Function: Phone_WriteVideoStream
Description: 写一帧视频数据做发送给对方
Param:
	pStream  		 in 视频帧数据包
Return:
	成功返回   	 true
	失败返回	 false
Others:
    该函数只会在有模拟门口机或者启用喂码流模式的时候调用有效
******************************************************************************/
bool Phone_WriteVideoStream(StreamPacket_S *pStream);

/******************************************************************************
Function: Phone_CheckHaveSession
Description: 检查当前是否存在会话
Param: None
Return:
	存在返回      	 true
	不存在返回	     false
Others: None
******************************************************************************/
bool Phone_CheckHaveSession();

/******************************************************************************
Function: Phone_Montior_HangUpAll
Description: 挂断所有监视
Param: None
Return: None
Others: None
******************************************************************************/
void Phone_Montior_HangUpAll();

/******************************************************************************
Function: Phone_Call_HangUpAll
Description: 挂断所有会话
Param: None
Return: None
Others: None
******************************************************************************/
void Phone_Call_HangUpAll();

/******************************************************************************
Function: Phone_RegisterVideoStateCallBack
Description: 视频开启关闭回调
Param:
	cb in 传入应用层写的会话管理器函数指针
Return: None
Others: None
******************************************************************************/
void Phone_RegisterVideoStateCallBack(PhoneVideoStateCb cb);

#ifdef __cplusplus
}
#endif

#endif	// !__PHONECALLCORE_H__

