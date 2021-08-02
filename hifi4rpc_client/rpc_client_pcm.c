#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rpc_client_aipc.h"
#include "aipc_type.h"
#include "rpc_client_mp3.h"
#include "rpc_client_pcm.h"
#include "rpc_client_shm.h"
#include "aml_flatbuf_api.h"
#include "generic_macro.h"


struct tAmlPcmCtx {
    tAcodecPcmSrvHdl pcm_srv_hdl;
    rpc_pcm_config config;
    int aipchdl;
};

// loopback pcm operation
struct tAmlProPcmCtx {
    tAcodecPcmSrvHdl pcm_srv_hdl;
    rpc_pcm_config config;
    int aipchdl;
    struct flatbuffer_config raw_buf_cfg;
    AML_FLATBUF_HANDLE raw_buf_hdl;
    struct flatbuffer_config processe_buf_cfg;
    AML_FLATBUF_HANDLE processe_buf_hdl;
};

static unsigned int pcm_client_format_to_bytes(enum pcm_format format)
{
    switch (format) {
        case PCM_FORMAT_S32_LE:
        case PCM_FORMAT_S32_BE:
        case PCM_FORMAT_S24_LE:
        case PCM_FORMAT_S24_BE:
            return 32 >> 3;		/*4 Bytes*/
        case PCM_FORMAT_S24_3LE:
        case PCM_FORMAT_S24_3BE:
            return 24 >> 3;		/*3 Bytes*/
        default:
        case PCM_FORMAT_S16_LE:
        case PCM_FORMAT_S16_BE:
            return 16 >> 3;		/*2 Bytes*/
        case PCM_FORMAT_S8:
            return 8 >> 3;		/*1 Bytes*/
    };
}

uint32_t pcm_client_frame_to_bytes(tAmlPcmhdl hdl, uint32_t frame) {
    struct tAmlPcmCtx* pAmlPcmCtx = (struct tAmlPcmCtx*)hdl;
    int bytes_per_sample = pcm_client_format_to_bytes(pAmlPcmCtx->config.format);
    return (frame * bytes_per_sample * (pAmlPcmCtx->config.channels));
}

uint32_t pcm_client_bytes_to_frame(tAmlPcmhdl hdl, uint32_t bytes) {
    struct tAmlPcmCtx* pAmlPcmCtx = (struct tAmlPcmCtx*)hdl;
    int bytes_per_sample = pcm_client_format_to_bytes(pAmlPcmCtx->config.format);
    return (bytes / ((pAmlPcmCtx->config.channels) * bytes_per_sample));
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
    memcpy(&pAmlPcmCtx->config, config, sizeof(rpc_pcm_config));
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

tAmlPcmhdl pcm_process_client_open(unsigned int card,
                         unsigned int device,
                         unsigned int flags,
                         rpc_pcm_config *config)
{
    pcm_process_open_st arg;
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)malloc(sizeof(struct tAmlProPcmCtx));
    memset(pAmlPcmCtx, 0, sizeof(struct tAmlProPcmCtx));
    memset(&arg, 0, sizeof(arg));
    arg.card = card;
    arg.device = device;
    arg.flags = flags;
    memcpy(&arg.pcm_config, config, sizeof(rpc_pcm_config));
    arg.out_pcm = (xpointer)NULL;
    pAmlPcmCtx->aipchdl = xAudio_Ipc_init();
    xAIPC(pAmlPcmCtx->aipchdl, MBX_CMD_APROCESS_OPEN_DSPC, &arg, sizeof(arg));
    pAmlPcmCtx->pcm_srv_hdl = arg.out_pcm;
    memcpy(&pAmlPcmCtx->config, config, sizeof(rpc_pcm_config));
    return pAmlPcmCtx;
}

int pcm_process_client_close(tAmlPcmhdl hdl)
{
    pcm_process_close_st arg;
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.out_ret = 0;
    xAIPC(pAmlPcmCtx->aipchdl, MBX_CMD_APROCESS_CLOSE_DSPC, &arg, sizeof(arg));
    xAudio_Ipc_Deinit(pAmlPcmCtx->aipchdl);
    free(pAmlPcmCtx);
    return arg.out_ret;
}

