#include "definitions.h"

using namespace RefinementSelectors;
using namespace Views;

//  This is another example that allows you to compare h- and hp-adaptivity from the point of view
//  of both CPU time requirements and discrete problem size, look at the quality of the a-posteriori
//  error estimator used by Hermes (exact error is provided), etc. You can also change
//  the parameter MESH_REGULARITY to see the influence of hanging nodes on the adaptive process.
//  The problem is made harder for adaptive algorithms by increasing the parameter SLOPE.
//
//  PDE: -Laplace u + f = 0
//
//  Known exact solution.
//
//  Domain: unit square (0, 1) x (0, 1), see the file square.mesh->
//
//  BC:  Dirichlet, given by exact solution.
//
//  The following parameters can be changed:

// Initial polynomial degree of mesh elements.
const int P_INIT = 2;
// Number of initial uniform mesh refinements.
const int INIT_REF_NUM = 1;
// This is a quantitative parameter of the adapt(...) function and
// it has different meanings for various adaptive strategies.
const double THRESHOLD = 0.6;
// Error calculation & adaptivity.
DefaultErrorCalculator<double, HERMES_H1_NORM> errorCalculator(RelativeErrorToGlobalNorm, 1);
// Stopping criterion for an adaptivity step.
AdaptStoppingCriterionSingleElement<double> stoppingCriterion(THRESHOLD);
// Adaptivity processor class.
Adapt<double> adaptivity(&errorCalculator, &stoppingCriterion);
// Predefined list of element refinement candidates.
const CandList CAND_LIST = H2D_HP_ANISO;
// Stopping criterion for adaptivity.
const double ERR_STOP = 1e-5;

// Problem parameters.
// Slope of the layer.
double slope = 60;

