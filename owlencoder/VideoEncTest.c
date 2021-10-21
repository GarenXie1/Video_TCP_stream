#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <time.h>
#include <getopt.h>
#include "VideoEncTest.h"
#include "ACT_Com.h"
#include "omx_malloc.h"

#if WDF_FORMAT_TEST
char* wfd_argb_file = "/mnt/cifs/testdb/bus_kvga_ARGB8888.yuv";
char* wfd_rgb565_file = "/data/11272_RGB565.data";
char* wfd_yuv420sp_file = "/data/bus_kvga_sp.yuv";
char* wfd_yuv420p_file = "/data/bus_kvga.yuv";
int wfd_format;
#endif

//#define  printf  if(0)printf

char strStoreMetaDataInBuffers[] = "OMX.google.android.index.storeMetaDataInBuffers";
char strFaceDet[] = "OMX.actions.index.facedetection";
char strThumb[] = "OMX.actions.index.thumbcontrol";
char strExifInfo[] = "OMX.actions.index.exifcontrol";
char strTsPacket[] = "OMX.actions.index.tspacket";
char strMVC[] = "OMX.actions.index.MVC";
char strRingBuf[] = "OMX.actions.index.ringbuffer";
OMX_STRING StrVideoEncoder = "OMX.Action.Video.Encoder";
static const float fQ16_Const = (float) (1 << 16);

static int fToQ16(float f)
{
	return (int) (f * fQ16_Const);
}

void VIDENCTEST_Log(const char *szFileName, int iLineNum, const char *szFunctionName, const char *strFormat, ...)
{
	va_list list;
	VIDENCTEST_UNUSED_ARG(szFileName);
	VIDENCTEST_UNUSED_ARG(iLineNum);
	fprintf(stdout, "%s():", szFunctionName);
	va_start(list, strFormat);
	vfprintf(stdout, strFormat, list);
	va_end(list);
}

int maxint(int a, int b)
{
	return (a > b) ? a : b;
}

