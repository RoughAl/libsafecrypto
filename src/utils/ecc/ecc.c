/*****************************************************************************
 * Copyright (C) Queen's University Belfast, ECIT, 2017                      *
 *                                                                           *
 * This file is part of tachyon.                                             *
 *                                                                           *
 * This file is subject to the terms and conditions defined in the file      *
 * 'LICENSE', which is part of this source code package.                     *
 *****************************************************************************/

#include "utils/ecc/ecc.h"
#include "utils/ecc/secret_bits.h"
#include "utils/arith/sc_mpz.h"
#include "utils/crypto/prng.h"
#include "safecrypto_debug.h"
#include "safecrypto_error.h"


typedef struct ecc_metadata {
	sc_mpz_t lambda;
	sc_mpz_t temp;
	sc_mpz_t x;
	sc_mpz_t y;
	sc_mpz_t z;
	sc_mpz_t h;
	sc_mpz_t w;
	sc_mpz_t m;
	sc_mpz_t m_inv;
	sc_mpz_t order_m;
	sc_mpz_t a;
	size_t   k;
} ecc_metadata_t;

const ec_set_t param_ec_secp192r1 = {
	192,
	24,
	192 >> SC_LIMB_BITS_SHIFT,
	"-3",//"FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC",
	"64210519E59C80E70FA7E9AB72243049FEB8DEECC146B9B1",
	"188DA80EB03090F67CBF20EB43A18800F4FF0AFD82FF1012",
	"07192B95FFC8DA78631011ED6B24CDD573F977A11E794811",
	"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFF",
	"1000000000000000000000000000000010000000000000001",
	"FFFFFFFFFFFFFFFFFFFFFFFF99DEF836146BC9B1B4D22831",
};

const ec_set_t param_ec_secp224r1 = {
	224,
	28,
	(224 + SC_LIMB_BITS - 1) >> SC_LIMB_BITS_SHIFT,
	"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFE",
	"B4050A850C04B3ABF54132565044B0B7D7BFD8BA270B39432355FFB4",
	"B70E0CBD6BB4BF7F321390B94A03C1D356C21122343280D6115C1D21",
	"BD376388B5F723FB4C22DFE6CD4375A05A07476444D5819985007E34",
	"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000000001",
	"100000000000000000000000000000000ffffffffffffffffffffffff",
	"FFFFFFFFFFFFFFFFFFFFFFFFFFFF16A2E0B8F03E13DD29455C5C2A3D",
};

const ec_set_t param_ec_secp256r1 = {
	256,
	32,
	256 >> SC_LIMB_BITS_SHIFT,
	"-3",//"FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC",
	"5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b",
	"6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296",
	"4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5",
	"ffffffff00000001000000000000000000000000ffffffffffffffffffffffff",
	"100000000fffffffffffffffefffffffefffffffeffffffff0000000000000003", // from https://defuse.ca/big-number-calculator.htm
	"ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551",
};

const ec_set_t param_ec_secp384r1 = {
	384,
	48,
	384 >> SC_LIMB_BITS_SHIFT,
	"-3", // "ffffffff00000001000000000000000000000000fffffffffffffffffffffffc",
	"b3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef",
	"aa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7",
	"3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f",
	"fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff",
	"1000000000000000000000000000000000000000000000000000000000000000100000000ffffffffffffffff00000001",
	"ffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973",
};

const ec_set_t param_ec_secp521r1 = {
	521,
	66,
	(521 + SC_LIMB_BITS - 1) >> SC_LIMB_BITS_SHIFT,
	"-3", // "01fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffc",
	"51953eb9618e1c9a1f929a21a0b68540eea2da725b99b315f3b8b489918ef109e156193951ec7e937b1652c0bd3bb1bf073573df883d2c34f1ef451fd46b503f00",
	"c6858e06b70404e9cd9e3ecb662395b4429c648139053fb521f828af606b4d3dbaa14b5e77efe75928fe1dc127a2ffa8de3348b3c1856a429bf97e7e31c2e5bd66",
	"11839296a789a3bc0045c8a5fb42c7d1bd998f54449579b446817afbd17273e662c97ee72995ef42640c550b9013fad0761353c7086a272c24088be94769fd16650",
	"1ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
	"8000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000",
	"1fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffa51868783bf2f966b7fcc0148f709a5d03bb5c9b8899c47aebb6fb71e91386409",
};


void point_reset(ecc_point_t *p)
{
	p->type = EC_COORD_AFFINE;
	sc_mpz_set_ui(&p->x, 0);
	sc_mpz_set_ui(&p->y, 0);
	sc_mpz_set_ui(&p->z, 0);
}

void point_init(ecc_point_t *p, size_t n)
{
	p->type = EC_COORD_AFFINE;
	sc_mpz_init2(&p->x, n * SC_LIMB_BITS);
	sc_mpz_init2(&p->y, n * SC_LIMB_BITS);
	sc_mpz_init2(&p->z, n * SC_LIMB_BITS);
	p->n = n;
}

void point_clear(ecc_point_t *p)
{
	sc_mpz_clear(&p->x);
	sc_mpz_clear(&p->y);
	sc_mpz_clear(&p->z);
}

void point_copy(ecc_point_t *p_out, const ecc_point_t *p_in)
{
	sc_mpz_copy(&p_out->x, &p_in->x);
	sc_mpz_copy(&p_out->y, &p_in->y);
	sc_mpz_copy(&p_out->z, &p_in->z);
	p_out->type = p_in->type;
	p_out->n = p_in->n;
}

void point_negate(ecc_point_t *p_inout)
{
	sc_mpz_negate(&p_inout->y, &p_inout->y);
}

SINT32 point_is_zero(const ecc_point_t *p)
{
	return sc_mpz_is_zero(&p->x) && sc_mpz_is_zero(&p->y) && sc_mpz_is_zero(&p->z);
}

