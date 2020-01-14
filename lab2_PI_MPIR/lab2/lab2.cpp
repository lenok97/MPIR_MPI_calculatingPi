#include "pch.h"
#include <iostream>
#include <string>
#include <mpi.h>
#include "mpir/mpir.h"
#include "mpf_packer.h"
using namespace std;

const int root = 0;
string realPi = // 150 decimal places
"3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128";
size_t mpf_packed_size;

mpf_t t16, t1;
int num[4] = { 4,2,1,1 };
int den_add[4] = { 1,4,5,6 };

void calculateTerm(mpf_t term, int b, int c, int i )
{
	mpf_t k;
	mpf_init_set_d(k, i);
	mpf_mul_ui(term, k, 8);
	mpf_add_ui(term, term, b);
	mpf_ui_div(term, c, term);
}

// Bailey–Borwein–Plouffe formula (Бэйли — Боруэйна — Плаффа)
//pi = Σ(i=0.. n)((1 / pow(16,i))*((4 / ((8 * i) + 1))- (2 / ((8 * i) + 4)) - (1 / ((8 * i) + 5)) - (1 / ((8 * i) +6))));
void calculate_BBP(mpf_t term[5], mpf_t pi_term, int number)
{
	mpf_pow_ui(term[0], t16, number);
	mpf_div(term[0], t1, term[0]);
	for (int i = 1; i < 5; i++)
	{
		calculateTerm(term[i], den_add[i-1], num[i-1], number);
	}
	//pi_term = term1 - term2 - term3 - term4;
	mpf_sub(pi_term, term[1], term[2]);
	mpf_sub(pi_term, pi_term, term[3]);
	mpf_sub(pi_term, pi_term, term[4]);
	////pi_term = pi_term * term0;
	mpf_mul(pi_term, pi_term, term[0]);
}

int main(int argc, char *argv[])
{
	int rank, size, n = 0;
	double endTime, startTime;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// Create MPI Datatype based on the target precision
	mpf_set_default_prec(PREC); 
	mpf_packed_size = ((NLIMBS(mpf_get_default_prec()) +5) * sizeof(mp_limb_t));
	MPI_Datatype MPI_MPF;
	// Производный тип MPI - скрытый объект, который специфицирует: последовательность базовых типов и последовательность смещений.
	MPI_Type_contiguous(mpf_packed_size, MPI_CHAR, &MPI_MPF); 
	// Create MPI Operation for adding mpfs
	MPI_Type_commit(&MPI_MPF); 
	MPI_Op MPI_SUM_MPF;
	MPI_Op_create(mpf_packed_add, true, &MPI_SUM_MPF); // true if commutative
	while (true)
	{
		if (rank == root)
		{
			cout <<"Computing pi" << endl <<"Process count = " << size << endl;
			cout << "Count of decimal places (1 - 2000): ";
			cin >> n;
			startTime = MPI_Wtime();
		}
		MPI_Bcast(&n, 1, MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);

		int workPerProc = n/size;
		int extraElement = n % size;
		int startPoint = rank * workPerProc;
		int endPoint = startPoint + workPerProc-1 ;
		if (rank <= extraElement)
		{
			startPoint +=rank;
			endPoint = startPoint + workPerProc;
		}
		else
			if (rank > extraElement)
			{
				startPoint += extraElement +1;
				endPoint = startPoint + workPerProc-1;
			}

		// init mpir variables 
		mpf_t  pi_term, temp_pi, pi, term[5];
		mp_limb_t *sum_packed;
		for (int i=0; i<5; i++)
			mpf_init(term[i]);  
		mpf_init(temp_pi); mpf_init(pi_term);
		mpf_init_set_d(t1, 1); mpf_init_set_d(t16, 16);

		for (int number = startPoint; number <= endPoint; number++)
		{
			calculate_BBP(term, pi_term, number);
			mpf_add(temp_pi, temp_pi, pi_term);
		}

		sum_packed = (mp_limb_t*)malloc(mpf_packed_size);
		mp_limb_t *packed = mpf_pack(NULL, &temp_pi, 1);
		MPI_Reduce(packed, sum_packed, 1, MPI_MPF, MPI_SUM_MPF, root, MPI_COMM_WORLD);

		if (rank == root)
		{
			mpf_init(pi);
			mpf_unpack(&pi, sum_packed, 1);
			endTime = MPI_Wtime();
			cout.precision(n + 1);
			cout << endl << pi << " - computed pi" << endl << realPi << " - real pi" << endl;
			cout.precision(3);
			cout<<"Time elapsed: "<< (endTime - startTime) * 1000 << "ms\n" << endl << endl;
			mpf_clear(pi);
		}

		for (int i = 0; i < 5; i++)
			mpf_clear(term[i]);
		mpf_clear(pi_term);
		mpf_clear(temp_pi);
	}
	MPI_Finalize();
	return 0;
}
