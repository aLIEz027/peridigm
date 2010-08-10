/*! \file Peridigm_RythmosObserver.cpp */
// ***********************************************************************
//
//                             Peridigm
//                 Copyright (2009) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//
// Questions?
// David J. Littlewood   djlittl@sandia.gov
// John A. Mitchell      jamitch@sandia.gov
// Michael L. Parks      mlparks@sandia.gov
// Stewart A. Silling    sasilli@sandia.gov
//
// ***********************************************************************

#include "Peridigm_RythmosObserver.hpp"
#include <Thyra_VectorBase.hpp>
#include <Thyra_EpetraModelEvaluator.hpp>
#include <Thyra_EpetraThyraWrappers.hpp>
#include <EpetraExt_ModelEvaluator.h>
#include <Epetra_Vector.h>
#include <Epetra_Comm.h>
#include <Epetra_Map.h>
#include "Peridigm_OutputManager_VTK_XML.hpp"

Peridigm::RythmosObserver::RythmosObserver(Teuchos::RCP<EpetraExt::ModelEvaluator> model_,
                                           const Teuchos::RCP<Teuchos::ParameterList>& params) {
  active = false;

  model = Teuchos::rcp_dynamic_cast<Peridigm::ModelEvaluator>(model_);
  TEST_FOR_EXCEPT_MSG( model.get() == NULL, "Peridigm::RythmosObserver: Peridigm::ModelEvaluator not passed in.");

  if (params->isSublist("Output")) { 
    active = true;
    outputParams =  Teuchos::rcp(&(params->sublist("Output")),false);
    outputParams->set("NumProc", (int)(model->get_x_map()->Comm()).NumProc());
    outputParams->set("MyPID", (int)(model->get_x_map()->Comm()).MyPID());
  }

  if (active) {
    // Make the default format "VTK_XML"
    string outputFormat = outputParams->get("Output File Type", "VTK_XML");
    TEST_FOR_EXCEPTION( outputFormat != "VTK_XML",
                        std::invalid_argument,
                        "Peridigm::RythmosObserver: \"Output File Type\" must be \"VTK_XML\".");
    if (outputFormat == "VTK_XML")
       outputManager = Teuchos::rcp(new Peridigm::OutputManager_VTK_XML( outputParams ));
    else 
      TEST_FOR_EXCEPTION( true, std::invalid_argument,"Peridigm::RythmosObserver: \"Output File Type\" must be \"VTK_XML\".");

    // Query material models for their force state data descriptions
    forceStateDesc = Teuchos::rcp( new Teuchos::ParameterList() );
    std::vector< Teuchos::RCP<Peridigm::Material> > materials = model->getMaterials();
    for(unsigned int i=0; i<materials.size(); ++i){
      Teuchos::ParameterList& subList = forceStateDesc->sublist(materials[i]->Name());
      for(int j=0;j<materials[i]->NumScalarConstitutiveVariables(); ++j){
        subList.set( materials[i]->ScalarConstitutiveVariableName(j), j );
      }
    }
    // Initialize current time in this parameterlist
    forceStateDesc->set("Time", 0.0);
    // Set RCP to neighborlist
    forceStateDesc->set("Bond Family",model->getNeighborhoodData());
    // Ask OutputManager to write initial conditions to disk
    outputManager->write(model->get_x_init(),
						 model->getScalarConstitutiveDataOverlap(),
						 model->getNeighborhoodData(),
						 forceStateDesc);
  }

  //  verbose = problemParams->get("Verbose", false);

}

void Peridigm::RythmosObserver::observeCompletedTimeStep(
    const Rythmos::StepperBase<Scalar> &stepper,
    const Rythmos::StepControlInfo<Scalar> &stepCtrlInfo,
    const int timeStepIter) {

  Teuchos::RCP<const Epetra_Vector> currentSolution = Thyra::get_Epetra_Vector(*model->get_x_map(), stepper.getStepStatus().solution);

  // We have completed a time step, tell the Peridigm_ModelEvaluator to update its state information
  model->updateState();

  // Callback allowing the ModelEvaluator to update the contact configuration, if necessary
  model->updateContact(currentSolution);

  // Only report status if Observer is active
  if (!active) return;

  //cout << "PERIDIGM OBSERVER CALLED step=" <<  timeStepIter  << ",  time=" << stepper.getStepStatus().time << endl;

  // Set current time in this parameterlist
  forceStateDesc->set("Time", stepper.getStepStatus().time);
  outputManager->write(currentSolution,
					   model->getScalarConstitutiveDataOverlap(),
					   model->getNeighborhoodData(),
					   forceStateDesc);

/*
  Teuchos::RCP<const Epetra_Vector> finalSolution = Thyra::get_Epetra_Vector(*model->get_x_map(), stepper.getStepStatus().solution);
  cout << *finalSolution;
*/

/*
  const Epetra_Vector soln= *(Thyra::get_Epetra_Vector(*disc->getMap(), solution));
  if (vtk != Teuchos::null) vtk->visualizeField (soln, disc);
  if (su  != Teuchos::null) su ->visualizeField (soln, disc);
*/
}
