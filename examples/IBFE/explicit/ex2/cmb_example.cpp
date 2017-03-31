// Copyright (c) 2002-2014, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of The University of North Carolina nor the names of
//      its contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Config files
#include <IBAMR_config.h>
#include <IBTK_config.h>
#include <SAMRAI_config.h>

// Headers for basic PETSc functions
#include <petscsys.h>

// Headers for basic SAMRAI objects
#include <BergerRigoutsos.h>
#include <CartesianGridGeometry.h>
#include <LoadBalancer.h>
#include <StandardTagAndInitialize.h>

// Headers for basic libMesh objects
#include <libmesh/boundary_info.h>
#include <libmesh/equation_systems.h>
#include <libmesh/exodusII_io.h>
#include <libmesh/gmv_io.h>
#include <libmesh/mesh.h>
#include <libmesh/mesh_generation.h>

// Headers for application-specific algorithm/data structure objects
#include <ibamr/IBExplicitHierarchyIntegrator.h>
#include <ibamr/IBFEMethod.h>
#include <ibamr/INSCollocatedHierarchyIntegrator.h>
#include <ibamr/INSStaggeredHierarchyIntegrator.h>
#include <ibtk/AppInitializer.h>
#include <ibtk/libmesh_utilities.h>
#include <ibtk/muParserCartGridFunction.h>
#include <ibtk/muParserRobinBcCoefs.h>

// Set up application namespace declarations
#include <ibamr/app_namespaces.h>
#include <ibamr/IBAMRInit.h>
// Elasticity model data.
namespace ModelData
{
// Problem parameters.
static const double mu = 10.0;

// Stress tensor functions.
void
PK1_dev_stress_function(TensorValue<double>& PP,
                        const TensorValue<double>& FF,
                        const libMesh::Point& /*X*/,
                        const libMesh::Point& /*s*/,
                        Elem* const /*elem*/,
                        const vector<NumericVector<double>*>& /*system_data*/,
                        double /*time*/,
                        void* /*ctx*/)
{
    PP = mu * FF;
    return;
} // PK1_dev_stress_function

void
PK1_dil_stress_function(TensorValue<double>& PP,
                        const TensorValue<double>& FF,
                        const libMesh::Point& /*X*/,
                        const libMesh::Point& /*s*/,
                        Elem* const /*elem*/,
                        const vector<NumericVector<double>*>& /*system_data*/,
                        double /*time*/,
                        void* /*ctx*/)
{
    PP = -mu * tensor_inverse_transpose(FF, NDIM);
    return;
} // PK1_dil_stress_function
}
using namespace ModelData;

// Function prototypes
void output_data(Pointer<PatchHierarchy<NDIM> > patch_hierarchy,
                 Pointer<INSHierarchyIntegrator> navier_stokes_integrator,
                 Mesh& mesh,
                 EquationSystems* equation_systems,
                 const int iteration_num,
                 const double loop_time,
                 const string& data_dump_dirname);

/*******************************************************************************
 * For each run, the input filename and restart information (if needed) must   *
 * be given on the command line.  For non-restarted case, command line is:     *
 *                                                                             *
 *    executable <input file name>                                             *
 *                                                                             *
 * For restarted run, command line is:                                         *
 *                                                                             *
 *    executable <input file name> <restart directory> <restart number>        *
 *                                                                             *
 *******************************************************************************/