void point_affine_to_projective(ecc_point_t *p)
{
	p->type = EC_COORD_PROJECTIVE;
	sc_mpz_set_ui(&p->z, 1);
}

void point_projective_to_affine(ecc_point_t *p, sc_mpz_t *tmul, sc_mpz_t *temp, sc_mpz_t *m)
{
	p->type = EC_COORD_AFFINE;
	sc_mpz_invmod(temp, &p->z, m);
	sc_mpz_mul(tmul, temp, &p->x);
	sc_mpz_mod(&p->x, tmul, m);
	sc_mpz_mul(tmul, temp, &p->y);
	sc_mpz_mod(&p->y, tmul, m);
}

static ecc_retcode_e point_double_projective(ecc_metadata_t *metadata, ecc_point_t *point)
{
	sc_mpz_t *temp, *w, *s, *b, *h, *m, *a;
	temp          = &metadata->temp;
	w             = &metadata->x;
	s             = &metadata->y;
	b             = &metadata->lambda;
	h             = &metadata->h;
	m             = &metadata->m;
	a             = &metadata->a;

    // w = a * z^2 + 3 * x^2
	sc_mpz_sqr(temp, &point->z);
	sc_mpz_mod(w, temp, m);
	sc_mpz_mul(temp, w, a);
	sc_mpz_mod(w, temp, m);
	sc_mpz_sqr(temp, &point->x);
	sc_mpz_mul_ui(s, temp, 3);
	sc_mpz_add(w, w, s);
	sc_mpz_mod(w, w, m);

	// s = y * z
	sc_mpz_mul(temp, &point->y, &point->z);
	sc_mpz_mod(s, temp, m);

	// b = x * y * s
	sc_mpz_mul(temp, &point->x, &point->y);
	sc_mpz_mod(b, temp, m);
	sc_mpz_mul(temp, b, s);
	sc_mpz_mod(b, temp, m);

	// h = w^2 - 8 * b
	sc_mpz_sqr(temp, w);
	sc_mpz_mod(h, temp, m);
	sc_mpz_mul_ui(temp, b, 8);
	sc_mpz_sub(h, h, temp);
	sc_mpz_mod(h, h, m);

	// x = 2 * h * s
	sc_mpz_mul(temp, h, s);
	sc_mpz_mul_ui(temp, temp, 2);
	sc_mpz_mod(&point->x, temp, m);

	// y = w*(4*b - h) - 8*y^2*s^2
	sc_mpz_mul_ui(temp, b, 4);
	sc_mpz_sub(h, temp, h);
	sc_mpz_mul(temp, w, h);
	sc_mpz_mod(w, temp, m);
	sc_mpz_sqr(temp, s);
	sc_mpz_mod(h, temp, m);    // s^2
	sc_mpz_sqr(temp, &point->y);
	sc_mpz_mul_ui(temp, temp, 8);
	sc_mpz_mod(w, temp, m);
	sc_mpz_mul(temp, w, h);
	sc_mpz_mod(&point->y, temp, m);

	// z = 8 * s^3
	sc_mpz_mul(temp, h, s);
	sc_mpz_mul_ui(temp, temp, 8);
	sc_mpz_mod(&point->z, temp, m);

	return EC_GEOMETRY_OK;
}

static ecc_retcode_e point_double_affine(ecc_metadata_t *metadata, ecc_point_t *point)
{
	sc_mpz_t *lambda, *temp, *x, *y, *m, *a, *m_inv, *order_m;
	lambda        = &metadata->lambda;
	temp          = &metadata->temp;
	x             = &metadata->x;
	y             = &metadata->y;
	m             = &metadata->m;
	m_inv         = &metadata->m_inv;
	order_m       = &metadata->order_m;
	a             = &metadata->a;

	// lambda = (3*x^2 + a)/(2*y)
#if 0
	sc_mpz_mul(temp, &point->x, &point->x);
	sc_mpz_mul_ui(lambda, temp, 3);
	sc_mpz_add(temp, lambda, a);
	sc_mpz_mod_barrett(lambda, temp, m, metadata->k, m_inv);
	sc_mpz_add(temp, &point->y, &point->y);
	sc_mpz_invmod(y, temp, m);
	sc_mpz_mul(temp, lambda, y);
	sc_mpz_mod_barrett(lambda, temp, m, metadata->k, m_inv);
#else
	sc_mpz_mul(temp, &point->x, &point->x);
	sc_mpz_mul_ui(lambda, temp, 3);
	sc_mpz_add(temp, lambda, a);
	sc_mpz_mod(lambda, temp, m);
	sc_mpz_add(temp, &point->y, &point->y);
	sc_mpz_invmod(y, temp, m);
	sc_mpz_mul(temp, lambda, y);
	sc_mpz_mod(lambda, temp, m);
#endif

	// xr = lambda^2 - 2*xp
	sc_mpz_mul(temp, lambda, lambda);
	sc_mpz_sub(temp, temp, &point->x);
    sc_mpz_sub(temp, temp, &point->x);
	sc_mpz_mod(x, temp, m);

	// yr = lambda*(xp - xr) - yp
    sc_mpz_sub(y, x, &point->x);
	sc_mpz_mul(temp, lambda, y);
    sc_mpz_add(y, temp, &point->y);
    sc_mpz_negate(y, y);
    sc_mpz_mod(&point->y, y, m);

    // Overwrite the input point X coordinate with it's new value
    sc_mpz_copy(&point->x, x);

    return EC_GEOMETRY_OK;
}

