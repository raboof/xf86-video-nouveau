 /***************************************************************************\
|*                                                                           *|
|*       Copyright 1993-2003 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 1993-2003 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
|*                                                                           *|
 \***************************************************************************/
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_hw.c,v 1.21 2006/06/16 00:19:33 mvojkovi Exp $ */

#include "nv_local.h"
#include "compiler.h"
#include "nv_include.h"


void NVLockUnlock (
    NVPtr pNv,
    Bool  Lock
)
{
    CARD8 cr11;

    VGA_WR08(pNv->PCIO, 0x3D4, 0x1F);
    VGA_WR08(pNv->PCIO, 0x3D5, Lock ? 0x99 : 0x57);

    VGA_WR08(pNv->PCIO, 0x3D4, 0x11);
    cr11 = VGA_RD08(pNv->PCIO, 0x3D5);
    if(Lock) cr11 |= 0x80;
    else cr11 &= ~0x80;
    VGA_WR08(pNv->PCIO, 0x3D5, cr11);
}

int NVShowHideCursor (
    NVPtr pNv,
    int   ShowHide
)
{
    int current = pNv->CurrentState->cursor1;

    pNv->CurrentState->cursor1 = (pNv->CurrentState->cursor1 & 0xFE) |
                                 (ShowHide & 0x01);
    VGA_WR08(pNv->PCIO, 0x3D4, 0x31);
    VGA_WR08(pNv->PCIO, 0x3D5, pNv->CurrentState->cursor1);

    if(pNv->Architecture == NV_ARCH_40) {  /* HW bug */
       volatile CARD32 curpos = pNv->PRAMDAC[0x0300/4];
       pNv->PRAMDAC[0x0300/4] = curpos;
    }

    return (current & 0x01);
}

/****************************************************************************\
*                                                                            *
* The video arbitration routines calculate some "magic" numbers.  Fixes      *
* the snow seen when accessing the framebuffer without it.                   *
* It just works (I hope).                                                    *
*                                                                            *
\****************************************************************************/

typedef struct {
  int graphics_lwm;
  int video_lwm;
  int graphics_burst_size;
  int video_burst_size;
  int valid;
} nv4_fifo_info;

typedef struct {
  int pclk_khz;
  int mclk_khz;
  int nvclk_khz;
  char mem_page_miss;
  char mem_latency;
  int memory_width;
  char enable_video;
  char gr_during_vid;
  char pix_bpp;
  char mem_aligned;
  char enable_mp;
} nv4_sim_state;

typedef struct {
  int graphics_lwm;
  int video_lwm;
  int graphics_burst_size;
  int video_burst_size;
  int valid;
} nv10_fifo_info;

typedef struct {
  int pclk_khz;
  int mclk_khz;
  int nvclk_khz;
  char mem_page_miss;
  char mem_latency;
  int memory_type;
  int memory_width;
  char enable_video;
  char gr_during_vid;
  char pix_bpp;
  char mem_aligned;
  char enable_mp;
} nv10_sim_state;


static void nvGetClocks(NVPtr pNv, unsigned int *MClk, unsigned int *NVClk)
{
    unsigned int pll, N, M, MB, NB, P;

    if(pNv->Architecture >= NV_ARCH_40) {
       pll = pNv->PMC[0x4020/4];
       P = (pll >> 16) & 0x07;
       pll = pNv->PMC[0x4024/4];
       M = pll & 0xFF;
       N = (pll >> 8) & 0xFF;
       if(((pNv->Chipset & 0xfff0) == CHIPSET_G71) ||
          ((pNv->Chipset & 0xfff0) == CHIPSET_G73))
       {
          MB = 1;
          NB = 1;
       } else {
          MB = (pll >> 16) & 0xFF;
          NB = (pll >> 24) & 0xFF;
       }
       *MClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;

       pll = pNv->PMC[0x4000/4];
       P = (pll >> 16) & 0x07;  
       pll = pNv->PMC[0x4004/4];
       M = pll & 0xFF;
       N = (pll >> 8) & 0xFF;
       MB = (pll >> 16) & 0xFF;
       NB = (pll >> 24) & 0xFF;

       *NVClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;
    } else
    if(pNv->twoStagePLL) {
       pll = pNv->PRAMDAC0[0x0504/4];
       M = pll & 0xFF; 
       N = (pll >> 8) & 0xFF; 
       P = (pll >> 16) & 0x0F;
       pll = pNv->PRAMDAC0[0x0574/4];
       if(pll & 0x80000000) {
           MB = pll & 0xFF; 
           NB = (pll >> 8) & 0xFF;
       } else {
           MB = 1;
           NB = 1;
       }
       *MClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;

       pll = pNv->PRAMDAC0[0x0500/4];
       M = pll & 0xFF; 
       N = (pll >> 8) & 0xFF; 
       P = (pll >> 16) & 0x0F;
       pll = pNv->PRAMDAC0[0x0570/4];
       if(pll & 0x80000000) {
           MB = pll & 0xFF;
           NB = (pll >> 8) & 0xFF;
       } else {
           MB = 1;
           NB = 1;
       }
       *NVClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;
    } else 
    if(((pNv->Chipset & 0x0ff0) == CHIPSET_NV30) ||
       ((pNv->Chipset & 0x0ff0) == CHIPSET_NV35))
    {
       pll = pNv->PRAMDAC0[0x0504/4];
       M = pll & 0x0F; 
       N = (pll >> 8) & 0xFF;
       P = (pll >> 16) & 0x07;
       if(pll & 0x00000080) {
           MB = (pll >> 4) & 0x07;     
           NB = (pll >> 19) & 0x1f;
       } else {
           MB = 1;
           NB = 1;
       }
       *MClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;

       pll = pNv->PRAMDAC0[0x0500/4];
       M = pll & 0x0F;
       N = (pll >> 8) & 0xFF;
       P = (pll >> 16) & 0x07;
       if(pll & 0x00000080) {
           MB = (pll >> 4) & 0x07;
           NB = (pll >> 19) & 0x1f;
       } else {
           MB = 1;
           NB = 1;
       }
       *NVClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;
    } else {
       pll = pNv->PRAMDAC0[0x0504/4];
       M = pll & 0xFF; 
       N = (pll >> 8) & 0xFF; 
       P = (pll >> 16) & 0x0F;
       *MClk = (N * pNv->CrystalFreqKHz / M) >> P;

       pll = pNv->PRAMDAC0[0x0500/4];
       M = pll & 0xFF; 
       N = (pll >> 8) & 0xFF; 
       P = (pll >> 16) & 0x0F;
       *NVClk = (N * pNv->CrystalFreqKHz / M) >> P;
    }

#if 0
    ErrorF("NVClock = %i MHz, MEMClock = %i MHz\n", *NVClk/1000, *MClk/1000);
#endif
}