bool
run_example(int argc, char* argv[])
{
    // Initialize libMesh, PETSc, MPI, and SAMRAI.
    LibMeshInit init(argc, argv);
    SAMRAI_MPI::setCommunicator(PETSC_COMM_WORLD);
    SAMRAI_MPI::setCallAbortInSerialInsteadOfExit();
    SAMRAIManager::startup();

    { // cleanup dynamically allocated objects prior to shutdown

        // Parse command line options, set some standard options from the input
        // file, initialize the restart database (if this is a restarted run),
        // and enable file logging.
        Mesh mesh(init.comm(), NDIM);
        IBAMRInit ibamr_init = IBAMRInit::getInstance(argc, argv, & mesh);
      //  ibamr_init.parse_inputdb();
#if (NDIM == 2)
        ibamr_init.build_square(0.5, 0.55, 0.0, 1.0);
#endif
#if (NDIM == 3)
        mesh.read("/home/deleeke/sfw/myIBAMRfork/ibamr-test-bundled-gtest/examples/IBFE/explicit/ex2/Mesh_copy.e");
        ibamr_init.translate_mesh(0.9, 0.5, 0.5);
/*        for (MeshBase::node_iterator it = mesh.nodes_begin();
             it != mesh.nodes_end(); ++it)
        {
            Node* n = *it;
            libMesh::Point& X = *n;
            X(0) += 0.9;
            X(1) += 0.5;
            X(2) += 0.5;
        }
*/
#endif
        const MeshBase::const_element_iterator end_el = mesh.elements_end();
        for (MeshBase::const_element_iterator el = mesh.elements_begin(); el != end_el; ++el)
        {
            Elem* const elem = *el;
            for (unsigned int side = 0; side < elem->n_sides(); ++side)
            {
                const bool at_mesh_bdry = !elem->neighbor(side);
                if (at_mesh_bdry)
                {
                    BoundaryInfo* boundary_info = mesh.boundary_info.get();
#if (NDIM == 2)
                    if (boundary_info->has_boundary_id(elem, side, 0) || boundary_info->has_boundary_id(elem, side, 2))
                    {
                        boundary_info->add_side(elem, side, FEDataManager::ZERO_DISPLACEMENT_XY_BDRY_ID);
                    }
#endif
#if (NDIM == 3)
                    if (!(boundary_info->has_boundary_id(elem, side, 2) ||
                          boundary_info->has_boundary_id(elem, side, 4)))
                    {
                        boundary_info->add_side(elem, side, FEDataManager::ZERO_DISPLACEMENT_XYZ_BDRY_ID);
                    }
#endif
                }
            }
        }

        // Create major algorithm and data objects that comprise the
        // application.  These objects are configured from the input database
        // and, if this is a restarted run, from the restart database.
        Pointer<INSHierarchyIntegrator> navier_stokes_integrator;
        navier_stokes_integrator = ibamr_init.getIntegrator();
        Pointer<IBFEMethod> ib_method_ops = ibamr_init.getIBFEMethod();
        Pointer<IBHierarchyIntegrator> time_integrator = ibamr_init.getExplicitTimeIntegrator();
        Pointer<CartesianGridGeometry<NDIM> > grid_geometry = ibamr_init.getCartesianGridGeometry();
        Pointer<PatchHierarchy<NDIM> > patch_hierarchy = ibamr_init.getPatchHierarchy();
        Pointer<StandardTagAndInitialize<NDIM> > error_detector = ibamr_init.getErrorDetector();
        Pointer<BergerRigoutsos<NDIM> > box_generator = ibamr_init.getBoxGenerator();
        Pointer<LoadBalancer<NDIM> > load_balancer = ibamr_init.getLoadBalancer();
        Pointer<GriddingAlgorithm<NDIM> > gridding_algorithm = ibamr_init.getGriddingAlgorithm();

        // Configure the IBFE solver.
        IBFEMethod::PK1StressFcnData PK1_dev_stress_data(PK1_dev_stress_function);
        IBFEMethod::PK1StressFcnData PK1_dil_stress_data(PK1_dil_stress_function);
        PK1_dev_stress_data.quad_order = ibamr_init.getPK1DevOrder();
        PK1_dil_stress_data.quad_order = ibamr_init.getPK1DilOrder();
        ib_method_ops->registerPK1StressFunction(PK1_dev_stress_data);
        ib_method_ops->registerPK1StressFunction(PK1_dil_stress_data);
        EquationSystems* equation_systems = ib_method_ops->getFEDataManager()->getEquationSystems();

        // Register pressure and velocity ibamr_initial conditions.
        ibamr_init.registerVelocityInitialConditions(grid_geometry);
        ibamr_init.registerPressureInitialConditions(grid_geometry);

        // Create Eulerian boundary condition specification objects (when necessary).
        const IntVector<NDIM>& periodic_shift = grid_geometry->getPeriodicShift();
        vector<RobinBcCoefStrategy<NDIM>*> u_bc_coefs(NDIM);

       if (periodic_shift.min() > 0)
        {
            for (unsigned int d = 0; d < NDIM; ++d)
            {
                u_bc_coefs[d] = NULL;
            }
        }
        else
        {
            for (unsigned int d = 0; d < NDIM; ++d)
            {
                ostringstream bc_coefs_name_stream;
                bc_coefs_name_stream << "u_bc_coefs_" << d;
                const string bc_coefs_name = bc_coefs_name_stream.str();

                ostringstream bc_coefs_db_name_stream;
                bc_coefs_db_name_stream << "VelocityBcCoefs_" << d;
                const string bc_coefs_db_name = bc_coefs_db_name_stream.str();

                u_bc_coefs[d] = new muParserRobinBcCoefs(
                    bc_coefs_name, ibamr_init.getAppInitializer()->getComponentDatabase(bc_coefs_db_name), grid_geometry);
            }
            navier_stokes_integrator->registerPhysicalBoundaryConditions(u_bc_coefs);
        }

        // Create Eulerian body force function specification objects.
        if (ibamr_init.getInputDB()->keyExists("ForcingFunction"))
        {
            Pointer<CartGridFunction> f_fcn = new muParserCartGridFunction(
                "f_fcn", ibamr_init.getAppInitializer()->getComponentDatabase("ForcingFunction"), grid_geometry);
            time_integrator->registerBodyForceFunction(f_fcn);
        }

        // Set up visualization plot file writers.
        Pointer<VisItDataWriter<NDIM> > visit_data_writer = ibamr_init.getAppInitializer()->getVisItDataWriter();
        if (ibamr_init.uses_visit)
        {
            time_integrator->registerVisItDataWriter(visit_data_writer);
        }
        AutoPtr<ExodusII_IO> exodus_io(ibamr_init.uses_exodus ? new ExodusII_IO(mesh) : NULL);
        AutoPtr<GMVIO> gmv_io(ibamr_init.uses_gmv ? new GMVIO(mesh) : NULL);

        // Initialize hierarchy configuration and data on all patches.
        ib_method_ops->initializeFEData();
        time_integrator->initializePatchHierarchy(patch_hierarchy, gridding_algorithm);

        // Deallocate initialization objects.
        ibamr_init.getAppInitializer().setNull();

        // Print the input database contents to the log file.
        plog << "Input database:\n";
        ibamr_init.getInputDB()->printClassData(plog);

        // Write out initial visualization data.
        int iteration_num = time_integrator->getIntegratorStep();
        double loop_time = time_integrator->getIntegratorTime();
        if (ibamr_init.dump_viz_data)
        {
            pout << "\n\nWriting visualization files...\n\n";
            if (ibamr_init.uses_visit)
            {
                time_integrator->setupPlotData();
                visit_data_writer->writePlotData(patch_hierarchy, iteration_num, loop_time);
            }
            if (ibamr_init.uses_exodus)
            {
                exodus_io->write_timestep(
                    ibamr_init.exodus_filename, *equation_systems, iteration_num / ibamr_init.viz_dump_interval + 1, loop_time);
            }
            if (ibamr_init.uses_gmv)
            {
                std::ostringstream file_name;
                file_name << ibamr_init.gmv_filename + "_" << std::setw(6) << std::setfill('0') << std::right << iteration_num;
                gmv_io->write_equation_systems(file_name.str() + ".gmv", *equation_systems);
            }
        }

        // Main time step loop.
        double loop_time_end = time_integrator->getEndTime();
        double dt = 0.0;

        while (!MathUtilities<double>::equalEps(loop_time, loop_time_end) && time_integrator->stepsRemaining())
        {
            iteration_num = time_integrator->getIntegratorStep();
            loop_time = time_integrator->getIntegratorTime();

            ibamr_init.log_start(iteration_num, loop_time);

            dt = time_integrator->getMaximumTimeStepSize();
            time_integrator->advanceHierarchy(dt);
            loop_time += dt;

            ibamr_init.log_end(iteration_num, loop_time);

            // At specified intervals, write visualization and restart files,
            // print out timer data, and store hierarchy data for post
            // processing.
            iteration_num += 1;
            const bool last_step = !time_integrator->stepsRemaining();
            if (ibamr_init.dump_viz_data && (iteration_num % ibamr_init.viz_dump_interval == 0 || last_step))
            {
                pout << "\nWriting visualization files...\n\n";
                if (ibamr_init.uses_visit)
                {
                    time_integrator->setupPlotData();
                    visit_data_writer->writePlotData(patch_hierarchy, iteration_num, loop_time);
                }
                if (ibamr_init.uses_exodus)
                {
                    exodus_io->write_timestep(
                        ibamr_init.exodus_filename, *equation_systems, iteration_num / ibamr_init.viz_dump_interval + 1, loop_time);
                }
                if (ibamr_init.uses_gmv)
                {
                    std::ostringstream file_name;
                    file_name << ibamr_init.gmv_filename + "_" << std::setw(6) << std::setfill('0') << std::right << iteration_num;
                    gmv_io->write_equation_systems(file_name.str() + ".gmv", *equation_systems);
                }
            }
            if (ibamr_init.dump_restart_data && (iteration_num % ibamr_init.restart_dump_interval == 0 || last_step))
            {
                pout << "\nWriting restart files...\n\n";
                RestartManager::getManager()->writeRestartFile(ibamr_init.restart_dump_dirname, iteration_num);
                ib_method_ops->writeFEDataToRestartFile(ibamr_init.restart_dump_dirname, iteration_num);
            }
            if (ibamr_init.dump_timer_data && (iteration_num % ibamr_init.timer_dump_interval == 0 || last_step))
            {
                pout << "\nWriting timer data...\n\n";
                TimerManager::getManager()->print(plog);
            }
            if (ibamr_init.dump_postproc_data && (iteration_num % ibamr_init.postproc_data_dump_interval == 0 || last_step))
            {
                pout << "\nWriting state data...\n\n";
                output_data(patch_hierarchy,
                            navier_stokes_integrator,
                            mesh,
                            equation_systems,
                            iteration_num,
                            loop_time,
                            ibamr_init.postproc_data_dump_dirname);
            }
        }

        // Cleanup Eulerian boundary condition specification objects (when
        // necessary).
        for (unsigned int d = 0; d < NDIM; ++d) delete u_bc_coefs[d];

    } // cleanup dynamically allocated objects prior to shutdown

    SAMRAIManager::shutdown();
    return true;
}

