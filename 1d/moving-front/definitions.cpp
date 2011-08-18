#include "definitions.h"

double CustomExactSolution::value(double x, double y) const 
{
  double S = s;
  double C = c;
  double t = *t_ptr;
  return (x - x0) * (x - x1) * Hermes::atan(t) * (M_PI/2. - Hermes::atan(S*(x - t))) / C;
}

void CustomExactSolution::derivatives(double x, double y, double& dx, double& dy) const 
{
  double S = s;
  double C = c;
  double t = *t_ptr;
  dx = -S*(x - x0)*(x - x1)*Hermes::atan(t)/(C*(Hermes::pow(S, 2)*Hermes::pow(-t + x, 2) + 1)) + (x - x0)*(-Hermes::atan(S*(-t + x)) + M_PI/2)*Hermes::atan(t)/C + (x - x1)*(-Hermes::atan(S*(-t + x)) + M_PI/2)*Hermes::atan(t)/C;
  dy = 0;
}

Ord CustomExactSolution::ord(Ord x, Ord y) const 
{
  return Ord(20);
}

double CustomFunction::value(double x, double y, double t) const 
{
  double S = s;
  double C = c;

  // Generated by Sympy.
  double f = -Hermes::pow(S, 3)*(-2*t + 2*x)*(x - x0)*(x - x1)*Hermes::atan(t)/(C*Hermes::pow(Hermes::pow(S, 2)*Hermes::pow(-t + x, 2) + 1, 2)) + S*(x - x0)*(x - x1)*Hermes::atan(t)/(C*(Hermes::pow(S, 2)*Hermes::pow(-t + x, 2) + 1)) + 2*S*(x - x0)*Hermes::atan(t)/(C*(Hermes::pow(S, 2)*Hermes::pow(-t + x, 2) + 1)) + 2*S*(x - x1)*Hermes::atan(t)/(C*(Hermes::pow(S, 2)*Hermes::pow(-t + x, 2) + 1)) - 2*(-Hermes::atan(S*(-t + x)) + M_PI/2)*Hermes::atan(t)/C + (x - x0)*(x - x1)*(-Hermes::atan(S*(-t + x)) + M_PI/2)/(C*(Hermes::pow(t, 2) + 1));

  return -f;
}

Ord CustomFunction::value(Ord x, Ord y) const 
{
  return Ord(20);
}

CustomVectorFormVol::CustomVectorFormVol(int i, std::string area,
					 Hermes::Hermes2DFunction<double>* coeff,
  GeomType gt)
  : VectorFormVol<double>(i, area), coeff(coeff), gt(gt)
{
  // If coeff is HERMES_ONE, initialize it to be constant 1.0.
  if (coeff == HERMES_ONE) this->coeff = new Hermes::Hermes2DFunction<double>(1.0);
}

CustomVectorFormVol::CustomVectorFormVol(int i, Hermes::vector<std::string> areas,
					 Hermes::Hermes2DFunction<double>* coeff,
  GeomType gt)
  : VectorFormVol<double>(i, areas), coeff(coeff), gt(gt)
{
  // If coeff is HERMES_ONE, initialize it to be constant 1.0.
  if (coeff == HERMES_ONE) this->coeff = new Hermes::Hermes2DFunction<double>(1.0);
}

CustomVectorFormVol::~CustomVectorFormVol() 
{
  // FIXME: Should be deleted here only if it was created here.
  //if (coeff != HERMES_ONE) delete coeff;
};

double CustomVectorFormVol::value(int n, double *wt, Func<double> *u_ext[], Func<double> *v,
                                  Geom<double> *e, ExtData<double> *ext) const 
{
  double result = 0;
  if (gt == HERMES_PLANAR) {
    for (int i = 0; i < n; i++) {
      result += wt[i] * static_cast<CustomFunction*>(coeff)->value(e->x[i], e->y[i], this->get_current_stage_time()) * v->val[i];
    }
  }
  else {
    if (gt == HERMES_AXISYM_X) {
      for (int i = 0; i < n; i++) {
        result += wt[i] * e->y[i] * static_cast<CustomFunction*>(coeff)->value(e->x[i], e->y[i], this->get_current_stage_time()) * v->val[i];
      }
    }
    else {
      for (int i = 0; i < n; i++) {
        result += wt[i] * e->x[i] * static_cast<CustomFunction*>(coeff)->value(e->x[i], e->y[i], this->get_current_stage_time()) * v->val[i];
      }
    }
  }
  return result;
}

Ord CustomVectorFormVol::ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *v,
  Geom<Ord> *e, ExtData<Ord> *ext) const 
{
  Ord result = Ord(0);
  if (gt == HERMES_PLANAR) {
    for (int i = 0; i < n; i++) {
      result += wt[i] * coeff->value(e->x[i], e->y[i]) * v->val[i];
    }
  }
  else {
    if (gt == HERMES_AXISYM_X) {
      for (int i = 0; i < n; i++) {
        result += wt[i] * e->y[i] * coeff->value(e->x[i], e->y[i]) * v->val[i];
      }
    }
    else {
      for (int i = 0; i < n; i++) {
        result += wt[i] * e->x[i] * coeff->value(e->x[i], e->y[i]) * v->val[i];
      }
    }
  }

  return result;
}

VectorFormVol<double>* CustomVectorFormVol::clone() 
{
  return new CustomVectorFormVol(*this);
}

CustomWeakFormPoisson::CustomWeakFormPoisson(std::string area,
  Hermes::Hermes1DFunction<double>* coeff, Hermes::Hermes2DFunction<double>* f,
  GeomType gt) : WeakFormsH1::DefaultWeakFormPoisson<double>()
{
  // Jacobian.
  // NOTE: The flag HERMES_NONSYM is important here.
  add_matrix_form(new WeakFormsH1::DefaultJacobianDiffusion<double>(0, 0, area, coeff, HERMES_NONSYM, gt));

  // Residual.
  add_vector_form(new WeakFormsH1::DefaultResidualDiffusion<double>(0, area, coeff, gt));
  add_vector_form(new CustomVectorFormVol(0, area, f, gt));
};

ZeroInitialCondition::ZeroInitialCondition(Mesh* mesh) : ExactSolutionScalar<double>(mesh) 
{
}

double ZeroInitialCondition::value (double x, double y) const {
  return 0.0; 
}

void ZeroInitialCondition::derivatives (double x, double y, double& dx, double& dy) const {
  dx = 0;
  dy = 0;
}

Ord ZeroInitialCondition::ord(Ord x, Ord y) const {
  return Ord(0);
}