static void nv4CalcArbitration (
    nv4_fifo_info *fifo,
    nv4_sim_state *arb
)
{
    int data, pagemiss, cas,width, video_enable, bpp;
    int nvclks, mclks, pclks, vpagemiss, crtpagemiss, vbs;
    int found, mclk_extra, mclk_loop, cbs, m1, p1;
    int mclk_freq, pclk_freq, nvclk_freq, mp_enable;
    int us_m, us_n, us_p, video_drain_rate, crtc_drain_rate;
    int vpm_us, us_video, vlwm, video_fill_us, cpm_us, us_crt,clwm;

    fifo->valid = 1;
    pclk_freq = arb->pclk_khz;
    mclk_freq = arb->mclk_khz;
    nvclk_freq = arb->nvclk_khz;
    pagemiss = arb->mem_page_miss;
    cas = arb->mem_latency;
    width = arb->memory_width >> 6;
    video_enable = arb->enable_video;
    bpp = arb->pix_bpp;
    mp_enable = arb->enable_mp;
    clwm = 0;
    vlwm = 0;
    cbs = 128;
    pclks = 2;
    nvclks = 2;
    nvclks += 2;
    nvclks += 1;
    mclks = 5;
    mclks += 3;
    mclks += 1;
    mclks += cas;
    mclks += 1;
    mclks += 1;
    mclks += 1;
    mclks += 1;
    mclk_extra = 3;
    nvclks += 2;
    nvclks += 1;
    nvclks += 1;
    nvclks += 1;
    if (mp_enable)
        mclks+=4;
    nvclks += 0;
    pclks += 0;
    found = 0;
    vbs = 0;
    while (found != 1)
    {
        fifo->valid = 1;
        found = 1;
        mclk_loop = mclks+mclk_extra;
        us_m = mclk_loop *1000*1000 / mclk_freq;
        us_n = nvclks*1000*1000 / nvclk_freq;
        us_p = nvclks*1000*1000 / pclk_freq;
        if (video_enable)
        {
            video_drain_rate = pclk_freq * 2;
            crtc_drain_rate = pclk_freq * bpp/8;
            vpagemiss = 2;
            vpagemiss += 1;
            crtpagemiss = 2;
            vpm_us = (vpagemiss * pagemiss)*1000*1000/mclk_freq;
            if (nvclk_freq * 2 > mclk_freq * width)
                video_fill_us = cbs*1000*1000 / 16 / nvclk_freq ;
            else
                video_fill_us = cbs*1000*1000 / (8 * width) / mclk_freq;
            us_video = vpm_us + us_m + us_n + us_p + video_fill_us;
            vlwm = us_video * video_drain_rate/(1000*1000);
            vlwm++;
            vbs = 128;
            if (vlwm > 128) vbs = 64;
            if (vlwm > (256-64)) vbs = 32;
            if (nvclk_freq * 2 > mclk_freq * width)
                video_fill_us = vbs *1000*1000/ 16 / nvclk_freq ;
            else
                video_fill_us = vbs*1000*1000 / (8 * width) / mclk_freq;
            cpm_us = crtpagemiss  * pagemiss *1000*1000/ mclk_freq;
            us_crt =
            us_video
            +video_fill_us
            +cpm_us
            +us_m + us_n +us_p
            ;
            clwm = us_crt * crtc_drain_rate/(1000*1000);
            clwm++;
        }
        else
        {
            crtc_drain_rate = pclk_freq * bpp/8;
            crtpagemiss = 2;
            crtpagemiss += 1;
            cpm_us = crtpagemiss  * pagemiss *1000*1000/ mclk_freq;
            us_crt =  cpm_us + us_m + us_n + us_p ;
            clwm = us_crt * crtc_drain_rate/(1000*1000);
            clwm++;
        }
        m1 = clwm + cbs - 512;
        p1 = m1 * pclk_freq / mclk_freq;
        p1 = p1 * bpp / 8;
        if ((p1 < m1) && (m1 > 0))
        {
            fifo->valid = 0;
            found = 0;
            if (mclk_extra ==0)   found = 1;
            mclk_extra--;
        }
        else if (video_enable)
        {
            if ((clwm > 511) || (vlwm > 255))
            {
                fifo->valid = 0;
                found = 0;
                if (mclk_extra ==0)   found = 1;
                mclk_extra--;
            }
        }
        else
        {
            if (clwm > 519)
            {
                fifo->valid = 0;
                found = 0;
                if (mclk_extra ==0)   found = 1;
                mclk_extra--;
            }
        }
        if (clwm < 384) clwm = 384;
        if (vlwm < 128) vlwm = 128;
        data = (int)(clwm);
        fifo->graphics_lwm = data;
        fifo->graphics_burst_size = 128;
        data = (int)((vlwm+15));
        fifo->video_lwm = data;
        fifo->video_burst_size = vbs;
    }
}

static void nv4UpdateArbitrationSettings (
    unsigned      VClk, 
    unsigned      pixelDepth, 
    unsigned     *burst,
    unsigned     *lwm,
    NVPtr        pNv
)
{
    nv4_fifo_info fifo_data;
    nv4_sim_state sim_data;
    unsigned int MClk, NVClk, cfg1;

    nvGetClocks(pNv, &MClk, &NVClk);

    cfg1 = pNv->PFB[0x00000204/4];
    sim_data.pix_bpp        = (char)pixelDepth;
    sim_data.enable_video   = 0;
    sim_data.enable_mp      = 0;
    sim_data.memory_width   = (pNv->PEXTDEV[0x0000/4] & 0x10) ? 128 : 64;
    sim_data.mem_latency    = (char)cfg1 & 0x0F;
    sim_data.mem_aligned    = 1;
    sim_data.mem_page_miss  = (char)(((cfg1 >> 4) &0x0F) + ((cfg1 >> 31) & 0x01));
    sim_data.gr_during_vid  = 0;
    sim_data.pclk_khz       = VClk;
    sim_data.mclk_khz       = MClk;
    sim_data.nvclk_khz      = NVClk;
    nv4CalcArbitration(&fifo_data, &sim_data);
    if (fifo_data.valid)
    {
        int  b = fifo_data.graphics_burst_size >> 4;
        *burst = 0;
        while (b >>= 1) (*burst)++;
        *lwm   = fifo_data.graphics_lwm >> 3;
    }
}

