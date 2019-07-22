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

/*TINYALSA*/
#define CAPTURE_DEMO        0x0
#define PLAYBACK_DEMO       0x1
#define LOOPBACK_DEMO       0x2
#define AUDIOTEST_DEMO	    0x3

/** Pipeline test case */
#define MP3PLAY_DEMO        0x0
#define RECORDOPUS_DEMO     0x1

/*******************************************************************************
 * Mssage Comopsition
 ******************************************************************************/
#define MBX_CMD_HIFI4_PS        __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_HIFI4PS)
#define MBX_CMD_MEMINFO         __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_MEMINFO)
#define MBX_CMD_IPC_TEST        __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_IPC_TEST)
#define MBX_CMD_SHM_ALLOC        __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_SHM_ALLOC)
#define MBX_CMD_SHM_FREE        __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_SHM_FREE)


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


#define MBX_CAPTURE_DEMO        __MBX_COMPOSE_MSG(MBX_TINYALSA, CAPTURE_DEMO)
#define MBX_PLAYBACK_DEMO       __MBX_COMPOSE_MSG(MBX_TINYALSA, PLAYBACK_DEMO)
#define MBX_LOOPBACK_DEMO       __MBX_COMPOSE_MSG(MBX_TINYALSA, LOOPBACK_DEMO)
#define MBX_AUDIOTEST_DEMO	__MBX_COMPOSE_MSG(MBX_TINYALSA, AUDIOTEST_DEMO)

#define MBX_MP3_PLAY_DEMO       __MBX_COMPOSE_MSG(MBX_PIPELINE, MP3PLAY_DEMO)
#define MBX_RECORD_OPUS_DEMO    __MBX_COMPOSE_MSG(MBX_PIPELINE, RECORDOPUS_DEMO)

typedef uint64_t xpointer;

#if __cplusplus
}
#endif

#endif
