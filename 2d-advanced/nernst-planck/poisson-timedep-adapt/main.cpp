#include "definitions.h"
#include "timestep_controller.h"

/** \addtogroup e_newton_np_timedep_adapt_system Newton Time-dependant System with Adaptivity
\{
\brief This example shows how to combine the automatic adaptivity with the Newton's method for a nonlinear time-dependent PDE system.

This example shows how to combine the automatic adaptivity with the
Newton's method for a nonlinear time-dependent PDE system.
The time discretization is done using implicit Euler or
Crank Nicholson method (see parameter TIME_DISCR).
The following PDE's are solved:
Nernst-Planck (describes the diffusion and migration of charged particles):
\f[dC/dt - D*div[grad(C)] - K*C*div[grad(\phi)]=0,\f]
where D and K are constants and C is the cation concentration variable,
phi is the voltage variable in the Poisson equation:
\f[ - div[grad(\phi)] = L*(C - C_0),\f]
where \f$C_0\f$, and L are constant (anion concentration). \f$C_0\f$ is constant
anion concentration in the domain and L is material parameter.
So, the equation variables are phi and C and the system describes the
migration/diffusion of charged particles due to applied voltage.
The simulation domain looks as follows:
\verbatim
Top
+----------+
|          |
Side|          |Side
|          |
+----------+
Bottom
\endverbatim
For the Nernst-Planck equation, all the boundaries are natural i.e. Neumann.
Which basically means that the normal derivative is 0:
\f[ BC: -D*dC/dn - K*C*d\phi/dn = 0 \f]
For Poisson equation, boundary 1 has a natural boundary condition
(electric field derivative is 0).
The voltage is applied to the boundaries 2 and 3 (Dirichlet boundaries)
It is possible to adjust system paramter VOLT_BOUNDARY to apply
Neumann boundary condition to 2 (instead of Dirichlet). But by default:
- BC 2: \f$\phi = VOLTAGE\f$
- BC 3: \f$\phi = 0\f$
- BC 1: \f$\frac{d\phi}{dn} = 0\f$
*/

// Parameters to tweak the amount of output to the console.
#define NOSCREENSHOT

// True if scaled dimensionless variables are used, false otherwise.
bool SCALED = true;

/*** Fundamental coefficients ***/

// [m^2/s] Diffusion coefficient.
const double D = 10e-11;
// [J/mol*K] Gas constant.
const double R = 8.31;
// [K] Aboslute temperature.
const double T = 293;
// [s * A / mol] Faraday constant.
const double F = 96485.3415;
// [F/m] Electric permeability.
const double eps = 2.5e-2;
// Mobility of ions.
const double mu = D / (R * T);
// Charge number.
const double z = 1;
// Constant for equation.
const double K = z * mu * F;
// Constant for equation.
const double L = F / eps;
// [mol/m^3] Anion and counterion concentration.
const double C0 = 1200;

// Scaling constants.
// Scaling const, domain thickness [m].
const double l = 200e-6;
// Debye length [m].
double lambda = Hermes::sqrt((eps)*R*T / (2.0*F*F*C0));
double epsilon = lambda / l;

// [V] Applied voltage.
const double VOLTAGE = 1;
const double SCALED_VOLTAGE = VOLTAGE*F / (R*T);

/* Simulation parameters */

const double T_FINAL = 3;
double INIT_TAU = 0.05;
// Size of the time step.
double *TAU = &INIT_TAU;

// Scaling time variables.
//double SCALED_INIT_TAU = INIT_TAU*D/(lambda * l);
//double TIME_SCALING = lambda * l / D;

// Initial polynomial degree of all mesh elements.
const int P_INIT = 2;
// Number of initial refinements.
const int REF_INIT = 3;
// Multimesh?
const bool MULTIMESH = true;
// 1 for implicit Euler, 2 for Crank-Nicolson.
const int TIME_DISCR = 2;

// Stopping criterion for Newton on coarse mesh.
const double NEWTON_TOL_COARSE = 0.01;
// Stopping criterion for Newton on fine mesh.
const double NEWTON_TOL_FINE = 0.05;
// Maximum allowed number of Newton iterations.
const int NEWTON_MAX_ITER = 100;

