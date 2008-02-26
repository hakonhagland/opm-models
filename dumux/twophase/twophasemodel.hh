#ifndef DUNE_TWOPHASEMODEL_HH
#define DUNE_TWOPHASEMODEL_HH

#include<dune/disc/shapefunctions/lagrangeshapefunctions.hh>
#include<dune/disc/functions/p1function.hh>
#include<dune/disc/operators/p1operator.hh>
#include"dumux/nonlinear/nonlinearmodel.hh"

namespace Dune
{
  template<class G, class RT, class ProblemType, class LocalJacobian, 
            class FunctionType, class OperatorAssembler>
  class TwoPhaseModel 
  : public NonlinearModel<G, RT, ProblemType, LocalJacobian, FunctionType, OperatorAssembler> 
  {
  public:	
	typedef NonlinearModel<G, RT, ProblemType, LocalJacobian, 
	                          FunctionType, OperatorAssembler> NonlinearModel;
	
	TwoPhaseModel(const G& g, ProblemType& prob)
	: NonlinearModel(g, prob), uOldTimeStep(g)
	{ }
	
	TwoPhaseModel(const G& g, ProblemType& prob, int level)
	: NonlinearModel(g, prob, level), uOldTimeStep(g, level)
	{ 	}
	
	virtual void initial() = 0;
	
	virtual void update(double& dt) = 0;
	
	virtual void solve() = 0;
	
	FunctionType uOldTimeStep;
  };


  
  
  
  template<class G, class RT, class ProblemType, class LocalJac, int m=2>
  class LeafP1TwoPhaseModel : public TwoPhaseModel<G, RT, ProblemType, LocalJac, 
                                        LeafP1Function<G, RT, m>, LeafP1OperatorAssembler<G, RT, m> >
  {
  public:
	  // define the function type:
	  typedef LeafP1Function<G, RT, m> FunctionType;

	  // define the operator assembler type:
	  typedef LeafP1OperatorAssembler<G, RT, m> OperatorAssembler;

	  typedef TwoPhaseModel<G, RT, ProblemType, LocalJac, 
	                          FunctionType, OperatorAssembler> TwoPhaseModel;
	  
	  typedef LeafP1TwoPhaseModel<G, RT, ProblemType, LocalJac, m> ThisType;

	  typedef LocalJac LocalJacobian;
	  
	  // mapper: one data element per vertex
	  template<int dim>
	  struct P1Layout
	  {
		  bool contains (Dune::GeometryType gt)
		  {
			  return gt.dim() == 0;
		  }
	  }; 

	  typedef typename G::Traits::LeafIndexSet IS;
	  typedef MultipleCodimMultipleGeomTypeMapper<G,IS,P1Layout> VertexMapper;
	  typedef typename IntersectionIteratorGetter<G,LeafTag>::IntersectionIterator IntersectionIterator;

	  LeafP1TwoPhaseModel (const G& g, ProblemType& prob) 
	  : TwoPhaseModel(g, prob), grid(g), vertexmapper(g, g.leafIndexSet()), 
	    size((*(this->u)).size()), pW(size), pN(size), pC(size), satW(size), satN(size)
	  { }
	  
	  virtual void initial() 
	  {
		  typedef typename G::Traits::template Codim<0>::Entity Entity;
		  typedef typename G::ctype DT;
		  typedef typename IS::template Codim<0>::template Partition<All_Partition>::Iterator Iterator;
		  enum{dim = G::dimension};
		  enum{dimworld = G::dimensionworld};
		  
		  const IS& indexset(grid.leafIndexSet());
		  
		  // iterate through leaf grid an evaluate c0 at cell center
		  Iterator eendit = indexset.template end<0, All_Partition>();
		  for (Iterator it = indexset.template begin<0, All_Partition>(); it != eendit; ++it)
		  {
			  // get geometry type
			  Dune::GeometryType gt = it->geometry().type();

			  // get entity 
			  const Entity& entity = *it;

			  const typename Dune::LagrangeShapeFunctionSetContainer<DT,RT,dim>::value_type& 
		      	sfs=Dune::LagrangeShapeFunctions<DT,RT,dim>::general(gt, 1);
		      int size = sfs.size();

		      for (int i = 0; i < size; i++) {
		    	  // get cell center in reference element
		    	  const Dune::FieldVector<DT,dim>& 
					  local = sfs[i].position();

		    	  // get global coordinate of cell center
		    	  Dune::FieldVector<DT,dimworld> global = it->geometry().global(local);

	    		  int globalId = vertexmapper.template map<dim>(entity, sfs[i].entity());
	    	  
	    		  // initialize cell concentration
	    		  (*(this->u))[globalId] = this->problem.initial(global, entity, local);
		      }
		  }

		  // set Dirichlet boundary conditions
		  for (Iterator it = indexset.template begin<0, All_Partition>(); it != eendit; ++it)
		  {
			  // get geometry type
			  Dune::GeometryType gt = it->geometry().type();

			  // get entity 
			  const Entity& entity = *it;

			  const typename Dune::LagrangeShapeFunctionSetContainer<DT,RT,dim>::value_type& 
		      	sfs=Dune::LagrangeShapeFunctions<DT,RT,dim>::general(gt, 1);
		      int size = sfs.size();

			  // set type of boundary conditions 
			  this->localJacobian.template assembleBC<LeafTag>(entity);
			  
			  IntersectionIterator endit = IntersectionIteratorGetter<G,LeafTag>::end(entity);
			  for (IntersectionIterator is = IntersectionIteratorGetter<G,LeafTag>::begin(entity); 
			       is!=endit; ++is)
				  if (is.boundary())
				  {
				    for (int i = 0; i < size; i++) 
					  // handle subentities of this face
					  for (int j = 0; j < ReferenceElements<DT,dim>::general(gt).size(is.numberInSelf(), 1, sfs[i].codim()); j++)
						if (sfs[i].entity() == ReferenceElements<DT,dim>::general(gt).subEntity(is.numberInSelf(), 1, j, sfs[i].codim()))
						{
							if (this->localJacobian.bc(i)[0] == BoundaryConditions::dirichlet) 
							{
								// get cell center in reference element
								Dune::FieldVector<DT,dim> local = sfs[i].position();

								// get global coordinate of cell center
								Dune::FieldVector<DT,dimworld> global = it->geometry().global(local);

								int globalId = vertexmapper.template map<dim>(entity, sfs[i].entity());
		    	  
								FieldVector<BoundaryConditions::Flags, m> bctype = this->problem.bctype(global, entity, is, local);
						  
								if (bctype[0] == BoundaryConditions::dirichlet) {
									(*(this->u))[globalId] = this->problem.g(global, entity, is, local);
								}
							}
						}
			  }
		  }

		  *(this->uOldTimeStep) = *(this->u);
		  return;
	  }

		virtual void vtkout (const char* name, int k) 
		{
			VTKWriter<G> vtkwriter(this->grid);
			char fname[128];	
			sprintf(fname,"%s-%05d",name,k);
			for (int i = 0; i < size; i++) {
				pW[i] = (*(this->u))[i][0];
				//pN[i] = (*(this->u))[i][1];
				//pC[i] = pN[i] - pW[i];
				//satW[i] = this->problem.materialLaw().saturationW(pC[i]);
				//satN[i] = 1 - satW[i];
				satN[i] = (*(this->u))[i][1];
				satW[i] = 1 - satN[i];
			}
			vtkwriter.addVertexData(pW,"wetting phase pressure");
			vtkwriter.addVertexData(satW,"wetting phase saturation");
			vtkwriter.addVertexData(satN,"nonwetting phase saturation");
			vtkwriter.write(fname, VTKOptions::ascii);		
		}

  protected:
	  const G& grid;
	  VertexMapper vertexmapper;
	  int size;
	  BlockVector<FieldVector<RT, 1> > pW; 
	  BlockVector<FieldVector<RT, 1> > pN; 
	  BlockVector<FieldVector<RT, 1> > pC; 
	  BlockVector<FieldVector<RT, 1> > satW; 
	  BlockVector<FieldVector<RT, 1> > satN; 
  };

}
#endif
