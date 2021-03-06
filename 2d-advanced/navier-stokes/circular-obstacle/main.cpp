#include "definitions.h"

// The time-dependent laminar incompressible Navier-Stokes equations are
// discretized in time via the implicit Euler method. If NEWTON == true,
// the Newton's method is used to solve the nonlinear problem at each time
// step. We also show how to use discontinuous ($L^2$) elements for pressure
// and thus make the velocity discreetely divergence free. Comparison to
// approximating the pressure with the standard (continuous) Taylor-Hood elements
// is enabled. The Reynolds number Re = 200 which is embarrassingly low. You
// can increase it but then you will need to make the mesh finer, and the
// computation will take more time.
//
// PDE: incompressible Navier-Stokes equations in the form
// \partial v / \partial t - \Delta v / Re + (v \cdot \nabla) v + \nabla p = 0,
// div v = 0.
//
// BC: u_1 is a time-dependent constant and u_2 = 0 on Gamma_4 (inlet),
//     u_1 = u_2 = 0 on Gamma_1 (bottom), Gamma_3 (top) and Gamma_5 (obstacle),
//     "do nothing" on Gamma_2 (outlet).
//
// Geometry: Rectangular channel containing an off-axis circular obstacle. The
//           radius and position of the circle, as well as other geometry
//           parameters can be changed in the mesh file "domain.mesh".
//
// The following parameters can be changed:

// Visualization.
// Set to "true" to enable Hermes OpenGL visualization.
const bool HERMES_VISUALIZATION = true;
// Set to "true" to enable VTK output.
const bool VTK_VISUALIZATION = true;

// For application of Stokes flow (creeping flow).
const bool STOKES = false;
// If this is defined, the pressure is approximated using
// discontinuous L2 elements (making the velocity discreetely
// divergence-free, more accurate than using a continuous
// pressure approximation). Otherwise the standard continuous
// elements are used. The results are striking - check the
// tutorial for comparisons.
#define PRESSURE_IN_L2
// Initial polynomial degree for velocity components.
const int P_INIT_VEL = 2;
// Initial polynomial degree for pressure.
// Note: P_INIT_VEL should always be greater than
// P_INIT_PRESSURE because of the inf-sup condition.
const int P_INIT_PRESSURE = 1;
// Reynolds number.
const double RE = 2000.0;
// Inlet velocity (reached after STARTUP_TIME).
const double VEL_INLET = 1.0;
// During this time, inlet velocity increases gradually
// from 0 to VEL_INLET, then it stays constant.
const double STARTUP_TIME = 1.0;
// Time step.
const double TAU = 0.1;
// Time interval length.
const double T_FINAL = 30000.0;
// Stopping criterion for the Newton's method.
const double NEWTON_TOL = 1e-4;
// Maximum allowed number of Newton iterations.
const int NEWTON_MAX_ITER = 50;
// Domain height - necessary to define the parabolic
// velocity profile at inlet (if relevant).
const double H = 5;

// Boundary markers.
const std::string BDY_BOTTOM = "b1";
const std::string BDY_RIGHT = "b2";
const std::string BDY_TOP = "b3";
const std::string BDY_LEFT = "b4";
const std::string BDY_OBSTACLE = "b5";

// Current time (used in weak forms).
double current_time = 0;

