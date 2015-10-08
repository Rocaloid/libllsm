/*
libpyin
===

Copyright (c) 2015, Kanru Hua
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "external/libgvps/gvps.h"
#include "math-funcs.h"
#include "pyin.h"

FP_TYPE* pyin_yincorr(FP_TYPE* x, int nx, int w);

static int* find_valleys(FP_TYPE* x, int nx, FP_TYPE threshold, FP_TYPE step, int* nv) {
  int* ret = calloc(nx, sizeof(int));
  *nv = 0;
  for(int i = 1; i < nx - 1; i ++)
    if(x[i - 1] > x[i] && x[i + 1] > x[i] && x[i] < threshold) {
      threshold = x[i] - step;
      ret[(*nv) ++] = i;
    }
  return ret;
}

double ptransition_same(void* task, int ds, int t) {
  int ntran = *((int*)task);
  return 0.998 * (1.0 - (double)ds / (ntran + 1)) * (ntran + 1);
}
    
double ptransition_diff(void* task, int ds, int t) {
  int ntran = *((int*)task);
  return 0.002 * (1.0 - (double)ds / (ntran + 1)) * (ntran + 1);
}
    
int fntran(void* task, int t) {
  return *((int*)task);
}

pyin_paramters pyin_init(int nhop) {
  pyin_paramters ret;
  ret.fmin = 50.0;
  ret.fmax = 800.0;
  ret.nq = 480;
  ret.w = 300;
  ret.beta_a = 1.7;
  ret.beta_u = 0.2;
  ret.emph = 0.5;
  ret.trange = 12;
  ret.nf = 1024;
  ret.nhop = nhop;
  return ret;
}

FP_TYPE* pyin_analyze(pyin_paramters param, FP_TYPE* x, int nx, FP_TYPE fs, int* nfrm) {
  int nf = param.nf;
  int yin_w = param.w;
  int nhop = param.nhop;
  *nfrm = nx / nhop;
  FP_TYPE* ret = calloc(*nfrm, sizeof(FP_TYPE));
  int* pint = calloc(*nfrm, sizeof(int));
  int nd = nf - yin_w;
  
  pyin_semitone_wrapper smtdesc = pyin_wrapper_from_frange(param.fmin, param.fmax);
  smtdesc.nq = param.nq;

  FP_TYPE* betapdf = pyin_normalized_betapdf(param.beta_a,
    pyin_beta_b_from_au(param.beta_a, param.beta_u), 0, 1, 100);
  gvps_obsrv* obsrv = gvps_obsrv_create(*nfrm);
  
  for(int i = 0; i < *nfrm; i ++) {
    FP_TYPE* xfrm = fetch_frame(x, nx, i * nhop, nf);
    FP_TYPE xmean = sumfp(xfrm, nf) / nf;
    for(int j = 0; j < nf; j ++)
      xfrm[j] -= xmean;
    
    int nv = 0;
    FP_TYPE* d = pyin_yincorr(xfrm, nf, yin_w);
    int* vi = find_valleys(d, nd, 1, 0.01, & nv);

    obsrv -> slice[i] = gvps_obsrv_slice_create(nv);
    for(int j = 0; j < nv; j ++) {
      int period = vi[j];
      FP_TYPE freq = fs / period;
      int bin = pyin_semitone_from_freq(smtdesc, freq);

      FP_TYPE p = 0;
      FP_TYPE v0 = j == 0 ? 1 : (d[vi[j - 1]] + EPS);
      FP_TYPE v1 = d[vi[j]] + EPS;
      for(int k = floor(v1 * 100); k < floor(v0 * 100); k ++)
        p += betapdf[k];
      p = p > 0.99 ? 0.99 : p;
      p = (sqrt(1 - (1 - p) * (1 - p)) - p) * param.emph + p;
      
      obsrv -> slice[i] -> pair[j].state = bin;
      obsrv -> slice[i] -> pair[j].p = p;
    }

    free(vi);
    free(d);
    free(xfrm);
  }

  gvps_sparse_sampled_hidden_static(pint, & param.trange, smtdesc.nq, obsrv,
    ptransition_same, ptransition_diff, fntran, 4);
  
  for(int i = 0; i < *nfrm; i ++) {
    if(pint[i] >= smtdesc.nq)
      ret[i] = 0;
    else
      ret[i] = pyin_freq_from_semitone(smtdesc, pint[i]);
  }
  
  // fill in the blank
  int frame_offset = ceil(nf / nhop);
  for(int i = 1; i < *nfrm; i ++)
    if(pint[i] < smtdesc.nq && pint[i - 1] >= smtdesc.nq) // from unvoiced to voiced
      for(int j = 1; j <= frame_offset && i - j >= 0; j ++)
        ret[i - j] = ret[i];
  
  free(betapdf);
  free(pint);
  gvps_obsrv_free(obsrv);
  return ret;
}

