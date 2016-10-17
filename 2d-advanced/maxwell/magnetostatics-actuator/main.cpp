#include "definitions.h"

//  This example shows how to handle stiff nonlinear problems.
//
//  PDE: magnetostatics with nonlinear magnetic permeability
//  curl[1/mu curl u] = current_density.

//  The following parameters can be changed:

// Initial polynomial degree.
const int P_INIT = 3;
// Stopping criterion for the Newton's method.
const double NEWTON_TOL = 1e-8;
// Maximum allowed number of Newton iterations.
const int NEWTON_MAX_ITER = 1000;
// Number between 0 and 1 to damp Newton's iterations.
const double NEWTON_DAMPING = 1.0;
// Number of initial uniform mesh refinements.
const int INIT_REF_NUM = 0;

// Problem parameters.
double MU_VACUUM = 4. * M_PI * 1e-7;
// Constant initial condition for the magnetic potential.
double INIT_COND = 0.0;
// Volume source term.
double CURRENT_DENSITY = 1e9;

// Material and boundary markers.
const std::string MAT_AIR = "e2";
const std::string MAT_IRON_1 = "e0";
const std::string MAT_IRON_2 = "e3";
const std::string MAT_COPPER = "e1";
const std::string BDY_DIRICHLET = "bdy";

int main(int argc, char* argv[])
{
  // Define nonlinear magnetic permeability via a cubic spline.
  std::vector<double> mu_inv_pts({ 0.0, 0.5, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.6, 1.7, 1.8, 1.9, 3.0, 5.0, 10.0 });
  std::vector<double> mu_inv_val({ 1 / 1500.0, 1 / 1480.0, 1 / 1440.0, 1 / 1400.0, 1 / 1300.0, 1 / 1150.0, 1 / 950.0, 1 / 750.0, 1 / 250.0, 1 / 180.0, 1 / 175.0, 1 / 150.0, 1 / 20.0, 1 / 10.0, 1 / 5.0 });

  // Create the cubic spline (and plot it for visual control).
  double bc_left = 0.0;
  double bc_right = 0.0;
  bool first_der_left = false;
  bool first_der_right = false;
  bool extrapolate_der_left = false;
  bool extrapolate_der_right = false;
  CubicSpline mu_inv_iron(mu_inv_pts, mu_inv_val, bc_left, bc_right, first_der_left, first_der_right,
    extrapolate_der_left, extrapolate_der_right);
  Hermes::Mixins::Loggable::Static::info("Saving cubic spline into a Pylab file spline.dat.");
  // The interval of definition of the spline will be
  // extended by "interval_extension" on both sides.
  double interval_extension = 1.0;
  bool plot_derivative = false;
  mu_inv_iron.calculate_coeffs();
  mu_inv_iron.plot("spline.dat", interval_extension, plot_derivative);
  plot_derivative = true;
  mu_inv_iron.plot("spline_der.dat", interval_extension, plot_derivative);

  // Load the mesh.
  MeshSharedPtr mesh(new Mesh);
  MeshReaderH2D mloader;
  mloader.load("actuator.mesh", mesh);

  // View the mesh.
  MeshView m_view;
  m_view.show(mesh);

  // Perform initial mesh refinements.
  for (int i = 0; i < INIT_REF_NUM; i++)
    mesh->refine_all_elements();

  // Initialize boundary conditions.
  DefaultEssentialBCConst<double> bc_essential(BDY_DIRICHLET, 0.0);
  EssentialBCs<double> bcs(&bc_essential);

  // Create an H1 space with default shapeset.
  SpaceSharedPtr<double> space(new H1Space<double>(mesh, &bcs, P_INIT));
  int ndof = space->get_num_dofs();
  Hermes::Mixins::Loggable::Static::info("ndof: %d", ndof);

  // Initialize the weak formulation
  // This determines the increase of integration order
  // for the axisymmetric term containing 1/r. Default is 3.
  int order_inc = 3;
  WeakFormSharedPtr<double> wf(new CustomWeakFormMagnetostatics(MAT_IRON_1, MAT_IRON_2, &mu_inv_iron, MAT_AIR, MAT_COPPER, MU_VACUUM, CURRENT_DENSITY, order_inc));

  // Initialize the FE problem.
  DiscreteProblem<double> dp(wf, space);

  // Initialize the solution.
  MeshFunctionSharedPtr<double> sln(new ConstantSolution<double>(mesh, INIT_COND));

  // Project the initial condition on the FE space to obtain initial
  // coefficient vector for the Newton's method.
  Hermes::Mixins::Loggable::Static::info("Projecting to obtain initial vector for the Newton's method.");

  // Perform Newton's iteration.
  Hermes::Hermes2D::NewtonSolver<double> newton(&dp);
  newton.set_initial_auto_damping_coeff(0.5);
  newton.set_sufficient_improvement_factor(1.1);
  newton.set_necessary_successful_steps_to_increase(1);
  newton.set_sufficient_improvement_factor_jacobian(0.5);
  newton.set_max_steps_with_reused_jacobian(5);
  newton.set_max_allowed_iterations(NEWTON_MAX_ITER);
  newton.set_tolerance(NEWTON_TOL, Hermes::Solvers::ResidualNormAbsolute);
  
  try
  {
    newton.solve(sln);
  }
  catch (Hermes::Exceptions::Exception e)
  {
    e.print_msg();
    throw Hermes::Exceptions::Exception("Newton's iteration failed.");
    return 1;
  };

  // Translate the resulting coefficient vector into the Solution sln.
  Solution<double>::vector_to_solution(newton.get_sln_vector(), space, sln);

  // Visualise the solution and mesh.
  ScalarView s_view1("Vector potential", new WinGeom(0, 0, 350, 450));
  s_view1.show_mesh(false);
  s_view1.show(sln);

  ScalarView s_view2("Flux density", new WinGeom(360, 0, 350, 450));
  MeshFunctionSharedPtr<double> flux_density(new FilterFluxDensity(std::vector<MeshFunctionSharedPtr<double> >({ sln, sln })));
  s_view2.show_mesh(false);
  s_view2.show(flux_density);

  // Output solution in VTK format.
  Linearizer lin(FileExport);
  bool mode_3D = true;
  lin.save_solution_vtk(flux_density, "sln.vtk", "Flux-density", mode_3D);
  Hermes::Mixins::Loggable::Static::info("Solution in VTK format saved to file %s.", "sln.vtk");

  OrderView o_view("Mesh", new WinGeom(720, 0, 350, 450));
  o_view.show(space);

  // Wait for all views to be closed.
  View::wait();
  return 0;
}
