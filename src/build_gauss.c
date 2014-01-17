#include "STeM.h"

// Fonciton qui construit les eigen

int main(int argc, char *argv[])
{
	int all; /*Nombre datomes dans pdb*/
	
	int atom; /*Nombre de carbones CA*/
	
	int i,j,k;
	
	int help_flag = 1;
	double beta = 0.000001;
	char file_name[500];
	int lig = 0;
	char matrix_name[500] = "interaction_m.dat";
	char param_name[500] = "param_m.dat";
	char inputname[500] ="none";
	int verbose = 0;
	int nconn;
	float vinit = 1; // Valeur de base
	float bond_factor = 1;		// Facteur pour poid des bond strechcing
	float angle_factor = 1;		// Facteur pour poid des angles
	double K_phi1 = 1;				// Facteurs pour angles dièdres
	double K_phi3 = 0.5;
	float init_templaate = 1;
	float kp_factor = 1;					// Facteur pour poid des angles dièdres
	int weight_factor = 0;
	char out_name[500] = "DEFAULT";
	char hessian_name[500] = "hessian.dat";
	int hessian_flag = 0;
	float temperature = 310;
	int print_template = 0;
	int no_write = 0;
	int ihess = 0;
	
 	for (i = 1;i < argc;i++)
	{
 		if (strcmp("-i",argv[i]) == 0) {strcpy(file_name,argv[i+1]);--help_flag;}
 		if (strcmp("-inp",argv[i]) == 0) {strcpy(inputname,argv[i+1]);--help_flag;}
 		if (strcmp("-h",argv[i]) == 0) {++help_flag;}
 		if (strcmp("-v",argv[i]) == 0) {verbose = 1;}
 		if (strcmp("-lig",argv[i]) == 0) {lig= 1;}
 		if (strcmp("-m",argv[i]) == 0) {strcpy(matrix_name,argv[i+1]);}
 		if (strcmp("-param",argv[i]) == 0) {strcpy(param_name,argv[i+1]);}
 		if (strcmp("-init",argv[i]) == 0) {float temp;sscanf(argv[i+1],"%f",&temp);vinit = temp;}
 		if (strcmp("-t",argv[i]) == 0) {float temp;sscanf(argv[i+1],"%f",&temp);init_templaate = temp;} 
 		if (strcmp("-kr",argv[i]) == 0) {float temp;sscanf(argv[i+1],"%f",&temp);bond_factor = temp;}
 		if (strcmp("-kt",argv[i]) == 0) {float temp;sscanf(argv[i+1],"%f",&temp);angle_factor = temp;}
 		if (strcmp("-kpf",argv[i]) == 0) {float temp;sscanf(argv[i+1],"%f",&temp); kp_factor = temp;}
 		if (strcmp("-temp",argv[i]) == 0) {float temp;sscanf(argv[i+1],"%f",&temp); temperature = temp;}
 		if (strcmp("-w",argv[i]) == 0) {weight_factor = 1;}
 		if (strcmp("-o",argv[i]) == 0) {strcpy(out_name,argv[i+1]);}
 		if (strcmp("-hes",argv[i]) == 0) {strcpy(hessian_name,argv[i+1]);++hessian_flag;}
 		if (strcmp("-pt",argv[i]) == 0) {print_template = 1;}
 		if (strcmp("-no",argv[i]) == 0) {no_write = 1;}
 		if (strcmp("-b",argv[i]) == 0) {float temp;sscanf(argv[i+1],"%f",&temp);beta = temp;}
 		if (strcmp("-ih",argv[i]) == 0) {ihess = 1;}
 	}
 	
 	printf("\n********************\nFile:%s\nAlpha1:%f\nAlpha2:%f\nAlpha3:%f\nAlpha4:%f\nAlpha5:%f\n********************\n",
 	file_name,bond_factor,angle_factor,kp_factor,init_templaate,vinit);
 	
 	//***************************************************
 	//*													*
 	//*Build a structure contaning information on the pdb
 	//*													*
 	//***************************************************
 	
 	all = count_atom(file_name);
	
 	nconn = count_connect(file_name);
 	
 	if (verbose == 1) {printf("Connect:%d\n",nconn);}
 	
	if (verbose == 1) {printf("Assigning Structure\n\tAll Atom\n");}
	
	// Array qui contient tous les connects
	
	int **connect_h=(int **)malloc(nconn*sizeof(int *));
	
	for(i=0;i<nconn;i++) { connect_h[i]=(int *)malloc(7*sizeof(int));}
	
	assign_connect(file_name,connect_h);
	
	// Assign tous les atoms
	
	struct pdb_atom strc_all[all];
	
	atom = build_all_strc(file_name,strc_all); // Retourne le nombre de Node
	
	if (verbose == 1) {printf("	Node:%d\n	Atom:%d\n",atom,all);}

	check_lig(strc_all,connect_h,nconn,all);

	// Assign les Nodes
	
	if (verbose == 1) {printf("	CA Structure\n");}
	
	if (verbose == 1) {printf("	Node:%d\n",atom);}
	struct pdb_atom strc_node[atom];
	atom = build_cord_CA(strc_all, strc_node,all,lig,connect_h,nconn);
	if (verbose == 1) {printf("	Assign Node:%d\n",atom);}
		
	// Pour les besoins... on limite à 800 atomes
	
	if (atom > 2000) {
		printf("Too much atoms, if you want to remove the limit.... vincent.frappier@usherbrooke.ca\n");
		return(1);
	}
	
	
	//***************************************************
	//*													*
	//*Build Hessian									*
	//*													*
	//***************************************************
	
	
	double **hessian=(double **)malloc(3*atom*sizeof(double *)); // Matrix of the Hessian 1 2 3 (bond, angle, dihedral)
	
	for(i=0;i<3*atom;i++) { hessian[i]=(double *)malloc(3*atom*sizeof(double));}
	
	for(i=0;i<3*atom;i++)for(j=0;j<(3*atom);j++){hessian[i][j]=0;}
	
	gsl_matrix *h_matrix = gsl_matrix_alloc(3*atom, 3*atom);	/*Déclare une matrice hessian matrix de 3n par 3n*/
	
	//***************************************************
	//*													*
	//*Build template									*
	//*													*
	//***************************************************
	
	assign_atom_type(strc_all, all);
	
	if (strcmp(inputname,"none") == 0) {} else {assign_lig_type(strc_all, all, inputname);}
	
	gsl_matrix *vcon = gsl_matrix_alloc(all,all);
	
	gsl_matrix *inter_m = gsl_matrix_alloc(8,8);
	
	gsl_matrix *templaate = gsl_matrix_alloc(atom*3, atom*3);
	
	gsl_matrix_set_all(templaate,vinit);
	
	gsl_matrix_set_all(vcon,0);
	
	if (verbose == 1) {printf("Do Vcon !!!\n");}
	
	vcon_file_dom(strc_all,vcon,all);
	
	if (verbose == 1) {printf("Reading Interaction Matrix %s\n",matrix_name);}
	
	load_matrix(inter_m,matrix_name);
	
	//write_matrix("vcon_vince.dat", vcon,all,all);
	
	if (verbose == 1) {printf("Building templaate\n");}
	
	all_interaction(strc_all,all, atom, templaate,lig,vcon,inter_m,strc_node);
	
	gsl_matrix_scale (templaate, init_templaate);
	
	if (print_template == 1) {write_matrix("template.dat", templaate,atom,atom);}
	
	if (verbose == 1) {printf("Building Hessian\n");}
	
	if (verbose == 1) {printf("\tCovalent Bond Potential\n");}
	build_1st_matrix(strc_node,hessian,atom,bond_factor);
	
	if (verbose == 1) {printf("\tAngle Potential\n");}	
	build_2_matrix(strc_node,hessian,atom,angle_factor);
	
	if (verbose == 1) {printf("\tDihedral Potential\n");}	
	build_3_matrix(strc_node, hessian,atom,K_phi1/2+K_phi3*9/2,kp_factor);
	
	if (verbose == 1) {printf("\tNon Local Interaction Potential\n");}	
	build_4h_matrix(strc_node,hessian,atom,1.0,templaate);
	
 	if (verbose == 1) {printf("\tAssigning Array\n");}	
	assignArray(h_matrix,hessian,3*atom,3*atom);
	
	if (weight_factor == 1) {mass_weight_hessian(h_matrix,atom,strc_node);}
	
	if (hessian_flag != 0)
	{
		printf("Writing Hessian\n");
		write_matrix(hessian_name,h_matrix,3*atom,3*atom);
	}
	
	
	
	//***************************************************
	//*													*
	//*Diagonalyse the matrix										*
	//*													*
	//***************************************************
	
	
	if (verbose == 1) {printf("Diagonalizing Hessian\n");}
	
	gsl_vector *eval = gsl_vector_alloc(3*atom); /*Déclare un vecteur qui va contenir les eigenvalues*/
	
	gsl_matrix *evec= gsl_matrix_alloc (3*atom,3*atom); /*Déclare une matrice qui va contenir les eigenvectors correspondant aux evals*/
	
	diagonalyse_matrix(h_matrix,3*atom,eval,evec); /*Diagonalise la matrice*/
	
	if (weight_factor == 1) {adjust_weight_evec(evec,atom,strc_node);}
	
	if (verbose == 1)
	{
		printf("First eigenvalues\n");
		for (i=0;i<10;++i) {printf("I:%d %.10f\n",i,gsl_vector_get(eval,i));}
	}
	
	/*
	
	char eigen_name[500];
	
	strcpy(eigen_name, out_name);
	strcat(eigen_name, "_eigen.dat");
	
	if (no_write == 0) {write_eigen(eigen_name,evec,eval,3*atom);}
	
	*/
	
	gsl_matrix *k_totinv = gsl_matrix_alloc(atom*3, atom*3); /* Déclare et crée une matrice qui va être le pseudo inverse */
	
	k_cov_inv_matrix_stem(k_totinv,atom,eval,evec,6,atom*3-6); /* Génère une matrice contenant les superéléments diagonaux de la pseudo-inverse. */
	
	gsl_matrix_free(templaate);
	gsl_matrix_free(inter_m);
	gsl_matrix_free(vcon);
	
	printf("Generating gaussians\n");
	
	if(ihess == 1) {double_gauss(strc_node, k_totinv, atom, beta);}
	else {gen_gauss(strc_node, evec, eval, atom, beta, atom*3 - 6);}
	
	gsl_matrix_free(h_matrix);
	gsl_vector_free(eval);
	gsl_matrix_free(evec);
	
	/*
	
	gsl_vector *gauss_evals = gsl_vector_alloc(atom);
	
	for(i = 0; i < atom; i++)
	{
		if(ihess == 1)
		{
			gsl_vector_set(gauss_evals, i, strc_node[i].super_ihvars[0]);
		}
		else
		{
			gsl_vector_set(gauss_evals, i, strc_node[i].main_vars[0]);
		}
	}
	
	gsl_sort_vector(gauss_evals);
	
	for(i = 0; i < atom; i++)
	{
		for(j = 0; j < 3; j++)
		{
			if(ihess == 1)
			{
				strc_node[i].super_ihvars[j] = strc_node[i].super_ihvars[j] / gsl_vector_get(gauss_evals, 0) * 500.0;
			}
			else
			{
				strc_node[i].main_vars[j] = strc_node[i].main_vars[j] / gsl_vector_get(gauss_evals, 0) * 500.0;
			}
			
			for(k = 0; k < 3; k++)
			{
				if(ihess == 1)
				{
					strc_node[i].super_ih[j][k] = strc_node[i].super_ih[j][k] / gsl_vector_get(gauss_evals, 0) * 500.0;
				}
				else
				{
					strc_node[i].covar[j][k] = strc_node[i].covar[j][k] / gsl_vector_get(gauss_evals, 0) * 500.0;
				}
			}
		}
	}
	
	*/
	
	assign_anisou_all(strc_node,atom,strc_all,all, ihess);
	
	char writefile_name[500];
	
	strcpy(writefile_name, out_name);
	
	if(ihess == 1)
	{
		strcat(writefile_name, "_ihess.pdb");
	}
	else
	{
		strcat(writefile_name, "_specc.pdb"); /* SPECC : Strictly Positive Energies Covariance Calculation */
	}
	
	write_anisou_file(writefile_name, strc_all,all,ihess);
	
	free(connect_h);
	
	return(1);
	
}