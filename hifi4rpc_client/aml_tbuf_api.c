/*
 * Copyright (C) 2014-2018 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * amlogic T buffer API.
 *
 * Author: Wenjie Zhou <Wenjie.Zhou@amlogic.com>
 * Version:
 * - 0.1        init
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "rpc_client_shm.h"
#include "aml_tbuf_api.h"

typedef struct _TBUFS {
    AML_MEM_HANDLE hShm;
    size_t tbufSize;
    void* phyBase;
    void* virBase;
    uint32_t wr;
    uint32_t rdA;
    uint32_t rdB;
    bool isAEnabled;
    bool isBEnabled;
    TBUF_READER_T rdMaster;
    pthread_mutex_t tMutex;
} TBUFS;

static void internal_tbuf_destroy(AML_TBUF_HANDLE hTbuf)
{
    TBUFS* tbuf = (TBUFS*)hTbuf;
    if (tbuf) {
        if (tbuf->hShm)
            AML_MEM_Free(tbuf->hShm);
        pthread_mutex_destroy(&tbuf->tMutex);
        free(tbuf);
    }
}

AML_TBUF_HANDLE AML_TBUF_Create(size_t size)
{
    TBUFS* tbuf = malloc(sizeof(TBUFS));
    /**
     * pad a continuous buffer right after the end of tbuf,
     * to handle buffer wrap around
    */
    size_t padSize = size;
    if (!tbuf) {
        printf("Failed to allocate tbuf context\n");
        goto recycle_of_tbuf_create;
    }
    memset(tbuf, 0, sizeof(TBUFS));

    tbuf->hShm = AML_MEM_Allocate(size + padSize);
    if (!tbuf->hShm) {
        printf("Failed to allocate shared memory\n");
        goto recycle_of_tbuf_create;
    }
    tbuf->phyBase = AML_MEM_GetPhyAddr(tbuf->hShm);
    tbuf->virBase = AML_MEM_GetVirtAddr(tbuf->hShm);
    tbuf->tbufSize = size;
    tbuf->isAEnabled = false;
    tbuf->isBEnabled = false;
    tbuf->rdMaster = TBUF_READER_INV;
    pthread_mutex_init(&tbuf->tMutex, NULL);

    return tbuf;
recycle_of_tbuf_create:
    internal_tbuf_destroy((AML_TBUF_HANDLE)tbuf);
    return NULL;
}

void AML_TBUF_Destory(AML_TBUF_HANDLE hTbuf)
{
    internal_tbuf_destroy(hTbuf);
}

TBUF_RET AML_TBUF_AddReader(AML_TBUF_HANDLE hTbuf,  TBUF_READER_T rdType)
{
    TBUF_RET ret = TBUF_RET_OK;
    TBUFS* tbuf = (TBUFS*)hTbuf;
    pthread_mutex_lock(&tbuf->tMutex);
    if (rdType == TBUF_READER_A) {
        if (tbuf->rdMaster == TBUF_READER_INV) {
            tbuf->rdA = 0;
        } else if (tbuf->rdMaster == TBUF_READER_B) {
            tbuf->rdA = tbuf->rdB;
        }
        tbuf->rdMaster = TBUF_READER_A;
        tbuf->isAEnabled = true;
    }
    else if (rdType == TBUF_READER_B) {
        if (tbuf->rdMaster == TBUF_READER_INV) {
            tbuf->rdB = 0;
        } else if (tbuf->rdMaster == TBUF_READER_A) {
            tbuf->rdB = tbuf->rdA;
        }
        tbuf->rdMaster = TBUF_READER_B;
        tbuf->isBEnabled = true;
    }
    else {
        printf("Invalid rdType:%d\n", rdType);
        ret = TBUF_RET_ERR_INV_PARAM;
    }
    pthread_mutex_unlock(&tbuf->tMutex);
    return ret;
}

TBUF_RET AML_TBUF_RemoveReader(AML_TBUF_HANDLE hTbuf,  TBUF_READER_T rdType)
{
    TBUF_RET ret = TBUF_RET_OK;
    TBUFS* tbuf = (TBUFS*)hTbuf;
    pthread_mutex_lock(&tbuf->tMutex);
    if (rdType == TBUF_READER_A) {
        tbuf->isAEnabled = false;
        tbuf->rdMaster = (tbuf->isBEnabled)?TBUF_READER_B:TBUF_READER_INV;
    }
    else if (rdType == TBUF_READER_B) {
        tbuf->isBEnabled = false;
        tbuf->rdMaster = (tbuf->isAEnabled)?TBUF_READER_A:TBUF_READER_INV;
    }
    else {
        printf("Invalid rdType:%d\n", rdType);
        ret = TBUF_RET_ERR_INV_PARAM;
    }
    pthread_mutex_unlock(&tbuf->tMutex);
    return ret;
}

