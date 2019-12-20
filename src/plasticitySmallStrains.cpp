/*
Brian Staber (brian.staber@gmail.com)
*/

#include "plasticitySmallStrains.hpp"

/*plasticitySmallStrains::plasticitySmallStrains(){

}*/

plasticitySmallStrains::~plasticitySmallStrains(){

}

plasticitySmallStrains::plasticitySmallStrains(Epetra_Comm & comm, Teuchos::ParameterList & parameterlist){

  Delta         = Teuchos::getParameter<double>(parameterlist.sublist("Newton"), "delta");
  iter_min      = Teuchos::getParameter<int>(parameterlist.sublist("Newton"),    "iterMin");
  iter_max      = Teuchos::getParameter<int>(parameterlist.sublist("Newton"),    "iterMax");
  nb_bis_max    = Teuchos::getParameter<int>(parameterlist.sublist("Newton"),    "nbBisMax");
  norm_inf_tol  = Teuchos::getParameter<double>(parameterlist.sublist("Newton"), "NormFTol");
  norm_inf_max  = Teuchos::getParameter<double>(parameterlist.sublist("Newton"), "NormFMax");
  eps           = Teuchos::getParameter<double>(parameterlist.sublist("Newton"), "eps");
  success       = Teuchos::getParameter<double>(parameterlist.sublist("Newton"), "success_parameter");
  failure       = Teuchos::getParameter<double>(parameterlist.sublist("Newton"), "failure_parameter");
  bc_disp       = Teuchos::getParameter<double>(parameterlist.sublist("Newton"), "bc_disp");
  pressure_load = Teuchos::getParameter<double>(parameterlist.sublist("Newton"), "pressure_load");
  tol           = Teuchos::getParameter<double>(parameterlist.sublist("Newton"), "tol");

  Krylov = &parameterlist.sublist("Krylov");

  std::string mesh_file = Teuchos::getParameter<std::string>(parameterlist.sublist("Mesh"), "mesh_file");
  Mesh                  = new mesh(comm, mesh_file, 1.0);
  Comm                  = Mesh->Comm;

  StandardMap           = new Epetra_Map(-1,3*Mesh->n_local_nodes_without_ghosts,&Mesh->local_dof_without_ghosts[0],0,*Comm);
  OverlapMap            = new Epetra_Map(-1,3*Mesh->n_local_nodes,&Mesh->local_dof[0],0,*Comm);
  ImportToOverlapMap    = new Epetra_Import(*OverlapMap,*StandardMap);

  constructGaussMap(*GaussMap);

  eel = new Epetra_Vector(*GaussMap);
  eto = new Epetra_Vector(*GaussMap);
  epi = new Epetra_Vector(*GaussMap);

  ELASTICITY.Reshape(6,6);
}