int main(int argc, char* argv[])
{
	// Load the mesh.
	MeshSharedPtr mesh(new Mesh);
	MeshReaderH2D mloader;
	// Quadrilaterals.
	mloader.load("square_quad.mesh", mesh);
	// Triangles.
	// mloader.load("square_tri.mesh", mesh);

	MeshView m;
	m.show(mesh);
	m.save_screenshot("initialmesh.bmp", true);

	// Perform initial mesh refinements.
	for (int i = 0; i < INIT_REF_NUM; i++) mesh->refine_all_elements();

	// Define exact solution.
	MeshFunctionSharedPtr<double> exact_sln(new CustomExactSolution(mesh, slope));

	// Define custom function f.
	CustomFunction f(slope);

	// Initialize the weak formulation.
	Hermes::Hermes1DFunction<double> lambda(1.0);
	WeakFormSharedPtr<double> wf(new DefaultWeakFormPoisson<double>(HERMES_ANY, &lambda, &f));

	// Initialize boundary conditions
	DefaultEssentialBCNonConst<double> bc_essential("Bdy", exact_sln);
	EssentialBCs<double> bcs(&bc_essential);

	// Create an H1 space with default shapeset.
	SpaceSharedPtr<double> space(new H1Space<double>(mesh, &bcs, P_INIT));

	// Initialize approximate solution.
	MeshFunctionSharedPtr<double> sln(new Solution<double>());

	// Initialize refinement selector.
	H1ProjBasedSelector<double> selector(CAND_LIST);

	// Initialize views.
	Views::ScalarView sview("Solution", new Views::WinGeom(0, 0, 440, 350));
	sview.show_mesh(false);
	sview.fix_scale_width(50);
	Views::OrderView oview("Polynomial orders", new Views::WinGeom(450, 0, 420, 350));

	// DOF and CPU convergence graphs.
	SimpleGraph graph_dof_est, graph_cpu_est, graph_dof_exact, graph_cpu_exact;

	// Time measurement.
	Hermes::Mixins::TimeMeasurable cpu_time;
	cpu_time.tick();

	ScalarView s;

	// Adaptivity loop:
	int as = 1; bool done = false;
	try
	{
		do
		{
			cpu_time.tick();

			// Construct globally refined reference mesh and setup reference space.
			Mesh::ReferenceMeshCreator refMeshCreator(mesh);
			MeshSharedPtr ref_mesh = refMeshCreator.create_ref_mesh();

			Space<double>::ReferenceSpaceCreator refSpaceCreator(space, ref_mesh);
			SpaceSharedPtr<double> ref_space = refSpaceCreator.create_ref_space();
			int ndof_ref = ref_space->get_num_dofs();

			Hermes::Mixins::Loggable::Static::info("---- Adaptivity step %d (%d DOF):", as, ndof_ref);
			cpu_time.tick();

			Hermes::Mixins::Loggable::Static::info("Solving on reference mesh.");

			// Assemble the discrete problem.
			DiscreteProblem<double> dp(wf, ref_space);

			NewtonSolver<double> newton(&dp);
			//newton.set_verbose_output(false);

			MeshFunctionSharedPtr<double> ref_sln(new Solution<double>());
			newton.solve();

			// Translate the resulting coefficient vector into the instance of Solution.
			Solution<double>::vector_to_solution(newton.get_sln_vector(), ref_space, ref_sln);

			cpu_time.tick();
			Hermes::Mixins::Loggable::Static::info("Solution: %g s", cpu_time.last());

			// Project the fine mesh solution onto the coarse mesh.
			Hermes::Mixins::Loggable::Static::info("Calculating error estimate and exact error.");
			OGProjection<double>::project_global(space, ref_sln, sln);

			// Calculate element errors and total error estimate.
			adaptivity.set_space(space);
			errorCalculator.calculate_errors(sln, exact_sln, false);
			double err_exact_rel = errorCalculator.get_total_error_squared() * 100;

			errorCalculator.calculate_errors(sln, ref_sln, true);
			double err_est_rel = errorCalculator.get_total_error_squared() * 100;

			cpu_time.tick();
			Hermes::Mixins::Loggable::Static::info("Error calculation: %g s", cpu_time.last());

			// Report results.
			Hermes::Mixins::Loggable::Static::info("ndof_coarse: %d, ndof_fine: %d", space->get_num_dofs(), ref_space->get_num_dofs());
			Hermes::Mixins::Loggable::Static::info("err_est_rel: %g%%, err_exact_rel: %g%%", err_est_rel, err_exact_rel);

			// Time measurement.
			cpu_time.tick();
			double accum_time = cpu_time.accumulated();

			// View the coarse mesh solution and polynomial orders.
			sview.show(ref_sln);
			sview.save_numbered_screenshot("solution%i.bmp", as, true);
			oview.show(ref_space);
			oview.set_b_orders(true);
			oview.save_numbered_screenshot("space%i.bmp", as, true);

			// Add entry to DOF and CPU convergence graphs.
			graph_dof_est.add_values(space->get_num_dofs(), err_est_rel);
			graph_dof_est.save("conv_dof_est.dat");
			graph_cpu_est.add_values(accum_time, err_est_rel);
			graph_cpu_est.save("conv_cpu_est.dat");
			graph_dof_exact.add_values(space->get_num_dofs(), err_exact_rel);
			graph_dof_exact.save("conv_dof_exact.dat");
			graph_cpu_exact.add_values(accum_time, err_exact_rel);
			graph_cpu_exact.save("conv_cpu_exact.dat");

			cpu_time.tick(Hermes::Mixins::TimeMeasurable::HERMES_SKIP);

			// If err_est too large, adapt the mesh. The NDOF test must be here, so that the solution may be visualized
			// after ending due to this criterion.
			if (err_exact_rel < ERR_STOP)
				done = true;
			else
				done = adaptivity.adapt(&selector);

			cpu_time.tick();
			Hermes::Mixins::Loggable::Static::info("Adaptation: %g s", cpu_time.last());

			// Increase the counter of adaptivity steps.
			if (done == false)
				as++;
		} while (done == false);
	}
	catch (Hermes::Exceptions::Exception e)
	{
		e.print_msg();
		throw Hermes::Exceptions::Exception("Newton's iteration failed.");
	};

	Hermes::Mixins::Loggable::Static::info("Total running time: %g s", cpu_time.accumulated());

	// Wait for all views to be closed.
	Views::View::wait();
	return 0;
}