/*
 *  Copyright (c) 2022 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "./vpx_dsp_rtcd.h"
#include "vpx_dsp/loongarch/loopfilter_lsx.h"

void vpx_lpf_horizontal_8_lsx(uint8_t *dst, int32_t stride,
                              const uint8_t *b_limit_ptr,
                              const uint8_t *limit_ptr,
                              const uint8_t *thresh_ptr) {
  __m128i mask, hev, flat, thresh, b_limit, limit;
  __m128i p3, p2, p1, p0, q3, q2, q1, q0;
  __m128i p2_out, p1_out, p0_out, q0_out, q1_out, q2_out;
  __m128i p2_filter8, p1_filter8, p0_filter8;
  __m128i q0_filter8, q1_filter8, q2_filter8;
  __m128i p3_l, p2_l, p1_l, p0_l, q3_l, q2_l, q1_l, q0_l;
  __m128i zero = __lsx_vldi(0);

  int32_t stride2 = stride << 1;
  int32_t stride3 = stride2 + stride;
  int32_t stride4 = stride2 << 1;

  /* load vector elements */
  DUP4_ARG2(__lsx_vldx, dst, -stride4, dst, -stride3, dst, -stride2, dst,
            -stride, p3, p2, p1, p0);
  q0 = __lsx_vld(dst, 0);
  DUP2_ARG2(__lsx_vldx, dst, stride, dst, stride2, q1, q2);
  q3 = __lsx_vldx(dst, stride3);

  thresh = __lsx_vreplgr2vr_b(*thresh_ptr);
  b_limit = __lsx_vreplgr2vr_b(*b_limit_ptr);
  limit = __lsx_vreplgr2vr_b(*limit_ptr);

  LPF_MASK_HEV(p3, p2, p1, p0, q0, q1, q2, q3, limit, b_limit, thresh, hev,
               mask, flat);
  VP9_FLAT4(p3, p2, p0, q0, q2, q3, flat);
  VP9_LPF_FILTER4_4W(p1, p0, q0, q1, mask, hev, p1_out, p0_out, q0_out, q1_out);

  flat = __lsx_vilvl_d(zero, flat);

  if (__lsx_bz_v(flat)) {
    __lsx_vstelm_d(p1_out, dst - stride2, 0, 0);
    __lsx_vstelm_d(p0_out, dst - stride, 0, 0);
    __lsx_vstelm_d(q0_out, dst, 0, 0);
    __lsx_vstelm_d(q1_out, dst + stride, 0, 0);
  } else {
    DUP4_ARG2(__lsx_vilvl_b, zero, p3, zero, p2, zero, p1, zero, p0, p3_l, p2_l,
              p1_l, p0_l);
    DUP4_ARG2(__lsx_vilvl_b, zero, q0, zero, q1, zero, q2, zero, q3, q0_l, q1_l,
              q2_l, q3_l);
    VP9_FILTER8(p3_l, p2_l, p1_l, p0_l, q0_l, q1_l, q2_l, q3_l, p2_filter8,
                p1_filter8, p0_filter8, q0_filter8, q1_filter8, q2_filter8);

    /* convert 16 bit output data into 8 bit */
    DUP4_ARG2(__lsx_vpickev_b, zero, p2_filter8, zero, p1_filter8, zero,
              p0_filter8, zero, q0_filter8, p2_filter8, p1_filter8, p0_filter8,
              q0_filter8);
    DUP2_ARG2(__lsx_vpickev_b, zero, q1_filter8, zero, q2_filter8, q1_filter8,
              q2_filter8);
    DUP4_ARG3(__lsx_vbitsel_v, p2, p2_filter8, flat, p1_out, p1_filter8, flat,
              p0_out, p0_filter8, flat, q0_out, q0_filter8, flat, p2_out,
              p1_out, p0_out, q0_out);
    DUP2_ARG3(__lsx_vbitsel_v, q1_out, q1_filter8, flat, q2, q2_filter8, flat,
              q1_out, q2_out);
    dst -= stride3;

    __lsx_vstelm_d(p2_out, dst, 0, 0);
    __lsx_vstelm_d(p1_out, dst + stride, 0, 0);
    __lsx_vstelm_d(p0_out, dst + stride2, 0, 0);
    __lsx_vstelm_d(q0_out, dst + stride3, 0, 0);

    dst += stride4;
    __lsx_vstelm_d(q1_out, dst, 0, 0);
    dst += stride;
    __lsx_vstelm_d(q2_out, dst, 0, 0);
  }
}