static ecc_retcode_e point_add_projective(ecc_metadata_t *metadata, ecc_point_t *p_a, const ecc_point_t *p_b)
{
	sc_mpz_t *temp, *w, *a, *u1, *u2, *v1, *v2, *m;
	temp   = &metadata->temp;
	w      = &metadata->w;
	a      = &metadata->z;
	u1     = &metadata->x;
	u2     = &metadata->y;
	v1     = &metadata->lambda;
	v2     = &metadata->h;
	m      = &metadata->m;

	sc_mpz_mul(temp, &p_b->y, &p_a->z);
	sc_mpz_mod(u1, temp, m);
	sc_mpz_mul(temp, &p_a->y, &p_b->z);
	sc_mpz_mod(u2, temp, m);
	sc_mpz_mul(temp, &p_b->x, &p_a->z);
	sc_mpz_mod(v1, temp, m);
	sc_mpz_mul(temp, &p_a->x, &p_b->z);
	sc_mpz_mod(v2, temp, m);

	if (0 == sc_mpz_cmp(v1, v2)) {
		if (0 != sc_mpz_cmp(u1, u2)) {
			return EC_GEOMETRY_INFINITY;
		}
		else {
			return EC_GEOMETRY_DOUBLE;
		}
	}

	sc_mpz_sub(u1, u1, u2); // u1 no longer needed
	sc_mpz_mod(u1, u1, m);
	sc_mpz_sub(v1, v1, v2); // v1 no longer needed
	sc_mpz_mod(v1, v1, m);
	
	sc_mpz_sqr(temp, v1);
	sc_mpz_mod(w, temp, m);  // v^2
	sc_mpz_mul(temp, v2, w);
	sc_mpz_mod(v2, temp, m);  // v^2 * v2
	sc_mpz_mul(temp, w, v1);
	sc_mpz_mod(a, temp, m);  // v^3
	sc_mpz_mul(temp, &p_a->z, &p_b->z);
	sc_mpz_mod(w, temp, m);  // w
	sc_mpz_mul(temp, w, a);
	sc_mpz_mod(&p_a->z, temp, m);  // z = w * v^3
	sc_mpz_mul(temp, u2, a);
	sc_mpz_mod(&p_a->y, temp, m);  // y = v^3 * u2, u2 no longer needed
	sc_mpz_mul_ui(temp, v2, 2);
	sc_mpz_add(temp, temp, a); // v^3 + 2 * v^2 * v2
	sc_mpz_sqr(temp, u1);
	sc_mpz_mod(u2, temp, m);  // u^2
	sc_mpz_mul(temp, u1, w);
	sc_mpz_mod(a, temp, m);  // w * u^2
	sc_mpz_sub(temp, a, u2);
	sc_mpz_mod(a, temp, m);  // w * u^2 - v^3 - 2 * v^2 * v2
	sc_mpz_mul(temp, v1, a);
	sc_mpz_mod(&p_a->x, temp, m);  // v * a
	sc_mpz_sub(a, v2, a);
	sc_mpz_mul(temp, u1, a);
	sc_mpz_mod(u1, temp, m);  // u * (v^2 * v2 - a)
	sc_mpz_sub(a, u1, &p_a->y);
	sc_mpz_mod(&p_a->y, a, m);

	return EC_GEOMETRY_OK;
}

static ecc_retcode_e point_add_affine(ecc_metadata_t *metadata, ecc_point_t *p_a, const ecc_point_t *p_b)
{
	sc_mpz_t *lambda, *temp, *x, *y, *m;
	lambda = &metadata->lambda;
	temp   = &metadata->temp;
	x      = &metadata->x;
	y      = &metadata->y;
	m      = &metadata->m;

	// lambda = (yb - ya) / (xb - xa)
	sc_mpz_sub(y, &p_b->y, &p_a->y);
	sc_mpz_mod(y, y, m);
	sc_mpz_sub(x, &p_b->x, &p_a->x);
	sc_mpz_mod(x, x, m);
	sc_mpz_invmod(lambda, x, m);
	sc_mpz_mul(temp, lambda, y);
	sc_mpz_mod(lambda, temp, m);

	// xr = lambda^2 - xp - xq
	sc_mpz_mul(temp, lambda, lambda);
    sc_mpz_sub(x, temp, &p_a->x);
    sc_mpz_sub(x, x, &p_b->x);
    sc_mpz_mod(&p_a->x, x, m);

	// yr = lambda*(xp - xq) - a
    sc_mpz_sub(y, &p_a->x, &p_b->x);
	sc_mpz_mul(temp, lambda, y);
	sc_mpz_mod(y, temp, m);
    sc_mpz_add(y, y, &p_b->y);
    sc_mpz_negate(y, y);
    sc_mpz_mod(&p_a->y, y, m);

    return EC_GEOMETRY_OK;
}

static ecc_retcode_e point_double(ecc_metadata_t *metadata, ecc_point_t *point)
{
	// If x and y are zero the result is zero
	if (point_is_zero(point)) {
		return EC_GEOMETRY_ZERO;
	}

	switch (point->type) {
		case EC_COORD_AFFINE:     return point_double_affine(metadata, point);
		case EC_COORD_PROJECTIVE: return point_double_projective(metadata, point);
	}
}

static ecc_retcode_e point_add(ecc_metadata_t *metadata, ecc_point_t *p_a, const ecc_point_t *p_b)
{
	ecc_retcode_e retcode;

	switch (p_a->type) {
		case EC_COORD_AFFINE:
		{
			retcode = point_add_affine(metadata, p_a, p_b);
			if (EC_GEOMETRY_DOUBLE == retcode) {
				retcode = point_double_affine(metadata, p_a);
			}
		} break;
		case EC_COORD_PROJECTIVE:
		{
			retcode = point_add_projective(metadata, p_a, p_b);
			if (EC_GEOMETRY_DOUBLE == retcode) {
				retcode = point_double_projective(metadata, p_a);
			}
		} break;
	}

	return retcode;
}

