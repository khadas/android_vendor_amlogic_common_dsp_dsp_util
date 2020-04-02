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

int bcm_client_write(int aipchdl, const void *data, unsigned int count);

#ifdef __cplusplus
}
#endif


#endif
