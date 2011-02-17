// iPBS.cc Read a gmsh file and solve PB eq.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// std includes
#include<math.h>
#include<iostream>
#include<vector>
#include<string>


// dune includes
#include<dune/common/mpihelper.hh>
#include<dune/common/exceptions.hh>
#include<dune/common/fvector.hh>
#include<dune/grid/io/file/gmshreader.hh>

// Input/Output
#include <dune/grid/io/file/gnuplot.hh>

// we use UG
#include<dune/grid/uggrid.hh>

// pdelab includes
#include<dune/pdelab/finiteelementmap/conformingconstraints.hh>
// #include<dune/pdelab/instationary/onestep.hh>   // Filenamehelper
#include<dune/pdelab/finiteelementmap/p1fem.hh>	// P1 in 1,2,3 dimensions
//#include<dune/pdelab/finiteelementmap/p0fem.hh>	// P1 in 1,2,3 dimensions
#include<dune/pdelab/gridfunctionspace/gridfunctionspace.hh>
#include<dune/pdelab/gridfunctionspace/gridfunctionspaceutilities.hh>
#include<dune/pdelab/gridfunctionspace/genericdatahandle.hh>
#include<dune/pdelab/gridfunctionspace/interpolate.hh>
#include<dune/pdelab/gridfunctionspace/constraints.hh>
#include<dune/pdelab/gridoperatorspace/gridoperatorspace.hh>
#include<dune/pdelab/backend/istlvectorbackend.hh>
#include<dune/pdelab/backend/istlmatrixbackend.hh>
#include<dune/pdelab/backend/istlsolverbackend.hh>
#include<dune/pdelab/stationary/linearproblem.hh>

const int dimgrid = 2;
typedef Dune::UGGrid<dimgrid> GridType;         // 2d mesh
typedef GridType::LeafGridView GV;
typedef GV::Grid::ctype Coord;
typedef double Real;
const int dim = GV::dimension;
typedef Dune::PDELab::P1LocalFiniteElementMap<Coord,Real,dim> FEM;
//typedef Dune::PDELab::P0LocalFiniteElementMap<Coord,Real,dim> FEM;
typedef Dune::PDELab::ConformingDirichletConstraints CON;
typedef Dune::PDELab::ISTLVectorBackend<1> VBE;
typedef Dune::PDELab::GridFunctionSpace<GV,FEM,CON,VBE> GFS;
typedef GFS::ConstraintsContainer<Real>::Type CC;
typedef GFS::VectorContainer<Real>::Type U;
typedef Dune::PDELab::DiscreteGridFunction<GFS,U> DGF;
typedef Dune::PDELab::ISTLBCRSMatrixBackend<1,1> MBE;

#ifndef _P0LAYOUT_H
#define _P0LAYOUT_H
#include "p0layout.hh"
#endif

#include <dune/grid/common/mcmgmapper.hh>
typedef Dune::LeafMultipleCodimMultipleGeomTypeMapper<GridType, P0Layout> Mapper;

#include "boundaries.hh"
typedef Regions<GV,double,std::vector<int>> M;
typedef BCType<GV,std::vector<int>> B;
typedef BCExtension_init<GV,double,std::vector<int>> G_init;
//typedef BCExtension_iterate<GV,double,std::vector<int>> G;
typedef BoundaryFlux<GV,double,std::vector<int> > J;

#include"PB_operator.hh"
typedef PBLocalOperator<M,B,J> LOP;
typedef Dune::PDELab::GridOperatorSpace<GFS,GFS,LOP,CC,CC,MBE,true> GOS;
typedef Dune::PDELab::ISTLBackend_SEQ_BCGS_SSOR LS;
typedef Dune::PDELab::Newton<GOS,LS,U> NEWTON;
typedef Dune::PDELab::StationaryLinearProblemSolver<GOS,LS,U> SLP;
typedef DGF::Traits Traits;

// include application heaeders
#include "ipbs.hh"
#include "solve.hh"

#ifndef _SYSPARAMS_H
#define _SYSPARAMS_H
#include "sysparams.hh"
#endif