static void nv10CalcArbitration (
    nv10_fifo_info *fifo,
    nv10_sim_state *arb
)
{
    int data, pagemiss, width, video_enable, bpp;
    int nvclks, mclks, pclks, vpagemiss, crtpagemiss;
    int nvclk_fill;
    int found, mclk_extra, mclk_loop, cbs, m1;
    int mclk_freq, pclk_freq, nvclk_freq, mp_enable;
    int us_m, us_m_min, us_n, us_p, crtc_drain_rate;
    int vus_m;
    int vpm_us, us_video, cpm_us, us_crt,clwm;
    int clwm_rnd_down;
    int m2us, us_pipe_min, p1clk, p2;
    int min_mclk_extra;
    int us_min_mclk_extra;

    fifo->valid = 1;
    pclk_freq = arb->pclk_khz; /* freq in KHz */
    mclk_freq = arb->mclk_khz;
    nvclk_freq = arb->nvclk_khz;
    pagemiss = arb->mem_page_miss;
    width = arb->memory_width/64;
    video_enable = arb->enable_video;
    bpp = arb->pix_bpp;
    mp_enable = arb->enable_mp;
    clwm = 0;

    cbs = 512;

    pclks = 4; /* lwm detect. */

    nvclks = 3; /* lwm -> sync. */
    nvclks += 2; /* fbi bus cycles (1 req + 1 busy) */

    mclks  = 1;   /* 2 edge sync.  may be very close to edge so just put one. */

    mclks += 1;   /* arb_hp_req */
    mclks += 5;   /* ap_hp_req   tiling pipeline */

    mclks += 2;    /* tc_req     latency fifo */
    mclks += 2;    /* fb_cas_n_  memory request to fbio block */
    mclks += 7;    /* sm_d_rdv   data returned from fbio block */

    /* fb.rd.d.Put_gc   need to accumulate 256 bits for read */
    if (arb->memory_type == 0)
      if (arb->memory_width == 64) /* 64 bit bus */
        mclks += 4;
      else
        mclks += 2;
    else
      if (arb->memory_width == 64) /* 64 bit bus */
        mclks += 2;
      else
        mclks += 1;

    if ((!video_enable) && (arb->memory_width == 128))
    {  
      mclk_extra = (bpp == 32) ? 31 : 42; /* Margin of error */
      min_mclk_extra = 17;
    }
    else
    {
      mclk_extra = (bpp == 32) ? 8 : 4; /* Margin of error */
      /* mclk_extra = 4; */ /* Margin of error */
      min_mclk_extra = 18;
    }

    nvclks += 1; /* 2 edge sync.  may be very close to edge so just put one. */
    nvclks += 1; /* fbi_d_rdv_n */
    nvclks += 1; /* Fbi_d_rdata */
    nvclks += 1; /* crtfifo load */

    if(mp_enable)
      mclks+=4; /* Mp can get in with a burst of 8. */
    /* Extra clocks determined by heuristics */

    nvclks += 0;
    pclks += 0;
    found = 0;
    while(found != 1) {
      fifo->valid = 1;
      found = 1;
      mclk_loop = mclks+mclk_extra;
      us_m = mclk_loop *1000*1000 / mclk_freq; /* Mclk latency in us */
      us_m_min = mclks * 1000*1000 / mclk_freq; /* Minimum Mclk latency in us */
      us_min_mclk_extra = min_mclk_extra *1000*1000 / mclk_freq;
      us_n = nvclks*1000*1000 / nvclk_freq;/* nvclk latency in us */
      us_p = pclks*1000*1000 / pclk_freq;/* nvclk latency in us */
      us_pipe_min = us_m_min + us_n + us_p;

      vus_m = mclk_loop *1000*1000 / mclk_freq; /* Mclk latency in us */

      if(video_enable) {
        crtc_drain_rate = pclk_freq * bpp/8; /* MB/s */

        vpagemiss = 1; /* self generating page miss */
        vpagemiss += 1; /* One higher priority before */

        crtpagemiss = 2; /* self generating page miss */
        if(mp_enable)
            crtpagemiss += 1; /* if MA0 conflict */

        vpm_us = (vpagemiss * pagemiss)*1000*1000/mclk_freq;

        us_video = vpm_us + vus_m; /* Video has separate read return path */

        cpm_us = crtpagemiss  * pagemiss *1000*1000/ mclk_freq;
        us_crt =
          us_video  /* Wait for video */
          +cpm_us /* CRT Page miss */
          +us_m + us_n +us_p /* other latency */
          ;

        clwm = us_crt * crtc_drain_rate/(1000*1000);
        clwm++; /* fixed point <= float_point - 1.  Fixes that */
      } else {
        crtc_drain_rate = pclk_freq * bpp/8; /* bpp * pclk/8 */

        crtpagemiss = 1; /* self generating page miss */
        crtpagemiss += 1; /* MA0 page miss */
        if(mp_enable)
            crtpagemiss += 1; /* if MA0 conflict */
        cpm_us = crtpagemiss  * pagemiss *1000*1000/ mclk_freq;
        us_crt =  cpm_us + us_m + us_n + us_p ;
        clwm = us_crt * crtc_drain_rate/(1000*1000);
        clwm++; /* fixed point <= float_point - 1.  Fixes that */

          /* Finally, a heuristic check when width == 64 bits */
          if(width == 1){
              nvclk_fill = nvclk_freq * 8;
              if(crtc_drain_rate * 100 >= nvclk_fill * 102)
                      clwm = 0xfff; /*Large number to fail */

              else if(crtc_drain_rate * 100  >= nvclk_fill * 98) {
                  clwm = 1024;
                  cbs = 512;
              }
          }
      }


      /*
        Overfill check:

        */

      clwm_rnd_down = ((int)clwm/8)*8;
      if (clwm_rnd_down < clwm)
          clwm += 8;

      m1 = clwm + cbs -  1024; /* Amount of overfill */
      m2us = us_pipe_min + us_min_mclk_extra;

      /* pclk cycles to drain */
      p1clk = m2us * pclk_freq/(1000*1000); 
      p2 = p1clk * bpp / 8; /* bytes drained. */

      if((p2 < m1) && (m1 > 0)) {
          fifo->valid = 0;
          found = 0;
          if(min_mclk_extra == 0)   {
            if(cbs <= 32) {
              found = 1; /* Can't adjust anymore! */
            } else {
              cbs = cbs/2;  /* reduce the burst size */
            }
          } else {
            min_mclk_extra--;
          }
      } else {
        if (clwm > 1023){ /* Have some margin */
          fifo->valid = 0;
          found = 0;
          if(min_mclk_extra == 0)   
              found = 1; /* Can't adjust anymore! */
          else 
              min_mclk_extra--;
        }
      }

      if(clwm < (1024-cbs+8)) clwm = 1024-cbs+8;
      data = (int)(clwm);
      /*  printf("CRT LWM: %f bytes, prog: 0x%x, bs: 256\n", clwm, data ); */
      fifo->graphics_lwm = data;   fifo->graphics_burst_size = cbs;

      fifo->video_lwm = 1024;  fifo->video_burst_size = 512;
    }
}

