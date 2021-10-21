#ifndef __ACT_COM_H__
#define __ACT_COM_H__

#include "VideoEncTest.h"
#include "omx_malloc.h"

extern void *actal_malloc_uncache_fd(int size, void *phy_add, int* fd_out);


static OMX_ERRORTYPE App_InBuffer_AllocaBuffer(OMX_HANDLETYPE pHandle, MYDATATYPE*pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int nCounter;

	for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++)
	{
		if (pAppData->bAllocateIBuf == OMX_TRUE)
		{
			unsigned int inbuf_size = pAppData->pInPortDef->nBufferSize;
			unsigned long vir_addr, phy_addr,fd_out;
			if (pAppData->pMediaStoreType[0].bStoreMetaData == OMX_TRUE)
			{
				pAppData->pIBuffer[nCounter] = calloc(1, sizeof(video_metadata_t));

				((video_metadata_t*) (pAppData->pIBuffer[nCounter]))->handle = calloc(1, sizeof(video_handle_t));
				//phy_addr = (unsigned long)omx_malloc_phy(inbuf_size,&vir_addr);
				vir_addr = (unsigned long) actal_malloc_uncache_fd(inbuf_size, (int *) (&phy_addr), (int *) (&fd_out));
				((video_handle_t*) (((video_metadata_t*) (pAppData->pIBuffer[nCounter]))->handle))->ion_share_fd
						= fd_out;
				((video_handle_t*) (((video_metadata_t*) (pAppData->pIBuffer[nCounter]))->handle))->size = inbuf_size;
				((video_handle_t*) (((video_metadata_t*) (pAppData->pIBuffer[nCounter]))->handle))->phys_addr
						= phy_addr;
				printf("test,malloc ion addr! phy:%lx,vir:%lx,len:%x\n", phy_addr, vir_addr, inbuf_size);
				printf("bStoreMetaData,in!pAppData,phy_addr:%lx,vir_addr:%lx,inbuf_size:%x,fd_out:%lx\n", phy_addr,
						vir_addr, inbuf_size, fd_out);

				eError = OMX_UseBuffer(pHandle, &pAppData->pInBuff[nCounter], pAppData->pInPortDef->nPortIndex,
						pAppData, sizeof(video_metadata_t), pAppData->pIBuffer[nCounter]);

				VIDENCTEST_PRINT("malloc ptr is buffers %lx,%d\n", (unsigned long) pAppData->pInBuff[nCounter]->pBuffer,
						inbuf_size);
				VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_UseBuffer function");
			}
			else
			{
				phy_addr = (unsigned long) omx_malloc_phy(inbuf_size, &vir_addr);
				pAppData->pIBuffer[nCounter] = (OMX_U8*) vir_addr;
				printf("no bStoreMetaData,in!pAppData,phy_addr:%lx,vir_addr:%lx,inbuf_size:%x\n", phy_addr, vir_addr,
						inbuf_size);

				eError = OMX_UseBuffer(pHandle, &pAppData->pInBuff[nCounter], pAppData->pInPortDef->nPortIndex,
						pAppData, pAppData->pInPortDef->nBufferSize, pAppData->pIBuffer[nCounter]);

				VIDENCTEST_PRINT("malloc ptr is buffers %lx,%d\n", (unsigned long) pAppData->pInBuff[nCounter]->pBuffer,
						inbuf_size);
				VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_UseBuffer function");
			}
		}
		else
		{
			eError = OMX_AllocateBuffer(pHandle, &pAppData->pInBuff[nCounter], pAppData->pInPortDef->nPortIndex,
					pAppData, pAppData->pInPortDef->nBufferSize);
			VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_AllocateBuffer State function");
		}
	}

EXIT:
	return eError;
}