static void scalar_point_mult_binary(size_t num_bits, ecc_metadata_t *metadata,
	const ecc_point_t *p_in, const sc_ulimb_t *secret, ecc_point_t *p_out)
{
	size_t i;
	point_secret_t bit_ctx;
	ecc_point_t p_dummy;

	point_init(&p_dummy, metadata->k);
	point_reset(&p_dummy);
	point_reset(p_out);
	point_copy(p_out, p_in);

	/*fprintf(stderr, "in   x: "); sc_mpz_out_str(stderr, 16, &p_in->x); fprintf(stderr, "\n");
	fprintf(stderr, "     y: "); sc_mpz_out_str(stderr, 16, &p_in->y); fprintf(stderr, "\n");
	fprintf(stderr, "out  x: "); sc_mpz_out_str(stderr, 16, &p_out->x); fprintf(stderr, "\n");
	fprintf(stderr, "     y: "); sc_mpz_out_str(stderr, 16, &p_out->y); fprintf(stderr, "\n");
	fprintf(stderr, "secret: %016llX %016llX %016llX %016llX\n", secret[3], secret[2], secret[1], secret[0]);*/

	// Windowing
	size_t w = 4;
	ecc_point_t *p_window = SC_MALLOC(sizeof(ecc_point_t) * (1 << w));
	point_init(&p_window[0], metadata->k);
	point_copy(&p_window[0], p_in);
	for (i=1; i<(1 << w); i++) {
		point_init(&p_window[i], metadata->k);
		point_copy(&p_window[i], &p_window[i-1]);
		point_add(metadata, &p_window[i], p_in);
	}

	num_bits = secret_bits_init(ECC_K_BINARY, &bit_ctx, secret, num_bits);
	secret_bits_pull(&bit_ctx);
	num_bits--;

	for (i=num_bits; i--;) {
		sc_ulimb_t bit;

		// Point doubling
		point_double(metadata, p_out);

		// Determine if an asserted bit requires a point addition (or a dummy point addition as an SCA countermeasure)
		bit = secret_bits_pull(&bit_ctx);
		//fprintf(stderr, "index = %zu, bit = %d\n", i, bit);
		if (ECC_K_IS_LOW != bit) {
			// Create a mask of all zeros if ECC_K_IS_HIGH or all ones if an ECC_K_IS_SCA_DUMMY operation
			intptr_t temp   = (intptr_t) (bit << (SC_LIMB_BITS - 1));
			intptr_t mask   = (bit ^ temp) - temp;

			// Branch-free pointer selection in constant time
			intptr_t p_temp = (intptr_t) p_out ^ (((intptr_t) p_out ^ (intptr_t) &p_dummy) & mask);
			//fprintf(stderr, "%zu %zu %zu\n", p_temp, (intptr_t) p_out, (intptr_t) &p_dummy);

			// Point addition
			point_add(metadata, (ecc_point_t *) p_temp, p_in);
		}
	}

	point_clear(&p_dummy);

	for (i=0; i<(1 << w); i++) {
		point_clear(&p_window[i]);
	}
	SC_FREE(p_window, sizeof(ecc_point_t) * (1 << w));

	//fprintf(stderr, "result x: "); sc_mpz_out_str(stderr, 16, &p_out->x); fprintf(stderr, "\n");
	//fprintf(stderr, "       y: "); sc_mpz_out_str(stderr, 16, &p_out->y); fprintf(stderr, "\n");
}

static void scalar_point_mult_naf(size_t num_bits, ecc_metadata_t *metadata,
	const ecc_point_t *p_in, const sc_ulimb_t *secret, ecc_point_t *p_out)
{
	size_t i;
	UINT32 bit;
	point_secret_t bit_ctx;
	ecc_point_t p_dummy, p_in_minus;

	point_init(&p_dummy, metadata->k);
	point_reset(&p_dummy);
	point_reset(p_out);
	point_copy(p_out, p_in);
	point_init(&p_in_minus, metadata->k);
	point_copy(&p_in_minus, p_in);
	point_negate(&p_in_minus);

	/*fprintf(stderr, "in   x: "); sc_mpz_out_str(stderr, 16, &p_in->x); fprintf(stderr, "\n");
	fprintf(stderr, "     y: "); sc_mpz_out_str(stderr, 16, &p_in->y); fprintf(stderr, "\n");
	fprintf(stderr, "secret: %016llX %016llX %016llX %016llX %016llX %016llX %016llX %016llX %016llX\n",
		secret[8], secret[7], secret[6], secret[5], secret[4], secret[3], secret[2], secret[1], secret[0]);*/

	// Windowing
	/*size_t w = 4;
	ecc_point_t *p_window = SC_MALLOC(sizeof(ecc_point_t) * (1 << w));
	point_init(&p_window[0], MAX_ECC_LIMBS);
	point_copy(&p_window[0], p_in);
	for (i=1; i<(1 << w); i++) {
		point_init(&p_window[i], MAX_ECC_LIMBS);
		point_copy(&p_window[i], &p_window[i-1]);
		point_add(metadata, &p_window[i], p_in);
	}*/

	num_bits = secret_bits_init(ECC_K_NAF_2, &bit_ctx, secret, num_bits);
	bit = secret_bits_pull(&bit_ctx);
	num_bits--;

	for (i=num_bits; i--;) {
		// Point doubling
		point_double(metadata, p_out);

		// Determine if an asserted bit requires a point addition (or a dummy point addition as an SCA countermeasure)
		bit = secret_bits_pull(&bit_ctx);
		//fprintf(stderr, "index = %zu, bit = %d\n", i, bit);
		if (ECC_K_IS_LOW != bit) {
			// Branch-free pointer selection in constant time where we create a mask of all zeros
			// if ECC_K_IS_HIGH or all ones if an ECC_K_IS_SCA_DUMMY operation
			intptr_t mask, p_temp, p_temp2;
			sc_ulimb_t hide     = (bit & 0x1) - ((bit & 0x2) >> 1);
			sc_ulimb_t subtract = (bit >> 1);
			mask   = (intptr_t) 0 - (intptr_t) hide;
			p_temp = (intptr_t) p_out ^ (((intptr_t) p_out ^ (intptr_t) &p_dummy) & mask);
			mask   = (intptr_t) 0 - (intptr_t) subtract;
			p_temp2 = (intptr_t) p_in ^ (((intptr_t) p_in ^ (intptr_t) &p_in_minus) & mask);

			// Point addition
			point_add(metadata, (ecc_point_t *) p_temp, (ecc_point_t *) p_temp2);
		}
	}

	point_clear(&p_dummy);
	point_clear(&p_in_minus);

	/*for (i=0; i<(1 << w); i++) {
		point_clear(&p_window[i]);
	}
	SC_FREE(p_window, sizeof(ecc_point_t) * (1 << w));*/

	/*fprintf(stderr, "result x: "); sc_mpz_out_str(stderr, 16, &p_out->x); fprintf(stderr, "\n");
	fprintf(stderr, "       y: "); sc_mpz_out_str(stderr, 16, &p_out->y); fprintf(stderr, "\n");*/
}

