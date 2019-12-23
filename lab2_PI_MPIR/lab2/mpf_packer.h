#include "pch.h"
#include "mpir/mpir.h"
#include <mpi.h>

#define PREC 12000U
#define NLIMBS(n) \
    ((mp_size_t) ((__GMP_MAX (53, n) + 2 * GMP_NUMB_BITS - 1) / GMP_NUMB_BITS))
#define MPF_D_SIZE(prec) (((prec) + 1) * sizeof(mp_limb_t))
#define MPF_PACKED_LIMBS(prec) ((prec) + 4);

mp_limb_t *mpf_pack(mp_limb_t *dest, mpf_t *src, int n);

mpf_t *mpf_unpack(mpf_t *dest, mp_limb_t *src, int n);

void mpf_packed_add(void *_in, void *_inout, int *len, MPI_Datatype *datatype);
