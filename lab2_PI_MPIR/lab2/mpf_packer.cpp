#include "pch.h"
#include "mpf_packer.h"

#define PREC 12000U
#define NLIMBS(n) \
    ((mp_size_t) ((__GMP_MAX (53, n) + 2 * GMP_NUMB_BITS - 1) / GMP_NUMB_BITS))
#define MPF_D_SIZE(prec) (((prec) + 1) * sizeof(mp_limb_t))
#define MPF_PACKED_LIMBS(prec) ((prec) + 4);

mp_limb_t *mpf_pack(mp_limb_t *dest, mpf_t *src, int n)
{
	if (dest == NULL) {
		size_t size = 0;
		for (int i = 0; i < n; i++)
			size += MPF_PACKED_LIMBS(src[i]->_mp_prec);
		dest = (mp_limb_t *)malloc(size * sizeof(mp_limb_t));
	}
	size_t offset = 0;
	for (int i = 0; i < n; i++) {
		mp_limb_t prec = dest[offset] = src[i]->_mp_prec;
		dest[offset + 1] = src[i]->_mp_size;
		dest[offset + 2] = src[i]->_mp_exp;
		memcpy(&dest[offset + 3], src[i]->_mp_d, MPF_D_SIZE(prec));
		offset += MPF_PACKED_LIMBS(prec);
	}
	return dest;
}

mpf_t *mpf_unpack(mpf_t *dest, mp_limb_t *src, int n)
{
	if (dest == NULL) {
		dest = (mpf_t *)malloc(sizeof(mpf_t) * n);
	}
	else {
		for (int i = 0; i < n; i++)
			if (dest[i]->_mp_d)
				free(dest[i]->_mp_d);
	}
	size_t offset = 0;
	for (int i = 0; i < n; i++) {
		int prec = src[offset] = dest[i]->_mp_prec = src[offset];
		dest[i]->_mp_size = src[offset + 1];
		dest[i]->_mp_exp = src[offset + 2];
		dest[i]->_mp_d = (mp_limb_t *)malloc(MPF_D_SIZE(prec));
		memcpy(dest[i]->_mp_d, &src[offset + 3], MPF_D_SIZE(prec));
		offset += MPF_PACKED_LIMBS(prec);
	}
	return dest;
}

void mpf_packed_add(void *_in, void *_inout, int *len, MPI_Datatype *datatype)
{
	mpf_t *in = mpf_unpack(NULL, (mp_limb_t *)_in, *len);
	mpf_t *inout = mpf_unpack(NULL, (mp_limb_t *)_inout, *len);
	for (int i = 0; i < *len; i++)
		mpf_add(inout[i], in[i], inout[i]);
	mpf_pack((mp_limb_t *)_inout, inout, *len);
	for (int i = 0; i < *len; i++) {
		mpf_clear(in[i]);
		mpf_clear(inout[i]);
	}
	free(in);
	free(inout);
}
// credit https://github.com/TheBB/m-pi