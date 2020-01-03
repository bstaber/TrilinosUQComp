/*
Brian Staber (brian.staber@gmail.com)

1) von Mises plate under tension
2) Fenics and Zset give same results

*/

#ifndef TRESCA_PLATE_HPP
#define TRESCA_PLATE_HPP

#include "plasticitySmallStrains.hpp"

class tresca_plate : public plasticitySmallStrains
{
public:

    double E; // = 210000.0;
    double nu; // = 0.3;
    double lambda; // = E*nu/((1.0+nu)*(1.0-2.0*nu));
    double mu; // = E/(2.0*(1.0+nu));
    double kappa; // = lambda + 2.0*mu/3.0;

    double R0; // = 1000.0;
    double H; //  = 10000.0;

    Epetra_SerialDenseMatrix K4;
    Epetra_SerialDenseMatrix J4;

    Teuchos::ParameterList * Krylov;

    tresca_plate(Epetra_Comm & comm, Teuchos::ParameterList & Parameters){

      plasticitySmallStrains::initialize(comm, Parameters);

      E  = Teuchos::getParameter<double>(Parameters.sublist("Behavior"), "young");
      nu = Teuchos::getParameter<double>(Parameters.sublist("Behavior"), "poisson");
      R0 = Teuchos::getParameter<double>(Parameters.sublist("Behavior"), "yield");
      H  = Teuchos::getParameter<double>(Parameters.sublist("Behavior"), "hardening");

      lambda = E*nu/((1.0+nu)*(1.0-2.0*nu));
      mu = E/(2.0*(1.0+nu));
      kappa = lambda + 2.0*mu/3.0;

      K4.Reshape(6,6);
      J4.Reshape(6,6);

      double c = 1.0/3.0;
      double d = 2.0/3.0;

      J4(0,0) = c;   J4(0,1) = c;   J4(0,2) = c;   J4(0,3) = 0.0; J4(0,4) = 0.0; J4(0,5) = 0.0;
      J4(1,0) = c;   J4(1,1) = c;   J4(1,2) = c;   J4(1,3) = 0.0; J4(1,4) = 0.0; J4(1,5) = 0.0;
      J4(2,0) = c;   J4(2,1) = c;   J4(2,2) = c;   J4(2,3) = 0.0; J4(2,4) = 0.0; J4(2,5) = 0.0;
      J4(3,0) = 0.0; J4(3,1) = 0.0; J4(3,2) = 0.0; J4(3,3) = 0.0; J4(3,4) = 0.0; J4(3,5) = 0.0;
      J4(4,0) = 0.0; J4(4,1) = 0.0; J4(4,2) = 0.0; J4(4,3) = 0.0; J4(4,4) = 0.0; J4(4,5) = 0.0;
      J4(5,0) = 0.0; J4(5,1) = 0.0; J4(5,2) = 0.0; J4(5,3) = 0.0; J4(5,4) = 0.0; J4(5,5) = 0.0;

      K4(0,0) = d;    K4(0,1) = -c;   K4(0,2) = -c;  K4(0,3) = 0.0; K4(0,4) = 0.0; K4(0,5) = 0.0;
      K4(1,0) = -c;   K4(1,1) = d;    K4(1,2) = -c;  K4(1,3) = 0.0; K4(1,4) = 0.0; K4(1,5) = 0.0;
      K4(2,0) = -c;   K4(2,1) = -c;   K4(2,2) = d;   K4(2,3) = 0.0; K4(2,4) = 0.0; K4(2,5) = 0.0;
      K4(3,0) = 0.0;  K4(3,1) = 0.0;  K4(3,2) = 0.0; K4(3,3) = 1.0; K4(3,4) = 0.0; K4(3,5) = 0.0;
      K4(4,0) = 0.0;  K4(4,1) = 0.0;  K4(4,2) = 0.0; K4(4,3) = 0.0; K4(4,4) = 1.0; K4(4,5) = 0.0;
      K4(5,0) = 0.0;  K4(5,1) = 0.0;  K4(5,2) = 0.0; K4(5,3) = 0.0; K4(5,4) = 0.0; K4(5,5) = 1.0;

    }

    ~tresca_plate(){
    }

    void constitutive_problem(const unsigned int & elid, const unsigned int & igp,
                              const Epetra_SerialDenseVector & DETO, Epetra_SerialDenseVector & SIG,
                              double & EPCUM, Epetra_SerialDenseMatrix & TGM){

      Epetra_SerialDenseVector SIGTR(6);
      Epetra_SerialDenseVector DSIG(6);
      Epetra_SerialDenseVector DEVTR(6);
      Epetra_SerialDenseVector EYE(6);
      EYE(0) = 1.0; EYE(1) = 1.0; EYE(2) = 1.0; EYE(3) = 0.0; EYE(4) = 0.0; EYE(5) = 0.0;

      get_elasticity_tensor(elid, igp, ELASTICITY);
      DSIG.Multiply('N','N',1.0,ELASTICITY,DETO,0.0);
      for (unsigned int k=0; k<6; ++k) SIGTR(k) = SIG(k) + DSIG(k);

      double pressure = (1.0/3.0)*(SIGTR(0)+SIGTR(1)+SIGTR(2));

      DEVTR = SIGTR;
      DEVTR(0) -= pressure;
      DEVTR(1) -= pressure;
      DEVTR(2) -= pressure;

      double devdev = DEVTR(0)*DEVTR(0) + DEVTR(1)*DEVTR(1) + DEVTR(2)*DEVTR(2)
                    + DEVTR(3)*DEVTR(3) + DEVTR(4)*DEVTR(4) + DEVTR(5)*DEVTR(5);
      double qtrial = std::sqrt(1.5*devdev);
      double yield = qtrial - R0 - H*EPCUM;

      Epetra_SerialDenseMatrix NN(6,6);
      for (unsigned int i=0; i<6; ++i){
        for (unsigned int j=0; j<6; ++j){
          NN(i,j) = DEVTR(i)*DEVTR(j)/devdev;
        }
      }

      SIG = SIGTR;
      TGM = ELASTICITY;
      if (yield>1.0e-10) {
        double gamma = yield/(3.0*mu+H);
        double beta  = 3.0*mu*gamma/qtrial;
        for (unsigned int k=0; k<6; ++k) SIG(k) -= beta*DEVTR(k);
        EPCUM += gamma;
        for (unsigned int i=0; i<6; ++i){
          for (unsigned int j=0; j<6; ++j){
            TGM(i,j) += 6.0*mu*mu*(gamma/qtrial - (1.0/(3.0*mu+H)))*NN(i,j) - 2.0*mu*beta*K4(i,j);
          }
        }
      }

    }

