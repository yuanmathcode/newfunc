#include<iostream>
#include<set>
#include<vector>
#include<map>
#include<memory>
#include<array>
#include<string>
#include<sstream>
#include<fstream>
#include<cstring>
#include<iomanip>
#include<algorithm>
#include "gurobi_c++.h"
#define M 1000000

using namespace std;
class master;
class slave;
//class connectivity;

class master{
private:
	int _id;
public:
	master()=default;
	master(int given_id)
	:_id(given_id){}
	const int &id() {return _id;}
	void define_id(int g_id){
		_id=g_id;}
};
class slave{
private:
	int _id;
public:
	slave()=default;
	slave(int given_id)
	:_id(given_id){}
	const int &id() {return _id;}
	void define_id(int g_id){
		_id=g_id;}
};

class variable{
public:
        int id;
        int type;//0=bool,1=int,2=double
        double upbound;
        double lobound;
        bool bool_value;
        int int_value;
        double double_value;
        set<int> constr_involved;
        map<int,double> constr_coeff;

};

int objcons;
int sum_constr;
map<int,int> constr_sense_map;
map<int,double> constr_rightside_map;
map<int,set<shared_ptr<variable> > > constr_variable_map;
vector<shared_ptr<variable> > vvar;
map<int,map<int,map<int,shared_ptr<variable> > > > color_map;//i:color,j:origin node,k:end node
//map<int,map<int,shared_ptr<variable>>> color_map;
vector<shared_ptr<master> > vecmaster;
vector<shared_ptr<slave> > vecslave;
//vector<shared_ptr<connectivity>> veccon;
map<int,map<int,int> > edge;//edge[i][j]=1 or 2
double adfloss;
double droploss;
map<int,map<int,shared_ptr<variable> > > loop;
vector<set<int> > endslave;
void readfile(const char* filename){
	ifstream read(filename);
	string str_temp;
	int int_temp,int_temp1,int_temp2,int_temp3;
	double double_temp;
	while (read>>str_temp){
		//read master and slave
		if(str_temp=="Master"){
			while(read>>str_temp){
				if(str_temp=="end")
					break;
				int_temp=stoi(str_temp);
				shared_ptr<master> master_temp=make_shared<master>(int_temp);
				vecmaster.push_back(master_temp);
			}
		}
		if(str_temp=="Slave"){
			while(read>>str_temp){
				if(str_temp=="end")
					break;
				int_temp=stoi(str_temp);
				shared_ptr<slave> slave_temp=make_shared<slave>(int_temp);
				vecslave.push_back(slave_temp);
			}
		}
		//Read connectivity
		if(str_temp=="Connectivity:"){
			int index_temp=0;
			while(getline(read,str_temp)){
				if(str_temp=="end")
					break;
				if(str_temp.empty())
					continue;
			index_temp++;
			istringstream connect_record(str_temp);
			connect_record>>int_temp1>>int_temp2>>int_temp3;
			edge[int_temp1][int_temp2]=int_temp3;
			}
		}
                if(str_temp=="Adf:"){
                        while(read>>str_temp){
                                if(str_temp=="end")
                                        break;
                      		double_temp=stod(str_temp);
                        	adfloss=double_temp;
			}
                }
                if(str_temp=="Drop:"){
                        while(read>>str_temp){
                                if(str_temp=="end")
                                        break;
                        	double_temp=stod(str_temp);
                        	droploss=double_temp;
			}
                }
	}
}
/*	for(auto &i:vecnode){
		set<int> setofend;
		for(auto &j:vecedge){
			if(j->origin()->id()==i->id())
				setofend.insert(j->end()->id());
		}
		endnode.push_back(setofend);
        }
	for(int i=0;i<endnode.size();i++){
		for(auto j:endnode[i]){
			cout<<j;}}
        for(auto &i:endnode){
                cout<<"{";
                for(auto &j:i){
                        cout<<j;
                }
                cout<<"}"<<endl;
        }
	
        cout<<endnode.size()<<" "<<endl;*/

//define color_map[i][j][h] as binary variable:i means color,j means node;
void set_color_map(){
	for(int l=1;l<=vecmaster.size();l++){
		for(int h=1;h<=vecslave.size();h++){
			for(int i=0;i<=vecmaster.size();i++){
				color_map[i][l][h]=make_shared<variable>();
				color_map[i][l][h]->id=vvar.size();
				color_map[i][l][h]->type=0;
				color_map[i][l][h]->upbound=1;
				color_map[i][l][h]->lobound=0;
				vvar.push_back(color_map[i][l][h]);
			}
		}
	}
}