static OMX_ERRORTYPE App_InBuffer_FreeBuffer(OMX_HANDLETYPE pHandle, MYDATATYPE*pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int nCounter;

	for (nCounter = 0; nCounter < NUM_OF_IN_BUFFERS; nCounter++)
	{
		if (pAppData->bAllocateIBuf == OMX_TRUE)
		{
			if (pAppData->pIBuffer[nCounter])
			{
				unsigned long phy_addr;
				if (pAppData->pMediaStoreType[0].bStoreMetaData == OMX_TRUE)
				{
					omx_free_phy( (void *)((video_handle_t*)( ((video_metadata_t*)(pAppData->pIBuffer[nCounter]))->handle ))->phys_addr );
					free(((video_metadata_t*) (pAppData->pIBuffer[nCounter]))->handle);

					free(pAppData->pIBuffer[nCounter]);
				}
				else
				{
					phy_addr = (unsigned long) omx_getphy_from_vir((void*) (pAppData->pIBuffer[nCounter]));
					omx_free_phy((void*) phy_addr);
				}
				pAppData->pIBuffer[nCounter] = NULL;
			}
		}

		VIDENCTEST_PRINT("free ptr buffer is %lx\n", (unsigned long) (pAppData->pInBuff[nCounter]->pBuffer));
		eError = OMX_FreeBuffer(pHandle, pAppData->pInPortDef->nPortIndex, pAppData->pInBuff[nCounter]);
	}

EXIT:
	return eError;
}

static OMX_ERRORTYPE App_OutBuffer_AllocaBuffer(OMX_HANDLETYPE pHandle, MYDATATYPE*pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int nCounter;

	for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++)
	{
		if (pAppData->bAllocateOBuf == OMX_TRUE)
		{
			unsigned int outbuf_size = pAppData->pOutPortDef->nBufferSize;
			unsigned long vir_addr, phy_addr,fd_out;
			if (pAppData->pMediaStoreType[1].bStoreMetaData == OMX_TRUE)
			{
				pAppData->pOBuffer[nCounter] = calloc(1, sizeof(video_metadata_t));

				((video_metadata_t*) (pAppData->pOBuffer[nCounter]))->handle = calloc(1, sizeof(video_handle_t));
				//phy_addr = (unsigned long)omx_malloc_phy(outbuf_size,&vir_addr);
				vir_addr = (unsigned long) actal_malloc_uncache_fd(outbuf_size, (int *) (&phy_addr), (int *) (&fd_out));
				((video_handle_t*) (((video_metadata_t*) (pAppData->pOBuffer[nCounter]))->handle))->ion_share_fd
						= fd_out;
				((video_handle_t*) (((video_metadata_t*) (pAppData->pOBuffer[nCounter]))->handle))->size = outbuf_size;
				((video_handle_t*) (((video_metadata_t*) (pAppData->pOBuffer[nCounter]))->handle))->phys_addr
						= phy_addr;
				printf("test,malloc ion addr! phy:%lx,vir:%lx,len:%x\n", phy_addr, vir_addr, outbuf_size);
				printf("bStoreMetaData,out!pAppData,phy_addr:%lx,vir_addr:%lx,outbuf_size:%x,fd_out:%lx\n", phy_addr,
						vir_addr, outbuf_size, fd_out);

				eError = OMX_UseBuffer(pHandle, &pAppData->pOutBuff[nCounter], pAppData->pOutPortDef->nPortIndex,
						pAppData, sizeof(video_metadata_t), pAppData->pOBuffer[nCounter]);
				VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_UseBuffer function");
			}
			else
			{
				phy_addr = (unsigned long) omx_malloc_phy(outbuf_size, &vir_addr);
				pAppData->pOBuffer[nCounter] = (OMX_U8 *) vir_addr;
				printf("no bStoreMetaData,out!pAppData,phy_addr:%lx,vir_addr:%lx,outbuf_size:%x\n", phy_addr, vir_addr,
						outbuf_size);

				eError = OMX_UseBuffer(pHandle, &pAppData->pOutBuff[nCounter], pAppData->pOutPortDef->nPortIndex,
						pAppData, pAppData->pOutPortDef->nBufferSize, pAppData->pOBuffer[nCounter]);
				VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_UseBuffer function");
			}
		}
		else
		{
			eError = OMX_AllocateBuffer(pHandle, &pAppData->pOutBuff[nCounter], pAppData->pOutPortDef->nPortIndex,
					pAppData, pAppData->pOutPortDef->nBufferSize);
			VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_AllocateBuffer function");
		}
	}

EXIT:
	return eError;
}

