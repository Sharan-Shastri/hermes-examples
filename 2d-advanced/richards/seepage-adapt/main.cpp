#include "definitions.h"

//  This example uses adaptivity with dynamical meshes to solve
//  the time-dependent Richard's equation. The time discretization 
//  is backward Euler or Crank-Nicolson, and the Newton's method 
//  is applied to solve the nonlinear problem in each time step. 
//
//  PDE: C(h)dh/dt - div(K(h)grad(h)) - (dK/dh)*(dh/dy) = 0
//  where K(h) = K_S*exp(alpha*h)                          for h < 0,
//        K(h) = K_S                                       for h >= 0,
//        C(h) = alpha*(theta_s - theta_r)*exp(alpha*h)    for h < 0,
//        C(h) = alpha*(theta_s - theta_r)                 for h >= 0.
//
//  Domain: rectangle (0, 8) x (0, 6.5).
//
//  BC: Dirichlet, given by the initial condition.
//  IC: See the function init_cond().
//
//  The parameters are located in definitions.h.

// Use van Genuchten's constitutive relations, or Gardner's.
// #define CONSTITUTIVE_GENUCHTEN

// Initial polynomial degree of all mesh elements.
const int P_INIT = 1;                             
// Number of initial uniform mesh refinements.
const int INIT_REF_NUM = 0;                       
// Number of initial mesh refinements towards the top edge.
const int INIT_REF_NUM_BDY = 0;                   
// 1... implicit Euler, 2... Crank-Nicolson.
const int TIME_INTEGRATION = 2;                   

// Adaptivity
// Every UNREF_FREQth time step the mesh is unrefined.
const int UNREF_FREQ = 1;                         
// 1... mesh reset to basemesh and poly degrees to P_INIT.   
// 2... one ref. layer shaved off, poly degrees reset to P_INIT.
// 3... one ref. layer shaved off, poly degrees decreased by one. 
const int UNREF_METHOD = 3;                       
// This is a quantitative parameter of the adapt(...) function and
// it has different meanings for various adaptive strategies.
const double THRESHOLD = 0.3;                     
// Adaptive strategy:
// STRATEGY = 0 ... refine elements until sqrt(THRESHOLD) times total
//   error is processed. If more elements have similar errors, refine
//   all to keep the mesh symmetric.
// STRATEGY = 1 ... refine all elements whose error is larger
//   than THRESHOLD times maximum element error.
// STRATEGY = 2 ... refine all elements whose error is larger
//   than THRESHOLD.
const int STRATEGY = 1;                           
// Predefined list of element refinement candidates. Possible values are
// H2D_P_ISO, H2D_P_ANISO, H2D_H_ISO, H2D_H_ANISO, H2D_HP_ISO,
// H2D_HP_ANISO_H, H2D_HP_ANISO_P, H2D_HP_ANISO.
const CandList CAND_LIST = H2D_HP_ANISO;          
// Maximum allowed level of hanging nodes:
// MESH_REGULARITY = -1 ... arbitrary level hangning nodes (default),
// MESH_REGULARITY = 1 ... at most one-level hanging nodes,
// MESH_REGULARITY = 2 ... at most two-level hanging nodes, etc.
// Note that regular meshes are not supported, this is due to
// their notoriously bad performance.
const int MESH_REGULARITY = -1;                   
// This parameter influences the selection of
// candidates in hp-adaptivity. Default value is 1.0. 
const double CONV_EXP = 1.0;                      
// Stopping criterion for adaptivity.
const double ERR_STOP = 0.5;                      
// Adaptivity process stops when the number of degrees of freedom grows
// over this limit. This is to prevent h-adaptivity to go on forever.
const int NDOF_STOP = 60000;                      

// Newton's method
// Stopping criterion for Newton on fine mesh.
const double NEWTON_TOL = 0.0005;                 
// Maximum allowed number of Newton iterations.
const int NEWTON_MAX_ITER = 50;                   

// Maximum value, used in function q_function(); 
double Q_MAX_VALUE = 0.07;                        
double q_function() {
  if (STARTUP_TIME > TIME) return Q_MAX_VALUE * TIME / STARTUP_TIME;
  else return Q_MAX_VALUE;
}

double STORATIVITY = 0.05;

// Material properties.
bool is_in_mat_1(double x, double y) {
  if (y >= -0.5) return true;
  else return false; 
}

bool is_in_mat_2(double x, double y) {
  if (y >= -1.0 && y < -0.5) return true;
  else return false; 
}

bool is_in_mat_4(double x, double y) {
  if (x >= 1.0 && x <= 3.0 && y >= -2.5 && y < -1.5) return true;
  else return false; 
}

bool is_in_mat_3(double x, double y) {
  if (!is_in_mat_1(x, y) && !is_in_mat_2(x, y) && !is_in_mat_4(x, y)) return true;
  else return false; 
}