//each edge can only has one color;
void set_constraint_group1(){
	for(auto &j:veccon){
		if(j->type()==1){
                	sum_constr++;
                	constr_sense_map[sum_constr]=0;
                	constr_rightside_map[sum_constr]=1;
			constr_variable_map[sum_constr].insert(color_map[0][j->origin()->id()][j->end()->id()]);
                	color_map[0][j->origin()->id()][j->end()->id()]->constr_involved.insert(sum_constr);
                	color_map[0][j->origin()->id()][j->end()->id()]->constr_coeff[sum_constr]=1;
			for(int i=0;i<=vecmaster.size();i++){
                        	constr_variable_map[sum_constr].insert(color_map[i][j->origin()->id()][j->end()->id()]);
                       		color_map[i][j->origin()->id()][j->end()->id()]->constr_involved.insert(sum_constr);
                        	color_map[i][j->origin()->id()][j->end()->id()]->constr_coeff[sum_constr]=1;
                }
        }
}
//each row and column must has different color
void set_constraint_group2(){
	for(int k=0;k<=vecmaster.size();k++){
		for(auto &i:vecmaster){
			sum_constr++;
			constr_sense_map[sum_constr]=-1;
			constr_rightside_map[sum_constr]=1;
			for(auto &j:vecslave){	
				constr_variable_map[sum_constr].insert(color_map[k][i->id()][j->id()]);
				color_map[k][i->id()][j->id()]->constr_involved.insert(sum_constr);
				color_map[k][i->id()][j->id()]->constr_coeff[sum_constr]=1;
			}
		}
	}

	for(int k=0;k<=vecnode.size();k++){
		for(auto &j:vecslave){
			sum_constr++;
			constr_sense_map[sum_constr]=-1;
			constr_rightside_map[sum_constr]=1;
			for(auto &i:vecmaster){	
				constr_variable_map[sum_constr].insert(color_map[k][i->id()][j->id()]);
				color_map[k][i->id()][j->id()]->constr_involved.insert(sum_constr);
				color_map[k][i->id()][j->id()]->constr_coeff[sum_constr]=1;
			}
		}
	}
}
map<int,map<int,map<int,map<int,shared_ptr<variable>>>>> share;//share_ihjl;j=i2,i=i1,l=j2,h=j1
void set_sharedvalue(){
        for(int i=1;i<=vecmaster.size();i++){
		for(int j=1;j<=vecmaster.size();j++){
			for(int h=1;h<=vecslave.size();h++){
				for(int l=1;l<=vecslave.size();l++){         
					share[i][h][j][l]=make_shared<variable>();
                   			share[i][h][j][l]->id=vvar.size();
                        		share[i][h][j][l]->type=0;
                        		share[i][h][j][l]->upbound=1;
                        		share[i][h][j][l]->lobound=0;
                        		vvar.push_back(share[i][h][j][l]);
				}
			}
		}
        }
}
///constraint 78910
void set_constraint_group3(){
	for(&p:veccon){
		for(&q:veccon){
			if(p->type()==1&&q->type()==1){
				i=p->origin()->id();
				j=q->origin()->id();
				h=p->end()->id();
				l=p->end()->id();
				shared_ptr<connectivity> newcon;
				if(i,l)->type==1 and (j,h)->type==1;
				then given following constraints
				
        for(int i=1;i<=vecmaster.size();i++){
                for(int j=1;j<=vecmaster.size();j++){
			for(int h=1;h<=vecslave.size();h++){
				for(int l=1;l<=vecslave.size();l++){
                        		sum_constr++;
                       			constr_sense_map[sum_constr]=1;
                       			constr_rightside_map[sum_constr]=0;	
                       			constr_variable_map[sum_constr].insert(color_map[0][i][l]);
                        		color_map[0][j][h]->constr_involved.insert(sum_constr);
                        		color_map[0][j][h]->constr_coeff[sum_constr]=1;
                        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
                        		share[i][h][j][l]->constr_involved.insert(sum_constr);
                        		share[i][h][j][l]->constr_coeff[sum_constr]=-1;
                		}
			}
		}
	}
        for(int i=1;i<=vecnode.size();i++){
                for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
                        		sum_constr++;
                        		constr_sense_map[sum_constr]=1;
                        		constr_rightside_map[sum_constr]=0;
                        		constr_variable_map[sum_constr].insert(color_map[0][j][h]);
                        		color_map[0][j][h]->constr_involved.insert(sum_constr);
                        		color_map[0][j][h]->constr_coeff[sum_constr]=1;
                        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
                        		share[i][h][j][l]->constr_involved.insert(sum_constr);
                        		share[i][h][j][l]->constr_coeff[sum_constr]=-1;
		                }       
			}
		}
        }
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
					sum_constr++;
					constr_sense_map[sum_constr]=-1;
					constr_rightside_map[sum_constr]=M;     
       		        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
       	        			share[i][h][j][l]->constr_involved.insert(sum_constr);
       		        		share[i][h][j][l]->constr_coeff[sum_constr]=M;
					for(int k=1;k<=vecnode.size();k++){
						constr_variable_map[sum_constr].insert(color_map[k][i][h]);
						color_map[k][i][h]->constr_involved.insert(sum_constr);
						color_map[k][i][h]->constr_coeff[sum_constr]=k;
                                		constr_variable_map[sum_constr].insert(color_map[k][j][l]);
                                		color_map[k][j][l]->constr_involved.insert(sum_constr);
                                		color_map[k][j][l]->constr_coeff[sum_constr]=-k;
					}
				}
			}
		}
	}	
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
					sum_constr++;
					constr_sense_map[sum_constr]=-1;
					constr_rightside_map[sum_constr]=M;     
       		        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
       	        			share[i][h][j][l]->constr_involved.insert(sum_constr);
       		        		share[i][h][j][l]->constr_coeff[sum_constr]=M;
					for(int k=1;k<=vecnode.size();k++){
						constr_variable_map[sum_constr].insert(color_map[k][j][l]);
						color_map[k][j][l]->constr_involved.insert(sum_constr);
						color_map[k][j][l]->constr_coeff[sum_constr]=k;
                                		constr_variable_map[sum_constr].insert(color_map[k][i][h]);
                                		color_map[k][i][h]->constr_involved.insert(sum_constr);
                                		color_map[k][i][h]->constr_coeff[sum_constr]=-k;
					}
				}
			}
		}
	}
}
//define binary variable for ADF
map<int,map<int,shared_ptr<variable>>> adf;
void set_adf(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			adf[i][j]=make_shared<variable>();
			adf[i][j]->id=vvar.size();
			adf[i][j]->type=0;
			adf[i][j]->upbound=1;
			adf[i][j]->lobound=0;
			vvar.push_back(adf[i][j]);
		}
	}
}