void vpx_lpf_horizontal_8_dual_lsx(
    uint8_t *dst, int32_t stride, const uint8_t *b_limit0,
    const uint8_t *limit0, const uint8_t *thresh0, const uint8_t *b_limit1,
    const uint8_t *limit1, const uint8_t *thresh1) {
  __m128i p3, p2, p1, p0, q3, q2, q1, q0;
  __m128i p2_out, p1_out, p0_out, q0_out, q1_out, q2_out;
  __m128i flat, mask, hev, tmp, thresh, b_limit, limit;
  __m128i p3_l, p2_l, p1_l, p0_l, q0_l, q1_l, q2_l, q3_l;
  __m128i p3_h, p2_h, p1_h, p0_h, q0_h, q1_h, q2_h, q3_h;
  __m128i p2_filt8_l, p1_filt8_l, p0_filt8_l;
  __m128i q0_filt8_l, q1_filt8_l, q2_filt8_l;
  __m128i p2_filt8_h, p1_filt8_h, p0_filt8_h;
  __m128i q0_filt8_h, q1_filt8_h, q2_filt8_h;
  __m128i zero = __lsx_vldi(0);

  int32_t stride2 = stride << 1;
  int32_t stride3 = stride2 + stride;
  int32_t stride4 = stride2 << 1;

  DUP4_ARG2(__lsx_vldx, dst, -stride4, dst, -stride3, dst, -stride2, dst,
            -stride, p3, p2, p1, p0);
  q0 = __lsx_vld(dst, 0);
  DUP2_ARG2(__lsx_vldx, dst, stride, dst, stride2, q1, q2);
  q3 = __lsx_vldx(dst, stride3);

  thresh = __lsx_vreplgr2vr_b(*thresh0);
  tmp = __lsx_vreplgr2vr_b(*thresh1);
  thresh = __lsx_vilvl_d(tmp, thresh);

  b_limit = __lsx_vreplgr2vr_b(*b_limit0);
  tmp = __lsx_vreplgr2vr_b(*b_limit1);
  b_limit = __lsx_vilvl_d(tmp, b_limit);

  limit = __lsx_vreplgr2vr_b(*limit0);
  tmp = __lsx_vreplgr2vr_b(*limit1);
  limit = __lsx_vilvl_d(tmp, limit);

  /* mask and hev */
  LPF_MASK_HEV(p3, p2, p1, p0, q0, q1, q2, q3, limit, b_limit, thresh, hev,
               mask, flat);
  VP9_FLAT4(p3, p2, p0, q0, q2, q3, flat);
  VP9_LPF_FILTER4_4W(p1, p0, q0, q1, mask, hev, p1_out, p0_out, q0_out, q1_out);

  if (__lsx_bz_v(flat)) {
    __lsx_vst(p1_out, dst - stride2, 0);
    __lsx_vst(p0_out, dst - stride, 0);
    __lsx_vst(q0_out, dst, 0);
    __lsx_vst(q1_out, dst + stride, 0);
  } else {
    DUP4_ARG2(__lsx_vilvl_b, zero, p3, zero, p2, zero, p1, zero, p0, p3_l, p2_l,
              p1_l, p0_l);
    DUP4_ARG2(__lsx_vilvl_b, zero, q0, zero, q1, zero, q2, zero, q3, q0_l, q1_l,
              q2_l, q3_l);
    VP9_FILTER8(p3_l, p2_l, p1_l, p0_l, q0_l, q1_l, q2_l, q3_l, p2_filt8_l,
                p1_filt8_l, p0_filt8_l, q0_filt8_l, q1_filt8_l, q2_filt8_l);

    DUP4_ARG2(__lsx_vilvh_b, zero, p3, zero, p2, zero, p1, zero, p0, p3_h, p2_h,
              p1_h, p0_h);
    DUP4_ARG2(__lsx_vilvh_b, zero, q0, zero, q1, zero, q2, zero, q3, q0_h, q1_h,
              q2_h, q3_h);
    VP9_FILTER8(p3_h, p2_h, p1_h, p0_h, q0_h, q1_h, q2_h, q3_h, p2_filt8_h,
                p1_filt8_h, p0_filt8_h, q0_filt8_h, q1_filt8_h, q2_filt8_h);

    /* convert 16 bit output data into 8 bit */
    DUP4_ARG2(__lsx_vpickev_b, p2_filt8_h, p2_filt8_l, p1_filt8_h, p1_filt8_l,
              p0_filt8_h, p0_filt8_l, q0_filt8_h, q0_filt8_l, p2_filt8_l,
              p1_filt8_l, p0_filt8_l, q0_filt8_l);
    DUP2_ARG2(__lsx_vpickev_b, q1_filt8_h, q1_filt8_l, q2_filt8_h, q2_filt8_l,
              q1_filt8_l, q2_filt8_l);

    /* store pixel values */
    p2_out = __lsx_vbitsel_v(p2, p2_filt8_l, flat);
    p1_out = __lsx_vbitsel_v(p1_out, p1_filt8_l, flat);
    p0_out = __lsx_vbitsel_v(p0_out, p0_filt8_l, flat);
    q0_out = __lsx_vbitsel_v(q0_out, q0_filt8_l, flat);
    q1_out = __lsx_vbitsel_v(q1_out, q1_filt8_l, flat);
    q2_out = __lsx_vbitsel_v(q2, q2_filt8_l, flat);

    __lsx_vst(p2_out, dst - stride3, 0);
    __lsx_vst(p1_out, dst - stride2, 0);
    __lsx_vst(p0_out, dst - stride, 0);
    __lsx_vst(q0_out, dst, 0);
    __lsx_vst(q1_out, dst + stride, 0);
    __lsx_vst(q2_out, dst + stride2, 0);
  }
}

