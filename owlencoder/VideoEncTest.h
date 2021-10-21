#ifndef __VIDEO_ENC_TEST_H__
#define __VIDEO_ENC_TEST_H__

#include <dlfcn.h>
#include <pthread.h>

/************************************************************************/
#ifndef _OPENMAX_V1_2_
#include "ACT_OMX_Common_V1_2__V1_1.h"
#else
#define OMX_StateInvalid 0x0
#define OMX_ErrorInvalidState 0x0
#endif

#include "OMX_Component.h"
#include "OMX_Index.h"
#include "ACT_OMX_IVCommon.h"
#include "ACT_OMX_Index.h"
#include "video_mediadata.h"
#include "omx_malloc.h"

/*宏*/
#define  Test_Frame_Nums               300000      /*测试帧数*/
#define  WDF_FORMAT_TEST               0       /*WFD功能测试：即ARGB8888格式支持*/
#define  Enable_IDR_Refresh            0       /*IDR帧测试*/
#define  ACTION_OMX_TEST_DEBUG         1       /*DEBUG打印*/

#ifndef SPECVERSIONMAJOR
#define SPECVERSIONMAJOR  1
#endif
#ifndef SPECVERSIONMINOR
#define SPECVERSIONMINOR  1
#endif
#ifndef SPECREVISION
#define SPECREVISION      0
#endif
#ifndef SPECSTEP
#define SPECSTEP          0
#endif

#ifndef OMX_PARA_INIT_STRUCT
#define OMX_PARA_INIT_STRUCT(_s_, _name_)       \
	memset((_s_), 0x0, sizeof(_name_));         \
	(_s_)->nSize = sizeof(_name_);              \
	(_s_)->nVersion.s.nVersionMajor = SPECVERSIONMAJOR;      \
	(_s_)->nVersion.s.nVersionMinor = SPECVERSIONMINOR;      \
	(_s_)->nVersion.s.nRevision = SPECREVISION;       \
	(_s_)->nVersion.s.nStep = SPECSTEP;
#endif

/************************************************************************/

#define NUM_OF_IN_BUFFERS 5
#define MAX_UNRESPONSIVE_COUNT 50
#define NUM_OF_OUT_BUFFERS /*5*/5
#define MAX_NUM_OF_PORTS 16
#define MAX_EVENTS 256

/* For debug printing
 Add -DAPP_DEBUG to CFLAGS in test Makefile */
#define VIDENCTEST_DEBUG 0
#define KHRONOS_1_2
#if VIDENCTEST_DEBUG
#define VIDENCTEST_MAX_TIME_OUTS 1000000
#define __VIDENCTEST_PRINT__
#define __VIDENCTEST_DPRINT__
#define __VIDENCTEST_MTRACE__
#else
#define VIDENCTEST_MAX_TIME_OUTS 1000000
#endif
#define VIDENCTEST_UNUSED_ARG(arg) (void)(arg)

#define VIDENCTEST_USE_DEFAULT_VALUE (OMX_U32)-1
#define VIDENCTEST_USE_DEFAULT_VALUE_UI (unsigned int)-1

#define VIDENCTEST_MALLOC(_p_, _s_, _c_, _h_)                           \
    _p_ = (_c_*)malloc(_s_);    \
    if (_p_ == NULL) {                                                  \
        VIDENCTEST_MTRACE("malloc() error.\n");                         \
        eError = OMX_ErrorInsufficientResources;                        \
    }                                                                   \
    else {                                                              \
        VIDENCTEST_MTRACE(": %d :malloc() -> %p\n", __LINE__, _p_);     \
        memset((_p_), 0x0, _s_);                                        \
        if ((_p_) == NULL) {                                            \
            VIDENCTEST_MTRACE("memset() error.\n");                     \
            eError = OMX_ErrorUndefined;                                \
        }                                                               \
        else{                                                           \
            eError = VIDENCTEST_ListAdd(_h_, _p_);                      \
            if (eError == OMX_ErrorInsufficientResources) {             \
                VIDENCTEST_MTRACE("malloc() error.\n");                 \
            }                                                           \
        }                                                               \
    }                                                                   \

#define VIDENCTEST_FREE(_p_, _h_)                           \
    VIDENCTEST_ListRemove(_h_, _p_);                        \
    _p_ = NULL;                                             \