/*-----------------------------------------------------------------------------*/
/**
 * ListCreate()
 *
 * Creates the List Head of the Component Memory List.
 *
 * @param pListHead VIDENCTEST_NODE double pointer with the List Header of the Memory List.
 *
 * @retval OMX_ErrorNone
 *               OMX_ErrorInsufficientResources if the malloc fails
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_ListCreate(struct VIDENCTEST_NODE** pListHead)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	*pListHead = (VIDENCTEST_NODE*) malloc(sizeof(VIDENCTEST_NODE)); /* need to malloc!!! */
	if (*pListHead == NULL)
	{
		VIDENCTEST_DPRINT("malloc() error.\n");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}
	VIDENCTEST_PRINT("malloc ptr is %lx,%d\n", (unsigned long) *pListHead, sizeof(VIDENCTEST_NODE));
	VIDENCTEST_DPRINT("Create MemoryListHeader[%p]\n", *pListHead);
	memset(*pListHead, 0x0, sizeof(VIDENCTEST_NODE));

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * ListAdd()
 *
 * Add a new node to Component Memory List
 *
 * @param pListHead VIDENCTEST_NODE Points List Header of the Memory List.
 *                pData OMX_PTR points to the new allocated data.
 * @retval OMX_ErrorNone
 *               OMX_ErrorInsufficientResources if the malloc fails
 *
 **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE VIDENCTEST_ListAdd(struct VIDENCTEST_NODE* pListHead, OMX_PTR pData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	VIDENCTEST_NODE* pTmp = NULL;
	VIDENCTEST_NODE* pNewNode = NULL;

	pNewNode = (VIDENCTEST_NODE*) malloc(sizeof(VIDENCTEST_NODE)); /* need to malloc!!! */
	if (pNewNode == NULL)
	{
		VIDENCTEST_DPRINT("malloc() error.\n");
		eError = OMX_ErrorInsufficientResources;
		goto EXIT;
	}
	VIDENCTEST_PRINT("malloc ptr is %lx,%d\n", (unsigned long) pNewNode, sizeof(VIDENCTEST_NODE));
	memset(pNewNode, 0x0, sizeof(VIDENCTEST_NODE));
	pNewNode->pData = pData;
	pNewNode->pNext = NULL;
	VIDENCTEST_DPRINT("Add MemoryNode[%p] -> [%p]\n", pNewNode, pNewNode->pData);

	pTmp = pListHead;

	while (pTmp->pNext != NULL)
	{
		pTmp = pTmp->pNext;
	}
	pTmp->pNext = pNewNode;

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * ListRemove()
 *
 * Called inside VIDENC_FREE Macro remove  node from Component Memory List and free the memory pointed by the node.
 *
 * @param pListHead VIDENCTEST_NODE Points List Header of the Memory List.
 *                pData OMX_PTR points to the new allocated data.
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE VIDENCTEST_ListRemove(struct VIDENCTEST_NODE* pListHead, OMX_PTR pData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	VIDENCTEST_NODE* pNode = NULL;
	VIDENCTEST_NODE* pTmp = NULL;

	pNode = pListHead;

	while (pNode->pNext != NULL)
	{
		if (pNode->pNext->pData == pData)
		{
			pTmp = pNode->pNext;
			pNode->pNext = pTmp->pNext;
			VIDENCTEST_DPRINT("Remove MemoryNode[%p] -> [%p]\n", pTmp, pTmp->pData);
			VIDENCTEST_PRINT("free ptr is %lx\n", (unsigned long) pTmp->pData);
			VIDENCTEST_PRINT("free ptr is %lx\n", (unsigned long) pTmp);
			free(pTmp->pData);
			free(pTmp);
			pTmp = NULL;
			break;
			/* VIDENC_ListPrint2(pListHead); */
		}
		pNode = pNode->pNext;
	}
	return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * ListDestroy()
 *
 * Called inside OMX_ComponentDeInit()  Remove all nodes and free all the memory in the Component Memory List.
 *
 * @param pListHead VIDENCTEST_NODE Points List Header of the Memory List.
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE VIDENCTEST_ListDestroy(struct VIDENCTEST_NODE* pListHead)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	VIDENCTEST_NODE* pTmp = NULL;
	VIDENCTEST_NODE* pNode = NULL;
	pNode = pListHead;

	while (pNode->pNext != NULL)
	{
		pTmp = pNode->pNext;
		if (pTmp->pData != NULL)
		{
			VIDENCTEST_FREE(pTmp->pData, pListHead);
		}
		pNode->pNext = pTmp->pNext;
		VIDENCTEST_FREE(pTmp, pListHead);
	}

	VIDENCTEST_DPRINT("Destroy MemoryListHeader[%p]\n", pListHead);
	VIDENCTEST_PRINT("free ptr is %lx\n", (unsigned long) pListHead);
	free(pListHead);
	return eError;
}

void VIDENCTEST_EventHandler(OMX_HANDLETYPE hComponent, MYDATATYPE* pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1,
		OMX_U32 nData2, OMX_PTR pEventData)
{
	EVENT_PRIVATE* pEventPrivate = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle;
	VIDENCTEST_NODE* pListHead;

	pHandle = pAppData->pHandle;
	pListHead = pAppData->pMemoryListHead;
	VIDENCTEST_UNUSED_ARG(hComponent);
	VIDENCTEST_MALLOC(pEventPrivate, sizeof(EVENT_PRIVATE), EVENT_PRIVATE, pListHead);

	/* TODO: Optimize using a linked list */
	pAppData->pEventArray[pAppData->nEventCount] = pEventPrivate;
	pAppData->nEventCount++;
	if (eError != OMX_ErrorNone)
	{
		VIDENCTEST_DPRINT("Erro in function VIDENCTEST_EventHandler\n");
	}
	else
	{
		pEventPrivate->pAppData = pAppData;
		pEventPrivate->eEvent = eEvent;
		pEventPrivate->nData1 = nData1;
		pEventPrivate->nData2 = nData2;
		pEventPrivate->pEventData = pEventData;

		printf("eventPipe!eEvent:%d,nData1:%lx,nData2:%lx\n", eEvent,nData1,nData2);
        pthread_mutex_lock(&pAppData->pipe_mutex);
		write(pAppData->eventPipe[1], &pEventPrivate, sizeof(pEventPrivate));
        pthread_mutex_unlock(&pAppData->pipe_mutex);
	}
}

void VIDENCTEST_FillBufferDone(OMX_HANDLETYPE hComponent, MYDATATYPE* pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
{
	VIDENCTEST_UNUSED_ARG(hComponent);
	VIDENCTEST_DPRINT("FillBufferDone :: %p \n", pBuffer);
	printf("FillBufferDone :: %p ,pBuffer:%p,nFilledLen:%d,nOffset:%d\n", pBuffer, pBuffer->pBuffer,
			(int) pBuffer->nFilledLen, (int) pBuffer->nOffset);
    pthread_mutex_lock(&pAppData->pipe_mutex);
	write(pAppData->OpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
    pthread_mutex_unlock(&pAppData->pipe_mutex);
}

void VIDENCTEST_EmptyBufferDone(OMX_HANDLETYPE hComponent, MYDATATYPE* pAppData, OMX_BUFFERHEADERTYPE* pBuffer)
{
	VIDENCTEST_UNUSED_ARG(hComponent);
	VIDENCTEST_DPRINT("EmptyBufferDone :: %p \n", pBuffer);
	printf("EmptyBufferDone :: %p ,pBuffer:%p,nFilledLen:%d,nOffset:%d\n", pBuffer, pBuffer->pBuffer,
			(int) pBuffer->nFilledLen, (int) pBuffer->nOffset);
    pthread_mutex_lock(&pAppData->pipe_mutex);
	write(pAppData->IpBuf_Pipe[1], &pBuffer, sizeof(pBuffer));
    pthread_mutex_unlock(&pAppData->pipe_mutex);
}

static int fill_count = 0;
int VIDENCTEST_Fill_MetaData(OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn, int width, int height)
{
	int nRead = -1;
	int nError = 0;

	video_metadata_t* pMetadata = (video_metadata_t*) (pBuf->pBuffer);
	pMetadata->metadataBufferType = kMetadataBufferTypeCameraSource_act;

	void* vir_addr = omx_getvir_from_phy((void *) ((video_handle_t*) (pMetadata->handle))->phys_addr);

#if WDF_FORMAT_TEST
	int wfd_len;
	if(wfd_format == OMX_COLOR_Format32bitARGB8888)
	{
		//fseek(fIn,0,SEEK_SET);
		wfd_len = width*height*4;
	}
	else if(wfd_format == OMX_COLOR_Format16bitRGB565)
	{
		//fseek(fIn,0,SEEK_SET);
		wfd_len = width*height*2;
	}
	else
	{
		wfd_len = width*height*3/2;
	}
	nRead = fread(vir_addr,1, wfd_len, fIn);
#else
	nRead = fread(vir_addr, 1, width * height * 3 / 2, fIn);
#endif

	pMetadata->off_x = 0;
	pMetadata->off_y = 0;
	pMetadata->crop_w = width;
	pMetadata->crop_h = height;
	pBuf->nFilledLen = nRead;

	pBuf->nTimeStamp = fill_count * 1000000 / 30;
	//if(fill_count > 5) pBuf->nTimeStamp += 5 * 1000000/30 ;

	fill_count++;

    /* set crop area */
	/*if(fill_count >= 5 && fill_count < 15)
	{
    	 pMetadata->crop_w = 80;
    	 pMetadata->crop_h = 80;
    	 pMetadata->off_x = 0;
    	 pMetadata->off_y = 0;
	}*/
	 
	printf("Fill_MetaData! pBuf->pBuffer:%lx off_x:%d,off_y:%d,crop_w:%d,crop_h:%d, nRead:%d\n",
		(unsigned long)pBuf->pBuffer,pMetadata->off_x,pMetadata->off_y,pMetadata->crop_w,pMetadata->crop_h,nRead);

	if (nRead == -1)
	{
		VIDENCTEST_DPRINT("Error Reading File!\n");
	}

	nError = ferror(fIn);
	if (nError != 0)
	{
		VIDENCTEST_DPRINT("ERROR: reading file\n");
	}

	nError = feof(fIn);
	if (nError != 0)
	{
		VIDENCTEST_DPRINT("EOS reached...seek to header of file\n");
		fseek(fIn, 0, SEEK_SET);
	}

	if (feof(fIn))
	{
		VIDENCTEST_DPRINT("Setting OMX_BUFFERFLAGE_EOS -> %p\n", pBuf);
		pBuf->nFlags = OMX_BUFFERFLAG_EOS;
	}

	return nRead;
}

int VIDENCTEST_Fill_RawData(OMX_BUFFERHEADERTYPE *pBuf, FILE *fIn, int buffersize, int width, int height, int count)
{
	int nRead = -1;
	int nError = 0;

	VIDENCTEST_PRINT("%s,%d\n", __FILE__, __LINE__);
	/* Input video frame format: YUV422 interleaved (1) or YUV420 (0) */
#ifdef ACTION_OMX_TEST_DEBUG
	printf("filled in buff %lx, %s:%d,%d,%d,%d\n ",(unsigned long)pBuf->pBuffer, __FILE__,__LINE__,buffersize,width,height);
#endif

	nRead = fread(pBuf->pBuffer, 1, width * height * 3 / 2, fIn);
	pBuf->nFilledLen = nRead;
	pBuf->nTimeStamp = count * 1000000 / 30;
	VIDENCTEST_PRINT("pBuffHead->nTimeStamp: %lld \n", pBuf->nTimeStamp);
#ifdef ACTION_OMX_TEST_DEBUG
	printf("Fill_RawData! pBuf->pBuffer:%lx width:%d height:%d  nRead:%d\n",(unsigned long)pBuf->pBuffer,width,height,nRead);
#endif

	if (nRead == -1)
	{
		VIDENCTEST_DPRINT("Error Reading File!\n");
	}

	nError = ferror(fIn);
	if (nError != 0)
	{
		VIDENCTEST_DPRINT("ERROR: reading file\n");
	}

	nError = feof(fIn);
	if (nError != 0)
	{
		VIDENCTEST_DPRINT("EOS reached...seek to header of file\n");
		fseek(fIn, 0, SEEK_SET);
	}

	if (feof(fIn))
	{
		VIDENCTEST_DPRINT("Setting OMX_BUFFERFLAGE_EOS -> %p\n", pBuf);
		pBuf->nFlags = OMX_BUFFERFLAG_EOS;
	}

	return nRead;
}

/*-----------------------------------------------------------------------------*/
/**
 * HandleError()
 *
 * Function call when an error ocurrs. The function un-load and free all the resource
 * depending the eError recieved.
 * @param pHandle Handle of MYDATATYPE structure
 * @param eError Error ocurred.
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/

OMX_ERRORTYPE VIDENCTEST_HandleError(MYDATATYPE* pAppData, OMX_ERRORTYPE eError)
{
	OMX_ERRORTYPE eErrorHandleError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	OMX_U32 nCounter;
	VIDENCTEST_NODE* pListHead;
	OMX_ERRORTYPE eErr = OMX_ErrorNone;

	VIDENCTEST_DPRINT("Enters to HandleError\n");
	pListHead = pAppData->pMemoryListHead;

	switch (pAppData->eCurrentState)
	{
		case VIDENCTEST_StateReady:
		case VIDENCTEST_StateStarting:
		case VIDENCTEST_StateEncoding:
		case VIDENCTEST_StateStopping:
		case VIDENCTEST_StateConfirm:
		case VIDENCTEST_StatePause:
		case VIDENCTEST_StateStop:
		case VIDENCTEST_StateWaitEvent:
		VIDENCTEST_DPRINT("Free buffers\n");

		if (pAppData->is_face_en)
		{
			eError = App_SyncBuffer_FreeBuffer(pHandle, pAppData);
			VIDENCTEST_CHECK_EXIT(eError, "Error App_SyncBuffer_FreeBuffer");
		}

		eError = App_OutBuffer_FreeBuffer(pHandle, pAppData);
		VIDENCTEST_CHECK_EXIT(eError, "Error App_OutBuffer_FreeBuffer");

		eError = App_InBuffer_FreeBuffer(pHandle, pAppData);
		VIDENCTEST_CHECK_EXIT(eError, "Error App_InBuffer_FreeBuffer");

		case VIDENCTEST_StateLoaded:
		VIDENCTEST_DPRINT("DeInit Component\n");
		eErrorHandleError = pAppData->ActionOMX_FreeHandle(pHandle);
		VIDENCTEST_CHECK_EXIT(eErrorHandleError, "Error at TIOMX_FreeHandle function");
		eErrorHandleError = pAppData->ActionOMX_Deinit();
		VIDENCTEST_CHECK_EXIT(eErrorHandleError, "Error at TIOMX_Deinit function");
		fclose(pAppData->fIn);
		fclose(pAppData->fOut);
		if (pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE)
		{
			fclose(pAppData->fNalnd);
		}

		eErr = close(pAppData->IpBuf_Pipe[0]);
		if (0 != eErr && OMX_ErrorNone == eError)
		{
			eError = OMX_ErrorHardware;
			VIDENCTEST_DPRINT("Error while closing data pipe\n");
		}

		eErr = close(pAppData->OpBuf_Pipe[0]);
		if (0 != eErr && OMX_ErrorNone == eError)
		{
			eError = OMX_ErrorHardware;
			VIDENCTEST_DPRINT("Error while closing data pipe\n");
		}

		eErr = close(pAppData->eventPipe[0]);
		if (0 != eErr && OMX_ErrorNone == eError)
		{
			eError = OMX_ErrorHardware;
			VIDENCTEST_DPRINT("Error while closing data pipe\n");
		}

		eErr = close(pAppData->IpBuf_Pipe[1]);
		if (0 != eErr && OMX_ErrorNone == eError)
		{
			eError = OMX_ErrorHardware;
			VIDENCTEST_DPRINT("Error while closing data pipe\n");
		}

		eErr = close(pAppData->OpBuf_Pipe[1]);
		if (0 != eErr && OMX_ErrorNone == eError)
		{
			eError = OMX_ErrorHardware;
			VIDENCTEST_DPRINT("Error while closing data pipe\n");
		}

		eErr = close(pAppData->eventPipe[1]);
		if (0 != eErr && OMX_ErrorNone == eError)
		{
			eError = OMX_ErrorHardware;
			VIDENCTEST_DPRINT("Error while closing data pipe\n");
		}
		pAppData->fIn = NULL;
		pAppData->fOut = NULL;
		pAppData->fNalnd = NULL;

		case VIDENCTEST_StateUnLoad:
		VIDENCTEST_DPRINT("Free Resources\n");
		VIDENCTEST_ListDestroy(pListHead);
		default:
		;
	}

EXIT:
    return eErrorHandleError;
}

OMX_ERRORTYPE VIDENCTEST_SetPrePParameter(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	OMX_INDEXTYPE nCustomIndex = OMX_IndexMax;

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("Enter VIDENCTEST_SetPrePParameter\n");
#endif

	/* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (syncput) */
	/**********************************************************************/
	OMX_PARA_INIT_STRUCT(pAppData->pSyncPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	//pAppData->pOutPortDef->nBufferSize = pAppData->nOutBuffSize;
	//pAppData->pSyncPortDef->format.video.cMIMEType = "face";
	pAppData->pSyncPortDef->nBufferCountActual = NUM_OF_OUT_BUFFERS;
	pAppData->pSyncPortDef->nBufferCountMin = 1;
	pAppData->pSyncPortDef->bEnabled = OMX_TRUE;
	pAppData->pSyncPortDef->bPopulated = OMX_FALSE;
	pAppData->pSyncPortDef->eDomain = OMX_PortDomainVideo;
	pAppData->pSyncPortDef->format.video.pNativeRender = NULL;
	pAppData->pSyncPortDef->format.video.nFrameWidth = pAppData->nWidth;
	pAppData->pSyncPortDef->format.video.nFrameHeight = pAppData->nHeight;
	pAppData->pSyncPortDef->format.video.nStride = 0;
	pAppData->pSyncPortDef->format.video.nSliceHeight = 0;
	pAppData->pSyncPortDef->format.video.nBitrate = 0;
	pAppData->pSyncPortDef->format.video.xFramerate = 0;
	pAppData->pSyncPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
	pAppData->pSyncPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	pAppData->pSyncPortDef->nPortIndex = VIDENC_SYNCPUT_PORT;

	eError = OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pSyncPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pSyncPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");

	pAppData->nSizeSync = pAppData->pSyncPortDef->nBufferSize;
	VIDENCTEST_PRINT("%s,%d  nSizeSync:%d\n", __FILE__, __LINE__, (int) pAppData->nSizeSync);

	/* Set the component's OMX_ACTIONS_Params structure (syncput) */
	/**********************************************************************/
	OMX_ACTIONS_Params actparam;
	OMX_PARA_INIT_STRUCT(&actparam, OMX_ACTIONS_Params);
	actparam.bEnable = 1;
	actparam.nPortIndex = VIDENC_SYNCPUT_PORT;
	eError = OMX_SetParameter(pHandle, OMX_ACT_IndexConfig_FACEDETECTION, &actparam);

	/* OMX_SendCommand */
	OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_SYNCPUT_PORT, NULL);

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("Exit VIDENCTEST_SetPrePParameter\n");
#endif

EXIT:
    return eError;
}

OMX_ERRORTYPE VIDENCTEST_SetH264Parameter(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	OMX_INDEXTYPE nCustomIndex = OMX_IndexMax;

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("Enter VIDENCTEST_SetH264Parameter\n");
#endif

	/* Set the component's StoreMetaDataInBuffersParams structure (input) */
	/*************************************************************/
	OMX_PARA_INIT_STRUCT(&(pAppData->pMediaStoreType[0]), StoreMetaDataInBuffersParams);
	pAppData->pMediaStoreType[0].nPortIndex = VIDENC_INPUT_PORT;
	pAppData->pMediaStoreType[0].bStoreMetaData = OMX_TRUE/*OMX_FALSE*/;

	eError = OMX_GetExtensionIndex(pHandle, strStoreMetaDataInBuffers, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pAppData->pMediaStoreType[0]);
	//eError = OMX_SetParameter (pHandle, OMX_IndexParameterStoreMediaData, &pAppData->pMediaStoreType[0]);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's StoreMetaDataInBuffersParams structure (output) */
	/*************************************************************/
	OMX_PARA_INIT_STRUCT(&(pAppData->pMediaStoreType[1]), StoreMetaDataInBuffersParams);
	pAppData->pMediaStoreType[1].nPortIndex = VIDENC_OUTPUT_PORT;
	pAppData->pMediaStoreType[1].bStoreMetaData = OMX_FALSE;

	eError = OMX_GetExtensionIndex(pHandle, strStoreMetaDataInBuffers, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pAppData->pMediaStoreType[1]);
	//eError = OMX_SetParameter (pHandle, OMX_IndexParameterStoreMediaData, &pAppData->pMediaStoreType[1]);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's OMX_ACTIONS_Params Ringbuffer structure (output) */
	/*************************************************************/
	OMX_ACTIONS_Params pAct_Ringbufffer;
	OMX_PARA_INIT_STRUCT(&pAct_Ringbufffer, OMX_ACTIONS_Params);
	pAct_Ringbufffer.bEnable = OMX_FALSE; /*OMX_TRUE*/
	pAct_Ringbufffer.nPortIndex = VIDENC_OUTPUT_PORT;
	if (pAct_Ringbufffer.bEnable == OMX_TRUE)
	{
		/*同步*/
		pAppData->pMediaStoreType[1].bStoreMetaData = OMX_FALSE;
		pAppData->bAllocateOBuf = OMX_FALSE;
	}

	eError = OMX_GetExtensionIndex(pHandle, strRingBuf, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pAct_Ringbufffer);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (input) */
	/**********************************************************************/
	OMX_PARA_INIT_STRUCT(pAppData->pInPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	//pAppData->pInPortDef->format.video.cMIMEType = "yuv"
	pAppData->pInPortDef->nBufferCountActual = NUM_OF_IN_BUFFERS;
	pAppData->pInPortDef->nBufferCountMin = 1;
	pAppData->pInPortDef->bEnabled = OMX_TRUE;
	pAppData->pInPortDef->bPopulated = OMX_FALSE;
	pAppData->pInPortDef->eDomain = OMX_PortDomainVideo;
	pAppData->pInPortDef->format.video.pNativeRender = NULL;
	pAppData->pInPortDef->format.video.nStride = pAppData->nCWidth;
	pAppData->pInPortDef->format.video.nSliceHeight = -1;
	pAppData->pInPortDef->format.video.xFramerate = fToQ16(pAppData->nFramerate);
	pAppData->pInPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
	pAppData->pInPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	pAppData->pInPortDef->format.video.eColorFormat = pAppData->eColorFormat;//OMX_COLOR_FormatYUV420Planar;
	pAppData->pInPortDef->format.video.nFrameWidth = pAppData->nCWidth;
	pAppData->pInPortDef->format.video.nFrameHeight = pAppData->nCHeight;
	pAppData->pInPortDef->nPortIndex = VIDENC_INPUT_PORT;

	VIDENCTEST_PRINT("input eCompressionFormat:%d  eColorFormat:%d\n",
			pAppData->pInPortDef->format.video.eCompressionFormat, pAppData->pInPortDef->format.video.eColorFormat);

	eError = OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* To get nBufferSize */
	eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");

	pAppData->nSizeIn = pAppData->pInPortDef->nBufferSize;
	VIDENCTEST_PRINT("nCWidth:%d nCHeight:%d pAppData->nSizeIn:%d  eColorFormat:%d\n", pAppData->nCWidth,
			pAppData->nCHeight, (int) pAppData->nSizeIn, pAppData->pInPortDef->format.video.eColorFormat);

	/* Set the component's OMX_VIDEO_PARAM_AVCTYPE structure (output) */
	/*************************************************************/
	OMX_PARA_INIT_STRUCT(pAppData->pH264, OMX_VIDEO_PARAM_AVCTYPE);
	pAppData->pH264->nPortIndex = VIDENC_OUTPUT_PORT;
	pAppData->pH264->nSliceHeaderSpacing = 0;
	pAppData->pH264->nPFrames = 14; /*P帧数*/
	pAppData->pH264->nBFrames = 0; /*B帧数*/
	pAppData->pH264->bUseHadamard = 0;
	pAppData->pH264->nRefFrames = -1;
	pAppData->pH264->nRefIdx10ActiveMinus1 = -1;
	pAppData->pH264->nRefIdx11ActiveMinus1 = -1;
	pAppData->pH264->bEnableUEP = OMX_FALSE;
	pAppData->pH264->bEnableFMO = OMX_FALSE;
	pAppData->pH264->bEnableASO = OMX_FALSE;
	pAppData->pH264->bEnableRS = OMX_FALSE;
	pAppData->pH264->eProfile = OMX_VIDEO_AVCProfileBaseline;
	pAppData->pH264->eLevel = OMX_VIDEO_AVCLevel5 /*pAppData->eLevelH264*/;
	pAppData->pH264->nAllowedPictureTypes = -1;
	pAppData->pH264->bFrameMBsOnly = OMX_FALSE;
	pAppData->pH264->bMBAFF = OMX_FALSE;
	pAppData->pH264->bEntropyCodingCABAC = OMX_TRUE;
	pAppData->pH264->bWeightedPPrediction = OMX_FALSE;
	pAppData->pH264->nWeightedBipredicitonMode = -1;
	pAppData->pH264->bconstIpred = OMX_FALSE;
	pAppData->pH264->bDirect8x8Inference = OMX_FALSE;
	pAppData->pH264->bDirectSpatialTemporal = OMX_FALSE;
	pAppData->pH264->nCabacInitIdc = -1;
	pAppData->pH264->eLoopFilterMode = OMX_VIDEO_AVCLoopFilterDisable;

	eError = OMX_SetParameter(pHandle, OMX_IndexParamVideoAvc, pAppData->pH264);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (output) */
	/***********************************************************************/
	OMX_PARA_INIT_STRUCT(pAppData->pOutPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	//pAppData->pOutPortDef->nBufferSize = pAppData->nOutBuffSize;
	//pAppData->pOutPortDef->format.video.cMIMEType = "264";
	pAppData->pOutPortDef->eDir = OMX_DirOutput;
	pAppData->pOutPortDef->nBufferCountActual = NUM_OF_OUT_BUFFERS;
	pAppData->pOutPortDef->nBufferCountMin = 1;
	pAppData->pOutPortDef->bEnabled = OMX_TRUE;
	pAppData->pOutPortDef->bPopulated = OMX_FALSE;
	pAppData->pOutPortDef->eDomain = OMX_PortDomainVideo;
	pAppData->pOutPortDef->format.video.pNativeRender = NULL;
	pAppData->pOutPortDef->format.video.nFrameWidth = pAppData->nWidth;
	pAppData->pOutPortDef->format.video.nFrameHeight = pAppData->nHeight;
	pAppData->pOutPortDef->format.video.nStride = pAppData->nWidth;
	pAppData->pOutPortDef->format.video.nSliceHeight = 0;
	pAppData->pOutPortDef->format.video.nBitrate = pAppData->nBitrate;
	pAppData->pOutPortDef->format.video.xFramerate = fToQ16(pAppData->nFramerate);
	pAppData->pOutPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
	pAppData->pOutPortDef->format.video.eCompressionFormat = pAppData->eCompressionFormat;
	pAppData->pOutPortDef->nPortIndex = VIDENC_OUTPUT_PORT;

	VIDENCTEST_PRINT("bitrate %d  eCompressionFormat:%d\n", (int) pAppData->nBitrate, pAppData->eCompressionFormat);

	eError = OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Retreive nBufferSize */
	eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");

	pAppData->nSizeOut = pAppData->pOutPortDef->nBufferSize;
	VIDENCTEST_PRINT("pAppData->nSizeOut:%d  eColorFormat:%d\n", (int) pAppData->nSizeOut,
			pAppData->pOutPortDef->format.video.eColorFormat);

	/* Set the component's OMX_ACT_PARAM_TsPacketType structure (output) */
	/*************************************************************/
	OMX_ACT_PARAM_TsPacketType ActTsPacket;
	OMX_PARA_INIT_STRUCT(&ActTsPacket, OMX_ACT_PARAM_TsPacketType);
	ActTsPacket.nPortIndex = VIDENC_OUTPUT_PORT;
	ActTsPacket.TsPacketType = OMX_TsPacket_Disable;/*OMX_TsPacket_NoBlu;*/

	eError = OMX_GetExtensionIndex(pHandle, strTsPacket, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &ActTsPacket);
	//eError = OMX_SetParameter (pHandle, OMX_ACT_IndexParmaTsPacket, &ActTsPacket);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetConfig");

	/* OMX_SendCommand */
	eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_INPUT_PORT, NULL);
	eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_OUTPUT_PORT, NULL);
	//eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, OMX_ALL, NULL);

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("Exit VIDENCTEST_SetH264Parameter\n");
#endif

EXIT:
	return eError;
}

OMX_ERRORTYPE VIDENCTEST_SetMJPEGParameter(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	OMX_INDEXTYPE nCustomIndex = OMX_IndexMax;

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("Enter VIDENCTEST_SetMJPEGParameter\n");
#endif

	/* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (input) */
	/**********************************************************************/
	OMX_PARA_INIT_STRUCT(pAppData->pInPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	//pAppData->pInPortDef->format.video.cMIMEType = "yuv";
	//pAppData->pInPortDef->format.video.xFramerate = fToQ16(pAppData->nFramerate);
	pAppData->pInPortDef->nBufferCountActual = NUM_OF_IN_BUFFERS;
	pAppData->pInPortDef->nBufferCountMin = 1;
	pAppData->pInPortDef->bEnabled = OMX_TRUE;
	pAppData->pInPortDef->bPopulated = OMX_FALSE;
	pAppData->pInPortDef->eDomain = OMX_PortDomainVideo;
	pAppData->pInPortDef->format.video.pNativeRender = NULL;
	pAppData->pInPortDef->format.video.nStride = pAppData->nCWidth;
	pAppData->pInPortDef->format.video.nSliceHeight = -1;
	pAppData->pInPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
	pAppData->pInPortDef->format.video.eColorFormat = pAppData->eColorFormat; //OMX_COLOR_FormatYUV420Planar;
	pAppData->pInPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	pAppData->pInPortDef->format.video.nFrameWidth = pAppData->nCWidth;
	pAppData->pInPortDef->format.video.nFrameHeight = pAppData->nCHeight;
	pAppData->pInPortDef->nPortIndex = VIDENC_INPUT_PORT;

	VIDENCTEST_PRINT("input eCompressionFormat:%d  eColorFormat:%d\n",
			pAppData->pInPortDef->format.video.eCompressionFormat, pAppData->pInPortDef->format.video.eColorFormat);

	eError = OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* To get nBufferSize */
	eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");
	pAppData->nSizeIn = pAppData->pInPortDef->nBufferSize;
	printf("nCWidth:%d nCHeight:%d pAppData->nSizeIn:%d  eColorFormat:%d\n", pAppData->nCWidth, pAppData->nCHeight,
			(int) pAppData->nSizeIn, pAppData->pInPortDef->format.video.eColorFormat);

	/* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (output) */
	/***********************************************************************/
	OMX_PARA_INIT_STRUCT(pAppData->pOutPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	//pAppData->pOutPortDef->nBufferSize = pAppData->nOutBuffSize;
	//pAppData->pOutPortDef->format.video.cMIMEType = "mjpeg";
	pAppData->pOutPortDef->eDir = OMX_DirOutput;
	pAppData->pOutPortDef->nBufferCountActual = NUM_OF_OUT_BUFFERS;
	pAppData->pOutPortDef->nBufferCountMin = 1;
	pAppData->pOutPortDef->bEnabled = OMX_TRUE;
	pAppData->pOutPortDef->bPopulated = OMX_FALSE;
	pAppData->pOutPortDef->eDomain = OMX_PortDomainVideo;
	pAppData->pOutPortDef->format.video.pNativeRender = NULL;
	pAppData->pOutPortDef->format.video.nFrameWidth = pAppData->nWidth;
	pAppData->pOutPortDef->format.video.nFrameHeight = pAppData->nHeight;
	pAppData->pOutPortDef->format.video.nStride = pAppData->nWidth;
	pAppData->pOutPortDef->format.video.nSliceHeight = 0;
	pAppData->pOutPortDef->format.video.nBitrate = 0;
	pAppData->pOutPortDef->format.video.xFramerate = 0;
	pAppData->pOutPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
	pAppData->pOutPortDef->format.video.eCompressionFormat = pAppData->eCompressionFormat;
	pAppData->pOutPortDef->nPortIndex = VIDENC_OUTPUT_PORT;

	eError = OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Retreive nBufferSize */
	eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");
	pAppData->nSizeOut = pAppData->pOutPortDef->nBufferSize;
	printf("pAppData->nSizeOut:%d  eColorFormat:%d\n", (int) pAppData->nSizeOut,
			pAppData->pOutPortDef->format.video.eColorFormat);

	/* Set the component's OMX_IMAGE_PARAM_QFACTORTYPE structure (output) */
	/***********************************************************************/
	OMX_IMAGE_PARAM_QFACTORTYPE Iquanty;
	OMX_PARA_INIT_STRUCT(&Iquanty, OMX_IMAGE_PARAM_QFACTORTYPE);
	Iquanty.nPortIndex = VIDENC_OUTPUT_PORT;
	Iquanty.nQFactor = 90;
	eError = OMX_SetParameter(pHandle, OMX_IndexParamQFactor, &Iquanty);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's OMX_ACT_PARAM_THUMBPARAM structure (output) */
	/***********************************************************************/
	OMX_ACT_PARAM_THUMBPARAM pVideoTumb;
	OMX_PARA_INIT_STRUCT(&pVideoTumb, OMX_ACT_PARAM_THUMBPARAM);
	pVideoTumb.bThumbEnable = OMX_TRUE;
	pVideoTumb.nHeight = 64;
	pVideoTumb.nWidth = 128;
	pVideoTumb.nPortIndex = VIDENC_OUTPUT_PORT;

	eError = OMX_GetExtensionIndex(pHandle, strThumb, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pVideoTumb);
	//eError = OMX_SetParameter (pHandle,OMX_ACT_IndexParamThumbControl, &pVideoTumb);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's OMX_ACT_PARAM_EXIFPARAM structure (output) */
	/***********************************************************************/
	OMX_ACT_PARAM_EXIFPARAM VideoExif;
	OMX_PARA_INIT_STRUCT(&VideoExif, OMX_ACT_PARAM_EXIFPARAM);
	VideoExif.nPortIndex = VIDENC_OUTPUT_PORT;
	{
		int mgpsLATH[3] = {22,18,0}; /*纬度*/
		int mgpsLATL[3] = {1,1,1};
		int mgpsLONGH[3] = {113,31,0};
		int mgpsLONGL[3] = {1,1,1};
		int mgpsTimeH[3] = {16,45,12};
		int mgpsTimeL[3] = {1,1,1};

		int i;
		VideoExif.bExifEnable = OMX_TRUE;
		VideoExif.ImageOri = 3;
		VideoExif.dataTime = "2012.9.11";
		VideoExif.exifmake = "tsh";
		VideoExif.exifmodel = "act_exif_model";
		VideoExif.bGPS = OMX_TRUE;
		VideoExif.focalLengthH = 720;
		VideoExif.focalLengthL = 1;
		for (i = 0; i < 3; i++)
		{
			VideoExif.gpsLATH[i] = mgpsLATH[i];
			VideoExif.gpsLATL[i] = mgpsLATL[i];
			VideoExif.gpsLONGH[i] = mgpsLONGH[i];
			VideoExif.gpsLONGL[i] = mgpsLONGL[i];
			VideoExif.gpsTimeH[i] = mgpsTimeH[i];
			VideoExif.gpsTimeL[i] = mgpsTimeL[i];
		}
		VideoExif.gpsDate = "2012.9.12";
		VideoExif.gpsprocessMethod = "act_gps_model";
	}

	eError = OMX_GetExtensionIndex(pHandle, strExifInfo, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &VideoExif);
	//eError = OMX_SetParameter (pHandle, OMX_ACT_IndexParamExifControl, &VideoExif);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's StoreMetaDataInBuffersParams structure (input) */
	/***********************************************************************/
	OMX_PARA_INIT_STRUCT(&(pAppData->pMediaStoreType[0]), StoreMetaDataInBuffersParams);
	pAppData->pMediaStoreType[0].nPortIndex = VIDENC_INPUT_PORT;
	pAppData->pMediaStoreType[0].bStoreMetaData = OMX_TRUE; /*OMX_FALSE*/

	eError = OMX_GetExtensionIndex(pHandle, strStoreMetaDataInBuffers, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pAppData->pMediaStoreType[0]);
	//eError = OMX_SetParameter (pHandle, OMX_IndexParameterStoreMediaData, &pAppData->pMediaStoreType[0]);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's StoreMetaDataInBuffersParams structure (output) */
	/***********************************************************************/
	OMX_PARA_INIT_STRUCT(&(pAppData->pMediaStoreType[1]), StoreMetaDataInBuffersParams);
	pAppData->pMediaStoreType[1].nPortIndex = VIDENC_OUTPUT_PORT;
	pAppData->pMediaStoreType[1].bStoreMetaData = OMX_FALSE; /*OMX_TRUE*/

	eError = OMX_GetExtensionIndex(pHandle, strStoreMetaDataInBuffers, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pAppData->pMediaStoreType[1]);
	//eError = OMX_SetParameter (pHandle, OMX_IndexParameterStoreMediaData, &pAppData->pMediaStoreType[1]);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's OMX_ACTIONS_Params Ringbuffer structure (output) */
	/*************************************************************/
	OMX_ACTIONS_Params pAct_Ringbufffer;
	OMX_PARA_INIT_STRUCT(&pAct_Ringbufffer, OMX_ACTIONS_Params);
	pAct_Ringbufffer.bEnable = OMX_FALSE; /*OMX_TRUE*/
	pAct_Ringbufffer.nPortIndex = VIDENC_OUTPUT_PORT;
	if (pAct_Ringbufffer.bEnable == OMX_TRUE)
	{
		/*同步*/
		pAppData->pMediaStoreType[1].bStoreMetaData = OMX_FALSE;
		pAppData->bAllocateOBuf = OMX_FALSE;
	}

	eError = OMX_GetExtensionIndex(pHandle, strRingBuf, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pAct_Ringbufffer);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* OMX_SendCommand */
	eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_INPUT_PORT, NULL);
	eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_OUTPUT_PORT, NULL);
	//eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, OMX_ALL, NULL);

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("Exit VIDENCTEST_SetMJPEGParameter");
#endif

EXIT:
    return eError;
}

OMX_ERRORTYPE VIDENCTEST_SetPreViewParameter(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	OMX_INDEXTYPE nCustomIndex = OMX_IndexMax;

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("Enter VIDENCTEST_SetPreViewParameter\n");
#endif

	/* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (input) */
	/**********************************************************************/
	OMX_PARA_INIT_STRUCT(pAppData->pInPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	pAppData->pInPortDef->nBufferCountActual = NUM_OF_IN_BUFFERS;
	pAppData->pInPortDef->nBufferCountMin = 1;
	pAppData->pInPortDef->bEnabled = OMX_TRUE;
	pAppData->pInPortDef->bPopulated = OMX_FALSE;
	pAppData->pInPortDef->eDomain = OMX_PortDomainVideo;
	pAppData->pInPortDef->format.video.pNativeRender = NULL;
	pAppData->pInPortDef->format.video.nStride = pAppData->nCWidth;
	pAppData->pInPortDef->format.video.nSliceHeight = -1;
	pAppData->pInPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
	pAppData->pInPortDef->format.video.eColorFormat = pAppData->eColorFormat;
	pAppData->pInPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	pAppData->pInPortDef->format.video.nFrameWidth = pAppData->nCWidth;
	pAppData->pInPortDef->format.video.nFrameHeight = pAppData->nCHeight;
	pAppData->pInPortDef->nPortIndex = VIDENC_INPUT_PORT;

	VIDENCTEST_PRINT("input eCompressionFormat:%d  eColorFormat:%d\n",
			pAppData->pInPortDef->format.video.eCompressionFormat, pAppData->pInPortDef->format.video.eColorFormat);
	printf("PreView!int nPortIndex:%d  out nPortIndex:%d\n", (int) pAppData->pInPortDef->nPortIndex,
			(int) pAppData->pOutPortDef->nPortIndex);

	eError = OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* To get nBufferSize */
	eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pInPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");
	pAppData->nSizeIn = pAppData->pInPortDef->nBufferSize;
	VIDENCTEST_PRINT("nCWidth:%d nCHeight:%d pAppData->nSizeIn:%d  eColorFormat:%d\n", pAppData->nCWidth,
			pAppData->nCHeight, (int) pAppData->nSizeIn, pAppData->pInPortDef->format.video.eColorFormat);

	/* Set the component's OMX_PARAM_PORTDEFINITIONTYPE structure (output) */
	/***********************************************************************/
	OMX_PARA_INIT_STRUCT(pAppData->pOutPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	/*pAppData->pOutPortDef->nBufferSize = pAppData->nOutBuffSize;*/
	pAppData->pOutPortDef->eDir = OMX_DirOutput;
	pAppData->pOutPortDef->nBufferCountActual = NUM_OF_OUT_BUFFERS;
	pAppData->pOutPortDef->nBufferCountMin = 1;
	pAppData->pOutPortDef->bEnabled = OMX_TRUE;
	pAppData->pOutPortDef->bPopulated = OMX_FALSE;
	pAppData->pOutPortDef->eDomain = OMX_PortDomainVideo;
	pAppData->pOutPortDef->format.video.pNativeRender = NULL;
	pAppData->pOutPortDef->format.video.nFrameWidth = pAppData->nWidth;
	pAppData->pOutPortDef->format.video.nFrameHeight = pAppData->nHeight;
	pAppData->pOutPortDef->format.video.nStride = pAppData->nWidth;
	pAppData->pOutPortDef->format.video.nSliceHeight = 0;
	pAppData->pOutPortDef->format.video.nBitrate = 0;
	pAppData->pOutPortDef->format.video.xFramerate = 0;
	pAppData->pOutPortDef->format.video.bFlagErrorConcealment = OMX_FALSE;
	pAppData->pOutPortDef->format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
	pAppData->pOutPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	pAppData->pOutPortDef->nPortIndex = VIDENC_OUTPUT_PORT;

	eError = OMX_SetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Retreive nBufferSize */
	eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pOutPortDef);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetParameter");
	pAppData->nSizeOut = pAppData->pOutPortDef->nBufferSize;
	VIDENCTEST_PRINT("pAppData->nSizeOut:%d  eColorFormat:%d\n", (int) pAppData->nSizeOut,
			pAppData->pOutPortDef->format.video.eColorFormat);

	/* Set the component's StoreMetaDataInBuffersParams structure (input) */
	/***********************************************************************/
	OMX_PARA_INIT_STRUCT(&(pAppData->pMediaStoreType[0]), StoreMetaDataInBuffersParams);
	pAppData->pMediaStoreType[0].nPortIndex = VIDENC_INPUT_PORT;
	pAppData->pMediaStoreType[0].bStoreMetaData = OMX_TRUE; /*OMX_FALSE*/

	eError = OMX_GetExtensionIndex(pHandle, strStoreMetaDataInBuffers, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pAppData->pMediaStoreType[0]);
	//eError = OMX_SetParameter (pHandle, OMX_IndexParameterStoreMediaData, &pAppData->pMediaStoreType[0]);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's StoreMetaDataInBuffersParams structure (output) */
	/***********************************************************************/
	OMX_PARA_INIT_STRUCT(&(pAppData->pMediaStoreType[1]), StoreMetaDataInBuffersParams);
	pAppData->pMediaStoreType[1].nPortIndex = VIDENC_OUTPUT_PORT;
	pAppData->pMediaStoreType[1].bStoreMetaData = OMX_FALSE; /*OMX_TRUE*/

	eError = OMX_GetExtensionIndex(pHandle, strStoreMetaDataInBuffers, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pAppData->pMediaStoreType[1]);
	//eError = OMX_SetParameter (pHandle, OMX_IndexParameterStoreMediaData, &pAppData->pMediaStoreType[1]);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* Set the component's OMX_ACTIONS_Params Ringbuffer structure (output) */
	/*************************************************************/
	OMX_ACTIONS_Params pAct_Ringbufffer;
	OMX_PARA_INIT_STRUCT(&pAct_Ringbufffer, OMX_ACTIONS_Params);
	pAct_Ringbufffer.bEnable = OMX_FALSE; /*OMX_TRUE*/
	pAct_Ringbufffer.nPortIndex = VIDENC_OUTPUT_PORT;
	if (pAct_Ringbufffer.bEnable == OMX_TRUE)
	{
		/*同步*/
		pAppData->pMediaStoreType[1].bStoreMetaData = OMX_FALSE;
		pAppData->bAllocateOBuf = OMX_FALSE;
	}

	eError = OMX_GetExtensionIndex(pHandle, strRingBuf, (OMX_INDEXTYPE*) &nCustomIndex);
	VIDENCTEST_CHECK_EXIT(eError, "Error at GetExtensionIndex");
	eError = OMX_SetParameter(pHandle, nCustomIndex, &pAct_Ringbufffer);
	VIDENCTEST_CHECK_EXIT(eError, "Error at SetParameter");

	/* OMX_SendCommand */
	eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_INPUT_PORT, NULL);
	eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_OUTPUT_PORT, NULL);
	//eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, OMX_ALL, NULL);

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("Exit VIDENCTEST_SetPreViewParameter");
#endif

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * AllocateResources()
 *
 * Allocate necesary resources.
 *
 * @param pAppData
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_AllocateResources(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int retval = 0;
	VIDENCTEST_NODE* pListHead;

	pListHead = pAppData->pMemoryListHead;

	VIDENCTEST_MALLOC(pAppData->pCb, sizeof(OMX_CALLBACKTYPE), OMX_CALLBACKTYPE, pListHead);
    VIDENCTEST_MALLOC(pAppData->pInPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE), OMX_PARAM_PORTDEFINITIONTYPE, pListHead);
    VIDENCTEST_MALLOC(pAppData->pOutPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE), OMX_PARAM_PORTDEFINITIONTYPE, pListHead);
    VIDENCTEST_MALLOC(pAppData->pSyncPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE), OMX_PARAM_PORTDEFINITIONTYPE, pListHead);

	if (pAppData->eCompressionFormat == OMX_VIDEO_CodingAVC)
	{
        VIDENCTEST_MALLOC(pAppData->pVidParamBitrate, sizeof(OMX_VIDEO_PARAM_BITRATETYPE), OMX_VIDEO_PARAM_BITRATETYPE, pListHead);
		VIDENCTEST_MALLOC(pAppData->pH264, sizeof(OMX_VIDEO_PARAM_AVCTYPE), OMX_VIDEO_PARAM_AVCTYPE, pListHead);
	}
	else if (pAppData->eCompressionFormat == OMX_VIDEO_CodingMJPEG)
	{
	}
	else if (pAppData->eCompressionFormat == OMX_VIDEO_CodingUnused)
	{
	}
	else
	{
		VIDENCTEST_DPRINT("Invalid compression format value.\n");
		eError = OMX_ErrorUnsupportedSetting;
		goto EXIT;
	}
	VIDENCTEST_MALLOC(pAppData->pQuantization, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE),
			OMX_VIDEO_PARAM_QUANTIZATIONTYPE, pListHead);

	/* Create a pipe used to queue data from the callback. */
	retval = pipe(pAppData->IpBuf_Pipe);
	if (retval != 0)
	{
		VIDENCTEST_DPRINT("Error: Fill Data Pipe failed to open\n");
		goto EXIT;
	}
	retval = pipe(pAppData->OpBuf_Pipe);
	if (retval != 0)
	{
		VIDENCTEST_DPRINT("Error: Empty Data Pipe failed to open\n");
		goto EXIT;
	}
	retval = pipe(pAppData->eventPipe);
	if (retval != 0)
	{
		VIDENCTEST_DPRINT("Error: Empty Data Pipe failed to open\n");
		goto EXIT;
	}

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * AllocateBuffers()
 *
 * Allocate necesary resources.
 *
 * @param pAppData
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_AllocateBuffers(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U8 nCounter = 0;
	VIDENCTEST_NODE* pListHead;

	pListHead = pAppData->pMemoryListHead;

	return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * FreeResources()
 *
 * Free all allocated memory.
 *
 * @param pAppData
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_FreeResources(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_U32 i = 0;
	VIDENCTEST_NODE* pListHead;

	pListHead = pAppData->pMemoryListHead;

	VIDENCTEST_FREE(pAppData->pCb, pListHead);
	VIDENCTEST_FREE(pAppData->pInPortDef, pListHead);
	VIDENCTEST_FREE(pAppData->pOutPortDef, pListHead);
	VIDENCTEST_FREE(pAppData->pSyncPortDef, pListHead);

	if ((pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE)
			&& pAppData->szOutFileNal)
	{
		VIDENCTEST_FREE(pAppData->szOutFileNal, pListHead);
	}

	if (pAppData->pH264 != NULL)
	{
		VIDENCTEST_FREE(pAppData->pH264, pListHead);
	}

	VIDENCTEST_FREE(pAppData->pVidParamBitrate, pListHead);

	if (pAppData->pH263 != NULL)
	{
		VIDENCTEST_FREE(pAppData->pH263, pListHead);
	}

	VIDENCTEST_FREE(pAppData->pQuantization, pListHead);

	if (pAppData->pMpeg4 != NULL)
	{
		VIDENCTEST_FREE(pAppData->pMpeg4, pListHead);
	}

	for (i = 0; i < pAppData->nEventCount; i++)
	{
		VIDENCTEST_FREE(pAppData->pEventArray[i], pListHead);
	}

	return eError;
}

static int GetCoreLibAndHandle(MYDATATYPE* pAppData)
{
	int ret = 0;

	void* pCoreModule = NULL;
	const char* pErr = dlerror();

	char buf[50] = "libOMX_Core.so";

	pCoreModule = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
	//VIDENCTEST_PRINT("%s,%d,pCoreModule:%lx\n",__FILE__,__LINE__,pCoreModule);
	if (pCoreModule == NULL)
	{
		VIDENCTEST_PRINT("dlopen %s failed because %s\n", buf, dlerror());
		ret = -1;
		goto UNLOCK_MUTEX;
	}

	pAppData->ActionOMX_Init = dlsym(pCoreModule, "OMX_Init");

	//VIDENCTEST_PRINT("%s,%d,ActionOMX_Init:%lx\n",__FILE__,__LINE__,pAppData->ActionOMX_Init);
	pErr = dlerror();
	if ((pErr != NULL) || (pAppData->ActionOMX_Init == NULL))
	{
		ret = -1;
		goto CLEAN_UP;
	}

	pAppData->ActionOMX_Deinit = dlsym(pCoreModule, "OMX_Deinit");

	VIDENCTEST_PRINT("%s,%d,ActionOMX_Deinit:%lx\n", __FILE__, __LINE__, (unsigned long) pAppData->ActionOMX_Deinit);
	pErr = dlerror();
	if ((pErr != NULL) || (pAppData->ActionOMX_Deinit == NULL))
	{
		ret = -1;
		goto CLEAN_UP;
	}

	pAppData->ActionOMX_GetHandle = dlsym(pCoreModule, "OMX_GetHandle");

	//VIDENCTEST_PRINT("%s,%d,ActionOMX_GetHandle:%lx\n",__FILE__,__LINE__,pAppData->ActionOMX_GetHandle);
	pErr = dlerror();
	if ((pErr != NULL) || (pAppData->ActionOMX_GetHandle == NULL))
	{
		ret = -1;
		goto CLEAN_UP;
	}

	pAppData->ActionOMX_FreeHandle = dlsym(pCoreModule, "OMX_FreeHandle");

	//VIDENCTEST_PRINT("%s,%d,ActionOMX_FreeHandle:%lx\n",__FILE__,__LINE__,pAppData->ActionOMX_FreeHandle);
	pErr = dlerror();
	if ((pErr != NULL) || (pAppData->ActionOMX_FreeHandle == NULL))
	{
		ret = -1;
		goto CLEAN_UP;
	}

	return 0;

CLEAN_UP:
	if(pCoreModule)
	{
		dlclose(pCoreModule);
		pCoreModule = NULL;
	}

UNLOCK_MUTEX:
    pAppData->pComHandle = pCoreModule;
	return ret;
}

static void freeCoreLib(MYDATATYPE* pAppData)
{
	if (pAppData->pComHandle)
	{
		dlclose(pAppData->pComHandle);
		pAppData->pComHandle = NULL;
	}
}
/*-----------------------------------------------------------------------------*/
/**
 * Init()
 *
 *Initialize all the parameters for the VideoEncoder Structures
 *
 * @param  pAppData MYDATA handle
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_PassToLoaded(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle;
	VIDENCTEST_NODE* pListHead;
	OMX_U32 i;
	int nPorts = 0;
	int ret = 0;
	OMX_CALLBACKTYPE sCb =
	{ (void*) VIDENCTEST_EventHandler, (void*) VIDENCTEST_EmptyBufferDone, (void*) VIDENCTEST_FillBufferDone };

	pListHead = pAppData->pMemoryListHead;
	eError = VIDENCTEST_AllocateResources(pAppData);
	VIDENCTEST_CHECK_EXIT(eError, "Error at Allocation of Resources");

	pAppData->fdmax = maxint(pAppData->IpBuf_Pipe[0], pAppData->eventPipe[0]);
	pAppData->fdmax = maxint(pAppData->OpBuf_Pipe[0], pAppData->fdmax);

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("get corelib and handle \n");
#endif
	ret = GetCoreLibAndHandle(pAppData);
	if (ret == -1)
	{
		VIDENCTEST_PRINT("err GetCoreLibAndHandle\n");
		return -1;
	}
	VIDENCTEST_PRINT("init start pAppData->ActionOMX_Init: %lx\n", (unsigned long) pAppData->ActionOMX_Init);

	/* Initialize OMX Core */
	eError = pAppData->ActionOMX_Init();
	*pAppData->pCb = sCb;

	/* Get VideoEncoder Component Handle */
	eError = pAppData->ActionOMX_GetHandle(&pHandle, StrVideoEncoder, pAppData, pAppData->pCb);
	VIDENCTEST_CHECK_EXIT(eError, "Error returned by TIOMX_Init()");
	if (pHandle == NULL)
	{
		VIDENCTEST_DPRINT("Error GetHandle() return Null Pointer\n");
		eError = OMX_ErrorUndefined;
		goto EXIT;
	}

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("sucess Get VideoEncoder Component Handle\n");//debug,should be deleted
#endif
	pAppData->pHandle = pHandle;

	/* Get starting port number and the number of ports */
	VIDENCTEST_MALLOC(pAppData->pVideoInit, sizeof(OMX_PORT_PARAM_TYPE), OMX_PORT_PARAM_TYPE, pListHead);

	OMX_PARA_INIT_STRUCT(pAppData->pVideoInit, OMX_PORT_PARAM_TYPE);
	eError = OMX_GetParameter(pHandle, OMX_IndexParamVideoInit, pAppData->pVideoInit);
	VIDENCTEST_CHECK_EXIT(eError, "Error returned from GetParameter()");

	VIDENCTEST_PRINT("%s,%d,%d\n", __FILE__, __LINE__, (int) pAppData->pVideoInit->nPorts);
	pAppData->nStartPortNumber = pAppData->pVideoInit->nStartPortNumber;
	pAppData->nPorts = pAppData->is_face_en ? pAppData->pVideoInit->nPorts : 2;
	nPorts = pAppData->pVideoInit->nPorts;
	VIDENCTEST_PRINT("is_face_en:%d, nPorts:%d\n", pAppData->is_face_en, (int) pAppData->pVideoInit->nPorts);
	VIDENCTEST_FREE(pAppData->pVideoInit, pListHead);

	/* TODO: Optimize - We should use a linked list instead of an array */
	for (i = pAppData->nStartPortNumber; i < pAppData->nPorts; i++)
	{
        VIDENCTEST_MALLOC(pAppData->pPortDef[i], sizeof(OMX_PARAM_PORTDEFINITIONTYPE), OMX_PARAM_PORTDEFINITIONTYPE, pListHead);
		OMX_PARA_INIT_STRUCT(pAppData->pPortDef[i], OMX_PARAM_PORTDEFINITIONTYPE);
		pAppData->pPortDef[i]->nPortIndex = i;
		eError = OMX_GetParameter(pHandle, OMX_IndexParamPortDefinition, pAppData->pPortDef[i]);
		VIDENCTEST_CHECK_EXIT(eError, "Error returned from GetParameter()");
		VIDENCTEST_PRINT("%s,%d,%d,%d\n", __FILE__, __LINE__, pAppData->pPortDef[i]->eDir,
				(int) pAppData->pPortDef[i]->nPortIndex);

		if (pAppData->pPortDef[i]->eDir == OMX_DirInput)
		{
			memcpy(pAppData->pInPortDef, pAppData->pPortDef[i], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
		}
		else
		{
			if ((pAppData->is_face_en == 1) && ((int) i == (nPorts - 1)))
			{
				memcpy(pAppData->pSyncPortDef, pAppData->pPortDef[i], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
			}
			else
			{
				memcpy(pAppData->pOutPortDef, pAppData->pPortDef[i], sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
			}
		}
		VIDENCTEST_PRINT("%s,%d,%d\n", __FILE__, __LINE__, (int) i);
		VIDENCTEST_FREE(pAppData->pPortDef[i], pListHead);
	}

	if (pAppData->is_face_en)
	{
		VIDENCTEST_SetPrePParameter(pAppData);
	}

	switch (pAppData->eCompressionFormat)
	{
		case OMX_VIDEO_CodingAVC:
		eError = VIDENCTEST_SetH264Parameter(pAppData);
		printf("set AVC Param\n");
		VIDENCTEST_CHECK_EXIT(eError, "Error returned from SetH264Parameter()");
		break;
		case OMX_VIDEO_CodingMJPEG:
		eError = VIDENCTEST_SetMJPEGParameter(pAppData);
		VIDENCTEST_CHECK_EXIT(eError, "Error returned from SetMJPEGParameter()");
		break;
		case OMX_VIDEO_CodingUnused:
		eError = VIDENCTEST_SetPreViewParameter(pAppData);
		VIDENCTEST_CHECK_EXIT(eError, "Error returned from VIDENCTEST_SetPreViewParameter()");
		break;

		default:
		VIDENCTEST_DPRINT("Invalid compression format value.\n");
		eError = OMX_ErrorUnsupportedSetting;
		goto EXIT;
	}

	VIDENCTEST_AllocateBuffers(pAppData);
	VIDENCTEST_CHECK_EXIT(eError, "Error at Allocation of Resources");
	VIDENCTEST_PRINT("%s,%d\n", __FILE__, __LINE__);
	pAppData->eCurrentState = VIDENCTEST_StateLoaded;

 EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * PassToReady()
 *
 *Pass the Component to Idle and allocate the buffers
 *
 * @param  pAppData MYDATA handle
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_PassToReady(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle;
	OMX_U32 nCounter;
	pHandle = pAppData->pHandle;

	eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	VIDENCTEST_CHECK_EXIT(eError, "Error from SendCommand-Idle(Init) State function");

	VIDENCTEST_PRINT("Enter App_InBuffer_AllocaBuffer\n ");
	eError = App_InBuffer_AllocaBuffer(pHandle, pAppData);
	VIDENCTEST_PRINT("Exit App_InBuffer_AllocaBuffer\n ");
	pAppData->nInBufferCount = NUM_OF_IN_BUFFERS;
	VIDENCTEST_PRINT("pAppData->nInBufferCount:%d\n", pAppData->nInBufferCount);
	VIDENCTEST_CHECK_EXIT(eError, "Error App_InBuffer_AllocaBuffer");

	VIDENCTEST_PRINT("Enter App_OutBuffer_AllocaBuffer\n");
	eError = App_OutBuffer_AllocaBuffer(pHandle, pAppData);
	VIDENCTEST_PRINT("Exit App_OutBuffer_AllocaBuffer\n");
	VIDENCTEST_CHECK_EXIT(eError, "Error App_OutBuffer_AllocaBuffer");

	if (pAppData->is_face_en)
	{
        printf("b4:pOSyncBuffer:%p\n", pAppData->pOSyncBuffer[0]);
        VIDENCTEST_PRINT("Enter App_SyncBuffer_AllocaBuffer\n");
		eError = App_SyncBuffer_AllocaBuffer(pHandle, pAppData);
        VIDENCTEST_PRINT("Exit App_SyncBuffer_AllocaBuffer\n");
        printf("aft:pOSyncBuffer:%p\n", pAppData->pOSyncBuffer[0]);
		VIDENCTEST_CHECK_EXIT(eError, "Error App_SyncBuffer_AllocaBuffer");
	}

	pAppData->nOutBufferCount = NUM_OF_OUT_BUFFERS;
	VIDENCTEST_PRINT("pAppData->nOutBufferCount:%d!\n", pAppData->nOutBufferCount);

	pAppData->bLastOutBuffer = 0;

	//#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("Exit VIDENCTEST_PassToReady %s:%d,\n ", __FILE__, __LINE__);
	//#endif

EXIT:
	return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * PassToExecuting()
 *
 *Pass the component to executing  and fill the first 4 buffers.
 *
 * @param  pAppData MYDATA handle
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_Starting(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle;
	OMX_U32 nCounter = 0;
	OMX_U32 nBuffersToSend;

	pHandle = pAppData->pHandle;
	pAppData->pComponent = (OMX_COMPONENTTYPE*) pHandle;

	/*Initialize Frame Counter */
	pAppData->nCurrentFrameIn = 0;
	pAppData->nCurrentFrameOut = 0;

	/*Validation of stopframe in cases of Pause->Resume and Stop->Restart test cases*/
	if (pAppData->eTypeOfTest == VIDENCTEST_StopRestart || pAppData->eTypeOfTest == VIDENCTEST_PauseResume)
	{
		if (pAppData->nReferenceFrame <= 1)
		{
			nBuffersToSend = 1;
		}
		else if (pAppData->nReferenceFrame < NUM_OF_IN_BUFFERS)
		{
			nBuffersToSend = pAppData->nReferenceFrame;
		}
		else
		{
			nBuffersToSend = NUM_OF_IN_BUFFERS;
		}
	}
	else
	{
		nBuffersToSend = NUM_OF_IN_BUFFERS;
	}
	/* Send FillThisBuffertoOMXVideoEncoder */

	if (pAppData->eTypeOfTest == VIDENCTEST_PartialRecord && pAppData->nReferenceFrame < NUM_OF_IN_BUFFERS)
	{
		nBuffersToSend = pAppData->nReferenceFrame;
	}
	printf("nBuffersToSend:%d\n", (int) nBuffersToSend);

	if (pAppData->is_face_en)
	{
		for (nCounter = 0; nCounter < nBuffersToSend; nCounter++)
		{
            printf("FillThisBuffer!SyncPort,nCounter:%d,pSyncBuffer:%p\n", (int) nCounter,pAppData->pOSyncBuff[nCounter]->pBuffer);
			pAppData->pOSyncBuff[nCounter]->nFilledLen = 0;
			eError = pAppData->pComponent->FillThisBuffer(pHandle, pAppData->pOSyncBuff[nCounter]);
			VIDENCTEST_CHECK_EXIT(eError, "Error in FillThisBuffer");
			//pAppData->nOutBufferCount--;
		}
	}

	/* Send EmptyThisBuffer to OMX Video Encoder */
	for (nCounter = 0; nCounter < nBuffersToSend; nCounter++)
	{
		VIDENCTEST_PRINT("b4 VIDENCTEST_fill_data!bStoreMetaData:%x\n", pAppData->pMediaStoreType[0].bStoreMetaData);
		if (pAppData->pMediaStoreType[0].bStoreMetaData == OMX_TRUE)
		{
			pAppData->pInBuff[nCounter]->nFilledLen = VIDENCTEST_Fill_MetaData(pAppData->pInBuff[nCounter],
					pAppData->fIn, pAppData->nCWidth, pAppData->nCHeight);
		}
		else
		{
			pAppData->pInBuff[nCounter]->nFilledLen = VIDENCTEST_Fill_RawData(pAppData->pInBuff[nCounter],
					pAppData->fIn, pAppData->pInPortDef->nBufferSize, pAppData->nCWidth, pAppData->nCHeight,
					pAppData->nCurrentFrameIn);
		}
		VIDENCTEST_PRINT("nCurrentFrameIn:%d  nFilledLen:%d\n", (int) pAppData->nCurrentFrameIn,
				(int) pAppData->pInBuff[nCounter]->nFilledLen);
		pAppData->nCurrentFrameIn++;

		if (pAppData->pInBuff[nCounter]->nFlags == OMX_BUFFERFLAG_EOS && pAppData->pInBuff[nCounter]->nFilledLen == 0)
		{
			pAppData->eCurrentState = VIDENCTEST_StateStopping;
			eError = pAppData->pComponent->EmptyThisBuffer(pHandle, pAppData->pInBuff[nCounter]);
			VIDENCTEST_CHECK_ERROR(eError, "Error at EmptyThisBuffer function");

			pAppData->nInBufferCount--;
			goto EXIT;
		}
		else
		{
			pAppData->pComponent->EmptyThisBuffer(pHandle, pAppData->pInBuff[nCounter]);
			pAppData->nInBufferCount--;
		}
	}

	for (nCounter = 0; nCounter < nBuffersToSend; nCounter++)
	{
		printf("FillThisBuffer!nCounter:%d\n", (int) nCounter);
		pAppData->pOutBuff[nCounter]->nFilledLen = 0;
		eError = pAppData->pComponent->FillThisBuffer(pHandle, pAppData->pOutBuff[nCounter]);
		VIDENCTEST_CHECK_EXIT(eError, "Error in FillThisBuffer");
		pAppData->nOutBufferCount--;
	}
	pAppData->eCurrentState = VIDENCTEST_StateEncoding;

#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("pAppData->eCurrentState %d\n",pAppData->eCurrentState);
#endif

EXIT:
	return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * DeInit()
 *
 *Pass the component to executing  and fill the first 4 buffers.
 *
 * @param  pAppData MYDATA handle
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_DeInit(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_ERRORTYPE eErr = OMX_ErrorNone;
	VIDENCTEST_NODE* pListHead;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	pListHead = pAppData->pMemoryListHead;

	/* Free Component Handle */
	eError = pAppData->ActionOMX_FreeHandle(pHandle);
	VIDENCTEST_CHECK_EXIT(eError, "Error in TIOMX_FreeHandle");
#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("ActionOMX_FreeHandle ok\n");
#endif
	/* De-Initialize OMX Core */
	eError = pAppData->ActionOMX_Deinit();
	VIDENCTEST_CHECK_EXIT(eError, "Error in TIOMX_Deinit");
	//#ifdef ACTION_OMX_TEST_DEBUG
	VIDENCTEST_PRINT("ActionOMX_Deinit ok\n");
	//#endif
	/* shutdown */
	fclose(pAppData->fIn);
	fclose(pAppData->fOut);

	if (pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE)
	{
		fclose(pAppData->fNalnd);
	}

	eErr = close(pAppData->IpBuf_Pipe[0]);
	if (0 != eErr && OMX_ErrorNone == eError)
	{
		eError = OMX_ErrorHardware;
		VIDENCTEST_DPRINT("Error while closing data pipe\n");
	}

	eErr = close(pAppData->OpBuf_Pipe[0]);
	if (0 != eErr && OMX_ErrorNone == eError)
	{
		eError = OMX_ErrorHardware;
		VIDENCTEST_DPRINT("Error while closing data pipe\n");
	}

	eErr = close(pAppData->eventPipe[0]);
	if (0 != eErr && OMX_ErrorNone == eError)
	{
		eError = OMX_ErrorHardware;
		VIDENCTEST_DPRINT("Error while closing data pipe\n");
	}

	eErr = close(pAppData->IpBuf_Pipe[1]);
	if (0 != eErr && OMX_ErrorNone == eError)
	{
		eError = OMX_ErrorHardware;
		VIDENCTEST_DPRINT("Error while closing data pipe\n");
	}

	eErr = close(pAppData->OpBuf_Pipe[1]);
	if (0 != eErr && OMX_ErrorNone == eError)
	{
		eError = OMX_ErrorHardware;
		VIDENCTEST_DPRINT("Error while closing data pipe\n");
	}

	eErr = close(pAppData->eventPipe[1]);
	if (0 != eErr && OMX_ErrorNone == eError)
	{
		eError = OMX_ErrorHardware;
		VIDENCTEST_DPRINT("Error while closing data pipe\n");
	}

	pAppData->fIn = NULL;
	pAppData->fOut = NULL;
	pAppData->fNalnd = NULL;

	freeCoreLib(pAppData);

	VIDENCTEST_FreeResources(pAppData);

	VIDENCTEST_CHECK_EXIT(eError, "Error in FillThisBuffer");

	VIDENCTEST_FREE(pAppData, pListHead);

	VIDENCTEST_ListDestroy(pListHead);

	pAppData = NULL;

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * HandleState()
 *
 *StateTransitions Driven.
 *
 * @param  pAppData
 * @param  nData2 - State that has been set.
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_HandleState(MYDATATYPE* pAppData, OMX_U32 eState)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	OMX_U32 nCounter = 0;
	VIDENCTEST_NODE* pListHead;
	int i = 0;
	static int startflag = 0;

	pListHead = pAppData->pMemoryListHead;

	switch (eState)
	{
		case OMX_StateLoaded:
		VIDENCTEST_PRINT("Component in OMX_StateLoaded\n");
		if (pAppData->eCurrentState == VIDENCTEST_StateLoaded)
		{
			pAppData->eCurrentState = VIDENCTEST_StateUnLoad;
		}
		break;

		case OMX_StateIdle:
		VIDENCTEST_PRINT("Component in OMX_StateIdle\n");
		if (pAppData->eCurrentState == VIDENCTEST_StateLoaded)
		{
			pAppData->eCurrentState = VIDENCTEST_StateReady;
			VIDENCTEST_PRINT("Send Component to OMX_StateExecuting\n");
			eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
			VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_SendCommand function\n");
		}
		else if (pAppData->eCurrentState == VIDENCTEST_StateWaitEvent)
		{
			if (pAppData->bStop == OMX_TRUE)
			{
				int count;
				VIDENCTEST_PRINT("Component in OMX_StateStop\n");
				pAppData->eCurrentState = VIDENCTEST_StateStop;
				/*VIDENCTEST_PRINT("Press any key to resume...");*/
				for (count = 5; count >= 0; count--)
				{
					VIDENCTEST_PRINT("App stopped: restart in %d seconds\n", count);
					sleep(1);
				}
				pAppData->bStop = OMX_FALSE;
			}

			VIDENCTEST_PRINT("OMX_CommandStateSet OMX_StateLoaded now\n");
			eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
			VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");
			pAppData->eCurrentState = VIDENCTEST_StateLoaded;

			VIDENCTEST_PRINT("Disable Input Port\n");
			/*可以disable或no*/
			//eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, VIDENC_INPUT_PORT, NULL);
			//VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_SendCommand function\n");

			eError = App_InBuffer_FreeBuffer(pHandle, pAppData);
			VIDENCTEST_CHECK_EXIT(eError, "Error App_InBuffer_FreeBuffer");

			VIDENCTEST_PRINT("Disable output Port\n");
			eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, VIDENC_OUTPUT_PORT, NULL);
			VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_SendCommand function\n");

			eError = App_OutBuffer_FreeBuffer(pHandle, pAppData);
			VIDENCTEST_CHECK_EXIT(eError, "Error App_OutBuffer_FreeBuffer");

			if (pAppData->is_face_en)
			{
				VIDENCTEST_PRINT("Disable sync put Port\n");
				/*可以disable或no，应该加OMX_SendCommand*/
				//eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, VIDENC_SYNCPUT_PORT, NULL);
				//VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_SendCommand function\n");

				eError = App_SyncBuffer_FreeBuffer(pHandle, pAppData);
				VIDENCTEST_CHECK_EXIT(eError, "Error App_SyncBuffer_FreeBuffer");
			}

			pAppData->istop = 2;
		}
		break;

		case OMX_StateExecuting:
		VIDENCTEST_PRINT("Component in OMX_StateExecuting\n");
		pAppData->eCurrentState = VIDENCTEST_StateStarting;
#ifdef ACTION_OMX_TEST_DEBUG
		VIDENCTEST_PRINT("startflag:%d doAgain:%d %d\n",startflag,pAppData->doAgain,__LINE__);
#endif
		if (0 == startflag || pAppData->doAgain)
		{
			eError = VIDENCTEST_Starting(pAppData);
		}
		else
		{
			pAppData->eCurrentState = VIDENCTEST_StateEncoding;
		}
		startflag++;
		break;

		case OMX_StatePause:
		VIDENCTEST_PRINT("Component in OMX_StatePause\n");
		pAppData->eCurrentState = VIDENCTEST_StatePause;
		int count;
		for (count = 5; count >= 0; count--)
		{
			VIDENCTEST_PRINT("App paused: resume in %d seconds\n", count);
			sleep(1);
		}
		pAppData->bStop = OMX_FALSE;
		eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
		break;

		case OMX_StateWaitForResources:
		VIDENCTEST_PRINT("Component in OMX_StateWaitForResources\n");
		break;

		case OMX_StateInvalid:
		VIDENCTEST_PRINT("Component in OMX_StateInvalid\n");
		eError = OMX_ErrorInvalidState;
	}

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * CheckArgs_AVC()
 *
 *Validates Mpeg4 input parameters
 *
 * @param  pAppData
 * @param  argv
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_CheckArgs_AVC(MYDATATYPE* pAppData, char** argv)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	pAppData->eLevelH264 = OMX_VIDEO_AVCLevel1;

	pAppData->nOutBuffSize = 1024;
	pAppData->bAllocateIBuf = OMX_TRUE;
	pAppData->bAllocateOBuf = OMX_FALSE;

	pAppData->bDeblockFilter = OMX_FALSE;
	pAppData->eControlRate = OMX_Video_ControlRateDisable;
	pAppData->nQpI = 10;

EXIT:
	return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * Confirm()
 *
 *Check what type of test, repetions to be done and takes certain path.
 *
 * @param  argc
 * @param  argv
 * @param pAppDataTmp
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_Confirm(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	pAppData->bExit = OMX_TRUE;

	eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateIdle, NULL);
	VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");
	pAppData->eCurrentState = VIDENCTEST_StateWaitEvent;

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * HandlePortDisable()
 *
 *Handles PortDisable Event
 *
 * @param  argc
 * @param  argv
 * @param pAppDataTmp
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_HandlePortDisable(MYDATATYPE* pAppData, OMX_U32 ePort)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	OMX_U32 nCounter;
	VIDENCTEST_NODE* pListHead;

	pAppData->nUnresponsiveCount = 0;
	pListHead = pAppData->pMemoryListHead;

	if (ePort == VIDENC_INPUT_PORT)
	{
		if (pAppData->is_face_en)
		{
			VIDENCTEST_PRINT("Disable sync Port now\n");
			eError = OMX_SendCommand(pHandle, OMX_CommandPortDisable, VIDENC_SYNCPUT_PORT, NULL);

			for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++)
			{
				eError = OMX_FreeBuffer(pHandle, pAppData->pSyncPortDef->nPortIndex, pAppData->pOSyncBuff[nCounter]);
			}
		}
	}
	else if ((ePort == VIDENC_OUTPUT_PORT) || (ePort == VIDENC_SYNCPUT_PORT))
	{
		VIDENCTEST_PRINT("pAppData->bExit:%d\n", pAppData->bExit);
		if (pAppData->bExit == OMX_TRUE)
		{
		}
		else
		{
			/*这个分支不会跑*/
		}
	}

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * HandlePortEnable()
 *
 *Handles PortEnable Event
 *
 * @param  pAppData
 * @param  nPort
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_HandlePortEnable(MYDATATYPE* pAppData, OMX_U32 ePort)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle = pAppData->pHandle;
	OMX_U32 nCounter;
	VIDENCTEST_NODE* pListHead;

	pAppData->nUnresponsiveCount = 0;
	pListHead = pAppData->pMemoryListHead;

	if (ePort == VIDENC_INPUT_PORT)
	{
		eError = OMX_SendCommand(pHandle, OMX_CommandPortEnable, VIDENC_OUTPUT_PORT, NULL);
		VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");

		eError = App_OutBuffer_AllocaBuffer(pHandle, pAppData);
		VIDENCTEST_CHECK_EXIT(eError, "Error App_OutBuffer_AllocaBuffer");

		if (pAppData->is_face_en)
		{
			eError = App_SyncBuffer_AllocaBuffer(pHandle, pAppData);
			VIDENCTEST_CHECK_EXIT(eError, "Error App_SyncBuffer_AllocaBuffer");
		}
	}
	else if (ePort == VIDENC_OUTPUT_PORT)
	{
		eError = OMX_SendCommand(pHandle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
		VIDENCTEST_CHECK_EXIT(eError, "Error at OMX_SendCommand function");
	}

EXIT:
    return eError;
}

/*-----------------------------------------------------------------------------*/
/**
 * HandlePortEnable()
 *
 *Handles PortEnable Event
 *
 * @param  pAppData
 * @param  nPort
 *
 * @retval OMX_ErrorNone
 *
 *
 **/
/*-----------------------------------------------------------------------------*/
OMX_ERRORTYPE VIDENCTEST_HandleEventError(MYDATATYPE* pAppData, OMX_U32 eErr, OMX_U32 nData2)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	VIDENCTEST_UNUSED_ARG(nData2);
	VIDENCTEST_PRINT("\n------VIDENCTEST ERROR-------\nOMX_EventError\n");
	VIDENCTEST_PRINT("eErr : %x \n", eErr);
	VIDENCTEST_PRINT("nData2 : %x \n", nData2);

	switch (eErr)
	{
		case OMX_ErrorNone:
		break;

		case OMX_ErrorInsufficientResources:
		case OMX_ErrorUndefined:
		case OMX_ErrorComponentNotFound:
		eError = eErr;
		break;

		case OMX_ErrorBadParameter:
		case OMX_ErrorNotImplemented:
		case OMX_ErrorUnderflow:
		case OMX_ErrorOverflow:
		break;

		case OMX_ErrorHardware:
		case OMX_ErrorStreamCorrupt:
		case OMX_ErrorPortsNotCompatible:
		case OMX_ErrorResourcesLost:
		case OMX_ErrorNoMore:
		case OMX_ErrorVersionMismatch:
		case OMX_ErrorNotReady:
		case OMX_ErrorTimeout:
		case OMX_ErrorSameState:
		case OMX_ErrorResourcesPreempted:
		eError = eErr;
		break;

		case OMX_ErrorIncorrectStateTransition:
		case OMX_ErrorIncorrectStateOperation:
		case OMX_ErrorUnsupportedSetting:
		case OMX_ErrorUnsupportedIndex:
		case OMX_ErrorBadPortIndex:
		case OMX_ErrorMax:
		default:
		;
	}

	return eError;
}

//OMX_ERRORTYPE coda_videoenc_Thread(MYDATATYPE* pAppData)
void* coda_videoenc_Thread(void* Void_pAppData)
{
	MYDATATYPE *pAppData = (MYDATATYPE *) Void_pAppData;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_HANDLETYPE pHandle;
	OMX_BUFFERHEADERTYPE* pBuffer;
	OMX_U32 nError;
	OMX_U32 nTimeCount;
	fd_set rfds;//一组文件描述字的组合
	VIDENCTEST_NODE* pListHead;
	sigset_t set;
	VIDENCTEST_PRINT("%s,%d\n", __FILE__, __LINE__);
	nTimeCount = 0;
	OMX_OTHER_EXTRADATATYPE_1_1_2 *pExtraDataType;
	OMX_U8* pTemp;
	OMX_U32* pIndexNal;
	OMX_U32 nNalSlices;
	int i;

	pListHead = pAppData->pMemoryListHead;
	pHandle = pAppData->pHandle;
	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET(pAppData->IpBuf_Pipe[0], &rfds);
		FD_SET(pAppData->OpBuf_Pipe[0], &rfds);
		FD_SET(pAppData->eventPipe[0], &rfds);
		VIDENCTEST_PRINT("%s,%d\n", __FILE__, __LINE__);
		printf("b4 sigemptyset!\n");
		sigemptyset(&set);//将参数set信号集初始化并清空
		printf("aft sigemptyset!\n");
		sigaddset(&set, SIGALRM);//将参数SIGALRM代表的信号加入至参数set 信号集里
		printf("aft sigaddset!\n");
		pAppData->nRetVal = pselect(pAppData->fdmax + 1, &rfds, NULL, NULL, NULL, &set);//检查可读性
		printf("pAppData->nRetVal:%d\n", (int) pAppData->nRetVal);

		if (pAppData->nRetVal == -1)
		{
			VIDENCTEST_PRINT("pselect()");
			VIDENCTEST_DPRINT("Error\n");
			break;
		}

		if (pAppData->nRetVal == 0)
		{
			if (nTimeCount++ > VIDENCTEST_MAX_TIME_OUTS)
			{
				VIDENCTEST_DPRINT("Application: Timeout!!!\n");
				VIDENCTEST_PRINT("\n------VIDENCTEST FATAL ERROR-------\n Component Unresponsive \n");
				VIDENCTEST_HandleError(pAppData, OMX_ErrorUndefined);
				goto EXIT;
			}
			sched_yield();
		}
		else
		{
			nTimeCount = 0;
		}

		if (FD_ISSET(pAppData->eventPipe[0], &rfds))
		{
			VIDENCTEST_PRINT("Pipe event receive\n");
			EVENT_PRIVATE* pEventPrivate = NULL;
			OMX_EVENTTYPE eEvent = -1;
			OMX_U32 nData1 = 0;
			OMX_U32 nData2 = 0;

            pthread_mutex_lock(&pAppData->pipe_mutex);
			read(pAppData->eventPipe[0], &pEventPrivate, sizeof(pEventPrivate));
            pthread_mutex_unlock(&pAppData->pipe_mutex);
			eEvent = pEventPrivate->eEvent;
			nData1 = pEventPrivate->nData1;
			nData2 = pEventPrivate->nData2;

			switch (eEvent)
			{
				case OMX_EventCmdComplete:
				switch (nData1)
				{
					case OMX_CommandStateSet:
					VIDENCTEST_PRINT("Enter to HandleState\n");
					eError = VIDENCTEST_HandleState(pAppData, nData2);
					VIDENCTEST_CHECK_ERROR(eError, "Error at HandleState function");
					VIDENCTEST_PRINT("Exit to HandleState\n");
					break;

					case OMX_CommandFlush:
					break;

					case OMX_CommandPortDisable:
					VIDENCTEST_PRINT("Enter to HandlePortDisable\n");
					VIDENCTEST_CHECK_ERROR(eError, "Error at HandlePortDisable function");
					VIDENCTEST_PRINT("Exits to HandlePortDisable\n");
					break;

					case OMX_CommandPortEnable:
					VIDENCTEST_PRINT("Enter to HandlePortEnable\n");
					/*不需要回调处理了*/
					VIDENCTEST_PRINT("Exits to HandlePortEnable\n");
					break;

					case OMX_CommandMarkBuffer:
					;
				}
				break;

				case OMX_EventError:
				eError = VIDENCTEST_HandleEventError(pAppData, nData1, nData2);
				VIDENCTEST_CHECK_ERROR(eError, "Fatal EventError");
				break;

				case OMX_EventMax:
				VIDENCTEST_PRINT("OMX_EventMax recived, nothing to do\n");
				break;

				case OMX_EventMark:
				VIDENCTEST_PRINT("OMX_EventMark recived, nothing to do\n");
				break;

				case OMX_EventPortSettingsChanged:
				VIDENCTEST_PRINT("OMX_EventPortSettingsChanged recived, nothing to do\n");
				break;

				case OMX_EventBufferFlag:
				VIDENCTEST_PRINT("OMX_EventBufferFlag recived, nothing to do\n");
				break;

				case OMX_EventResourcesAcquired:
				VIDENCTEST_PRINT("OMX_EventResourcesAcquired recived, nothing to do\n");
				break;

				case OMX_EventComponentResumed:
				VIDENCTEST_PRINT("OMX_EventComponentResumed recived, nothing to do\n");
				break;

				case OMX_EventDynamicResourcesAvailable:
				VIDENCTEST_PRINT("OMX_EventDynamicResourcesAvailable recived, nothing to do\n");
				break;

				case OMX_EventPortFormatDetected:
				VIDENCTEST_PRINT("OMX_EventPortFormatDetected recived, nothing to do\n");
				break;

				case OMX_EventKhronosExtensions:
				VIDENCTEST_PRINT("OMX_EventKhronosExtensions recived, nothing to do\n");
				break;

				case OMX_EventVendorStartUnused:
				VIDENCTEST_PRINT("OMX_EventVendorStartUnused recived, nothing to do\n");
				break;

				default:
				VIDENCTEST_CHECK_ERROR(OMX_ErrorUndefined, "Error at EmptyThisBuffer function");
				break;
			}

			VIDENCTEST_FREE(pEventPrivate, pListHead);
		}

		printf("pAppData->istop:%d!\n", pAppData->istop);
		if (pAppData->istop != 3 && pAppData->istop != 2)
		{
			if (FD_ISSET(pAppData->IpBuf_Pipe[0], &rfds))
			{
				VIDENCTEST_PRINT("Input Pipe event receive\n");
                pthread_mutex_lock(&pAppData->pipe_mutex);
				read(pAppData->IpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                pthread_mutex_unlock(&pAppData->pipe_mutex);
				pAppData->nInBufferCount++;

				//VIDENCTEST_PRINT("pAppData->eCurrentState %d, %s:%d,\n ", pAppData->eCurrentState ,__FILE__,__LINE__);
				if (pAppData->eCurrentState == VIDENCTEST_StateEncoding)
				{
					if (pBuffer->nOutputPortIndex == 0)
					{
						if (pAppData->pMediaStoreType[0].bStoreMetaData == OMX_TRUE)
						{
							pBuffer->nFilledLen = VIDENCTEST_Fill_MetaData(pBuffer, pAppData->fIn, pAppData->nCWidth,
									pAppData->nCHeight);
#if  Enable_IDR_Refresh
							if(pAppData->nCurrentFrameIn == 7)
							{
								OMX_CONFIG_INTRAREFRESHVOPTYPE IDR_Refresh;
								IDR_Refresh.nSize = sizeof(OMX_CONFIG_INTRAREFRESHVOPTYPE);
								IDR_Refresh.nVersion.s.nVersionMajor = 0x1;
								IDR_Refresh.nVersion.s.nVersionMinor = 0x1;
								IDR_Refresh.nVersion.s.nRevision = 0x0;
								IDR_Refresh.nVersion.s.nStep = 0x0;
								IDR_Refresh.nPortIndex = VIDENC_OUTPUT_PORT;
								IDR_Refresh.IntraRefreshVOP = OMX_TRUE;
								eError = OMX_SetConfig (pHandle, OMX_IndexConfigVideoIntraVOPRefresh, &IDR_Refresh);
								printf("OMX_SetConfig:OMX_IndexConfigVideoIntraVOPRefresh!\n");
							}
#endif
						}
						else
						{
							pBuffer->nFilledLen = VIDENCTEST_Fill_RawData(pBuffer, pAppData->fIn,
									pAppData->pInPortDef->nBufferSize, pAppData->nCWidth, pAppData->nCHeight,
									pAppData->nCurrentFrameIn);
						}
						VIDENCTEST_PRINT("nCurrentFrameIn:%d  nFilledLen:%d\n", (int) pAppData->nCurrentFrameIn,
								(int) pBuffer->nFilledLen);
						pAppData->nCurrentFrameIn++;
					}

					if (pAppData->nCurrentFrameIn == 10000)
					{
						pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
						pBuffer->nFilledLen = 0;
					}

					if (pBuffer->nFlags == OMX_BUFFERFLAG_EOS && pBuffer->nFilledLen == 0)
					{
						pAppData->eCurrentState = VIDENCTEST_StateStopping;
						//eError = pAppData->pComponent->EmptyThisBuffer(pHandle, pBuffer);
						VIDENCTEST_CHECK_ERROR(eError, "Error at EmptyThisBuffer function");
						pAppData->nInBufferCount--;
					}

					if (pAppData->eTypeOfTest != VIDENCTEST_FullRecord)
					{
						if (pAppData->nCurrentFrameIn == pAppData->nReferenceFrame)
						{
							pAppData->eCurrentState = VIDENCTEST_StateStopping;
							if (pAppData->eTypeOfTest == VIDENCTEST_PauseResume)
							{
								eError = pAppData->pComponent->EmptyThisBuffer(pHandle, pBuffer);
								VIDENCTEST_CHECK_ERROR(eError, "Error at EmptyThisBuffer function");
								pAppData->nInBufferCount--;
							}
						}
					}

					if (pAppData->eCurrentState == VIDENCTEST_StateEncoding)
					{
						eError = pAppData->pComponent->EmptyThisBuffer(pHandle, pBuffer);
						VIDENCTEST_CHECK_ERROR(eError, "Error at EmptyThisBuffer function");
						pAppData->nInBufferCount--;
					}
				}

				if (pAppData->eCurrentState == VIDENCTEST_StateStopping)
				{
					//VIDENCTEST_PRINT("main %s:%d,\n ", __FILE__,__LINE__);
					break;
					if (pAppData->nInBufferCount == NUM_OF_IN_BUFFERS)
					{
						pAppData->eCurrentState = VIDENCTEST_StateConfirm;
					}
				}
			}

			if (FD_ISSET(pAppData->OpBuf_Pipe[0], &rfds))
			{
				VIDENCTEST_PRINT("Output Pipe event receive\n");
                pthread_mutex_lock(&pAppData->pipe_mutex);
				read(pAppData->OpBuf_Pipe[0], &pBuffer, sizeof(pBuffer));
                pthread_mutex_unlock(&pAppData->pipe_mutex);
				if (pBuffer->nOutputPortIndex == 1)
					pAppData->nOutBufferCount++;
				VIDENCTEST_PRINT("nOutputPortIndex:%d,  pAppData->nOutBufferCount:%d\n",
						(int) pBuffer->nOutputPortIndex, pAppData->nOutBufferCount);
				VIDENCTEST_PRINT("nFlags:%d nFilledLen:%d   %d  %d\n", (int) pBuffer->nFlags,
						(int) pBuffer->nFilledLen, OMX_BUFFERFLAG_EOS, (int) pBuffer->nOutputPortIndex);
				/* check is it is the last buffer */
				if ((pBuffer->nFlags & OMX_BUFFERFLAG_EOS) || (pAppData->eTypeOfTest != VIDENCTEST_FullRecord
						&& pAppData->nCurrentFrameIn == pAppData->nReferenceFrame))
				{
					pAppData->bLastOutBuffer = 1;
				}

				/*App sends last buffer as null buffer, so buffer with EOS contains only garbage*/
				if (pBuffer->nOutputPortIndex == 1)
				{
					pAppData->nCurrentFrameOut++;
					//VIDENCTEST_PRINT("nCurrentFrameOut:%ld %ld\n",pAppData->nCurrentFrameOut,*(unsigned long*)pBuffer->pBuffer);

					printf("pBuffer:%lx,  nOffset:%d, nFilledLen:%d\n", (unsigned long) pBuffer->pBuffer,(int) pBuffer->nOffset, (int) pBuffer->nFilledLen);
					if (pAppData->pMediaStoreType[1].bStoreMetaData == OMX_TRUE)
					{
						printf("OMX_TRUE,handle:%lx  phy:%lx\n",(unsigned long) ((video_metadata_t*) (pBuffer->pBuffer))->handle,
								((video_handle_t*) ((video_metadata_t*) (pBuffer->pBuffer))->handle)->phys_addr);
						printf("noffset:%d, nfilledlen:%d\n",((video_metadata_t*) (pBuffer->pBuffer))->vce_attribute.noffset,
								((video_metadata_t*) (pBuffer->pBuffer))->vce_attribute.nfilledlen);
						fwrite(omx_getvir_from_phy((void *) (((video_handle_t*) ((video_metadata_t*) (pBuffer->pBuffer))->handle)->phys_addr
												+ ((video_metadata_t*) (pBuffer->pBuffer))->vce_attribute.noffset)), 1,((video_metadata_t*) (pBuffer->pBuffer))->vce_attribute.nfilledlen, pAppData->fOut);
					}
					else
					{
						printf("OMX_FALSE,pBuffer:%lx\n", (unsigned long) pBuffer->pBuffer);
						fwrite(pBuffer->pBuffer + pBuffer->nOffset, 1, pBuffer->nFilledLen, pAppData->fOut);
					}

					nError = ferror(pAppData->fOut);
					if (nError != 0)
					{
						VIDENCTEST_DPRINT("ERROR: writing to file\n");
					}

					nError = fflush(pAppData->fOut);
					if (nError != 0)
					{
						VIDENCTEST_DPRINT("ERROR: flushing file\n");
					}

					if (pAppData->NalFormat == VIDENC_TEST_NAL_SLICE)
					{
						nNalSlices = 1;
						fwrite(&nNalSlices, 1, sizeof(OMX_U32), pAppData->fNalnd);
						fwrite(&(pBuffer->nFilledLen), 1, sizeof(OMX_U32), pAppData->fNalnd);
						nError = ferror(pAppData->fNalnd);
						if (nError != 0)
						{
							VIDENCTEST_DPRINT("ERROR: writing to file\n");
						}
						nError = fflush(pAppData->fNalnd);
						if (nError != 0)
						{
							VIDENCTEST_DPRINT("ERROR: flushing file\n");
						}
					}

					/* Check if it is Nal format and if it has extra data*/
					if ((pAppData->NalFormat == VIDENC_TEST_NAL_FRAME) && (pBuffer->nFlags & OMX_BUFFERFLAG_EXTRADATA))
					{
						pTemp = pBuffer->pBuffer + pBuffer->nOffset + pBuffer->nFilledLen + 3;
						pExtraDataType = (OMX_OTHER_EXTRADATATYPE_1_1_2*) (((OMX_U64) pTemp) & ~3);
						pIndexNal = (OMX_U32*) (pExtraDataType->data);

						nNalSlices = *pIndexNal;
						fwrite(pIndexNal, 1, sizeof(OMX_U32), pAppData->fNalnd);

						while (nNalSlices--)
						{
							pIndexNal++;
							fwrite(pIndexNal, 1, sizeof(OMX_U32), pAppData->fNalnd);
							nError = ferror(pAppData->fNalnd);
							if (nError != 0)
							{
								VIDENCTEST_DPRINT("ERROR: writing to file\n");
							}
							nError = fflush(pAppData->fNalnd);
							if (nError != 0)
							{
								VIDENCTEST_DPRINT("ERROR: flushing file\n");
							}
						}
					}
				}
                else
                {
                    printf("faceDet result!\n");
                }

				if(pAppData->eCurrentState == VIDENCTEST_StateEncoding ||
					pAppData->eCurrentState == VIDENCTEST_StateStopping) 
				{
					pBuffer->nFilledLen = 0;
					eError = pAppData->pComponent->FillThisBuffer(pHandle, pBuffer);
					VIDENCTEST_CHECK_ERROR(eError, "Error at FillThisBuffer function");
					if (pBuffer->nOutputPortIndex == 1)
						pAppData->nOutBufferCount--;
				}
			}

			if (pAppData->eCurrentState == VIDENCTEST_StateConfirm)
			{
				if (pAppData->bLastOutBuffer)
				{
					VIDENCTEST_PRINT("Number of Input  Buffer at Client Side : %i\n", pAppData->nInBufferCount);
					VIDENCTEST_PRINT("Number of Output Buffer at Client Side : %i\n", pAppData->nOutBufferCount);
					VIDENCTEST_PRINT("Frames Out: %d\n", (int) pAppData->nCurrentFrameOut);
					VIDENCTEST_PRINT("Enter to Confirm Function\n");
					eError = VIDENCTEST_Confirm(pAppData);
					VIDENCTEST_CHECK_ERROR(eError, "Error at VIDENCTEST_Confirm function");
					VIDENCTEST_PRINT("Exits to Confirm Function\n");
				}
			}
		}

		if (pAppData->eCurrentState == VIDENCTEST_StateUnLoad)
		{
			VIDENCTEST_PRINT("Exiting while\n");
			break;
		}
		sched_yield();

		//VIDENCTEST_PRINT("pAppData->nCurrentFrameIn %d\n",pAppData->nCurrentFrameIn);
		if(pAppData->nCurrentFrameIn >= Test_Frame_Nums && pAppData->eCompressionFormat == OMX_VIDEO_CodingMJPEG && pAppData->istop <= 1)
		{
			VIDENCTEST_PRINT("pAppData->nCurrentFrameIn %d\n", (int) pAppData->nCurrentFrameIn);
			pAppData->istop = 1;
			printf("pAppData->istop! 0/1  to  1,%d\n", __LINE__);
		}

		if (pAppData->istop == 1)
		{
			VIDENCTEST_PRINT("%s,%d\n", __FILE__, __LINE__);
			pAppData->eCurrentState = VIDENCTEST_StateConfirm;
			VIDENCTEST_Confirm(pAppData);
			pAppData->istop = 3;
			printf("pAppData->istop! 1  to  3,%d\n", __LINE__);
			// break;
		}

		/*2012.08.07fix 为了等待处理完event命令后再结束*/
		if ((pAppData->istop == 2))
		{
			VIDENCTEST_PRINT("1stoop now!pAppData->istop!  =2,%d\n", __LINE__);
		}

		/*测试结果：不通过下面退出*/
		if ((pAppData->istop == 2) && (pAppData->nRetVal == 0))
		{
			VIDENCTEST_PRINT("2stoop now!pAppData->istop!  =2,%d\n", __LINE__);
			//usleep(400000);
			break;
		}
		usleep(4000);
	}

	if (pAppData->nCurrentFrameIn != pAppData->nCurrentFrameOut)
	{
		VIDENCTEST_PRINT("App: Warning!!! FrameIn: %d FrameOut: %d\n", (int)pAppData->nCurrentFrameIn, (int)pAppData->nCurrentFrameOut);
	}

	if (pAppData->eCompressionFormat == OMX_VIDEO_CodingMJPEG)
	{
		VIDENCTEST_DeInit(pAppData);
	}

	EXIT:
	//return eError;
	return NULL;
}

static OMX_ERRORTYPE coda_videoenc_init(MYDATATYPE* pParaConf, MYDATATYPE* pAppData)
{
	int retval = 0;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	VIDENCTEST_PRINT("Enter to CheckArgs\n");

	{
		pAppData->nMIRRate = VIDENCTEST_USE_DEFAULT_VALUE;
		pAppData->nResynchMarkerSpacing = VIDENCTEST_USE_DEFAULT_VALUE;
		pAppData->nIntraFrameInterval = VIDENCTEST_USE_DEFAULT_VALUE;
		pAppData->nEncodingPreset = VIDENCTEST_USE_DEFAULT_VALUE_UI;
		pAppData->nUnrestrictedMV = (OMX_U8) VIDENCTEST_USE_DEFAULT_VALUE_UI;
		pAppData->NalFormat = 0;
		pAppData->nQPIoF = 0;
		pAppData->bForceIFrame = 0;

		pAppData->szInFile = pParaConf->szInFile;
		pAppData->fIn = fopen(pAppData->szInFile, "r");
		if (!pAppData->fIn)
		{
			VIDENCTEST_PRINT("Error: failed to open the file <%s>", pAppData->szInFile);
			eError = OMX_ErrorBadParameter;
			goto EXIT;
		}

		pAppData->szOutFile = pParaConf->szOutFile;
		pAppData->fOut = fopen(pAppData->szOutFile, "w");
		if (!pAppData->fOut)
		{
			VIDENCTEST_DPRINT("Error: failed to open the file <%s>", pAppData->szOutFile);
			eError = OMX_ErrorBadParameter;
			goto EXIT;
		}

		pAppData->nWidth = pParaConf->nWidth;
		pAppData->nHeight = pParaConf->nHeight;
		pAppData->nCWidth = pParaConf->nCWidth;
		pAppData->nCHeight = pParaConf->nCHeight;

		if (pAppData->nWidth & 15)
		{
			VIDENCTEST_PRINT("**Warning: Input Argument WIDTH is not multiple of 16. \n");
		}

		if (pAppData->nHeight & 15)
		{
			VIDENCTEST_PRINT("**Warning: Input Argument HEIGHT is not multiple of 16. \n");
		}

		pAppData->eColorFormat = pParaConf->eColorFormat;

		pAppData->nBitrate = pParaConf->nBitrate;
		if (pAppData->nBitrate > 10000000)
		{
            VIDENCTEST_PRINT("**Warning: Input argument BITRATE outside of tested range, behavior of component unknown.\n");
		}

		pAppData->nFramerate = pParaConf->nFramerate;
		if (pAppData->nFramerate < 7 || pAppData->nFramerate > 30)
		{
            VIDENCTEST_PRINT("**Warning: Input argument FRAMERATE outside of tested range, behavior of component unknown %d.\n",pAppData->nFramerate);
		}

		pAppData->eCompressionFormat = pParaConf->eCompressionFormat;
		if (pAppData->eCompressionFormat == OMX_VIDEO_CodingAVC)
		{
			VIDENCTEST_PRINT("ENC IS AVC\n");
			eError = VIDENCTEST_CheckArgs_AVC(pAppData, NULL);
			pAppData->bAllocateIBuf = OMX_TRUE; /*OMX_TRUE*/
			pAppData->bAllocateOBuf = OMX_FALSE;
		}
		else if (pAppData->eCompressionFormat == OMX_VIDEO_CodingMJPEG)
		{
			VIDENCTEST_PRINT("ENC IS MJPG\n");
			pAppData->bAllocateIBuf = OMX_TRUE;/*OMX_TRUE*/
			pAppData->bAllocateOBuf = OMX_FALSE;
			pAppData->nQpI = 10;
			pAppData->nQualityfactor = 75;//jpegQuality;
			pAppData->appthumb.mjpg_thumbNailEnable = 0;//thumbNailEnable;
			pAppData->appthumb.mjpg_thumbNailWidth = 160;//thumbNailWidth;
			pAppData->appthumb.mjpg_thumbNailHeight = 120;//thumbNailHeight;
		}
		else if (pAppData->eCompressionFormat == OMX_VIDEO_CodingUnused)
		{
			VIDENCTEST_PRINT("ENC IS PreView\n");
			pAppData->bAllocateIBuf = OMX_TRUE;/*OMX_TRUE*/
			pAppData->bAllocateOBuf = OMX_FALSE;
		}

		pAppData->is_face_en = pParaConf->is_face_en;

		if (1)
		{
			pAppData->eTypeOfTest = VIDENCTEST_FullRecord/*VIDENCTEST_StopRestart*/;
			VIDENCTEST_PRINT("**Warning: Input Arguments TYPE OF TEST is not include input. Using Default value VIDENCTEST_FullRecord.\n");
		}

		if (1)
		{
			pAppData->nReferenceFrame = 0;
			VIDENCTEST_PRINT("**Warning: Input Arguments nReferenceFrame has not been specified. Using Default value 0.\n");
		}

		if (1)
		{
			pAppData->nNumberOfTimesTodo = 1;
		}

		if (pAppData->NalFormat == VIDENC_TEST_NAL_FRAME || pAppData->NalFormat == VIDENC_TEST_NAL_SLICE)
		{
			VIDENCTEST_MALLOC(pAppData->szOutFileNal, strlen(pAppData->szOutFile)+3, char, pAppData->pMemoryListHead);
			strcpy(pAppData->szOutFileNal, pAppData->szOutFile);
			strcat(pAppData->szOutFileNal, "nd");
			pAppData->fNalnd = fopen(pAppData->szOutFileNal, "w");
			if (!pAppData->szOutFileNal)
			{
				VIDENCTEST_DPRINT("Error: failed to open the file <%s>", pAppData->szOutFileNal);
				eError = OMX_ErrorBadParameter;
				goto EXIT;
			}
		}
	}
	VIDENCTEST_PRINT("Exit to CheckArgs\n");

	pAppData->bAllocateSBuf = OMX_FALSE;

    pthread_mutex_init(&pAppData->pipe_mutex,NULL);
    
	VIDENCTEST_PRINT("Enter to PassToLoaded\n");
	eError = VIDENCTEST_PassToLoaded(pAppData);
	VIDENCTEST_CHECK_ERROR(eError, "Error at Initialization of Component");
	VIDENCTEST_PRINT("Exit to PassToLoaded\n");

	VIDENCTEST_PRINT("Enter to PassToReady\n");
	eError = VIDENCTEST_PassToReady(pAppData);
	VIDENCTEST_CHECK_ERROR(eError, "Error at Passing to ComponentReady State");
	VIDENCTEST_PRINT("Exit to PassToReady\n");

	retval = pthread_create(&(pAppData->Enc_mainThread), NULL, coda_videoenc_Thread, (void *) pAppData);
	if (retval != 0 || !pAppData->Enc_mainThread)
	{
		VIDENCTEST_PRINT("error in creat Tractor_PlayThread!\n");
		goto EXIT;
	}

	pAppData->istop = 0;
	printf("pAppData->istop!  to  0,%d\n", __LINE__);

EXIT:
	return eError;
}

static OMX_ERRORTYPE coda_videoenc_encode(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

EXIT:
	return eError;
}

static OMX_ERRORTYPE coda_videoenc_dispose(MYDATATYPE* pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	while (pAppData->nCurrentFrameOut <= Test_Frame_Nums && pAppData->istop == 0)
	{
		//VIDENCTEST_PRINT("enc frm is %d\n",pAppData->nCurrentFrameOut);
		usleep(40000);
	}

	VIDENCTEST_PRINT("enc frm is %d\n", (int) pAppData->nCurrentFrameOut);

	if (pAppData->istop == 0)
		pAppData->istop = 1;
	printf("pAppData->istop!  to  1,%d\n", __LINE__);
	usleep(100000);
	//pAppData->istop = 2;
	usleep(100000);
	int retval = pthread_join(pAppData->Enc_mainThread, (void*) &eError);
	if (0 != retval)
	{
		VIDENCTEST_PRINT("error in pthread_join Coastguard_RePlayThread!\n");
	}
	pthread_mutex_destroy(&pAppData->pipe_mutex);
	
	//VIDENCTEST_PRINT("enc frm is %d\n",pAppData->nCurrentFrameOut);
	if (pAppData->eCompressionFormat != OMX_VIDEO_CodingMJPEG)
	{
		VIDENCTEST_PRINT("VIDENCTEST_DeInit now %d\n", __LINE__);
		eError = VIDENCTEST_DeInit(pAppData);
	}
	//VIDENCTEST_PRINT("enc frm is %d\n",pAppData->nCurrentFrameOut);
	return eError;
}

OMX_ERRORTYPE coda_videoenc_open(MYDATATYPE** ppAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	VIDENCTEST_NODE* pMemoryListHead;
	MYDATATYPE* pAppData;

	eError = VIDENCTEST_ListCreate(&pMemoryListHead);
	VIDENCTEST_CHECK_EXIT(eError, "Error at Creating Memory List");
	VIDENCTEST_MALLOC(pAppData, sizeof(MYDATATYPE), MYDATATYPE, pMemoryListHead);
	VIDENCTEST_CHECK_EXIT(eError, "Error at Allocating MYDATATYPE structure");
	*ppAppData = pAppData;
	pAppData->pMemoryListHead = pMemoryListHead;
	pAppData->eCurrentState = VIDENCTEST_StateUnLoad;
	pAppData->init = coda_videoenc_init;
	pAppData->encode = coda_videoenc_encode;
	pAppData->dispose = coda_videoenc_dispose;
	pAppData->doAgain = OMX_TRUE;
	printf("end of open!\n");
EXIT:
	return eError;
}



/*
 * Usage:
 * ./omx_vce_test [width] [height] [file.yuv]
 * if not set width and height, then use default setting 640x480 2_640_480_300.yuv
 * fps fixed to 15
 */
#define DEFAULT_W 1920
#define DEFAULT_H 1080
#define DEFAULT_FPS 30
#define DEFAULT_FORMAT OMX_COLOR_FormatYUV420Planar;	//input format, such as YUV420P
int main(int argc, char *argv[])
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	MYDATATYPE* videoEncHandle, *pAppData;

	/*parameters config*/
	MYDATATYPE paraConf;
	paraConf.szOutFile = "out.avc";	//output file name

#if WDF_FORMAT_TEST
	paraConf.szInFile = wfd_argb_file;
	wfd_format = OMX_COLOR_Format32bitARGB8888;
	//paraConf.szInFile = wfd_rgb565_file;
	//wfd_format = OMX_COLOR_Format16bitRGB565;
	//paraConf.szInFile = wfd_yuv420sp_file;
	//wfd_format = OMX_COLOR_FormatYUV420SemiPlanar;
	//paraConf.szInFile = wfd_yuv420p_file;
	//wfd_format = OMX_COLOR_FormatYUV420Planar;
	paraConf.eColorFormat = OMX_COLOR_FormatAndroidOpaque;/*OMX_COLOR_Format32bitARGB8888*/
#else
	//paraConf.szInFile = "/data/horsecab_720p_30f.yuv";
	//paraConf.szInFile = "/data/riverbed_30f.yuv";
	//paraConf.szInFile = "/data/flower_vga.yuv";
	//paraConf.szInFile = "/data/camera_data/bus_kvga_sp.yvu";
	//paraConf.szInFile = "/data/bus_kvga.yuv";
	paraConf.szInFile = "bus_vga.yuv";	//input file name
	paraConf.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;	//input format, such as YUV420SP
#endif
	paraConf.eColorFormat = DEFAULT_FORMAT;//OMX_COLOR_FormatYUV420Planar;	//input format, such as YUV420P

        if (argc == 1)
        {
            paraConf.nCWidth = DEFAULT_W;		//src width		640
            paraConf.nCHeight = DEFAULT_H;	//src height	480
            paraConf.nWidth = DEFAULT_W;		//dst width		640
            paraConf.nHeight = DEFAULT_H;		//dst height	480
        }
        else if (argc >= 4)
        {
            //TODO: parameters checking 
            paraConf.nCWidth = atoi(argv[1]);		//src width		640
            paraConf.nCHeight = atoi(argv[2]);	//src height	480
            paraConf.nWidth = atoi(argv[1]);		//dst width		640
            paraConf.nHeight = atoi(argv[2]);		//dst height	480
            paraConf.szInFile = argv[3];	//TODO: input file name checking
            if ( argv[4] != NULL && strcmp(argv[4], "yuv420sp") == 0)
            {
                printf("using yuv420sp input format\n");
	        paraConf.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;	//input format, such as YUV420SP
            }
        }   
        else
        {
            printf("Please using correct format: ./omx_vce_test width height (support list: 640*480 1920*1080)");
            goto EXIT;
        }

	paraConf.is_face_en = 0;	//close face detection
	paraConf.nBitrate = 0/*250000*/;	//0 close rate control
	paraConf.nFramerate = DEFAULT_FPS;	//frame rate	30
	paraConf.eCompressionFormat = OMX_VIDEO_CodingAVC;	//OMX_VIDEO_CodingAVC    H264
    													//OMX_VIDEO_CodingMJPEG  JPEG
    													//OMX_VIDEO_CodingUnused Preview

	/*open*/
	VIDENCTEST_PRINT("Enter to coda_videoenc_open\n");
	eError = coda_videoenc_open(&videoEncHandle);
	if (eError != OMX_ErrorNone)
	{
		VIDENCTEST_PRINT("open coda_videoenc error!!\n");
		goto EXIT;
	}
	VIDENCTEST_PRINT("Exit to coda_videoenc_open\n");
	pAppData = videoEncHandle;

	/*init*/
	VIDENCTEST_PRINT("Enter to coda_videoenc_init\n");
	eError = videoEncHandle->init(&paraConf, pAppData);
	VIDENCTEST_CHECK_ERROR(eError, "init coda_videoenc error!!\n");
	VIDENCTEST_PRINT("Exit to coda_videoenc_init\n");

	/*encode*/
	VIDENCTEST_PRINT("Enter to coda_videoenc_encode\n");
	eError = videoEncHandle->encode(pAppData);
	VIDENCTEST_CHECK_ERROR(eError, "encode coda_videoenc error!!\n");
	VIDENCTEST_PRINT("Exit to coda_videoenc_encode\n");

	/*dispose*/
	VIDENCTEST_PRINT("Enter to coda_videoenc_dispose\n");
	eError = videoEncHandle->dispose(pAppData);
	VIDENCTEST_CHECK_ERROR(eError, "dispose coda_videoenc error!!\n");
	VIDENCTEST_PRINT("Exit to coda_videoenc_dispose\n");

EXIT:
	return eError;
}