static void scalar_point_mult(size_t num_bits, ecc_metadata_t *metadata,
	const ecc_point_t *p_in, const sc_ulimb_t *secret, ecc_point_t *p_out)
{
#if 0
	scalar_point_mult_binary(num_bits, metadata, p_in, secret, p_out);
#else
	scalar_point_mult_naf(num_bits, metadata, p_in, secret, p_out);
#endif
}

SINT32 ecc_diffie_hellman(safecrypto_t *sc, const ecc_point_t *p_base, const sc_ulimb_t *secret, size_t *tlen, UINT8 **to, SINT32 final_flag)
{
	size_t i;
	ecc_point_t p_result;
	size_t num_bits, num_bytes, num_limbs;
	ecc_metadata_t metadata;

	// If the sc pointer is NULL then return with a failure
	if (NULL == sc) {
		return SC_FUNC_FAILURE;
	}

	// Obtain common array lengths
	num_bits  = sc->ec->params->num_bits;
	num_bytes = sc->ec->params->num_bytes;
	num_limbs = sc->ec->params->num_limbs;

	// Initialise the MP variables
	metadata.k = num_limbs;
	point_init(&p_result, num_limbs);
	sc_mpz_init2(&metadata.lambda, num_bits);
	sc_mpz_init2(&metadata.x, num_bits);
	sc_mpz_init2(&metadata.y, num_bits);
	sc_mpz_init2(&metadata.z, num_bits);
	sc_mpz_init2(&metadata.h, num_bits);
	sc_mpz_init2(&metadata.w, num_bits);
	sc_mpz_init2(&metadata.temp, 2*num_bits);
	sc_mpz_init2(&metadata.a, num_bits);
	sc_mpz_init2(&metadata.m, num_bits);
	sc_mpz_init2(&metadata.m_inv, num_bits+1);
	sc_mpz_set_str(&metadata.a, 16, sc->ec->params->a);
	sc_mpz_set_str(&metadata.m, 16, sc->ec->params->p);
	sc_mpz_set_str(&metadata.m_inv, 16, sc->ec->params->p_inv);

	// Perform a scalar point multiplication from the base point using the random secret
	scalar_point_mult(num_bits, &metadata, p_base, secret, &p_result);

	// Translate the output point (coordinates are MP variables) to the output byte stream
	*to = SC_MALLOC((2-final_flag)*num_bytes);
	*tlen = (2-final_flag) * num_bytes;
	sc_mpz_get_bytes(*to, &p_result.x, num_bytes);
	if (0 == final_flag) {
		sc_mpz_get_bytes(*to + num_bytes, &p_result.y, num_bytes);
	}

	// Free resources associated with the MP variables
	sc_mpz_clear(&metadata.lambda);
	sc_mpz_clear(&metadata.x);
	sc_mpz_clear(&metadata.y);
	sc_mpz_clear(&metadata.z);
	sc_mpz_clear(&metadata.h);
	sc_mpz_clear(&metadata.w);
	sc_mpz_clear(&metadata.temp);
	sc_mpz_clear(&metadata.a);
	sc_mpz_clear(&metadata.m);
	sc_mpz_clear(&metadata.m_inv);
	point_clear(&p_result);

	return SC_FUNC_SUCCESS;
}

SINT32 ecc_diffie_hellman_encapsulate(safecrypto_t *sc, const sc_ulimb_t *secret,
	size_t *tlen, UINT8 **to)
{
	// Use an ECC scalar point multiplication to geometrically transform the base point
	// to the intermediate point (i.e. the public key)
	ecc_diffie_hellman(sc, &sc->ec->base, secret, tlen, to, 0);

	return SC_FUNC_SUCCESS;
}

SINT32 ecc_diffie_hellman_decapsulate(safecrypto_t *sc, const sc_ulimb_t *secret,
	size_t flen, const UINT8 *from, size_t *tlen, UINT8 **to)
{
	size_t num_bytes = sc->ec->params->num_bytes;
	size_t num_limbs = sc->ec->params->num_limbs;

	// Convert the input byte stream (public key) to the intermediate coordinate
	ecc_point_t p_base;
	point_init(&p_base, num_limbs);
	sc_mpz_set_bytes(&p_base.x, from, num_bytes);
	sc_mpz_set_bytes(&p_base.y, from + num_bytes, num_bytes);

	// Use an ECC scalar point multiplication to geometrically transform the intermediate point
	// to the final point (shared secret)
	ecc_diffie_hellman(sc, &p_base, secret, tlen, to, 1);

	point_clear(&p_base);

	return SC_FUNC_SUCCESS;
}