//constraint 11; i,j means ij h,l means int a,b
void set_constraint_group4(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			sum_constr++;
			constr_sense_map[sum_constr]=1;
			constr_rightside_map[sum_constr]=0;
			constr_variable_map[sum_constr].insert(adf[i][j]);
			adf[i][j]->constr_involved.insert(sum_constr);
			adf[i][j]->constr_coeff[sum_constr]=1;
			for(int k=1;k<=vecnode.size();k++){
                                constr_variable_map[sum_constr].insert(color_map[k][i][j]);
                                color_map[k][i][j]->constr_involved.insert(sum_constr);
                                color_map[k][i][j]->constr_coeff[sum_constr]=-1;
			}
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){		
                        		constr_variable_map[sum_constr].insert(share[i][j][h][l]);
                        		share[i][j][h][l]->constr_involved.insert(sum_constr);
                        		share[i][j][h][l]->constr_coeff[sum_constr]=1;
				}
			}
		}
	}
}
//constraint 12 13;i:i1 h:j1 j:i2 l:j2
void set_constraint_group5(){
	for(int i=1;i<=4;i++){
		for(int h=1;h<=4;h++){
			for(int j=1;j<=4;j++){
				for(int l=1;l<=4;l++){
					sum_constr++;
					constr_sense_map[sum_constr]=-1;
					constr_rightside_map[sum_constr]=2;
					constr_variable_map[sum_constr].insert(adf[i][h]);
					adf[i][h]->constr_involved.insert(sum_constr);
					adf[i][h]->constr_coeff[sum_constr]=1;
					constr_variable_map[sum_constr].insert(adf[j][l]);
					adf[j][l]->constr_involved.insert(sum_constr);
					adf[j][l]->constr_coeff[sum_constr]=1;
					constr_variable_map[sum_constr].insert(share[i][h][j][l]);
					share[i][h][j][l]->constr_involved.insert(sum_constr);
					share[i][h][j][l]->constr_coeff[sum_constr]=1;
				}
			}
		}
	}
	for(int i=1;i<=4;i++){
		for(int h=1;h<=4;h++){
			for(int j=1;j<=4;j++){
				for(int l=1;l<=4;l++){			
					sum_constr++;
					constr_sense_map[sum_constr]=1;
					constr_rightside_map[sum_constr]=0;
					constr_variable_map[sum_constr].insert(adf[i][h]);
					adf[i][h]->constr_involved.insert(sum_constr);
					adf[i][h]->constr_coeff[sum_constr]=1;
					constr_variable_map[sum_constr].insert(adf[j][l]);
					adf[j][l]->constr_involved.insert(sum_constr);
					adf[j][l]->constr_coeff[sum_constr]=1;
					constr_variable_map[sum_constr].insert(share[i][h][j][l]);
					share[i][h][j][l]->constr_involved.insert(sum_constr);
					share[i][h][j][l]->constr_coeff[sum_constr]=-1;
				}
			}
		}
	}
}
//define insertion loss 
map<int,map<int,shared_ptr<variable>>> Il;//insertionloss
void set_insertionloss(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
        		Il[i][j]=make_shared<variable>();
        		Il[i][j]->id=vvar.size();
        		Il[i][j]->type=2;
        		Il[i][j]->upbound=10000;
        		Il[i][j]->lobound=0;
        		vvar.push_back(Il[i][j]);
		}
	}
}