#define VIDENCTEST_CHECK_ERROR(_e_, _s_)                    \
    if (_e_ != OMX_ErrorNone){                              \
        printf("\n------VIDENCTEST FATAL ERROR-------\n %x : %s \n", _e_, _s_);  \
        VIDENCTEST_HandleError(pAppData, _e_);               \
        goto EXIT;                                          \
    }                                                       \

#define VIDENCTEST_CHECK_EXIT(_e_, _s_)                     \
    if (_e_ != OMX_ErrorNone){                              \
        printf("\n------VIDENCTEST ERROR-------\n %x : %s \n", _e_, _s_);  \
        goto EXIT;                                          \
    }

/*
 *  ANSI escape sequences for outputing text in various colors
 */
#ifdef APP_COLOR
#define DBG_TEXT_WHITE   "\x1b[1;37;40m"
#define DBG_TEXT_YELLOW  "\x1b[1;33;40m"
#define DBG_TEXT_MAGENTA "\x1b[1;35;40m"
#define DBG_TEXT_GREEN   "\x1b[1;32;40m"
#define DBG_TEXT_CYAN    "\x1b[1;36;40m"
#define DBG_TEXT_RED     "\x1b[1;31;40m"
#else
#define DBG_TEXT_WHITE ""
#define DBG_TEXT_YELLOW ""
#define DBG_TEXT_MAGENTA ""
#define DBG_TEXT_GREEN ""
#define DBG_TEXT_CYAN ""
#define DBG_TEXT_RED ""
#endif

#define APP_CONVERT_STATE(_s_, _p_)        \
    if (_p_ == 0) {                        \
        _s_ = "OMX_StateInvalid";          \
    }                                      \
    else if (_p_ == 1) {                   \
        _s_ = "OMX_StateLoaded";           \
    }                                      \
    else if (_p_ == 2) {                   \
        _s_ = "OMX_StateIdle";             \
    }                                      \
    else if (_p_ == 3) {                   \
        _s_ = "OMX_StateExecuting";        \
    }                                      \
    else if (_p_ == 4) {                   \
        _s_ = "OMX_StatePause";            \
    }                                      \
    else if (_p_ == 5) {                   \
        _s_ = "OMX_StateWaitForResources"; \
    }                                      \
    else {                                 \
        _s_ = "UnsupportedCommand";        \
    }

#ifdef __VIDENCTEST_DPRINT__
#define VIDENCTEST_DPRINT(STR, ARG...) VIDENCTEST_Log(__FILE__, __LINE__, __FUNCTION__, STR, ##ARG)
#else
#define VIDENCTEST_DPRINT(...)
#endif

#ifdef __VIDENCTEST_MTRACE__
#define VIDENCTEST_MTRACE(STR, ARG...) VIDENCTEST_Log(__FILE__, __LINE__, __FUNCTION__, STR, ##ARG)
#else
#define VIDENCTEST_MTRACE(...)
#endif

#ifdef __VIDENCTEST_PRINT__
#define VIDENCTEST_PRINT(...) fprintf(stdout, __VA_ARGS__)
#else
#define VIDENCTEST_PRINT  printf
#endif

#ifndef VIDENC_INPUT_PORT
#define VIDENC_INPUT_PORT  0x0
#endif

#ifndef VIDENC_OUTPUT_PORT
#define VIDENC_OUTPUT_PORT  0x1
#endif
#define VIDENC_SYNCPUT_PORT  0x2

typedef enum VIDENCTEST_STATE
{
	VIDENCTEST_StateLoaded = 0x0,
	VIDENCTEST_StateUnLoad,
	VIDENCTEST_StateReady,
	VIDENCTEST_StateStarting,
	VIDENCTEST_StateEncoding,
	VIDENCTEST_StateStopping,
	VIDENCTEST_StateConfirm,
	VIDENCTEST_StateWaitEvent,
	VIDENCTEST_StatePause,
	VIDENCTEST_StateStop,
	VIDENCTEST_StateError
} VIDENCTEST_STATE;

typedef enum VIDENCTEST_TEST_TYPE
{
    VIDENCTEST_FullRecord = 0x0,
    VIDENCTEST_PartialRecord,
    VIDENCTEST_PauseResume,
    VIDENCTEST_StopRestart
} VIDENCTEST_TEST_TYPE;