SINT32 ecc_keygen(safecrypto_t *sc)
{
	SINT32 retval = SC_FUNC_FAILURE;
	size_t num_bits, num_bytes, num_limbs;
	sc_ulimb_t *secret;
	ecc_point_t p_public;
	ecc_metadata_t metadata;

	num_bits  = sc->ec->params->num_bits;
	num_bytes = sc->ec->params->num_bytes;
	num_limbs = sc->ec->params->num_limbs;

	metadata.k = num_limbs;
	sc_mpz_init2(&metadata.lambda, num_bits);
	sc_mpz_init2(&metadata.x, num_bits);
	sc_mpz_init2(&metadata.y, num_bits);
	sc_mpz_init2(&metadata.z, num_bits);
	sc_mpz_init2(&metadata.h, num_bits);
	sc_mpz_init2(&metadata.w, num_bits);
	sc_mpz_init2(&metadata.temp, 2*num_bits);
	sc_mpz_init2(&metadata.a, num_bits);
	sc_mpz_init2(&metadata.m, num_bits);
	sc_mpz_init2(&metadata.m_inv, num_bits+1);
	sc_mpz_init2(&metadata.order_m, num_bits + 64);
	sc_mpz_set_str(&metadata.a, 16, sc->ec->params->a);
	sc_mpz_set_str(&metadata.m, 16, sc->ec->params->p);
	sc_mpz_set_str(&metadata.m_inv, 16, sc->ec->params->p_inv);
	sc_mpz_set_str(&metadata.order_m, 16, sc->ec->params->order_m);

	// Allocate memory for the private key
    if (NULL == sc->privkey->key) {
        sc->privkey->key = SC_MALLOC(sizeof(sc_ulimb_t) * num_limbs);
        if (NULL == sc->privkey->key) {
            SC_LOG_ERROR(sc, SC_NULL_POINTER);
            goto finish_free;
        }
    }
	secret = sc->privkey->key;

	// Generate a secret key as a random number
	/*fprintf(stderr, "private key = ");
	for (size_t q=0; q<32; q++) {
		fprintf(stderr, "%02X", ((UINT8*)sc->privkey->key)[q]);
		if (7 == (q & 0x7)) {
			fprintf(stderr, " ");
		}
	}
	fprintf(stderr, "\n");*/
	prng_mem(sc->prng_ctx[0], (UINT8*)secret, num_bytes);
	//fprintf(stderr, "private key = %016lX %016lX %016lX %016lX\n", secret[3], secret[2], secret[1], secret[0]);

	// Generate the public key as the product of a scalar multiplication
	// of the base point with k
	point_init(&p_public, metadata.k);
	scalar_point_mult(num_bits, &metadata, &sc->ec->base, secret, &p_public);
	sc_mpz_mod(&p_public.x, &p_public.x, &metadata.order_m);
	sc_mpz_mod(&p_public.y, &p_public.y, &metadata.order_m);
	/*fprintf(stderr, "public x = "); sc_mpz_out_str(stderr, 16, &p_public.x); fprintf(stderr, "\n");
	fprintf(stderr, "public y = "); sc_mpz_out_str(stderr, 16, &p_public.y); fprintf(stderr, "\n");*/

	// Allocate memory for the public key
    if (NULL == sc->pubkey->key) {
        sc->pubkey->key = SC_MALLOC(sizeof(sc_ulimb_t) * 2 * num_limbs);
        if (NULL == sc->pubkey->key) {
            SC_LOG_ERROR(sc, SC_NULL_POINTER);
            goto finish_free;
        }
    }

    // Copy the public key to storage
	sc_mpz_get_bytes(sc->pubkey->key, &p_public.x, num_bytes);
	sc_mpz_get_bytes(sc->pubkey->key + num_bytes, &p_public.y, num_bytes);

	retval = SC_FUNC_SUCCESS;

finish_free:
	// Free resources
	point_clear(&p_public);
	sc_mpz_clear(&metadata.lambda);
	sc_mpz_clear(&metadata.x);
	sc_mpz_clear(&metadata.y);
	sc_mpz_clear(&metadata.z);
	sc_mpz_clear(&metadata.h);
	sc_mpz_clear(&metadata.w);
	sc_mpz_clear(&metadata.temp);
	sc_mpz_clear(&metadata.a);
	sc_mpz_clear(&metadata.m);
	sc_mpz_clear(&metadata.m_inv);
	sc_mpz_clear(&metadata.order_m);

	return retval;
}

