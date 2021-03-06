#include "definitions.h"

using namespace RefinementSelectors;

// This example is an analogy to the 2D tutorial example P04-adapt/03-system.
// It explains how to use the multimesh adaptive hp-FEM for 1D problems whose
// solution components behave very differently, so that using the same mesh
// for both is a waste of DOFs.
//
// PDE: Linearized FitzHugh-Nagumo equation
//      -d_u^2 u'' - f(u) + \sigma v - g1 = 0,
//      -d_v^2 v'' - u + v - g2 = 0.
// In the original equation, f(u) = \lambda u - u^3 - \kappa. For
// simplicity, here we just take f(u) = u.
//
// Domain: Interval (-1, 1).
//
// BC: Both solution components are zero on the boundary.
//
// Exact solution: The functions g1 and g2 were calculated so that
//                 the exact solution is:
//        U(x) = cos(M_PI*x/2)
//        V(x) = 1 - (exp(K*x) + exp(-K*x)) / (exp(K) + exp(-K))
// Note: V(x) is the exact solution of the 1D singularly perturbed equation
//       -u'' + K*K*u = K*K in (-1, 1) with zero Dirichlet BC.
//
// The following parameters can be changed: In particular, compare hp- and
// h-adaptivity via the CAND_LIST option, and compare the multi-mesh vs.
// single-mesh using the MULTI parameter.

// Should exact solution be used and exact error calculated?
#define WITH_EXACT_SOLUTION
// Initial polynomial degree for u.
const int P_INIT_U = 1;
// Initial polynomial degree for v.
const int P_INIT_V = 1;
// Number of initial boundary refinements
const int INIT_REF_BDY = 5;
// MULTI = true  ... use multi-mesh,
// MULTI = false ... use single-mesh->
// Note: In the single mesh option, the meshes are
// forced to be geometrically the same but the
// polynomial degrees can still vary.
const bool MULTI = true;
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

// Problem parameters.
const double D_u = 1;
const double D_v = 1;
const double SIGMA = 1;
const double LAMBDA = 1;
const double KAPPA = 1;
const double K = 100.;