void vpx_lpf_vertical_8_lsx(uint8_t *dst, int32_t stride,
                            const uint8_t *b_limit_ptr,
                            const uint8_t *limit_ptr,
                            const uint8_t *thresh_ptr) {
  __m128i p3, p2, p1, p0, q3, q2, q1, q0;
  __m128i p1_out, p0_out, q0_out, q1_out;
  __m128i flat, mask, hev, thresh, b_limit, limit;
  __m128i p3_l, p2_l, p1_l, p0_l, q0_l, q1_l, q2_l, q3_l;
  __m128i p2_filt8_l, p1_filt8_l, p0_filt8_l;
  __m128i q0_filt8_l, q1_filt8_l, q2_filt8_l;
  __m128i vec0, vec1, vec2, vec3, vec4;
  __m128i zero = __lsx_vldi(0);

  int32_t stride2 = stride << 1;
  int32_t stride3 = stride2 + stride;
  int32_t stride4 = stride2 << 1;
  uint8_t *dst_tmp = dst - 4;

  /* load vector elements */
  p3 = __lsx_vld(dst_tmp, 0);
  DUP2_ARG2(__lsx_vldx, dst_tmp, stride, dst_tmp, stride2, p2, p1);
  p0 = __lsx_vldx(dst_tmp, stride3);
  dst_tmp += stride4;
  q0 = __lsx_vld(dst_tmp, 0);
  DUP2_ARG2(__lsx_vldx, dst_tmp, stride, dst_tmp, stride2, q1, q2);
  q3 = __lsx_vldx(dst_tmp, stride3);

  LSX_TRANSPOSE8x8_B(p3, p2, p1, p0, q0, q1, q2, q3, p3, p2, p1, p0, q0, q1, q2,
                     q3);

  thresh = __lsx_vreplgr2vr_b(*thresh_ptr);
  b_limit = __lsx_vreplgr2vr_b(*b_limit_ptr);
  limit = __lsx_vreplgr2vr_b(*limit_ptr);

  /* mask and hev */
  LPF_MASK_HEV(p3, p2, p1, p0, q0, q1, q2, q3, limit, b_limit, thresh, hev,
               mask, flat);
  /* flat4 */
  VP9_FLAT4(p3, p2, p0, q0, q2, q3, flat);
  /* filter4 */
  VP9_LPF_FILTER4_4W(p1, p0, q0, q1, mask, hev, p1_out, p0_out, q0_out, q1_out);

  flat = __lsx_vilvl_d(zero, flat);

  /* if flat is zero for all pixels, then no need to calculate other filter */
  if (__lsx_bz_v(flat)) {
    /* Store 4 pixels p1-_q1 */
    DUP2_ARG2(__lsx_vilvl_b, p0_out, p1_out, q1_out, q0_out, vec0, vec1);
    vec2 = __lsx_vilvl_h(vec1, vec0);
    vec3 = __lsx_vilvh_h(vec1, vec0);

    dst -= 2;
    __lsx_vstelm_w(vec2, dst, 0, 0);
    __lsx_vstelm_w(vec2, dst + stride, 0, 1);
    __lsx_vstelm_w(vec2, dst + stride2, 0, 2);
    __lsx_vstelm_w(vec2, dst + stride3, 0, 3);
    dst += stride4;
    __lsx_vstelm_w(vec3, dst, 0, 0);
    __lsx_vstelm_w(vec3, dst + stride, 0, 1);
    __lsx_vstelm_w(vec3, dst + stride2, 0, 2);
    __lsx_vstelm_w(vec3, dst + stride3, 0, 3);
  } else {
    DUP4_ARG2(__lsx_vilvl_b, zero, p3, zero, p2, zero, p1, zero, p0, p3_l, p2_l,
              p1_l, p0_l);
    DUP4_ARG2(__lsx_vilvl_b, zero, q0, zero, q1, zero, q2, zero, q3, q0_l, q1_l,
              q2_l, q3_l);
    VP9_FILTER8(p3_l, p2_l, p1_l, p0_l, q0_l, q1_l, q2_l, q3_l, p2_filt8_l,
                p1_filt8_l, p0_filt8_l, q0_filt8_l, q1_filt8_l, q2_filt8_l);
    /* convert 16 bit output data into 8 bit */
    DUP4_ARG2(__lsx_vpickev_b, p2_filt8_l, p2_filt8_l, p1_filt8_l, p1_filt8_l,
              p0_filt8_l, p0_filt8_l, q0_filt8_l, q0_filt8_l, p2_filt8_l,
              p1_filt8_l, p0_filt8_l, q0_filt8_l);
    DUP2_ARG2(__lsx_vpickev_b, q1_filt8_l, q1_filt8_l, q2_filt8_l, q2_filt8_l,
              q1_filt8_l, q2_filt8_l);
    /* store pixel values */
    p2 = __lsx_vbitsel_v(p2, p2_filt8_l, flat);
    p1 = __lsx_vbitsel_v(p1_out, p1_filt8_l, flat);
    p0 = __lsx_vbitsel_v(p0_out, p0_filt8_l, flat);
    q0 = __lsx_vbitsel_v(q0_out, q0_filt8_l, flat);
    q1 = __lsx_vbitsel_v(q1_out, q1_filt8_l, flat);
    q2 = __lsx_vbitsel_v(q2, q2_filt8_l, flat);

    /* Store 6 pixels p2-_q2 */
    DUP2_ARG2(__lsx_vilvl_b, p1, p2, q0, p0, vec0, vec1);
    vec2 = __lsx_vilvl_h(vec1, vec0);
    vec3 = __lsx_vilvh_h(vec1, vec0);
    vec4 = __lsx_vilvl_b(q2, q1);

    dst -= 3;
    __lsx_vstelm_w(vec2, dst, 0, 0);
    __lsx_vstelm_h(vec4, dst, 4, 0);
    dst += stride;
    __lsx_vstelm_w(vec2, dst, 0, 1);
    __lsx_vstelm_h(vec4, dst, 4, 1);
    dst += stride;
    __lsx_vstelm_w(vec2, dst, 0, 2);
    __lsx_vstelm_h(vec4, dst, 4, 2);
    dst += stride;
    __lsx_vstelm_w(vec2, dst, 0, 3);
    __lsx_vstelm_h(vec4, dst, 4, 3);
    dst += stride;
    __lsx_vstelm_w(vec3, dst, 0, 0);
    __lsx_vstelm_h(vec4, dst, 4, 4);
    dst += stride;
    __lsx_vstelm_w(vec3, dst, 0, 1);
    __lsx_vstelm_h(vec4, dst, 4, 5);
    dst += stride;
    __lsx_vstelm_w(vec3, dst, 0, 2);
    __lsx_vstelm_h(vec4, dst, 4, 6);
    dst += stride;
    __lsx_vstelm_w(vec3, dst, 0, 3);
    __lsx_vstelm_h(vec4, dst, 4, 7);
  }
}