SINT32 ecc_sign(safecrypto_t *sc, const UINT8 *m, size_t mlen,
    UINT8 **sigret, size_t *siglen)
{
	size_t i, num_bits, num_bytes, num_limbs;
	ecc_point_t p_base, p_result;
	sc_ulimb_t secret[MAX_ECC_LIMBS];
	sc_mpz_t d, e, k, temp1, temp2;
	SINT32 mem_is_zero;
	ecc_metadata_t metadata;

	// Obtain common array lengths
	num_bits  = sc->ec->params->num_bits;
	num_bytes = sc->ec->params->num_bytes;
	num_limbs = sc->ec->params->num_limbs;

	if (0 != *siglen && *siglen < 2*num_bytes) {
		return SC_FUNC_FAILURE;
	}

	metadata.k = num_limbs;
	sc_mpz_init2(&metadata.lambda, num_bits);
	sc_mpz_init2(&metadata.x, num_bits);
	sc_mpz_init2(&metadata.y, num_bits);
	sc_mpz_init2(&metadata.z, num_bits);
	sc_mpz_init2(&metadata.h, num_bits);
	sc_mpz_init2(&metadata.w, num_bits);
	sc_mpz_init2(&metadata.temp, 2*num_bits);
	sc_mpz_init2(&metadata.a, num_bits);
	sc_mpz_init2(&metadata.m, num_bits);
	sc_mpz_init2(&metadata.m_inv, num_bits+1);
	sc_mpz_init2(&metadata.order_m, num_bits + 64);
	sc_mpz_set_str(&metadata.a, 16, sc->ec->params->a);
	sc_mpz_set_str(&metadata.m, 16, sc->ec->params->p);
	sc_mpz_set_str(&metadata.m_inv, 16, sc->ec->params->p_inv);
	sc_mpz_set_str(&metadata.order_m, 16, sc->ec->params->order_m);

	sc_mpz_init2(&d, num_bits);
	sc_mpz_init2(&e, num_bits);
	sc_mpz_init2(&k, num_bits);
	sc_mpz_init2(&temp1, 2*num_bits);
	sc_mpz_init2(&temp2, num_bits);
	point_init(&p_base, num_limbs);
	point_init(&p_result, num_limbs);

	p_base.n = (num_bits + SC_LIMB_BITS - 1) >> SC_LIMB_BITS_SHIFT;
	sc_mpz_set_str(&p_base.x, 16, sc->ec->params->g_x);
	sc_mpz_set_str(&p_base.y, 16, sc->ec->params->g_y);

	sc_mpz_set_limbs(&d, (sc_ulimb_t*) sc->privkey->key, num_limbs);
	sc_mpz_set_bytes(&e, m, mlen);

	/*fprintf(stderr, "base x      = "); sc_mpz_out_str(stderr, 16, &p_base.x); fprintf(stderr, "\n");
	fprintf(stderr, "base y      = "); sc_mpz_out_str(stderr, 16, &p_base.y); fprintf(stderr, "\n");
	fprintf(stderr, "private key = "); sc_mpz_out_str(stderr, 16, &d); fprintf(stderr, "\n");
	fprintf(stderr, "message     = "); sc_mpz_out_str(stderr, 16, &e); fprintf(stderr, "\n");*/

restart:
	// Generate a random secret k
	prng_mem(sc->prng_ctx[0], (UINT8*)secret, num_bytes);
	sc_mpz_set_limbs(&k, secret, num_limbs);
	//fprintf(stderr, "k           = "); sc_mpz_out_str(stderr, 16, &k); fprintf(stderr, "\n");

	// Perform a scalar point multiplication from the base point using the random secret k
	scalar_point_mult(num_bits, &metadata, &p_base, secret, &p_result);
	sc_mpz_mod(&p_result.x, &p_result.x, &metadata.order_m);
	if (sc_mpz_is_zero(&p_result.x)) {
		goto restart;
	}

	// s = k^(-1)*(z + r*d) mod n
	sc_mpz_mul(&temp1, &p_result.x, &d);
	sc_mpz_mod(&temp2, &temp1, &metadata.order_m);
	//fprintf(stderr, "r*d = "); sc_mpz_out_str(stderr, 16, &temp2); fprintf(stderr, "\n");
	sc_mpz_add(&temp1, &temp2, &e);
	sc_mpz_mod(&temp1, &temp1, &metadata.order_m);
	//fprintf(stderr, "z + r*d = "); sc_mpz_out_str(stderr, 16, &temp1); fprintf(stderr, "\n");
	sc_mpz_invmod(&temp2, &k, &metadata.order_m);
	//fprintf(stderr, "k^{-1} = "); sc_mpz_out_str(stderr, 16, &temp2); fprintf(stderr, "\n");
	sc_mpz_mul(&temp1, &temp2, &temp1);
	sc_mpz_mod(&temp2, &temp1, &metadata.order_m);
	if (sc_mpz_is_zero(&temp2)) {
		goto restart;
	}

	/*fprintf(stderr, "r = "); sc_mpz_out_str(stderr, 16, &p_result.x); fprintf(stderr, "\n");
	fprintf(stderr, "s = "); sc_mpz_out_str(stderr, 16, &temp2); fprintf(stderr, "\n");
	fprintf(stderr, "bases are %d and %d\n", sc_mpz_sizeinbase(&p_result.x, 16), sc_mpz_sizeinbase(&temp2, 16));*/

	// Pack r and s into the output signature
	if (0 == *siglen || 0 == *sigret) {
		*sigret = SC_MALLOC(2*num_bytes);
	}
	*siglen = 2*num_bytes;
	sc_mpz_get_bytes(*sigret, &p_result.x, num_bytes);
	sc_mpz_get_bytes(*sigret + num_bytes, &temp2, num_bytes);

	/*for (i=0; i<num_bytes; i++) {
		fprintf(stderr, "%02x", (int)((*sigret)[num_bytes-1-i]));
	}
	fprintf(stderr, "\n");
	for (i=0; i<num_bytes; i++) {
		fprintf(stderr, "%02x", (int)((*sigret)[2*num_bytes-1-i]));
	}
	fprintf(stderr, "\n");*/

	sc_mpz_clear(&metadata.lambda);
	sc_mpz_clear(&metadata.x);
	sc_mpz_clear(&metadata.y);
	sc_mpz_clear(&metadata.z);
	sc_mpz_clear(&metadata.h);
	sc_mpz_clear(&metadata.w);
	sc_mpz_clear(&metadata.temp);
	sc_mpz_clear(&metadata.a);
	sc_mpz_clear(&metadata.m);
	sc_mpz_clear(&metadata.m_inv);
	sc_mpz_clear(&metadata.order_m);

	sc_mpz_clear(&d);
	sc_mpz_clear(&e);
	sc_mpz_clear(&k);
	sc_mpz_clear(&temp1);
	sc_mpz_clear(&temp2);
	point_clear(&p_base);
	point_clear(&p_result);

	return SC_FUNC_SUCCESS;
}