// Every UNREF_FREQth time step the mesh is unrefined.
const int UNREF_FREQ = 1;
// This is a quantitative parameter of the adapt(...) function and
// it has different meanings for various adaptive strategies.
const double THRESHOLD = 0.3;
// Error calculation & adaptivity.
DefaultErrorCalculator<double, HERMES_H1_NORM> errorCalculator(RelativeErrorToGlobalNorm, 2);
// Stopping criterion for an adaptivity step.
AdaptStoppingCriterionSingleElement<double> stoppingCriterion(THRESHOLD);
// Adaptivity processor class.
Adapt<double> adaptivity(&errorCalculator, &stoppingCriterion);
// Predefined list of element refinement candidates.
const CandList CAND_LIST = H2D_HP_ANISO;
// Stopping criterion for adaptivity.
const double ERR_STOP = 1e-1;

// Weak forms.
#include "definitions.cpp"

// Boundary markers.
const std::string BDY_SIDE = "Side";
const std::string BDY_TOP = "Top";
const std::string BDY_BOT = "Bottom";

// scaling methods

double scaleTime(double t) {
  return SCALED ? t * D / (lambda * l) : t;
}

double scaleVoltage(double phi) {
  return SCALED ? phi * F / (R * T) : phi;
}

double scaleConc(double C) {
  return SCALED ? C / C0 : C;
}

double physTime(double t) {
  return SCALED ? lambda * l * t / D : t;
}

double physConc(double C) {
  return SCALED ? C0 * C : C;
}

double physVoltage(double phi) {
  return SCALED ? phi * R * T / F : phi;
}

double SCALED_INIT_TAU = scaleTime(INIT_TAU);