void vpx_lpf_vertical_8_dual_lsx(uint8_t *dst, int32_t stride,
                                 const uint8_t *b_limit0, const uint8_t *limit0,
                                 const uint8_t *thresh0,
                                 const uint8_t *b_limit1, const uint8_t *limit1,
                                 const uint8_t *thresh1) {
  uint8_t *dst_tmp = dst - 4;
  __m128i p3, p2, p1, p0, q3, q2, q1, q0;
  __m128i p1_out, p0_out, q0_out, q1_out;
  __m128i flat, mask, hev, thresh, b_limit, limit;
  __m128i row4, row5, row6, row7, row12, row13, row14, row15;
  __m128i p3_l, p2_l, p1_l, p0_l, q0_l, q1_l, q2_l, q3_l;
  __m128i p3_h, p2_h, p1_h, p0_h, q0_h, q1_h, q2_h, q3_h;
  __m128i p2_filt8_l, p1_filt8_l, p0_filt8_l;
  __m128i q0_filt8_l, q1_filt8_l, q2_filt8_l;
  __m128i p2_filt8_h, p1_filt8_h, p0_filt8_h;
  __m128i q0_filt8_h, q1_filt8_h, q2_filt8_h;
  __m128i vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
  __m128i zero = __lsx_vldi(0);
  int32_t stride2 = stride << 1;
  int32_t stride3 = stride2 + stride;
  int32_t stride4 = stride2 << 1;

  p0 = __lsx_vld(dst_tmp, 0);
  DUP2_ARG2(__lsx_vldx, dst_tmp, stride, dst_tmp, stride2, p1, p2);
  p3 = __lsx_vldx(dst_tmp, stride3);
  dst_tmp += stride4;
  row4 = __lsx_vld(dst_tmp, 0);
  DUP2_ARG2(__lsx_vldx, dst_tmp, stride, dst_tmp, stride2, row5, row6);
  row7 = __lsx_vldx(dst_tmp, stride3);
  dst_tmp += stride4;

  q3 = __lsx_vld(dst_tmp, 0);
  DUP2_ARG2(__lsx_vldx, dst_tmp, stride, dst_tmp, stride2, q2, q1);
  q0 = __lsx_vldx(dst_tmp, stride3);
  dst_tmp += stride4;
  row12 = __lsx_vld(dst_tmp, 0);
  DUP2_ARG2(__lsx_vldx, dst_tmp, stride, dst_tmp, stride2, row13, row14);
  row15 = __lsx_vldx(dst_tmp, stride3);

  /* transpose 16x8 matrix into 8x16 */
  LSX_TRANSPOSE16x8_B(p0, p1, p2, p3, row4, row5, row6, row7, q3, q2, q1, q0,
                      row12, row13, row14, row15, p3, p2, p1, p0, q0, q1, q2,
                      q3);

  thresh = __lsx_vreplgr2vr_b(*thresh0);
  vec0 = __lsx_vreplgr2vr_b(*thresh1);
  thresh = __lsx_vilvl_d(vec0, thresh);

  b_limit = __lsx_vreplgr2vr_b(*b_limit0);
  vec0 = __lsx_vreplgr2vr_b(*b_limit1);
  b_limit = __lsx_vilvl_d(vec0, b_limit);

  limit = __lsx_vreplgr2vr_b(*limit0);
  vec0 = __lsx_vreplgr2vr_b(*limit1);
  limit = __lsx_vilvl_d(vec0, limit);

  /* mask and hev */
  LPF_MASK_HEV(p3, p2, p1, p0, q0, q1, q2, q3, limit, b_limit, thresh, hev,
               mask, flat);
  /* flat4 */
  VP9_FLAT4(p3, p2, p0, q0, q2, q3, flat);
  /* filter4 */
  VP9_LPF_FILTER4_4W(p1, p0, q0, q1, mask, hev, p1_out, p0_out, q0_out, q1_out);
  /* if flat is zero for all pixels, then no need to calculate other filter */
  if (__lsx_bz_v(flat)) {
    DUP2_ARG2(__lsx_vilvl_b, p0_out, p1_out, q1_out, q0_out, vec0, vec1);
    vec2 = __lsx_vilvl_h(vec1, vec0);
    vec3 = __lsx_vilvh_h(vec1, vec0);
    DUP2_ARG2(__lsx_vilvh_b, p0_out, p1_out, q1_out, q0_out, vec0, vec1);
    vec4 = __lsx_vilvl_h(vec1, vec0);
    vec5 = __lsx_vilvh_h(vec1, vec0);

    dst -= 2;
    __lsx_vstelm_w(vec2, dst, 0, 0);
    __lsx_vstelm_w(vec2, dst + stride, 0, 1);
    __lsx_vstelm_w(vec2, dst + stride2, 0, 2);
    __lsx_vstelm_w(vec2, dst + stride3, 0, 3);
    dst += stride4;
    __lsx_vstelm_w(vec3, dst, 0, 0);
    __lsx_vstelm_w(vec3, dst + stride, 0, 1);
    __lsx_vstelm_w(vec3, dst + stride2, 0, 2);
    __lsx_vstelm_w(vec3, dst + stride3, 0, 3);
    dst += stride4;
    __lsx_vstelm_w(vec4, dst, 0, 0);
    __lsx_vstelm_w(vec4, dst + stride, 0, 1);
    __lsx_vstelm_w(vec4, dst + stride2, 0, 2);
    __lsx_vstelm_w(vec4, dst + stride3, 0, 3);
    dst += stride4;
    __lsx_vstelm_w(vec5, dst, 0, 0);
    __lsx_vstelm_w(vec5, dst + stride, 0, 1);
    __lsx_vstelm_w(vec5, dst + stride2, 0, 2);
    __lsx_vstelm_w(vec5, dst + stride3, 0, 3);
  } else {
    DUP4_ARG2(__lsx_vilvl_b, zero, p3, zero, p2, zero, p1, zero, p0, p3_l, p2_l,
              p1_l, p0_l);
    DUP4_ARG2(__lsx_vilvl_b, zero, q0, zero, q1, zero, q2, zero, q3, q0_l, q1_l,
              q2_l, q3_l);
    VP9_FILTER8(p3_l, p2_l, p1_l, p0_l, q0_l, q1_l, q2_l, q3_l, p2_filt8_l,
                p1_filt8_l, p0_filt8_l, q0_filt8_l, q1_filt8_l, q2_filt8_l);
    DUP4_ARG2(__lsx_vilvh_b, zero, p3, zero, p2, zero, p1, zero, p0, p3_h, p2_h,
              p1_h, p0_h);
    DUP4_ARG2(__lsx_vilvh_b, zero, q0, zero, q1, zero, q2, zero, q3, q0_h, q1_h,
              q2_h, q3_h);

    /* filter8 */
    VP9_FILTER8(p3_h, p2_h, p1_h, p0_h, q0_h, q1_h, q2_h, q3_h, p2_filt8_h,
                p1_filt8_h, p0_filt8_h, q0_filt8_h, q1_filt8_h, q2_filt8_h);

    /* convert 16 bit output data into 8 bit */
    DUP4_ARG2(__lsx_vpickev_b, p2_filt8_h, p2_filt8_l, p1_filt8_h, p1_filt8_l,
              p0_filt8_h, p0_filt8_l, q0_filt8_h, q0_filt8_l, p2_filt8_l,
              p1_filt8_l, p0_filt8_l, q0_filt8_l);
    DUP2_ARG2(__lsx_vpickev_b, q1_filt8_h, q1_filt8_l, q2_filt8_h, q2_filt8_l,
              q1_filt8_l, q2_filt8_l);

    /* store pixel values */
    p2 = __lsx_vbitsel_v(p2, p2_filt8_l, flat);
    p1 = __lsx_vbitsel_v(p1_out, p1_filt8_l, flat);
    p0 = __lsx_vbitsel_v(p0_out, p0_filt8_l, flat);
    q0 = __lsx_vbitsel_v(q0_out, q0_filt8_l, flat);
    q1 = __lsx_vbitsel_v(q1_out, q1_filt8_l, flat);
    q2 = __lsx_vbitsel_v(q2, q2_filt8_l, flat);

    DUP2_ARG2(__lsx_vilvl_b, p1, p2, q0, p0, vec0, vec1);
    vec3 = __lsx_vilvl_h(vec1, vec0);
    vec4 = __lsx_vilvh_h(vec1, vec0);
    DUP2_ARG2(__lsx_vilvh_b, p1, p2, q0, p0, vec0, vec1);
    vec6 = __lsx_vilvl_h(vec1, vec0);
    vec7 = __lsx_vilvh_h(vec1, vec0);
    vec2 = __lsx_vilvl_b(q2, q1);
    vec5 = __lsx_vilvh_b(q2, q1);

    dst -= 3;
    __lsx_vstelm_w(vec3, dst, 0, 0);
    __lsx_vstelm_h(vec2, dst, 4, 0);
    dst += stride;
    __lsx_vstelm_w(vec3, dst, 0, 1);
    __lsx_vstelm_h(vec2, dst, 4, 1);
    dst += stride;
    __lsx_vstelm_w(vec3, dst, 0, 2);
    __lsx_vstelm_h(vec2, dst, 4, 2);
    dst += stride;
    __lsx_vstelm_w(vec3, dst, 0, 3);
    __lsx_vstelm_h(vec2, dst, 4, 3);
    dst += stride;
    __lsx_vstelm_w(vec4, dst, 0, 0);
    __lsx_vstelm_h(vec2, dst, 4, 4);
    dst += stride;
    __lsx_vstelm_w(vec4, dst, 0, 1);
    __lsx_vstelm_h(vec2, dst, 4, 5);
    dst += stride;
    __lsx_vstelm_w(vec4, dst, 0, 2);
    __lsx_vstelm_h(vec2, dst, 4, 6);
    dst += stride;
    __lsx_vstelm_w(vec4, dst, 0, 3);
    __lsx_vstelm_h(vec2, dst, 4, 7);
    dst += stride;
    __lsx_vstelm_w(vec6, dst, 0, 0);
    __lsx_vstelm_h(vec5, dst, 4, 0);
    dst += stride;
    __lsx_vstelm_w(vec6, dst, 0, 1);
    __lsx_vstelm_h(vec5, dst, 4, 1);
    dst += stride;
    __lsx_vstelm_w(vec6, dst, 0, 2);
    __lsx_vstelm_h(vec5, dst, 4, 2);
    dst += stride;
    __lsx_vstelm_w(vec6, dst, 0, 3);
    __lsx_vstelm_h(vec5, dst, 4, 3);
    dst += stride;
    __lsx_vstelm_w(vec7, dst, 0, 0);
    __lsx_vstelm_h(vec5, dst, 4, 4);
    dst += stride;
    __lsx_vstelm_w(vec7, dst, 0, 1);
    __lsx_vstelm_h(vec5, dst, 4, 5);
    dst += stride;
    __lsx_vstelm_w(vec7, dst, 0, 2);
    __lsx_vstelm_h(vec5, dst, 4, 6);
    dst += stride;
    __lsx_vstelm_w(vec7, dst, 0, 3);
    __lsx_vstelm_h(vec5, dst, 4, 7);
  }
}
