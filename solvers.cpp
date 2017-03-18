#include <iostream>
#include <string>
#include "myfunctions.h"
using namespace std;

#include <mpi.h>

#define F77NAME(x) x##_
extern "C" 
{
  void F77NAME(dgbsv)(const int& n, const int& kl, const int& ku, 
                    const int& nrhs, const double * A, const int& ldab, 
                    int * ipiv, double * B, const int& ldb, int& info);
  void F77NAME(dcopy)(const int& n, const double * x, const int& incx, 
            const double * y, const int& incy);
  void F77NAME(dscal) (const int& n, const double& alpha, const double * x,
            const int& incx);
  void F77NAME(daxpy) (const int& n, const double& alpha, const double * x,
            const int& incx, const double * y, const int& incy);
  void F77NAME(dgemv) (const char& trans,  const int& m,
                const int& n,       const double& alpha,
                const double* a,    const int& lda,
                const double* x,    const int& incx,
                const double& beta, double* y, const int& incy);
  double F77NAME(dnrm2) (const int& n, const double* a, const int& incx);
  void F77NAME(dgbmv)(const char& trans, const int& m, const int& n,
                    const int& kl, const int& ku,
                    const double& alpha, const double* a, const int& lda,
                    const double* x, const int& incx, const double& beta,
                    double* y, const int& incy);
} 

void Static_Solver(double A, double E, double I, double L, double l, double qx, double qy, double Fy, int Nx, int ku, int kl, int N)
{
	const int lda = 1 + 2*kl + ku;

	double * Kb = new double[lda*N]();
	double * F = new double[N]();

	Build_K_global_banded(Kb, A, E, l, L, I, Nx, N, kl, ku, lda, kl);
  Build_F_global(F, Fy, qx, qy, 1.0, l, Nx, N);

  Banded_Matrix_Solver(Kb, F, N, lda, kl, ku);

  Write_Vector(F,N, l, L, "Task1");

  delete[] Kb;
  delete[] F;
}

void Dynamic_Solver_1(double A, double E, double I, double L, double l, double qx, double qy, double Fy, double rho, double T, int Nt, int Nx, int ku, int kl, int N)
{
   	const int lda = 1 + kl + ku;
	  const double del_t = T/(double)Nt;
   		
    double * Mb = new double[N]();
    double * u0 = new double[N]();
    double * u1 = new double[N]();
    double * F = new double[N]();
   	double * Kb = new double[lda*N]();
   	double * d = new double[Nt]();

   	Build_M_global_banded(Mb, rho, A, l, Nx, N);
   	Build_K_global_banded_altered(Kb, Mb, A, E, l, L, I, del_t, Nx, N, kl, ku, lda, 0);
   			
   	for (int i = 0; i<Nt; ++i)
   	{
   		Build_F_global(F, Fy, qx, qy, (double)i*del_t/T, l, Nx, N);
   		Build_Multiplier1(F, Kb, Mb, u0, u1, del_t, N, lda, kl, ku);
      F77NAME(dcopy)(N, u1, 1, u0, 1);
   		for (int j = 0; j<N; ++j)
   		{
   			u1[j] = F[j]/Mb[j];
   		}
   		d[i] = u1[(N-1)/2];					   				
   	}
   	Write_Point_Displacement(d, Nt, "Task2");

   	delete[] Mb;
   	delete[] u1;
   	delete[] F;
    delete[] Kb;
    delete[] d;
}

void Dynamic_Solver_2(double A, double E, double I, double L, double l, double qx, double qy, double Fy, double rho, double T, int Nt, int Nx, int ku, int kl, int N)
{
	  const int lda = 1 + 2*kl + ku;
	  const double del_t = T/(double)Nt;
   	const double beta = 0.25;
   	const double gamma = 0.5;
   	const double coeff1 = 1/(beta*del_t*del_t);
   	const double coeff2 = 1/(beta*del_t);
   	const double coeff3 = (1-2*beta)/(2*beta);
   	const double coeff4 = (1-gamma)*del_t;
   	const double coeff5 = gamma*del_t;

   	double * Mb = new double[N]();
   	double * Keff = new double[lda*N]();
   	double * F = new double[N]();
   	double * u0 = new double[N]();
   	double * u1 = new double[N]();
   	double * udot0 = new double[N]();
    double * udot1 = new double[N]();
   	double * udotdot0 = new double[N]();
   	double * udotdot1 = new double[N]();
    double * S = new double[N]();

    Build_M_global_banded(Mb, rho, A, l, Nx, N);
    Print_Matrix(Mb, N, 1);
   	Build_Keff(Keff, Mb, coeff1, A, E, l, L, I, Nx, N, kl, ku, lda, kl);

    for (int i = 0; i<Nt; ++i)
    {
        Build_F_global(F, Fy, qx, qy, (double)(i+1)*del_t/T, l, Nx, N);
        Build_Multiplier2(S, Mb, F, u0, udot0, udotdot0, coeff1, coeff2, coeff3, N);
        Banded_Matrix_Solver(Keff, S, N, lda, kl, ku);
        F77NAME(dcopy) (N, S, 1, u1, 1);
        Build_udotdot(udotdot1, u1, u0, udot0, udotdot0, coeff1, coeff2, coeff3, N);
        Build_udot(udot1, udot0, udotdot0, udotdot1, coeff4, coeff5, N);
        F77NAME(dcopy) (N, u1, 1, u0, 1);
        F77NAME(dcopy) (N, udot1, 1, udot0, 1);
        F77NAME(dcopy) (N, udotdot1, 1, udotdot0, 1);
    }
    Write_Vector(u0, N, l, L, "Task1");

   	delete[] Keff;
   	delete[] u0;
   	delete[] u1;
   	delete[] udot0;
    delete[] udot1;
   	delete[] udotdot0;
   	delete[] udotdot1;
    delete[] S;
}

void Dynamic_Solver_MPI_1(double A, double E, double I, double L, double l, double qx, double qy, double Fy, double rho, double T, int Nt, int Nx, int ku, int kl, int N, int size, int rank)
{
	const int lda = 1 + kl + ku;
	const double del_t = T/(double)Nt;
	const int Nxloc = Nx/2 + 2;
	const int Nloc = (Nxloc-1)*3 ;

	double * Mbloc = new double[Nloc]();
	double * u0loc = new double[Nloc]();
  double * u1loc = new double[Nloc]();
	double * Kbloc = new double[Nloc*lda]();
	double * Floc = new double[Nloc]();

  for (int i = 0; i<Nloc; ++i)
  {
    u0loc[i] = 1.0;
  }

  Build_M_global_banded_MPI(Mbloc, rho, A, l, Nx, Nxloc, Nloc, size, rank);
  Build_K_global_banded_altered_MPI(Kbloc, Mbloc, A, E, l, L, I, del_t, Nx, Nxloc, N, Nloc, kl, ku, lda, 0, size, rank);

  F77NAME(dgbmv) ('N', N, N, kl, ku, 1.0, Kbloc, lda, u1, 1, 0.0, temp, 1);

  MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
    {
      double * Mb = new double[N]();
      double * u0 = new double[N]();
      for (int i = 0; i<N; ++i)
      {
        u0[i] = 1.0;
      }
      double * Kb = new double[lda*N]();
      Build_M_global_banded(Mb, rho, A, l, Nx, N);
      Build_K_global_banded_altered(Kb, Mb, A, E, l, L, I, del_t, Nx, N, kl, ku, lda, 0);
   	}

	// delete[] Floc;
}