//===============================================================
// Main programm
//===============================================================
int main(int argc, char** argv)
{
 try{
  // Initialize Mpi
  Dune::MPIHelper& helper = Dune::MPIHelper::instance(argc, argv);
  std::cout << "Hello World! This is iPBS." << std::endl;
  if(Dune::MPIHelper::isFake)
    std::cout<< "This is (at the moment) a sequential program!" << std::endl;
  else
  {
    if(helper.rank()==0)
    std::cout << "parallel run on " << helper.size() << " process(es)" << std::endl;
  }

  // check arguments
  if (argc!=4)
  {
    if (helper.rank()==0)
    {
	std::cout << "usage: ./iPBS <meshfile> <refinement level> <SOR Parameter>" << std::endl;
	return 1;
    }
  }

  // Read in comandline arguments
  Cmdparam cmdparam;
  cmdparam.GridName=argv[1];
  sscanf(argv[2],"%d",&cmdparam.RefinementLevel);
  //sscanf(argv[3],"%f",&cmdparam.alpha_sor);
  double alpha;
  sscanf(argv[3],"%lf", &alpha);
  std::cout << "Using " << cmdparam.RefinementLevel << " refinement levels. alpha" << alpha << std::endl;
  sysParams.set_alpha(alpha);
  
  
//===============================================================
// Setup problem
//===============================================================
  
  
  // <<<1>>> Setup the problem from mesh file

  // instanciate ug grid object
  GridType grid;

  // define vectors to store boundary and element mapping
  std::vector<int> boundaryIndexToEntity;
  std::vector<int> elementIndexToEntity;

  // read a gmsh file
  Dune::GmshReader<GridType> gmshreader;
  gmshreader.read(grid, cmdparam.GridName, boundaryIndexToEntity, elementIndexToEntity, true, false);

  // refine grid
  grid.globalRefine(cmdparam.RefinementLevel);

  for (int i=0; i<3; i++)
  {
  // get a grid view - this one is for the refinement iterator...
  const GV& gv_tmp = grid.leafView();
  // iterate once over the surface and refine
  typedef GV::Codim<0>::Iterator ElementLeafIterator;
  for (ElementLeafIterator it = gv_tmp.begin<0>(); it != gv_tmp.end<0>(); ++it)
    {
      //if (it->hasBoundaryIntersections()==true && it->geometry().center().two_norm() < 4.7)
      if (it->geometry().center().two_norm() < 1.7 && i < 2) 
      {
	grid.mark(1,*it);
      }
      if (i >= 2 && it->hasBoundaryIntersections() == true && it->geometry().center().two_norm() < 4.7) grid.mark(1,*it);
    }
  grid.preAdapt();
  grid.adapt();
  grid.postAdapt();
  std::cout << "Adaptive Refinement step " << i << " done."  << std::endl;
  }
  
  const GV& gv = grid.leafView();

  //define boundaries
  
  // inner region
  M m(gv, elementIndexToEntity);
  // boundary
  B b(gv, boundaryIndexToEntity);
    // Dirichlet
  G_init g(gv, boundaryIndexToEntity);
  // boundary fluxes
  J j(gv, boundaryIndexToEntity);
  
  // Create finite element map
  FEM fem;

  // <<<2>>> Make grid function space
  GFS gfs(gv,fem);

  // Create coefficient vector (with zero values)
  U u(gfs,0.0);
  
  // <<<3>>> Make FE function extending Dirichlet boundary conditions
  CC cc;
  Dune::PDELab::constraints(b,gfs,cc); 
  //std::cout << "constrained dofs=" << cc.size() 
  //          << " of " << gfs.globalSize() << std::endl;

  // interpolate coefficient vector
  Dune::PDELab::interpolate(g,gfs,u);
  
  //std::cout << "VECTOR u" << std::endl << "==============" << std::endl << u << std::endl;

  // MAYBE GET AN INITAL SOLUTION (see below) 
  // get initial coefficient vector
/*  get_solution(u, gv, gfs, m, b, g, j);
  // Create Discrete Grid Function Space
  DGF udgf(gfs,u);
  // graphical output
  std::string vtk_filename = "step_0";
  save(udgf, u, gv, vtk_filename);
*/
  // ITERATION STARTS HERE
  // we don't need an initialisation step but can(!) use one...
  // (e.g. for random initial conditions of surface charge distribution)
  
  
  get_solution(u, gv, gfs, cc, grid, m, b, j);
  

  // done
  return 0;
 }
 catch (Dune::Exception &e){
  std::cerr << "Dune reported error: " << e << std::endl;
 }
 catch (...){
  std::cerr << "Unknown exception thrown!" << std::endl;
 }
} 

// ============================================================================



void solver (NEWTON &newton, SLP &slp)
{
	newton.apply();
	slp.apply();
}

// ============================================================================

void save(const DGF &udgf, const U &u, const GV &gv, const std::string filename)
{
  //Dune::PDELab::FilenameHelper fn("output");
  // {
    Dune::VTKWriter<GV> vtkwriter(gv,Dune::VTKOptions::conforming);
    vtkwriter.addVertexData(new Dune::PDELab::VTKGridFunctionAdapter<DGF>(udgf,"solution"));
    vtkwriter.write(filename,Dune::VTK::ascii);
  // }
  // Gnuplot output
  Dune::GnuplotWriter<GV> gnuplotwriter(gv);
  gnuplotwriter.addVertexData(u,"solution");
  gnuplotwriter.write(filename + ".dat");
}


