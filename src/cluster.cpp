#include<iostream>
#include<cstdio>
#include<vector>
#include<utility>
#include<string>
#include<cstring>
#include<dirent.h>
#include<cstdlib>
#include<ctype.h>
#include<map>
#include<cmath>

using namespace std;

#define MAX_PATH_LEN 1024
#define CHUNK_SZ 100000000
#define MAX_WORD_SZ 100

class Similarity
{
	public:
		char* unigram_dpath;	
		int bufp;
		int buflen;
		char* buf;
		int num_files;
		vector<vector<double> > sim;
		map<string,int> df;		

		Similarity(char* unigram_dp,char* df_path);
		void FindSim();
		bool Read(FILE* fp);
		bool BufpInc(FILE* fp);
		void LoadFile(char* fpath,vector<pair<string, double> >& wt);
		void LoadDf(char* fpath);
		double FindDocSim(vector<pair<string, double> >& wt1,vector<pair<string, double> >& wt2);
		void Print();
		~Similarity();
};

Similarity::Similarity(char* unigram_dp,char* df_path) : unigram_dpath(unigram_dp),bufp(0),buflen(0)
{
	buf = new char[CHUNK_SZ+4];
	char cmd[MAX_PATH_LEN];
	sprintf(cmd, "ls %s | wc -l", unigram_dpath);
	FILE* pipe = popen(cmd, "r");
	fscanf(pipe,"%d",&num_files);
	pclose(pipe);
	sim = vector< vector<double> >(num_files+2, vector<double> (num_files + 2));
	LoadDf(df_path);
}


Similarity::~Similarity()
{
	delete[] buf;
}

void Similarity::FindSim()
{
	DIR* dir = opendir(unigram_dpath);

	if(dir == NULL)
	{
		puts("Error:can't open directory");
		exit(-1);
	}

	struct dirent* file;
	char fpath[MAX_PATH_LEN];
	int unigram_dpath_len = strlen(unigram_dpath);
	int f1,f2;

	strcpy(fpath, unigram_dpath);
	fpath[unigram_dpath_len] = '/';

	while((file = readdir(dir)) != NULL)
	{
		if(file->d_name[0] == '.')
			continue;

		strcpy(fpath + unigram_dpath_len + 1, file->d_name);
		vector<pair<string, double> > wt;
		LoadFile(fpath,wt);
		f1=atoi(file->d_name);

		DIR* dir2 = opendir(unigram_dpath);
		struct dirent* file2;
		char fpath2[MAX_PATH_LEN];
		int unigram_dpath_len2 = strlen(unigram_dpath);
		
		strcpy(fpath2, unigram_dpath);
		fpath2[unigram_dpath_len2] = '/';

		while((file2 = readdir(dir2)) != NULL)
		{
			if(file2->d_name[0] == '.')
				continue;

			strcpy(fpath2 + unigram_dpath_len2 + 1, file2->d_name);
			f2=atoi(file2->d_name);
			vector<pair<string, double> > wt2;
			LoadFile(fpath2,wt2);
			sim[f2][f1]=sim[f1][f2]=FindDocSim(wt,wt2);	
		}
		closedir(dir2);
	}
	closedir(dir);
}

double Similarity::FindDocSim(vector<pair<string, double> >& wt1,vector<pair<string, double> >& wt2)
{
	int i=0;
	int j=0;
	double nume=0;
	double norm1=0;
	double norm2=0;
	double ret;
	
	while(i<wt1.size() && j<wt2.size())
	{
		if(wt1[i].first<wt2[j].first)
			i++;
		else if(wt1[i].first>wt2[j].first)
			j++;
		else
		{
			nume+=wt1[i].second*wt2[j].second;
			norm1+=wt1[i].second*wt1[i].second;
			norm2+=wt2[j].second*wt2[j].second;
			i++;
			j++;
		}
	}
	norm1=sqrt(norm1);
	norm2=sqrt(norm2);
	ret=nume/(norm1*norm2);
	return ret;
}

inline bool Similarity::Read(FILE* fp)
{
	bufp = 0;
	buflen = fread(buf, 1, CHUNK_SZ, fp);
	return (buflen != 0);
}

inline bool Similarity::BufpInc(FILE* fp)
{
	++bufp;
	if(bufp < buflen)
		return true;
	return Read(fp);
}


void Similarity::LoadFile(char* fpath,vector<pair<string, double> >& wt)
{
	FILE* fp = fopen(fpath,"r");
	char word[MAX_WORD_SZ];
	int word_len=0;
	int frequency=0;
	double weight=0;	

	bufp=buflen=0;	
	while(BufpInc(fp))
	{
		word_len=0;
		weight=0;
		
		while(isalnum(buf[bufp]))
		{
			word[word_len++]=buf[bufp];
			BufpInc(fp);
		}
		word[word_len]='\0';		
		BufpInc(fp);
		weight=log2((0.5+num_files)/df[string(word)])/(log2(1.0+num_files));	
		frequency=0;
		while(isdigit(buf[bufp]))
		{
			frequency=frequency*10+buf[bufp]-'0';
        		BufpInc(fp);	
		}
		weight*=frequency;
		
		wt.push_back(make_pair(string(word),weight));
	}
	fclose(fp);
}

void Similarity::LoadDf(char* fpath)
{
	
	FILE* fp = fopen(fpath,"r");
	char word[MAX_WORD_SZ];
	int word_len=0;
	int frequency=0;
	
	bufp=buflen=0;

	while(BufpInc(fp))
	{
		word_len=0;
		while(isalnum(buf[bufp]))
		{
			word[word_len++]=buf[bufp];
			BufpInc(fp);
		}
		word[word_len]='\0';		
		BufpInc(fp);
		
		frequency=0;
		while(isdigit(buf[bufp]))
		{
			frequency=frequency*10+buf[bufp]-'0';
			BufpInc(fp);
		}
		df[string(word)]=frequency;
	}
	fclose(fp);
}

void Similarity::Print()
{
	for(int i=1;i<sim.size()-1;i++)
	{
		for(int j=1;j<sim[i].size()-1;j++)
		{	
			cout<<sim[i][j]<<" ";
		}
		cout<<"\n";
	}
}

int main(int argc,char* argv[])
{
	if(argc!=3)
	{
		puts("Usage: ./a.out <unigram_dir> <df path>");
		return -1;
	}
	Similarity s(argv[1],argv[2]);
	s.FindSim();
	s.Print();
	return 0;
}