int plasticitySmallStrains::incremental_bvp(bool print){

    Epetra_Time Time(*Comm);

    Epetra_LinearProblem problem;
    AztecOO solver;

    Epetra_FECrsMatrix stiffness(Copy,*FEGraph);
    Epetra_FEVector    rhs(*StandardMap);
    Epetra_Vector      lhs(*StandardMap);
    Epetra_Vector      du(*OverlapMap);
    Epetra_Vector      x(*StandardMap);
    Epetra_Vector      y(*StandardMap);
    /*Epetra_Vector      ep_new(*GaussMap);
    Epetra_Vector      ep_old(*GaussMap);*/

    double delta = Delta;
    double Assemble_time;
    double Aztec_time;
    double displacement;
    double norm_inf_rhs;
    double time_max = 1.0;
    double Krylov_res = 0;
    int FLAG1, FLAG2, FLAG3, nb_bis, iter;
    int Krylov_its = 0;

    time = 0.0;

    std::string solver_its = "GMRES_its";
    std::string solver_res = "GMRES_res";

    eel->PutScalar(0.0);
    eto->PutScalar(0.0);
    epi->PutScalar(0.0);

    //FIRST ELASTIC STEP
    time += delta;
    rhs.PutScalar(0.0);
    assemblePureDirichlet_homogeneousForcing_LinearElasticity(stiffness);
    displacement = delta*bc_disp;
    apply_dirichlet_conditions(stiffness, rhs, displacement);

    lhs.PutScalar(0.0);
    problem.SetOperator(&stiffness);
    problem.SetLHS(&lhs);
    problem.SetRHS(&rhs);
    solver.SetProblem(problem);
    solver.SetParameters(*Krylov);

    Time.ResetStartTime();
    solver.Iterate(2000,tol);
    Aztec_time = Time.ElapsedTime();
    Krylov_its = solver.NumIters();
    Krylov_res = solver.TrueResidual();
    x.Update(1.0,lhs,1.0);

    du.Import(lhs, *ImportToOverlapMap, Insert);
    //END FIRST ELASTIC STEP

    FLAG1=1;
    while (FLAG1==1){

        FLAG1=0;
        FLAG2=1;
        FLAG3=1;
        nb_bis = 0;
        time += delta;
        y = x;

        while (FLAG2==1){
            FLAG2=0;
            if(time-time_max>eps){
                if(time_max+delta-time>eps){
                    delta=time_max+delta-time;
                    time=time_max;
                }
                else{
                    FLAG1=0;
                    break;
                }
            }
            //interface->pressure_load = time*pressure_load;

            iter = 0;
            while (FLAG3==1){

                FLAG3=0;
                iter++;

                if(iter>iter_max){
                    if (MyPID==0) std::cout << "Iteration limit exceeds.\n";
                    return 1;
                }

                Time.ResetStartTime();
                assemblePureDirichlet_homogeneousForcing(du, stiffness, rhs);
                Assemble_time = Time.ElapsedTime();

                displacement = (iter==1) ? (delta*bc_disp) : (0.0);
                apply_dirichlet_conditions(stiffness, rhs, displacement);

                if(iter>1){

                    rhs.NormInf(&norm_inf_rhs);

                    if (MyPID==0 && print){
                        if(iter>2){
                            std::cout << "\t\t\t" << iter << "\t" << norm_inf_rhs << "\t" << Krylov_its << "\t\t" << Krylov_res << "\t\t" << Assemble_time << "\t\t\t" << Aztec_time << "\n"; /* << "\t\t\t" << time*pressure_load << "\n"; */
                        }
                        else{
                            std::cout << "\n Time" << "\t" << "Timestep" << "\t" << "Iter" << "\t" << "NormInf" << "\t\t" << solver_its << "\t" << solver_res << "\t\t" << "assemble_time" << "\t\t" << "aztec_time" << "\t\t" << "\n"; /*"pressure_load [kPa]" << "\n";*/
                            std::cout << " " << time << "\t" << delta << "\t\t" << iter << "\t" << norm_inf_rhs << "\t" << Krylov_its << "\t\t" << Krylov_res << "\t\t" << Assemble_time << "\t\t\t" << Aztec_time << "\t\t\t" << "\n"; /*time*pressure_load << "\n";*/
                        }
                    }

                    if (norm_inf_rhs<norm_inf_tol){
                        if (iter<=iter_min) delta = success*delta;
                        if (iter>=iter_max) delta = delta/failure;
                        FLAG1=1;
                        break;
                    }

                    if (norm_inf_rhs>norm_inf_max||iter==iter_max){
                        nb_bis++;
                        if (nb_bis<nb_bis_max){
                            delta /= failure;
                            time -= delta;
                            x = y;
                            if (MyPID==0) std::cout << "Bisecting increment: " << nb_bis << "\n";
                        }
                        else{
                            if (MyPID==0) std::cout << "Bisection number exceeds.\n";
                            return 2;
                        }
                        FLAG2=1;
                        FLAG3=1;
                        break;
                    }

                }

                lhs.PutScalar(0.0);
                problem.SetOperator(&stiffness);
                problem.SetLHS(&lhs);
                problem.SetRHS(&rhs);
                solver.SetProblem(problem);
                solver.SetParameters(*Krylov);

                Time.ResetStartTime();
                solver.Iterate(2000,tol);
                Aztec_time = Time.ElapsedTime();
                Krylov_its = solver.NumIters();
                Krylov_res = solver.TrueResidual();
                x.Update(1.0,lhs,1.0);

                du.Import(lhs, *ImportToOverlapMap, Insert);

                FLAG3=1;
            }
        }
    }
    return 0;
}

void plasticitySmallStrains::assemblePureDirichlet_homogeneousForcing(const Epetra_Vector & du, Epetra_FECrsMatrix & K, Epetra_FEVector & F){

    F.PutScalar(0.0);
    K.PutScalar(0.0);

    stiffness_rhs_homogeneousForcing(du, K, F);

    Comm->Barrier();

    K.GlobalAssemble();
    K.FillComplete();
    F.GlobalAssemble();
}