//define adf number in the path
map<int,map<int,shared_ptr<variable>>> numadf;//number of adf
void set_numofadf(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
        		numadf[i][j]=make_shared<variable>();
        		numadf[i][j]->id=vvar.size();
        		numadf[i][j]->type=2;
        		numadf[i][j]->upbound=10000;
        		numadf[i][j]->lobound=0;
        		vvar.push_back(numadf[i][j]);
		}
	}
}

//set constraint 14
void set_constraint_group6(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			sum_constr++;
			constr_sense_map[sum_constr]=0;
			constr_rightside_map[sum_constr]=droploss-adfloss;
			constr_variable_map[sum_constr].insert(Il[i][j]);
			Il[i][j]->constr_involved.insert(sum_constr);
			Il[i][j]->constr_coeff[sum_constr]=1;
			constr_variable_map[sum_constr].insert(numadf[i][j]);
			numadf[i][j]->constr_involved.insert(sum_constr);
			numadf[i][j]->constr_coeff[sum_constr]=-adfloss;
			constr_variable_map[sum_constr].insert(color_map[0][i][j]);
			color_map[0][i][j]->constr_involved.insert(sum_constr);
			color_map[0][i][j]->constr_coeff[sum_constr]=droploss-adfloss;
		}
	}
}
//default path constraint 15
void set_constraint_group7(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){	
        		sum_constr++;
        		constr_sense_map[sum_constr]=1;
        		constr_rightside_map[sum_constr]=-M;
       			constr_variable_map[sum_constr].insert(numadf[i][j]);
        		numadf[i][j]->constr_involved.insert(sum_constr);
			numadf[i][j]->constr_coeff[sum_constr]=1;
			constr_variable_map[sum_constr].insert(color_map[0][i][j]);
			color_map[0][i][j]->constr_involved.insert(sum_constr);
			color_map[0][i][j]->constr_coeff[sum_constr]=-M;
			for(int k=1;k<=vecnode.size();k++){
       				constr_variable_map[sum_constr].insert(adf[i][k]);
        			adf[i][k]->constr_involved.insert(sum_constr);
				adf[i][k]->constr_coeff[sum_constr]=-1;
			}
			for(int k=1;k<=vecnode.size();k++){
				constr_variable_map[sum_constr].insert(adf[k][j]);
				adf[k][j]->constr_involved.insert(sum_constr);
				adf[k][j]->constr_coeff[sum_constr]=-1;
			}
		}			
        }
}
//constraint 16 
void set_constraint_group8(){
        for(int i=1;i<=vecnode.size();i++){
                for(int j=1;j<=vecnode.size();j++){    
                        sum_constr++;
                        constr_sense_map[sum_constr]=1;
                        constr_rightside_map[sum_constr]=-M;
                        constr_variable_map[sum_constr].insert(numadf[i][j]);
                        numadf[i][j]->constr_involved.insert(sum_constr);
                        numadf[i][j]->constr_coeff[sum_constr]=1;
                        constr_variable_map[sum_constr].insert(adf[i][j]);
                        adf[i][j]->constr_involved.insert(sum_constr);
                        adf[i][j]->constr_coeff[sum_constr]=-M;
                        for(int k=1;k<=j;k++){
                                constr_variable_map[sum_constr].insert(adf[i][k]);
                                adf[i][k]->constr_involved.insert(sum_constr);
                                adf[i][k]->constr_coeff[sum_constr]=-1;
			}
			for(int k=1;k<i;k++){
                                constr_variable_map[sum_constr].insert(adf[k][j]);
                                adf[k][j]->constr_involved.insert(sum_constr);
                                adf[k][j]->constr_coeff[sum_constr]=-1;
                        }   
                }    
        }   
}
//question i+1 j+1 loop problem
void set_constraint_group9(){
        for(int i=1;i<=vecnode.size();i++){
                for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
                        		sum_constr++;
                        		constr_sense_map[sum_constr]=1;
                        		constr_rightside_map[sum_constr]=-M;
                    		    	constr_variable_map[sum_constr].insert(numadf[i][h]);
                        		numadf[i][h]->constr_involved.insert(sum_constr);
                        		numadf[i][h]->constr_coeff[sum_constr]=1;
                        		constr_variable_map[sum_constr].insert(adf[i][h]);
                        		adf[i][h]->constr_involved.insert(sum_constr);
                        		adf[i][h]->constr_coeff[sum_constr]=M;
                        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
                        		share[i][h][j][l]->constr_involved.insert(sum_constr);
                        		share[i][h][j][l]->constr_coeff[sum_constr]=-M;
                        		for(int k=1;k<=vecnode.size();k++){
                                		constr_variable_map[sum_constr].insert(adf[i][k]);
                                		adf[i][k]->constr_involved.insert(sum_constr);
                                		adf[i][k]->constr_coeff[sum_constr]=-1;
                        		}
                        		for(int k=h;k<=vecnode.size();k++){
                               			constr_variable_map[sum_constr].insert(adf[k][l]);
                                		adf[k][l]->constr_involved.insert(sum_constr);
                                		adf[k][l]->constr_coeff[sum_constr]=-1;
                        		}
                        		for(int k=l+1;k<=vecnode.size();k++){
                                		constr_variable_map[sum_constr].insert(adf[k][j]);
                                		adf[k][j]->constr_involved.insert(sum_constr);
                                		adf[k][j]->constr_coeff[sum_constr]=-1;
                        		}
                        		for(int k=1;k<=vecnode.size();k++){
                                		constr_variable_map[sum_constr].insert(adf[k][h]);
                                		adf[k][h]->constr_involved.insert(sum_constr);
                                		adf[k][h]->constr_coeff[sum_constr]=-1;
                        		}
				}
			}
                }
        }
}
void set_loop(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			loop[i][j]=make_shared<variable>();
			loop[i][j]->id=vvar.size();
			loop[i][j]->type=0;
			loop[i][j]->upbound=1;
			loop[i][j]->lobound=0;
			vvar.push_back(loop[i][j]);
		}
	}
}