static void nv10UpdateArbitrationSettings (
    unsigned      VClk, 
    unsigned      pixelDepth, 
    unsigned     *burst,
    unsigned     *lwm,
    NVPtr        pNv
)
{
    nv10_fifo_info fifo_data;
    nv10_sim_state sim_data;
    unsigned int MClk, NVClk, cfg1;

    nvGetClocks(pNv, &MClk, &NVClk);

    cfg1 = pNv->PFB[0x0204/4];
    sim_data.pix_bpp        = (char)pixelDepth;
    sim_data.enable_video   = 1;
    sim_data.enable_mp      = 0;
    sim_data.memory_type    = (pNv->PFB[0x0200/4] & 0x01) ? 1 : 0;
    sim_data.memory_width   = (pNv->PEXTDEV[0x0000/4] & 0x10) ? 128 : 64;
    sim_data.mem_latency    = (char)cfg1 & 0x0F;
    sim_data.mem_aligned    = 1;
    sim_data.mem_page_miss  = (char)(((cfg1>>4) &0x0F) + ((cfg1>>31) & 0x01));
    sim_data.gr_during_vid  = 0;
    sim_data.pclk_khz       = VClk;
    sim_data.mclk_khz       = MClk;
    sim_data.nvclk_khz      = NVClk;
    nv10CalcArbitration(&fifo_data, &sim_data);
    if (fifo_data.valid) {
        int  b = fifo_data.graphics_burst_size >> 4;
        *burst = 0;
        while (b >>= 1) (*burst)++;
        *lwm   = fifo_data.graphics_lwm >> 3;
    }
}


static void nv30UpdateArbitrationSettings (
    NVPtr        pNv,
    unsigned     *burst,
    unsigned     *lwm
)   
{
    unsigned int MClk, NVClk;
    unsigned int fifo_size, burst_size, graphics_lwm;

    fifo_size = 2048;
    burst_size = 512;
    graphics_lwm = fifo_size - burst_size;

    nvGetClocks(pNv, &MClk, &NVClk);
    
    *burst = 0;
    burst_size >>= 5;
    while(burst_size >>= 1) (*burst)++;
    *lwm = graphics_lwm >> 3;
}

static void nForceUpdateArbitrationSettings (
    unsigned      VClk,
    unsigned      pixelDepth,
    unsigned     *burst,
    unsigned     *lwm,
    NVPtr        pNv
)
{
    nv10_fifo_info fifo_data;
    nv10_sim_state sim_data;
    unsigned int M, N, P, pll, MClk, NVClk, memctrl;

    if((pNv->Chipset & 0x0FF0) == CHIPSET_NFORCE) {
       unsigned int uMClkPostDiv;

       uMClkPostDiv = (pciReadLong(pciTag(0, 0, 3), 0x6C) >> 8) & 0xf;
       if(!uMClkPostDiv) uMClkPostDiv = 4; 
       MClk = 400000 / uMClkPostDiv;
    } else {
       MClk = pciReadLong(pciTag(0, 0, 5), 0x4C) / 1000;
    }

    pll = pNv->PRAMDAC0[0x0500/4];
    M = (pll >> 0)  & 0xFF; N = (pll >> 8)  & 0xFF; P = (pll >> 16) & 0x0F;
    NVClk  = (N * pNv->CrystalFreqKHz / M) >> P;
    sim_data.pix_bpp        = (char)pixelDepth;
    sim_data.enable_video   = 0;
    sim_data.enable_mp      = 0;
    sim_data.memory_type    = (pciReadLong(pciTag(0, 0, 1), 0x7C) >> 12) & 1;
    sim_data.memory_width   = 64;

    memctrl = pciReadLong(pciTag(0, 0, 3), 0x00) >> 16;

    if((memctrl == 0x1A9) || (memctrl == 0x1AB) || (memctrl == 0x1ED)) {
        int dimm[3];

        dimm[0] = (pciReadLong(pciTag(0, 0, 2), 0x40) >> 8) & 0x4F;
        dimm[1] = (pciReadLong(pciTag(0, 0, 2), 0x44) >> 8) & 0x4F;
        dimm[2] = (pciReadLong(pciTag(0, 0, 2), 0x48) >> 8) & 0x4F;

        if((dimm[0] + dimm[1]) != dimm[2]) {
             ErrorF("WARNING: "
              "your nForce DIMMs are not arranged in optimal banks!\n");
        } 
    }

    sim_data.mem_latency    = 3;
    sim_data.mem_aligned    = 1;
    sim_data.mem_page_miss  = 10;
    sim_data.gr_during_vid  = 0;
    sim_data.pclk_khz       = VClk;
    sim_data.mclk_khz       = MClk;
    sim_data.nvclk_khz      = NVClk;
    nv10CalcArbitration(&fifo_data, &sim_data);
    if (fifo_data.valid)
    {
        int  b = fifo_data.graphics_burst_size >> 4;
        *burst = 0;
        while (b >>= 1) (*burst)++;
        *lwm   = fifo_data.graphics_lwm >> 3;
    }
}


/****************************************************************************\
*                                                                            *
*                          RIVA Mode State Routines                          *
*                                                                            *
\****************************************************************************/

/*
 * Calculate the Video Clock parameters for the PLL.
 */
static void CalcVClock (
    int           clockIn,
    int          *clockOut,
    U032         *pllOut,
    NVPtr        pNv
)
{
    unsigned lowM, highM;
    unsigned DeltaNew, DeltaOld;
    unsigned VClk, Freq;
    unsigned M, N, P;
    
    DeltaOld = 0xFFFFFFFF;

    VClk = (unsigned)clockIn;
    
    if (pNv->CrystalFreqKHz == 13500) {
        lowM  = 7;
        highM = 13;
    } else {
        lowM  = 8;
        highM = 14;
    }

    for (P = 0; P <= 4; P++) {
        Freq = VClk << P;
        if ((Freq >= 128000) && (Freq <= 350000)) {
            for (M = lowM; M <= highM; M++) {
                N = ((VClk << P) * M) / pNv->CrystalFreqKHz;
                if(N <= 255) {
                    Freq = ((pNv->CrystalFreqKHz * N) / M) >> P;
                    if (Freq > VClk)
                        DeltaNew = Freq - VClk;
                    else
                        DeltaNew = VClk - Freq;
                    if (DeltaNew < DeltaOld) {
                        *pllOut   = (P << 16) | (N << 8) | M;
                        *clockOut = Freq;
                        DeltaOld  = DeltaNew;
                    }
                }
            }
        }
    }
}