void
output_data(Pointer<PatchHierarchy<NDIM> > patch_hierarchy,
            Pointer<INSHierarchyIntegrator> navier_stokes_integrator,
            Mesh& mesh,
            EquationSystems* equation_systems,
            const int iteration_num,
            const double loop_time,
            const string& data_dump_dirname)
{
    plog << "writing hierarchy data at iteration " << iteration_num << " to disk" << endl;
    plog << "simulation time is " << loop_time << endl;

    // Write Cartesian data.
    string file_name = data_dump_dirname + "/" + "hier_data.";
    char temp_buf[128];
    sprintf(temp_buf, "%05d.samrai.%05d", iteration_num, SAMRAI_MPI::getRank());
    file_name += temp_buf;
    Pointer<HDFDatabase> hier_db = new HDFDatabase("hier_db");
    hier_db->create(file_name);
    VariableDatabase<NDIM>* var_db = VariableDatabase<NDIM>::getDatabase();
    ComponentSelector hier_data;
    hier_data.setFlag(var_db->mapVariableAndContextToIndex(navier_stokes_integrator->getVelocityVariable(),
                                                           navier_stokes_integrator->getCurrentContext()));
    hier_data.setFlag(var_db->mapVariableAndContextToIndex(navier_stokes_integrator->getPressureVariable(),
                                                           navier_stokes_integrator->getCurrentContext()));
    patch_hierarchy->putToDatabase(hier_db->putDatabase("PatchHierarchy"), hier_data);
    hier_db->putDouble("loop_time", loop_time);
    hier_db->putInteger("iteration_num", iteration_num);
    hier_db->close();

    // Write Lagrangian data.
    file_name = data_dump_dirname + "/" + "fe_mesh.";
    sprintf(temp_buf, "%05d", iteration_num);
    file_name += temp_buf;
    file_name += ".xda";
    mesh.write(file_name);
    file_name = data_dump_dirname + "/" + "fe_equation_systems.";
    sprintf(temp_buf, "%05d", iteration_num);
    file_name += temp_buf;
    equation_systems->write(file_name, (EquationSystems::WRITE_DATA | EquationSystems::WRITE_ADDITIONAL_DATA));
    return;
} // output_data