map<int,map<int,shared_ptr<variable>>> banzou;
void set_banzou(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			banzou[i][j]=make_shared<variable>();
			banzou[i][j]->id=vvar.size();
			banzou[i][j]->type=0;
			banzou[i][j]->upbound=1;
			banzou[i][j]->lobound=0;
			vvar.push_back(banzou[i][j]);
		}
	}
}
//const 181920
void set_constraint_group13(){
      	 for(int i=1;i<=vecnode.size();i++){
                for(int j=1;j<=vecnode.size();j++){    
                        sum_constr++;
                        constr_sense_map[sum_constr]=-1;
                        constr_rightside_map[sum_constr]=0;
                        constr_variable_map[sum_constr].insert(loop[i][j]);
                        loop[i][j]->constr_involved.insert(sum_constr);
                        loop[i][j]->constr_coeff[sum_constr]=1;
                        constr_variable_map[sum_constr].insert(color_map[0][i][j]);
                        color_map[0][i][j]->constr_involved.insert(sum_constr);
                        color_map[0][i][j]->constr_coeff[sum_constr]=-1;
		}
	}
        for(int i=1;i<=vecnode.size();i++){
                for(int j=1;j<=vecnode.size();j++){    
                        sum_constr++;
                        constr_sense_map[sum_constr]=1;
                        constr_rightside_map[sum_constr]=-M;
                        constr_variable_map[sum_constr].insert(loop[i][j]);
                        loop[i][j]->constr_involved.insert(sum_constr);
                        loop[i][j]->constr_coeff[sum_constr]=-M;
			for(int k=j+1;k<=vecnode.size();k++){
                        	constr_variable_map[sum_constr].insert(adf[i][k]);
                        	adf[i][k]->constr_involved.insert(sum_constr);
                        	adf[i][k]->constr_coeff[sum_constr]=-1;
			}
			for(int k=i+1;k<=vecnode.size();k++){
                        	constr_variable_map[sum_constr].insert(adf[k][j]);
                        	adf[k][j]->constr_involved.insert(sum_constr);
                        	adf[k][j]->constr_coeff[sum_constr]=-1;
			}
		}
	}
	sum_constr++;
	constr_sense_map[sum_constr]=-1;
        constr_rightside_map[sum_constr]=0;
        constr_variable_map[sum_constr].insert(banzou[1][1]);
        banzou[1][1]->constr_involved.insert(sum_constr);
        banzou[1][1]->constr_coeff[sum_constr]=1;
	sum_constr++;
	constr_sense_map[sum_constr]=-1;
        constr_rightside_map[sum_constr]=0;
        constr_variable_map[sum_constr].insert(banzou[1][2]);
        banzou[1][2]->constr_involved.insert(sum_constr);
        banzou[1][2]->constr_coeff[sum_constr]=1;
	constr_variable_map[sum_constr].insert(loop[1][1]);
        loop[1][1]->constr_involved.insert(sum_constr);
        loop[1][1]->constr_coeff[sum_constr]=-1;
	sum_constr++;
	constr_sense_map[sum_constr]=-1;
        constr_rightside_map[sum_constr]=0;
        constr_variable_map[sum_constr].insert(banzou[2][1]);
        banzou[2][1]->constr_involved.insert(sum_constr);
        banzou[2][1]->constr_coeff[sum_constr]=1;
	constr_variable_map[sum_constr].insert(loop[1][1]);
        loop[1][1]->constr_involved.insert(sum_constr);
        loop[1][1]->constr_coeff[sum_constr]=-1;
        for(int i=2;i<=vecnode.size();i++){
                for(int j=2;j<=vecnode.size();j++){    
                        sum_constr++;
                        constr_sense_map[sum_constr]=-1;
                        constr_rightside_map[sum_constr]=0;
                        constr_variable_map[sum_constr].insert(banzou[i][j]);
                        banzou[i][j]->constr_involved.insert(sum_constr);
                        banzou[i][j]->constr_coeff[sum_constr]=1;
			for(int k=1;k<=j-1;k++){
                        	constr_variable_map[sum_constr].insert(loop[i][k]);
                        	loop[i][k]->constr_involved.insert(sum_constr);
                        	loop[i][k]->constr_coeff[sum_constr]=-1;
			}
			for(int k=1;k<=i-1;k++){
                        	constr_variable_map[sum_constr].insert(loop[k][j]);
                        	loop[k][j]->constr_involved.insert(sum_constr);
                        	loop[k][j]->constr_coeff[sum_constr]=-1;
			}
		}
	}
}

