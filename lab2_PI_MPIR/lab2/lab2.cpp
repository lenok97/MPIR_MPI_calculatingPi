#include "pch.h"
#include <iostream>
#include <string>
#include <mpi.h>
#include "mpir/mpir.h"
#include "mpf_packer.h"
using namespace std;

string realPi = // 150 decimal places
"3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128";
size_t mpf_packed_size;

void calculateTerm(mpf_t term, int a, int b, int c, int i )
{
	mpf_t k;
	mpf_init_set_d(k, i);
	mpf_mul_ui(term, k, a);
	mpf_add_ui(term, term, b);
	mpf_ui_div(term, c, term);
}

int main(int argc, char *argv[])
{
	int rank, size, root = 0, startTag = 0, endTag = 1, n=0, workPerProc, startPoint, endPoint;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// Create MPI Datatype based on the target precision
	mpf_set_default_prec(PREC); 
	mpf_packed_size = ((NLIMBS(mpf_get_default_prec()) +5) * sizeof(mp_limb_t));
	MPI_Datatype MPI_MPF;
	MPI_Type_contiguous(mpf_packed_size, MPI_CHAR, &MPI_MPF);
	// Create MPI Operation for adding mpfs
	MPI_Type_commit(&MPI_MPF); 
	MPI_Op MPI_SUM_MPF;
	MPI_Op_create(mpf_packed_add, 1, &MPI_SUM_MPF); 
	while (true)
	{
		if (rank == root)
		{
			cout << "Process count = " << size << endl;
			cout << "Count of decimal places: ";
			cin >> n;
			workPerProc = n /size;
			int rest = n % size;
			int start = 0;
			int end = workPerProc;
			startPoint = start;
			endPoint = end;
			for (int i = 1; i < size; i++)
			{
				start = end+1;
				end = start + workPerProc-1;
				if (rest > 0)
				{
					end++;
					rest--;
				}
				MPI_Send(&start, 1, MPI_INT, i, startTag, MPI_COMM_WORLD);
				MPI_Send(&end, 1, MPI_INT, i, endTag, MPI_COMM_WORLD);
			}
		}
		// init mpir variables 
		mpf_t term0, term1, term2, term3, term4, k, pi_term, final_pi, pi, t16, t1;
		mp_limb_t *sum_packed;
		mpf_init(term0); mpf_init(term1); mpf_init(term2); mpf_init(term3); mpf_init(term4); mpf_init(pi_term);  
		mpf_init(final_pi); mpf_init(pi);
		mpf_init_set_d(t1, 1); mpf_init_set_d(t16, 16);

		if (rank != root)
		{
			MPI_Recv(&startPoint, 1, MPI_INT, root, startTag, MPI_COMM_WORLD, &status);
			MPI_Recv(&endPoint, 1, MPI_INT, root, endTag, MPI_COMM_WORLD, &status);
		}
		for (int i = startPoint; i <= endPoint; i++)
		{
			// Bailey–Borwein–Plouffe formula
			//pi = Σ(i=0.. n)((1 / pow(16,i))*((4 / ((8 * i) + 1))- (2 / ((8 * i) + 4)) - (1 / ((8 * i) + 5)) - (1 / ((8 * i) +6))));
			mpf_pow_ui(term0, t16, i);
			mpf_div(term0, t1, term0);
			calculateTerm(term1, 8, 1, 4, i);
			calculateTerm(term2, 8, 4, 2, i);
			calculateTerm(term3, 8, 5, 1, i);
			calculateTerm(term4, 8, 6, 1, i);
			//pi_term = term1 - term2 - term3 - term4;
			mpf_sub(pi_term, term1, term2);
			mpf_sub(pi_term, pi_term, term3);
			mpf_sub(pi_term, pi_term, term4);
			////pi_term = pi_term * term0;
			mpf_mul(pi_term, pi_term, term0);
			//final_pi = final_pi + pi_term;
			mpf_add(final_pi, final_pi, pi_term);
		}
		sum_packed = (mp_limb_t*)malloc(mpf_packed_size);
		mp_limb_t *packed = mpf_pack(NULL, &final_pi, 1);
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Reduce(packed, sum_packed, 1, MPI_MPF, MPI_SUM_MPF, root, MPI_COMM_WORLD);

		if (rank == root)
		{
			mpf_unpack(&pi, sum_packed, 1);
			cout.precision(n + 1);
			cout << endl << pi << " - computed pi" << endl << realPi << " - real pi" << endl<< endl;
			mpf_clear(pi);
		}
		mpf_clear(term0);
		mpf_clear(term1);
		mpf_clear(term2);
		mpf_clear(term3);
		mpf_clear(term4);
		mpf_clear(pi_term);
		mpf_clear(final_pi);
	}
	MPI_Finalize();
	return 0;
}
