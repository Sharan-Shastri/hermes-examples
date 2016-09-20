	std::vector<MeshFunctionSharedPtr<double> > prev_slns({ prev_rho, prev_rho_v_x, prev_rho_v_y, prev_e });

#pragma region 4. Filters for visualization of Mach number, pressure + visualization setup.
	MeshFunctionSharedPtr<double>  Mach_number(new MachNumberFilter(prev_slns, KAPPA));
	MeshFunctionSharedPtr<double>  pressure(new PressureFilter(prev_slns, KAPPA));

	ScalarView pressure_view("Pressure", new WinGeom(0, 0, 600, 300));
	ScalarView Mach_number_view("Mach number", new WinGeom(650, 0, 600, 300));
	ScalarView eview("Error - density", new WinGeom(0, 330, 600, 300));
	ScalarView eview1("Error - momentum", new WinGeom(0, 660, 600, 300));
	OrderView order_view("Orders", new WinGeom(650, 330, 600, 300));
#pragma endregion

#pragma region 5. Some stabilization approaches.
	WeakFormSharedPtr<double> wf_stabilization(new EulerEquationsWeakFormStabilization(prev_rho));
	DiscreteProblem<double> dp_stabilization(wf_stabilization, space_stabilization);

	LinearSolver<double> solver(wf, spaces);
	EulerEquationsWeakFormSemiImplicit* wf_ptr = (EulerEquationsWeakFormSemiImplicit*)(wf.get());

	Vector<double>* rhs_stabilization = create_vector<double>(HermesCommonApi.get_integral_param_value(matrixSolverType));

	if (SHOCK_CAPTURING && SHOCK_CAPTURING_TYPE == FEISTAUER)
		wf_ptr->set_stabilization(prev_rho, prev_rho_v_x, prev_rho_v_y, prev_e, NU_1, NU_2);
#pragma endregion

