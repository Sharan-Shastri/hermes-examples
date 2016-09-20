#include "definitions.h"

//  This example models a nonstationary distribution of temperature within a wall
//  exposed to ISO fire. Spatial adaptivity is ON by default, adaptivity in time
//  can be turned ON and OFF using the flag ADAPTIVE_TIME_STEP_ON.
//
//  PDE: non-stationary heat transfer equation
//       HEATCAP * RHO * dT/dt - div (LAMBDA grad T) = 0.
//  Here LAMBDA(x, y) is an external, user-defined function.
//
//  For the sake of using arbitrary RK methods, this equation is written in such
//  a way that the time-derivative is on the left and everything else on the right:
//
//  dT/dt = div(LAMBDA grad T) / (HEATCAP * RHO)
//
//  Domain: rectangle 4.0 x 0.5 (file wall.mesh).
//
//  IC:  T = TEMP_INIT.
//  BC:  Bottom edge: LAMBDA * dT/dn = ALPHA_BOTTOM*(T_fire(x, time) - T)
//       Vertical edges: dT/dn = 0
//       Top edge: LAMBDA * dT/dn = ALPHA_TOP*(TEMP_EXT_AIR - T)
//
//  Time-stepping: Arbitrary Runge-Kutta methods (choose one of the predefined
//                 Butcher's tables below or create your own).
//
//  The following parameters can be changed:

// Polynomial degree of all mesh elements.
const int P_INIT = 1;
// Number of initial uniform mesh refinements.
const int INIT_REF_NUM = 1;
// Number of initial uniform mesh refinements towards the boundary.
const int INIT_REF_NUM_BDY = 1;
// Time step in seconds.
double time_step = 20;

// Spatial adaptivity.
// Every UNREF_FREQth time step the mesh is derefined.
const int UNREF_FREQ = 1;
// 1... mesh reset to basemesh and poly degrees to P_INIT.
// 2... one ref. layer shaved off, poly degrees reset to P_INIT.
// 3... one ref. layer shaved off, poly degrees decreased by one.
const int UNREF_METHOD = 3;
// This is a quantitative parameter of the adapt(...) function and
// it has different meanings for various adaptive strategies.
const double THRESHOLD = 0.3;
// Error calculation & adaptivity.
DefaultErrorCalculator<double, HERMES_H1_NORM> errorCalculator(RelativeErrorToGlobalNorm, 1);
// Stopping criterion for an adaptivity step.
AdaptStoppingCriterionSingleElement<double> stoppingCriterion(THRESHOLD);
// Adaptivity processor class.
Adapt<double> adaptivity(&errorCalculator, &stoppingCriterion);
// Predefined list of element refinement candidates.
const CandList CAND_LIST = H2D_HP_ANISO;
// Stopping criterion for adaptivity.
const double ERR_STOP = 1e-1;

// Temporal adaptivity.
// This flag decides whether adaptive time stepping will be done.
// The methods for the adaptive and fixed-step versions are set
// below. An embedded method must be used with adaptive time stepping.
bool ADAPTIVE_TIME_STEP_ON = false;
// If rel. temporal error is greater than this threshold, decrease time
// step size and repeat time step.
const double TIME_ERR_TOL_UPPER = 1.0;
// If rel. temporal error is less than this threshold, increase time step
// but do not repeat time step (this might need further research).
const double TIME_ERR_TOL_LOWER = 0.5;
// Time step increase ratio (applied when rel. temporal error is too small).
const double TIME_STEP_INC_RATIO = 1.1;
// Time step decrease ratio (applied when rel. temporal error is too large).
const double TIME_STEP_DEC_RATIO = 0.8;
// Space error tolerance.
const double SPACE_ERR_TOL = 10.;

// Newton's method.
// Stopping criterion for Newton on fine mesh.
const double NEWTON_TOL_COARSE = 0.001;
// Stopping criterion for Newton on fine mesh.
const double NEWTON_TOL_FINE = 0.005;
// Maximum allowed number of Newton iterations.
const int NEWTON_MAX_ITER = 100;