typedef enum VIDENC_TEST_NAL_FORMAT
{
    VIDENC_TEST_NAL_UNIT = 0,
    VIDENC_TEST_NAL_SLICE,
    VIDENC_TEST_NAL_FRAME
} VIDENC_TEST_NAL_FORMAT;

#ifndef KHRONOS_1_2
typedef enum OMX_EXTRADATATYPE
{
	OMX_ExtraDataNone = 0,
	OMX_ExtraDataQuantization
}OMX_EXTRADATATYPE;
#endif

typedef struct OMX_OTHER_EXTRADATATYPE_1_1_2
{
	OMX_U32 nSize;
	OMX_VERSIONTYPE nVersion;
	OMX_U32 nPortIndex;
	OMX_EXTRADATATYPE eType;
	OMX_U32 nDataSize;
	OMX_U8 data[1];
} OMX_OTHER_EXTRADATATYPE_1_1_2;

typedef struct APP_TIME
{
	time_t rawTime;
	struct tm* pTimeInfo;
	int nHrs;
	int nMin;
	int nSec;
	OMX_BOOL bInitTime;
	int nTotalTime;
} APP_TIME;

/* Structure used for the Memory List (Link-List)*/
typedef struct VIDENCTEST_NODE
{
	OMX_PTR pData;
	struct VIDENCTEST_NODE* pNext;
} VIDENCTEST_NODE;

typedef struct MYBUFFER_DATA
{
	time_t rawTime;
	struct tm* pTimeInfo;
} MYBUFFER_DATA;

typedef struct MJPEG_APPTHUMB
{
	int mjpg_thumbNailEnable;
	int mjpg_thumbNailWidth;
	int mjpg_thumbNailHeight;
} MJPEG_APPTHUMB;

