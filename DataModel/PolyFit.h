/*
 * Function fitting for LED profile
 * the funcition is the averaged LED profile L[x], it is a pair of vectors (v_xWl, v_yInt)
 * It is used to fit data with function f(x) = a+b*L[x]
 */

#ifndef PolyFit_H
#define PolyFit_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cmath>

#include "Eigen/Dense"

class PolyFit
{
	public:
		PolyFit(std::vector<double> ax, std::vector<double> &dt, int n);
		double ff(double x);
		double fe(double x);
		double fd(double x);
  double Root(double x0, double y);
		Eigen::VectorXd Residual();
		Eigen::VectorXd Value();
		Eigen::MatrixXd Weight();
		Eigen::MatrixXd Jacobian();
		Eigen::VectorXd Delta();
		Eigen::VectorXd Beta0();
		Eigen::MatrixXd VarBeta();
		std::vector<double> Solve(bool verbose = false);
		std::vector<double> LeastSquare(bool verbose = false);
		void Print(std::vector<double> &r);
		void Reset();

	private:
		int order;
		std::vector<double> axis;
		std::vector<double> data;
		int size;
		Eigen::VectorXd beta;
		bool weighted;
};

#endif
