/*
    Autor: Ondřej Šlampa, xslamp01@stud.fit.vutbr.cz
    Projekt: PRL proj3
    Popis: Implementace algoitmu mesh multiplication pomocí OpenMPI.
*/

#include<mpi.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>

using namespace std;

#define BROAD 0
#define M1 1
#define M2 2
#define SYNC 3

//jméno vstupních souborů
string INPUT_FILE1="mat1";
string INPUT_FILE2="mat2";

/*
	Rozdělí řetězec celých čísel na vektor celých čísel.
	s řetězec
	d oddělovač
*/
vector<int> split_string_into_ints(string s, char d){
	stringstream ss;
	ss<<s;
	vector<int>r;
	string tmp;
	
	while(!ss.eof()){
		getline(ss, tmp, d);
		r.push_back(stoi(tmp));
	}
	
	return r;
}

/*
	Načte matici ze souboru.
	input cesta ke vstupnímu souboru
*/
vector<vector<int>> load_matrix(string input){
	//výsledná matice
	vector<vector<int>> r;
	//načítaný řádek matice
	string tmp;
	
	//vstupní soubor
	ifstream s;
    s.open(input.c_str(), ios::in);
	
	if(!s.is_open()){
		return r;
	}
	
	//přeskočení prvního řádku
	while(s.get()!='\n');
	
	while(!s.eof()){
		getline(s, tmp);
		if(!tmp.empty()){
			r.push_back(split_string_into_ints(tmp,' '));
		}
	}
	
	return r;
}

/*
	Vypíše výslednou matici
	n,p rozměry matice
*/
void print_result(int n,int p){
	MPI_Status stat;
	//tisk rozměrů výstupní matice
    cout<<n<<":"<<p<<"\n";
    int r=0;
    
    for(int i=0;i<n;i++){
    	for(int j=0;j<p;j++){
    		if(j!=0){
    			cout<<" ";
    		}
    		MPI_Recv(&r, 1, MPI_INT, i*p+j, SYNC, MPI_COMM_WORLD, &stat);
    		cout<<r;
    	}
    	cout<<endl;
    }
}

/*
	Činnost výpočetního procesu.
	id id procesu
	numprocs počet procesů
	n,m,p rozměry matic
*/
void agent(int id, int numprocs, int n, int m, int p){
	MPI_Status stat;
	
	//načtení rozměrů od hlavního procesu
	if(n==-1||m==-1||p==-1){
		MPI_Bcast(&n, 1, MPI_INT, BROAD, MPI_COMM_WORLD);
		MPI_Bcast(&m, 1, MPI_INT, BROAD, MPI_COMM_WORLD);
		MPI_Bcast(&p, 1, MPI_INT, BROAD, MPI_COMM_WORLD);
	}
	
	//výpočet sousedních procesů
	int left=id%p==0?0:id-1;
	int right=id%p==p-1?-1:id+1;
	int up=id-p<=0?0:id-p;
	int down=id+p>=numprocs?-1:id+p;
	
	//čísla matice a suma
	int a;
	int b;
	int sum=0;
	
	//výpočet sumy m součinů
	for(int i=0;i<m;i++){
		//přijmutí čísel od sousedních procesů
		MPI_Recv(&a, 1, MPI_INT, left, M1, MPI_COMM_WORLD, &stat);
		MPI_Recv(&b, 1, MPI_INT, up, M2, MPI_COMM_WORLD, &stat);
		
		sum+=a*b;
		
		//odeslání čísel k sousedním procesů
		if(right!=-1){
			MPI_Send(&a, 1, MPI_INT, right, M1, MPI_COMM_WORLD);
		}
		
		if(down!=-1){
			MPI_Send(&b, 1, MPI_INT, down, M2, MPI_COMM_WORLD);
		}
	}
	
	//odeslání čísla k tisku řídícímu procesu.
	if(id!=0){
		MPI_Send(&sum, 1, MPI_INT, 0, SYNC, MPI_COMM_WORLD);
	}
	else{
		//struktura pro uložení dat o pořadavku na odeslání
    	MPI_Request request;
		MPI_Isend(&sum, 1, MPI_INT, 0, SYNC, MPI_COMM_WORLD, &request);
		print_result(n,p);
	}
}

/*
	Činnost prvního procesu.
	numprocs počet procesů
*/
void master(int numprocs){
	//načtení vstupních matic
	vector<vector<int>> m1=load_matrix(INPUT_FILE1);
    vector<vector<int>> m2=load_matrix(INPUT_FILE2);
	
	//načtení rozměrů
	int n=m1.size();
	int m=m1[0].size();
	int p=m2[0].size();
	
	//kontrola rozměrů
	if(m2.size()!=m){
		cerr<<"Matrix size error."<<endl;
	}
	
	//odeslání rozměrů ostatním procesům
    MPI_Bcast(&n, 1, MPI_INT, BROAD, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, BROAD, MPI_COMM_WORLD);
    MPI_Bcast(&p, 1, MPI_INT, BROAD, MPI_COMM_WORLD);
    
    //udává jestli bylo v předchozím cyklu odesílání čísel odesláno číslo
    bool send=true;
    int i=0;
    
    //struktura pro uložení dat o pořadavku na odeslání
    MPI_Request request;
    
    for(int i=0;i<m;i++){
    	for(int j=0;j<n;j++){
    		MPI_Isend(&(m1[j][i]), 1, MPI_INT, p*j, M1, MPI_COMM_WORLD, &request);
    	}
    	for(int k=0;k<p;k++){
    		MPI_Isend(&(m2[i][k]), 1, MPI_INT, k, M2, MPI_COMM_WORLD, &request);
    	}
    }
    
    //provedení výpočtu tohoto procesoru
    agent(0,numprocs,n,m,p);
}

/*
    Funkce Main.
    argc počet argumentů
    argv argumenty
*/
int main(int argc, char *argv[]){
    int numprocs;
    int id;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    
    if(id==0){
    	master(numprocs);
    }
    else{
    	agent(id, numprocs,-1,-1,-1);
    }
    
    MPI_Finalize();
}

