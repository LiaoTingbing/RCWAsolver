﻿
// #pragma once
#include "../include/rcwa.h"

RCWA::RCWA()
{
}


RCWA::RCWA(DataRCWA& In)
{
	Index_ = In.Index;
	layer_pos_ = In.LayerPos;
	x_pos_ = In.x;
	y_pos_ = In.y;

	z_pos_ = In.z;
	ku_ = In.ku;
	kv_ = In.kv;
	lambda_ = In.lambda;
	theta_ = In.theta;
	phi_angle_ = In.phi;

	n_lower_value = In.n_lower;
	n_upper_value = In.n_upper;;
}

RCWA::~RCWA()
{
	 
}

void RCWA::Run()
{
	cout << " 初始化\n";
	clock_t t1, t2, t3, t4;
	t1 = clock();

	size_t Nx = x_pos_.size();
	size_t Ny = y_pos_.size();
	size_t Ncx = x_pos_.size() - 1;
	size_t Ncy = y_pos_.size() - 1;
	size_t Nc = Ncx * Ncy;

	double Lx = x_pos_(Nx - 1) - x_pos_(0);
	double Ly = y_pos_(Ny - 1) - y_pos_(0);
	double thetaArc = theta_ * pi / 180.0;
	double phiArc = phi_angle_ * pi / 180.0;
	vec incident_direction = {
		sin(thetaArc) * cos(phiArc),
		sin(thetaArc) * sin(phiArc),
		cos(thetaArc)
	};
	double k0 = 2 * pi / lambda_;

	cout << "\t 计算入射波矢:";
	cout << "\t lambda=" << to_string(lambda_ * 1e6);
	cout << "\t theta=" << to_string(theta_);
	cout << "\t phi=" << to_string(phi_angle_);
	cout << endl;

	double ER_inc = n_lower_value * n_lower_value;
	double UR_inc = 1;
	vec k_inc = sqrt(ER_inc) * incident_direction;
	double kx_inc = k_inc(0);
	double ky_inc = k_inc(1);
	double kz_inc = k_inc(2);

	cout << "\t 计算周期矢量";
	cout << "\t Lx = " << to_string(Lx*1e6);
	cout << "\t\t Ly = " << to_string(Ly*1e6);
	cout << endl;

	double Tx = 2.0 * pi / Lx / k0;
	double Ty = 2.0 * pi / Ly / k0;

	cout << "\t 计算谐波展开:";
	cout << "\t ku=" << to_string(ku_);
	cout << "\t\t kv=" << to_string(kv_);
	cout << endl;

	vec m = linspace(-ku_, ku_, 2 * ku_ + 1);
	vec n = linspace(-kv_, kv_, 2 * kv_ + 1);

	const size_t M = m.size();
	const size_t N = n.size();
	const size_t Nh = M * N;


	cx_mat kx_mn(M, N);
	cx_mat ky_mn(M, N);

	for (size_t i = 0; i < M; i++)
	{
		for (size_t j = 0; j < N; j++)
		{
			kx_mn(i, j) = kx_inc - m(i) * Tx;
			ky_mn(i, j) = ky_inc - n(j) * Ty;
		}
	}

	//real(kx_mn).print();
	//cout << endl;
	//real(ky_mn).print();
	//kx_mn.reshape(Nh, 1).print();
	cx_mat Kx = diagmat(kx_mn.reshape(Nh, 1));        //对角矩阵
	cx_mat Ky = diagmat(ky_mn.reshape(Nh, 1));			//对角矩阵


	//diagvec(Kx).print();

	//cout << "\t初始化散射矩阵\n";
	const cx_mat I(Nh, Nh, fill::eye);
	const cx_mat II(2 * Nh, 2 * Nh, fill::eye);
	const cx_mat Z(Nh, Nh);
	const cx_mat ZZ(Nh * 2, Nh * 2);

	Smatrix G{ZZ,II,II,ZZ};

	//G.S11 = ZZ;
	//G.S12 = II;
	//G.S21 = II;
	//G.S22 = ZZ;

	// -------------------------------------------------------GAP-------

	cout << " 计算GAP区\n";
	cx_mat Kz = conj(sqrt(I - Kx * Kx - Ky * Ky));
	cx_mat Q = MatrixConnect(Kx * Ky, I - Kx * Kx, Ky * Ky - I, -Kx * Ky);
	cx_mat W0 = MatrixConnect(I, Z, Z, I);
	cx_mat LAM = MatrixConnect(iI * Kz, Z, Z, iI * Kz);
	cx_mat V0 = Q * inv(LAM);

	//V0.diag().print();

	//V0.print();
	//---------------------------------------------------------反射---------
	cout << " 计算反射区\n";
	double  ER_ref = ER_inc;
	double  UR_ref = UR_inc;

	cx_mat Kz_ref = -conj(sqrt(conj(UR_ref) * conj(ER_ref) * I - Kx * Kx - Ky * Ky));

	cx_mat Qref = 1.0 / UR_ref * MatrixConnect(
		Kx * Ky, UR_ref * ER_ref * I - Kx * Kx,
		Ky * Ky - UR_ref * ER_ref * I, -Ky * Kx);
	cx_mat Wref = MatrixConnect(I, Z, Z, I);
	cx_mat LAMref = MatrixConnect(-1i * Kz_ref, Z, Z, -1i * Kz_ref);
	cx_mat Vref = Qref * inv(LAMref);
	//Vref.diag().print();

	/*cx_mat Aref = inv(W0) * Wref + inv(V0) * Vref;
	cx_mat Bref = inv(W0) * Wref - inv(V0) * Vref;*/

	cx_mat Aref = solve(W0,Wref) + solve(V0,Vref);
	cx_mat Bref = solve(W0, Wref) - solve(V0, Vref);

	Smatrix SR;

	SR.S11 = -inv(Aref) * Bref;
	SR.S12 = 2.0 * inv(Aref);
	SR.S21 = 0.5 * (Aref - Bref * inv(Aref) * Bref);
	SR.S22 = Bref * inv(Aref);

	G = SconnectRight(G, SR);

	//G.S12.diag().print();

	//---------------------------------------------------器件
	cout << " 计算器件区\n";

	vec ZL = diff(z_pos_);
	double di;
	cx_mat ERi, URi(Nx, Ny, fill::ones);
	cx_mat ERC, URC;

	cx_mat Indexi, Wi, Vi, Ai, Bi, Xi, LAMi, Pi, Qi, Oi;
	cx_vec LAMi2;
	Smatrix D;
	for (size_t Layer = 0; Layer < ZL.size();Layer++)
	{
		cout << "\t 第" << Layer + 1 << "层: ";
		t2 = clock();
		  
		di = ZL(Layer);

		Indexi = conj(Index_[Layer]);

		ERi = pow(Indexi, 2.0);

		ERC = Convulation_Matrix(ERi, m, n);
		//ERC.print();
		//ERC.col(0).print();
		//URi.diag().print();

		URC = Convulation_Matrix(URi, m, n);

		Pi = MatrixConnect(
			Kx * inv(ERC) * Ky, URC - Kx * inv(ERC) * Kx,
			Ky * inv(ERC) * Ky - URC, -Ky * inv(ERC) * Kx);
		Qi = MatrixConnect(
			Kx * inv(URC) * Ky, ERC - Kx * inv(URC) * Kx,
			Ky * inv(URC) * Ky - ERC, -Ky * inv(URC) * Kx);
		Oi = Pi * Qi;

		//[Wi, LAMi2] = eig(Oi);
		eig_gen(LAMi2, Wi, Oi);
		LAMi = diagmat(sqrt(LAMi2));

		Vi = Qi * Wi * inv(LAMi);

		Ai = inv(Wi) * W0 + inv(Vi) * V0;
		Bi = inv(Wi) * W0 - inv(Vi) * V0;
		Xi = expmat(-LAMi * k0 * di);
		//real(Ai).print();


		D.S11 = solve(Ai - Xi * Bi * inv(Ai) * Xi * Bi,
			Xi * Bi * inv(Ai) * Xi * Ai - Bi);
		D.S12 = solve(Ai - Xi * Bi * inv(Ai) * Xi * Bi, Xi)
			* (Ai - Bi * inv(Ai) * Bi);
		D.S21 = D.S12;
		D.S22 = D.S11;
		G = SconnectRight(G, D);

		//G.S12.diag().print();

		t3 = clock();
		cout << "\t 耗时: " << double(t3 - t2) / CLOCKS_PER_SEC << "s\n";
	}

	//G.S12.diag().print();

	// --------------------------------计算透射区

	cout << " 计算透射区\n";
	double ER_trn = n_upper_value * n_upper_value;
	double UR_trn = 1;
	cx_mat Kz_trn = conj(sqrt(conj(UR_trn) * conj(ER_trn) * I - Kx * Kx - Ky * Ky));

	cx_mat 	Qtrn = 1.0 / UR_trn * MatrixConnect(
		Kx * Ky, UR_trn * ER_trn * I - Kx * Kx,
		Ky * Ky - UR_trn * ER_trn * I, -Ky * Kx);
	cx_mat Wtrn = MatrixConnect(I, Z, Z, I);
	cx_mat LAMtrn = MatrixConnect(1i * Kz_trn, Z, Z, 1i * Kz_trn);
	cx_mat Vtrn = Qtrn * inv(LAMtrn);

	cx_mat Atrn = inv(W0) * Wtrn + inv(V0) * Vtrn;
	cx_mat Btrn = inv(W0) * Wtrn - inv(V0) * Vtrn;

	Smatrix ST;
	ST.S11 = Btrn * inv(Atrn);
	ST.S12 = 0.5 * (Atrn - Btrn * inv(Atrn) * Btrn);
	ST.S21 = 2 * inv(Atrn);
	ST.S22 = -inv(Atrn) * Btrn;

	G = SconnectRight(G, ST);

	//G.S12.diag().print();


	//  -----------------------------------计算透射谱

	vec s{ sin(phiArc) , -cos(phiArc) , 0 };
	vec p{ cos(thetaArc) * cos(phiArc),
			cos(thetaArc) * sin(phiArc),
			-sin(thetaArc) };
	mat SP = join_rows(s, p);
	//SP.print();
 
	for (size_t SP_index = 0; SP_index < 2; SP_index++)
	{
		vec Pol = SP.col(SP_index);

		double px = Pol(0);
		double py = Pol(1);
		double pz = Pol(2);

		vec del(Nh);
		size_t pq = Nh / 2 + 1;
		del(pq - 1) = 1;

		vec StInc = join_cols(px * del, py * del);
		cx_vec Cinc = inv(Wref) * StInc;
		cx_vec Cref = G.S11 * Cinc;
		cx_vec Ctrn = G.S21 * Cinc;

		cx_vec rT = Wref * G.S11 * Cinc;
		cx_vec tT = Wtrn * G.S21 * Cinc;

		cx_vec rX = rT.rows(0, Nh - 1);
		//rX.print();
		cx_vec rY = rT.rows(Nh, rT.n_rows - 1);

		cx_vec tX = tT.rows(0, Nh - 1);
		cx_vec tY = tT.rows(Nh, tT.n_rows - 1);

		////%% 纵向分量

		cx_vec rZ = -inv(Kz_ref) * (Kx * rX + Ky * rY);
		cx_vec tZ = -inv(Kz_trn) * (Kx * tX + Ky * tY);

		////%% 计算衍射系数
		vec r2 = real(rX % conj(rX) + rY % conj(rY) + rZ % conj(rZ));
		vec t2 = real(tX % conj(tX) + tY % conj(tY) + tZ % conj(tZ));


		vec  R = real(-Kz_ref / UR_ref) / real(kz_inc / UR_ref) * r2;
		vec T = real(Kz_trn / UR_trn) / real(kz_inc / UR_inc) * t2;

		if (SP_index == 0)
		{
			Rs = sum(R);
			Ts = sum(T);
		}

		if (SP_index == 1)
		{
			Rp = sum(R);
			Tp = sum(T);
		}
	}

	t4 = clock();
	cout << "\n 时间消耗 : " << double(t4 - t1) / CLOCKS_PER_SEC << "s" << endl;
	cout << " 结果：" << endl;
	cout << " Rp" << "+" << "Tp" << "=" << Rp << "+" << Tp << "=" << Rp + Tp << endl;
	cout << " Rs" << "+" << "Ts" << "=" << Rs << "+" << Ts << "=" << Rs + Ts << endl;

}

double RCWA::getRs()
{
	return Rs;
}

double RCWA::getRp()
{
	return Rp;
}

double RCWA::getTs()
{
	return Ts;
}

double RCWA::getTp()
{
	return Tp;
}

void RCWA::set_lambda(double In)
{
	lambda_ = In;
}

void RCWA::set_theta(double In)
{
	theta_ = In;
}

void RCWA::set_phi(double In)
{
	phi_angle_ = In;
}

void RCWA::set_n_lower(double In)
{
	n_lower_value = In;
}

void RCWA::set_n_upper(double In)
{
	n_upper_value = In;
}

void RCWA::set_Index(vector<cx_mat> In)
{
	Index_ = In;
}