static OMX_ERRORTYPE App_OutBuffer_FreeBuffer(OMX_HANDLETYPE pHandle, MYDATATYPE*pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int nCounter;

	for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++)
	{
		if (pAppData->bAllocateOBuf == OMX_TRUE)
		{
			if (pAppData->pOBuffer[nCounter])
			{
				unsigned long phy_addr;
				if (pAppData->pMediaStoreType[1].bStoreMetaData == OMX_TRUE)
				{
					omx_free_phy( (void *) ((video_handle_t*)( ((video_metadata_t*)(pAppData->pOBuffer[nCounter]))->handle ))->phys_addr );
					free(((video_metadata_t*) (pAppData->pOBuffer[nCounter]))->handle);

					free(pAppData->pOBuffer[nCounter]);
				}
				else
				{
					phy_addr = (unsigned long) omx_getphy_from_vir((void*) (pAppData->pOBuffer[nCounter]));
					omx_free_phy((void*) phy_addr);
				}
				pAppData->pOBuffer[nCounter] = NULL;
			}
		}

		VIDENCTEST_PRINT("free ptr buffer is %lx\n", (unsigned long) (pAppData->pOutBuff[nCounter]->pBuffer));
		eError = OMX_FreeBuffer(pHandle, pAppData->pOutPortDef->nPortIndex, pAppData->pOutBuff[nCounter]);
	}

EXIT:
	return eError;
}

static OMX_ERRORTYPE App_SyncBuffer_AllocaBuffer(OMX_HANDLETYPE pHandle, MYDATATYPE*pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int nCounter;

	for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++)
	{
		if (pAppData->bAllocateSBuf == OMX_TRUE)
		{
			unsigned int syncbuf_size = pAppData->pSyncPortDef->nBufferSize;
			unsigned long vir_addr, phy_addr,fd_out;
			phy_addr = (unsigned long) omx_malloc_phy(syncbuf_size, &vir_addr);
			//pAppData->pOBuffer[nCounter] = (OMX_U8 *) vir_addr;
			pAppData->pOSyncBuffer[nCounter] = (OMX_U8 *) vir_addr;
			printf("no bStoreMetaData,sync!pAppData,phy_addr:%lx,vir_addr:%lx,syncbuf_size:%x\n", phy_addr, vir_addr,
					syncbuf_size);
			eError = OMX_UseBuffer(pHandle, &pAppData->pOSyncBuff[nCounter], pAppData->pSyncPortDef->nPortIndex,
					pAppData, pAppData->pSyncPortDef->nBufferSize, pAppData->pOSyncBuffer[nCounter]);
			VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_UseBuffer function");
		}
		else
		{
			eError = OMX_AllocateBuffer(pHandle, &pAppData->pOSyncBuff[nCounter], pAppData->pSyncPortDef->nPortIndex,
					pAppData, pAppData->pSyncPortDef->nBufferSize);
			VIDENCTEST_CHECK_EXIT(eError, "Error from OMX_AllocateBuffer function");
		}
	}

EXIT:
	return eError;
}

static OMX_ERRORTYPE App_SyncBuffer_FreeBuffer(OMX_HANDLETYPE pHandle, MYDATATYPE*pAppData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	int nCounter;

	for (nCounter = 0; nCounter < NUM_OF_OUT_BUFFERS; nCounter++)
	{
		if (pAppData->bAllocateOBuf == OMX_TRUE)
		{
			if (pAppData->pOSyncBuffer[nCounter])
			{
				unsigned long phy_addr;
				phy_addr = (unsigned long) omx_getphy_from_vir((void*) (pAppData->pOSyncBuffer[nCounter]));
				omx_free_phy((void*) phy_addr);
				pAppData->pOSyncBuffer[nCounter] = NULL;
			}
		}

		VIDENCTEST_PRINT("free ptr buffer is %lx\n", (unsigned long) (pAppData->pOSyncBuff[nCounter]->pBuffer));
		eError = OMX_FreeBuffer(pHandle, pAppData->pSyncPortDef->nPortIndex, pAppData->pOSyncBuff[nCounter]);
	}

EXIT:
	return eError;
}

#endif