static void CalcVClock2Stage (
    int           clockIn,
    int          *clockOut,
    U032         *pllOut,
    U032         *pllBOut,
    NVPtr        pNv
)
{
    unsigned DeltaNew, DeltaOld;
    unsigned VClk, Freq;
    unsigned M, N, P;

    DeltaOld = 0xFFFFFFFF;

    *pllBOut = 0x80000401;  /* fixed at x4 for now */

    VClk = (unsigned)clockIn;

    for (P = 0; P <= 6; P++) {
        Freq = VClk << P;
        if ((Freq >= 400000) && (Freq <= 1000000)) {
            for (M = 1; M <= 13; M++) {
                N = ((VClk << P) * M) / (pNv->CrystalFreqKHz << 2);
                if((N >= 5) && (N <= 255)) {
                    Freq = (((pNv->CrystalFreqKHz << 2) * N) / M) >> P;
                    if (Freq > VClk)
                        DeltaNew = Freq - VClk;
                    else
                        DeltaNew = VClk - Freq;
                    if (DeltaNew < DeltaOld) {
                        *pllOut   = (P << 16) | (N << 8) | M;
                        *clockOut = Freq;
                        DeltaOld  = DeltaNew;
                    }
                }
            }
        }
    }
}

/*
 * Calculate extended mode parameters (SVGA) and save in a 
 * mode state structure.
 */
void NVCalcStateExt (
    NVPtr pNv,
    RIVA_HW_STATE *state,
    int            bpp,
    int            width,
    int            hDisplaySize,
    int            height,
    int            dotClock,
    int		   flags 
)
{
    int pixelDepth, VClk;
    /*
     * Save mode parameters.
     */
    state->bpp    = bpp;    /* this is not bitsPerPixel, it's 8,15,16,32 */
    state->width  = width;
    state->height = height;
    /*
     * Extended RIVA registers.
     */
    pixelDepth = (bpp + 1)/8;
    if(pNv->twoStagePLL)
        CalcVClock2Stage(dotClock, &VClk, &state->pll, &state->pllB, pNv);
    else
        CalcVClock(dotClock, &VClk, &state->pll, pNv);

    switch (pNv->Architecture)
    {
        case NV_ARCH_04:
            nv4UpdateArbitrationSettings(VClk, 
                                         pixelDepth * 8, 
                                        &(state->arbitration0),
                                        &(state->arbitration1),
                                         pNv);
            state->cursor0  = 0x00;
            state->cursor1  = 0xbC;
	    if (flags & V_DBLSCAN)
		state->cursor1 |= 2;
            state->cursor2  = 0x00000000;
            state->pllsel   = 0x10000700;
            state->config   = 0x00001114;
            state->general  = bpp == 16 ? 0x00101100 : 0x00100100;
            state->repaint1 = hDisplaySize < 1280 ? 0x04 : 0x00;
            break;
        case NV_ARCH_10:
        case NV_ARCH_20:
        case NV_ARCH_30:
        default:
            if(((pNv->Chipset & 0xfff0) == CHIPSETS_C51) ||
               ((pNv->Chipset & 0xfff0) == 0x03D0))
            {
                state->arbitration0 = 256; 
                state->arbitration1 = 0x0480; 
            } else
            if(((pNv->Chipset & 0xffff) == CHIPSET_NFORCE) ||
               ((pNv->Chipset & 0xffff) == CHIPSET_NFORCE2))
            {
                nForceUpdateArbitrationSettings(VClk,
                                          pixelDepth * 8,
                                         &(state->arbitration0),
                                         &(state->arbitration1),
                                          pNv);
            } else if(pNv->Architecture < NV_ARCH_30) {
                nv10UpdateArbitrationSettings(VClk, 
                                          pixelDepth * 8, 
                                         &(state->arbitration0),
                                         &(state->arbitration1),
                                          pNv);
            } else {
                nv30UpdateArbitrationSettings(pNv,
                                         &(state->arbitration0),
                                         &(state->arbitration1));
            }
            state->cursor0  = 0x80 | (pNv->CursorStart >> 17);
            state->cursor1  = (pNv->CursorStart >> 11) << 2;
	    state->cursor2  = pNv->CursorStart >> 24;
	    if (flags & V_DBLSCAN) 
		state->cursor1 |= 2;
            state->pllsel   = 0x10000700;
            state->config   = pNv->PFB[0x00000200/4];
            state->general  = bpp == 16 ? 0x00101100 : 0x00100100;
            state->repaint1 = hDisplaySize < 1280 ? 0x04 : 0x00;
            break;
    }

    if(bpp != 8) /* DirectColor */
	state->general |= 0x00000030;

    state->repaint0 = (((width / 8) * pixelDepth) & 0x700) >> 3;
    state->pixel    = (pixelDepth > 2) ? 3 : pixelDepth;
}