//define total number of adfs
shared_ptr<variable> totaladf;
shared_ptr<variable> totallambda;
shared_ptr<variable> totalloss;
shared_ptr<variable> totalremove;
void set_totaladf(){
        totaladf=make_shared<variable>();
        totaladf->id=vvar.size();
        totaladf->type=1;
        totaladf->upbound=10000;
        totaladf->lobound=0;
        vvar.push_back(totaladf);
}
void set_totalwavelength(){
        totallambda=make_shared<variable>();
        totallambda->id=vvar.size();
        totallambda->type=1;
        totallambda->upbound=10000;
        totallambda->lobound=0;
        vvar.push_back(totallambda);
}
void set_totalloss(){
        totalloss=make_shared<variable>();
        totalloss->id=vvar.size();
        totalloss->type=2;
        totalloss->upbound=10000;
        totalloss->lobound=0;
        vvar.push_back(totalloss);
}
void set_totalremove(){
        totalremove=make_shared<variable>();
        totalremove->id=vvar.size();
        totalremove->type=1;
        totalremove->upbound=10000;
        totalremove->lobound=0;
        vvar.push_back(totalremove);
}
//constraint 21
void set_constraint_group10(){
	sum_constr++;
        constr_sense_map[sum_constr]=0;
        constr_rightside_map[sum_constr]=0;
        constr_variable_map[sum_constr].insert(totaladf);
        totaladf->constr_involved.insert(sum_constr);
        totaladf->constr_coeff[sum_constr]=1;
	for(auto &i:vecnode){
		for(auto &j:vecnode){
        		constr_variable_map[sum_constr].insert(adf[i->id()][j->id()]);
                        adf[i->id()][j->id()]->constr_involved.insert(sum_constr);
                        adf[i->id()][j->id()]->constr_coeff[sum_constr]=-1;
		}
	}
}
//constraint 22
void set_constraint_group11(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			sum_constr++;
        		constr_sense_map[sum_constr]=1;
        		constr_rightside_map[sum_constr]=0;
        		constr_variable_map[sum_constr].insert(totallambda);
        		totallambda->constr_involved.insert(sum_constr);
        		totallambda->constr_coeff[sum_constr]=1;
			for(int k=1;k<=vecnode.size();k++){
        			constr_variable_map[sum_constr].insert(color_map[k][i][j]);
                        	color_map[k][i][j]->constr_involved.insert(sum_constr);
                        	color_map[k][i][j]->constr_coeff[sum_constr]=-k;
			}
		}
	}
}

