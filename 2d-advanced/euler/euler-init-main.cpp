#pragma region 1. Load mesh and initialize spaces.
// Load the mesh.
MeshSharedPtr mesh(new Mesh);
MeshReaderH2D mloader;
mloader.load(MESH_FILENAME, mesh);

// Perform initial mesh refinements.
for (int i = 0; i < INIT_REF_NUM; i++)
	mesh->refine_all_elements(0, true);

// Initialize boundary condition types and spaces with default shapesets.
SpaceSharedPtr<double> space_rho(new L2Space<double>(mesh, P_INIT, (SHOCK_CAPTURING && SHOCK_CAPTURING_TYPE == KUZMIN ? (Shapeset*)new L2ShapesetTaylor : (Shapeset*)new L2Shapeset)));
SpaceSharedPtr<double> space_rho_v_x(new L2Space<double>(mesh, P_INIT, (SHOCK_CAPTURING && SHOCK_CAPTURING_TYPE == KUZMIN ? (Shapeset*)new L2ShapesetTaylor : (Shapeset*)new L2Shapeset)));
SpaceSharedPtr<double> space_rho_v_y(new L2Space<double>(mesh, P_INIT, (SHOCK_CAPTURING && SHOCK_CAPTURING_TYPE == KUZMIN ? (Shapeset*)new L2ShapesetTaylor : (Shapeset*)new L2Shapeset)));
SpaceSharedPtr<double> space_e(new L2Space<double>(mesh, P_INIT, (SHOCK_CAPTURING && SHOCK_CAPTURING_TYPE == KUZMIN ? (Shapeset*)new L2ShapesetTaylor : (Shapeset*)new L2Shapeset)));
SpaceSharedPtr<double> space_stabilization(new L2Space<double>(mesh, 0));
std::vector<SpaceSharedPtr<double> > spaces({ space_rho, space_rho_v_x, space_rho_v_y, space_e });
int ndof = Space<double>::get_num_dofs(spaces);
Hermes::Mixins::Loggable::Static::info("ndof: %d", ndof);
#pragma endregion

#pragma region 2. Initialize solutions.
MeshFunctionSharedPtr<double> sln_rho(new Solution<double>(mesh));
MeshFunctionSharedPtr<double> sln_rho_v_x(new Solution<double>(mesh));
MeshFunctionSharedPtr<double> sln_rho_v_y(new Solution<double>(mesh));
MeshFunctionSharedPtr<double> sln_e(new Solution<double>(mesh));
std::vector<MeshFunctionSharedPtr<double> > slns({ sln_rho, sln_rho_v_x, sln_rho_v_y, sln_e });

// Set up CFL calculation class.
CFLCalculation CFL(CFL_NUMBER, KAPPA);
#pragma endregion