int main(int argc, char* argv[])
{
  // Load the mesh.
  MeshSharedPtr mesh(new Mesh);
  MeshReaderH2D mloader;
  mloader.load("domain.mesh", mesh);

  // Initial mesh refinements.
  mesh->refine_all_elements();
  mesh->refine_all_elements();
  mesh->refine_towards_boundary(BDY_OBSTACLE, 2, false);
  // 'true' stands for anisotropic refinements.
  mesh->refine_towards_boundary(BDY_TOP, 2, true);
  mesh->refine_towards_boundary(BDY_BOTTOM, 2, true);

  // Show mesh.
  MeshView mv;
  mv.show(mesh);
  Hermes::Mixins::Loggable::Static::info("Close mesh window to continue.");

  // Initialize boundary conditions.
  EssentialBCNonConst bc_left_vel_x(BDY_LEFT, VEL_INLET, H, STARTUP_TIME);
  DefaultEssentialBCConst<double> bc_other_vel_x({ BDY_BOTTOM, BDY_TOP, BDY_OBSTACLE }, 0.0);
  EssentialBCs<double> bcs_vel_x({ &bc_left_vel_x, &bc_other_vel_x });
  DefaultEssentialBCConst<double> bc_vel_y({ BDY_LEFT, BDY_BOTTOM, BDY_TOP, BDY_OBSTACLE }, 0.0);
  EssentialBCs<double> bcs_vel_y(&bc_vel_y);

  SpaceSharedPtr<double> xvel_space(new H1Space<double>(mesh, &bcs_vel_x, P_INIT_VEL));
  SpaceSharedPtr<double> yvel_space(new H1Space<double>(mesh, &bcs_vel_y, P_INIT_VEL));
#ifdef PRESSURE_IN_L2
  SpaceSharedPtr<double> p_space(new L2Space<double>(mesh, P_INIT_PRESSURE));
#else
  SpaceSharedPtr<double> p_space(new H1Space<double>(mesh, P_INIT_PRESSURE));
#endif
  std::vector<SpaceSharedPtr<double> > spaces({ xvel_space, yvel_space, p_space });

  // Calculate and report the number of degrees of freedom.
  int ndof = Space<double>::get_num_dofs(spaces);
  Hermes::Mixins::Loggable::Static::info("ndof = %d.", ndof);

  // Define projection norms.
  NormType vel_proj_norm = HERMES_H1_NORM;
#ifdef PRESSURE_IN_L2
  NormType p_proj_norm = HERMES_L2_NORM;
#else
  NormType p_proj_norm = HERMES_H1_NORM;
#endif

  // Solutions for the Newton's iteration and time stepping.
  Hermes::Mixins::Loggable::Static::info("Setting zero initial conditions.");
  MeshFunctionSharedPtr<double>  xvel_prev_time(new ZeroSolution<double>(mesh));
  MeshFunctionSharedPtr<double>  yvel_prev_time(new ZeroSolution<double>(mesh));
  MeshFunctionSharedPtr<double>  p_prev_time(new ZeroSolution<double>(mesh));

  // Initialize weak formulation.
  WeakFormSharedPtr<double> wf(new WeakFormNSNewton(STOKES, RE, TAU, xvel_prev_time, yvel_prev_time));

  // Initialize the FE problem.
  Hermes::Hermes2D::NewtonSolver<double> newton(wf, spaces);
  Hermes::Mixins::Loggable::Static::info("Solving nonlinear problem:");
  newton.set_max_allowed_iterations(NEWTON_MAX_ITER);
  newton.set_tolerance(NEWTON_TOL, Hermes::Solvers::ResidualNormAbsolute);
  newton.set_jacobian_constant();

  // Initialize views.
  VectorView vview("velocity [m/s]", new WinGeom(0, 0, 750, 240));
  ScalarView pview("pressure [Pa]", new WinGeom(0, 290, 750, 240));
  vview.set_min_max_range(0, 1.6);
  vview.fix_scale_width(80);
  //pview.set_min_max_range(-0.9, 1.0);
  pview.fix_scale_width(80);
  pview.show_mesh(true);

  // Time-stepping loop:
  char title[100];
  int num_time_steps = T_FINAL / TAU;
  for (int ts = 1; ts <= num_time_steps; ts++)
  {
    current_time += TAU;
    Hermes::Mixins::Loggable::Static::info("---- Time step %d, time = %g:", ts, current_time);

    // Update time-dependent essential BCs.
    if (current_time <= STARTUP_TIME) {
      Hermes::Mixins::Loggable::Static::info("Updating time-dependent essential BC.");
      Space<double>::update_essential_bc_values(spaces, current_time);
    }

    // Perform Newton's iteration.
    try
    {
      newton.solve();
    }
    catch (Hermes::Exceptions::Exception e)
    {
      e.print_msg();
    };

    // Update previous time level solutions.
    Solution<double>::vector_to_solutions(newton.get_sln_vector(), spaces, { xvel_prev_time, yvel_prev_time, p_prev_time });

    // Visualization.
    // Hermes visualization.
    if (HERMES_VISUALIZATION)
    {
      // Show the solution at the end of time step.
      sprintf(title, "Velocity, time %g", current_time);
      vview.set_title(title);
      vview.show(xvel_prev_time, yvel_prev_time);
      sprintf(title, "Pressure, time %g", current_time);
      pview.set_title(title);
      pview.show(p_prev_time);
    }
  }

  // Wait for all views to be closed.
  View::wait();
  return 0;
}