void set_constraint_group12(){
	for(int i=1;i<vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			sum_constr++;
			constr_sense_map[sum_constr]=1;
			constr_rightside_map[sum_constr]=0;
			constr_variable_map[sum_constr].insert(totalloss);
			totalloss->constr_involved.insert(sum_constr);
			totalloss->constr_coeff[sum_constr]=1;
			constr_variable_map[sum_constr].insert(Il[i][j]);
			Il[i][j]->constr_involved.insert(sum_constr);
			Il[i][j]->constr_coeff[sum_constr]=-1;
		}
	}
	sum_constr++;
        constr_sense_map[sum_constr]=0;
        constr_rightside_map[sum_constr]=0;
        constr_variable_map[sum_constr].insert(totalremove);
        totalremove->constr_involved.insert(sum_constr);
        totalremove->constr_coeff[sum_constr]=1;
	for(auto &i:vecnode){
		for(auto &j:vecnode){
        		constr_variable_map[sum_constr].insert(banzou[i->id()][j->id()]);
                        banzou[i->id()][j->id()]->constr_involved.insert(sum_constr);
                        banzou[i->id()][j->id()]->constr_coeff[sum_constr]=-1;
		}
	}
	sum_constr++;
        constr_variable_map[sum_constr].insert(totallambda);
        totallambda->constr_involved.insert(sum_constr);
        totallambda->constr_coeff[sum_constr]=10;
        constr_variable_map[sum_constr].insert(totaladf);
        totaladf->constr_involved.insert(sum_constr);
        totaladf->constr_coeff[sum_constr]=10;
        constr_variable_map[sum_constr].insert(totalloss);
        totalloss->constr_involved.insert(sum_constr);
        totalloss->constr_coeff[sum_constr]=100;
        constr_variable_map[sum_constr].insert(totalremove);
        totalremove->constr_involved.insert(sum_constr);
        totalremove->constr_coeff[sum_constr]=-1;
}
void funcGurobi(double time, double absgap, int idisplay)
{
        GRBEnv env = GRBEnv();
        GRBModel model = GRBModel(env);
        model.getEnv().set(GRB_DoubleParam_TimeLimit, time); // case 900
//      model.getEnv().set(GRB_DoubleParam_MIPGapAbs, 0); // case 0.02
//      model.getEnv().set(GRB_DoubleParam_Heuristics, 1); // case none
//      model.getEnv().set(GRB_DoubleParam_ImproveStartGap, 0.02); // case 0.02
        model.getEnv().set(GRB_IntParam_OutputFlag, idisplay);
        map<shared_ptr<variable>,string> mapvs;
        for(int i=0;i<vvar.size();i++)
        {
                ostringstream convi;
                convi<<i;
                mapvs[vvar[i]]="x"+convi.str();
        }
        GRBVar *x=new GRBVar [vvar.size()+1];
        for(int i=0;i<vvar.size();i++)
        {
                if(vvar[i]->type==0)
                x[i]=model.addVar(vvar[i]->lobound,vvar[i]->upbound,0.0,GRB_BINARY,mapvs[vvar[i]]);
                else if(vvar[i]->type==1)
                x[i]=model.addVar(vvar[i]->lobound,vvar[i]->upbound,0.0,GRB_INTEGER,mapvs[vvar[i]]);
                else if(vvar[i]->type==2)
                x[i]=model.addVar(vvar[i]->lobound,vvar[i]->upbound,0.0,GRB_CONTINUOUS,mapvs[vvar[i]]);
        }
        model.update();
        for(int i=1;i<=sum_constr;i++)
        if(constr_variable_map[i].size()!=0) // cons with 0 var is eliminated
        {
                ostringstream convi;
                convi<<i;
                GRBLinExpr expr;

                if(i!=objcons)
                {
                        for(auto setit:constr_variable_map[i])
                        {
                                expr+=x[setit->id]*(setit->constr_coeff[i]);
                                //expr+=x[(*setit)->index]*((*setit)->constr_coeff[i]);
                        }
                        string name='c'+convi.str();
                        if(constr_sense_map[i]==1)
                                model.addConstr(expr,GRB_GREATER_EQUAL,constr_rightside_map[i],name);
                        else if(constr_sense_map[i]==-1)
                                model.addConstr(expr,GRB_LESS_EQUAL,constr_rightside_map[i],name);
                        else if(constr_sense_map[i]==0)
                                model.addConstr(expr,GRB_EQUAL,constr_rightside_map[i],name);
                }
                else
                {
                        for(auto setit:constr_variable_map[i])
			{
//                        for(set<variable*>::iterator setit=constr_variable_map[i].begin();setit!=constr_variable_map[i].end();setit++)
                                expr+=x[setit->id]*(setit->constr_coeff[i]);
                //      model.setObjective(expr,GRB_MAXIMIZE);
                                model.setObjective(expr,GRB_MINIMIZE);
                	}
        	}
	}
//      mycallback cb(time, absgap);
//      model.setCallback(&cb);
        model.optimize();

        while(model.get(GRB_IntAttr_SolCount)==0)
        {
                time+=5;
                model.getEnv().set(GRB_DoubleParam_TimeLimit, time); // case 900
                model.optimize();
        }
        for(int i=0;i<vvar.size();i++)
        {
                if(vvar[i]->type==0||vvar[i]->type==1)
                {
                        if(x[i].get(GRB_DoubleAttr_X)-(int)x[i].get(GRB_DoubleAttr_X)<0.5)
                                vvar[i]->int_value=(int)x[i].get(GRB_DoubleAttr_X);
                        else
                                vvar[i]->int_value=(int)x[i].get(GRB_DoubleAttr_X)+1;
                }
                else if(vvar[i]->type==2)
                        vvar[i]->double_value=x[i].get(GRB_DoubleAttr_X);
                else
                        cout<<"new type"<<endl;
        }
}