typedef struct MYDATATYPE
{
	OMX_HANDLETYPE pHandle;
	char* szInFile;
	char* szOutFile;
	char* szOutFileNal;
	int nCWidth;
	int nCHeight;
	int nWidth;
	int nHeight;
	OMX_U32 eColorFormat;
	OMX_U32 nBitrate;
	OMX_U8 nFramerate;
	OMX_U8 eCompressionFormat;
	OMX_U8 eLevel;
	OMX_U32 nOutBuffSize;
	OMX_STATETYPE eState;
	OMX_PORT_PARAM_TYPE* pVideoInit;
	OMX_PARAM_PORTDEFINITIONTYPE* pPortDef[MAX_NUM_OF_PORTS];
	OMX_PARAM_PORTDEFINITIONTYPE* pInPortDef;
	OMX_PARAM_PORTDEFINITIONTYPE* pOutPortDef;
	OMX_PARAM_PORTDEFINITIONTYPE* pSyncPortDef;
	OMX_VIDEO_PARAM_AVCTYPE* pH264;
	OMX_VIDEO_AVCLEVELTYPE eLevelH264;
	OMX_VIDEO_PARAM_H263TYPE* pH263;
	OMX_VIDEO_H263LEVELTYPE eLevelH63;
	OMX_VIDEO_PARAM_MPEG4TYPE* pMpeg4;
	OMX_VIDEO_MPEG4LEVELTYPE eLevelMpeg4;
	int IpBuf_Pipe[2];
	int OpBuf_Pipe[2];
	int eventPipe[2];
    pthread_mutex_t pipe_mutex;
	int fdmax;
	FILE* fIn;
	FILE* fOut;
	FILE* fNalnd;
	OMX_U32 nCurrentFrameIn;
	OMX_U32 nCurrentFrameOut;
	OMX_S32 nRetVal;
	OMX_CALLBACKTYPE* pCb;
	OMX_COMPONENTTYPE* pComponent;
	OMX_BUFFERHEADERTYPE* pInBuff[NUM_OF_IN_BUFFERS];
	OMX_BUFFERHEADERTYPE* pOutBuff[NUM_OF_OUT_BUFFERS];
	OMX_BUFFERHEADERTYPE* pOSyncBuff[NUM_OF_OUT_BUFFERS];
	OMX_U8* pIBuffer[NUM_OF_IN_BUFFERS];
	OMX_U8* pOBuffer[NUM_OF_OUT_BUFFERS];
	OMX_U8* pOSyncBuffer[NUM_OF_OUT_BUFFERS];
	OMX_VIDEO_PARAM_BITRATETYPE* pVidParamBitrate;
	OMX_VIDEO_CONTROLRATETYPE eControlRate;
	OMX_VIDEO_PARAM_QUANTIZATIONTYPE* pQuantization;
	OMX_U32 nQpI;
	OMX_BOOL bAllocateIBuf;
	OMX_BOOL bAllocateOBuf;
	OMX_BOOL bAllocateSBuf;
	OMX_INDEXTYPE nVideoEncodeCustomParamIndex;
	OMX_U32 nVBVSize;
	OMX_BOOL bDeblockFilter;
	OMX_BOOL bForceIFrame;
	OMX_U32 nIntraFrameInterval;
	OMX_U32 nGOBHeaderInterval;
	OMX_U32 nTargetFrameRate;
	OMX_U32 nAIRRate;
	OMX_U32 nTargetBitRate;
	OMX_U32 nStartPortNumber;
	OMX_U32 nPorts;
	OMX_U8 nInBufferCount;
	OMX_U8 nOutBufferCount;
	void* pEventArray[MAX_EVENTS];
	OMX_U8 nEventCount;
	OMX_BOOL bStop;
	OMX_BOOL bExit;
	OMX_U32 nSizeIn;
	OMX_U32 nSizeOut;
	OMX_U32 nSizeSync;
	/*MJPG parameter,add by jiapeng 2012.2.10*/
	MJPEG_APPTHUMB appthumb;
	unsigned int nQualityfactor;
	unsigned int nDRI;

	OMX_U32 nReferenceFrame;
	OMX_U32 nNumberOfTimesTodo;
	OMX_U32 nNumberOfTimesDone;
	OMX_U32 nUnresponsiveCount;
	VIDENCTEST_NODE* pMemoryListHead; /* Used in Memory List (Link-List) */
	VIDENCTEST_STATE eCurrentState;
	VIDENCTEST_TEST_TYPE eTypeOfTest;
	OMX_U32 nMIRRate;
	OMX_U32 nResynchMarkerSpacing;
	unsigned int nEncodingPreset;
	OMX_U8 nUnrestrictedMV;
	OMX_U8 NalFormat;
	OMX_U8 bLastOutBuffer;
	OMX_U32 nQPIoF;
	OMX_ERRORTYPE (*init)(struct MYDATATYPE* pParaConf, struct MYDATATYPE* pAppData);
	OMX_ERRORTYPE (*encode)(struct MYDATATYPE* pAppData);
	OMX_ERRORTYPE (*dispose)(struct MYDATATYPE* pAppData);
	OMX_BOOL doAgain;
	pthread_t Enc_mainThread;
	int istop;
	void *pComHandle;
	int is_face_en;
	OMX_ERRORTYPE (*ActionOMX_Init)();
	OMX_ERRORTYPE (*ActionOMX_Deinit)();
	OMX_ERRORTYPE (*ActionOMX_GetHandle)(OMX_HANDLETYPE *, OMX_STRING, OMX_PTR, OMX_CALLBACKTYPE *);
	OMX_ERRORTYPE (*ActionOMX_FreeHandle)(OMX_HANDLETYPE *);
	StoreMetaDataInBuffersParams pMediaStoreType[2];
} MYDATATYPE;

typedef struct EVENT_PRIVATE
{
	OMX_EVENTTYPE eEvent;
	OMX_U32 nData1;
	OMX_U32 nData2;
	MYDATATYPE* pAppData;
	OMX_PTR pEventData;
} EVENT_PRIVATE;

typedef struct VIDENCTEST_BUFFER_PRIVATE
{
	OMX_BUFFERHEADERTYPE* pBufferHdr;
	OMX_U32 eBufferOwner;
	OMX_U32 bAllocByComponent;
	OMX_U32 nBufferIndex;
	OMX_BOOL bReadFromPipe;
	OMX_U32 nBufferWidth;
	OMX_U32 nBufferHeight;
} VIDENCTEST_BUFFER_PRIVATE;

//OMX_ERRORTYPE coda_videoenc_init(MYDATATYPE* pParaConf,MYDATATYPE* pAppData);
//OMX_ERRORTYPE coda_videoenc_encode(MYDATATYPE* pAppData);
//OMX_ERRORTYPE coda_videoenc_dispose(MYDATATYPE* pAppData);
OMX_ERRORTYPE coda_videoenc_open(MYDATATYPE** ppAppData);
#endif

