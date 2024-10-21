/* cmd_inject: Command injector for both Steam Windows and Steam Linux
 *             plus some other launcher with editable launch command
 * Original project on Github: https://github.com/Stereo-3D/cmd_inject
 * Copyleft 🄯 2024 Tjandra Satria Gunawan (discord: tjandra, email: tjandra@yandex.com)
 * License: GNU GENERAL PUBLIC LICENSE
 *          Version 3, 29 June 2007
 *          https://www.gnu.org/licenses/gpl-3.0.en.html
 * Non complete list of term inside GPL v3 ^for more info visit the link above^:
 *   Permissions:
 *     - Anyone can copy, modify and distribute this software.
 *     - You can use this software privately.
 *     - You can use this software for commercial purposes.
 *   Conditions:
 *     - License and copyright notice: You have to include the license and copyright notice
 *                                     with each and every distribution.
 *     - State changes: If you modify it, you have to indicate changes made to the code.
 *     - Same license: Any modifications of this code base MUST be distributed
 *                     with the same license, GPLv3.
 *     - Disclose source: Source code must be made available
 *                        when the licensed material is distributed.
 *   Limitations:
 *     - Warranty: This software is provided without warranty.
 *     - Liability: The software author or license can not be held liable for
 *                  any damages inflicted by the software.
 */
