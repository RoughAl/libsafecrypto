/*****************************************************************************
 * Copyright (C) Queen's University Belfast, ECIT, 2016                      *
 *                                                                           *
 * This file is part of libsafecrypto.                                       *
 *                                                                           *
 * This file is subject to the terms and conditions defined in the file      *
 * 'LICENSE', which is part of this source code package.                     *
 *****************************************************************************/

/*
 * Git commit information:
 *   Author: $SC_AUTHOR$
 *   Date:   $SC_DATE$
 *   Branch: $SC_BRANCH$
 *   Id:     $SC_IDENT$
 */

#pragma once

#include "safecrypto_types.h"
#include <stdio.h>

#include <limits.h>
#ifndef NATIVE_WORD_SIZE
#define NATIVE_WORD_SIZE   __WORDSIZE
#endif

/// Macro definitions for mathematical constants that are not defined by C99
/// @{
#define SC_M_2_SQRTPIl  1.128379167095512573896158903121545172L
#define SC_M_SQRT1_2l   0.707106781186547524400844362104849039L
/// @}


/// Arithmetic comparison functions (constant-time)
/// @{
/// Return 1 if less than, 0 otherwise
#ifdef HAVE_64BIT
volatile SINT32 sc_const_time_u64_lessthan(volatile UINT64 a, volatile UINT64 b);
#endif
volatile SINT32 sc_const_time_u32_lessthan(volatile UINT32 a, volatile UINT32 b);
/// @}



/// Modulo range limiting functions - the return value is -q <= x <= q, |x| must be < 2*x
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
SINT128 sc_mod_limit_s128(SINT128 x, SINT128 q);
#endif
#ifdef HAVE_64BIT
SINT64 sc_mod_limit_s64(SINT64 x, SINT64 q);
#endif
SINT32 sc_mod_limit_s32(SINT32 x, SINT32 q);
SINT16 sc_mod_limit_s16(SINT16 x, SINT16 q);
SINT8 sc_mod_limit_s8(SINT8 x, SINT8 q);
/// @}

/// Fast math estimation routines
/// @{
DOUBLE sc_exp_dbl_coarse(DOUBLE y);
FLOAT sc_exp_flt_coarse(FLOAT y);

DOUBLE sc_pow_estimate_dbl(DOUBLE a, DOUBLE b);
FLOAT sc_pow_estimate_flt(FLOAT a, FLOAT b);

DOUBLE sc_exp_dbl_taylor(DOUBLE y);
FLOAT sc_exp_flt_taylor(FLOAT y);
/// @}

/// Log base 2
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
UINT32 sc_log2_128(UINT128 x);
#endif
#ifdef HAVE_64BIT
UINT32 sc_log2_64(UINT64 x);
#endif
UINT32 sc_log2_32(UINT32 x);
UINT32 sc_log2_16(UINT16 x);
UINT32 sc_log2_8(UINT8 x);
size_t sc_log2(size_t x);
/// @}

/// Ceiling of log base 2
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
UINT32 sc_ceil_log2_128(UINT128 x);
#endif
#ifdef HAVE_64BIT
UINT32 sc_ceil_log2_64(UINT64 x);
#endif
UINT32 sc_ceil_log2_32(UINT32 x);
UINT32 sc_ceil_log2_16(UINT16 x);
UINT32 sc_ceil_log2_8(UINT8 x);
size_t sc_ceil_log2(size_t x);
/// @}

/// Parity computation
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
UINT32 sc_bit_parity_128(UINT128 x);
#endif
#ifdef HAVE_64BIT
UINT32 sc_bit_parity_64(UINT64 x);
#endif
UINT32 sc_bit_parity_32(UINT32 x);
UINT32 sc_bit_parity_16(UINT16 x);
UINT32 sc_bit_parity_8(UINT8 x);
/// @}

/// Hamming weight computation
/// @{
#ifdef HAVE_64BIT
UINT64 sc_hamming_64(UINT64 x);
#endif
UINT32 sc_hamming_32(UINT32 x);
UINT16 sc_hamming_16(UINT16 x);
UINT8 sc_hamming_8(UINT8 x);
/// @}

/// Count trailing zeros
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
UINT32 sc_ctz_128(UINT128 x);
#endif
#ifdef HAVE_64BIT
UINT32 sc_ctz_64(UINT64 x);
#endif
UINT32 sc_ctz_32(UINT32 x);
UINT32 sc_ctz_16(UINT16 x);
UINT32 sc_ctz_8(UINT8 x);
/// @}

/// Count leading zeros
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
UINT32 sc_clz_128(UINT128 x);
#endif
#ifdef HAVE_64BIT
UINT32 sc_clz_64(UINT64 x);
#endif
UINT32 sc_clz_32(UINT32 x);
UINT32 sc_clz_16(UINT16 x);
UINT32 sc_clz_8(UINT8 x);
/// @}

/// Bit reversal
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
UINT128 sc_bit_reverse_128(UINT128 x);
#endif
#ifdef HAVE_64BIT
UINT64 sc_bit_reverse_64(UINT64 x);
#endif
UINT32 sc_bit_reverse_32(UINT32 x);
UINT16 sc_bit_reverse_16(UINT16 x);
UINT8 sc_bit_reverse_8(UINT8 x);
size_t sc_bit_reverse(size_t x);
/// @}

/// Rotation of types
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
UINT128 sc_rotl_128(UINT128 w, SINT32 n);
#endif
#ifdef HAVE_64BIT
UINT64 sc_rotl_64(UINT64 w, SINT32 n);
#endif
UINT32 sc_rotl_32(UINT32 w, SINT32 n);
UINT16 sc_rotl_16(UINT16 w, SINT32 n);
UINT8 sc_rotl_8(UINT8 w, SINT32 n);
/// @}

/// Rotation of arrays
/// @{
SINT32 sc_arr_rotl_32(UINT32 *w, size_t n, SINT32 m);
/// @}

/// Unsigned integer square root
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
UINT128 sqrt_u128(UINT128 x);
#endif
#ifdef HAVE_64BIT
UINT64 sqrt_u64(UINT64 x);
#endif
UINT32 sqrt_u32(UINT32 x);
UINT16 sqrt_u16(UINT16 x);
UINT8 sqrt_u8(UINT8 x);
/// @}

/// Binary fraction expansion
/// @{
#if defined(HAVE_128BIT) && defined(__x86_64__)
UINT128 get_binary_expansion_fraction_128(DOUBLE x);
#endif
#ifdef HAVE_64BIT
UINT64 get_binary_expansion_fraction_64(DOUBLE x);
#endif
UINT32 get_binary_expansion_fraction_32(DOUBLE x);
/// @}

/// Macros for common and simple mathemtical operations
/// @{
#define SC_MIN(x,y)   (((x) < (y))? (x) : (y))
#define SC_MAX(x,y)   (((x) >= (y))? (x) : (y))
#define SC_ABS(x)     (((x) < 0)? -(x) : (x))
/// @}

