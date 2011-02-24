#ifndef _SYSPARAMS_H
#define _SYSPARAMS_H
#include "sysparams.hh"
#endif

#include <iostream>

// Implementation of SysParams functionality

// Constructor
// Debye and Bjerrum length in [nm], colloid charge in [e], radius in Bjerrum length
SysParams::SysParams(double _lambda=0.1, double _bjerrum=0.1, double _charge_density=0.1, double _epsilon=80.0, double _radius=1.0)
    :lambda(_lambda),bjerrum(_bjerrum),charge_density(_charge_density),epsilon(_epsilon),radius(_radius)
{
	lambda2i = 1 / (lambda * lambda);
	totalError = 1E8;
}

double SysParams::get_radius()
{
	return radius;
}

void SysParams::add_error(double error)
{
	//double error = 2.0 * (inValue - oldValue) / (inValue + oldValue);
	//std::cout << "aktueller Error: " << error;
	if (error > totalError)
	{
	  totalError = error;
	  // oldValue = inValue;
	}
}

void SysParams::set_alpha(double alpha_in)
{
	alpha=alpha_in;
	std::cout << " SOR alpha = " << alpha;
}

double SysParams::get_alpha()
{
	return alpha;
}

void SysParams::reset_error()
{
	totalError = 0;
}

double SysParams::get_error()
{
		return totalError;
}

double SysParams::get_epsilon()
{
	return epsilon;
}

double SysParams::get_charge()
{
	return charge;
}

double SysParams::get_bjerrum()
{
	return bjerrum;
}

double SysParams::get_lambda2i()
{
	return lambda2i;
}


double SysParams::get_E_init()
{
	return E_init;
}

double SysParams::get_charge_density()
{
	return charge_density;
}

void SysParams::set_lambda(double value)
{
	lambda = value;
	lambda2i = 1 / (lambda * lambda);
}

void SysParams::set_E_init(double value)
{
	E_init = value;
}



SysParams sysParams;