void plasticitySmallStrains::stiffness_rhs_homogeneousForcing(const Epetra_Vector & du, Epetra_FECrsMatrix & K, Epetra_FEVector & F){

  int node, e_gid, error;
  int n_gauss_points = Mesh->n_gauss_cells;
  double gauss_weight;

  int *Indices_cells;
  Indices_cells = new int [3*Mesh->el_type];


  Epetra_SerialDenseVector du_el(3*Mesh->el_type);
  Epetra_SerialDenseVector sig_el(6);
  Epetra_SerialDenseVector deto_el(6);
  Epetra_SerialDenseMatrix m_tg_matrix(6,6);

  Epetra_SerialDenseVector Re(3*Mesh->el_type);
  Epetra_SerialDenseMatrix Ke(3*Mesh->el_type,3*Mesh->el_type);

  Epetra_SerialDenseMatrix dx_shape_functions(Mesh->el_type,3);
  Epetra_SerialDenseMatrix matrix_B(6,3*Mesh->el_type);
  Epetra_SerialDenseMatrix B_times_TM(3*Mesh->el_type,6);

  for (unsigned int e_lid=0; e_lid<Mesh->n_local_cells; ++e_lid){
      e_gid = Mesh->local_cells[e_lid];

      for (unsigned int inode=0; inode<Mesh->el_type; ++inode){
          node = Mesh->cells_nodes[Mesh->el_type*e_gid+inode];
          for (int iddl=0; iddl<3; ++iddl){
              Indices_cells[3*inode+iddl] = 3*node+iddl;
              Re(3*inode+iddl) = 0.0;
              du_el(3*inode+iddl) = du[OverlapMap->LID(3*node+iddl)];
              for (unsigned int jnode=0; jnode<Mesh->el_type; ++jnode){
                  for (int jddl=0; jddl<3; ++jddl){
                      Ke(3*inode+iddl,3*jnode+jddl) = 0.0;
                  }
              }
          }
      }

      for (unsigned int gp=0; gp<n_gauss_points; ++gp){
          gauss_weight = Mesh->gauss_weight_cells(gp);
          for (unsigned int inode=0; inode<Mesh->el_type; ++inode){
              dx_shape_functions(inode,0) = Mesh->DX_N_cells(gp+n_gauss_points*inode,e_lid);
              dx_shape_functions(inode,1) = Mesh->DY_N_cells(gp+n_gauss_points*inode,e_lid);
              dx_shape_functions(inode,2) = Mesh->DZ_N_cells(gp+n_gauss_points*inode,e_lid);
          }

          compute_B_matrices(dx_shape_functions,matrix_B);
          deto_el.Multiply('N','N',1.0,matrix_B,du_el,0.0);
          constitutive_problem(e_lid, gp, deto_el, sig_el, m_tg_matrix);

          error = Re.Multiply('T','N',gauss_weight*Mesh->detJac_cells(e_lid,gp),matrix_B,sig_el,1.0);
          error = B_times_TM.Multiply('T','N',gauss_weight*Mesh->detJac_cells(e_lid,gp),matrix_B,m_tg_matrix,0.0);
          error = Ke.Multiply('N','N',1.0,B_times_TM,matrix_B,1.0);
      }

      for (unsigned int i=0; i<3*Mesh->el_type; ++i){
          error = F.SumIntoGlobalValues(1, &Indices_cells[i], &Re(i));
          for (unsigned int j=0; j<3*Mesh->el_type; ++j){
              error = K.SumIntoGlobalValues(1, &Indices_cells[i], 1, &Indices_cells[j], &Ke(i,j));
          }
      }
  }
  delete[] Indices_cells;
}

void plasticitySmallStrains::assemblePureDirichlet_homogeneousForcing_LinearElasticity(Epetra_FECrsMatrix & K){
  int error;

  K.PutScalar(0.0);

  stiffness_homogeneousForcing_LinearElasticity(K);

  Comm->Barrier();

  error=K.GlobalAssemble();
  error=K.FillComplete();
}

