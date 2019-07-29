#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rpc_client_aipc.h"
#include "aipc_type.h"
#include "rpc_client_mp3.h"
#include "aipc_type.h"
#include "rpc_client_tinyalsa.h"
#include "rpc_client_shm.h"


struct tAmlPcmCtx {
	tAcodecPcmSrvHdl pcm_srv_hdl;
	rpc_pcm_config config;
	int aipchdl;
};


// mailbox is slow, add hack function here. It only work for 32bit, 2ch
uint32_t pcm_frame_to_bytes(tAmlPcmhdl hdl, uint32_t fr) {
    return fr * 8;
}

uint32_t pcm_bytes_to_frame(tAmlPcmhdl hdl, uint32_t b) {
    return b / 8;
}

tAmlPcmhdl pcm_client_open(unsigned int card,
                         unsigned int device,
                         unsigned int flags,
                         rpc_pcm_config *config)
{
    pcm_open_st arg;
	struct tAmlPcmCtx* pAmlPcmCtx = (struct tAmlPcmCtx*)malloc(sizeof(struct tAmlPcmCtx));
	memset(pAmlPcmCtx, 0, sizeof(struct tAmlPcmCtx));
    memset(&arg, 0, sizeof(arg));
    arg.card = card;
    arg.device = device;
    arg.flags = flags;
    memcpy(&arg.pcm_config, config, sizeof(rpc_pcm_config));
    arg.out_pcm = (xpointer)NULL;
	pAmlPcmCtx->aipchdl = xAudio_Ipc_init();
	xAIPC(pAmlPcmCtx->aipchdl, MBX_TINYALSA_OPEN, &arg, sizeof(arg));
    pAmlPcmCtx->pcm_srv_hdl = arg.out_pcm;
    return pAmlPcmCtx;
}

int pcm_client_close(tAmlPcmhdl hdl) {
    pcm_close_st arg;
	struct tAmlPcmCtx* pAmlPcmCtx = (struct tAmlPcmCtx*)hdl;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.out_ret = 0;
	xAIPC(pAmlPcmCtx->aipchdl, MBX_TINYALSA_CLOSE, &arg, sizeof(arg));
	xAudio_Ipc_Deinit(pAmlPcmCtx->aipchdl);
	free(pAmlPcmCtx);
    return arg.out_ret;
}



int pcm_client_writei(tAmlPcmhdl hdl, const void *data, unsigned int count)
{
    pcm_io_st arg;
	struct tAmlPcmCtx* pAmlPcmCtx = (struct tAmlPcmCtx*)hdl;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.data = (xpointer)data;
    arg.count = count;
    arg.out_ret = 0;

	xAIPC(pAmlPcmCtx->aipchdl, MBX_TINYALSA_WRITEI, &arg, sizeof(arg));
    return arg.out_ret;
}

int pcm_client_readi(tAmlPcmhdl hdl, const void *data, unsigned int count)
{
    pcm_io_st arg;
	struct tAmlPcmCtx* pAmlPcmCtx = (struct tAmlPcmCtx*)hdl;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.data = (xpointer)data;
    arg.count = count;
    arg.out_ret = 0;

	printf("data:%p, count:%d\n", data, count);
	xAIPC(pAmlPcmCtx->aipchdl, MBX_TINYALSA_READI, &arg, sizeof(arg));
    return arg.out_ret;
}

long pcm_client_get_latency(tAmlPcmhdl hdl)
{
    pcm_get_latency_st arg;
	struct tAmlPcmCtx* pAmlPcmCtx = (struct tAmlPcmCtx*)hdl;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.out_ret = 0;
  	xAIPC(pAmlPcmCtx->aipchdl, MBX_TINYALSA_GETLATENCY, &arg, sizeof(arg));
    return arg.out_ret;
}