void NVLoadStateExt (
    ScrnInfoPtr pScrn,
    RIVA_HW_STATE *state
)
{
    NVPtr pNv = NVPTR(pScrn);
    int i, j;

    if (!pNv->IRQ)
        pNv->PMC[0x0140/4] = 0x00000000;
    pNv->PMC[0x0200/4] = 0xFFFF00FF;
    pNv->PMC[0x0200/4] = 0xFFFFFFFF;

    pNv->PTIMER[0x0200] = 0x00000008;
    pNv->PTIMER[0x0210] = 0x00000003;
    /*TODO: DRM handle PTIMER interrupts */
    pNv->PTIMER[0x0140] = 0x00000000;
    pNv->PTIMER[0x0100] = 0xFFFFFFFF;

    if(pNv->Architecture == NV_ARCH_04) {
        pNv->PFB[0x0200/4] = state->config;
    } else 
    if((pNv->Architecture < NV_ARCH_40) ||
       ((pNv->Chipset & 0xfff0) == CHIPSET_NV40))
    {
        for(i = 0; i < 8; i++) {
           pNv->PFB[(0x0240 + (i * 0x10))/4] = 0;
           pNv->PFB[(0x0244 + (i * 0x10))/4] = pNv->FbMapSize - 1;
        }
    } else {
        int regions = 12;

        if(((pNv->Chipset & 0xfff0) == CHIPSET_G70) ||
           ((pNv->Chipset & 0xfff0) == CHIPSET_G71) ||
           ((pNv->Chipset & 0xfff0) == CHIPSET_G72) ||
           ((pNv->Chipset & 0xfff0) == CHIPSET_G73) ||
           ((pNv->Chipset & 0xfff0) == 0x03D0))
        {
           regions = 15;
        }
 
       for(i = 0; i < regions; i++) {
          pNv->PFB[(0x0600 + (i * 0x10))/4] = 0;
          pNv->PFB[(0x0604 + (i * 0x10))/4] = pNv->FbMapSize - 1;
       }
    }

#if 1
		/*FIXME: do this in the DRM */
	    /* setup DMA object for command buffer */
	    pNv->PRAMIN[0x07f8] = 0x00003002;
	    pNv->PRAMIN[0x07f9] = 0x00007FFF;
	    pNv->PRAMIN[0x07fa] = pNv->FbUsableSize | 0x00000002;
	    pNv->PRAMIN[0x07fb] = 0x00000002;
#endif

    if(pNv->Architecture < NV_ARCH_10) {
       if((pNv->Chipset & 0x0fff) == CHIPSET_NV04) {
           pNv->PRAMIN[0x0824] |= 0x00020000;
           pNv->PRAMIN[0x0826] += pNv->FbAddress;
       }
       pNv->PGRAPH[0x0080/4] = 0x000001FF;
       pNv->PGRAPH[0x0080/4] = 0x1230C000;
       pNv->PGRAPH[0x0084/4] = 0x72111101;
       pNv->PGRAPH[0x0088/4] = 0x11D5F071;
       pNv->PGRAPH[0x008C/4] = 0x0004FF31;
       pNv->PGRAPH[0x008C/4] = 0x4004FF31;

       if (!pNv->IRQ) {
           pNv->PGRAPH[0x0140/4] = 0x00000000;
           pNv->PGRAPH[0x0100/4] = 0xFFFFFFFF;
       }
       pNv->PGRAPH[0x0170/4] = 0x10010100;
       pNv->PGRAPH[0x0710/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0720/4] = 0x00000001;

       pNv->PGRAPH[0x0810/4] = 0x00000000;
       pNv->PGRAPH[0x0608/4] = 0xFFFFFFFF; 
    } else {
       pNv->PGRAPH[0x0080/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0080/4] = 0x00000000;

       if (!pNv->IRQ) {
           pNv->PGRAPH[0x0140/4] = 0x00000000;
           pNv->PGRAPH[0x0100/4] = 0xFFFFFFFF;
       }
       pNv->PGRAPH[0x0144/4] = 0x10010100;
       pNv->PGRAPH[0x0714/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0720/4] = 0x00000001;
       pNv->PGRAPH[0x0710/4] &= 0x0007ff00;
       pNv->PGRAPH[0x0710/4] |= 0x00020100;

       if(pNv->Architecture == NV_ARCH_10) {
           pNv->PGRAPH[0x0084/4] = 0x00118700;
           pNv->PGRAPH[0x0088/4] = 0x24E00810;
           pNv->PGRAPH[0x008C/4] = 0x55DE0030;

           for(i = 0; i < 32; i++)
             pNv->PGRAPH[(0x0B00/4) + i] = pNv->PFB[(0x0240/4) + i];

           pNv->PGRAPH[0x640/4] = 0;
           pNv->PGRAPH[0x644/4] = 0;
           pNv->PGRAPH[0x684/4] = pNv->FbMapSize - 1;
           pNv->PGRAPH[0x688/4] = pNv->FbMapSize - 1;

           pNv->PGRAPH[0x0810/4] = 0x00000000;
           pNv->PGRAPH[0x0608/4] = 0xFFFFFFFF;
       } else {
           if(pNv->Architecture >= NV_ARCH_40) {
              pNv->PGRAPH[0x0084/4] = 0x401287c0;
              pNv->PGRAPH[0x008C/4] = 0x60de8051;
              pNv->PGRAPH[0x0090/4] = 0x00008000;
              pNv->PGRAPH[0x0610/4] = 0x00be3c5f;

              j = pNv->REGS[0x1540/4] & 0xff;
              if(j) {
                  for(i = 0; !(j & 1); j >>= 1, i++);
                  pNv->PGRAPH[0x5000/4] = i;
              }

              if((pNv->Chipset & 0xfff0) == CHIPSET_NV40) {
                 pNv->PGRAPH[0x09b0/4] = 0x83280fff;
                 pNv->PGRAPH[0x09b4/4] = 0x000000a0;
              } else {
                 pNv->PGRAPH[0x0820/4] = 0x83280eff;
                 pNv->PGRAPH[0x0824/4] = 0x000000a0;
              }

              switch(pNv->Chipset & 0xfff0) {
              case CHIPSET_NV40:
              case CHIPSET_NV45:
                 pNv->PGRAPH[0x09b8/4] = 0x0078e366;
                 pNv->PGRAPH[0x09bc/4] = 0x0000014c;
                 pNv->PFB[0x033C/4] &= 0xffff7fff;
                 break;
              case CHIPSET_NV41:
              case 0x0120:
                 pNv->PGRAPH[0x0828/4] = 0x007596ff;
                 pNv->PGRAPH[0x082C/4] = 0x00000108;
                 break;
              case CHIPSET_NV44:
              case CHIPSET_G72:
              case CHIPSET_C51:
              case 0x03D0:
                 pNv->PMC[0x1700/4] = pNv->PFB[0x020C/4];
                 pNv->PMC[0x1704/4] = 0;
                 pNv->PMC[0x1708/4] = 0;
                 pNv->PMC[0x170C/4] = pNv->PFB[0x020C/4];
                 pNv->PGRAPH[0x0860/4] = 0;
                 pNv->PGRAPH[0x0864/4] = 0;
                 pNv->PRAMDAC[0x0608/4] |= 0x00100000;
                 break;
              case CHIPSET_NV43:
                 pNv->PGRAPH[0x0828/4] = 0x0072cb77;
                 pNv->PGRAPH[0x082C/4] = 0x00000108;
                 break;
              case CHIPSET_NV44A:
                 pNv->PGRAPH[0x0860/4] = 0;
                 pNv->PGRAPH[0x0864/4] = 0;
                 pNv->PRAMDAC[0x0608/4] |= 0x00100000;
                 break;
              case CHIPSET_G70:
              case CHIPSET_G71:
              case CHIPSET_G73:
                 pNv->PRAMDAC[0x0608/4] |= 0x00100000;
                 pNv->PGRAPH[0x0828/4] = 0x07830610;
                 pNv->PGRAPH[0x082C/4] = 0x0000016A;
                 break;
              default:
                 break;
              };

              pNv->PGRAPH[0x0b38/4] = 0x2ffff800;
              pNv->PGRAPH[0x0b3c/4] = 0x00006000;
              pNv->PGRAPH[0x032C/4] = 0x01000000; 
           } else
           if(pNv->Architecture == NV_ARCH_30) {
              pNv->PGRAPH[0x0084/4] = 0x40108700;
              pNv->PGRAPH[0x0890/4] = 0x00140000;
              pNv->PGRAPH[0x008C/4] = 0xf00e0431;
              pNv->PGRAPH[0x0090/4] = 0x00008000;
              pNv->PGRAPH[0x0610/4] = 0xf04b1f36;
              pNv->PGRAPH[0x0B80/4] = 0x1002d888;
              pNv->PGRAPH[0x0B88/4] = 0x62ff007f;
           } else {
              pNv->PGRAPH[0x0084/4] = 0x00118700;
              pNv->PGRAPH[0x008C/4] = 0xF20E0431;
              pNv->PGRAPH[0x0090/4] = 0x00000000;
              pNv->PGRAPH[0x009C/4] = 0x00000040;

              if((pNv->Chipset & 0x0ff0) >= CHIPSET_NV25) {
                 pNv->PGRAPH[0x0890/4] = 0x00080000;
                 pNv->PGRAPH[0x0610/4] = 0x304B1FB6; 
                 pNv->PGRAPH[0x0B80/4] = 0x18B82880; 
                 pNv->PGRAPH[0x0B84/4] = 0x44000000; 
                 pNv->PGRAPH[0x0098/4] = 0x40000080; 
                 pNv->PGRAPH[0x0B88/4] = 0x000000ff; 
              } else {
                 pNv->PGRAPH[0x0880/4] = 0x00080000;
                 pNv->PGRAPH[0x0094/4] = 0x00000005;
                 pNv->PGRAPH[0x0B80/4] = 0x45CAA208; 
                 pNv->PGRAPH[0x0B84/4] = 0x24000000;
                 pNv->PGRAPH[0x0098/4] = 0x00000040;
                 pNv->PGRAPH[0x0750/4] = 0x00E00038;
                 pNv->PGRAPH[0x0754/4] = 0x00000030;
                 pNv->PGRAPH[0x0750/4] = 0x00E10038;
                 pNv->PGRAPH[0x0754/4] = 0x00000030;
              }
           }

           if((pNv->Architecture < NV_ARCH_40) ||
              ((pNv->Chipset & 0xfff0) == CHIPSET_NV40)) 
           {
              for(i = 0; i < 32; i++) {
                pNv->PGRAPH[(0x0900/4) + i] = pNv->PFB[(0x0240/4) + i];
                pNv->PGRAPH[(0x6900/4) + i] = pNv->PFB[(0x0240/4) + i];
              }
           } else {
              if(((pNv->Chipset & 0xfff0) == CHIPSET_G70) ||
                 ((pNv->Chipset & 0xfff0) == CHIPSET_G71) ||
                 ((pNv->Chipset & 0xfff0) == CHIPSET_G72) ||
                 ((pNv->Chipset & 0xfff0) == CHIPSET_G73) ||
                 ((pNv->Chipset & 0xfff0) == 0x03D0))
              {
                 for(i = 0; i < 60; i++) {
                   pNv->PGRAPH[(0x0D00/4) + i] = pNv->PFB[(0x0600/4) + i];
                   pNv->PGRAPH[(0x6900/4) + i] = pNv->PFB[(0x0600/4) + i];
                 }
              } else {
                 for(i = 0; i < 48; i++) {
                   pNv->PGRAPH[(0x0900/4) + i] = pNv->PFB[(0x0600/4) + i];
                   if(((pNv->Chipset & 0xfff0) != CHIPSET_NV44) &&
                      ((pNv->Chipset & 0xfff0) != CHIPSET_NV44A) &&
                      ((pNv->Chipset & 0xfff0) != CHIPSET_C51))
                   {
                      pNv->PGRAPH[(0x6900/4) + i] = pNv->PFB[(0x0600/4) + i];
                   }
                 }
              }
           }

           if(pNv->Architecture >= NV_ARCH_40) {
              if((pNv->Chipset & 0xfff0) == CHIPSET_NV40) {
                 pNv->PGRAPH[0x09A4/4] = pNv->PFB[0x0200/4];
                 pNv->PGRAPH[0x09A8/4] = pNv->PFB[0x0204/4];
                 pNv->PGRAPH[0x69A4/4] = pNv->PFB[0x0200/4];
                 pNv->PGRAPH[0x69A8/4] = pNv->PFB[0x0204/4];

                 pNv->PGRAPH[0x0820/4] = 0;
                 pNv->PGRAPH[0x0824/4] = 0;
                 pNv->PGRAPH[0x0864/4] = pNv->FbMapSize - 1;
                 pNv->PGRAPH[0x0868/4] = pNv->FbMapSize - 1;
              } else {
                 if(((pNv->Chipset & 0xfff0) == CHIPSET_G70) ||
                    ((pNv->Chipset & 0xfff0) == CHIPSET_G71) ||
                    ((pNv->Chipset & 0xfff0) == CHIPSET_G72) ||
                    ((pNv->Chipset & 0xfff0) == CHIPSET_G73)) 
                 {
                    pNv->PGRAPH[0x0DF0/4] = pNv->PFB[0x0200/4];
                    pNv->PGRAPH[0x0DF4/4] = pNv->PFB[0x0204/4];
                 } else {
                    pNv->PGRAPH[0x09F0/4] = pNv->PFB[0x0200/4];
                    pNv->PGRAPH[0x09F4/4] = pNv->PFB[0x0204/4];
                 }
                 pNv->PGRAPH[0x69F0/4] = pNv->PFB[0x0200/4];
                 pNv->PGRAPH[0x69F4/4] = pNv->PFB[0x0204/4];

                 pNv->PGRAPH[0x0840/4] = 0;
                 pNv->PGRAPH[0x0844/4] = 0;
                 pNv->PGRAPH[0x08a0/4] = pNv->FbMapSize - 1;
                 pNv->PGRAPH[0x08a4/4] = pNv->FbMapSize - 1;
              }
           } else {
              pNv->PGRAPH[0x09A4/4] = pNv->PFB[0x0200/4];
              pNv->PGRAPH[0x09A8/4] = pNv->PFB[0x0204/4];
              pNv->PGRAPH[0x0750/4] = 0x00EA0000;
              pNv->PGRAPH[0x0754/4] = pNv->PFB[0x0200/4];
              pNv->PGRAPH[0x0750/4] = 0x00EA0004;
              pNv->PGRAPH[0x0754/4] = pNv->PFB[0x0204/4];

              pNv->PGRAPH[0x0820/4] = 0;
              pNv->PGRAPH[0x0824/4] = 0;
              pNv->PGRAPH[0x0864/4] = pNv->FbMapSize - 1;
              pNv->PGRAPH[0x0868/4] = pNv->FbMapSize - 1;
           }

           pNv->PGRAPH[0x0B20/4] = 0x00000000;
           pNv->PGRAPH[0x0B04/4] = 0xFFFFFFFF;
       }
    }
    pNv->PGRAPH[0x053C/4] = 0;
    pNv->PGRAPH[0x0540/4] = 0;
    pNv->PGRAPH[0x0544/4] = 0x00007FFF;
    pNv->PGRAPH[0x0548/4] = 0x00007FFF;

    /* Seems we have to reinit some/all of the FIFO regs on a mode switch */
    drmCommandNone(pNv->drm_fd, DRM_NOUVEAU_PFIFO_REINIT);

    if(pNv->Architecture >= NV_ARCH_10) {
        if(pNv->twoHeads) {
           pNv->PCRTC0[0x0860/4] = state->head;
           pNv->PCRTC0[0x2860/4] = state->head2;
        }
        pNv->PRAMDAC[0x0404/4] |= (1 << 25);
    
        pNv->PMC[0x8704/4] = 1;
        pNv->PMC[0x8140/4] = 0;
        pNv->PMC[0x8920/4] = 0;
        pNv->PMC[0x8924/4] = 0;
        pNv->PMC[0x8908/4] = pNv->FbMapSize - 1;
        pNv->PMC[0x890C/4] = pNv->FbMapSize - 1;
        pNv->PMC[0x1588/4] = 0;

        pNv->PCRTC[0x0810/4] = state->cursorConfig;
        pNv->PCRTC[0x0830/4] = state->displayV - 3;
        pNv->PCRTC[0x0834/4] = state->displayV - 1;
    
        if(pNv->FlatPanel) {
           if((pNv->Chipset & 0x0ff0) == CHIPSET_NV11) {
               pNv->PRAMDAC[0x0528/4] = state->dither;
           } else 
           if(pNv->twoHeads) {
               pNv->PRAMDAC[0x083C/4] = state->dither;
           }
    
           VGA_WR08(pNv->PCIO, 0x03D4, 0x53);
           VGA_WR08(pNv->PCIO, 0x03D5, state->timingH);
           VGA_WR08(pNv->PCIO, 0x03D4, 0x54);
           VGA_WR08(pNv->PCIO, 0x03D5, state->timingV);
           VGA_WR08(pNv->PCIO, 0x03D4, 0x21);
           VGA_WR08(pNv->PCIO, 0x03D5, 0xfa);
        }

        VGA_WR08(pNv->PCIO, 0x03D4, 0x41);
        VGA_WR08(pNv->PCIO, 0x03D5, state->extra);
    }

    VGA_WR08(pNv->PCIO, 0x03D4, 0x19);
    VGA_WR08(pNv->PCIO, 0x03D5, state->repaint0);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1A);
    VGA_WR08(pNv->PCIO, 0x03D5, state->repaint1);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x25);
    VGA_WR08(pNv->PCIO, 0x03D5, state->screen);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x28);
    VGA_WR08(pNv->PCIO, 0x03D5, state->pixel);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x2D);
    VGA_WR08(pNv->PCIO, 0x03D5, state->horiz);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1C);
    VGA_WR08(pNv->PCIO, 0x03D5, state->fifo);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1B);
    VGA_WR08(pNv->PCIO, 0x03D5, state->arbitration0);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x20);
    VGA_WR08(pNv->PCIO, 0x03D5, state->arbitration1);
    if(pNv->Architecture >= NV_ARCH_30) {
      VGA_WR08(pNv->PCIO, 0x03D4, 0x47);
      VGA_WR08(pNv->PCIO, 0x03D5, state->arbitration1 >> 8);
    }
    VGA_WR08(pNv->PCIO, 0x03D4, 0x30);
    VGA_WR08(pNv->PCIO, 0x03D5, state->cursor0);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x31);
    VGA_WR08(pNv->PCIO, 0x03D5, state->cursor1);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x2F);
    VGA_WR08(pNv->PCIO, 0x03D5, state->cursor2);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x39);
    VGA_WR08(pNv->PCIO, 0x03D5, state->interlace);

    if(!pNv->FlatPanel) {
       pNv->PRAMDAC0[0x050C/4] = state->pllsel;
       pNv->PRAMDAC0[0x0508/4] = state->vpll;
       if(pNv->twoHeads)
          pNv->PRAMDAC0[0x0520/4] = state->vpll2;
       if(pNv->twoStagePLL) {
          pNv->PRAMDAC0[0x0578/4] = state->vpllB;
          pNv->PRAMDAC0[0x057C/4] = state->vpll2B;
       }
    } else {
       pNv->PRAMDAC[0x0848/4] = state->scale;
       pNv->PRAMDAC[0x0828/4] = state->crtcSync;
    }
    pNv->PRAMDAC[0x0600/4] = state->general;

    pNv->PCRTC[0x0140/4] = 0;
    pNv->PCRTC[0x0100/4] = 1;

    pNv->CurrentState = state;
}

