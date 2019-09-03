#ifndef __IPC_CMD_TYPE_H__
#define __IPC_CMD_TYPE_H__

#if __cplusplus
 extern "C" {
#endif

/*Define Message Type Here*/
/*******************************************************************************
 * Message composition
 ******************************************************************************/

/* ...Message composition with module(6bits), function(10bits) */
#define __MBX_COMPOSE_MSG(mod, func)           (((mod) << 10) | ((func) & 0x3FF))

/* ...accessors */
#define MBX_MSG_MOD(msgcode)         (((msgcode) & 0x3F) >> 10)
#define MBX_MSG_FUNC(msgcode)          ((msgcode) & (0x3FF))

/*******************************************************************************
 * Define moudle type here, 6bits valid
 ******************************************************************************/
#define MBX_SYSTEM          0x0
#define MBX_CODEC           0x1
#define MBX_TINYALSA        0x2
#define MBX_PIPELINE        0x3

/*******************************************************************************
 * Define function here, 16bits valid
 ******************************************************************************/
/*SYSTEM*/
#define CMD_MEMINFO         0x0
#define CMD_HIFI4PS         0x1
#define CMD_IPC_TEST        0x2
#define CMD_SHM_ALLOC       0x3
#define CMD_SHM_FREE        0x4
#define CMD_SHM_TRANSFER    0x5
#define CMD_MBX_TEST        0x6
#define CMD_LISTEN_TEST     0x7
#define CMD_RPC_TEST        0x8
#define CMD_HIFI4VERSION    0x9


#define CMD_CCBUF_CREATE    0x10
#define CMD_CCBUF_DESTORY   0x12
#define CMD_CCBUF_READ      0x13
#define CMD_CCBUF_WRITE     0x14
#define CMD_CCBUF_RSV0      0x15
#define CMD_CCBUF_RSV1      0x16
#define CMD_CCBUF_RSV2      0x17
#define CMD_CCBUF_RSV3      0x18
#define CMD_CCBUF_RSV4      0x19
#define CMD_CCBUF_RSV6      0x1a
#define CMD_CCBUF_RSV7      0x1b
#define CMD_CCBUF_RSV8      0x1c
#define CMD_CCBUF_RSV9      0x1d
#define CMD_CCBUF_RSV10     0x1e
#define CMD_CCBUF_RSV11     0x1f


/*MBX_CODEC*/
#define MP3_DEC_DEMO        0x0
#define SPEEX_ENC_DEMO      0x1
#define OPUS_ENC_DEMO       0x2
#define OPUS_DEC_DEMO       0x3
#define AAC_ENC_DEMO        0x4
#define AAC_DEC_DEMO        0x5
#define UNIT_TEST           0x6

#define MP3_API_INIT        0x7
#define MP3_API_DECODE		0x8
#define MP3_API_RESET		0x9
#define MP3_API_DEINIT		0xa

#define AACDEC_API_INIT     0xb
#define AACDEC_API_DECODE   0xc
#define AACDEC_API_DEINIT	0xd
#define AACDEC_API_SETPARAM	0xe
#define AACDEC_API_ANCINIT	0xf
#define AACDEC_API_CFGRAW	0x10

//voice signal processing command belong to MBX_CODEC
#define VSP_API_INIT       0x300
#define VSP_API_DEINIT     0x301
#define VSP_API_PROCESS    0x302
#define VSP_API_OPEN       0x303
#define VSP_API_CLOSE      0x304
#define VSP_API_SETPARAM   0x305
#define VSP_API_GETPARAM   0x306

#define MBX_CODEC_MAX      0x3ff


/*TINYALSA*/
#define CAPTURE_DEMO        0x0
#define PLAYBACK_DEMO       0x1
#define LOOPBACK_DEMO       0x2
#define AUDIOTEST_DEMO	    0x3
#define TINYALSA_API_OPEN   0x4
#define TINYALSA_API_CLOSE  0x5
#define TINYALSA_API_WRITEI  0x6
#define TINYALSA_API_READI   0x7
#define TINYALSA_API_GETLATENCY   0x8


/** Pipeline test case */
#define MP3PLAY_DEMO        0x0
#define RECORDOPUS_DEMO     0x1
#define STRESS_DEMO         0x2
#define WAKE_ENGINE_DEMO    0x3

/*******************************************************************************
 * Mssage Comopsition
 ******************************************************************************/
#define MBX_CMD_HIFI4_PS        __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_HIFI4PS)
#define MBX_CMD_MEMINFO         __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_MEMINFO)
#define MBX_CMD_IPC_TEST        __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_IPC_TEST)
#define MBX_CMD_SHM_ALLOC        __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_SHM_ALLOC)
#define MBX_CMD_SHM_FREE        __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_SHM_FREE)
#define MBX_CMD_SHM_TRANSFER    __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_SHM_TRANSFER)
#define MBX_TEST_DEMO           __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_MBX_TEST)
#define MBX_LISTEN_DEMO           __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_LISTEN_TEST)
#define MBX_CMD_RPC_TEST        __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_RPC_TEST)
#define MBX_CMD_HIFI4_VERSION   __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_HIFI4VERSION)
#define MBX_CMD_CCBUF_CREATE    __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_CCBUF_CREATE)
#define MBX_CMD_CCBUF_DESTORY   __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_CCBUF_DESTORY)
#define MBX_CMD_CCBUF_READ      __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_CCBUF_READ)
#define MBX_CMD_CCBUF_WRITE     __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_CCBUF_WRITE)


#define MBX_MP3_DEC_DEMO        __MBX_COMPOSE_MSG(MBX_CODEC, MP3_DEC_DEMO)
#define MBX_SPEEX_ENC_DEMO      __MBX_COMPOSE_MSG(MBX_CODEC, SPEEX_ENC_DEMO)
#define MBX_OPUS_ENC_DEMO       __MBX_COMPOSE_MSG(MBX_CODEC, OPUS_ENC_DEMO)
#define MBX_AAC_ENC_DEMO        __MBX_COMPOSE_MSG(MBX_CODEC, AAC_ENC_DEMO)
#define MBX_AAC_DEC_DEMO        __MBX_COMPOSE_MSG(MBX_CODEC, AAC_DEC_DEMO)
#define MBX_CODEC_UNIT_TEST     __MBX_COMPOSE_MSG(MBX_CODEC, UNIT_TEST)
#define MBX_CODEC_MP3_API_INIT  __MBX_COMPOSE_MSG(MBX_CODEC, MP3_API_INIT)
#define MBX_CODEC_MP3_API_DECODE  __MBX_COMPOSE_MSG(MBX_CODEC, MP3_API_DECODE)
#define MBX_CODEC_MP3_API_RESET  __MBX_COMPOSE_MSG(MBX_CODEC, MP3_API_RESET)
#define MBX_CODEC_MP3_API_DEINIT  __MBX_COMPOSE_MSG(MBX_CODEC, MP3_API_DEINIT)
#define MBX_CODEC_AACDEC_API_INIT  __MBX_COMPOSE_MSG(MBX_CODEC, AACDEC_API_INIT)
#define MBX_CODEC_AACDEC_API_DECODE  __MBX_COMPOSE_MSG(MBX_CODEC, AACDEC_API_DECODE)
#define MBX_CODEC_AACDEC_API_DEINIT  __MBX_COMPOSE_MSG(MBX_CODEC, AACDEC_API_DEINIT)
#define MBX_CODEC_AACDEC_API_SETPARAM  __MBX_COMPOSE_MSG(MBX_CODEC, AACDEC_API_SETPARAM)
#define MBX_CODEC_AACDEC_API_ANCINIT  __MBX_COMPOSE_MSG(MBX_CODEC, AACDEC_API_ANCINIT)
#define MBX_CODEC_AACDEC_API_CFGRAW  __MBX_COMPOSE_MSG(MBX_CODEC, AACDEC_API_CFGRAW)
#define MBX_CODEC_VSP_API_INIT  __MBX_COMPOSE_MSG(MBX_CODEC, VSP_API_INIT)
#define MBX_CODEC_VSP_API_DEINIT  __MBX_COMPOSE_MSG(MBX_CODEC, VSP_API_DEINIT)
#define MBX_CODEC_VSP_API_PROCESS  __MBX_COMPOSE_MSG(MBX_CODEC, VSP_API_PROCESS)
#define MBX_CODEC_VSP_API_OPEN  __MBX_COMPOSE_MSG(MBX_CODEC, VSP_API_OPEN)
#define MBX_CODEC_VSP_API_CLOSE  __MBX_COMPOSE_MSG(MBX_CODEC, VSP_API_CLOSE)
#define MBX_CODEC_VSP_API_SETPARAM  __MBX_COMPOSE_MSG(MBX_CODEC, VSP_API_SETPARAM)
#define MBX_CODEC_VSP_API_GETPARAM  __MBX_COMPOSE_MSG(MBX_CODEC, VSP_API_GETPARAM)


#define MBX_CAPTURE_DEMO        __MBX_COMPOSE_MSG(MBX_TINYALSA, CAPTURE_DEMO)
#define MBX_PLAYBACK_DEMO       __MBX_COMPOSE_MSG(MBX_TINYALSA, PLAYBACK_DEMO)
#define MBX_LOOPBACK_DEMO       __MBX_COMPOSE_MSG(MBX_TINYALSA, LOOPBACK_DEMO)
#define MBX_AUDIOTEST_DEMO	__MBX_COMPOSE_MSG(MBX_TINYALSA, AUDIOTEST_DEMO)
#define MBX_TINYALSA_OPEN	__MBX_COMPOSE_MSG(MBX_TINYALSA, TINYALSA_API_OPEN)
#define MBX_TINYALSA_CLOSE	__MBX_COMPOSE_MSG(MBX_TINYALSA, TINYALSA_API_CLOSE)
#define MBX_TINYALSA_WRITEI	__MBX_COMPOSE_MSG(MBX_TINYALSA, TINYALSA_API_WRITEI)
#define MBX_TINYALSA_READI	__MBX_COMPOSE_MSG(MBX_TINYALSA, TINYALSA_API_READI)
#define MBX_TINYALSA_GETLATENCY	__MBX_COMPOSE_MSG(MBX_TINYALSA, TINYALSA_API_GETLATENCY)

#define MBX_MP3_PLAY_DEMO       __MBX_COMPOSE_MSG(MBX_PIPELINE, MP3PLAY_DEMO)
#define MBX_RECORD_OPUS_DEMO    __MBX_COMPOSE_MSG(MBX_PIPELINE, RECORDOPUS_DEMO)
#define MBX_STRESS_DEMO         __MBX_COMPOSE_MSG(MBX_PIPELINE, STRESS_DEMO)
#define MBX_WAKE_ENGINE_DEMO    __MBX_COMPOSE_MSG(MBX_PIPELINE, WAKE_ENGINE_DEMO)

typedef uint64_t xpointer;

#if __cplusplus
}
#endif

#endif
