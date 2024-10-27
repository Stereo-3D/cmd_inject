//this source is just for demonsration purpose on how I extract hash data from "Overwatch.fxh"
#include<stdio.h>
#include<stdlib.h>
typedef unsigned u;
int cmp(const void*a,const void*b)
{
	if(*(u*)a>*(u*)b)return 1;
	if(*(u*)a<*(u*)b)return-1;
	return 0;
}
int val[128],hash_len;
u hashes[65536];
int main()
{
	FILE*f;int i,j;u k;
	for(i=0,j='0';i<10;)val[j++]=i++;
	for(j='A';i<16;++j)val[j]=val[j^32]=i++;
	f=fopen("/media/tjandra/DATA/share/cmd_inject/Depth3D/Shaders/Overwatch.fxh","r");
	if(f==NULL){printf("Cannot open Overwatch.fxh!\n");return 0;}
	for(j='?';(i=fgetc(f))!=EOF;j=i)
	{
		if(i!='A')continue;
		if(j=='s')continue;//discard sApp variable
		if(fgetc(f)!='p')continue;
		if(fgetc(f)!='p')continue;
		while((i=fgetc(f))<=' ');
		if(i!='=')continue;
		if(fgetc(f)!='=')continue;
		while((i=fgetc(f))<=' ');
		if(i!='0')continue;
		if(fgetc(f)!='x')continue;
		for(k=0;(i=fgetc(f))>='0';)k=k<<4|val[i];
		hashes[hash_len++]=k;
	}
	fclose(f);
	printf("hash_len=%u\n",--hash_len);//discard last 0x77777777 hash :v
	qsort(hashes,hash_len,sizeof(u),cmp);
	putchar('{');
	for(i=-1;++i<hash_len;)printf("%s0x%xu%c",i&7?"":"\n",hashes[i],i<hash_len-1?',':'}');
	putchar('\n');
	return 0;
}