void NVUnloadStateExt
(
    NVPtr pNv,
    RIVA_HW_STATE *state
)
{
    VGA_WR08(pNv->PCIO, 0x03D4, 0x19);
    state->repaint0     = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1A);
    state->repaint1     = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x25);
    state->screen       = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x28);
    state->pixel        = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x2D);
    state->horiz        = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1C);
    state->fifo         = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1B);
    state->arbitration0 = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x20);
    state->arbitration1 = VGA_RD08(pNv->PCIO, 0x03D5);
    if(pNv->Architecture >= NV_ARCH_30) {
       VGA_WR08(pNv->PCIO, 0x03D4, 0x47);
       state->arbitration1 |= (VGA_RD08(pNv->PCIO, 0x03D5) & 1) << 8;
    }
    VGA_WR08(pNv->PCIO, 0x03D4, 0x30);
    state->cursor0      = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x31);
    state->cursor1      = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x2F);
    state->cursor2      = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x39);
    state->interlace    = VGA_RD08(pNv->PCIO, 0x03D5);
    state->vpll         = pNv->PRAMDAC0[0x0508/4];
    if(pNv->twoHeads)
       state->vpll2     = pNv->PRAMDAC0[0x0520/4];
    if(pNv->twoStagePLL) {
        state->vpllB    = pNv->PRAMDAC0[0x0578/4];
        state->vpll2B   = pNv->PRAMDAC0[0x057C/4];
    }
    state->pllsel       = pNv->PRAMDAC0[0x050C/4];
    state->general      = pNv->PRAMDAC[0x0600/4];
    state->scale        = pNv->PRAMDAC[0x0848/4];
    state->config       = pNv->PFB[0x0200/4];

    if(pNv->Architecture >= NV_ARCH_10) {
        if(pNv->twoHeads) {
           state->head     = pNv->PCRTC0[0x0860/4];
           state->head2    = pNv->PCRTC0[0x2860/4];
           VGA_WR08(pNv->PCIO, 0x03D4, 0x44);
           state->crtcOwner = VGA_RD08(pNv->PCIO, 0x03D5);
        }
        VGA_WR08(pNv->PCIO, 0x03D4, 0x41);
        state->extra = VGA_RD08(pNv->PCIO, 0x03D5);
        state->cursorConfig = pNv->PCRTC[0x0810/4];

        if((pNv->Chipset & 0x0ff0) == CHIPSET_NV11) {
           state->dither = pNv->PRAMDAC[0x0528/4];
        } else 
        if(pNv->twoHeads) {
            state->dither = pNv->PRAMDAC[0x083C/4];
        }

        if(pNv->FlatPanel) {
           VGA_WR08(pNv->PCIO, 0x03D4, 0x53);
           state->timingH = VGA_RD08(pNv->PCIO, 0x03D5);
           VGA_WR08(pNv->PCIO, 0x03D4, 0x54);
           state->timingV = VGA_RD08(pNv->PCIO, 0x03D5);
        }
    }

    if(pNv->FlatPanel) {
       state->crtcSync = pNv->PRAMDAC[0x0828/4];
    }
}

void NVSetStartAddress (
    NVPtr   pNv,
    CARD32 start
)
{
    pNv->PCRTC[0x800/4] = start;
}


