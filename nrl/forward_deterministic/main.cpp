#include "Epetra_ConfigDefs.h"
#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

#include "Compressible_Mooney_Transverse_Isotropic.hpp"
#include "Newton_Raphsonpp.hpp"

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
    if(xmlInFileName.length()) {
        Teuchos::updateParametersFromXmlFile(xmlInFileName, inoutArg(*paramList));
    }
    
    if (Comm.MyPID()==0){
        paramList->print(std::cout,2,true,true);
    }
    
    Epetra_SerialDenseVector parameters(7);
    Teuchos::RCP<TIMooney> interface = Teuchos::rcp(new TIMooney(Comm,*paramList));
    
    parameters(0)    = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu1");
    parameters(1)    = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu2");
    parameters(2)    = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu3");
    parameters(3)    = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu4");
    parameters(4)    = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"mu5");
    //parameters(5)    = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta3");
    parameters(5)    = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta4");
    parameters(6)    = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"beta5");
    double plyagldeg = Teuchos::getParameter<double>(paramList->sublist("TIMooney"),"angle");
    double plyagl    = 2.0*M_PI*plyagldeg/360.0;
    for (unsigned int i=0; i<5; i++){
        parameters(i) = 1.0e3*parameters(i);
    }
    
    interface->set_parameters(parameters);
    interface->set_plyagl(plyagl);
    
    Teuchos::RCP<Newton_Raphson> Newton = Teuchos::rcp(new Newton_Raphson(*interface,*paramList));
    
    std::vector<double> bcdisp(10);
    bcdisp[0] = 0.00033234;
    bcdisp[1] = 0.018369;
    bcdisp[2] = 0.038198;
    bcdisp[3] = 0.060977;
    bcdisp[4] = 0.073356;
    bcdisp[5] = 0.092648;
    bcdisp[6] = 0.11062;
    bcdisp[7] = 0.12838;
    bcdisp[8] = 0.14934;
    bcdisp[9] = 0.1571809118641;
    
    double xi = 0.0;
    
    Newton->Initialization();
    for (unsigned int i=0; i<bcdisp.size(); ++i){
        Newton->setParameters(*paramList);
        Newton->bc_disp = bcdisp[i];
        int error = Newton->Solve_with_Aztec(true);
        std::string path1 = "/home/s/staber/Trilinos_results/nrl/forward_deterministic/displacement_" + std::to_string(i) + ".mtx";
        std::string path2 = "/home/s/staber/Trilinos_results/nrl/forward_deterministic/greenlag_" + std::to_string(i) + ".mtx";
        Newton->print_newton_solution(path1);
        interface->compute_green_lagrange(*Newton->x,xi,xi,xi,path2);
    }
    
    
#ifdef HAVE_MPI
    MPI_Finalize();
#endif
return 0;
    
}