int pcm_process_client_start(tAmlPcmhdl hdl)
{
    pcm_process_io_st arg;
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.out_ret = 0;
    xAIPC(pAmlPcmCtx->aipchdl, MBX_CMD_APROCESS_START_DSPC, &arg, sizeof(arg));
    return arg.out_ret;
}

int pcm_process_client_stop(tAmlPcmhdl hdl)
{
    pcm_process_io_st arg;
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.out_ret = 0;
    xAIPC(pAmlPcmCtx->aipchdl, MBX_CMD_APROCESS_STOP_DSPC, &arg, sizeof(arg));
    return arg.out_ret;
}

int pcm_process_client_writei(tAmlPcmhdl hdl, const void *data, unsigned int count, unsigned int type)
{
    AMX_UNUSED(hdl);
    AMX_UNUSED(data);
    AMX_UNUSED(count);
    AMX_UNUSED(type);
    return -1;
}

int pcm_process_client_readi(tAmlPcmhdl hdl, const void *data, unsigned int count, unsigned int type)
{
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    pcm_process_io_st arg;
    if (type == PROCESSBUF) {
        arg.id = 0;
    } else {
        arg.id = 1;
    }
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.data = (xpointer)data;
    arg.count = count;
    arg.out_ret = 0;

    xAIPC(pAmlPcmCtx->aipchdl, CMD_APROCESS_READ_DSPC, &arg, sizeof(arg));
    return arg.out_ret;
}

int pcm_process_client_dqbuf(tAmlPcmhdl hdl, struct buf_info *buf, struct buf_info *release_buf, unsigned int type)
{
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    pcm_process_buf_st arg;
    if (type == PROCESSBUF) {
        arg.id = 0;
    } else {
        arg.id = 1;
    }
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.buf_handle = 0;
    arg.data = 0;
    arg.count = 0;
    arg.out_ret = 0;
    if (release_buf && release_buf->handle && release_buf->phyaddr &&
        release_buf->viraddr && release_buf->size &&
        (release_buf->viraddr == AML_MEM_GetVirtAddr((void *)release_buf->phyaddr)))
        arg.release_buf_handle = release_buf->handle;
    else
        arg.release_buf_handle = 0;

    xAIPC(pAmlPcmCtx->aipchdl, CMD_APROCESS_DQBUF_DSPC, &arg, sizeof(arg));
    buf->handle = arg.buf_handle;
    buf->phyaddr = arg.data;
    buf->size = arg.count;
    if (arg.buf_handle) {
        buf->viraddr = AML_MEM_GetVirtAddr((void *)buf->phyaddr);
        AML_MEM_Invalidate((void *)buf->phyaddr, buf->size);
    }
    return arg.out_ret;
}

int pcm_process_client_qbuf(tAmlPcmhdl hdl, struct buf_info *buf, unsigned int type)
{
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    pcm_process_buf_st arg;
    if (!buf->handle)
        return 0;
    if (type == PROCESSBUF) {
        arg.id = 0;
    } else {
        arg.id = 1;
    }
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.buf_handle = buf->handle;
    arg.data = buf->phyaddr;
    arg.count = 0;
    arg.out_ret = 0;
    xAIPC(pAmlPcmCtx->aipchdl, CMD_APROCESS_QBUF_DSPC, &arg, sizeof(arg));

    return arg.out_ret;
}

int pcm_process_client_get_volume_gain(tAmlPcmhdl hdl, int *gain)
{
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    pcm_process_gain_st arg;

    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.gain = 0;
    arg.out_ret = 0;
    xAIPC(pAmlPcmCtx->aipchdl, CMD_APROCESS_GET_GAIN_DSPC, &arg, sizeof(arg));
    if (!arg.out_ret)
        *gain = arg.gain;

    return arg.out_ret;
}