#include<stdio.h>
#include<stdlib.h>
#define CMD_INJECT_VERSION "v0.2.3"
int is_both_string_equal(char*a,char*b)//with this no need to include string.h anymore!
{
	for(;*a!='\0'&&*b!='\0';)if(*a++!=*b++)return 0;
	return*a==*b;
}
typedef struct Dynamic_String{char*data;int length,alloc;}dstr;
int check_if_str_is_prefix_of_dstr(dstr*str,int str_index,char*keyword)
{
	while(*keyword!='\0'&&str_index<str->length)
		if(str->data[str_index++]!=*keyword++)return 0;
	return*keyword=='\0';
}
int check_for_special_program_keyword(dstr*str,int left_index,int right_index)
{
	//generating data backup before doing check operation
	char backup_char,left_backup,right_backup;
	if(str->data[left_index]==str->data[right_index]&&
		(str->data[left_index]=='\''||str->data[left_index]=='\"'))
			left_backup=right_backup=str->data[left_index];
	else if(right_index-left_index>2&&
		check_if_str_is_prefix_of_dstr(str,left_index,"\\\"")&&
		check_if_str_is_prefix_of_dstr(str,right_index-1,"\\\""))
	{
		left_backup=str->data[++left_index];
		right_backup=str->data[--right_index];
	}
	else
	{
		left_backup=str->data[--left_index];
		right_backup=str->data[++right_index];
	}
	//change the data format to make checking process easier
	str->data[left_index]=str->data[right_index]='\"';
	backup_char=str->data[right_index+1];
	str->data[right_index+1]='\0';
	str->data+=left_index;
	str->length-=left_index;
	//begin checking
	int satisfy=0,i,ret_code=0;
	if(is_both_string_equal(str->data,"\"waitforexitandrun\""))
	{
		ret_code=1;//proton's magic keyword detected
		goto restore_data_and_return;
	}
	for(i=right_index-left_index+1;i--;)if(str->data[i]=='\\'||str->data[i]=='/')break;
	if(i<0)i=0;
	if(check_if_str_is_prefix_of_dstr(str,++i,"proton")){i+=6;satisfy=1;}//proton binary
	else if(check_if_str_is_prefix_of_dstr(str,i,"wine")){i+=4;satisfy=1;}//wine binary
	if(!satisfy)
	{
		//linux shell binary (so many of them)! this list may not a complete list
		if(check_if_str_is_prefix_of_dstr(str,i,"bash")||
			check_if_str_is_prefix_of_dstr(str,i,"fish")||
			check_if_str_is_prefix_of_dstr(str,i,"tcsh")||
			check_if_str_is_prefix_of_dstr(str,i,"dash")){i+=4;satisfy=1;}
		else if(check_if_str_is_prefix_of_dstr(str,i,"csh")||
			check_if_str_is_prefix_of_dstr(str,i,"ksh")||
			check_if_str_is_prefix_of_dstr(str,i,"zsh")){i+=3;satisfy=1;}
		else if(check_if_str_is_prefix_of_dstr(str,i,"sh")){i+=2;satisfy=1;}
		if(satisfy&&str->data[i]=='\"')ret_code=4;//linux shell binary detected
		goto restore_data_and_return;
	}
	//suffix like 32 and 64 is also allowed
	if(check_if_str_is_prefix_of_dstr(str,i,"32")||
		check_if_str_is_prefix_of_dstr(str,i,"64"))i+=2;
	//in case if there is wine or proton exe version (unlikely)
	if(check_if_str_is_prefix_of_dstr(str,i,".exe"))i+=4;
	if(str->data[i]=='\"')ret_code=2;//wine or proton binary detected
	//end of check time to restore the data to original
	restore_data_and_return:;
	str->data-=left_index;
	str->length+=left_index;
	str->data[left_index]=left_backup;
	str->data[right_index]=right_backup;
	str->data[right_index+1]=backup_char;
	return ret_code;
}
dstr log_str;FILE*log_file;char*argv0;
void append_char_to_dstr(dstr*str,char c,int escape_special)//and log file IO management
{
	if(str==&log_str&&log_file==NULL)
	{
		//because log file is an emergency it need to be prioritized! (have own safe code)
		int i,j;char*log_file_path;for(i=-1;argv0[++i];);
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
		free(log_file_path);
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
void append_string_to_dstr(dstr*str,char*string,char end_symbol,int write_quotes)
{
	if(write_quotes)append_char_to_dstr(str,'\"',0);
	while(*string)append_char_to_dstr(str,*string++,write_quotes);
	if(write_quotes)append_char_to_dstr(str,'\"',0);
	append_char_to_dstr(str,end_symbol,0);
	return;
}
dstr launch_command,extra_command;int arg_index,arg_index_inside_quotes;
void convert_critical_argument_to_windows_format()//and copy arguments to corresponding var
{
	//Initializing the converter
	append_string_to_dstr(&log_str,"\n===[Argument Converter]===",'\n',0);
	char c=launch_command.data[arg_index],terminate_symbol=' ';
	int i=0,j,converting=1,escape=0;
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
	//copying main game argument (and converting it to windows format if required)
	append_char_to_dstr(&game_arg,'\"',0);
	if(converting)
	{
		if(launch_command.data[arg_index+i]=='/')append_string_to_dstr(&game_arg,"Z",':',0);
		if(check_if_str_is_prefix_of_dstr(&launch_command,arg_index+i,"./"))i+=2;
	}
	append_string_to_dstr(&log_str,"Current game argument:",' ',0);
	for(j=-1;++j<i;)append_char_to_dstr(&log_str,launch_command.data[arg_index+j],0);
	while((c=launch_command.data[arg_index+i++])!=terminate_symbol)
	{
		if(arg_index_inside_quotes&&!(escape&1)&&c=='\"')break;
		append_char_to_dstr(&log_str,c,0);
		if(c=='/'&&converting)append_char_to_dstr(&game_arg,'\\',1);
		else append_char_to_dstr(&game_arg,c,0);
		if(c=='\\')++escape;
		else escape=0;
	}
	//finalizing the data for main game argument
	if(terminate_symbol==' ')--i;
	append_char_to_dstr(&log_str,terminate_symbol,0);
	append_char_to_dstr(&log_str,'\n',0);
	if(arg_index_inside_quotes&&(escape&1)&&c=='\"')game_arg.data[game_arg.length-1]=c;
	else append_char_to_dstr(&game_arg,'\"',0);
	if(converting)
	{
		append_char_to_dstr(&game_arg,'\0',0);
		append_string_to_dstr(&log_str,"New game argument:",' ',0);
		append_string_to_dstr(&log_str,game_arg.data,'\n',0);
		--game_arg.length;
	}
	//copy the remaining argument for game argument
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
	//copy the remaining argument outside of linux shell if exist
	for(;c!='\0';c=launch_command.data[arg_index+i++])append_char_to_dstr(&extra_command,c,0);
	//finalizing the data for remaining variables
	append_char_to_dstr(&extra_command,'\0',0);
	append_char_to_dstr(&game_arg,'\0',0);
	launch_command.length=arg_index;
	append_string_to_dstr(&launch_command,game_arg.data,'\0',0);
	if(game_arg.data)free(game_arg.data);
	append_string_to_dstr(&log_str,"Done converting game argument!",'\n',0);
	return;
}
dstr*argvx;char buffer[128];
int steam_index,argix,arglx,nested_shell_detected;
void append_argument(char*value)//also parse and identify each arguments on the fly
{
	dstr*str;int index,i,left=launch_command.length,right,end,write_arg_index=0;char c;
	sprintf(buffer,"Argument[%d]:",argix);
	append_string_to_dstr(&log_str,buffer,' ',0);
	if(arglx==argix)argvx=(dstr*)realloc(argvx,(arglx=arglx<<1|1)*sizeof(dstr));
	str=&argvx[index=argix++];
	str->data=NULL;
	str->length=str->alloc=0;
	append_string_to_dstr(str,value,'\0',1);
	append_string_to_dstr(&log_str,value,'\n',1);
	if(steam_index)//the remaining command after "--"
	{
		append_string_to_dstr(&launch_command,value,' ',1);
		if(!nested_shell_detected)//not inside child linux shell
		{
			//do the argument check
			i=check_for_special_program_keyword(str,0,str->length-2);
			if(i&1)//proton magic keyword detected
			{
				arg_index=launch_command.length;
				append_string_to_dstr(&log_str,"\n===[Game Argument]===",'\n',0);
			}
			if(i&2)//wine or proton executable detected
			{
				arg_index=launch_command.length;
				append_string_to_dstr(&log_str,"^wine or proton arg above^",'\n',0);
			}
			if(i&4)//linux shell detected
			{
				nested_shell_detected=1;
				append_string_to_dstr(&log_str,"^bash shell fork above^",'\n',0);
			}
			return;
		}
		for(end=launch_command.length-2;left<end;)//scan the nested command (for linux shell)
		{
			if(launch_command.data[++left]<=' ')continue;
			if(write_arg_index){arg_index=left;write_arg_index=0;arg_index_inside_quotes=1;}
			//getting a token (sub argument) inside linux shell
			if(check_if_str_is_prefix_of_dstr(&launch_command,left,"\\\""))
				{c='\"';right=left+1;}
			else if(launch_command.data[left]=='\''){c='\'';right=left;}
			else{c=' ';right=left;}
			while(launch_command.data[++right]!=c)if(right>=end){right=end-1;break;}
			if(launch_command.data[right]==' '&&c==' ')--right;
			//print the sub argument to the log
			append_string_to_dstr(&log_str,"  --> sub argument:",' ',0);
			c=launch_command.data[right+1];
			launch_command.data[right+1]='\0';
			append_string_to_dstr(&log_str,launch_command.data+left,'\n',0);
			launch_command.data[right+1]=c;
			//do the sub argument check
			i=check_for_special_program_keyword(&launch_command,left,right);
			if(i&1)//proton magic keyword detected
			{
				write_arg_index=1;//the arg_index cannot be determined yet
				append_string_to_dstr(&log_str,"^proton magic keyword above^",'\n',0);
			}
			if(i&2)//wine or proton executable detected
			{
				write_arg_index=1;//the arg_index cannot be determined yet
				append_string_to_dstr(&log_str,"^wine or proton arg above^",'\n',0);
			}
			if(i&4)//If some launcher has this in practice, that launcher is very evil! :(
				append_string_to_dstr(&log_str,"[WARN] ^bash shell on bash shell^",'\n',0);
			left=right+1;
		}
		if(write_arg_index)//unable to properly parse linux shell argument (bad input?)
			append_string_to_dstr(&log_str,"[WARN] Unexpected end of string!",'\n',0);
	}
	else if(is_both_string_equal(str->data,"\"--\""))//this program arguments before "--"
	{
		steam_index=index;
		arg_index=launch_command.length;
		append_string_to_dstr(&log_str,"\n===[Steam Argument]===",'\n',0);
	}
	return;
}
void write_argument_to_file(FILE*f,int index)//write arg received by this program to file
{
	if(index>=steam_index)return;//for safety reasons, any arguments after "--" is not allowed
	argvx[index].data[argvx[index].length-2]='\0';
	fprintf(f,"%s",argvx[index].data+1);
	argvx[index].data[argvx[index].length-2]='\"';
	return;
}
/* Begin code for custom file I/O management (until "End of custom file I/O code" comment)
 * This is needed because using fopen directly will access the file from random directory:
 *  it could be launcer directory, steam directory, game directory or even injector directory
 * This custom I/O let the program to access the file under injector directory:
 *  the folder where the cmd_inject executable is located
 */
int injector_path_index;dstr injector_path;
void init_injector_path()//find the path of cmd_inject and save it
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
void create_full_file_injector_path(char*filename)//construct full path of filename
{
	injector_path.length=injector_path_index;
	while(*filename)append_char_to_dstr(&injector_path,*filename++,0);
	append_char_to_dstr(&injector_path,'\"',0);
	append_char_to_dstr(&injector_path,'\0',0);
	return;
}
FILE* open_file_on_injector_path(char*filename,char*mode)//open on the same path as cmd_inject
{
	FILE*f;
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
//End of custom file I/O code
void check_an_injector(FILE*f,char*injector_name,dstr*arg)//check any progam not just injector
{
	FILE*ff;int i;char c;
	ff=open_file_on_injector_path(injector_name,"r");
	append_string_to_dstr(&log_str,"  -->",' ',0);
	if(ff!=NULL)
	{
		fclose(ff);
		fprintf(f,"start \"\" \"%s\"",injector_name);
		for(i=-1;++i<arg->length;)//write the corresponding program arguments to config.bat
		{
			if(arg->data[i]<'0'||'9'<arg->data[i])continue;
			fprintf(f," \"%%");
			for(--i;++i<arg->length&&'0'<=(c=arg->data[i])&&c<='9';)fputc(c,f);
			fputc('\"',f);
		}
		fputc('\n',f);
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
void load_injector_list(FILE*f,char*config_name)//can parse & auto generate the injector list
{
	FILE*ff;int cur,max_arg,value;dstr arg,exe;
	ff=open_file_on_injector_path(config_name,"r");
	append_string_to_dstr(&log_str,"Opening config file:",' ',0);
	append_string_to_dstr(&log_str,config_name,'\n',1);
	if(ff==NULL)
	{
		append_string_to_dstr(&log_str,"Config file not found, generating new one!",'\n',0);
		generate_injector_list_conf:;//generate the list
		ff=open_file_on_injector_path(config_name,"w");
		fprintf(ff,"#cmd inject version: %s\n",CMD_INJECT_VERSION);
		fprintf(ff,"#source code: https://github.com/Stereo-3D/cmd_inject\n");
		fprintf(ff,"#note: you can list your custom injector here to be auto detected!\n");
		fprintf(ff,"#      don't forget to remove the first line if you want to prevent'\n");
		fprintf(ff,"#      this file overwritten by the program (keep changes permanent)'\n");
		fprintf(ff,"#format: \"<injector exe>\" <arg_idx_a> <arg_idx_b> ... <arg_idx_n>\n");
		fprintf(ff,"#        don't forget to put quote your program name when editing!'\n");
		fprintf(ff,"#arg_idx: is a non negative integer but useful arguments start with 1\n");
		fprintf(ff,"#  if arg_idx is equal to zero (0) that contain this program name \n");
		fprintf(ff,"#  and it's fullpath which is useless for the injector arguments'\n");
		fprintf(ff,"#  remember you could also edit \"config.bat\" to whatever you like\n");
		fprintf(ff,"#  don't forget to remove the first commented line on \"config.bat\"!\n");
		fprintf(ff,"#  so that your changes made to \"config.bat\" is kept permanent!\n");
		fprintf(ff,"#  in other word to prevent \"config.bat\" overwritten by the program\n");
		fprintf(ff,"\"inject64.exe\" 1\n");
		fprintf(ff,"\"inject32.exe\" 1\n");
		fprintf(ff,"\"3DMigoto Loader.exe\"\n");
		fclose(ff);
		ff=open_file_on_injector_path(config_name,"r");
	}
	else
	{
		append_string_to_dstr(&log_str,"Config file found!",'\n',0);
		cur=fscanf(ff,"%20c",buffer);buffer[20]='\0';//cheack if the file is autogenerated
		if(is_both_string_equal(buffer,"#cmd inject version:"))
		{
			append_string_to_dstr(&log_str,"Config file seems to be auto generated!",' ',0);
			append_string_to_dstr(&log_str,"Replacing it with new one...",'\n',0);
			fclose(ff);
			goto generate_injector_list_conf;
		}
	}
	arg.data=NULL;arg.length=arg.alloc=0;
	exe.data=NULL;exe.length=exe.alloc=0;
	append_string_to_dstr(&log_str,"Detecting injectors:",'\n',0);
	for(fseek(ff,0,SEEK_SET),cur=fgetc(ff);cur!=EOF;)//parsing the entire lists
	{
		if((char)cur=='#')while((cur=fgetc(ff))!=EOF&&(char)cur!='\r'&&(char)cur!='\n');
		else if((char)cur=='\"')//valid line detected
		{
			exe.length=arg.length=0;
			while((cur=fgetc(ff))!=EOF&&(char)cur!='\"'&&(char)cur!='\r'&&(char)cur!='\n')
				append_char_to_dstr(&exe,(char)cur,0);//copy the exe name
			append_char_to_dstr(&exe,'\0',0);
			if(cur=='\"')
			{
				max_arg=0;
				while((cur=fgetc(ff))!=EOF&&(char)cur!='\r'&&(char)cur!='\n')//parse the arg
				{
					if((char)cur<'0'||'9'<(char)cur)continue;
					for(value=0;'0'<=(char)cur&&(char)cur<='9';cur=fgetc(ff))
					{
						value=10*value+(cur&15);
						append_char_to_dstr(&arg,(char)cur,0);//copy the arg digit
					}
					if(value>max_arg)max_arg=value;
					append_char_to_dstr(&arg,',',0);//separate the arg (separator symbol: ',')
					if(cur==EOF||(char)cur=='\r'||(char)cur=='\n')break;
				}
				//finalize arg data
				if(!arg.length)*arg.data='\0';
				else arg.data[arg.length-1]='\0';
				if(max_arg<steam_index)check_an_injector(f,exe.data,&arg);//all args is valid
				else
				{
					append_string_to_dstr(&log_str,"  --> [WARN] detection of",' ',0);
					append_string_to_dstr(&log_str,exe.data,' ',1);
					append_string_to_dstr(&log_str,"is skipped because number of",' ',0);
					append_string_to_dstr(&log_str,"program arguments needed ",'(',0);
					sprintf(buffer,"%d) is exceeding number of given arguments ",max_arg);
					append_string_to_dstr(&log_str,buffer,'(',0);
					sprintf(buffer,"%d)!",steam_index-1);
					append_string_to_dstr(&log_str,buffer,'\n',0);
				}
				//print all args to log file (useful for debuging)
				append_string_to_dstr(&log_str,"      Full argument index",' ',0);
				append_string_to_dstr(&log_str,"list for that program: ",'[',0);
				append_string_to_dstr(&log_str,arg.data,']',0);
				append_char_to_dstr(&log_str,'\n',0);
			}
		}
		while((cur=fgetc(ff))!=EOF)if(cur=='#'||cur=='\"')break;//#for comment, \ for progname
	}
	append_string_to_dstr(&log_str,"Done detecting all injectors in the list!",'\n',0);
	if(arg.data)free(arg.data);
	if(exe.data)free(exe.data);
	return;
}
FILE* generate_bat_config(char*bat_filename)//return FILE pointer to generated config.bat
{
	/* Note that "config.bat" is not meant to execute directly because it only contain partial
	 *  bat command that will be processed further to create "launcher.bat"!
	 * It will not support receiving argument because the argument variables is different than
	 *  the standard bat script, the argument variables will be replaced by the exact argument
	 *  this program receive when generating "launcher.bat"!
	*/
	FILE*f;
	f=open_file_on_injector_path(bat_filename,"w");
	fprintf(f,"rem ove this line to make this config permanent! ");
	fprintf(f,"cmd_inject version: %s\n",CMD_INJECT_VERSION);
	fprintf(f,"rem ind that cmd_inject source code is available on github: ");
	fprintf(f,"https://github.com/Stereo-3D/cmd_inject\n");
	fprintf(f,"pushd %%~dp0\n");//bat command to change the directory to where config.bat is
	fprintf(f,"rem ember to put your injetor and its arguments between pushd and popd\n");
	load_injector_list(f,"injecttor_list.conf");
	fprintf(f,"popd");//bat command to go back to previous directory you are in before pushd
	fclose(f);
	return open_file_on_injector_path(bat_filename,"r");
}
int main(int argc,char**argv)
{
	//receiving argumnents parameter and initializing the program
	int i,j,ret=-1;FILE*f,*ff;argv0=argv[0];
	sprintf(buffer,"cmd_inject version: %s\nSource code:",CMD_INJECT_VERSION);
	append_string_to_dstr(&log_str,buffer,' ',0);
	append_string_to_dstr(&log_str,"https://github.com/Stereo-3D/cmd_inject",'\n',0);
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
		j=fscanf(f,"%8c",buffer);buffer[8]='\0';//cheack if the file is autogenerated
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
			for(i=j,j=0;'0'<=(char)i&&(char)i<='9';i=fgetc(f))j=10*j+(i&15);
			if((char)i<=' '||(char)i=='\"')write_argument_to_file(ff,j);
			else fprintf(ff,"%%%d",j);
		}
		else{fputc('%',ff);i=j;}
		if(i==EOF)break;
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
	append_string_to_dstr(&launch_command,injector_path.data,'?',0);
	--launch_command.length;//remove the extra '?' symbol from launch_command
	append_string_to_dstr(&launch_command,extra_command.data,'\0',0);
	append_string_to_dstr(&log_str,"\n===[Launch Command]===",'\n',0);
	append_string_to_dstr(&log_str,launch_command.data,'\0',0);
	//cleaning up and freeing memory
	for(i=-1;++i<argix;)free(argvx[i].data);
	free(argvx);free(extra_command.data);free(injector_path.data);free(log_str.data);
	ret=system(launch_command.data);//launching injected launch command
	free(launch_command.data);
	return ret;
}