int main(int argc, char * argv[]){
	readfile("input.txt");
	set_color_map();
	set_sharedvalue();
	set_adf();
	set_numofadf();
	set_insertionloss();
	set_totalwavelength();
	set_totaladf();
	set_totalloss();
	set_banzou();
	set_totalremove();
	set_loop();
	set_constraint_group1();
	set_constraint_group2();
	set_constraint_group3();
	set_constraint_group4();
//	set_constraint_group5();
	set_constraint_group6();
	set_constraint_group7();
	set_constraint_group8();
	set_constraint_group9();
	set_constraint_group13();
	set_constraint_group10();	
	set_constraint_group11();
	set_constraint_group12();
	objcons=sum_constr;
	funcGurobi(30,0,1);
	cout<<adfloss<<" "<<droploss<<" "<<endl;
	cout<<totaladf->int_value<<" "<<totallambda->int_value<<"  "<<totalloss->double_value<<" "<<totalremove->int_value<<" "<<endl;
//	cout<<totaladf->int_value+totallambda->int_value+totalloss->int_value<<"  "<<endl;
/*	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			cout<<"("<<i<<","<<j<<")";
			cout<<numadf[i][j]->double_value<<" "<<endl;
		}
	}
//			cout<<i;
			cout<<adf[i][j]->int_value<<" "<<endl;
			for(int k=0;k<=vecnode.size();k++){
				cout<<"("<<k<<",";
				cout<<i<<","<<j<<")";
				cout<<color_map[k][i][j]->int_value<<" "<<endl;
			}
		}
	}*/
/*			cout<<"{";
		for(auto &j:vecedge)
		{		
			cout<<"("<<j->origin()->id()<<","<<j->end()->id()<<")"<<color_map[i][j->origin()->id()][j->end()->id()]->int_value<<", ";

		}
	cout<<"}"<<endl;
	}*/
}	

