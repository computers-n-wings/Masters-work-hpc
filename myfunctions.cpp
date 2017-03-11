#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#define F77NAME(x) x##_
extern "C" 
{
	void F77NAME(dgesv)(const int& n, const int& nrhs, const double * A,
                    const int& lda, int * ipiv, double * B, const int& ldb,
                    int& info);
	void F77NAME(dgbsv)(const int& n, const int& kl, const int& ku, 
                    const int& nrhs, const double * A, const int& ldab, 
                    int * ipiv, double * B, const int& ldb, int* info);
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

void Print_Matrix(double A[], int n, int m) 
{
	for (int i = 0; i<m; ++i)
	{
		for (int j = 0; j<n; ++j)
		{
			cout << setprecision(3) << setw(15) << A[j*m+i];
		} 
		cout << endl;
	}
}

void Print_Vector(double b[], int n)
{
	for (int i = 0; i<n; ++i)
	{
		cout << setw(15) << b[i] << endl;
	}
}

void Zero_Vector(double a[], int N)
{
	for (int i = 0; i<N; ++i)
	{
		a[i] = 0.0;
	}
}

void Copy_Vector(double a[], double b[], int N)
{
	F77NAME(dcopy) (N, a, 1, b, 1);
}

void Build_K_elemental(double Ke[], int n, double A, double E, double l, double I)
{
	double K1 = A*E/l; 							
	double K2 = 12.0*E*I/(l*l*l); 				
	double K3 = 6.0*E*I/(l*l); 					
	double K4 = E*I/l;
	for (int i = 0; i<n; ++i)
	{
		for (int j = 0; j<n; ++j)
		{
			if (i <= j)
			{
				Ke[0*n+0] = K1;
				Ke[0*n+3] = -K1;
				Ke[1*n+1] = K2;
				Ke[1*n+2] = K3;
				Ke[1*n+4] = -K2;
				Ke[1*n+5] = K3;
				Ke[2*n+2] = 4.0*K4;
				Ke[2*n+4] = -K3;
				Ke[2*n+5] = 2.0*K4;
				Ke[3*n+3] = K1;
				Ke[4*n+4] = K2;
				Ke[4*n+5] = -K3;
				Ke[5*n+5] = 4.0*K4;
			}
			else
			{
				Ke[i*n+j] = Ke[j*n+i];
			}
		}
	}
}

void Build_K_global_banded(double Kb[], double A, double E, double l, double L, double I, int Nx, int N, int kl, int ku, int lda, int bfr)
{
	double * Ke = new double[6*6]();
	Build_K_elemental(Ke, 6, A, E, l, I);

	for (int k = 0; k < Nx; ++k)
		{
			if (k==0)
			{
				for (int i = 0; i<3; ++i)
				{
					int pnt1 = (3*k+i)*lda+bfr+ku;
					int pnt2 = (i+3)*6+(i+3);
					int bnd1 = 3-i;
					int bnd2 = i+1;

					Kb[pnt1] += Ke[pnt2];

					for (int j = 1; j<bnd1; ++j)
					{
						Kb[pnt1+j] += Ke[pnt2+j];
					}
					for (int l = 1; l < bnd2; ++l)
					{
						Kb[pnt1-l] += Ke[pnt2-l];
					}
				}
			}
			else if (k == Nx-1)
			{
				for (int i = 0; i<3; ++i)
				{
					int pnt1 = (3*(k-1)+i)*lda+bfr+ku;
					int pnt2 = i*6+i;
					int bnd1 = 3-i;
					int bnd2 = i+1;

					Kb[pnt1] += Ke[pnt2];

					for (int j = 1; j<bnd1; ++j)
					{
						Kb[pnt1+j] += Ke[pnt2+j];
					}
					for (int l = 1; l < bnd2; ++l)
					{
						Kb[pnt1-l] += Ke[pnt2-l];
					}
				}
			}
			else
			{
				for (int i = 0; i<6; ++i)
				{	
					int pnt1 = (3*(k-1)+i)*lda+bfr+ku;
					int pnt2 = i*6+i;
					int bnd1 = 6-i;
					int bnd2 = i+1;
					
					Kb[pnt1] += Ke[pnt2];

					for (int j = 1; j<bnd1; ++j)
					{
						Kb[pnt1+j] += Ke[pnt2+j];
					}
					for (int l = 1; l < bnd2; ++l)
					{
						Kb[pnt1-l] += Ke[pnt2-l];
					}

				}
			}
		}
		delete[] Ke;
}

void Build_F_elemental(double Fe[], double qx, double qy, double l, double time)
{
	Zero_Vector(Fe, 6);

	const double F1 = time*qx*l/2.0; 							
	const double F2 = time*qy*l/2.0; 							
	const double F3 = time*qy*l*l/12.0; 	
	Fe[0] = F1;
	Fe[1] = F2;
	Fe[2] = F3;
	Fe[3] = F1;
	Fe[4] = F2;
	Fe[5] = -F3;
}

void Build_F_global(double F[], double Fy, double qx, double qy, double time, double l, int Nx, int N)
{
	Zero_Vector(F, N);

	double * Fe = new double[6]();
	Build_F_elemental(Fe, qx, qy, l, time);

	for (int k = 0; k<Nx; ++k)
	{
		if (k == 0)
		{
			F[0] = Fe[3];
			F[1] = Fe[4];
			F[2] = Fe[5];
		}
		else if (k == Nx-1)
		{
			for (int i = 0; i<3; ++i)
			{
				F[3*(k-1)+i] += Fe[i];
			}
		}
		else
		{
			for (int i = 0; i<6; ++i)
			{
				F[3*(k-1) +i] += Fe[i];
			}
		}
		if (k == Nx/2)
		{
			F[3*(k-1) +1] += time*Fy;
		}
	}

	delete[] Fe;
}

void Build_M_elemental(double Me[], int Ne, double rho, double A, double l)
{
	double alpha = 0.0416667;
	double M1 = 0.5*rho*A*l;
   	double M2 = rho*A*alpha*l*l*l;

	Me[0] = M1;
	Me[Ne + 1] = M1;
	Me[2*Ne + 2] = M2;
	Me[3*Ne + 3] = M1;
	Me[4*Ne + 4] = M1;
	Me[5*Ne + 5] = M2;
}

void Build_M_global_banded(double Mb[], double rho, double A, double l, int Nx, int N)
{
	double * Me = new double[6*6]();
	Build_M_elemental(Me, 6, rho, A, l);

	for (int k = 0; k<Nx; ++k)
	{
		if (k == 0)
		{
			for (int i = 0; i<3; ++i)
			{
				Mb[i] += Me[(i+3)*6+(i+3)];
			}
		}
		else if (k == Nx-1)
		{
			for (int i = 0; i<3; ++i)
			{
				Mb[3*(k-1)+i] += Me[i*6+i];
			}
		}
		else
		{
			for (int i = 0; i<6; ++i)
			{
				Mb[3*(k-1) +i] += Me[i*6+i];
			}
		}
	}

	delete[] Me;
}

void Matrix_System_Solver(double A[], double b[], int N)
{
	double * A_temp = new double[N*N]();
    Copy_Vector(A, A_temp, N*N);
    
	const int nrhs = 1;
    int info = 0;
    int * ipiv = new int[N];

    F77NAME(dgesv) (N, nrhs, A_temp, N, ipiv, b, N, info);
    
    delete[] A_temp;
}

void Banded_Matrix_Solver(double Ab[], double b[], int N, int lda, int kl, int ku)
{
	double * Ab_temp = new double[lda*N]();
	Copy_Vector(Ab, Ab_temp, lda*N);

	const int nrhs = 1;
	int info = 0;
	int * ipiv = new int[N];

	F77NAME(dgbsv) (N, kl, ku, nrhs, Ab_temp, lda, ipiv, b, N, &info);

	delete[] Ab_temp;
}

void Write_Vector(double F[], int N, double l, double L, string mystring)
{
	ofstream File;
	File.open(mystring + ".txt");

	if (File.good())
	{
		File << 0 << ' ' << setw(5) << setprecision(5) << 0 << '\n';
		for (int i = 0; i < N/3; ++i)
		{
			File << l*(i+1) << ' ' << setw(5) << setprecision(5) << F[3*i+1] << '\n';
		}
		File << L << ' ' << setw(5) << setprecision(5) << 0;
	}
	else
	{
		cout << "The file is not good." << endl;
	}

	File.close();
}

void Write_Point_Displacement(double F[], int N, string mystring)
{
	ofstream File;
	File.open(mystring + ".txt");

	if (File.good())
	{
		for (int i = 0; i < N; ++i)
		{
			File << i << ' ' << setw(5) << setprecision(5) << F[i] << '\n';
		}
	}
	else
	{
		cout << "The file is not good." << endl;
	}

	File.close();
}

void Build_Fn(double F[], double Fn[], double del_t, int N)
{
	Zero_Vector(Fn, N);
	Copy_Vector(F, Fn, N);

	F77NAME(dscal) (N, del_t*del_t, Fn, 1);
}

void Build_Un1_Multiplier(double Un1[], double Kb[], double Mb[], double u1[], int N, int lda, double del_t, int kl, int ku)
{
	Zero_Vector(Un1, N);

	double * K_temp = new double[lda*N]();
	Copy_Vector(Kb, K_temp, lda*N);

	F77NAME(dscal) (lda*N, del_t*del_t, K_temp, 1);

	for (int i = 0; i < N; ++i)
	{
		int pnt1 = i*lda+ku;
		K_temp[pnt1] += -2.0*Mb[i];
	}

	F77NAME(dgbmv) ('N', N, N, kl, ku, 1.0, K_temp, lda, u1, 1, 0.0, Un1, 1);

	delete[] K_temp;
}

void Build_Un0_Multiplier(double Un0[], double Mb[], double u0[], int N)
{
	Zero_Vector(Un0, N);

	double * M_temp = new double[N]();
	Copy_Vector(Mb, M_temp, N);

	F77NAME(dgbmv) ('N', N, N, 0, 0, 1.0, M_temp, 1, u0, 1, 0.0, Un0, 1);

	delete[] M_temp;
}

void Build_Multiplier1(double S[], double F[], double Kb[], double Mb[], double u0[], double u1[], double del_t, int N, int lda, int kl, int ku)
{
	Zero_Vector(S, N);

	double * Fn = new double[N]();
	double * Un0 = new double[N]();
	double * Un1 = new double[N]();
	
	Build_Fn(F, Fn, del_t, N);
	Build_Un1_Multiplier(Un1, Kb, Mb, u1, N, lda, del_t, kl, ku);
	Build_Un0_Multiplier(Un0, Mb, u0, N);
	
	F77NAME(daxpy) (N, 1.0, Fn, 1, S, 1);
	F77NAME(daxpy) (N, -1.0, Un1, 1, S, 1);
	F77NAME(daxpy) (N, -1.0, Un0, 1, S, 1);

	delete[] Fn;
	delete[] Un1;
   	delete[] Un0;
}

double RMS_error(double u1[], double S[], double M[], int N)
{
	double * res = new double[N]();
	double * M_temp = new double[N]();
	Copy_Vector(M, M_temp, N);

	F77NAME(dgbmv) ('N', N, N, 0, 0, 1.0, M_temp, 1, u1, 1, 0.0, res, 1);
	F77NAME(daxpy) (N, -1.0, S, 1, res, 1);
	double residual = F77NAME(dnrm2)(N, res, 1);

	delete[] M_temp;
	delete[] res;
	return residual;
}

void Build_K_eff(double Kb[], double Mb[], double del_t, double beta, int N, int ku, int kl, int lda)
{
	for (int i = 0; i < N; ++i)
	{
		int pnt1 = i*lda+ku+kl;
		Kb[pnt1] += (1.0/(beta*del_t*del_t))*Mb[i];
	}
}

void Build_Multiplier2(double S[], double F[], double Mb[], double u0[], double udot[], double uddot0[], double del_t, double beta, int N)
{
	Zero_Vector(S, N);

	double * temp = new double[N]();
	double * M_temp = new double[N]();

	Copy_Vector(u0, temp, N);
	Copy_Vector(Mb, M_temp, N);

	F77NAME(dscal) (N, 1.0/(beta*del_t*del_t), temp, 1);
	F77NAME(daxpy) (N, 1.0/(beta*del_t), udot, 1, temp, 1);
	F77NAME(daxpy) (N, ((1.0-2.0*beta)/(2.0*beta)), uddot0, 1, temp, 1);
	F77NAME(dgbmv) ('N', N, N, 0, 0, 1.0, M_temp, 1, temp, 1, 0.0, S, 1);
	F77NAME(daxpy) (N, 1.0, F, 1, S, 1);

	delete[] temp;
	delete[] M_temp;
}

void Build_uddot(double uddot1[], double uddot0[], double u1[], double u0[], double udot[], double beta, double del_t, int N)
{
	Copy_Vector(uddot0, uddot1, N);
	F77NAME(dscal) (N, -((1.0-(2.0*beta))/(2.0*beta)), uddot1, 1);
	F77NAME(daxpy) (N, -1.0/(beta*del_t), udot, 1, uddot1, 1);
	F77NAME(daxpy) (N, 1.0/(beta*del_t*del_t), u1, 1, uddot1, 1);
	F77NAME(daxpy) (N, -1.0/(beta*del_t*del_t), u0, 1, uddot1, 1);
}

void Build_udot(double udot[], double u0[], double uddot0[] , double uddot1[], double del_t, double gamma, int N)
{
	Zero_Vector(udot, N);

	F77NAME(daxpy) (N, 1.0, u0, 1, udot, 1);
	F77NAME(daxpy) (N, del_t*(1.0-gamma), uddot0, 1, udot, 1);
	F77NAME(daxpy) (N, del_t*gamma, uddot1, 1, udot, 1);
}
