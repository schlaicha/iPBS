/** \brief Calculate the force between colloids

    Some more description :-)
*/

/*!
   \param
*/

#include <fstream>

template <typename GV, typename GFS, typename U>
void force(const GV& gv, const std::vector<int>& boundaryIndexToEntity,
    const GFS& gfs, const U& u)
{
  const int dim = GFS::Traits::GridViewType::dimension;

  // Here we once more loop trough all elements (as for the measurement of force performance is
  // not important anyway) and integrate Maxwell stress tensor over the particles surface
  // (see Hsu06a, eq. 61)

  typedef typename GV::template Codim<0>::template Partition
          <Dune::Interior_Partition>::Iterator LeafIterator;
  typedef typename GV::IntersectionIterator IntersectionIterator;
  
  
  // Do the loop for all boundaryIDs > 1 (all colloids)
  for (int i = 2; i < sysParams.get_npart()+2; i++)
  {
    std::cout << "Calculating the force actin on particle " << i << std::endl;
    Dune::FieldVector<Real, dim> F(0);
    // loop over elements on this processor
    for (LeafIterator it = gv.template begin<0,Dune::Interior_Partition>();
              	it!=gv.template end<0,Dune::Interior_Partition>(); ++it)
    {
     if(it->hasBoundaryIntersections() == true)
      {
        for (IntersectionIterator ii = gv.ibegin(*it); ii != gv.iend(*it); ++ii)
        {
          if(ii->boundary() == true)
          {
            if (boundaryIndexToEntity[ii->boundarySegmentIndex()] == i) // check if IPBS boundary
            {
              Dune::FieldVector<Real, dim> normal = ii->centerUnitOuterNormal();
              normal *= -1.0 * ii->geometry().volume() * ii->geometry().center()[1]
                * 2.0 * sysParams.pi; // Surface normal
              Dune::FieldMatrix<Real, GFS::Traits::GridViewType::dimension, 
                  GFS::Traits::GridViewType::dimension>
                      sigma = maxwelltensor(gfs, it, u);
              sigma.umv(normal,F);
            }
          }
        }
      }
    }

  std::ofstream myfile;
  myfile.open ("forces.dat", std::ios::out | std::ios::app);
  myfile << i << " " << F << std::endl;
  myfile.close();
  }
}