#ifdef CONSTITUTIVE_GENUCHTEN
#include "constitutive_genuchten.cpp"
#else
#include "constitutive_gardner.cpp"
#endif

// Initial condition.
double init_cond(double x, double y, double& dx, double& dy) {
  dx = 0;
  dy = -1;
  return -y + H_INIT;
}

// Essential (Dirichlet) boundary condition values.
double essential_bc_values(double x, double y, double time)
{
  if (STARTUP_TIME > time) return -y + H_INIT + time/STARTUP_TIME*H_ELEVATION;
  else return -y + H_INIT + H_ELEVATION;
}

int main(int argc, char* argv[])
{
  // Time measurement.
  TimePeriod cpu_time;

  cpu_time.tick();
  // Load the mesh.
  MeshSharedPtr mesh, basemesh;
  MeshReaderH2D mloader;
  mloader.load("domain.mesh", &basemesh);

  // Perform initial mesh refinements.
  mesh->copy(&basemesh);
  for(int i = 0; i < INIT_REF_NUM; i++) mesh->refine_all_elements();
  mesh->refine_towards_boundary(BDY_3, INIT_REF_NUM_BDY);

  // Initialize boundary conditions.
  BCTypes bc_types;
  bc_types.add_bc_dirichlet(BDY_3);
  bc_types.add_bc_neumann({BDY_2, BDY_5});
  bc_types.add_bc_newton({BDY_1, BDY_4, BDY_6});

  // Enter Dirichlet boundary values.
  BCValues bc_values(&TIME);
  bc_values.add_timedep_function(BDY_3, essential_bc_values);

SpaceSharedPtr<double> space(new // Create an H1 space with default shapeset.
  H1Space<double>(mesh, &bc_types, &bc_values, P_INIT));
  int ndof = Space<double>::get_num_dofs(&space);
  info("ndof = %d.", ndof);

SpaceSharedPtr<double> init_space(&basemesh, &bc_types, &bc_values, P_INIT);

  // Create a selector which will select optimal candidate.
  H1ProjBasedSelector<double> selector(CAND_LIST, CONV_EXP, H2DRS_DEFAULT_ORDER);

  // Solutions for the time stepping and the Newton's method.
  Solution<double> sln, ref_sln, sln_prev_time;
  
  // Initialize views.
  char title_init[200];
  sprintf(title_init, "Projection of initial condition");
  ScalarView* view_init = new ScalarView(title_init, new WinGeom(0, 0, 410, 300));
  sprintf(title_init, "Initial mesh");
  OrderView* ordview_init = new OrderView(title_init, new WinGeom(420, 0, 350, 300));
  view_init->fix_scale_width(80);

  // Adapt mesh to represent initial condition with given accuracy.
  info(new // Create an H1 space for the initial coarse mesh solution.
  H1Space<double>("Mesh adaptivity to an exact function:"));
  int as = 1; bool done = false;
  do
  {
    // Setup space for the reference solution.
    Space<double>*rspace = Space<double>::construct_refined_space(init_space);

    // Assign the function f() to the fine mesh.
    ref_sln.set_exact(rspace->get_mesh(), init_cond);

    // Project the function f() on the coarse mesh.
    OGProjection<double>::project_global(init_space, ref_sln, sln_prev_time);

    // Calculate element errors and total error estimate.
    Adapt adaptivity(init_space);
    double err_est_rel = adaptivity.calc_err_est(sln_prev_time, &ref_sln) * 100;

    info("Step %d, ndof %d, proj_error %g%%", as, Space<double>::get_num_dofs(init_space), err_est_rel);

    // If err_est_rel too large, adapt the mesh.
    if (err_est_rel < ERR_STOP) done = true;
    else {
      double to_be_processed = 0;
      done = adaptivity.adapt(&selector, THRESHOLD, STRATEGY, MESH_REGULARITY, to_be_processed);

      if (Space<double>::get_num_dofs(init_space) >= NDOF_STOP) done = true;

      view_init->show(sln_prev_time);
      char title_init[100];
      sprintf(title_init, "Initial mesh, step %d", as);
      ordview_init->set_title(title_init);
      ordview_init->show(init_space);
    }
    as++;
  }
  while (done == false);
  
  // Initialize the weak formulation.
  WeakFormSharedPtr<double> wf(new CustomWeakForm);

  // Error estimate and discrete problem size as a function of physical time.
  SimpleGraph graph_time_err_est, graph_time_err_exact, graph_time_dof, graph_time_cpu;
 
  // Visualize the projection and mesh.
  ScalarView view("Initial condition", new WinGeom(0, 0, 440, 350));
  OrderView ordview("Initial mesh", new WinGeom(450, 0, 400, 350));
  view.show(sln_prev_time);
  ordview.show(&space);

  // Time stepping loop.
  int num_time_steps = (int)(T_FINAL/TAU + 0.5);
  for(int ts = 1; ts <= num_time_steps; ts++)
  {
    // Time measurement.
    cpu_time.tick();

    // Updating current time.
    TIME = ts*TAU;
    info("---- Time step %d:", ts);

    // Periodic global derefinement.
    if (ts > 1 && ts % UNREF_FREQ == 0) 
    {
      info("Global mesh derefinement.");
      switch (UNREF_METHOD) {
        case 1: mesh->copy(&basemesh);
                space.set_uniform_order(P_INIT);
                break;
        case 2: mesh->unrefine_all_elements();
                space.set_uniform_order(P_INIT);
                break;
        case 3: mesh->unrefine_all_elements();
                //space.adjust_element_order(-1, P_INIT);
                space.adjust_element_order(-1, -1, P_INIT, P_INIT);
                break;
        default: error("Wrong global derefinement method.");
      }

      ndof = Space<double>::get_num_dofs(&space);
    }

    // Spatial adaptivity loop. Note; sln_prev_time must not be changed 
    // during spatial adaptivity.
    bool done = false;
    int as = 1;
    do
    {
      info("---- Time step %d, adaptivity step %d:", ts, as);

      // Construct globally refined reference mesh
      // and setup reference space.
      Space<double>* ref_space = Space<double>::construct_refined_space(&space);

      double* coeff_vec = new double[ref_space->get_num_dofs()];
     
      // Calculate initial coefficient vector for Newton on the fine mesh.
      if (as == 1 && ts == 1) {
        info("Projecting coarse mesh solution to obtain initial vector on new fine mesh.");
        OGProjection<double>::project_global(ref_space, sln_prev_time, coeff_vec);
      }
      else {
        info("Projecting previous fine mesh solution to obtain initial vector on new fine mesh.");
        OGProjection<double>::project_global(ref_space, ref_sln, coeff_vec);
        delete ref_sln.get_mesh();
      }

      // Initialize the FE problem.
      bool is_linear = false;
      DiscreteProblem<double> dp(wf, ref_space, is_linear);

      // Perform Newton's iteration.
      info("Solving nonlinear problem:");
      bool verbose = true;
      if (!solve_newton(coeff_vec, &dp,
          NEWTON_TOL, NEWTON_MAX_ITER, verbose)) error("Newton's iteration failed.");

      // Translate the resulting coefficient vector into the actual solutions. 
      Solution<double>::vector_to_solution(newton.get_sln_vector(), ref_space, ref_sln);

      // Project the fine mesh solution on the coarse mesh.
      info("Projecting fine mesh solution on coarse mesh for error calculation.");
      OGProjection<double>::project_global(space, &ref_sln, sln);

      // Calculate element errors.
      info("Calculating error estimate."); 
      Adapt<double>* adaptivity = new Adapt<double>(&space, HERMES_H1_NORM);
      
      // Calculate error estimate wrt. fine mesh solution.
      double err_est_rel = adaptivity->calc_err_est(sln, &ref_sln) * 100;

      // Report results.
      info("ndof_coarse: %d, ndof_fine: %d, space_err_est_rel: %g%%", 
        Space<double>::get_num_dofs(space), Space<double>::get_num_dofs(ref_space), err_est_rel);

      // Add entries to convergence graphs.
      graph_time_err_est.add_values(ts*TAU, err_est_rel);
      graph_time_err_est.save("time_err_est.dat");
      graph_time_dof.add_values(ts*TAU, Space<double>::get_num_dofs(&space));
      graph_time_dof.save("time_dof.dat");
      graph_time_cpu.add_values(ts*TAU, cpu_time.accumulated());
      graph_time_cpu.save("time_cpu.dat");

      // If space_err_est too large, adapt the mesh.
      if (err_est_rel < ERR_STOP) done = true;
      else {
        info("Adapting coarse mesh.");
        done = adaptivity->adapt(&selector, THRESHOLD, STRATEGY, MESH_REGULARITY);
        if (Space<double>::get_num_dofs(&space) >= NDOF_STOP) {
          done = true;
          break;
        }
        as++;
      }

      // Cleanup.
      delete [] coeff_vec;
      delete adaptivity;
      delete ref_space;
    }
    while (!done);

    // Visualize the solution and mesh.
    char title[100];
    sprintf(title, "Solution, time level %d", ts);
    view.set_title(title);
    view.show(sln);
    sprintf(title, "Mesh, time level %d", ts);
    ordview.set_title(title);
    ordview.show(&space);

    // Copy new time level solution into sln_prev_time.
    sln_prev_time.copy(ref_sln);
  }

  // Wait for all views to be closed.
  View::wait();
  return 0;
}