SINT32 ecc_verify(safecrypto_t *sc, const UINT8 *m, size_t mlen,
    const UINT8 *sigbuf, size_t siglen)
{
	SINT32 retval;
	size_t i, num_bits, num_bytes, num_limbs;
	sc_mpz_t r, s, w, temp, z;
	sc_ulimb_t *secret;
	ecc_point_t p_base, p_public, p_u1, p_u2;
	ecc_metadata_t metadata;

	// Obtain common array lengths
	num_bits  = sc->ec->params->num_bits;
	num_bytes = sc->ec->params->num_bytes;
	num_limbs = sc->ec->params->num_limbs;

	// VErify that the signature length is valid and accomodates the r and s components
	if (siglen != 2*num_bytes) {
		return SC_FUNC_FAILURE;
	}

	// Configure the curve parameters
	metadata.k = num_limbs;
	sc_mpz_init2(&metadata.lambda, num_bits);
	sc_mpz_init2(&metadata.x, num_bits);
	sc_mpz_init2(&metadata.y, num_bits);
	sc_mpz_init2(&metadata.z, num_bits);
	sc_mpz_init2(&metadata.h, num_bits);
	sc_mpz_init2(&metadata.w, num_bits);
	sc_mpz_init2(&metadata.temp, 2*num_bits);
	sc_mpz_init2(&metadata.a, num_bits);
	sc_mpz_init2(&metadata.m, num_bits);
	sc_mpz_init2(&metadata.m_inv, num_bits+1);
	sc_mpz_init2(&metadata.order_m, num_bits + 64);
	sc_mpz_set_str(&metadata.a, 16, sc->ec->params->a);
	sc_mpz_set_str(&metadata.m, 16, sc->ec->params->p);
	sc_mpz_set_str(&metadata.m_inv, 16, sc->ec->params->p_inv);
	sc_mpz_set_str(&metadata.order_m, 16, sc->ec->params->order_m);
	sc_mpz_init2(&temp, 2*num_bits);
	sc_mpz_init2(&r, num_bits);
	sc_mpz_init2(&s, num_bits);
	sc_mpz_init2(&w, num_bits);
	sc_mpz_init2(&z, num_bits);
	point_init(&p_base, sc->ec->params->num_limbs);
	point_init(&p_public, sc->ec->params->num_limbs);
	point_init(&p_u1, sc->ec->params->num_limbs);
	point_init(&p_u2, sc->ec->params->num_limbs);

	/*for (i=0; i<num_bytes; i++) {
		fprintf(stderr, "%02x", (int)(sigbuf[num_bytes-1-i]));
	}
	fprintf(stderr, "\n");
	for (i=0; i<num_bytes; i++) {
		fprintf(stderr, "%02x", (int)(sigbuf[2*num_bytes-1-i]));
	}
	fprintf(stderr, "\n");*/

	sc_mpz_set_bytes(&r, sigbuf, num_bytes);
	sc_mpz_set_bytes(&s, sigbuf + num_bytes, num_bytes);
	//fprintf(stderr, "r = "); sc_mpz_out_str(stderr, 16, &r); fprintf(stderr, "\n");
	//fprintf(stderr, "s = "); sc_mpz_out_str(stderr, 16, &s); fprintf(stderr, "\n");

	sc_mpz_set_bytes(&z, m, mlen);

	// w = s^{-1}
	sc_mpz_invmod(&w, &s, &metadata.order_m);

	// Obtain the public key Q in the form of an elliptic curve point
	//p_public.n = sc->ec->params->num_limbs;
	sc_mpz_set_bytes(&p_public.x, sc->pubkey->key, num_bytes);
	sc_mpz_set_bytes(&p_public.y, sc->pubkey->key + num_bytes, num_bytes);
	/*fprintf(stderr, "public x = "); sc_mpz_out_str(stderr, 16, &p_public.x); fprintf(stderr, "\n");
	fprintf(stderr, "public y = "); sc_mpz_out_str(stderr, 16, &p_public.y); fprintf(stderr, "\n");*/

	// Obtain the base point G
	p_base.n = sc->ec->params->num_limbs;
	sc_mpz_set_str(&p_base.x, 16, sc->ec->params->g_x);
	sc_mpz_set_str(&p_base.y, 16, sc->ec->params->g_y);

	// u1 = w * z * G
	sc_mpz_mul(&temp, &w, &z);
	sc_mpz_mod(&temp, &temp, &metadata.order_m);
	secret = sc_mpz_get_limbs(&temp);
	scalar_point_mult(num_bits, &metadata, &p_base, secret, &p_u1);

	// u2 = w * r * Q
	sc_mpz_mul(&temp, &w, &r);
	sc_mpz_mod(&temp, &temp, &metadata.order_m);
	secret = sc_mpz_get_limbs(&temp);
	scalar_point_mult(num_bits, &metadata, &p_public, secret, &p_u2);

	// Point addition to obtain the signature point on the curve
	/*fprintf(stderr, "u1 x = "); sc_mpz_out_str(stderr, 16, &p_u1.x); fprintf(stderr, "\n");
	fprintf(stderr, "u1 y = "); sc_mpz_out_str(stderr, 16, &p_u1.y); fprintf(stderr, "\n");
	fprintf(stderr, "u2 x = "); sc_mpz_out_str(stderr, 16, &p_u2.x); fprintf(stderr, "\n");
	fprintf(stderr, "u2 y = "); sc_mpz_out_str(stderr, 16, &p_u2.y); fprintf(stderr, "\n");*/
	point_add(&metadata, &p_u1, &p_u2);
	/*fprintf(stderr, "u1 x = "); sc_mpz_out_str(stderr, 16, &p_u1.x); fprintf(stderr, "\n");
	fprintf(stderr, "u1 y = "); sc_mpz_out_str(stderr, 16, &p_u1.y); fprintf(stderr, "\n");*/

	// Validate the signature
	if (0 == sc_mpz_cmp(&p_u1.x, &r)) {
		retval = SC_FUNC_SUCCESS;
	}
	else {
		retval = SC_FUNC_FAILURE;
	}

	// Free memory resources
	sc_mpz_clear(&metadata.lambda);
	sc_mpz_clear(&metadata.x);
	sc_mpz_clear(&metadata.y);
	sc_mpz_clear(&metadata.z);
	sc_mpz_clear(&metadata.h);
	sc_mpz_clear(&metadata.w);
	sc_mpz_clear(&metadata.temp);
	sc_mpz_clear(&metadata.a);
	sc_mpz_clear(&metadata.m);
	sc_mpz_clear(&metadata.m_inv);
	sc_mpz_clear(&metadata.order_m);
	sc_mpz_clear(&temp);
	sc_mpz_clear(&r);
	sc_mpz_clear(&s);
	sc_mpz_clear(&w);
	sc_mpz_clear(&z);
	point_clear(&p_base);
	point_clear(&p_public);
	point_clear(&p_u1);
	point_clear(&p_u2);

	return retval;
}



//
// end of file
//