int main(int argc, char* argv[]) {
  // Load the mesh file.
  MeshSharedPtr C_mesh(new Mesh), phi_mesh(new Mesh), basemesh(new Mesh);
  MeshReaderH2D mloader;
  mloader.load("small.mesh", basemesh);

  if (SCALED) {
    bool ret = basemesh->rescale(l, l);
    if (ret) {
      Hermes::Mixins::Loggable::Static::info("SCALED mesh is used");
    }
    else {
      Hermes::Mixins::Loggable::Static::info("UNSCALED mesh is used");
    }
  }

  // When nonadaptive solution, refine the mesh.
  basemesh->refine_towards_boundary(BDY_TOP, REF_INIT);
  basemesh->refine_towards_boundary(BDY_BOT, REF_INIT - 1);
  basemesh->refine_all_elements(1);
  basemesh->refine_all_elements(1);
  C_mesh->copy(basemesh);
  phi_mesh->copy(basemesh);

  DefaultEssentialBCConst<double> bc_phi_voltage(BDY_TOP, scaleVoltage(VOLTAGE));
  DefaultEssentialBCConst<double> bc_phi_zero(BDY_BOT, scaleVoltage(0.0));

  EssentialBCs<double> bcs_phi(std::vector<EssentialBoundaryCondition<double>* >({ &bc_phi_voltage, &bc_phi_zero }));

  SpaceSharedPtr<double> C_space(new H1Space<double>(C_mesh, P_INIT));
  SpaceSharedPtr<double> phi_space(new  H1Space<double>(MULTIMESH ? phi_mesh : C_mesh, &bcs_phi, P_INIT));

  std::vector<SpaceSharedPtr<double> > spaces({ C_space, phi_space });

  MeshFunctionSharedPtr<double> C_sln(new Solution<double>), C_ref_sln(new Solution<double>);
  MeshFunctionSharedPtr<double> phi_sln(new Solution<double>), phi_ref_sln(new Solution<double>);

  // Assign initial condition to mesh->
  MeshFunctionSharedPtr<double> C_prev_time(new ConstantSolution<double>(C_mesh, scaleConc(C0)));
  MeshFunctionSharedPtr<double> phi_prev_time(new ConstantSolution<double>(MULTIMESH ? phi_mesh : C_mesh, 0.0));

  // XXX not necessary probably
  if (SCALED) {
    TAU = &SCALED_INIT_TAU;
  }

  // The weak form for 2 equations.
  WeakForm<double> *wf;
  if (TIME_DISCR == 2) {
    if (SCALED) {
      wf = new ScaledWeakFormPNPCranic(TAU, ::epsilon, C_prev_time, phi_prev_time);
      Hermes::Mixins::Loggable::Static::info("Scaled weak form, with time step %g and epsilon %g", *TAU, ::epsilon);
    }
    else {
      wf = new WeakFormPNPCranic(TAU, C0, K, L, D, C_prev_time, phi_prev_time);
    }
  }
  else {
    if (SCALED)
      throw Hermes::Exceptions::Exception("Forward Euler is not implemented for scaled problem");
    wf = new WeakFormPNPEuler(TAU, C0, K, L, D, C_prev_time);
  }

  DiscreteProblem<double> dp_coarse(wf, spaces);

  NewtonSolver<double>* solver_coarse = new NewtonSolver<double>(&dp_coarse);

  // Project the initial condition on the FE space to obtain initial
  // coefficient vector for the Newton's method.
  Hermes::Mixins::Loggable::Static::info("Projecting to obtain initial vector for the Newton's method.");
  int ndof = Space<double>::get_num_dofs({ C_space, phi_space });
  double* coeff_vec_coarse = new double[ndof];

  OGProjection<double>::project_global({ C_space, phi_space }, { C_prev_time, phi_prev_time },
    coeff_vec_coarse);

  // Create a selector which will select optimal candidate.
  H1ProjBasedSelector<double> selector(CAND_LIST);

  // Visualization windows.
  char title[1000];
  ScalarView Cview("Concentration [mol/m3]", new WinGeom(0, 0, 800, 800));
  ScalarView phiview("Voltage [V]", new WinGeom(650, 0, 600, 600));
  OrderView Cordview("C order", new WinGeom(0, 300, 600, 600));
  OrderView phiordview("Phi order", new WinGeom(600, 300, 600, 600));

  Cview.show(C_prev_time);
  Cordview.show(C_space);
  phiview.show(phi_prev_time);
  phiordview.show(phi_space);

  // Newton's loop on the coarse mesh.
  Hermes::Mixins::Loggable::Static::info("Solving on initial coarse mesh");
  try
  {
    solver_coarse->set_max_allowed_iterations(NEWTON_MAX_ITER);
    solver_coarse->set_tolerance(NEWTON_TOL_COARSE, Hermes::Solvers::ResidualNormAbsolute);
    solver_coarse->solve(coeff_vec_coarse);
  }
  catch (Hermes::Exceptions::Exception e)
  {
    e.print_msg();
    throw Hermes::Exceptions::Exception("Newton's iteration failed.");
  };

  //View::wait(HERMES_WAIT_KEYPRESS);

  // Translate the resulting coefficient vector into the Solution<double> sln->
  Solution<double>::vector_to_solutions(solver_coarse->get_sln_vector(), { C_space, phi_space },
  { C_sln, phi_sln });

  Cview.show(C_sln);
  phiview.show(phi_sln);

  // Cleanup after the Newton loop on the coarse mesh.
  delete solver_coarse;
  delete[] coeff_vec_coarse;

  // Time stepping loop.
  PidTimestepController pid(scaleTime(T_FINAL), true, scaleTime(INIT_TAU));
  TAU = pid.timestep;
  Hermes::Mixins::Loggable::Static::info("Starting time iteration with the step %g", *TAU);

  NewtonSolver<double> solver;
  solver.set_weak_formulation(wf);

  do {
    pid.begin_step();
    // Periodic global derefinements.
    if (pid.get_timestep_number() > 1 && pid.get_timestep_number() % UNREF_FREQ == 0)
    {
      Hermes::Mixins::Loggable::Static::info("Global mesh derefinement.");
      C_mesh->copy(basemesh);
      if (MULTIMESH)
        phi_mesh->copy(basemesh);
      C_space->set_uniform_order(P_INIT);
      phi_space->set_uniform_order(P_INIT);
      C_space->assign_dofs();
      phi_space->assign_dofs();
    }

    // Adaptivity loop. Note: C_prev_time and Phi_prev_time must not be changed during spatial adaptivity.
    bool done = false; int as = 1;
    double err_est;
    do {
      Hermes::Mixins::Loggable::Static::info("Time step %d, adaptivity step %d:", pid.get_timestep_number(), as);

      // Construct globally refined reference mesh
      // and setup reference space.
      Mesh::ReferenceMeshCreator refMeshCreatorC(C_mesh);
      MeshSharedPtr ref_C_mesh = refMeshCreatorC.create_ref_mesh();

      Space<double>::ReferenceSpaceCreator refSpaceCreatorC(C_space, ref_C_mesh);
      SpaceSharedPtr<double> ref_C_space = refSpaceCreatorC.create_ref_space();

      Mesh::ReferenceMeshCreator refMeshCreatorPhi(phi_mesh);
      MeshSharedPtr ref_phi_mesh = refMeshCreatorPhi.create_ref_mesh();

      Space<double>::ReferenceSpaceCreator refSpaceCreatorPhi(phi_space, ref_phi_mesh);
      SpaceSharedPtr<double> ref_phi_space = refSpaceCreatorPhi.create_ref_space();

      std::vector<SpaceSharedPtr<double> > ref_spaces({ ref_C_space, ref_phi_space });

      int ndof_ref = Space<double>::get_num_dofs(ref_spaces);

      // Newton's loop on the fine mesh.
      Hermes::Mixins::Loggable::Static::info("Solving on fine mesh:");
      try
      {
        solver.set_spaces(ref_spaces);
        solver.set_max_allowed_iterations(NEWTON_MAX_ITER);
        solver.set_tolerance(NEWTON_TOL_FINE, Hermes::Solvers::ResidualNormAbsolute);
        if (as == 1 && pid.get_timestep_number() == 1)
          solver.solve(std::vector<MeshFunctionSharedPtr<double> >({ C_sln, phi_sln }));
        else
          solver.solve(std::vector<MeshFunctionSharedPtr<double> >({ C_ref_sln, phi_ref_sln }));
      }
      catch (Hermes::Exceptions::Exception e)
      {
        e.print_msg();
        throw Hermes::Exceptions::Exception("Newton's iteration failed.");
      };

      // Store the result in ref_sln->
      Solution<double>::vector_to_solutions(solver.get_sln_vector(), ref_spaces, { C_ref_sln, phi_ref_sln });

      // Projecting reference solution onto the coarse mesh
      Hermes::Mixins::Loggable::Static::info("Projecting fine mesh solution on coarse mesh.");
      OGProjection<double>::project_global({ C_space, phi_space }, { C_ref_sln, phi_ref_sln }, { C_sln, phi_sln });

      // Calculate element errors and total error estimate.
      Hermes::Mixins::Loggable::Static::info("Calculating error estimate.");
      adaptivity.set_spaces({ C_space, phi_space });

      errorCalculator.calculate_errors({ C_sln, phi_sln }, { C_ref_sln, phi_ref_sln });
      double err_est_rel_total = errorCalculator.get_total_error_squared() * 100;

      // Report results.
      Hermes::Mixins::Loggable::Static::info("ndof_coarse_total: %d, ndof_fine_total: %d, err_est_rel: %g%%",
        Space<double>::get_num_dofs({ C_space, phi_space }),
        Space<double>::get_num_dofs(ref_spaces), err_est_rel_total);

      // If err_est too large, adapt the mesh.
      if (err_est_rel_total < ERR_STOP) done = true;
      else
      {
        Hermes::Mixins::Loggable::Static::info("Adapting the coarse mesh.");
        done = adaptivity.adapt({ &selector, &selector });
        Hermes::Mixins::Loggable::Static::info("Adapted...");
        as++;
      }

      // Visualize the solution and mesh.
      Hermes::Mixins::Loggable::Static::info("Visualization procedures: C");
      char title[100];
      sprintf(title, "Solution[C], step# %d, step size %g, time %g, phys time %g",
        pid.get_timestep_number(), *TAU, pid.get_time(), physTime(pid.get_time()));
      Cview.set_title(title);
      Cview.show(C_ref_sln);
      sprintf(title, "Mesh[C], step# %d, step size %g, time %g, phys time %g",
        pid.get_timestep_number(), *TAU, pid.get_time(), physTime(pid.get_time()));
      Cordview.set_title(title);
      Cordview.show(C_space);

      Hermes::Mixins::Loggable::Static::info("Visualization procedures: phi");
      sprintf(title, "Solution[phi], step# %d, step size %g, time %g, phys time %g",
        pid.get_timestep_number(), *TAU, pid.get_time(), physTime(pid.get_time()));
      phiview.set_title(title);
      phiview.show(phi_ref_sln);
      sprintf(title, "Mesh[phi], step# %d, step size %g, time %g, phys time %g",
        pid.get_timestep_number(), *TAU, pid.get_time(), physTime(pid.get_time()));
      phiordview.set_title(title);
      phiordview.show(phi_space);
      //View::wait(HERMES_WAIT_KEYPRESS);

    } while (done == false);

    pid.end_step({ C_ref_sln, phi_ref_sln }, { C_prev_time, phi_prev_time });
    // TODO! Time step reduction when necessary.

    // Copy last reference solution into sln_prev_time
    C_prev_time->copy(C_ref_sln);
    phi_prev_time->copy(phi_ref_sln);
  } while (pid.has_next());

  // Wait for all views to be closed.
  View::wait();
  return 0;
}