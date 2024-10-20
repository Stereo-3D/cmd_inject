#include<stdio.h>
#include<stdlib.h>
#define CMD_INJECT_VERSION "v0.2.1"
typedef struct Dynamic_String{char *data;int length,alloc;}dstr;
int is_both_string_equal(char*a,char*b)//with this, no need to include string.h anymore!
{
	for(;*a!='\0'&&*b!='\0';)if(*a++!=*b++)return 0;
	return *a==*b;
}
dstr log_str;FILE *log_file;char *argv0;
void append_char_to_dstr(dstr *str,char c,int escape_special)
{
	if(str==&log_str&&log_file==NULL)
	{
		//because log file is an emergency it need to be prioritized! (have own safe code)
		int i,j;char *log_file_path;for(i=-1;argv0[++i];);
		log_file_path=(char*)calloc(i+128,sizeof(char));
		while(i--)if(argv0[i]=='\\'||argv0[i]=='/')break;
		for(j=i+1,i=-1;++i<j;)log_file_path[i]=argv0[i];
		sprintf(log_file_path+i,"cmd_inject.log");
		log_file=fopen(log_file_path,"w");
		if(log_file==NULL)
		{
			printf("[ERROR] cannot write to log file: \"%s\"! ",log_file_path);
			printf("Make sure you have write permission on that folder and make ");
			printf("sure you have enough disk space to write the file to the disk!\n");
			fflush(stdout);
		}
		free(log_file_path);log_file_path=NULL;
	}
	if(str->length==str->alloc)
		str->data=(char*)realloc(str->data,(str->alloc=str->alloc<<1|1)*sizeof(char));
	if((c=='\"'||c=='\\')&&escape_special)
	{
		str->data[str->length++]='\\';
		if(str==&log_str)fputc('\\',log_file);
		append_char_to_dstr(str,c,0);
		return;
	}
	str->data[str->length++]=c;
	if(str==&log_str)
	{
		if(c!='\0')fputc(c,log_file);
		if(c=='\n')fflush(log_file);//force to write the log data to disk (clear cache)
		/* the above operation is slow (especially on non ssd drive)
		 * but this is necessary to capture all moments while the program alive
		 * to ensure the log data is saved to disk properly in case of program crash at random
		 */
		if(c=='\0'){fclose(log_file);log_file=NULL;}
	}
	return;
}
void append_string_to_dstr(dstr*str,char *string,char end_symbol,int write_quotes)
{
	if(write_quotes)append_char_to_dstr(str,'\"',0);
	while(*string)append_char_to_dstr(str,*string++,write_quotes);
	if(write_quotes)append_char_to_dstr(str,'\"',0);
	append_char_to_dstr(str,end_symbol,0);
	return;
}
int check_for_wine_or_proton_keyword(dstr*tmp,int left_index,int right_index)
{
	char backup_char,left_backup,right_backup;
	if(tmp->data[left_index]==tmp->data[right_index]&&
		(tmp->data[left_index]=='\''||tmp->data[left_index]=='\"'))
			left_backup=right_backup=tmp->data[left_index];
	else if(right_index-left_index>2&&tmp->data[left_index]=='\\'&&
		tmp->data[right_index]=='\"'&&tmp->data[left_index+1]=='\"'&&
		tmp->data[right_index-1]=='\\')
	{
		left_backup=tmp->data[++left_index];
		right_backup=tmp->data[--right_index];
	}
	else
	{
		left_backup=tmp->data[--left_index];
		right_backup=tmp->data[++right_index];
	}
	tmp->data[left_index]=tmp->data[right_index]='\"';
	backup_char=tmp->data[right_index+1];
	tmp->data[right_index+1]='\0';
	tmp->data+=left_index;
	int satisfy=0,i,ret_code=0;
	if(is_both_string_equal(tmp->data,"\"waitforexitandrun\""))
	{
		ret_code=1;
		goto restore_data_and_return;
	}
	for(i=right_index-left_index+1;i--;)if(tmp->data[i]=='\\'||tmp->data[i]=='/')break;
	if(i<0)i=0;
	if(tmp->data[++i]=='w'&&tmp->data[i+1]=='i'&&tmp->data[i+2]=='n'&&tmp->data[i+3]=='e')
		{i+=4;satisfy=1;}
	else if(tmp->data[i]=='p'&&tmp->data[i+1]=='r'&&tmp->data[i+2]=='o'&&
		tmp->data[i+3]=='t'&&tmp->data[i+4]=='o'&&tmp->data[i+5]=='n'){i+=6;satisfy=1;}
	if(!satisfy)
	{
		//so mach types of linux shell!!!
		if((tmp->data[i]=='b'&&tmp->data[i+1]=='a'&&
			tmp->data[i+2]=='s'&&tmp->data[i+3]=='h')||
			(tmp->data[i]=='f'&&tmp->data[i+1]=='i'&&
			tmp->data[i+2]=='s'&&tmp->data[i+3]=='h')||
			(tmp->data[i]=='t'&&tmp->data[i+1]=='c'&&
			tmp->data[i+2]=='s'&&tmp->data[i+3]=='h')||
			(tmp->data[i]=='d'&&tmp->data[i+1]=='a'&&
			tmp->data[i+2]=='s'&&tmp->data[i+3]=='h'))
				{i+=4;satisfy=1;}
		else if((tmp->data[i]=='c'||tmp->data[i]=='k'||tmp->data[i]=='z')&&
			tmp->data[i+1]=='s'&&tmp->data[i+2]=='h'){i+=3;satisfy=1;}
		else if(tmp->data[i]=='s'&&tmp->data[i+1]=='h'){i+=2;satisfy=1;}
		if(satisfy&&tmp->data[i]=='\"')ret_code=4;
		goto restore_data_and_return;
	}
	if((tmp->data[i]=='3'&&tmp->data[i+1]=='2')||
		(tmp->data[i]=='6'&&tmp->data[i+1]=='4'))i+=2;
	if(tmp->data[i]=='.'&&tmp->data[i+1]=='e'&&
		tmp->data[i+2]=='x'&&tmp->data[i+3]=='e')i+=4;
	if(tmp->data[i]=='\"')ret_code=2;
	restore_data_and_return:;
	tmp->data-=left_index;
	tmp->data[left_index]=left_backup;
	tmp->data[right_index]=right_backup;
	tmp->data[right_index+1]=backup_char;
	return ret_code;
}
dstr launch_command,extra_command;int arg_index,arg_index_inside_quotes;
void convert_critical_argument_to_windows_format()
{
	append_string_to_dstr(&log_str,"\n===[Argument Converter]===",'\n',0);
	int i=0,converting=1,escape=0;char c=launch_command.data[arg_index],terminate_symbol=' ';
	if(c=='\"'||c=='\''){terminate_symbol=c;i=1;}
	else if(arg_index_inside_quotes&&c=='\\'&&
		launch_command.data[arg_index+1]=='\"'){terminate_symbol='\"';i=2;}
	if(launch_command.data[arg_index+i]!='.'&&launch_command.data[arg_index+i]!='/')
	{
		append_string_to_dstr(&log_str,"No need to convert game argument!",'\n',0);
		converting=0;
	}
	else append_string_to_dstr(&log_str,"Converting game argument...",'\n',0);
	dstr game_arg;
	game_arg.data=NULL;
	game_arg.length=game_arg.alloc=0;
	append_char_to_dstr(&game_arg,'\"',0);
	if(converting)
	{
		if(launch_command.data[arg_index+i]=='/')append_string_to_dstr(&game_arg,"Z",':',0);
		if(launch_command.data[arg_index+i]=='.'&&
			launch_command.data[arg_index+i+1]=='/')i+=2;
	}
	while((c=launch_command.data[arg_index+i++])!=terminate_symbol)
	{
		if(arg_index_inside_quotes&&!(escape&1)&&c=='\"')break;
		if(c=='/'&&converting)append_char_to_dstr(&game_arg,'\\',1);
		else append_char_to_dstr(&game_arg,c,0);
		if(c=='\\')++escape;
		else escape=0;
	}
	if(terminate_symbol==' ')--i;
	if(arg_index_inside_quotes&&(escape&1)&&c=='\"')game_arg.data[game_arg.length-1]=c;
	else append_char_to_dstr(&game_arg,'\"',0);
	if(converting)
	{
		append_char_to_dstr(&game_arg,'\0',0);
		append_string_to_dstr(&log_str,"New game argument:",' ',0);
		append_string_to_dstr(&log_str,game_arg.data,'\n',0);
		--game_arg.length;
	}
	for(;(c=launch_command.data[arg_index+i++])!='\0';)
	{
		if(arg_index_inside_quotes&&c=='\"')
		{
			if(escape&1)game_arg.data[game_arg.length-1]=c;
			else break;
		}
		else if(arg_index_inside_quotes&&c=='\'')
			append_char_to_dstr(&game_arg,'\"',0);
		else append_char_to_dstr(&game_arg,c,0);
		if(c=='\\')++escape;
		else escape=0;
	}
	for(;c!='\0';c=launch_command.data[arg_index+i++])append_char_to_dstr(&extra_command,c,0);
	if(extra_command.data)append_char_to_dstr(&extra_command,'\0',0);
	append_char_to_dstr(&game_arg,'\0',0);
	launch_command.length=arg_index;
	append_string_to_dstr(&launch_command,game_arg.data,'\0',0);
	if(game_arg.data)free(game_arg.data);
	append_string_to_dstr(&log_str,"Done converting game argument!",'\n',0);
	return;
}
dstr *argvx;char buffer[128];
int steam_index,argix,arglx,nested_shell_detected;
void append_argument(char*value)
{
	dstr *tmp;int index,i,left=launch_command.length,right,end,write_arg_index=0;char c;
	sprintf(buffer,"Argument[%d]:",argix);
	append_string_to_dstr(&log_str,buffer,' ',0);
	if(arglx==argix)argvx=(dstr*)realloc(argvx,(arglx=arglx<<1|1)*sizeof(dstr));
	tmp=&argvx[index=argix++];
	tmp->data=NULL;
	tmp->length=tmp->alloc=0;
	append_string_to_dstr(tmp,value,'\0',1);
	append_string_to_dstr(&log_str,value,'\n',1);
	if(steam_index)
	{
		append_string_to_dstr(&launch_command,value,' ',1);
		if(!nested_shell_detected)
		{
			i=check_for_wine_or_proton_keyword(tmp,0,tmp->length-2);
			if(i&1)
			{
				arg_index=launch_command.length;
				append_string_to_dstr(&log_str,"\n===[Game Argument]===",'\n',0);
			}
			if(i&2)
			{
				arg_index=launch_command.length;
				append_string_to_dstr(&log_str,"^wine or proton arg above^",'\n',0);
			}
			if(i&4)
			{
				nested_shell_detected=1;
				append_string_to_dstr(&log_str,"^bash shell fork above^",'\n',0);
			}
			return;
		}
		for(end=launch_command.length-2;left<end;)
		{
			if(launch_command.data[++left]<=' ')continue;
			if(write_arg_index){arg_index=left;write_arg_index=0;arg_index_inside_quotes=1;}
			if(launch_command.data[left]=='\\'&&launch_command.data[left+1]=='\"')
				{c='\"';right=left+1;}
			else if(launch_command.data[left]=='\''){c='\'';right=left;}
			else{c=' ';right=left;}
			while(launch_command.data[++right]!=c)if(right>=end){right=end-1;break;}
			if(launch_command.data[right]==' '&&c==' ')--right;
			append_string_to_dstr(&log_str,"--> sub argument:",' ',0);
			c=launch_command.data[right+1];
			launch_command.data[right+1]='\0';
			append_string_to_dstr(&log_str,launch_command.data+left,'\n',0);
			launch_command.data[right+1]=c;
			i=check_for_wine_or_proton_keyword(&launch_command,left,right);
			if(i&1)
			{
				write_arg_index=1;
				append_string_to_dstr(&log_str,"^proton magic keyword above^",'\n',0);
			}
			if(i&2)
			{
				write_arg_index=1;
				append_string_to_dstr(&log_str,"^wine or proton arg above^",'\n',0);
			}
			if(i&4)append_string_to_dstr(&log_str,"[WARN] ^bash shell on bash shell^",'\n',0);
			left=right+1;
		}
		if(write_arg_index)
			append_string_to_dstr(&log_str,"[WARN] Unexpected end of string!",'\n',0);
	}
	else if(is_both_string_equal(tmp->data,"\"--\""))
	{
		steam_index=index;
		arg_index=launch_command.length;
		append_string_to_dstr(&log_str,"\n===[Steam Argument]===",'\n',0);
	}
	return;
}
void write_argument_to_file(FILE*f,int index)
{
	if(index>=steam_index)return;
	argvx[index].data[argvx[index].length-2]='\0';
	fprintf(f,"%s",argvx[index].data+1);
	argvx[index].data[argvx[index].length-2]='\"';
	return;
}
int injector_path_index;dstr injector_path;
void init_injector_path()
{
	append_string_to_dstr(&log_str,"\n===[injector path]===",'\n',0);
	append_string_to_dstr(&log_str,"Detecting injector path...",'\n',0);
	int i,j;
	for(j=argvx[0].length-1;j--;)
		if((argvx[0].data[j]=='\\'&&argvx[0].data[j+1]!='\"')||argvx[0].data[j]=='/')
			break;
	for(i=-1;i++<j;)append_char_to_dstr(&injector_path,argvx[0].data[i],0);
	if(!injector_path.data)append_char_to_dstr(&injector_path,'\"',0);
	injector_path_index=injector_path.length;
	append_char_to_dstr(&injector_path,'\0',0);
	append_string_to_dstr(&log_str,"injector folder location:",' ',0);
	append_string_to_dstr(&log_str,injector_path.data,'\"',0);
	append_string_to_dstr(&log_str,"!",'\n',0);
	return;
}
void create_full_file_injector_path(char *filename)
{
	injector_path.length=injector_path_index;
	while(*filename)append_char_to_dstr(&injector_path,*filename++,0);
	append_char_to_dstr(&injector_path,'\"',0);
	append_char_to_dstr(&injector_path,'\0',0);
	return;
}
FILE* open_file_on_injector_path(char *filename,char *mode)
{
	FILE *f;
	create_full_file_injector_path(filename);
	injector_path.data[injector_path.length-2]='\0';
	f=fopen(injector_path.data+1,mode);
	injector_path.data[injector_path.length-2]='\"';
	if(f==NULL&&*mode!='r')
	{
		append_string_to_dstr(&log_str,"[ERROR] Cannot write to",' ',0);
		append_string_to_dstr(&log_str,injector_path.data,'!',0);
		append_string_to_dstr(&log_str," Make sure you have write permission on",' ',0);
		append_string_to_dstr(&log_str,"that folder and make sure you have enough",' ',0);
		append_string_to_dstr(&log_str,"disk space to write the file to the disk!",'\n',0);
	}
	return f;
}
void check_an_injector(FILE *f,char *injector_name,dstr *arg)
{
	FILE *ff;int i;
	ff=open_file_on_injector_path(injector_name,"r");
	if(ff!=NULL)
	{
		fclose(ff);
		fprintf(f,"start \"\" \"%s\"",injector_name);
		for(i=-1;++i<arg->length;)fprintf(f," \"%%%c\"",arg->data[i]);
		fprintf(f,"\n");
		append_string_to_dstr(&log_str,"found",' ',0);
		append_string_to_dstr(&log_str,injector_name,' ',1);
		append_string_to_dstr(&log_str,"on injector path!",'\n',0);
	}
	else
	{
		append_string_to_dstr(&log_str,injector_name,' ',1);
		append_string_to_dstr(&log_str,"not found!",'\n',0);
	}
	return;
}
void load_injector_list(FILE *f,char*config_name)
{
	FILE*ff;int cur,max_arg;dstr arg,exe;
	ff=open_file_on_injector_path(config_name,"r");
	append_string_to_dstr(&log_str,"Opening config file:",' ',0);
	append_string_to_dstr(&log_str,config_name,'\n',1);
	if(ff==NULL)
	{
		append_string_to_dstr(&log_str,"Config file not found, generating new one!",'\n',0);
		ff=open_file_on_injector_path(config_name,"w");
		fprintf(ff,"#cmd inject version: %s\n",CMD_INJECT_VERSION);
		fprintf(ff,"#note: you can list your custom injector here to be auto detected\n");
		fprintf(ff,"#format: \"<injector exe>\" <arg_idx_a> <arg_idx_b> ... <arg_idx_n>\n");
		fprintf(ff,"#arg_idx: is a number between \'1\' and \'9\' inclusive\n");
		fprintf(ff,"# you could use \'0\' too as a valid arg_idx but\n");
		fprintf(ff,"# it will be this program name (full path) and useless in most cases.\n");
		fprintf(ff,"# for now you can add only up to 9 distinct arguments to the injector\n");
		fprintf(ff,"# let me know if you want more arguments by posting issue on github\n");
		fprintf(ff,"# if you have reasonable exuse I might add this support in the future\n");
		fprintf(ff,"# but IMHO 9 distinct arguments is a lot already!\n");
		fprintf(ff,"# you could also edit \"config.bat\" to whatever you like\n");
		fprintf(ff,"# don't forget to remove the first commented line on \"config.bat\"!\n");
		fprintf(ff,"# so that config.bat is not overwritten by this program\n");
		fprintf(ff,"\"inject64.exe\" 1\n");
		fprintf(ff,"\"inject32.exe\" 1\n");
		fprintf(ff,"\"3DMigoto Loader.exe\"\n");
		fclose(ff);
		ff=open_file_on_injector_path(config_name,"r");
	}
	else append_string_to_dstr(&log_str,"Config file found!",'\n',0);
	arg.data=NULL;arg.length=arg.alloc=0;
	exe.data=NULL;exe.length=exe.alloc=0;
	for(cur=fgetc(ff);cur!=EOF;)
	{
		if((char)cur=='#')while((cur=fgetc(ff))!=EOF&&(char)cur!='\r'&&(char)cur!='\n');
		else if((char)cur=='\"')
		{
			exe.length=arg.length=0;
			while((cur=fgetc(ff))!=EOF&&(char)cur!='\"'&&(char)cur!='\r'&&(char)cur!='\n')
				append_char_to_dstr(&exe,(char)cur,0);
			append_char_to_dstr(&exe,'\0',0);
			if(cur=='\"')
			{
				max_arg=0;
				while((cur=fgetc(ff))!=EOF&&(char)cur!='\r'&&(char)cur!='\n')
					if('0'<=(char)cur&&(char)cur<='9')
					{
						append_char_to_dstr(&arg,(char)cur,0);
						if((cur&15)>max_arg)max_arg=cur&15;
					}
				if(max_arg<steam_index)check_an_injector(f,exe.data,&arg);
				else
				{
					append_string_to_dstr(&log_str,"[WARN] detection of",' ',0);
					append_string_to_dstr(&log_str,exe.data,' ',1);
					append_string_to_dstr(&log_str,"is skipped because number of",' ',0);
					append_string_to_dstr(&log_str,"program arguments needed ",'(',0);
					sprintf(buffer,"%d) is exceeding number of given arguments ",max_arg);
					append_string_to_dstr(&log_str,buffer,'(',0);
					sprintf(buffer,"%d)!",steam_index-1);
					append_string_to_dstr(&log_str,buffer,'\n',0);
				}
			}
		}
		while((cur=fgetc(ff))!=EOF)if(cur=='#'||cur=='\"')break;
	}
	append_string_to_dstr(&log_str,"Done detecting all injectors in the list!",'\n',0);
	if(arg.data)free(arg.data);
	if(exe.data)free(exe.data);
	return;
}
FILE* generate_bat_config(char*bat_filename)
{
	FILE *f;
	f=open_file_on_injector_path(bat_filename,"w");
	fprintf(f,"rem ove this line to make this config permanent! ");
	fprintf(f,"cmd_inject version: %s\n",CMD_INJECT_VERSION);
	fprintf(f,"pushd %%~dp0\n");
	fprintf(f,"rem ember to put your injetor and its arguments between pushd and popd\n");
	load_injector_list(f,"injecttor_list.conf");
	fprintf(f,"popd");
	fclose(f);
	return open_file_on_injector_path(bat_filename,"r");
}
int main(int argc,char**argv)
{
	//receiving argumnents parameter and initializing the program
	int i,j,ret=-1;FILE *f,*ff;argv0=argv[0];
	sprintf(buffer,"cmd_inject version: %s",CMD_INJECT_VERSION);
	append_string_to_dstr(&log_str,buffer,'\n',0);
	append_string_to_dstr(&log_str,"\n===[Injector Argument]===",'\n',0);
	for(i=-1;++i<argc;)append_argument(argv[i]);
	init_injector_path();
	//creating or parsing "config.bat"
	f=open_file_on_injector_path("config.bat","r");
	if(f==NULL)
	{
		append_string_to_dstr(&log_str,"\n===[Config Bat Log]===",'\n',0);
		f=generate_bat_config("config.bat");
	}
	else
	{
		j=fscanf(f,"%8c",buffer);buffer[8]='\0';
		if(is_both_string_equal(buffer,"rem ove "))
		{
			append_string_to_dstr(&log_str,"\n===[Config Bat Log]===",'\n',0);
			append_string_to_dstr(&log_str,"Overwriting \"config.bat\" file!",'\n',0);
			fclose(f);
			f=generate_bat_config("config.bat");
		}
	}
	//generating "launcher.bat"
	ff=open_file_on_injector_path("launcher.bat","w");
	for(fseek(f,0,SEEK_SET);(i=fgetc(f))!=EOF;fputc(i,ff))if('%'==(char)i)
	{
		j=fgetc(f);
		if('0'<=(char)j&&(char)j<='9')
		{
			i=fgetc(f);
			if((char)i<=' '||(char)i=='\"')write_argument_to_file(ff,j&15);
			else{fputc('%',ff);fputc(j,ff);}
		}
		else{fputc('%',ff);i=j;}
	}
	fclose(f);
	if(launch_command.data!=NULL)launch_command.data[--launch_command.length]='\0';
	else
	{
		append_string_to_dstr(&launch_command,"\"",'\"',0);
		append_string_to_dstr(&log_str,"[WARN] launch command is empty! Make sure",' ',0);
		append_string_to_dstr(&log_str,"you have put \"--\" before %command%!",'\n',0);
	}
	convert_critical_argument_to_windows_format();
	fprintf(ff,"\nstart \"\" %s\n",launch_command.data+arg_index);
	fclose(ff);
	//injecting launcher.bat to launch command
	launch_command.length=arg_index;
	create_full_file_injector_path("launcher.bat");
	if(arg_index_inside_quotes)
		injector_path.data[injector_path.length-2]=injector_path.data[0]='\'';
	append_string_to_dstr(&launch_command,injector_path.data,'\0',0);
	if(extra_command.data)
	{
		--launch_command.length;
		append_string_to_dstr(&launch_command,extra_command.data,'\0',0);
		free(extra_command.data);
	}
	append_string_to_dstr(&log_str,"\n===[Launch Command]===",'\n',0);
	append_string_to_dstr(&log_str,launch_command.data,'\0',0);
	//cleaning up, freeing memory, and launching injected launch command
	if(argvx)
	{
		for(i=-1;++i<argix;)if(argvx[i].data)free(argvx[i].data);
		free(argvx);
	}
	if(injector_path.data)free(injector_path.data);
	if(log_str.data)free(log_str.data);
	ret=system(launch_command.data);
	free(launch_command.data);
	return ret;
}
