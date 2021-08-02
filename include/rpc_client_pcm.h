#ifndef _TINYALSA_CLIENT_H_
#define _TINYALSA_CLIENT_H_

#include "pcm.h"
#include "aipc_type.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void* tAmlPcmhdl;
tAmlPcmhdl pcm_client_open(unsigned int card,
                         unsigned int device,
                         unsigned int flags,
                         rpc_pcm_config *config);
int pcm_client_writei(tAmlPcmhdl hdl, const void *data, unsigned int count);
int pcm_client_readi(tAmlPcmhdl hdl, const void *data, unsigned int count);
int pcm_client_close(tAmlPcmhdl hdl);
long pcm_client_get_latency(tAmlPcmhdl hdl);
uint32_t pcm_client_frame_to_bytes(tAmlPcmhdl hdl, uint32_t fr);
uint32_t pcm_client_bytes_to_frame(tAmlPcmhdl hdl, uint32_t b);

tAmlPcmhdl pcm_process_client_open(unsigned int card,
						 unsigned int device,
						 unsigned int flags,
						 rpc_pcm_config *config);
int pcm_process_client_start(tAmlPcmhdl hdl);
int pcm_process_client_stop(tAmlPcmhdl hdl);
int pcm_process_client_writei(tAmlPcmhdl hdl, const void *data, unsigned int count, unsigned int type);
int pcm_process_client_readi(tAmlPcmhdl hdl, const void *data, unsigned int count, unsigned int type);
int pcm_process_client_close(tAmlPcmhdl hdl);
int pcm_process_client_alloc_buffer(tAmlPcmhdl hdl, unsigned int size, unsigned int type);
int pcm_process_client_free_buffer(tAmlPcmhdl hdl, unsigned int type);
int pcm_process_client_flatbuf_readi(tAmlPcmhdl hdl, const void *data, unsigned int count, unsigned int type);

struct buf_info {
	uint64_t handle;
	uint64_t phyaddr;
	void *viraddr;
	unsigned int size;
};
int pcm_process_client_dqbuf(tAmlPcmhdl hdl, struct buf_info *buf, struct buf_info *release_buf, unsigned int type);
int pcm_process_client_qbuf(tAmlPcmhdl hdl, struct buf_info *buf, unsigned int type);

int pcm_process_client_get_volume_gain(tAmlPcmhdl hdl, int *gain);
int pcm_process_client_set_volume_gain(tAmlPcmhdl hdl, int gain);

#ifdef __cplusplus
}
#endif


#endif