void plasticitySmallStrains::stiffness_homogeneousForcing_LinearElasticity(Epetra_FECrsMatrix & K){
  int node, e_gid, error;
  int n_gauss_points = Mesh->n_gauss_cells;
  double gauss_weight;

  int *Indices_cells;
  Indices_cells = new int [3*Mesh->el_type];

  Epetra_SerialDenseMatrix Ke(3*Mesh->el_type,3*Mesh->el_type);

  Epetra_SerialDenseMatrix dx_shape_functions(Mesh->el_type,3);
  Epetra_SerialDenseMatrix matrix_B(6,3*Mesh->el_type);
  Epetra_SerialDenseMatrix B_times_TM(3*Mesh->el_type,6);

  for (unsigned int e_lid=0; e_lid<Mesh->n_local_cells; ++e_lid){
      e_gid = Mesh->local_cells[e_lid];

      for (unsigned int inode=0; inode<Mesh->el_type; ++inode){
          node = Mesh->cells_nodes[Mesh->el_type*e_gid+inode];
          for (int iddl=0; iddl<3; ++iddl){
              Indices_cells[3*inode+iddl] = 3*node+iddl;
              for (unsigned int jnode=0; jnode<Mesh->el_type; ++jnode){
                  for (int jddl=0; jddl<3; ++jddl){
                      Ke(3*inode+iddl,3*jnode+jddl) = 0.0;
                  }
              }
          }
      }

      for (unsigned int gp=0; gp<n_gauss_points; ++gp){
          gauss_weight = Mesh->gauss_weight_cells(gp);
          for (unsigned int inode=0; inode<Mesh->el_type; ++inode){
              dx_shape_functions(inode,0) = Mesh->DX_N_cells(gp+n_gauss_points*inode,e_lid);
              dx_shape_functions(inode,1) = Mesh->DY_N_cells(gp+n_gauss_points*inode,e_lid);
              dx_shape_functions(inode,2) = Mesh->DZ_N_cells(gp+n_gauss_points*inode,e_lid);
          }

          compute_B_matrices(dx_shape_functions,matrix_B);
          get_elasticity_tensor(e_lid, gp, ELASTICITY);

          error = B_times_TM.Multiply('T','N',gauss_weight*Mesh->detJac_cells(e_lid,gp),matrix_B,ELASTICITY,0.0);
          error = Ke.Multiply('N','N',1.0,B_times_TM,matrix_B,1.0);
      }

      for (unsigned int i=0; i<3*Mesh->el_type; ++i){
          for (unsigned int j=0; j<3*Mesh->el_type; ++j){
              error = K.SumIntoGlobalValues(1, &Indices_cells[i], 1, &Indices_cells[j], &Ke(i,j));
          }
      }
  }
  delete[] Indices_cells;
}

void plasticitySmallStrains::compute_B_matrices(Epetra_SerialDenseMatrix & dx_shape_functions, Epetra_SerialDenseMatrix & B){
    double factor = 1.0/std::sqrt(2.0);
    for (unsigned inode=0; inode<Mesh->el_type; ++inode){
        B(0,3*inode) = dx_shape_functions(inode,0);
        B(0,3*inode+1) = 0.0;
        B(0,3*inode+2) = 0.0;

        B(1,3*inode) = 0.0;
        B(1,3*inode+1) = dx_shape_functions(inode,1);
        B(1,3*inode+2) = 0.0;

        B(2,3*inode) = 0.0;
        B(2,3*inode+1) = 0.0;
        B(2,3*inode+2) = dx_shape_functions(inode,2);

        B(3,3*inode) = 0.0;
        B(3,3*inode+1) = factor*dx_shape_functions(inode,2);
        B(3,3*inode+2) = factor*dx_shape_functions(inode,1);

        B(4,3*inode) = factor*dx_shape_functions(inode,2);
        B(4,3*inode+1) = 0.0;
        B(4,3*inode+2) = factor*dx_shape_functions(inode,0);

        B(5,3*inode) = factor*dx_shape_functions(inode,1);
        B(5,3*inode+1) = factor*dx_shape_functions(inode,0);
        B(5,3*inode+2) = 0.0;
    }
}


void plasticitySmallStrains::constructGaussMap(Epetra_Map & GaussMap){
  int e_gid;
  int n_local_cells = Mesh->n_local_cells;
  int n_gauss_cells = Mesh->n_gauss_cells;
  std::vector<int> local_gauss_points(n_local_cells*n_gauss_cells);
  for (unsigned int e_lid=0; e_lid<n_local_cells; ++e_lid){
      e_gid = Mesh->local_cells[e_lid];
      for (unsigned int gp=0; gp<n_gauss_cells; ++gp){
          local_gauss_points[e_lid*n_gauss_cells+gp] = e_gid*n_gauss_cells+gp;
      }
  }
  GaussMap = Epetra_Map(-1, n_local_cells*n_gauss_cells, &local_gauss_points[0], 0, *Comm);
}

void plasticitySmallStrains::create_FECrsGraph(){
    FEGraph = new Epetra_FECrsGraph(Copy,*StandardMap,100);
    int eglob, node;
    int *index;
    index = new int [3*Mesh->el_type];

    for (int e_lid=0; e_lid<Mesh->n_local_cells; ++e_lid){
        eglob = Mesh->local_cells[e_lid];
        for (int inode=0; inode<Mesh->el_type; ++inode){
            node = Mesh->cells_nodes[Mesh->el_type*eglob+inode];
            for (int ddl=0; ddl<3; ++ddl){
                index[3*inode+ddl] = 3*node+ddl;
            }
        }
        for (int i=0; i<3*Mesh->el_type; ++i){
            for (int j=0; j<3*Mesh->el_type; ++j){
                FEGraph->InsertGlobalIndices(1, &index[i], 1, &index[j]);
            }
        }
    }
    Comm->Barrier();
    FEGraph->GlobalAssemble();
    delete[] index;
}