int main(int argc, char* argv[])
{
  // Time measurement.
  Hermes::Mixins::TimeMeasurable cpu_time;
  cpu_time.tick();

  // Load the mesh.
  MeshSharedPtr u_mesh(new Mesh), v_mesh(new Mesh);
  MeshReaderH1DXML mloader;
  mloader.load("domain.xml", u_mesh);
  u_mesh->refine_all_elements();

  if (MULTI == false)
  {
    u_mesh->refine_towards_boundary("Left", INIT_REF_BDY);
    // Minus one for the sake of mesh symmetry.
    if (INIT_REF_BDY > 1) u_mesh->refine_towards_boundary("Right", INIT_REF_BDY - 1);
  }

  // Create initial mesh (master mesh).
  v_mesh->copy(u_mesh);

  // Initial mesh refinements in the v_mesh towards the boundary.
  if (MULTI == true)
  {
    v_mesh->refine_towards_boundary("Left", INIT_REF_BDY);
    // Minus one for the sake of mesh symmetry.
    if (INIT_REF_BDY > 1) v_mesh->refine_towards_boundary("Right", INIT_REF_BDY - 1);
  }

#ifdef WITH_EXACT_SOLUTION
  // Set exact solutions.
  MeshFunctionSharedPtr<double> exact_u(new ExactSolutionFitzHughNagumo1(u_mesh));
  MeshFunctionSharedPtr<double> exact_v(new ExactSolutionFitzHughNagumo2(v_mesh, K));
#endif

  // Define right-hand sides.
  CustomRightHandSide1 g1(K, D_u, SIGMA);
  CustomRightHandSide2 g2(K, D_v);

  // Initialize the weak formulation.
  WeakFormSharedPtr<double> wf(new CustomWeakForm(&g1, &g2));

  // Initialize boundary conditions
  DefaultEssentialBCConst<double> bc_u(std::vector<std::string>({ "Left", "Right" }), 0.0);
  EssentialBCs<double> bcs_u(&bc_u);
  DefaultEssentialBCConst<double> bc_v(std::vector<std::string>({ "Left", "Right" }), 0.0);
  EssentialBCs<double> bcs_v(&bc_v);

  // Create H1 spaces with default shapeset for both displacement components.
  SpaceSharedPtr<double>  u_space(new H1Space<double>(u_mesh, &bcs_u, P_INIT_U));
  SpaceSharedPtr<double>  v_space(new H1Space<double>(MULTI ? v_mesh : u_mesh, &bcs_v, P_INIT_V));

  // Initialize coarse and reference mesh solutions.
  MeshFunctionSharedPtr<double> u_sln(new Solution<double>), v_sln(new Solution<double>), u_ref_sln(new Solution<double>), v_ref_sln(new Solution<double>);

  // Initialize refinement selector.
  H1ProjBasedSelector<double> selector(CAND_LIST);

  // Initialize views.
  Views::ScalarView s_view_0("Solution[0]", new Views::WinGeom(0, 0, 440, 350));
  s_view_0.show_mesh(false);
  Views::OrderView  o_view_0("Mesh[0]", new Views::WinGeom(450, 0, 420, 350));
  Views::ScalarView s_view_1("Solution[1]", new Views::WinGeom(880, 0, 440, 350));
  s_view_1.show_mesh(false);
  Views::OrderView o_view_1("Mesh[1]", new Views::WinGeom(1330, 0, 420, 350));

  // DOF and CPU convergence graphs.
  SimpleGraph graph_dof_est, graph_cpu_est;
#ifdef WITH_EXACT_SOLUTION
  SimpleGraph graph_dof_exact, graph_cpu_exact;
#endif

  // Adaptivity loop:
  int as = 1;
  bool done = false;
  do
  {
    Hermes::Mixins::Loggable::Static::info("---- Adaptivity step %d:", as);

    // Construct globally refined reference mesh and setup reference space.
    // FIXME: This should be increase in the x-direction only.
    int order_increase = 1;
    // FIXME: This should be '2' but that leads to a segfault.
    int refinement_type = 0;

    Mesh::ReferenceMeshCreator refMeshCreator1(u_mesh);
    MeshSharedPtr ref_u_mesh = refMeshCreator1.create_ref_mesh();

    Space<double>::ReferenceSpaceCreator refSpaceCreator1(u_space, ref_u_mesh);
    SpaceSharedPtr<double> ref_u_space = refSpaceCreator1.create_ref_space();

    Mesh::ReferenceMeshCreator refMeshCreator2(v_mesh);
    MeshSharedPtr ref_v_mesh = refMeshCreator2.create_ref_mesh();

    Space<double>::ReferenceSpaceCreator refSpaceCreator2(v_space, ref_v_mesh);
    SpaceSharedPtr<double> ref_v_space = refSpaceCreator2.create_ref_space();

    std::vector<SpaceSharedPtr<double> > ref_spaces({ ref_u_space, ref_v_space });

    int ndof_ref = Space<double>::get_num_dofs(ref_spaces);

    // Initialize reference problem.
    Hermes::Mixins::Loggable::Static::info("Solving on reference mesh.");
    DiscreteProblem<double> dp(wf, ref_spaces);

    NewtonSolver<double> newton(&dp);
    newton.set_verbose_output(true);

    // Time measurement.
    cpu_time.tick();

    // Perform Newton's iteration.
    try
    {
      newton.solve();
    }
    catch (Hermes::Exceptions::Exception e)
    {
      e.print_msg();
      throw Hermes::Exceptions::Exception("Newton's iteration failed.");
    };

    // Translate the resulting coefficient vector into the Solution<double> sln->
    Solution<double>::vector_to_solutions(newton.get_sln_vector(), ref_spaces,
      std::vector<MeshFunctionSharedPtr<double> >({ u_ref_sln, v_ref_sln }));

    // Project the fine mesh solution onto the coarse mesh.
    Hermes::Mixins::Loggable::Static::info("Projecting reference solutions on coarse meshes.");
    OGProjection<double>::project_global({ u_space, v_space },
      std::vector<MeshFunctionSharedPtr<double> >({ u_ref_sln, v_ref_sln }),
      std::vector<MeshFunctionSharedPtr<double> >({ u_sln, v_sln }));

    cpu_time.tick();

    // View the coarse mesh solution and polynomial orders.
    s_view_0.show(u_ref_sln);
    o_view_0.show(u_space);
    s_view_1.show(v_ref_sln);
    o_view_1.show(v_space);

    // Calculate element errors.
    Hermes::Mixins::Loggable::Static::info("Calculating error estimate and exact error.");
    adaptivity.set_spaces({ u_space, v_space });

#ifdef WITH_EXACT_SOLUTION
    errorCalculator.calculate_errors({ u_sln, v_sln }, { exact_u, exact_v }, false);
    double err_exact_rel_total = errorCalculator.get_total_error_squared() * 100;
#endif

    errorCalculator.calculate_errors({ u_sln, v_sln }, { u_ref_sln, v_ref_sln }, true);
    double err_est_rel_total = errorCalculator.get_total_error_squared() * 100;

    // Time measurement.
    cpu_time.tick();

    // Report results.
#ifdef WITH_EXACT_SOLUTION
    Hermes::Mixins::Loggable::Static::info("ndof_coarse[0]: %d, ndof_fine[0]: %d",
      u_space->get_num_dofs(), ref_u_space->get_num_dofs());
    Hermes::Mixins::Loggable::Static::info("ndof_coarse[1]: %d, ndof_fine[1]: %d",
      v_space->get_num_dofs(), ref_v_space->get_num_dofs());
    Hermes::Mixins::Loggable::Static::info("ndof_coarse_total: %d, ndof_fine_total: %d",
      Space<double>::get_num_dofs({ u_space, v_space }),
      Space<double>::get_num_dofs(ref_spaces));
    Hermes::Mixins::Loggable::Static::info("err_est_rel_total: %g%%, err_est_exact_total: %g%%", err_est_rel_total, err_exact_rel_total);
#else
    Hermes::Mixins::Loggable::Static::info("ndof_coarse[0]: %d, ndof_fine[0]: %d", u_space->get_num_dofs(), u_ref_space->get_num_dofs());
    Hermes::Mixins::Loggable::Static::info("err_est_rel[0]: %g%%", err_est_rel[0] * 100);
    Hermes::Mixins::Loggable::Static::info("ndof_coarse[1]: %d, ndof_fine[1]: %d", v_space->get_num_dofs(), v_ref_space->get_num_dofs());
    Hermes::Mixins::Loggable::Static::info("err_est_rel[1]: %g%%", err_est_rel[1] * 100);
    Hermes::Mixins::Loggable::Static::info("ndof_coarse_total: %d, ndof_fine_total: %d",
      Space<double>::get_num_dofs({ u_space, v_space }),
      Space<double>::get_num_dofs(*ref_spaces));
    Hermes::Mixins::Loggable::Static::info("err_est_rel_total: %g%%", err_est_rel_total);
#endif

    // Add entry to DOF and CPU convergence graphs.
    graph_dof_est.add_values(Space<double>::get_num_dofs({ u_space, v_space }),
      err_est_rel_total);
    graph_dof_est.save("conv_dof_est.dat");
    graph_cpu_est.add_values(cpu_time.accumulated(), err_est_rel_total);
    graph_cpu_est.save("conv_cpu_est.dat");
#ifdef WITH_EXACT_SOLUTION
    graph_dof_exact.add_values(Space<double>::get_num_dofs({ u_space, v_space }),
      err_exact_rel_total);
    graph_dof_exact.save("conv_dof_exact.dat");
    graph_cpu_exact.add_values(cpu_time.accumulated(), err_exact_rel_total);
    graph_cpu_exact.save("conv_cpu_exact.dat");
#endif

    // If err_est too large, adapt the mesh.
    if (err_est_rel_total < ERR_STOP)
      done = true;
    else
    {
      Hermes::Mixins::Loggable::Static::info("Adapting coarse mesh.");
      done = adaptivity.adapt({ &selector, &selector });
    }

    // Increase counter.
    as++;
  } while (done == false);

  Hermes::Mixins::Loggable::Static::info("Total running time: %g s", cpu_time.accumulated());

  // Wait for all views to be closed.
  Views::View::wait();
  return 0;
}