#pragma region 6. Time stepping loop.
	int iteration = 0;
	for (double t = 0.0; t < TIME_INTERVAL_LENGTH; t += time_step_n)
	{
		// Info.
		Hermes::Mixins::Loggable::Static::info("---- Time step %d, time %3.5f.", iteration++, t);

		if (SHOCK_CAPTURING && SHOCK_CAPTURING_TYPE == FEISTAUER)
		{
			int mesh_size = space_stabilization->get_num_dofs();
			assert(mesh_size == space_stabilization->get_mesh()->get_num_active_elements());
			dp_stabilization.assemble(rhs_stabilization);
			if (!wf_ptr->discreteIndicator)
			{
				wf_ptr->set_discreteIndicator(new bool[mesh_size], mesh_size);
				memset(wf_ptr->discreteIndicator, 0, mesh_size * sizeof(bool));
			}
			Element* e;
			for_all_active_elements(e, space_stabilization->get_mesh())
			{
				AsmList<double> al;
				space_stabilization->get_element_assembly_list(e, &al);
				if (rhs_stabilization->get(al.get_dof()[0]) >= 1)
					wf_ptr->discreteIndicator[e->id] = true;
			}
		}

		// Set the current time step.
		wf_ptr->set_current_time_step(time_step_n);

#pragma region *. Get the solution with optional shock capturing.
		try
		{
			// Solve.
			solver.solve();

			if (!SHOCK_CAPTURING || (P_INIT == 0))
				Solution<double>::vector_to_solutions(solver.get_sln_vector(), spaces, prev_slns);
			else
			{
				if (SHOCK_CAPTURING_TYPE == KRIVODONOVA)
				{
					FluxLimiter* flux_limiter = new FluxLimiter(FluxLimiter::Krivodonova, solver.get_sln_vector(), spaces);
					flux_limiter->limit_according_to_detector();
					flux_limiter->get_limited_solutions(prev_slns);
					delete flux_limiter;
				}

				if (SHOCK_CAPTURING_TYPE == KUZMIN && P_INIT > 0)
				{
					if (P_INIT != 1)
						throw new Hermes::Exceptions::Exception("P_INIT must be <= 1.");

					PostProcessing::VertexBasedLimiter limiter(spaces, solver.get_sln_vector(), P_INIT);
					limiter.get_solutions(prev_slns);

					int running_dofs = 0;
					int ndof = spaces[0]->get_num_dofs();
					double* density_sln_vector = limiter.get_solution_vector();
					Element* e;
					AsmList<double> al_density;
					for (int component = 1; component < 4; component++)
					{
						if (spaces[component]->get_num_dofs() != ndof)
							throw Exceptions::Exception("Euler code is supposed to be executed on a single mesh.");

						double* conservative_vector = limiter.get_solution_vector() + component * ndof;
						double* real_vector = new double[ndof];
						memset(real_vector, 0, sizeof(double) * ndof);

						for_all_active_elements(e, spaces[0]->get_mesh())
						{
							spaces[0]->get_element_assembly_list(e, &al_density);

							real_vector[al_density.dof[0]] = conservative_vector[al_density.dof[0]] / density_sln_vector[al_density.dof[0]];
							real_vector[al_density.dof[1]] = (conservative_vector[al_density.dof[1]] - real_vector[al_density.dof[0]] * density_sln_vector[al_density.dof[1]]) / density_sln_vector[al_density.dof[0]];
							real_vector[al_density.dof[2]] = (conservative_vector[al_density.dof[2]] - real_vector[al_density.dof[0]] * density_sln_vector[al_density.dof[2]]) / density_sln_vector[al_density.dof[0]];
						}

						PostProcessing::VertexBasedLimiter real_component_limiter(spaces[0], real_vector, P_INIT);
						real_component_limiter.get_solution();
						real_vector = real_component_limiter.get_solution_vector();

						for_all_active_elements(e, spaces[0]->get_mesh())
						{
							spaces[0]->get_element_assembly_list(e, &al_density);

							conservative_vector[al_density.dof[1]] = density_sln_vector[al_density.dof[0]] * real_vector[al_density.dof[1]]
								+ density_sln_vector[al_density.dof[1]] * real_vector[al_density.dof[0]];

							conservative_vector[al_density.dof[2]] = density_sln_vector[al_density.dof[0]] * real_vector[al_density.dof[2]]
								+ density_sln_vector[al_density.dof[2]] * real_vector[al_density.dof[0]];
						}

						Solution<double>::vector_to_solution(conservative_vector, spaces[0], prev_slns[component]);
					}
				}
			}
			CFL.calculate(prev_slns, mesh, time_step_n);
		}
		catch (std::exception& e) { std::cout << e.what(); }
#pragma endregion

#pragma region *. Visualization
		if ((iteration - 1) % EVERY_NTH_STEP == 0)
		{
			// Hermes visualization.
			if (HERMES_VISUALIZATION)
			{
				Mach_number->reinit();
				pressure->reinit();
				pressure_view.show(prev_rho, 1);
				Mach_number_view.show(Mach_number, 1);
				order_view.show(space_rho);
			}
			// Output solution in VTK format.
			if (VTK_VISUALIZATION)
			{
				pressure->reinit();
				Linearizer lin(FileExport);
				char filename[40];
				sprintf(filename, "Pressure-%i.vtk", iteration - 1);
				lin.save_solution_vtk(pressure, filename, "Pressure", false);
				sprintf(filename, "VelocityX-%i.vtk", iteration - 1);
				lin.save_solution_vtk(prev_rho_v_x, filename, "VelocityX", false);
				sprintf(filename, "VelocityY-%i.vtk", iteration - 1);
				lin.save_solution_vtk(prev_rho_v_y, filename, "VelocityY", false);
				sprintf(filename, "Rho-%i.vtk", iteration - 1);
				lin.save_solution_vtk(prev_rho, filename, "Rho", false);
			}
		}
#pragma endregion
	}
#pragma endregion
	return 0;