TBUF_RET AML_TBUF_UpdateWrOffset(AML_TBUF_HANDLE hTbuf, size_t size)
{
    TBUFS* tbuf = (TBUFS*)hTbuf;
    tbuf->wr = (tbuf->wr + size) % tbuf->tbufSize;
    TBUF_DEBUG("%s:wr=%d\n", __FUNCTION__, tbuf->wr);
    return TBUF_RET_OK;
}

TBUF_RET AML_TBUF_UpdateRdOffset(AML_TBUF_HANDLE hTbuf,  TBUF_READER_T rdType, size_t size)
{
    TBUF_RET ret = TBUF_RET_OK;
    TBUFS* tbuf = (TBUFS*)hTbuf;
    if (rdType == TBUF_READER_A) {
        tbuf->rdA = (tbuf->rdA + size) % tbuf->tbufSize;
        TBUF_DEBUG("%s:A-rd=%d\n", __FUNCTION__, tbuf->rdA);
    } else if (rdType == TBUF_READER_B) {
        tbuf->rdB = (tbuf->rdB + size) % tbuf->tbufSize;
        TBUF_DEBUG("%s:B-rd=%d\n", __FUNCTION__, tbuf->rdB);
    } else {
        printf("Invalid reader:%d\n", rdType);
        ret = TBUF_RET_ERR_INV_PARAM;
    }
    return ret;
}

TBUF_RET AML_TBUF_GetWrPtr(AML_TBUF_HANDLE hTbuf, void** phy, void** vir)
{
    TBUFS* tbuf = (TBUFS*)hTbuf;
    uint32_t wr = tbuf->wr;
    *phy = (uint8_t*)tbuf->phyBase + wr;
    *vir = (uint8_t*)tbuf->virBase + wr;
    return TBUF_RET_OK;
}

TBUF_RET AML_TBUF_GetRdPtr(AML_TBUF_HANDLE hTbuf, TBUF_READER_T rdType, void** phy, void** vir)
{
    TBUFS* tbuf = (TBUFS*)hTbuf;
    uint32_t rd = (rdType == TBUF_READER_A)?tbuf->rdA:tbuf->rdB;
    *phy = (uint8_t*)tbuf->phyBase + rd;
    *vir = (uint8_t*)tbuf->virBase + rd;
    return TBUF_RET_OK;
}


TBUF_RET AML_TBUF_GetSpace(AML_TBUF_HANDLE hTbuf, size_t* szAvail)
{
    TBUFS* tbuf = (TBUFS*)hTbuf;
    pthread_mutex_lock(&tbuf->tMutex);
    size_t szAvailA = tbuf->tbufSize;
    size_t szAvailB = tbuf->tbufSize;

    uint32_t rd = 0;
    uint32_t wr = 0;
    if (tbuf->isAEnabled) {
        rd = tbuf->rdA;
        wr = tbuf->wr;
        szAvailA =  (rd > wr) ? \
                   (rd - wr - 1)  : \
                   (tbuf->tbufSize + rd - wr - 1);
    }

    if (tbuf->isBEnabled) {
        rd = tbuf->rdB;
        wr = tbuf->wr;
        szAvailB =  (rd > wr) ? \
                   (rd - wr - 1)  : \
                   (tbuf->tbufSize + rd - wr - 1);
    }
    pthread_mutex_unlock(&tbuf->tMutex);

    *szAvail = AML_TBUF_MIN(szAvailA, szAvailB);
    return TBUF_RET_OK;
}

TBUF_RET AML_TBUF_GetFullness(AML_TBUF_HANDLE hTbuf, TBUF_READER_T rdType, size_t* szAvail)
{
    TBUFS* tbuf = (TBUFS*)hTbuf;
    uint32_t rd = (rdType == TBUF_READER_A)?tbuf->rdA:tbuf->rdB;
    uint32_t wr = tbuf->wr;
    *szAvail = (wr >= rd) ? \
           (wr - rd) :
           (tbuf->tbufSize + wr - rd);
    TBUF_DEBUG("%s:type=%d, rd=%d, wr=%d, full=%d\n", __FUNCTION__, rdType, rd, wr, *szAvail);
    return TBUF_RET_OK;
}

