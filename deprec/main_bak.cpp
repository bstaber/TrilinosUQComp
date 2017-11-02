#include "Epetra_ConfigDefs.h"
#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

#include "Newton_Raphsonpp.hpp"
#include "objectiveFunction.hpp"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/math/special_functions/gamma.hpp>

Epetra_SerialDenseVector randhypersph(Epetra_SerialDenseVector & x,
                                      boost::random::normal_distribution<double> & w,
                                      boost::random::mt19937 & rng);
Epetra_SerialDenseVector mvrandn(Epetra_SerialDenseVector & x,
                                 Epetra_SerialDenseMatrix & sigma,
                                 boost::random::normal_distribution<double> & w,
                                 boost::random::mt19937 & rng);
void printHeader(Epetra_Comm & comm);
void printStatus(Epetra_Comm & comm, int eval, double value, Epetra_SerialDenseVector & x);

int main(int argc, char *argv[]){

    std::string    xmlInFileName = "";

    Teuchos::CommandLineProcessor  clp(false);
    clp.setOption("xml-in-file",&xmlInFileName,"The XML file to read into a parameter list");
    clp.setDocString("TO DO.");

    Teuchos::CommandLineProcessor::EParseCommandLineReturn
    parse_return = clp.parse(argc,argv);
    if( parse_return != Teuchos::CommandLineProcessor::PARSE_SUCCESSFUL ) {
        std::cout << "\nEnd Result: TEST FAILED" << std::endl;
        return parse_return;
    }

#ifdef HAVE_MPI
    MPI_Init(&argc, &argv);
    Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
    Epetra_SerialComm Comm;
#endif

    Teuchos::RCP<Teuchos::ParameterList> paramList = Teuchos::rcp(new Teuchos::ParameterList);
    if(xmlInFileName.length()){
        Teuchos::updateParametersFromXmlFile(xmlInFileName, inoutArg(*paramList));
    }
    if (Comm.MyPID()==0){
        paramList->print(std::cout,2,true,true);
    }

    Teuchos::RCP<objectiveFunction> obj = Teuchos::rcp(new objectiveFunction(Comm,*paramList));

    Teuchos::RCP<Teuchos::ParameterList> parlist = Teuchos::rcp( new Teuchos::ParameterList() );
    if(xmlInFileName.length()) {
        Teuchos::updateParametersFromXmlFile(xmlInFileName, inoutArg(*parlist));
    }

    int nparam = 8;
    Epetra_SerialDenseVector lb(nparam);
    Epetra_SerialDenseVector ub(nparam);

    lb(0) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu1_inf");
    ub(0) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu1_sup");
    lb(1) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu2_inf");
    ub(1) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu2_sup");
    lb(2) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu3_inf");
    ub(2) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu3_sup");
    lb(3) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu4_inf");
    ub(3) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu4_sup");
    lb(4) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu5_inf");
    ub(4) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu5_sup");
    lb(5) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta3_inf");
    ub(5) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta3_sup");
    lb(6) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta4_inf");
    ub(6) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta4_sup");
    lb(7) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta5_inf");
    ub(7) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta5_sup");

    //may be this should only be done on root
    boost::random::mt19937 rng(std::time(0));
    boost::random::normal_distribution<double> randn(0.0,1.0);
    boost::random::uniform_real_distribution<double> rand(0.0,1.0);

    Epetra_SerialDenseVector u(nparam);
    Epetra_SerialDenseVector v(nparam);
    Epetra_SerialDenseVector x(nparam);
    Epetra_SerialDenseMatrix L(nparam,nparam);

    printHeader(Comm);
    int eval = 0;
    double value = 1.0;
    x(0) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu1");
    x(1) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu2");
    x(2) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu3");
    x(3) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu4");
    x(4) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu5");
    x(5) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta3");
    x(6) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta4");
    x(7) = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta5");
    for (unsigned int i=0; i<5; i++){
        x(i) = 1.0e3*x(i);
    }
    /*while (value>=1.0){
        if (Comm.MyPID()==0){
            for (unsigned int j=0; j<nparam; ++j){
                x(j) = (ub(j)-lb(j))*rand(rng) + lb(j);
            }
        }
        Comm.Broadcast(x.Values(),x.Length(),0);
        Comm.Barrier();*/
        value = obj->value(x);
        eval++;
        printStatus(Comm,eval,value,x);
    //}
    /*
    double svalue = value;
    while(value>1e-2){
        for (unsigned int i=0; i<nparam; ++i){
            L(i,i) = 0.1*x(i);
        }
        v = x;

        while(value>=svalue){
            if (Comm.MyPID()==0){
                    x = mvrandn(v,L,randn,rng);
                    for (unsigned int j=0; j<nparam; ++j){
                        if (x(j)<lb(j)){
                            x(j)=lb(j);
                        }
                        if (x(j)>ub(j)){
                            x(j)=ub(j);
                        }
                    }
            }
            Comm.Broadcast(x.Values(),x.Length(),0);
            Comm.Barrier();
            value = obj->value(x);
            printStatus(Comm,eval,value,x);
            eval++;
        }//endwhile

        svalue = value;

    }//endwhile */

#ifdef HAVE_MPI
    MPI_Finalize();
#endif
return 0;
}

Epetra_SerialDenseVector mvrandn(Epetra_SerialDenseVector & x,
                                 Epetra_SerialDenseMatrix & L,
                                 boost::random::normal_distribution<double> & w,
                                 boost::random::mt19937 & rng){
    int n = x.Length();
    Epetra_SerialDenseVector z(n);
    Epetra_SerialDenseVector g(n);

    for (unsigned int j=0; j<n; ++j){
        z(j) = w(rng);
    }

    g.Multiply('N','N',1.0,L,z,0.0);
    g+=x;

    return g;
}

Epetra_SerialDenseVector randhypersph(Epetra_SerialDenseVector & v,
                                      boost::random::normal_distribution<double> & w,
                                      boost::random::mt19937 & rng){
    int n = v.Length();
    Epetra_SerialDenseVector X(n);
    Epetra_SerialDenseVector Y(n);
    Epetra_SerialDenseVector u(n);

    double radius = 0.25;

    for (unsigned int j=0; j<n; ++j){
        X(j) = w(rng);
    }
    double s = X.Norm2();
    double gammainc = boost::math::gamma_p<double,double>(s*s/2.0,double(n)/2.0);
    for (unsigned int j=0; j<n; ++j){
        Y(j) = X(j)*(radius*std::pow(gammainc,1.0/double(n))/s);
        u(j) = Y(j) + v(j);
    }

    return u;
}

void printHeader(Epetra_Comm & comm){
    comm.Barrier();
    if (comm.MyPID()==0){
        std::cout << "Direct Random Search Algorithm\n";
        std::cout << std::setw(10) << "#eval" << std::setw(15) << "value";
        for (unsigned int i=0; i<8; ++i){
            std::cout << std::setw(10) << "x(" << i << ")";
        }
        std::cout << "\n";
    }
}

void printStatus(Epetra_Comm & comm, int eval, double value, Epetra_SerialDenseVector & x){
    comm.Barrier();
    if (comm.MyPID()==0){
        std::cout << std::setw(10) << eval << std::setw(15) << std::scientific << value;
        for (unsigned int j=0; j<x.Length(); ++j){
            std::cout << std::setw(15) << x(j);
        }
        std::cout << "\n";
    }
}