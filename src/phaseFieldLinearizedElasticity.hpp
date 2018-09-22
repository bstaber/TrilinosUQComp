/*
Brian Staber (brian.staber@gmail.com)
*/

#ifndef PHASEFIELDLINEARIZEDELASTICITY_HPP
#define PHASEFIELDLINEARIZEDELASTICITY_HPP

#include "linearizedElasticity.hpp"
#include "damageField.hpp"

class phaseFieldLinearizedElasticity : public linearizedElasticity{

  Epetra_SerialDenseMatrix elasticity;
  Teuchos::RCP<damageField> damageInterface;

  double gc, lc;
  double E, nu, lambda, mu;

  Epetra_Map    * GaussMap;
  Epetra_Vector * damageHistory;
  Epetra_Vector * displacement;

  Epetra_FECrsMatrix * matrix;
  Epetra_FEVector * rhs;

  phaseFieldLinearizedElasticity(Epetra_Comm & comm, Teuchos::ParameterList & Parameters,
                                 double & gc_, double & lc_);
  ~phaseFieldLinearizedElasticity();

  void constructGaussMap();
  void computeDisplacement(Teuchos::ParameterList & ParameterList, double & bc_disp);
  void updateDamageHistory();
  void staggeredAlgorithmDirichletBC();

  void get_elasticity_tensor(unsigned int & e_lid, unsigned int & gp, Epetra_SerialDenseMatrix & tangent_matrix);
  void get_elasticity_tensor_for_recovery(unsigned int & e_lid, Epetra_SerialDenseMatrix & tangent_matrix);

};
#endif