// Choose one of the following time-integration methods, or define your own Butcher's table. The last number
// in the name of each method is its order. The one before last, if present, is the number of stages.
// Explicit methods:
//   Explicit_RK_1, Explicit_RK_2, Explicit_RK_3, Explicit_RK_4.
// Implicit methods:
//   Implicit_RK_1, Implicit_Crank_Nicolson_2_2, Implicit_SIRK_2_2, Implicit_ESIRK_2_2, Implicit_SDIRK_2_2,
//   Implicit_Lobatto_IIIA_2_2, Implicit_Lobatto_IIIB_2_2, Implicit_Lobatto_IIIC_2_2, Implicit_Lobatto_IIIA_3_4,
//   Implicit_Lobatto_IIIB_3_4, Implicit_Lobatto_IIIC_3_4, Implicit_Radau_IIA_3_5, Implicit_SDIRK_5_4.
// Embedded explicit methods:
//   Explicit_HEUN_EULER_2_12_embedded, Explicit_BOGACKI_SHAMPINE_4_23_embedded, Explicit_FEHLBERG_6_45_embedded,
//   Explicit_CASH_KARP_6_45_embedded, Explicit_DORMAND_PRINCE_7_45_embedded.
// Embedded implicit methods:
//   Implicit_SDIRK_CASH_3_23_embedded, Implicit_ESDIRK_TRBDF2_3_23_embedded, Implicit_ESDIRK_TRX2_3_23_embedded,
//   Implicit_SDIRK_BILLINGTON_3_23_embedded, Implicit_SDIRK_CASH_5_24_embedded, Implicit_SDIRK_CASH_5_34_embedded,
//   Implicit_DIRK_ISMAIL_7_45_embedded.
ButcherTableType butcher_table_type = Implicit_SDIRK_CASH_3_23_embedded;

// Boundary markers.
const std::string BDY_FIRE = "Bottom";
const std::string BDY_LEFT = "Left";
const std::string BDY_RIGHT = "Right";
const std::string BDY_AIR = "Top";

// Problem parameters.
// Initial temperature.
const double TEMP_INIT = 20;
// Exterior temperature top;
const double TEMP_EXT_AIR = 20;

// Heat flux coefficient on the bottom edge.
const double ALPHA_FIRE = 25;
// Heat flux coefficient on the top edge.
const double ALPHA_AIR = 8;
// Heat capacity.
const double HEATCAP = 1020;
// Material density.
const double RHO = 2200;
// Length of time interval in seconds.
const double T_FINAL = 18000;

