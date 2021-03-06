#include "hermes2d.h"

/* Namespaces used */

using namespace Hermes;
using namespace Hermes::Hermes2D;
using namespace Hermes::Hermes2D::Views;
using namespace Hermes::Hermes2D::RefinementSelectors;

class WeakFormNSNewton : public WeakForm < double >
{
public:
  WeakFormNSNewton(bool Stokes, double Reynolds, double time_step, MeshFunctionSharedPtr<double>  x_vel_previous_time,
    MeshFunctionSharedPtr<double>  y_vel_previous_time);

  class BilinearFormSymVel : public MatrixFormVol < double >
  {
  public:
    BilinearFormSymVel(unsigned int i, unsigned int j, bool Stokes, double Reynolds, double time_step)
      : MatrixFormVol<double>(i, j), Stokes(Stokes),
      Reynolds(Reynolds), time_step(time_step) {
      this->setSymFlag(HERMES_SYM);
    };

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *u, Func<double> *v,
      GeomVol<double> *e, Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *u, Func<Ord> *v, GeomVol<Ord> *e,
      Func<Ord>* *ext) const;

    MatrixFormVol<double>* clone() const;
  protected:
    bool Stokes;
    double Reynolds;
    double time_step;
  };

  class BilinearFormNonsymVel_0_0 : public MatrixFormVol < double >
  {
  public:
    BilinearFormNonsymVel_0_0(unsigned int i, unsigned int j, bool Stokes)
      : MatrixFormVol<double>(i, j), Stokes(Stokes) {};

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *u, Func<double> *v,
      GeomVol<double> *e, Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *u, Func<Ord> *v, GeomVol<Ord> *e,
      Func<Ord>* *ext) const;
    MatrixFormVol<double>* clone() const;

  protected:
    bool Stokes;
  };

  class BilinearFormNonsymVel_0_1 : public MatrixFormVol < double >
  {
  public:
    BilinearFormNonsymVel_0_1(unsigned int i, unsigned int j, bool Stokes)
      : MatrixFormVol<double>(i, j), Stokes(Stokes) {};

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *u, Func<double> *v,
      GeomVol<double> *e, Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *u, Func<Ord> *v, GeomVol<Ord> *e,
      Func<Ord>* *ext) const;
    MatrixFormVol<double>* clone() const;

  protected:
    bool Stokes;
  };

  class BilinearFormNonsymVel_1_0 : public MatrixFormVol < double >
  {
  public:
    BilinearFormNonsymVel_1_0(unsigned int i, unsigned int j, bool Stokes)
      : MatrixFormVol<double>(i, j), Stokes(Stokes) {};

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *u, Func<double> *v,
      GeomVol<double> *e, Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *u, Func<Ord> *v, GeomVol<Ord> *e,
      Func<Ord>* *ext) const;
    MatrixFormVol<double>* clone() const;

  protected:
    bool Stokes;
  };

  class BilinearFormNonsymVel_1_1 : public MatrixFormVol < double >
  {
  public:
    BilinearFormNonsymVel_1_1(unsigned int i, unsigned int j, bool Stokes)
      : MatrixFormVol<double>(i, j), Stokes(Stokes) {};

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *u, Func<double> *v,
      GeomVol<double> *e, Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *u, Func<Ord> *v, GeomVol<Ord> *e,
      Func<Ord>* *ext) const;

    MatrixFormVol<double>* clone() const;
  protected:
    bool Stokes;
  };

  class BilinearFormNonsymXVelPressure : public MatrixFormVol < double >
  {
  public:
    BilinearFormNonsymXVelPressure(int i, int j) : MatrixFormVol<double>(i, j) { this->setSymFlag(HERMES_ANTISYM); };

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *u, Func<double> *v,
      GeomVol<double> *e, Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *u, Func<Ord> *v, GeomVol<Ord> *e,
      Func<Ord>* *ext) const;
    MatrixFormVol<double>* clone() const;
  };

  class BilinearFormNonsymYVelPressure : public MatrixFormVol < double >
  {
  public:
    BilinearFormNonsymYVelPressure(int i, int j) : MatrixFormVol<double>(i, j) { this->setSymFlag(HERMES_ANTISYM); };

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *u, Func<double> *v,
      GeomVol<double> *e, Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *u, Func<Ord> *v, GeomVol<Ord> *e,
      Func<Ord>* *ext) const;
    MatrixFormVol<double>* clone() const;
  };

  class VectorFormNS_0 : public VectorFormVol < double >
  {
  public:
    VectorFormNS_0(int i, bool Stokes, double Reynolds, double time_step) : VectorFormVol<double>(i),
      Stokes(Stokes), Reynolds(Reynolds), time_step(time_step) {};

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *v, GeomVol<double> *e,
      Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *v, GeomVol<Ord> *e, Func<Ord>* *ext) const;

    VectorFormVol<double>* clone() const;
  protected:
    bool Stokes;
    double Reynolds;
    double time_step;
  };

  class VectorFormNS_1 : public VectorFormVol < double >
  {
  public:
    VectorFormNS_1(int i, bool Stokes, double Reynolds, double time_step)
      : VectorFormVol<double>(i), Stokes(Stokes), Reynolds(Reynolds), time_step(time_step) {};

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *v, GeomVol<double> *e,
      Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *v, GeomVol<Ord> *e, Func<Ord>* *ext) const;
    VectorFormVol<double>* clone() const;

  protected:
    bool Stokes;
    double Reynolds;
    double time_step;
  };

  class VectorFormNS_2 : public VectorFormVol < double >
  {
  public:
    VectorFormNS_2(int i) : VectorFormVol<double>(i) {};

    virtual double value(int n, double *wt, Func<double> *u_ext[], Func<double> *v, GeomVol<double> *e,
      Func<double>* *ext) const;

    virtual Ord ord(int n, double *wt, Func<Ord> *u_ext[], Func<Ord> *v, GeomVol<Ord> *e, Func<Ord>* *ext) const;
    VectorFormVol<double>* clone() const;
  };

protected:
  bool Stokes;
  double Reynolds;
  double time_step;
  MeshFunctionSharedPtr<double>  x_vel_previous_time;
  MeshFunctionSharedPtr<double>  y_vel_previous_time;
};

/* Essential boundary conditions */

// Time-dependent surface x-velocity of inner circle.
class EssentialBCNonConstX : public EssentialBoundaryCondition < double >
{
public:
  EssentialBCNonConstX(std::vector<std::string> markers, double vel, double startup_time)
    : EssentialBoundaryCondition<double>(markers), startup_time(startup_time), vel(vel)  {};

  EssentialBCNonConstX(std::string marker, double vel, double startup_time)
    : EssentialBoundaryCondition<double>(marker), startup_time(startup_time), vel(vel)  {};

  ~EssentialBCNonConstX() {};

  virtual EssentialBCValueType get_value_type() const;

  virtual double value(double x, double y) const;

protected:
  double startup_time;
  double vel;
};

// Time-dependent surface y-velocity of inner circle.
class EssentialBCNonConstY : public EssentialBoundaryCondition < double >
{
public:
  EssentialBCNonConstY(std::vector<std::string> markers, double vel, double startup_time)
    : EssentialBoundaryCondition<double>(markers), startup_time(startup_time), vel(vel)  {};

  EssentialBCNonConstY(std::string marker, double vel, double startup_time)
    : EssentialBoundaryCondition<double>(marker), startup_time(startup_time), vel(vel)  {};

  ~EssentialBCNonConstY() {};

  virtual EssentialBCValueType get_value_type() const;

  virtual double value(double x, double y) const;

protected:
  double startup_time;
  double vel;
};
