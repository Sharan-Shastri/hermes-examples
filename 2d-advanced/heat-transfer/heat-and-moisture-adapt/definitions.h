#include "hermes2d.h"

using namespace Hermes;
using namespace Hermes::Hermes2D;
using namespace Hermes::Hermes2D::WeakFormsH1;
using namespace Hermes::Hermes2D::Views;
using namespace Hermes::Hermes2D::RefinementSelectors;

/* Weak forms */

class CustomWeakFormHeatMoistureRK : public WeakForm<double>
{
public:
  CustomWeakFormHeatMoistureRK(double c_TT, double c_ww, double d_TT, double d_Tw, double d_wT, double d_ww, 
      double k_TT, double k_ww, double T_ext, double w_ext, const std::string bdy_ext);
};

/* Time-dependent Dirichlet condition for temperature */

class EssentialBCNonConst : public EssentialBoundaryCondition<double>
{
public:
  EssentialBCNonConst(std::string marker, double reactor_start_time, double temp_initial, double temp_reactor_max);
  
  ~EssentialBCNonConst() {};

  virtual EssentialBCValueType get_value_type() const;

  virtual double value(double x, double y) const;

protected:
  double reactor_start_time;
  double temp_initial;
  double temp_reactor_max;
};

/* Custom error forms */

class CustomErrorForm : public NormFormVol<double>
{
public:
  CustomErrorForm(int i, int j, double d, double c) : NormFormVol<double>(i, j, SolutionsDifference), d(d), c(c) {};

  virtual double value(int n, double *wt, Func<double> *u, Func<double> *v, GeomVol<double> *e) const
  {
    return d / c * int_grad_u_grad_v<double, double>(n, wt, u, v);
  }

  double d, c;
};