int main(int argc, char* argv[])
{
  // Choose a Butcher's table or define your own.
  ButcherTable bt(butcher_table_type);
  if (bt.is_explicit()) Hermes::Mixins::Loggable::Static::info("Using a %d-stage explicit R-K method.", bt.get_size());
  if (bt.is_diagonally_implicit()) Hermes::Mixins::Loggable::Static::info("Using a %d-stage diagonally implicit R-K method.", bt.get_size());
  if (bt.is_fully_implicit()) Hermes::Mixins::Loggable::Static::info("Using a %d-stage fully implicit R-K method.", bt.get_size());

  // Turn off adaptive time stepping if R-K method is not embedded.
  if (bt.is_embedded() == false && ADAPTIVE_TIME_STEP_ON == true) {
    Hermes::Mixins::Loggable::Static::warn("R-K method not embedded, turning off adaptive time stepping.");
    ADAPTIVE_TIME_STEP_ON = false;
  }

  // Load the mesh.
  MeshSharedPtr mesh(new Mesh), basemesh(new Mesh);
  MeshReaderH2D mloader;
  mloader.load("wall.mesh", basemesh);
  mesh->copy(basemesh);

  // Perform initial mesh refinements.
  for (int i = 0; i < INIT_REF_NUM; i++) mesh->refine_all_elements();
  mesh->refine_towards_boundary(BDY_RIGHT, 2);
  mesh->refine_towards_boundary(BDY_FIRE, INIT_REF_NUM_BDY);

  // Initialize essential boundary conditions (none).
  EssentialBCs<double> bcs;

  // Initialize an H1 space with default shapeset.
  SpaceSharedPtr<double> space(new H1Space<double>(mesh, &bcs, P_INIT));
  int ndof = Space<double>::get_num_dofs(space);
  Hermes::Mixins::Loggable::Static::info("ndof = %d.", ndof);

  // Convert initial condition into a Solution.
  MeshFunctionSharedPtr<double> sln_prev_time(new ConstantSolution<double>(mesh, TEMP_INIT));

  // Initialize the weak formulation.
  double current_time = 0;
  WeakFormSharedPtr<double> wf(new CustomWeakFormHeatRK(BDY_FIRE, BDY_AIR, ALPHA_FIRE, ALPHA_AIR,
    RHO, HEATCAP, TEMP_EXT_AIR, TEMP_INIT, &current_time));

  // Initialize the FE problem.
  DiscreteProblem<double> dp(wf, space);

  // Create a refinement selector.
  H1ProjBasedSelector<double> selector(CAND_LIST);

  // Visualize initial condition.
  char title[100];
  ScalarView sln_view("Initial condition", new WinGeom(0, 0, 1500, 360));
  OrderView ordview("Initial mesh", new WinGeom(0, 410, 1500, 360));
  ScalarView time_error_view("Temporal error", new WinGeom(0, 800, 1500, 360));
  time_error_view.fix_scale_width(40);
  ScalarView space_error_view("Spatial error", new WinGeom(0, 1220, 1500, 360));
  space_error_view.fix_scale_width(40);
  sln_view.show(sln_prev_time);
  ordview.show(space);

  // Graph for time step history.
  SimpleGraph time_step_graph;
  if (ADAPTIVE_TIME_STEP_ON) Hermes::Mixins::Loggable::Static::info("Time step history will be saved to file time_step_history.dat.");

  // Class for projections.
  OGProjection<double> ogProjection;

  // Time stepping loop:
  int ts = 1;
  do
  {
    Hermes::Mixins::Loggable::Static::info("Begin time step %d.", ts);
    // Periodic global derefinement.
    if (ts > 1 && ts % UNREF_FREQ == 0)
    {
      Hermes::Mixins::Loggable::Static::info("Global mesh derefinement.");
      switch (UNREF_METHOD) {
      case 1: mesh->copy(basemesh);
        space->set_uniform_order(P_INIT);
        break;
      case 2: space->unrefine_all_mesh_elements();
        space->set_uniform_order(P_INIT);
        break;
      case 3: space->unrefine_all_mesh_elements();
        //space->adjust_element_order(-1, P_INIT);
        space->adjust_element_order(-1, -1, P_INIT, P_INIT);
        break;
      default: throw Hermes::Exceptions::Exception("Wrong global derefinement method.");
      }

      space->assign_dofs();
      ndof = Space<double>::get_num_dofs(space);
    }

    // Spatial adaptivity loop. Note: sln_prev_time must not be
    // changed during spatial adaptivity.
    MeshFunctionSharedPtr<double> ref_sln(new Solution<double>());
    MeshFunctionSharedPtr<double> time_error_fn(new Solution<double>(mesh));
    bool done = false; int as = 1;
    double err_est;
    do {
      // Construct globally refined reference mesh and setup reference space.
      Mesh::ReferenceMeshCreator refMeshCreator(mesh);
      MeshSharedPtr ref_mesh = refMeshCreator.create_ref_mesh();

      Space<double>::ReferenceSpaceCreator refSpaceCreator(space, ref_mesh);
      SpaceSharedPtr<double> ref_space = refSpaceCreator.create_ref_space();

      // Initialize Runge-Kutta time stepping on the reference mesh.
      RungeKutta<double> runge_kutta(wf, ref_space, &bt);

      try
      {
        ogProjection.project_global(ref_space, sln_prev_time,
          sln_prev_time);
      }
      catch (Exceptions::Exception& e)
      {
        std::cout << e.what() << std::endl;
        Hermes::Mixins::Loggable::Static::error("Projection failed.");

        return -1;
      }

      // Runge-Kutta step on the fine mesh.
      Hermes::Mixins::Loggable::Static::info("Runge-Kutta time step on fine mesh (t = %g s, tau = %g s, stages: %d).",
        current_time, time_step, bt.get_size());
      bool verbose = true;
      bool jacobian_changed = false;

      try
      {
        runge_kutta.set_time(current_time);
        runge_kutta.set_time_step(time_step);
        runge_kutta.set_newton_max_allowed_iterations(NEWTON_MAX_ITER);
        runge_kutta.set_newton_tolerance(NEWTON_TOL_FINE);
        runge_kutta.rk_time_step_newton(sln_prev_time, ref_sln, bt.is_embedded() ? time_error_fn : NULL);
      }
      catch (Exceptions::Exception& e)
      {
        std::cout << e.what() << std::endl;
        Hermes::Mixins::Loggable::Static::error("Runge-Kutta time step failed");

        return -1;
      }

      /* If ADAPTIVE_TIME_STEP_ON == true, estimate temporal error.
      If too large or too small, then adjust it and restart the time step. */

      double rel_err_time = 0;
      if (bt.is_embedded() == true)
      {
        Hermes::Mixins::Loggable::Static::info("Calculating temporal error estimate.");

        // Show temporal error.
        char title[100];
        sprintf(title, "Temporal error est, spatial adaptivity step %d", as);
        time_error_view.set_title(title);
        //time_error_view.show_mesh(false);
        time_error_view.show(time_error_fn);

        DefaultNormCalculator<double, HERMES_H1_NORM> normCalculator(1);
        rel_err_time = 100. * normCalculator.calculate_norm(time_error_fn) / normCalculator.calculate_norm(ref_sln);

        if (ADAPTIVE_TIME_STEP_ON == false) Hermes::Mixins::Loggable::Static::info("rel_err_time: %g%%", rel_err_time);
      }

      if (ADAPTIVE_TIME_STEP_ON)
      {
        if (rel_err_time > TIME_ERR_TOL_UPPER)
        {
          Hermes::Mixins::Loggable::Static::info("rel_err_time %g%% is above upper limit %g%%", rel_err_time, TIME_ERR_TOL_UPPER);
          Hermes::Mixins::Loggable::Static::info("Decreasing tau from %g to %g s and restarting time step.",
            time_step, time_step * TIME_STEP_DEC_RATIO);
          time_step *= TIME_STEP_DEC_RATIO;
          continue;
        }
        else if (rel_err_time < TIME_ERR_TOL_LOWER)
        {
          Hermes::Mixins::Loggable::Static::info("rel_err_time = %g%% is below lower limit %g%%", rel_err_time, TIME_ERR_TOL_LOWER);
          Hermes::Mixins::Loggable::Static::info("Increasing tau from %g to %g s.", time_step, time_step * TIME_STEP_INC_RATIO);
          time_step *= TIME_STEP_INC_RATIO;
        }
        else
        {
          Hermes::Mixins::Loggable::Static::info("rel_err_time = %g%% is in acceptable interval (%g%%, %g%%)",
            rel_err_time, TIME_ERR_TOL_LOWER, TIME_ERR_TOL_UPPER);
        }

        // Add entry to time step history graph.
        time_step_graph.add_values(current_time, time_step);
        time_step_graph.save("time_step_history.dat");
      }

      /* Estimate spatial errors and perform mesh refinement */

      Hermes::Mixins::Loggable::Static::info("Spatial adaptivity step %d.", as);

      // Project the fine mesh solution onto the coarse mesh.
      MeshFunctionSharedPtr<double> sln(new Solution<double>());
      Hermes::Mixins::Loggable::Static::info("Projecting fine mesh solution on coarse mesh for error estimation.");
      ogProjection.project_global(space, ref_sln, sln);

      // Show spatial error.
      sprintf(title, "Spatial error est, spatial adaptivity step %d", as);
      MeshFunctionSharedPtr<double> space_error_fn(new DiffFilter<double>(std::vector<MeshFunctionSharedPtr<double> >({ ref_sln, sln })));
      space_error_view.set_title(title);
      //space_error_view.show_mesh(false);
      MeshFunctionSharedPtr<double> abs_sef(new AbsFilter(space_error_fn));

      space_error_view.show(abs_sef);

      // Calculate element errors and spatial error estimate.
      Hermes::Mixins::Loggable::Static::info("Calculating spatial error estimate.");
      adaptivity.set_space(space); 
      errorCalculator.calculate_errors(sln, ref_sln);
      double err_rel_space = errorCalculator.get_total_error_squared() * 100;

      // Report results.
      Hermes::Mixins::Loggable::Static::info("ndof: %d, ref_ndof: %d, err_rel_space: %g%%",
        Space<double>::get_num_dofs(space), Space<double>::get_num_dofs(ref_space), err_rel_space);

      // If err_est too large, adapt the mesh.
      if (err_rel_space < SPACE_ERR_TOL) done = true;
      else
      {
        Hermes::Mixins::Loggable::Static::info("Adapting the coarse mesh.");
        done = adaptivity.adapt(&selector);

        // Increase the counter of performed adaptivity steps.
        as++;
      }
    } while (done == false);

    // Visualize the solution and mesh.
    char title[100];
    sprintf(title, "Solution, time %g s", current_time);
    sln_view.set_title(title);
    //sln_view.show_mesh(false);
    sln_view.show(ref_sln);
    sprintf(title, "Mesh, time %g s", current_time);
    ordview.set_title(title);
    ordview.show(space);

    // Copy last reference solution into sln_prev_time
    sln_prev_time->copy(ref_sln);

    // Increase current time and counter of time steps.
    current_time += time_step;
    ts++;
  } while (current_time < T_FINAL);

  // Wait for all views to be closed.
  View::wait();
  return 0;
}