    void get_elasticity_tensor(const unsigned int & e_lid, const unsigned int & gp, Epetra_SerialDenseMatrix & tgm){
        double c1 = lambda+2.0*mu;
        tgm(0,0) = c1;     tgm(0,1) = lambda; tgm(0,2) = lambda; tgm(0,3) = 0.0;     tgm(0,4) = 0.0;    tgm(0,5) = 0.0;
        tgm(1,0) = lambda; tgm(1,1) = c1;     tgm(1,2) = lambda; tgm(1,3) = 0.0;     tgm(1,4) = 0.0;    tgm(1,5) = 0.0;
        tgm(2,0) = lambda; tgm(2,1) = lambda; tgm(2,2) = c1;     tgm(2,3) = 0.0;     tgm(2,4) = 0.0;    tgm(2,5) = 0.0;
        tgm(3,0) = 0.0;    tgm(3,1) = 0.0;    tgm(3,2) = 0.0;    tgm(3,3) = 2.0*mu;  tgm(3,4) = 0.0;    tgm(3,5) = 0.0;
        tgm(4,0) = 0.0;    tgm(4,1) = 0.0;    tgm(4,2) = 0.0;    tgm(4,3) = 0.0;     tgm(4,4) = 2.0*mu; tgm(4,5) = 0.0;
        tgm(5,0) = 0.0;    tgm(5,1) = 0.0;    tgm(5,2) = 0.0;    tgm(5,3) = 0.0;     tgm(5,4) = 0.0;    tgm(5,5) = 2.0*mu;
    }

    void setup_dirichlet_conditions(){
        n_bc_dof = 0;
        double x,y,z;
        unsigned int node;
        for (unsigned int i=0; i<Mesh->n_local_nodes_without_ghosts; ++i){
            node = Mesh->local_nodes[i];
            x = Mesh->nodes_coord[3*node+0];
            y = Mesh->nodes_coord[3*node+1];
            z = Mesh->nodes_coord[3*node+2];
            if(y==0.0||y==10.0){
                n_bc_dof+=3;
            }
        }

        int indbc = 0;
        dof_on_boundary = new int [n_bc_dof];
        for (unsigned int inode=0; inode<Mesh->n_local_nodes_without_ghosts; ++inode){
            node = Mesh->local_nodes[inode];
            x = Mesh->nodes_coord[3*node+0];
            y = Mesh->nodes_coord[3*node+1];
            z = Mesh->nodes_coord[3*node+2];
            if(y==0.0||y==10.0){
                dof_on_boundary[indbc+0] = 3*inode+0;
                dof_on_boundary[indbc+1] = 3*inode+1;
                dof_on_boundary[indbc+2] = 3*inode+2;
                indbc+=3;
            }
        }
    }

    void apply_dirichlet_conditions(Epetra_FECrsMatrix & K, Epetra_FEVector & F, double factor){
        Epetra_MultiVector v(*StandardMap,true);
        v.PutScalar(0.0);

        int node;
        double x,y,z;
        for (unsigned int inode=0; inode<Mesh->n_local_nodes_without_ghosts; ++inode){
            node = Mesh->local_nodes[inode];
            x = Mesh->nodes_coord[3*node+0];
            y = Mesh->nodes_coord[3*node+1];
            z = Mesh->nodes_coord[3*node+2];
            if (y==0.0||y==10.0){
                v[0][StandardMap->LID(3*node+0)] = 0.0;
                v[0][StandardMap->LID(3*node+1)] = factor*y;
                v[0][StandardMap->LID(3*node+2)] = 0.0;
            }
        }

        Epetra_MultiVector rhs_dir(*StandardMap,true);
        K.Apply(v,rhs_dir);
        F.Update(-1.0,rhs_dir,1.0);

        for (unsigned int inode=0; inode<Mesh->n_local_nodes_without_ghosts; ++inode){
            node = Mesh->local_nodes[inode];
            x = Mesh->nodes_coord[3*node+0];
            y = Mesh->nodes_coord[3*node+1];
            z = Mesh->nodes_coord[3*node+2];
            if (y==0.0||y==10.0){
                F[0][StandardMap->LID(3*node+0)] = v[0][StandardMap->LID(3*node+0)];
                F[0][StandardMap->LID(3*node+1)] = v[0][StandardMap->LID(3*node+1)];
                F[0][StandardMap->LID(3*node+2)] = v[0][StandardMap->LID(3*node+2)];
            }
        }
        ML_Epetra::Apply_OAZToMatrix(dof_on_boundary,n_bc_dof,K);
    }
};
#endif