int pcm_process_client_set_volume_gain(tAmlPcmhdl hdl, int gain)
{
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    pcm_process_gain_st arg;

    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.gain = gain;
    arg.out_ret = 0;
    xAIPC(pAmlPcmCtx->aipchdl, CMD_APROCESS_SET_GAIN_DSPC, &arg, sizeof(arg));

    return arg.out_ret;
}

int pcm_process_client_alloc_buffer(tAmlPcmhdl hdl, unsigned int size, unsigned int type)
{
    pcm_process_memory_st arg;
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.out_ret = 0;
    arg.size = size;
    arg.phy_ch = FLATBUF_CH_ARM2DSPA;
    if (type == PROCESSBUF) {
        arg.chanid = 0;
        strcpy(arg.buf_name, "DSP2ARM_DSPC_PROCESSEDDATA");
        xAIPC(pAmlPcmCtx->aipchdl, CMD_APROCESS_ALLOC_BUF_DSPC, &arg, sizeof(arg));
        memset(&pAmlPcmCtx->processe_buf_cfg, 0, sizeof(struct flatbuffer_config));
        pAmlPcmCtx->processe_buf_cfg.size = arg.size;
        pAmlPcmCtx->processe_buf_cfg.phy_ch = arg.phy_ch;
        pAmlPcmCtx->processe_buf_hdl = AML_FLATBUF_Create(arg.buf_name, FLATBUF_FLAG_RD, &pAmlPcmCtx->processe_buf_cfg);
        if (pAmlPcmCtx->processe_buf_hdl == NULL) {
            printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
            return -1;
        }
    } else {
        arg.chanid = 1;
        strcpy(arg.buf_name, "DSP2ARM_DSPC_RAWDATA");
        xAIPC(pAmlPcmCtx->aipchdl, CMD_APROCESS_ALLOC_BUF_DSPC, &arg, sizeof(arg));
        memset(&pAmlPcmCtx->raw_buf_cfg, 0, sizeof(struct flatbuffer_config));
        pAmlPcmCtx->raw_buf_cfg.size = arg.size;
        pAmlPcmCtx->raw_buf_cfg.phy_ch = arg.phy_ch;
        pAmlPcmCtx->raw_buf_hdl = AML_FLATBUF_Create(arg.buf_name, FLATBUF_FLAG_RD, &pAmlPcmCtx->raw_buf_cfg);
        if (pAmlPcmCtx->raw_buf_hdl == NULL) {
            printf("%s, %d, AML_FLATBUF_Create failed\n", __func__, __LINE__);
            return -1;
        }
    }
    return size;
}

int pcm_process_client_free_buffer(tAmlPcmhdl hdl, unsigned int type)
{
    pcm_process_memory_st arg;
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    memset(&arg, 0, sizeof(arg));
    arg.pcm = (xpointer)pAmlPcmCtx->pcm_srv_hdl;
    arg.out_ret = 0;
    if (type == PROCESSBUF) {
        AML_FLATBUF_Destroy(pAmlPcmCtx->processe_buf_hdl);
        arg.chanid = 0;
    } else {
        AML_FLATBUF_Destroy(pAmlPcmCtx->raw_buf_hdl);
        arg.chanid = 1;
    }
    xAIPC(pAmlPcmCtx->aipchdl, CMD_APROCESS_FREE_BUF_DSPC, &arg, sizeof(arg));
    return 0;
}

int pcm_process_client_flatbuf_readi(tAmlPcmhdl hdl, const void *data, unsigned int count, unsigned int type)
{
    struct tAmlProPcmCtx* pAmlPcmCtx = (struct tAmlProPcmCtx*)hdl;
    struct flatbuffer_config *config;
    AML_FLATBUF_HANDLE hFbuf;
    int size;
    if (type == PROCESSBUF) {
        config = &pAmlPcmCtx->processe_buf_cfg;
        hFbuf = pAmlPcmCtx->processe_buf_hdl;
    } else {
        config = &pAmlPcmCtx->raw_buf_cfg;
        hFbuf = pAmlPcmCtx->raw_buf_hdl;
    }
    size = config->size;
    size = AMX_MIN(count, size);
    size = AML_FLATBUF_Read(hFbuf, data, size, -1);
    return size;
}
