Iron-Water
----------

Loading mesh in ExodusII format
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This example is very similar to the example "saphir", the main difference being that 
it reads a mesh file in the exodusii format (created by Cubit). This example only builds 
if you have the `ExodusII <http://sourceforge.net/projects/exodusii/>`_ and
`NetCDF <http://www.unidata.ucar.edu/software/netcdf/>`_ libraries installed on your 
system and the variables WITH_EXODUSII, EXODUSII_ROOT and NETCDF_ROOT defined properly. 
The latter can be done, for example, in the CMake.vars file as follows:

::

    SET(WITH_EXODUSII YES)
    SET(EXODUSII_ROOT /opt/packages/exodusii)
    SET(NETCDF_ROOT   /opt/packages/netcdf)

The mesh is now loaded using the ExodusIIReader::

    // Load the mesh
    Mesh mesh;
    ExodusIIReader mloader;
    if (!mloader.load("iron-water.e", &mesh)) error("ExodusII mesh load failed.");

Problem description
~~~~~~~~~~~~~~~~~~~

The model describes an external-force-driven configuration without fissile materials present.
We will solve the one-group neutron diffusion equation

.. math::
    :label: iron-water

       -\nabla \cdot (D(x,y) \nabla \Phi) + \Sigma_a(x,y) \Phi - Q_{ext}(x,y) = 0.

The domain of interest is a 30 x 30 cm square consisting of four regions.
A uniform volumetric source is placed in water in the lower-left corner 
of the domain, surrounded with a layer of water, a layer of iron, and finally
another layer of water:

.. figure:: example-iron-water/iron-water.png
   :align: center
   :scale: 40% 
   :figclass: align-center
   :alt: Schematic picture for the iron-water example.

The unknown is the neutron flux $\Phi(x, y)$. The values of the diffusion coefficient 
$D(x, y)$, absorption cross-section $\Sigma_a(x, y)$ and the source term $Q_{ext}(x,y)$
are constant in the subdomains. The source $Q_{ext} = 1$ in area 1 and zero 
elsewhere. The boundary conditions for this problem are zero Dirichlet (right and top edges)
and zero Neumann (bottom and left edges). 

Sample results
~~~~~~~~~~~~~~

Solution:

.. figure:: example-iron-water/iron-water-sol.png
   :align: center
   :scale: 50%
   :alt: Solution to the iron-water example.


Final mesh (h-FEM with linear elements):

.. figure:: example-iron-water/iron-water-mesh-h1.png
   :align: center
   :scale: 40%
   :alt: Final finite element mesh for the iron-water example (h-FEM with linear elements).

Final mesh (h-FEM with quadratic elements):

.. figure:: example-iron-water/iron-water-mesh-h2.png
   :align: center
   :scale: 40%
   :alt: Final finite element mesh for the iron-water example (h-FEM with quadratic elements).

Final mesh (hp-FEM):

.. figure:: example-iron-water/iron-water-mesh-hp.png
   :align: center
   :scale: 40%
   :alt: Final finite element mesh for the iron-water example (hp-FEM).

DOF convergence graphs:

.. figure:: example-iron-water/conv_dof.png
   :align: center
   :scale: 50% 
   :figclass: align-center
   :alt: DOF convergence graph for example iron-water.

CPU time convergence graphs:

.. figure:: example-iron-water/conv_cpu.png
   :align: center
   :scale: 50% 
   :figclass: align-center
   :alt: CPU convergence graph for example iron-water.

